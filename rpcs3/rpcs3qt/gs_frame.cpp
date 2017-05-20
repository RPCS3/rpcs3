#include "gs_frame.h"

#include "Utilities/Config.h"
#include "Utilities/Timer.h"
#include "Emu/System.h"

#include <QKeyEvent>
#include <QTimer>
#include <QThread>

cfg::bool_entry g_cfg_start_fullscreen_mode(cfg::root.misc, "Start Games in Fullscreen Mode");

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

gs_frame::gs_frame(const QString& title, int w, int h, QIcon appIcon)
	: QWindow()
{
	m_windowTitle = title;
	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

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
		case Qt::Key_Return: if (keyEvent->modifiers() == Qt::AltModifier) { OnFullScreen(); return; } break;
		case Qt::Key_Escape: if (visibility() == FullScreen) { setVisibility(Windowed); return; } break;
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
		if (g_cfg_start_fullscreen_mode)
		{
			setVisibility(FullScreen);
		}
	});
}

void* gs_frame::handle() const
{
	return (HWND) this->winId();
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
	return size().width();
}

int gs_frame::client_height()
{
	return size().height();
}

void gs_frame::flip(draw_context_t)
{
	++m_frames;

	static Timer fps_t;

	if (fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		QString title = qstr(fmt::format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));

		if (!m_windowTitle.isEmpty())
		{
			title += " | " + m_windowTitle;
		}

		if (!Emu.GetTitle().empty())
		{
			title += qstr(" | " + Emu.GetTitle());
		}

		if (!Emu.GetTitleID().empty())
		{
			title += qstr(" | [" + Emu.GetTitleID() + ']');
		}

		Emu.CallAfter([this, title = std::move(title)]() {setTitle(title); });

		m_frames = 0;
		fps_t.Start();
	}
}

void gs_frame::mouseDoubleClickEvent(QMouseEvent* ev)
{
	OnFullScreen();
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
