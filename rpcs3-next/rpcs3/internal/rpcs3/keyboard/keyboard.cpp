#include "stdafx.h"
#include "Emu/System.h"

#include "Keyboard.h"

void KeyboardManager::Init(u32 max_connect)
{
	m_keyboard_handler = Emu.GetCallbacks().get_kb_handler();
	m_keyboard_handler->Init(max_connect);
}

void KeyboardManager::Close()
{
	m_keyboard_handler.reset();
}
