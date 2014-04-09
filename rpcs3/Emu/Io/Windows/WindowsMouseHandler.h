#pragma once

#include <algorithm>
#include "Emu/Io/MouseHandler.h"

class WindowsMouseHandler final
	: public wxWindow
	, public MouseHandlerBase
{
	AppConnector m_app_connector;

public:
	WindowsMouseHandler() : wxWindow()
	{
		m_app_connector.Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(WindowsMouseHandler::MouseButtonDown), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(WindowsMouseHandler::MouseButtonDown), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(WindowsMouseHandler::MouseButtonDown), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_LEFT_UP, wxMouseEventHandler(WindowsMouseHandler::MouseButtonUp), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(WindowsMouseHandler::MouseButtonUp), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(WindowsMouseHandler::MouseButtonUp), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(WindowsMouseHandler::MouseScroll), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_MOTION, wxMouseEventHandler(WindowsMouseHandler::MouseMove), (wxObject*)0, this);
	}

	virtual void MouseButtonDown(wxMouseEvent& event)
	{
		if (event.LeftDown())        MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 1);
		else if (event.RightDown())  MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 1);
		else if (event.MiddleDown()) MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 1);
		event.Skip();
	}
	virtual void MouseButtonUp(wxMouseEvent& event)
	{
		if (event.LeftUp())          MouseHandlerBase::Button(CELL_MOUSE_BUTTON_1, 0);
		else if (event.RightUp())    MouseHandlerBase::Button(CELL_MOUSE_BUTTON_2, 0);
		else if (event.MiddleUp())   MouseHandlerBase::Button(CELL_MOUSE_BUTTON_3, 0);
		event.Skip();
	}
	virtual void MouseScroll(wxMouseEvent& event) { MouseHandlerBase::Scroll(event.GetWheelRotation()); event.Skip(); }
	virtual void MouseMove(wxMouseEvent& event)   { MouseHandlerBase::Move(event.m_x, event.m_y); event.Skip(); }

	virtual void Init(const u32 max_connect)
	{
		m_mice.emplace_back(Mouse());
		memset(&m_info, 0, sizeof(MouseInfo));
		m_info.max_connect = max_connect;
		m_info.now_connect = std::min(m_mice.size(), (size_t)max_connect);
		m_info.info = 0; // Ownership of mouse data: 0=Application, 1=System
		m_info.status[0] = CELL_MOUSE_STATUS_CONNECTED;										// (TODO: Support for more mice)
		for(u32 i=1; i<max_connect; i++) m_info.status[i] = CELL_MOUSE_STATUS_DISCONNECTED;
		m_info.vendor_id[0] = 0x1234;
		m_info.product_id[0] = 0x1234;
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(MouseInfo));
		m_mice.clear();
	}
};