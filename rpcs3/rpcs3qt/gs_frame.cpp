#include "gs_frame.h"
#include "gui_settings.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Utilities/date_time.h"
#include "Utilities/File.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/Modules/cellScreenshot.h"
#include "Emu/RSX/rsx_utils.h"

#include <QCoreApplication>
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
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#endif
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif
#endif

#ifdef _WIN32
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

LOG_CHANNEL(screenshot_log, "SCREENSHOT");
LOG_CHANNEL(mark_log, "MARK");

extern atomic_t<bool> g_user_asked_for_frame_capture;

constexpr auto qstr = QString::fromStdString;

gs_frame::gs_frame(QScreen* screen, const QRect& geometry, const QIcon& appIcon, std::shared_ptr<gui_settings> gui_settings)
	: QWindow()
	, m_initial_geometry(geometry)
	, m_gui_settings(std::move(gui_settings))
{
	m_disable_mouse = m_gui_settings->GetValue(gui::gs_disableMouse).toBool();
	m_disable_kb_hotkeys = m_gui_settings->GetValue(gui::gs_disableKbHotkeys).toBool();
	m_show_mouse_in_fullscreen = m_gui_settings->GetValue(gui::gs_showMouseFs).toBool();
	m_hide_mouse_after_idletime = m_gui_settings->GetValue(gui::gs_hideMouseIdle).toBool();
	m_hide_mouse_idletime = m_gui_settings->GetValue(gui::gs_hideMouseIdleTime).toUInt();

	m_window_title = Emu.GetFormattedTitle(0);

	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

#ifdef __APPLE__
	// Needed for MoltenVK to work properly on MacOS
	if (g_cfg.video.renderer == video_renderer::vulkan)
		setSurfaceType(QSurface::VulkanSurface);
#endif

	setMinimumWidth(160);
	setMinimumHeight(90);
	setScreen(screen);
	setGeometry(geometry);
	setTitle(qstr(m_window_title));
	setVisibility(Hidden);
	create();

	// Change cursor when in fullscreen.
	connect(this, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility)
	{
		handle_cursor(visibility, true, true);
	});

	// Change cursor when this window gets or loses focus.
	connect(this, &QWindow::activeChanged, this, [this]()
	{
		handle_cursor(visibility(), false, true);
	});

	// Configure the mouse hide on idle timer
	connect(&m_mousehide_timer, &QTimer::timeout, this, &gs_frame::MouseHideTimeout);
	m_mousehide_timer.setSingleShot(true);

#ifdef _WIN32
	m_tb_button = new QWinTaskbarButton();
	m_tb_progress = m_tb_button->progress();
	m_tb_progress->setRange(0, m_gauge_max);
	m_tb_progress->setVisible(false);

#elif HAVE_QTDBUS
	UpdateProgress(0);
	m_progress_value = 0;
#endif
}

gs_frame::~gs_frame()
{
#ifdef _WIN32
	// QWinTaskbarProgress::hide() will crash if the application is already about to close, even if the object is not null.
	if (m_tb_progress && !QCoreApplication::closingDown())
	{
		m_tb_progress->hide();
	}
	if (m_tb_button)
	{
		m_tb_button->deleteLater();
	}
#elif HAVE_QTDBUS
	UpdateProgress(0, false);
#endif
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

void gs_frame::keyPressEvent(QKeyEvent *keyEvent)
{
	if (keyEvent->isAutoRepeat())
	{
		keyEvent->ignore();
		return;
	}

	// NOTE: needs to be updated with keyboard_pad_handler::processKeyEvent

	switch (keyEvent->key())
	{
	case Qt::Key_L:
		if (keyEvent->modifiers() == Qt::AltModifier)
		{
			static int count = 0;
			mark_log.success("Made forced mark %d in log", ++count);
			return;
		}
		else if (keyEvent->modifiers() == Qt::ControlModifier)
		{
			toggle_mouselock();
			return;
		}
		break;
	case Qt::Key_Return:
		if (keyEvent->modifiers() == Qt::AltModifier)
		{
			toggle_fullscreen();
			return;
		}
		break;
	case Qt::Key_Escape:
		if (visibility() == FullScreen)
		{
			toggle_fullscreen();
			return;
		}
		break;
	case Qt::Key_P:
		if (keyEvent->modifiers() == Qt::ControlModifier && !m_disable_kb_hotkeys && Emu.IsRunning())
		{
			Emu.Pause();
			return;
		}
		break;
	case Qt::Key_R:
		if (keyEvent->modifiers() == Qt::ControlModifier && !m_disable_kb_hotkeys)
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
			default: break;
			}
		}
		break;
	case Qt::Key_C:
		if (keyEvent->modifiers() == Qt::AltModifier && !m_disable_kb_hotkeys)
		{
			g_user_asked_for_frame_capture = true;
			return;
		}
		break;
	case Qt::Key_F12:
		screenshot_toggle = true;
		break;
	default:
		break;
	}
}

void gs_frame::toggle_fullscreen()
{
	Emu.CallAfter([this]()
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

void gs_frame::toggle_mouselock()
{
	// first we toggle the value
	m_mouse_hide_and_lock = !m_mouse_hide_and_lock;

	// and update the cursor
	handle_cursor(visibility(), false, true);
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
	handle_cursor(visibility(), false, true);

	return isActive() && m_mouse_hide_and_lock;
}

void gs_frame::close()
{
	Emu.CallAfter([this]()
	{
		QWindow::hide(); // Workaround

		if (!Emu.IsStopped())
		{
			Emu.Stop();
		}

		deleteLater();
	});
}

bool gs_frame::shown()
{
	return QWindow::isVisible();
}

void gs_frame::hide()
{
	Emu.CallAfter([this]() { QWindow::hide(); });
}

void gs_frame::show()
{
	Emu.CallAfter([this]()
	{
		QWindow::show();
		if (g_cfg.misc.start_fullscreen)
		{
			setVisibility(FullScreen);
		}
	});

#ifdef _WIN32
	// if we do this before show, the QWinTaskbarProgress won't show
	if (m_tb_button)
	{
		m_tb_button->setWindow(this);
		m_tb_progress->show();
	}
#endif
}

display_handle_t gs_frame::handle() const
{
#ifdef _WIN32
	return reinterpret_cast<HWND>(this->winId());
#elif defined(__APPLE__)
	return reinterpret_cast<void*>(this->winId()); //NSView
#else
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
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
	{
#endif
#ifdef HAVE_X11
		return std::make_pair(XOpenDisplay(0), static_cast<ulong>(this->winId()));
#else
		fmt::throw_exception("Vulkan X11 support disabled at compile-time.");
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	}
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

double gs_frame::client_device_pixel_ratio() const
{
	return devicePixelRatio();
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

			Emu.CallAfter([this, title = std::move(new_title)]()
			{
				setTitle(qstr(title));
			});
		}

		m_frames = 0;
		fps_t.Start();
	}
}

void gs_frame::take_screenshot(std::vector<u8> data, const u32 sshot_width, const u32 sshot_height, bool is_bgra)
{
	std::thread(
		[sshot_width, sshot_height, is_bgra](std::vector<u8> sshot_data)
		{
			screenshot_log.notice("Taking screenshot (%dx%d)", sshot_width, sshot_height);

			std::string screen_path = fs::get_config_dir() + "screenshots/";

			if (!fs::create_dir(screen_path) && fs::g_tls_error != fs::error::exist)
			{
				screenshot_log.error("Failed to create screenshot path \"%s\" : %s", screen_path, fs::g_tls_error);
				return;
			}

			const std::string filename = screen_path + "screenshot-" + date_time::current_time_narrow<'_'>() + ".png";

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

			std::vector<u8*> rows(sshot_height);
			for (usz y = 0; y < sshot_height; y++)
				rows[y] = sshot_data_alpha.data() + y * sshot_width * 4;

			std::vector<u8> encoded_png;

			const auto write_png = [&]()
			{
				const scoped_png_ptrs ptrs;
				png_set_IHDR(ptrs.write_ptr, ptrs.info_ptr, sshot_width, sshot_height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
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

					if (!overlay_img.load(qstr(cell_sshot_overlay_path)))
					{
						screenshot_log.error("Failed to read cell screenshot overlay '%s' : %s", cell_sshot_overlay_path, fs::g_tls_error);
						return;
					}

					// Games choose the overlay file and the offset based on the current video resolution.
					// We need to scale the overlay if our resolution scaling causes the image to have a different size.
					auto& avconf = g_fxo->get<rsx::avconf>();

					// TODO: handle wacky PS3 resolutions (without resolution scaling)
					if (avconf.resolution_x != sshot_width || avconf.resolution_y != sshot_height)
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

					if (manager.overlay_offset_x < static_cast<s64>(sshot_width) &&
					    manager.overlay_offset_y < static_cast<s64>(sshot_height) &&
					    manager.overlay_offset_x + overlay_img.width() > 0 &&
					    manager.overlay_offset_y + overlay_img.height() > 0)
					{
						QImage screenshot_img(rows[0], sshot_width, sshot_height, QImage::Format_RGBA8888);
						QPainter painter(&screenshot_img);
						painter.drawImage(manager.overlay_offset_x, manager.overlay_offset_y, overlay_img);

						std::memcpy(rows[0], screenshot_img.constBits(), static_cast<usz>(sshot_height) * screenshot_img.bytesPerLine());

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

			return;
		},
		std::move(data))
		.detach();
}

void gs_frame::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (m_disable_mouse || g_cfg.io.move == move_handler::mouse) return;

	if (ev->button() == Qt::LeftButton)
	{
		toggle_fullscreen();
	}
}

void gs_frame::handle_cursor(QWindow::Visibility visibility, bool from_event, bool start_idle_timer)
{
	// Update the mouse lock state if the visibility changed.
	if (from_event)
	{
		// In fullscreen we default to hiding and locking. In windowed mode we do not want the lock by default.
		m_mouse_hide_and_lock = visibility == QWindow::Visibility::FullScreen;
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

void gs_frame::MouseHideTimeout()
{
	// Our idle timeout occured, so we update the cursor
	if (m_hide_mouse_after_idletime && m_show_mouse)
	{
		handle_cursor(visibility(), false, false);
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
			atomic_t<bool> called = false;

			Emu.CallAfter([this, &result, &called]()
			{
				m_gui_settings->ShowConfirmationBox(tr("Exit Game?"),
					tr("Do you really want to exit the game?\n\nAny unsaved progress will be lost!\n"),
					gui::ib_confirm_exit, &result, nullptr);

				called = true;
				called.notify_one();
			});

			called.wait(false);

			if (result != QMessageBox::Yes)
			{
				return true;
			}
		}
		close();
	}
	else if (ev->type() == QEvent::MouseMove && (!m_show_mouse || m_mousehide_timer.isActive()))
	{
		// This will make the cursor visible again if it was hidden by the mouse idle timeout
		handle_cursor(visibility(), false, true);
	}
	return QWindow::event(ev);
}

void gs_frame::progress_reset(bool reset_limit)
{
#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->reset();
	}
#elif HAVE_QTDBUS
	UpdateProgress(0);
#endif

	if (reset_limit)
	{
		progress_set_limit(100);
	}
}

void gs_frame::progress_set_value(int value)
{
#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->setValue(std::clamp(value, m_tb_progress->minimum(), m_tb_progress->maximum()));
	}
#elif HAVE_QTDBUS
	m_progress_value = std::clamp(value, 0, m_gauge_max);
	UpdateProgress(m_progress_value);
#endif
}

void gs_frame::progress_increment(int delta)
{
	if (delta == 0)
	{
		return;
	}

#ifdef _WIN32
	if (m_tb_progress)
	{
		progress_set_value(m_tb_progress->value() + delta);
	}
#elif HAVE_QTDBUS
	progress_set_value(m_progress_value + delta);
#endif
}

void gs_frame::progress_set_limit(int limit)
{
#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->setMaximum(limit);
	}
#elif HAVE_QTDBUS
	m_gauge_max = limit;
#endif
}

#ifdef HAVE_QTDBUS
void gs_frame::UpdateProgress(int progress, bool disable)
{
	QDBusMessage message = QDBusMessage::createSignal
	(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update")
	);
	QVariantMap properties;
	if (disable)
		properties.insert(QStringLiteral("progress-visible"), false);
	else
		properties.insert(QStringLiteral("progress-visible"), true);
	//Progress takes a value from 0.0 to 0.1
	properties.insert(QStringLiteral("progress"), 1. * progress / m_gauge_max);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
