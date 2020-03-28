#include "gs_frame.h"
#include "gui_settings.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Utilities/date_time.h"
#include "Emu/System.h"
#include "Emu/Cell/Modules/cellScreenshot.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <string>

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
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

LOG_CHANNEL(screenshot);

constexpr auto qstr = QString::fromStdString;

gs_frame::gs_frame(const QRect& geometry, const QIcon& appIcon, const std::shared_ptr<gui_settings>& gui_settings)
	: QWindow(), m_gui_settings(gui_settings)
{
	m_disable_mouse = gui_settings->GetValue(gui::gs_disableMouse).toBool();

	m_window_title = qstr(Emu.GetFormattedTitle(0));

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
	setGeometry(geometry);
	setTitle(m_window_title);
	setVisibility(Hidden);
	create();

	// Change cursor when in fullscreen.
	connect(this, &QWindow::visibilityChanged, this, &gs_frame::HandleCursor);

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
	if (m_tb_progress)
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
	Q_UNUSED(event);
}

void gs_frame::showEvent(QShowEvent *event)
{
	// we have to calculate new window positions, since the frame is only known once the window was created
	// the left and right margins are too big on my setup for some reason yet unknown, so we'll have to ignore them
	int x = geometry().left(); //std::max(geometry().left(), frameMargins().left());
	int y = std::max(geometry().top(), frameMargins().top());

	setPosition(x, y);

	QWindow::showEvent(event);
}

void gs_frame::keyPressEvent(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_L:
		if (keyEvent->modifiers() == Qt::AltModifier) { static int count = 0; screenshot.success("Made forced mark %d in log", ++count); }
		break;
	case Qt::Key_Return:
		if (keyEvent->modifiers() == Qt::AltModifier) { toggle_fullscreen(); return; }
		break;
	case Qt::Key_Escape:
		if (visibility() == FullScreen) { toggle_fullscreen(); return; }
		break;
	case Qt::Key_P:
		if (keyEvent->modifiers() == Qt::ControlModifier && Emu.IsRunning()) { Emu.Pause(); return; }
		break;
	case Qt::Key_S:
		if (keyEvent->modifiers() == Qt::ControlModifier && (!Emu.IsStopped())) { Emu.Stop(); return; }
		break;
	case Qt::Key_R:
		if (keyEvent->modifiers() == Qt::ControlModifier && (!Emu.GetBoot().empty())) { Emu.Restart(); return; }
		break;
	case Qt::Key_E:
		if (keyEvent->modifiers() == Qt::ControlModifier)
		{
			if (Emu.IsReady()) { Emu.Run(true); return; }
			else if (Emu.IsPaused()) { Emu.Resume(); return; }
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
			setVisibility(Windowed);
		}
		else
		{
			setVisibility(FullScreen);
		}
	});
}

void gs_frame::close()
{
	Emu.Stop();
	Emu.CallAfter([this]() { deleteLater(); });
}

bool gs_frame::shown()
{
	return QWindow::isVisible();
}

void gs_frame::hide()
{
	Emu.CallAfter([this]() {QWindow::hide(); });
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

void gs_frame::set_current(draw_context_t ctx)
{
	Q_UNUSED(ctx);
}

void gs_frame::delete_context(draw_context_t ctx)
{
	Q_UNUSED(ctx);
}

int gs_frame::client_width()
{
#ifdef _WIN32
	RECT rect;
	if (GetClientRect(HWND(winId()), &rect))
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
	if (GetClientRect(HWND(winId()), &rect))
	{
		return rect.bottom - rect.top;
	}
#endif // _WIN32
	return height() * devicePixelRatio();
}

void gs_frame::flip(draw_context_t, bool /*skip_frame*/)
{
	static Timer fps_t;

	++m_frames;

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		const QString new_title = qstr(Emu.GetFormattedTitle(m_frames / fps_t.GetElapsedTimeInSec()));

		if (new_title != m_window_title)
		{
			m_window_title = new_title;

			Emu.CallAfter([this, title = std::move(new_title)]()
			{
				setTitle(title);
			});
		}

		m_frames = 0;
		fps_t.Start();
	}
}

void gs_frame::take_screenshot(const std::vector<u8> sshot_data, const u32 sshot_width, const u32 sshot_height)
{
	std::thread(
		[sshot_width, sshot_height](const std::vector<u8> sshot_data)
		{
			std::string screen_path = fs::get_config_dir() + "screenshots/";

			if (!fs::create_dir(screen_path) && fs::g_tls_error != fs::error::exist)
			{
				screenshot.error("Failed to create screenshot path \"%s\" : %s", screen_path, fs::g_tls_error);
				return;
			}

			std::string filename = screen_path + "screenshot-" + date_time::current_time_narrow<'_'>() + ".png";

			fs::file sshot_file(filename, fs::open_mode::create + fs::open_mode::write + fs::open_mode::excl);
			if (!sshot_file)
			{
				screenshot.error("[Screenshot] Failed to save screenshot \"%s\" : %s", filename, fs::g_tls_error);
				return;
			}

			std::vector<u8> sshot_data_alpha(sshot_data.size());
			const u32* sshot_ptr = reinterpret_cast<const u32*>(sshot_data.data());
			u32* alpha_ptr       = reinterpret_cast<u32*>(sshot_data_alpha.data());

			for (size_t index = 0; index < sshot_data.size() / sizeof(u32); index++)
			{
				alpha_ptr[index] = ((sshot_ptr[index] & 0xFF) << 16) | (sshot_ptr[index] & 0xFF00) | ((sshot_ptr[index] & 0xFF0000) >> 16) | 0xFF000000;
			}

			std::vector<u8> encoded_png;

			png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			png_infop info_ptr    = png_create_info_struct(write_ptr);
			png_set_IHDR(write_ptr, info_ptr, sshot_width, sshot_height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

			std::vector<u8*> rows(sshot_height);
			for (size_t y = 0; y < sshot_height; y++)
				rows[y] = sshot_data_alpha.data() + y * sshot_width * 4;

			png_set_rows(write_ptr, info_ptr, &rows[0]);
			png_set_write_fn(write_ptr, &encoded_png,
				[](png_structp png_ptr, png_bytep data, png_size_t length)
				{
					std::vector<u8>* p = static_cast<std::vector<u8>*>(png_get_io_ptr(png_ptr));
					p->insert(p->end(), data, data + length);
				},
				nullptr);

			png_write_png(write_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

			png_free_data(write_ptr, info_ptr, PNG_FREE_ALL, -1);
			png_destroy_write_struct(&write_ptr, nullptr);

			sshot_file.write(encoded_png.data(), encoded_png.size());

			screenshot.success("[Screenshot] Successfully saved screenshot to %s", filename);

			const auto fxo = g_fxo->get<screenshot_manager>();

			if (fxo->is_enabled)
			{
				const std::string cell_sshot_filename = fxo->get_screenshot_path();
				const std::string cell_sshot_dir      = fs::get_parent_dir(cell_sshot_filename);

				screenshot.notice("[Screenshot] Saving cell screenshot to %s", cell_sshot_filename);

				if (!fs::create_path(cell_sshot_dir) && fs::g_tls_error != fs::error::exist)
				{
					screenshot.error("Failed to create cell screenshot dir \"%s\" : %s", cell_sshot_dir, fs::g_tls_error);
					return;
				}

				fs::file cell_sshot_file(cell_sshot_filename, fs::open_mode::create + fs::open_mode::write + fs::open_mode::excl);
				if (!cell_sshot_file)
				{
					screenshot.error("[Screenshot] Failed to save cell screenshot \"%s\" : %s", cell_sshot_filename, fs::g_tls_error);
					return;
				}

				const std::string cell_sshot_overlay_path = fxo->get_overlay_path();
				if (fs::is_file(cell_sshot_overlay_path))
				{
					screenshot.notice("[Screenshot] Adding overlay to cell screenshot from %s", cell_sshot_overlay_path);
					// TODO: add overlay to screenshot
				}

				// TODO: add tEXt chunk with creation time, source, title id
				// TODO: add tTXt chunk with data procured from cellScreenShotSetParameter (get_photo_title, get_game_title, game_comment)

				cell_sshot_file.write(encoded_png.data(), encoded_png.size());

				screenshot.success("[Screenshot] Successfully saved cell screenshot to %s", cell_sshot_filename);
			}

			return;
		},
		std::move(sshot_data))
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

void gs_frame::HandleCursor(QWindow::Visibility visibility)
{
	if (visibility == QWindow::Visibility::FullScreen)
	{
		setCursor(Qt::BlankCursor);
	}
	else
	{
		setCursor(Qt::ArrowCursor);
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

void gs_frame::progress_increment(int delta)
{
	if (delta == 0)
	{
		return;
	}

#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->setValue(std::clamp(m_tb_progress->value() + delta, m_tb_progress->minimum(), m_tb_progress->maximum()));
	}
#elif HAVE_QTDBUS
	m_progress_value = std::clamp(m_progress_value + delta, 0, m_gauge_max);
	UpdateProgress(m_progress_value);
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
