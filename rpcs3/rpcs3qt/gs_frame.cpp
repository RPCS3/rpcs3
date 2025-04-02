#include "gs_frame.h"
#include "gui_settings.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Utilities/date_time.h"
#include "Utilities/File.h"
#include "util/video_provider.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_progress.hpp"
#include "Emu/IdManager.h"
#include "Emu/Audio/audio_utils.h"
#include "Emu/Cell/Modules/cellScreenshot.h"
#include "Emu/Cell/Modules/cellAudio.h"
#include "Emu/Cell/lv2/sys_rsxaudio.h"
#include "Emu/RSX/rsx_utils.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/Io/interception.h"
#include "Emu/Io/recording_config.h"

#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>

#include <string>
#include <thread>

#include "png.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
//nothing
#else
#ifdef HAVE_WAYLAND
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#endif
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif
#endif

LOG_CHANNEL(screenshot_log, "SCREENSHOT");
LOG_CHANNEL(mark_log, "MARK");
LOG_CHANNEL(gui_log, "GUI");

extern atomic_t<bool> g_user_asked_for_recording;
extern atomic_t<bool> g_user_asked_for_screenshot;
extern atomic_t<bool> g_user_asked_for_frame_capture;
extern atomic_t<bool> g_disable_frame_limit;
extern atomic_t<bool> g_game_window_focused;
extern atomic_t<recording_mode> g_recording_mode;

namespace pad
{
	extern atomic_t<bool> g_home_menu_requested;
}

gs_frame::gs_frame(QScreen* screen, const QRect& geometry, const QIcon& appIcon, std::shared_ptr<gui_settings> gui_settings, bool force_fullscreen)
	: QWindow()
	, m_initial_geometry(geometry)
	, m_gui_settings(std::move(gui_settings))
	, m_start_games_fullscreen(force_fullscreen)
	, m_renderer(g_cfg.video.renderer)
{
	m_window_title = Emu.GetFormattedTitle(0);

	if (!g_cfg_recording.load())
	{
		gui_log.notice("Could not load recording config. Using defaults.");
	}

	g_fxo->need<utils::video_provider>();
	m_video_encoder = std::make_shared<utils::video_encoder>();

	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

#ifdef __APPLE__
	// Needed for MoltenVK to work properly on MacOS
	if (g_cfg.video.renderer == video_renderer::vulkan)
		setSurfaceType(QSurface::VulkanSurface);
#endif

	// NOTE: You cannot safely create a wayland window that has hidden initial status and perform any changes on the window while it is still hidden.
	// Doing this will create a surface with deferred commands that require a buffer. When binding to your session, this may assert in your compositor due to protocol restrictions.
	Visibility startup_visibility = Hidden;
#ifndef _WIN32
	if (const char* session_type = ::getenv("XDG_SESSION_TYPE"))
	{
		if (!strcasecmp(session_type, "wayland"))
		{
			// Start windowed. This is a featureless rectangle on-screen with no window decorations.
			// It does not even resemble a window until the WM attaches later on.
			// Fullscreen could technically work with some fiddling, but easily breaks depending on geometry input.
			startup_visibility = Windowed;
		}
	}
#endif

	setMinimumWidth(160);
	setMinimumHeight(90);
	setScreen(screen);
	setGeometry(geometry);
	setTitle(QString::fromStdString(m_window_title));

	if (g_cfg.video.renderer != video_renderer::opengl)
	{
		// Do not display the window before OpenGL is configured!
		// This works fine in windows and X11 but wayland-egl will crash later.
		setVisibility(startup_visibility);
		create();
	}

	load_gui_settings();

	// Change cursor when in fullscreen.
	connect(this, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility)
	{
		handle_cursor(visibility, true, false, true);
	});

	// Change cursor when this window gets or loses focus.
	connect(this, &QWindow::activeChanged, this, [this]()
	{
		g_game_window_focused = isActive();
		handle_cursor(visibility(), false, true, true);
	});

	// Configure the mouse hide on idle timer
	connect(&m_mousehide_timer, &QTimer::timeout, this, &gs_frame::mouse_hide_timeout);
	m_mousehide_timer.setSingleShot(true);

	m_progress_indicator = std::make_unique<progress_indicator>(0, 100);
}

gs_frame::~gs_frame()
{
	g_user_asked_for_screenshot = false;
	pad::g_home_menu_requested = false;

	// Save active screen to gui settings
	const QScreen* current_screen = screen();
	const QList<QScreen*> screens = QGuiApplication::screens();
	int screen_index = 0;

	for (int i = 0; i < screens.count(); i++)
	{
		if (current_screen == ::at32(screens, i))
		{
			screen_index = i;
			break;
		}
	}

	m_gui_settings->SetValue(gui::gs_screen, screen_index);
}

void gs_frame::load_gui_settings()
{
	m_disable_mouse = m_gui_settings->GetValue(gui::gs_disableMouse).toBool();
	m_disable_kb_hotkeys = m_gui_settings->GetValue(gui::gs_disableKbHotkeys).toBool();
	m_show_mouse_in_fullscreen = m_gui_settings->GetValue(gui::gs_showMouseFs).toBool();
	m_lock_mouse_in_fullscreen  = m_gui_settings->GetValue(gui::gs_lockMouseFs).toBool();
	m_hide_mouse_after_idletime = m_gui_settings->GetValue(gui::gs_hideMouseIdle).toBool();
	m_hide_mouse_idletime = m_gui_settings->GetValue(gui::gs_hideMouseIdleTime).toUInt();

	if (m_disable_kb_hotkeys)
	{
		if (m_shortcut_handler)
		{
			m_shortcut_handler->deleteLater();
			m_shortcut_handler = nullptr;
		}
	}
	else if (!m_shortcut_handler)
	{
		m_shortcut_handler = new shortcut_handler(gui::shortcuts::shortcut_handler_id::game_window, this, m_gui_settings);
		connect(m_shortcut_handler, &shortcut_handler::shortcut_activated, this, &gs_frame::handle_shortcut);
	}
}

void gs_frame::update_shortcuts()
{
	if (m_shortcut_handler)
	{
		m_shortcut_handler->update();
	}
}

void gs_frame::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
}

void gs_frame::showEvent(QShowEvent *event)
{
	// We have to calculate new window positions, since the frame is only known once the window was created.
	// We will try to find the originally requested dimensions if possible by moving the frame.

	// NOTES: The parameter m_initial_geometry is not necessarily equal to our actual geometry() at this point.
	//        That's why we use m_initial_geometry instead of the frameGeometry() in some places.
	//        All of these values, including the screen geometry, can also be negative numbers.

	const QRect available_geometry = screen()->availableGeometry(); // The available screen geometry
	const QRect inner_geometry = geometry();      // The current inner geometry
	const QRect outer_geometry = frameGeometry(); // The current outer geometry

	// Calculate the left and top frame borders (this will include window handles)
	const int left_border = inner_geometry.left() - outer_geometry.left();
	const int top_border = inner_geometry.top() - outer_geometry.top();

	// Calculate the initially expected frame origin
	const QPoint expected_pos(m_initial_geometry.left() - left_border,
	                          m_initial_geometry.top() - top_border);

	// Make sure that the expected position is inside the screen (check left and top borders)
	QPoint pos(std::max(expected_pos.x(), available_geometry.left()),
	           std::max(expected_pos.y(), available_geometry.top()));

	// Find the maximum position that still ensures that the frame is completely visible inside the screen (check right and bottom borders)
	QPoint max_pos(available_geometry.left() + available_geometry.width() - frameGeometry().width(),
	               available_geometry.top() + available_geometry.height() - frameGeometry().height());

	// Make sure that the "maximum" position is inside the screen (check left and top borders)
	max_pos.setX(std::max(max_pos.x(), available_geometry.left()));
	max_pos.setY(std::max(max_pos.y(), available_geometry.top()));

	// Adjust the expected position accordingly
	pos.setX(std::min(pos.x(), max_pos.x()));
	pos.setY(std::min(pos.y(), max_pos.y()));

	// Set the new position
	setFramePosition(pos);

	QWindow::showEvent(event);
}

void gs_frame::handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence)
{
	gui_log.notice("Game window registered shortcut: %s (%s)", shortcut_key, key_sequence.toString());

	if (m_disable_kb_hotkeys)
	{
		return;
	}

	switch (shortcut_key)
	{
	case gui::shortcuts::shortcut::gw_toggle_fullscreen:
	{
		toggle_fullscreen();
		break;
	}
	case gui::shortcuts::shortcut::gw_exit_fullscreen:
	{
		if (visibility() == FullScreen)
		{
			toggle_fullscreen();
		}
		break;
	}
	case gui::shortcuts::shortcut::gw_log_mark:
	{
		static int count = 0;
		mark_log.success("Made forced mark %d in log", ++count);
		break;
	}
	case gui::shortcuts::shortcut::gw_mouse_lock:
	{
		toggle_mouselock();
		break;
	}
	case gui::shortcuts::shortcut::gw_screenshot:
	{
		g_user_asked_for_screenshot = true;
		break;
	}
	case gui::shortcuts::shortcut::gw_toggle_recording:
	{
		toggle_recording();
		break;
	}
	case gui::shortcuts::shortcut::gw_pause_play:
	{
		switch (Emu.GetStatus())
		{
		case system_state::ready:
		{
			Emu.Run(true);
			return;
		}
		case system_state::paused:
		{
			Emu.Resume();
			return;
		}
		default:
		{
			Emu.Pause();
			return;
		}
		}
		break;
	}
	case gui::shortcuts::shortcut::gw_restart:
	{
		if (Emu.IsStopped())
		{
			Emu.Restart();
			return;
		}

		extern bool boot_last_savestate(bool testing);
		boot_last_savestate(false);
		break;
	}
	case gui::shortcuts::shortcut::gw_savestate:
	{
		if (!g_cfg.savestate.suspend_emu)
		{
			Emu.after_kill_callback = []()
			{
				Emu.Restart();
			};

			// Make sure we keep the game window opened
			Emu.SetContinuousMode(true);
		}

		Emu.Kill(false, true);
		return;
	}
	case gui::shortcuts::shortcut::gw_rsx_capture:
	{
		g_user_asked_for_frame_capture = true;
		break;
	}
	case gui::shortcuts::shortcut::gw_frame_limit:
	{
		g_disable_frame_limit = !g_disable_frame_limit;
		gui_log.warning("%s boost mode", g_disable_frame_limit.load() ? "Enabled" : "Disabled");
		break;
	}
	case gui::shortcuts::shortcut::gw_toggle_mouse_and_keyboard:
	{
		input::toggle_mouse_and_keyboard();
		break;
	}
	case gui::shortcuts::shortcut::gw_home_menu:
	{
		pad::g_home_menu_requested = true;
		break;
	}
	case gui::shortcuts::shortcut::gw_mute_unmute:
	{
		audio::toggle_mute();
		break;
	}
	case gui::shortcuts::shortcut::gw_volume_up:
	{
		audio::change_volume(5);
		break;
	}
	case gui::shortcuts::shortcut::gw_volume_down:
	{
		audio::change_volume(-5);
		break;
	}
	default:
	{
		break;
	}
	}
}

void gs_frame::toggle_fullscreen()
{
	Emu.CallFromMainThread([this]()
	{
		if (visibility() == FullScreen)
		{
			// Change to the last recorded visibility. Sanitize it just in case.
			if (m_last_visibility != Visibility::Maximized && m_last_visibility != Visibility::Windowed)
			{
				m_last_visibility = Visibility::Windowed;
			}
			setVisibility(m_last_visibility);
		}
		else
		{
			// Backup visibility for exiting fullscreen mode later. Don't do this in the visibilityChanged slot,
			// since entering fullscreen from maximized will first change the visibility to windowed.
			m_last_visibility = visibility();
			setVisibility(FullScreen);
		}
	});
}

void gs_frame::toggle_recording()
{
	utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();

	if (g_recording_mode == recording_mode::cell)
	{
		gui_log.warning("A video recorder is already in use by cell. Regular recording can not proceed.");
		m_video_encoder->stop();
		return;
	}

	if (g_recording_mode.exchange(recording_mode::stopped) == recording_mode::rpcs3)
	{
		m_video_encoder->stop();

		if (!video_provider.set_video_sink(nullptr, recording_mode::rpcs3))
		{
			gui_log.warning("The video provider could not release the video sink. A sink with higher priority must have been set.");
		}

		// Play a sound
		if (const std::string sound_path = fs::get_config_dir() + "sounds/snd_recording.wav"; fs::is_file(sound_path))
		{
			Emu.GetCallbacks().play_sound(sound_path);
		}
		else
		{
			QApplication::beep();
		}

		ensure(m_video_encoder->path().starts_with(fs::get_config_dir()));
		const std::string shortpath = m_video_encoder->path().substr(fs::get_config_dir().size() - 1); // -1 for /
		rsx::overlays::queue_message(tr("Recording saved: %0").arg(QString::fromStdString(shortpath)).toStdString());
	}
	else
	{
		m_video_encoder->stop();

		const std::string& id = Emu.GetTitleID();
		std::string video_path = fs::get_config_dir() + "recordings/";
		if (!id.empty())
		{
			video_path += id + "/";
		}

		if (!fs::create_path(video_path) && fs::g_tls_error != fs::error::exist)
		{
			screenshot_log.error("Failed to create recordings path \"%s\" : %s", video_path, fs::g_tls_error);
			return;
		}

		if (!id.empty())
		{
			video_path += id + "_";
		}

		video_path += "recording_" + date_time::current_time_narrow<'_'>() + ".mp4";

		utils::video_encoder::frame_format output_format{};
		output_format.av_pixel_format = static_cast<AVPixelFormat>(g_cfg_recording.video.pixel_format.get());
		output_format.width = g_cfg_recording.video.width;
		output_format.height = g_cfg_recording.video.height;
		output_format.pitch = g_cfg_recording.video.width * 4;

		m_video_encoder->use_internal_audio = true;
		m_video_encoder->use_internal_video = true;
		m_video_encoder->set_path(video_path);
		m_video_encoder->set_framerate(g_cfg_recording.video.framerate);
		m_video_encoder->set_video_bitrate(g_cfg_recording.video.video_bps);
		m_video_encoder->set_video_codec(g_cfg_recording.video.video_codec);
		m_video_encoder->set_max_b_frames(g_cfg_recording.video.max_b_frames);
		m_video_encoder->set_gop_size(g_cfg_recording.video.gop_size);
		m_video_encoder->set_output_format(output_format);

		switch (g_cfg.audio.provider)
		{
		case audio_provider::none:
		{
			// Disable audio recording
			m_video_encoder->use_internal_audio = false;
			break;
		}
		case audio_provider::cell_audio:
		{
			const cell_audio_config& cfg = g_fxo->get<cell_audio>().cfg;
			m_video_encoder->set_sample_rate(cfg.audio_sampling_rate);
			m_video_encoder->set_audio_channels(cfg.audio_channels);
			break;
		}
		case audio_provider::rsxaudio:
		{
			const auto& rsx_audio = g_fxo->get<rsx_audio_backend>();
			m_video_encoder->set_sample_rate(rsx_audio.get_sample_rate());
			m_video_encoder->set_audio_channels(rsx_audio.get_channel_count());
			break;
		}
		}

		m_video_encoder->set_audio_bitrate(g_cfg_recording.audio.audio_bps);
		m_video_encoder->set_audio_codec(g_cfg_recording.audio.audio_codec);
		m_video_encoder->encode();

		if (m_video_encoder->has_error)
		{
			rsx::overlays::queue_message(tr("Recording not possible").toStdString());
			m_video_encoder->stop();
			return;
		}

		if (!video_provider.set_video_sink(m_video_encoder, recording_mode::rpcs3))
		{
			gui_log.warning("The video provider could not set the video sink. A sink with higher priority must have been set.");
			rsx::overlays::queue_message(tr("Recording not possible").toStdString());
			m_video_encoder->stop();
			return;
		}

		video_provider.set_pause_time_us(0);

		g_recording_mode = recording_mode::rpcs3;

		rsx::overlays::queue_message(tr("Recording started").toStdString());
	}
}

void gs_frame::toggle_mouselock()
{
	// first we toggle the value
	m_mouse_hide_and_lock = !m_mouse_hide_and_lock;

	// and update the cursor
	handle_cursor(visibility(), false, false, true);
}

void gs_frame::update_cursor()
{
	bool show_mouse;

	if (!isActive())
	{
		// Show the mouse by default if this is not the active window
		show_mouse = true;
	}
	else if (m_hide_mouse_after_idletime && !m_mousehide_timer.isActive())
	{
		// Hide the mouse if the idle timeout was reached (which means that the timer isn't running)
		show_mouse = false;
	}
	else if (visibility() == QWindow::Visibility::FullScreen)
	{
		// Fullscreen: Show or hide the mouse depending on the settings.
		show_mouse = m_show_mouse_in_fullscreen;
	}
	else
	{
		// Windowed: Hide the mouse if it was locked by the user
		show_mouse = !m_mouse_hide_and_lock;
	}

	// Update Cursor if necessary
	if (show_mouse != m_show_mouse.exchange(show_mouse))
	{
		setCursor(m_show_mouse ? Qt::ArrowCursor : Qt::BlankCursor);
	}
}

bool gs_frame::get_mouse_lock_state()
{
	handle_cursor(visibility(), false, false, true);

	return isActive() && m_mouse_hide_and_lock;
}

void gs_frame::hide_on_close()
{
	// Make sure not to save the hidden state, which is useless to us.
	const Visibility current_visibility = visibility();
	m_gui_settings->SetValue(gui::gs_visibility, current_visibility == Visibility::Hidden ? Visibility::AutomaticVisibility : current_visibility, false);
	m_gui_settings->SetValue(gui::gs_geometry, geometry(), true);

	if (!g_progr_text)
	{
		// Hide the dialog before stopping if no progress bar is being shown.
		// Otherwise users might think that the game softlocked if stopping takes too long.
		QWindow::hide();
	}
}

void gs_frame::close()
{
	if (m_is_closing.exchange(true))
	{
		gui_log.notice("Closing game window (ignored, already closing)");
		return;
	}

	gui_log.notice("Closing game window");

	if (m_ignore_stop_events)
	{
		return;
	}

	Emu.CallFromMainThread([this]()
	{
		// Hide window if necessary
		hide_on_close();

		if (!Emu.IsStopped())
		{
			// Notify progress dialog cancellation
			g_system_progress_canceled = true;

			// Blocking shutdown request. Obsolete, but I'm keeping it here as last resort.
			Emu.after_kill_callback = [this](){ deleteLater(); };
			Emu.GracefulShutdown(true);
		}
		else
		{
			deleteLater();
		}
	});
}

void gs_frame::reset()
{
}

bool gs_frame::shown()
{
	return QWindow::isVisible();
}

void gs_frame::hide()
{
	Emu.CallFromMainThread([this]() { QWindow::hide(); });
}

void gs_frame::show()
{
	Emu.CallFromMainThread([this]()
	{
		QWindow::show();

		if (g_cfg.misc.start_fullscreen || m_start_games_fullscreen)
		{
			setVisibility(FullScreen);
		}
		else if (const QVariant var = m_gui_settings->GetValue(gui::gs_visibility); var.canConvert<Visibility>())
		{
			// Restore saved visibility from last time. Make sure not to hide the window, or the user can't access it anymore.
			if (const Visibility visibility = var.value<Visibility>(); visibility != Visibility::Hidden)
			{
				setVisibility(visibility);
			}
		}
	});
}

display_handle_t gs_frame::handle() const
{
#ifdef _WIN32
	return reinterpret_cast<HWND>(this->winId());
#elif defined(__APPLE__)
	return reinterpret_cast<void*>(this->winId()); //NSView
#else
#ifdef HAVE_WAYLAND
	QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
	struct wl_display *wl_dpy = static_cast<struct wl_display *>(
		native->nativeResourceForWindow("display", NULL));
	struct wl_surface *wl_surf = static_cast<struct wl_surface *>(
		native->nativeResourceForWindow("surface", const_cast<QWindow*>(static_cast<const QWindow*>(this))));
	if (wl_dpy != nullptr && wl_surf != nullptr)
	{
		return std::make_pair(wl_dpy, wl_surf);
	}
	else
#endif
#ifdef HAVE_X11
	{
		return std::make_pair(XOpenDisplay(0), static_cast<ulong>(this->winId()));
	}
#else
		fmt::throw_exception("Vulkan X11 support disabled at compile-time.");
#endif
#endif
}

draw_context_t gs_frame::make_context()
{
	return nullptr;
}

void gs_frame::set_current(draw_context_t context)
{
	Q_UNUSED(context)
}

void gs_frame::delete_context(draw_context_t context)
{
	Q_UNUSED(context)
}

int gs_frame::client_width()
{
#ifdef _WIN32
	RECT rect;
	if (GetClientRect(reinterpret_cast<HWND>(winId()), &rect))
	{
		return rect.right - rect.left;
	}
#endif // _WIN32
	return width() * devicePixelRatio();
}

int gs_frame::client_height()
{
#ifdef _WIN32
	RECT rect;
	if (GetClientRect(reinterpret_cast<HWND>(winId()), &rect))
	{
		return rect.bottom - rect.top;
	}
#endif // _WIN32
	return height() * devicePixelRatio();
}

f64 gs_frame::client_display_rate()
{
	f64 rate = 20.; // Minimum is 20

	Emu.BlockingCallFromMainThread([this, &rate]()
	{
		const QList<QScreen*> screens = QGuiApplication::screens();

		for (int i = 0; i < screens.count(); i++)
		{
			rate = std::fmax(rate, ::at32(screens, i)->refreshRate());
		}
	});

	return rate;
}

void gs_frame::flip(draw_context_t, bool /*skip_frame*/)
{
	static Timer fps_t;

	if (!m_flip_showed_frame)
	{
		// Show on first flip
		m_flip_showed_frame = true;
		show();
		fps_t.Start();
	}

	++m_frames;

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		std::string new_title = Emu.GetFormattedTitle(m_frames / fps_t.GetElapsedTimeInSec());

		if (new_title != m_window_title)
		{
			m_window_title = new_title;

			Emu.CallFromMainThread([this, title = std::move(new_title)]()
			{
				setTitle(QString::fromStdString(title));
			});
		}

		m_frames = 0;
		fps_t.Start();
	}

	if (g_user_asked_for_recording.exchange(false))
	{
		Emu.CallFromMainThread([this]()
		{
			toggle_recording();
		});
	}
}

bool gs_frame::can_consume_frame() const
{
	utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
	return video_provider.can_consume_frame();
}

void gs_frame::present_frame(std::vector<u8>& data, u32 pitch, u32 width, u32 height, bool is_bgra) const
{
	utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
	video_provider.present_frame(data, pitch, width, height, is_bgra);
}

void gs_frame::take_screenshot(std::vector<u8>&& data, u32 sshot_width, u32 sshot_height, bool is_bgra)
{
	std::thread(
		[sshot_width, sshot_height, is_bgra](std::vector<u8> sshot_data)
		{
			thread_base::set_name("Screenshot");

			screenshot_log.notice("Taking screenshot (%dx%d)", sshot_width, sshot_height);

			const std::string& id = Emu.GetTitleID();
			std::string screen_path = fs::get_config_dir() + "screenshots/";
			if (!id.empty())
			{
				screen_path += id + "/";
			}

			if (!fs::create_path(screen_path) && fs::g_tls_error != fs::error::exist)
			{
				screenshot_log.error("Failed to create screenshot path \"%s\" : %s", screen_path, fs::g_tls_error);
				return;
			}

			std::string filename = screen_path;
			if (!id.empty())
			{
				filename += id + "_";
			}

			filename += "screenshot_" + date_time::current_time_narrow<'_'>() + ".png";

			fs::file sshot_file(filename, fs::open_mode::create + fs::open_mode::write + fs::open_mode::excl);
			if (!sshot_file)
			{
				screenshot_log.error("Failed to save screenshot \"%s\" : %s", filename, fs::g_tls_error);
				return;
			}

			std::vector<u8> sshot_data_alpha(sshot_data.size());
			const u32* sshot_ptr = reinterpret_cast<const u32*>(sshot_data.data());
			u32* alpha_ptr       = reinterpret_cast<u32*>(sshot_data_alpha.data());

			if (is_bgra) [[likely]]
			{
				for (usz index = 0; index < sshot_data.size() / sizeof(u32); index++)
				{
					alpha_ptr[index] = ((sshot_ptr[index] & 0xFF) << 16) | (sshot_ptr[index] & 0xFF00) | ((sshot_ptr[index] & 0xFF0000) >> 16) | 0xFF000000;
				}
			}
			else
			{
				for (usz index = 0; index < sshot_data.size() / sizeof(u32); index++)
				{
					alpha_ptr[index] = sshot_ptr[index] | 0xFF000000;
				}
			}

			screenshot_info manager;
			{
				auto& s = g_fxo->get<screenshot_manager>();
				std::lock_guard lock(s.mutex);
				manager = s;
			}

			struct scoped_png_ptrs
			{
				png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
				png_infop info_ptr    = png_create_info_struct(write_ptr);

				~scoped_png_ptrs()
				{
					png_free_data(write_ptr, info_ptr, PNG_FREE_ALL, -1);
					png_destroy_write_struct(&write_ptr, &info_ptr);
				}
			};

			png_text text[6] = {};
			int num_text = 0;

			const QDateTime date_time       = QDateTime::currentDateTime();
			const std::string creation_time = date_time.toString("yyyy:MM:dd hh:mm:ss").toStdString();
			const std::string photo_title   = manager.get_photo_title();
			const std::string game_title    = manager.get_game_title();
			const std::string game_comment  = manager.get_game_comment();
			const std::string& title_id     = Emu.GetTitleID();

			// Write tEXt chunk
			text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
			text[num_text].key         = const_cast<char*>("Creation Time");
			text[num_text].text        = const_cast<char*>(creation_time.c_str());
			++num_text;
			text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
			text[num_text].key         = const_cast<char*>("Source");
			text[num_text].text        = const_cast<char*>("RPCS3"); // Originally PlayStation(R)3
			++num_text;
			text[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
			text[num_text].key         = const_cast<char*>("Title ID");
			text[num_text].text        = const_cast<char*>(title_id.c_str());
			++num_text;

			// Write tTXt chunk (they probably meant zTXt)
			text[num_text].compression = PNG_TEXT_COMPRESSION_zTXt;
			text[num_text].key         = const_cast<char*>("Title");
			text[num_text].text        = const_cast<char*>(photo_title.c_str());
			++num_text;
			text[num_text].compression = PNG_TEXT_COMPRESSION_zTXt;
			text[num_text].key         = const_cast<char*>("Game Title");
			text[num_text].text        = const_cast<char*>(game_title.c_str());
			++num_text;
			text[num_text].compression = PNG_TEXT_COMPRESSION_zTXt;
			text[num_text].key         = const_cast<char*>("Comment");
			text[num_text].text        = const_cast<char*>(game_comment.c_str());

			// Create image from data
			QImage img(sshot_data_alpha.data(), sshot_width, sshot_height, sshot_width * 4, QImage::Format_RGBA8888);

			// Scale image if necessary
			const auto& avconf = g_fxo->get<rsx::avconf>();
			auto new_size = avconf.aspect_convert_dimensions(size2u{ u32(img.width()), u32(img.height()) });

			if (new_size.width != static_cast<u32>(img.width()) || new_size.height != static_cast<u32>(img.height()))
			{
				img = img.scaled(QSize(new_size.width, new_size.height), Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
				img.convertTo(QImage::Format_RGBA8888); // The current Qt version changes the image format during smooth scaling, so we have to change it back.
			}

			// Create row pointers for libpng
			std::vector<u8*> rows(img.height());
			for (int y = 0; y < img.height(); y++)
				rows[y] = img.scanLine(y);

			std::vector<u8> encoded_png;

			const auto write_png = [&]()
			{
				const scoped_png_ptrs ptrs;
				png_set_IHDR(ptrs.write_ptr, ptrs.info_ptr, img.width(), img.height(), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				png_set_text(ptrs.write_ptr, ptrs.info_ptr, text, 6);
				png_set_rows(ptrs.write_ptr, ptrs.info_ptr, &rows[0]);
				png_set_write_fn(ptrs.write_ptr, &encoded_png, [](png_structp png_ptr, png_bytep data, png_size_t length)
					{
						std::vector<u8>* p = static_cast<std::vector<u8>*>(png_get_io_ptr(png_ptr));
						p->insert(p->end(), data, data + length);
					}, nullptr);
				png_write_png(ptrs.write_ptr, ptrs.info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
			};

			write_png();
			sshot_file.write(encoded_png.data(), encoded_png.size());

			screenshot_log.success("Successfully saved screenshot to %s", filename);

			if (manager.is_enabled)
			{
				const std::string cell_sshot_overlay_path = manager.get_overlay_path();
				if (fs::is_file(cell_sshot_overlay_path))
				{
					screenshot_log.notice("Adding overlay to cell screenshot from %s", cell_sshot_overlay_path);

					QImage overlay_img;

					if (!overlay_img.load(QString::fromStdString(cell_sshot_overlay_path)))
					{
						screenshot_log.error("Failed to read cell screenshot overlay '%s' : %s", cell_sshot_overlay_path, fs::g_tls_error);
						return;
					}

					// Games choose the overlay file and the offset based on the current video resolution.
					// We need to scale the overlay if our resolution scaling causes the image to have a different size.

					// Scale the resolution first (as seen before with the image)
					new_size = avconf.aspect_convert_dimensions(size2u{ avconf.resolution_x, avconf.resolution_y });

					if (new_size.width != static_cast<u32>(img.width()) || new_size.height != static_cast<u32>(img.height()))
					{
						const int scale = rsx::get_resolution_scale_percent();
						const int x = (scale * manager.overlay_offset_x) / 100;
						const int y = (scale * manager.overlay_offset_y) / 100;
						const int width = (scale * overlay_img.width()) / 100;
						const int height = (scale * overlay_img.height()) / 100;

						screenshot_log.notice("Scaling overlay from %dx%d at offset (%d,%d) to %dx%d at offset (%d,%d)",
							overlay_img.width(), overlay_img.height(), manager.overlay_offset_x, manager.overlay_offset_y, width, height, x, y);

						manager.overlay_offset_x = x;
						manager.overlay_offset_y = y;
						overlay_img = overlay_img.scaled(QSize(width, height), Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
					}

					if (manager.overlay_offset_x < static_cast<s64>(img.width()) &&
					    manager.overlay_offset_y < static_cast<s64>(img.height()) &&
					    manager.overlay_offset_x + overlay_img.width() > 0 &&
					    manager.overlay_offset_y + overlay_img.height() > 0)
					{
						QImage screenshot_img(rows[0], img.width(), img.height(), QImage::Format_RGBA8888);
						QPainter painter(&screenshot_img);
						painter.drawImage(manager.overlay_offset_x, manager.overlay_offset_y, overlay_img);
						painter.end();

						std::memcpy(rows[0], screenshot_img.constBits(), screenshot_img.sizeInBytes());

						screenshot_log.success("Applied screenshot overlay '%s'", cell_sshot_overlay_path);
					}
				}

				const std::string cell_sshot_filename = manager.get_screenshot_path(date_time.toString("yyyy/MM/dd").toStdString());
				const std::string cell_sshot_dir      = fs::get_parent_dir(cell_sshot_filename);

				screenshot_log.notice("Saving cell screenshot to %s", cell_sshot_filename);

				if (!fs::create_path(cell_sshot_dir) && fs::g_tls_error != fs::error::exist)
				{
					screenshot_log.error("Failed to create cell screenshot dir \"%s\" : %s", cell_sshot_dir, fs::g_tls_error);
					return;
				}

				fs::file cell_sshot_file(cell_sshot_filename, fs::open_mode::create + fs::open_mode::write + fs::open_mode::excl);
				if (!cell_sshot_file)
				{
					screenshot_log.error("Failed to save cell screenshot \"%s\" : %s", cell_sshot_filename, fs::g_tls_error);
					return;
				}

				encoded_png.clear();
				write_png();
				cell_sshot_file.write(encoded_png.data(), encoded_png.size());

				screenshot_log.success("Successfully saved cell screenshot to %s", cell_sshot_filename);
			}

			// Play a sound
			Emu.CallFromMainThread([]()
			{
				if (const std::string sound_path = fs::get_config_dir() + "sounds/snd_screenshot.wav"; fs::is_file(sound_path))
				{
					Emu.GetCallbacks().play_sound(sound_path);
				}
				else
				{
					QApplication::beep();
				}
			});

			ensure(filename.starts_with(fs::get_config_dir()));
			const std::string shortpath = filename.substr(fs::get_config_dir().size() - 1); // -1 for /
			rsx::overlays::queue_message(tr("Screenshot saved: %0").arg(QString::fromStdString(shortpath)).toStdString());

			return;
		},
		std::move(data))
		.detach();
}

void gs_frame::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (m_disable_mouse)
	{
		return;
	}

	switch (g_cfg.io.move)
	{
	case move_handler::mouse:
	case move_handler::raw_mouse:
#ifdef HAVE_LIBEVDEV
	case move_handler::gun:
#endif
		return;
	default:
		break;
	}

	if (ev->button() == Qt::LeftButton)
	{
		toggle_fullscreen();
	}
}

void gs_frame::handle_cursor(QWindow::Visibility visibility, bool visibility_changed, bool active_changed, bool start_idle_timer)
{
	// Let's reload the gui settings when the visibility or the active window changes.
	if (visibility_changed || active_changed)
	{
		load_gui_settings();

		// Update the mouse lock state if the visibility changed.
		if (visibility_changed)
		{
			// In fullscreen we default to hiding and locking. In windowed mode we do not want the lock by default.
			m_mouse_hide_and_lock = (visibility == QWindow::Visibility::FullScreen) && m_lock_mouse_in_fullscreen;
		}
	}

	// Update the mouse hide timer
	if (m_hide_mouse_after_idletime && start_idle_timer)
	{
		m_mousehide_timer.start(m_hide_mouse_idletime);
	}
	else
	{
		m_mousehide_timer.stop();
	}

	// Update the cursor visibility
	update_cursor();
}

void gs_frame::mouse_hide_timeout()
{
	// Our idle timeout occurred, so we update the cursor
	if (m_hide_mouse_after_idletime && m_show_mouse)
	{
		handle_cursor(visibility(), false, false, false);
	}
}

bool gs_frame::event(QEvent* ev)
{
	if (ev->type() == QEvent::Close)
	{
		if (m_gui_settings->GetValue(gui::ib_confirm_exit).toBool())
		{
			if (visibility() == FullScreen)
			{
				toggle_fullscreen();
			}

			int result = QMessageBox::Yes;
			m_gui_settings->ShowConfirmationBox(tr("Exit Game?"),
				tr("Do you really want to exit the game?<br><br>Any unsaved progress will be lost!<br>"),
				gui::ib_confirm_exit, &result, nullptr);

			if (result != QMessageBox::Yes)
			{
				return true;
			}
		}

		gui_log.notice("Game window close event issued");

		if (m_ignore_stop_events)
		{
			return QWindow::event(ev);
		}

		if (Emu.IsStopped())
		{
			// This should be unreachable, but never say never. Properly close the window anyway.
			close();
		}
		else
		{
			// Notify progress dialog cancellation
			g_system_progress_canceled = true;

			// Issue async shutdown
			Emu.GracefulShutdown(true, true);

			// Hide window if necessary
			hide_on_close();

			// Do not propagate the close event. It will be closed by the rsx_thread.
			return true;
		}
	}
	else if (ev->type() == QEvent::MouseMove && (!m_show_mouse || m_mousehide_timer.isActive()))
	{
		// This will make the cursor visible again if it was hidden by the mouse idle timeout
		handle_cursor(visibility(), false, false, true);
	}
	return QWindow::event(ev);
}

void gs_frame::progress_reset(bool reset_limit)
{
	m_progress_indicator->reset();

	if (reset_limit)
	{
		progress_set_limit(100);
	}
}

void gs_frame::progress_set_value(int value)
{
	m_progress_indicator->set_value(value);
}

void gs_frame::progress_increment(int delta)
{
	if (delta != 0)
	{
		m_progress_indicator->set_value(m_progress_indicator->value() + delta);
	}
}

void gs_frame::progress_set_limit(int limit)
{
	m_progress_indicator->set_range(0, limit);
}

bool gs_frame::has_alpha()
{
	return format().hasAlpha();
}
