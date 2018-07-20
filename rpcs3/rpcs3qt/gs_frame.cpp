#include "gs_frame.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Emu/System.h"

#include <QKeyEvent>
#include <QTimer>
#include <QThread>
#include <string>

#include "rpcs3_version.h"

#ifdef _WIN32
#include <windows.h>
#else
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#endif
#include <X11/Xlib.h>
#endif

constexpr auto qstr = QString::fromStdString;

gs_frame::gs_frame(const QString& title, const QRect& geometry, QIcon appIcon, bool disableMouse)
	: QWindow(), m_windowTitle(title), m_disable_mouse(disableMouse)
{
	//Get version by substringing VersionNumber-buildnumber-commithash to get just the part before the dash
	std::string version = rpcs3::version.to_string();
	version = version.substr(0 , version.find_last_of("-"));

	//Add branch and commit hash to version on frame , unless it's master.
	if ((rpcs3::get_branch().compare("master") != 0) && (rpcs3::get_branch().compare("HEAD") != 0))
	{
		version = version + "-" + rpcs3::version.to_string().substr((rpcs3::version.to_string().find_last_of("-") + 1), 8) + "-" + rpcs3::get_branch();
	}

	m_windowTitle += qstr(" | " + version);

	if (!Emu.GetTitle().empty())
	{
		m_windowTitle += qstr(" | " + Emu.GetTitle());
	}

	if (!Emu.GetTitleID().empty())
	{
		m_windowTitle += qstr(" [" + Emu.GetTitleID() + ']');
	}

	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

	m_show_fps = static_cast<bool>(g_cfg.misc.show_fps_in_title);

	setGeometry(geometry);
	setTitle(m_windowTitle);
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
	auto l_handleKeyEvent = [this ,keyEvent]()
	{
		switch (keyEvent->key())
		{
		case Qt::Key_L:
			if (keyEvent->modifiers() == Qt::AltModifier) { static int count = 0; LOG_SUCCESS(GENERAL, "Made forced mark %d in log", ++count); }
			break;
		case Qt::Key_Return:
			if (keyEvent->modifiers() == Qt::AltModifier) { OnFullScreen(); return; }
			break;
		case Qt::Key_Escape:
			if (visibility() == FullScreen) { setVisibility(Windowed); return; }
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
				if (Emu.IsReady()) { Emu.Run(); return; }
				else if (Emu.IsPaused()) { Emu.Resume(); return; }
			}
			break;
		}
	};
	Emu.CallAfter(l_handleKeyEvent);
}

void gs_frame::OnFullScreen()
{
	auto l_setFullScreenVis = [=]()
	{
		if (visibility() == FullScreen)
		{
			setVisibility(Windowed);
		}
		else
		{
			setVisibility(FullScreen);
		}
	};
	Emu.CallAfter(l_setFullScreenVis);
}

void gs_frame::close()
{
	Emu.Stop();
	Emu.CallAfter([=]() { deleteLater(); });
}

bool gs_frame::shown()
{
	return QWindow::isVisible();
}

void gs_frame::hide()
{
	Emu.CallAfter([=]() {QWindow::hide(); });
}

void gs_frame::show()
{
	Emu.CallAfter([=]()
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
	return (HWND) this->winId();
#else
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
	struct wl_display *wl_dpy = static_cast<struct wl_display *>(
		native->nativeResourceForWindow("display", NULL));
	struct wl_surface *wl_surf = static_cast<struct wl_surface *>(
		native->nativeResourceForWindow("surface", (QWindow *)this));
	if (wl_dpy != nullptr && wl_surf != nullptr)
	{
		return std::make_pair(wl_dpy, wl_surf);
	}
	else
	{
#endif
		return std::make_pair(XOpenDisplay(0), (unsigned long)(this->winId()));
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
	return size().width();
#else
	return size().width() * devicePixelRatio();
#endif
}

int gs_frame::client_height()
{
#ifdef _WIN32
	return size().height();
#else
	return size().height() * devicePixelRatio();
#endif
}

void gs_frame::flip(draw_context_t, bool /*skip_frame*/)
{
	if (m_show_fps)
	{
		++m_frames;

		static Timer fps_t;

		if (fps_t.GetElapsedTimeInSec() >= 0.5)
		{
			QString fps_title = qstr(fmt::format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));

			if (!m_windowTitle.isEmpty())
			{
				fps_title += " | " + m_windowTitle;
			}

			Emu.CallAfter([this, title = std::move(fps_title)]() {setTitle(title); });

			m_frames = 0;
			fps_t.Start();
		}
	}
}

void gs_frame::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (m_disable_mouse) return;

	if (ev->button() == Qt::LeftButton)
	{
		OnFullScreen();
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
	if (ev->type()==QEvent::Close)
	{
		close();
	}
	return QWindow::event(ev);
}

bool gs_frame::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#ifdef _WIN32
	if (wm_event_queue_enabled.load(std::memory_order_consume) && !Emu.IsStopped())
	{
		//Wait for consumer to clear notification pending flag
		while (wm_event_raised.load(std::memory_order_consume) && !Emu.IsStopped());

		{
			std::lock_guard<std::mutex> lock(wm_event_lock);

			// https://bugreports.qt.io/browse/QTBUG-69074?focusedCommentId=409797&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-409797
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
			MSG* msg = *reinterpret_cast<MSG**>(message);
#else
			MSG* msg = reinterpret_cast<MSG*>(message);
#endif

			switch (msg->message)
			{
			case WM_WINDOWPOSCHANGING:
			{
				const auto flags = reinterpret_cast<LPWINDOWPOS>(msg->lParam)->flags & SWP_NOSIZE;
				if (m_in_sizing_event || flags != 0)
					break;

				//About to resize
				m_in_sizing_event = true;
				m_interactive_resize = false;
				m_raised_event = wm_event::geometry_change_notice;
				wm_event_raised.store(true);
				break;
			}
			case WM_WINDOWPOSCHANGED:
			{
				const auto flags = reinterpret_cast<LPWINDOWPOS>(msg->lParam)->flags & (SWP_NOSIZE | SWP_NOMOVE);
				if (!m_in_sizing_event || m_user_interaction_active || flags == (SWP_NOSIZE | SWP_NOMOVE))
					break;

				if (flags & SWP_NOSIZE)
				{
					m_raised_event = wm_event::window_moved;
				}
				else
				{
					LPWINDOWPOS wpos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
					if (wpos->cx <= GetSystemMetrics(SM_CXMINIMIZED) || wpos->cy <= GetSystemMetrics(SM_CYMINIMIZED))
					{
						//Minimize event
						m_minimized = true;
						m_raised_event = wm_event::window_minimized;
					}
					else if (m_minimized)
					{
						m_minimized = false;
						m_raised_event = wm_event::window_restored;
					}
					else
					{
						m_raised_event = wm_event::window_resized;
					}
				}

				//Just finished resizing using maximize or SWP
				m_in_sizing_event = false;
				wm_event_raised.store(true);
				break;
			}
			case WM_ENTERSIZEMOVE:
				m_user_interaction_active = true;
				break;
			case WM_EXITSIZEMOVE:
				m_user_interaction_active = false;
				if (m_in_sizing_event && !m_user_interaction_active)
				{
					//Just finished resizing using manual interaction. The corresponding WM_SIZE is consumed before this event fires
					m_raised_event = m_interactive_resize ? wm_event::window_resized : wm_event::window_moved;
					m_in_sizing_event = false;
					wm_event_raised.store(true);
				}
				break;
			case WM_SIZE:
			{
				if (m_user_interaction_active)
				{
					//Interaction is a resize not a move
					m_interactive_resize = true;
				}
				else if (m_in_sizing_event)
				{
					//Any other unexpected resize mode will give an unconsumed WM_SIZE event
					m_raised_event = wm_event::window_resized;
					m_in_sizing_event = false;
					wm_event_raised.store(true);
				}
				break;
			}
			}
		}

		//Do not enter DefWndProc until the consumer has consumed the message
		while (wm_event_raised.load(std::memory_order_consume) && !Emu.IsStopped());
	}
#endif

	//Let the default handler deal with this. Only the notification is required
	return false;
}

wm_event gs_frame::get_default_wm_event() const
{
	return (m_user_interaction_active) ? wm_event::geometry_change_in_progress : wm_event::none;
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
	properties.insert(QStringLiteral("progress"), (double)progress / (double)m_gauge_max);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
