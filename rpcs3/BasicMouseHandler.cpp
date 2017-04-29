#include "BasicMouseHandler.h"

void BasicMouseHandler::Init(const u32 max_connect)
{
	m_mice.emplace_back(Mouse());
	memset(&m_info, 0, sizeof(MouseInfo));
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(m_mice.size(), (size_t)max_connect);
	m_info.info = 0; // Ownership of mouse data: 0=Application, 1=System
	m_info.status[0] = CELL_MOUSE_STATUS_CONNECTED;										// (TODO: Support for more mice)
	for (u32 i = 1; i<max_connect; i++) m_info.status[i] = CELL_MOUSE_STATUS_DISCONNECTED;
	m_info.vendor_id[0] = 0x1234;
	m_info.product_id[0] = 0x1234;
}

BasicMouseHandler::BasicMouseHandler(QObject* target, QObject* parent) : QObject(parent), m_target(target)
{
	target->installEventFilter(this);
}

bool BasicMouseHandler::eventFilter(QObject* obj, QEvent* ev)
{
	// Commenting target since I don't know how to target game window yet.
	//if (m_target == obj)
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
		}
	}
	return false;
}

void BasicMouseHandler::MouseButtonDown(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)        MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 1);
	else if (event->button() == Qt::RightButton)  MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 1);
	else if (event->button() == Qt::MiddleButton) MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 1);
}

void BasicMouseHandler::MouseButtonUp(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)        MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 0);
	else if (event->button() == Qt::RightButton)  MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 0);
	else if (event->button() == Qt::MiddleButton) MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 0);
}

void BasicMouseHandler::MouseScroll(QWheelEvent* event)
{
	// Woo lads, Qt handles multidimensional scrolls. Just gonna grab the x for now. Not sure if this works. TODO: Test
	MouseHandlerBase::Scroll(event->angleDelta().x());
}

void BasicMouseHandler::MouseMove(QMouseEvent* event)
{
	MouseHandlerBase::Move(event->x(), event->y());
}
