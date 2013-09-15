#include "stdafx.h"
#include "Keyboard.h"
#include "Null/NullKeyboardHandler.h"
#include "Windows/WindowsKeyboardHandler.h"

KeyboardManager::KeyboardManager()
	: m_keyboard_handler(nullptr)
	, m_inited(false)
{
}

KeyboardManager::~KeyboardManager()
{
}

void KeyboardManager::Init(const u32 max_connect)
{
	if(m_inited) return;

	switch(Ini.KeyboardHandlerMode.GetValue())
	{
	case 1: m_keyboard_handler = new WindowsKeyboardHandler(); break;

	default:
	case 0: m_keyboard_handler = new NullKeyboardHandler(); break;
	}

	m_keyboard_handler->Init(max_connect);
	m_inited = true;
}

void KeyboardManager::Close()
{
	if(m_keyboard_handler) m_keyboard_handler->Close();
	m_keyboard_handler = nullptr;

	m_inited = false;
}