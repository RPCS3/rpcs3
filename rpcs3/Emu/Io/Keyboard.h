#pragma once

#include "KeyboardHandler.h"

class KeyboardManager
{
	std::unique_ptr<KeyboardHandlerBase> m_keyboard_handler;

public:
	void Init(u32 max_connect);
	void Close();

	std::vector<Keyboard>& GetKeyboards() { return m_keyboard_handler->GetKeyboards(); }
	KbInfo& GetInfo() { return m_keyboard_handler->GetInfo(); }
	std::vector<KbButton>& GetButtons(const u32 keyboard) { return m_keyboard_handler->GetButtons(keyboard); }
	KbData& GetData(const u32 keyboard) { return m_keyboard_handler->GetData(keyboard); }
	KbConfig& GetConfig(const u32 keyboard) { return m_keyboard_handler->GetConfig(keyboard); }

	bool IsInited() const { return m_keyboard_handler.operator bool(); }
};
