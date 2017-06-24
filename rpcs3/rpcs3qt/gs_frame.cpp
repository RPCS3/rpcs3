#include "gs_frame.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Emu/System.h"

#include <QKeyEvent>
#include <QTimer>
#include <QThread>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

gs_frame::gs_frame(const QString& title, int w, int h, QIcon appIcon)
	: QWindow()
{
	m_windowTitle = title;

	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

	g_cfg.misc.show_fps_in_title ? m_show_fps = true : m_show_fps = false;

	resize(w, h);

	// Change cursor when in fullscreen.
	connect(this, &QWindow::visibilityChanged, this, &gs_frame::HandleCursor);
}

void gs_frame::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
}

void gs_frame::keyPressEvent(QKeyEvent *keyEvent)
{
	auto l_handleKeyEvent = [this ,keyEvent]()
	{
		switch (keyEvent->key())
		{
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
			if (keyEvent->modifiers() == Qt::ControlModifier && (!Emu.GetPath().empty())) { Emu.Stop(); Emu.Load(); return; }
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
	Emu.CallAfter([=]() {QWindow::close(); });
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
	Emu.CallAfter([=]() {
		QWindow::show(); 
		if (g_cfg.misc.start_fullscreen)
		{
			setVisibility(FullScreen);
		}
	});
}

void* gs_frame::handle() const
{
#ifdef _WIN32
	return (HWND) this->winId();
#else
	return (void *)this->winId();
#endif
}

void* gs_frame::make_context()
{
	return nullptr;
}

void gs_frame::set_current(draw_context_t ctx)
{
	Q_UNUSED(ctx);
}

void gs_frame::delete_context(void* ctx)
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

void gs_frame::flip(draw_context_t)
{
	QString title;

	if (!m_windowTitle.isEmpty())
	{
		title += m_windowTitle;
	}

	if (!Emu.GetTitle().empty())
	{
		title += qstr(" | " + Emu.GetTitle());
	}

	if (!Emu.GetTitleID().empty())
	{
		title += qstr(" | [" + Emu.GetTitleID() + ']');
	}

	if (m_show_fps)
	{
		++m_frames;

		static Timer fps_t;

		if (fps_t.GetElapsedTimeInSec() >= 0.5)
		{
			QString fps_title = qstr(fmt::format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));

			if (!title.isEmpty())
			{
				fps_title += " | " + title;
			}

			Emu.CallAfter([this, title = std::move(fps_title)]() {setTitle(title); });

			m_frames = 0;
			fps_t.Start();
		}
	}
	else
	{
		if (this->title() != title)
		{
			Emu.CallAfter([this, title = std::move(title)]() {setTitle(title); });
		}
	}
}

void gs_frame::mouseDoubleClickEvent(QMouseEvent* ev)
{
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

/** Override qt hideEvent.
 * For some reason beyond me, hitting X hides the game window instead of closes. To remedy this, I forcefully murder it for commiting this transgression.
 * Closing the window has a side-effect of also stopping the emulator.
*/
void gs_frame::hideEvent(QHideEvent* ev)
{
	Q_UNUSED(ev);

	close();
}
