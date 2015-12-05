#include "stdafx.h"
#include "Emu/System.h"

#include "Mouse.h"

void MouseManager::Init(u32 max_connect)
{
	m_mouse_handler = Emu.GetCallbacks().get_mouse_handler();
	m_mouse_handler->Init(max_connect);
}

void MouseManager::Close()
{
	m_mouse_handler.reset();
}
