#pragma once

#include "Emu/Io/KeyboardHandler.h"

class WindowsKeyboardHandler
	: public wxWindow
	, public KeyboardHandlerBase
{
	AppConnector m_app_connector;

public:
	WindowsKeyboardHandler() : wxWindow()
	{
		m_app_connector.Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(WindowsKeyboardHandler::KeyDown), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_KEY_UP, wxKeyEventHandler(WindowsKeyboardHandler::KeyUp), (wxObject*)0, this);
	}

	virtual void KeyDown(wxKeyEvent& event)	{ Key(event.GetKeyCode(), 1); event.Skip(); }
	virtual void KeyUp(wxKeyEvent& event)	{ Key(event.GetKeyCode(), 0); event.Skip(); }

	virtual void Init(const u32 max_connect)
	{
		for(u32 i=0; i<max_connect; i++)
		{
			m_keyboards.Move(new Keyboard());
		}
		LoadSettings();
		memset(&m_info, 0, sizeof(KbInfo));
		m_info.max_connect = max_connect;
		m_info.now_connect = min<int>(GetKeyboards().GetCount(), max_connect);
		m_info.info = 0; // Ownership of keyboard data: 0=Application, 1=System
		m_info.status[0] = CELL_KB_STATUS_CONNECTED; // (TODO: Support for more keyboards)
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(KbInfo));
		m_keyboards.Clear();
	}

	void LoadSettings()
	{
		// Meta Keys
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_CONTROL, CELL_KB_MKEY_L_CTRL));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_SHIFT, CELL_KB_MKEY_L_SHIFT));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_ALT, CELL_KB_MKEY_L_ALT));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_WINDOWS_LEFT, CELL_KB_MKEY_L_WIN));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_COMMAND, CELL_KB_MKEY_L_WIN));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KB_MKEY_R_CTRL));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KB_MKEY_R_SHIFT));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KB_MKEY_R_ALT));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_WINDOWS_RIGHT, CELL_KB_MKEY_R_WIN));

		// CELL_KB_RAWDAT
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_NO_EVENT));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_E_ROLLOVER));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_E_POSTFAIL));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_E_UNDEF));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_ESCAPE, CELL_KEYC_ESCAPE));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_106_KANJI));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_CAPITAL, CELL_KEYC_CAPS_LOCK));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F1, CELL_KEYC_F1));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F2, CELL_KEYC_F2));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F3, CELL_KEYC_F3));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F4, CELL_KEYC_F4));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F5, CELL_KEYC_F5));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F6, CELL_KEYC_F6));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F7, CELL_KEYC_F7));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F8, CELL_KEYC_F8));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F9, CELL_KEYC_F9));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F10, CELL_KEYC_F10));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F11, CELL_KEYC_F11));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_F12, CELL_KEYC_F12));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_PRINT, CELL_KEYC_PRINTSCREEN));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_SCROLL, CELL_KEYC_SCROLL_LOCK));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_PAUSE, CELL_KEYC_PAUSE));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_INSERT, CELL_KEYC_INSERT));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_HOME));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_PAGEUP, CELL_KEYC_PAGE_UP));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_DELETE, CELL_KEYC_DELETE));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_END, CELL_KEYC_END));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_PAGEDOWN, CELL_KEYC_PAGE_DOWN));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_RIGHT, CELL_KEYC_RIGHT_ARROW));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_LEFT, CELL_KEYC_LEFT_ARROW));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_DOWN, CELL_KEYC_DOWN_ARROW));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_UP, CELL_KEYC_UP_ARROW));
		//m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMLOCK, CELL_KEYC_NUM_LOCK));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_APPLICATION));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_KANA));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_HENKAN));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_MUHENKAN));

		// CELL_KB_KEYPAD
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMLOCK, CELL_KEYC_KPAD_NUMLOCK));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_DIVIDE, CELL_KEYC_KPAD_SLASH));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_MULTIPLY, CELL_KEYC_KPAD_ASTERISK));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_SUBTRACT, CELL_KEYC_KPAD_MINUS));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_ADD, CELL_KEYC_KPAD_PLUS));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_ENTER, CELL_KEYC_KPAD_ENTER));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD1, CELL_KEYC_KPAD_1));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD2, CELL_KEYC_KPAD_2));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD3, CELL_KEYC_KPAD_3));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD4, CELL_KEYC_KPAD_4));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD5, CELL_KEYC_KPAD_5));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD6, CELL_KEYC_KPAD_6));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD7, CELL_KEYC_KPAD_7));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD8, CELL_KEYC_KPAD_8));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD9, CELL_KEYC_KPAD_9));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD0, CELL_KEYC_KPAD_0));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_NUMPAD_DELETE, CELL_KEYC_KPAD_PERIOD));

		// ASCII Printable characters
		m_keyboards[0].m_buttons.Move(new KbButton('A', CELL_KEYC_A));
		m_keyboards[0].m_buttons.Move(new KbButton('B', CELL_KEYC_B));
		m_keyboards[0].m_buttons.Move(new KbButton('C', CELL_KEYC_C));
		m_keyboards[0].m_buttons.Move(new KbButton('D', CELL_KEYC_D));
		m_keyboards[0].m_buttons.Move(new KbButton('E', CELL_KEYC_E));
		m_keyboards[0].m_buttons.Move(new KbButton('F', CELL_KEYC_F));
		m_keyboards[0].m_buttons.Move(new KbButton('G', CELL_KEYC_G));
		m_keyboards[0].m_buttons.Move(new KbButton('H', CELL_KEYC_H));
		m_keyboards[0].m_buttons.Move(new KbButton('I', CELL_KEYC_I));
		m_keyboards[0].m_buttons.Move(new KbButton('J', CELL_KEYC_J));
		m_keyboards[0].m_buttons.Move(new KbButton('K', CELL_KEYC_K));
		m_keyboards[0].m_buttons.Move(new KbButton('L', CELL_KEYC_L));
		m_keyboards[0].m_buttons.Move(new KbButton('M', CELL_KEYC_M));
		m_keyboards[0].m_buttons.Move(new KbButton('N', CELL_KEYC_N));
		m_keyboards[0].m_buttons.Move(new KbButton('O', CELL_KEYC_O));
		m_keyboards[0].m_buttons.Move(new KbButton('P', CELL_KEYC_P));
		m_keyboards[0].m_buttons.Move(new KbButton('Q', CELL_KEYC_Q));
		m_keyboards[0].m_buttons.Move(new KbButton('R', CELL_KEYC_R));
		m_keyboards[0].m_buttons.Move(new KbButton('S', CELL_KEYC_S));
		m_keyboards[0].m_buttons.Move(new KbButton('T', CELL_KEYC_T));
		m_keyboards[0].m_buttons.Move(new KbButton('U', CELL_KEYC_U));
		m_keyboards[0].m_buttons.Move(new KbButton('V', CELL_KEYC_V));
		m_keyboards[0].m_buttons.Move(new KbButton('W', CELL_KEYC_W));
		m_keyboards[0].m_buttons.Move(new KbButton('X', CELL_KEYC_X));
		m_keyboards[0].m_buttons.Move(new KbButton('Y', CELL_KEYC_Y));
		m_keyboards[0].m_buttons.Move(new KbButton('Z', CELL_KEYC_Z));

		m_keyboards[0].m_buttons.Move(new KbButton('1', CELL_KEYC_1));
		m_keyboards[0].m_buttons.Move(new KbButton('2', CELL_KEYC_2));
		m_keyboards[0].m_buttons.Move(new KbButton('3', CELL_KEYC_3));
		m_keyboards[0].m_buttons.Move(new KbButton('4', CELL_KEYC_4));
		m_keyboards[0].m_buttons.Move(new KbButton('5', CELL_KEYC_5));
		m_keyboards[0].m_buttons.Move(new KbButton('6', CELL_KEYC_6));
		m_keyboards[0].m_buttons.Move(new KbButton('7', CELL_KEYC_7));
		m_keyboards[0].m_buttons.Move(new KbButton('8', CELL_KEYC_8));
		m_keyboards[0].m_buttons.Move(new KbButton('9', CELL_KEYC_9));
		m_keyboards[0].m_buttons.Move(new KbButton('0', CELL_KEYC_0));

		m_keyboards[0].m_buttons.Move(new KbButton(WXK_RETURN, CELL_KEYC_ENTER));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_ESC));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_TAB, CELL_KEYC_TAB));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_SPACE, CELL_KEYC_SPACE));
		m_keyboards[0].m_buttons.Move(new KbButton(WXK_SUBTRACT, CELL_KEYC_MINUS));
		m_keyboards[0].m_buttons.Move(new KbButton('=', CELL_KEYC_EQUAL_101));
		m_keyboards[0].m_buttons.Move(new KbButton('^', CELL_KEYC_ACCENT_CIRCONFLEX_106));
		//m_keyboards[0].m_buttons.Move(new KbButton('(', CELL_KEYC_LEFT_BRACKET_101));
		m_keyboards[0].m_buttons.Move(new KbButton('@', CELL_KEYC_ATMARK_106));
		//m_keyboards[0].m_buttons.Move(new KbButton(')', CELL_KEYC_RIGHT_BRACKET_101));
		m_keyboards[0].m_buttons.Move(new KbButton('(', CELL_KEYC_LEFT_BRACKET_106));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_BACKSLASH_101));
		m_keyboards[0].m_buttons.Move(new KbButton('(', CELL_KEYC_RIGHT_BRACKET_106));
		m_keyboards[0].m_buttons.Move(new KbButton(';', CELL_KEYC_SEMICOLON));
		m_keyboards[0].m_buttons.Move(new KbButton('"', CELL_KEYC_QUOTATION_101));
		m_keyboards[0].m_buttons.Move(new KbButton(':', CELL_KEYC_COLON_106));
		m_keyboards[0].m_buttons.Move(new KbButton(',', CELL_KEYC_COMMA));
		m_keyboards[0].m_buttons.Move(new KbButton('.', CELL_KEYC_PERIOD));
		m_keyboards[0].m_buttons.Move(new KbButton('/', CELL_KEYC_SLASH));
		m_keyboards[0].m_buttons.Move(new KbButton('\\', CELL_KEYC_BACKSLASH_106));
		//m_keyboards[0].m_buttons.Move(new KbButton(, CELL_KEYC_YEN_106));
	}
};