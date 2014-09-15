#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Null/NullMouseHandler.h"
#include "Mouse.h"

GetMouseHandlerCountCb GetMouseHandlerCount = []()
{
	return 1;
};

GetMouseHandlerCb GetMouseHandler = [](int i) -> MouseHandlerBase*
{
	return new NullMouseHandler;
};

void SetGetMouseHandlerCountCallback(GetMouseHandlerCountCb cb)
{
	GetMouseHandlerCount = cb;
}

void SetGetMouseHandlerCallback(GetMouseHandlerCb cb)
{
	GetMouseHandler = cb;
}

MouseManager::MouseManager()
	: m_mouse_handler(nullptr)
	, m_inited(false)
{
}

MouseManager::~MouseManager()
{
}

void MouseManager::Init(const u32 max_connect)
{
	if(m_inited)
		return;

	// NOTE: Change these to std::make_unique assignments when C++14 is available.
	int numHandlers = GetMouseHandlerCount();
	int selectedHandler = Ini.MouseHandlerMode.GetValue();
	if (selectedHandler > numHandlers)
	{
		selectedHandler = 0;
	}
	m_mouse_handler.reset(GetMouseHandler(selectedHandler));

	m_mouse_handler->Init(max_connect);
	m_inited = true;
}

void MouseManager::Close()
{
	if(m_mouse_handler) m_mouse_handler->Close();
	m_mouse_handler = nullptr;

	m_inited = false;
}
