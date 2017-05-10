#include "GSFrame.h"

#include "Utilities/Timer.h"
#include "Emu/System.h"

#include <QKeyEvent>
#include <QTimer>
#include <QThread>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

GSFrame::GSFrame(const QString& title, int w, int h, QIcon appIcon)
	: QWindow()
{
	m_windowTitle = title;
	if (!appIcon.isNull())
	{
		setIcon(appIcon);
	}

	resize(w, h);

	// I'd love to not use blocking queued connections, but this seems to be the only way to force this code to happen.
	// When I have it as a nonblocking connection, the UI commands won't work. I have no idea why.
	connect(this, &GSFrame::RequestCommand, this, &GSFrame::HandleCommandRequest, Qt::BlockingQueuedConnection);

	// Change cursor when in fullscreen.
	connect(this, &QWindow::visibilityChanged, this, &GSFrame::HandleCursor);
}

void GSFrame::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
}

void GSFrame::keyPressEvent(QKeyEvent *keyEvent)
{
	auto l_handleKeyEvent = [this ,keyEvent]()
	{
		switch (keyEvent->key())
		{
		case Qt::Key_Return: if (keyEvent->modifiers() == Qt::AltModifier) { OnFullScreen(); return; } break;
		case Qt::Key_Escape: if (visibility() == FullScreen) { setVisibility(Windowed); return; } break;
		}
	};
	HandleUICommand(l_handleKeyEvent);
}

void GSFrame::OnFullScreen()
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
	HandleUICommand(l_setFullScreenVis);
}

void GSFrame::close()
{
	Emu.Stop();
	HandleUICommand([=]() {QWindow::close(); });
}

bool GSFrame::shown()
{
	return QWindow::isVisible();
}

void GSFrame::hide()
{
	HandleUICommand([=]() {QWindow::hide(); });
}

void GSFrame::show()
{
	HandleUICommand([=]() {QWindow::show(); });
}

void* GSFrame::handle() const
{
	return (HWND) this->winId();
}

void* GSFrame::make_context()
{
	return nullptr;
}

void GSFrame::set_current(draw_context_t ctx)
{
	Q_UNUSED(ctx);
}

void GSFrame::delete_context(void* ctx)
{
	Q_UNUSED(ctx);
}

int GSFrame::client_width()
{
	return size().width();
}

int GSFrame::client_height()
{
	return size().height();
}

void GSFrame::flip(draw_context_t)
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

		HandleUICommand([this, title = std::move(title)]() {setTitle(title); });

		m_frames = 0;
		fps_t.Start();
	}
}

void GSFrame::mouseDoubleClickEvent(QMouseEvent* ev)
{
	OnFullScreen();
}

void GSFrame::HandleCursor(QWindow::Visibility visibility)
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

/** Magic. Please don't remove.
 * Qt doesn't like to have UI functions called from non-UI threads [really bad thing to do!!!].  However, connects, WILL handle this scenario cleanly.
 * The idea is that for any UI functions you need to call from non-UI threads, you pass the function (perhaps even a lambda) here.
 * This is equivalent to call_after.  However, I wanted to avoid using Emu callbacks for when we potentially change which thread spawns this window.
*/
void GSFrame::HandleCommandRequest(std::function<void()> func)
{
	func();
}

/** Helper method for magic.
* Prevents having this if/else everywhere for this.
*/
void GSFrame::HandleUICommand(std::function<void()> func)
{
	// thread() returns the current thread that has the event handler for this QObject. Compare it to the current thread name to see if a special command request is needed.
	if (QThread::currentThread() != thread())
	{
		emit RequestCommand(func);
	}
	else
	{
		func();
	}
}

/** Override qt hideEvent.
 * For some reason beyond me, hitting X hides the game window instead of closes. To remedy this, I forcefully murder it for commiting this transgression.
 * Closing the window has a side-effect of also stopping the emulator.
*/
void GSFrame::hideEvent(QHideEvent* ev)
{
	Q_UNUSED(ev);

	close();
}
