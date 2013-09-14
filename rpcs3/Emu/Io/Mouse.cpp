#include "stdafx.h"
#include "Mouse.h"
#include "Null/NullMouseHandler.h"
#include "Windows/WindowsMouseHandler.h"

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
	if(m_inited) return;

	switch(Ini.MouseHandlerMode.GetValue())
	{
	case 1: m_mouse_handler = new WindowsMouseHandler(); break;

	default:
	case 0: m_mouse_handler = new NullMouseHandler(); break;
	}

	m_mouse_handler->Init(max_connect);
	m_inited = true;
}

void MouseManager::Close()
{
	if(m_mouse_handler) m_mouse_handler->Close();
	m_mouse_handler = nullptr;

	m_inited = false;
}