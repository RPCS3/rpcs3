#include "basic_mouse_handler.h"

#include <QApplication>

void basic_mouse_handler::Init(const u32 max_connect)
{
	m_mice.emplace_back(Mouse());
	memset(&m_info, 0, sizeof(MouseInfo));
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(m_mice.size(), (size_t)max_connect);
	m_info.info = 0; // Ownership of mouse data: 0=Application, 1=System
	for (u32 i = 1; i < max_connect; i++)
	{
		m_info.status[i] = CELL_MOUSE_STATUS_DISCONNECTED;
		m_info.mode[i] = CELL_MOUSE_INFO_TABLET_MOUSE_MODE;
		m_info.tablet_is_supported[i] = CELL_MOUSE_INFO_TABLET_NOT_SUPPORTED;
	}
	m_info.status[0] = CELL_MOUSE_STATUS_CONNECTED;										// (TODO: Support for more mice)
	m_info.vendor_id[0] = 0x1234;
	m_info.product_id[0] = 0x1234;
}

basic_mouse_handler::basic_mouse_handler() : QObject()
{}

/* Sets the target window for the event handler, and also installs an event filter on the target. */
void basic_mouse_handler::SetTargetWindow(QWindow* target)
{
	if (target != nullptr)
	{
		m_target = target;
		target->installEventFilter(this);
	}
	else
	{
		// If this is hit, it probably means that some refactoring occurs because currently a gsframe is created in Load.
		// We still want events so filter from application instead since target is null.
		QApplication::instance()->installEventFilter(this);
		LOG_ERROR(GENERAL, "Trying to set mouse handler to a null target window.");
	}
}

bool basic_mouse_handler::eventFilter(QObject* target, QEvent* ev)
{
	// !m_target is for future proofing when gsrender isn't automatically initialized on load to ensure events still occur
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible() || target == m_target)
	{
		switch (ev->type())
		{
		case QEvent::MouseButtonPress:
			MouseButtonDown(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseButtonRelease:
			MouseButtonUp(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseMove:
			MouseMove(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::Wheel:
			MouseScroll(static_cast<QWheelEvent*>(ev));
			break;
		default:
			return false;
		}
	}
	return false;
}

void basic_mouse_handler::MouseButtonDown(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)        MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 1);
	else if (event->button() == Qt::RightButton)  MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 1);
	else if (event->button() == Qt::MiddleButton) MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 1);
}

void basic_mouse_handler::MouseButtonUp(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)        MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 0);
	else if (event->button() == Qt::RightButton)  MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 0);
	else if (event->button() == Qt::MiddleButton) MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 0);
}

void basic_mouse_handler::MouseScroll(QWheelEvent* event)
{
	// Woo lads, Qt handles multidimensional scrolls. Just gonna grab the x for now. Not sure if this works. TODO: Test
	MouseHandlerBase::Scroll(event->angleDelta().x());
}

void basic_mouse_handler::MouseMove(QMouseEvent* event)
{
	MouseHandlerBase::Move(event->x(), event->y());
}
