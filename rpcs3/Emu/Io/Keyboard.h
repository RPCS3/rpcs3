#pragma once

#include <memory>
#include <vector>
#include "KeyboardHandler.h"

class KeyboardManager //: public wxWindow
{
	bool m_inited;
	std::unique_ptr<KeyboardHandlerBase> m_keyboard_handler;

public:
	KeyboardManager();
	~KeyboardManager();

	void Init(const u32 max_connect);
	void Close();

	std::vector<Keyboard>& GetKeyboards() { return m_keyboard_handler->GetKeyboards(); }
	KbInfo& GetInfo() { return m_keyboard_handler->GetInfo(); }
	std::vector<KbButton>& GetButtons(const u32 keyboard) { return m_keyboard_handler->GetButtons(keyboard); }
	CellKbData& GetData(const u32 keyboard) { return m_keyboard_handler->GetData(keyboard); }
	CellKbConfig& GetConfig(const u32 keyboard) { return m_keyboard_handler->GetConfig(keyboard); }

	bool IsInited() const { return m_inited; }

//private:
	//DECLARE_EVENT_TABLE();
};