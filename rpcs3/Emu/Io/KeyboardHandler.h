#pragma once

#include "Keyboard.h"

#include <mutex>

#include "util/init_mutex.hpp"

extern u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode); // (TODO: Can it be problematic to place SysCalls in middle of nowhere?)

enum QtKeys
{
	Key_Shift      = 0x01000020,
	Key_Control    = 0x01000021,
	Key_Meta       = 0x01000022,
	Key_Alt        = 0x01000023,
	Key_CapsLock   = 0x01000024,
	Key_NumLock    = 0x01000025,
	Key_ScrollLock = 0x01000026,
	Key_Super_L    = 0x01000053,
	Key_Super_R    = 0x01000054
};

struct KbInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 info;
	u8 status[CELL_KB_MAX_KEYBOARDS];
};

struct KbData
{
	u32 led;
	u32 mkey;
	s32 len;
	std::pair<u16, u32> keycode[CELL_KB_MAX_KEYCODES];

	KbData()
		: led(0)
		, mkey(0)
		, len(0)
	{ // (TODO: Set initial state of led)
	}
};

struct KbConfig
{
	u32 arrange;
	u32 read_mode;
	u32 code_type;

	KbConfig()
		: arrange(CELL_KB_MAPPING_101)
		, read_mode(CELL_KB_RMODE_INPUTCHAR)
		, code_type(CELL_KB_CODETYPE_ASCII)
	{
	}
};

struct KbButton
{
	u32 m_keyCode;
	u32 m_outKeyCode;
	bool m_pressed = false;

	KbButton(u32 keyCode, u32 outKeyCode)
		: m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
	{
	}
};

struct Keyboard
{
	bool m_key_repeat = false; // for future use
	KbData m_data;
	KbConfig m_config;
	std::vector<KbButton> m_buttons;

	Keyboard()
		: m_data()
		, m_config()
	{
	}
};

class KeyboardHandlerBase
{
protected:
	KbInfo m_info;
	std::vector<Keyboard> m_keyboards;

public:
	std::mutex m_mutex;

	virtual void Init(const u32 max_connect) = 0;

	virtual ~KeyboardHandlerBase() = default;

	void Key(u32 code, bool pressed);

	bool IsMetaKey(u32 code);

	KbInfo& GetInfo() { return m_info; }
	std::vector<Keyboard>& GetKeyboards() { return m_keyboards; }
	std::vector<KbButton>& GetButtons(const u32 keyboard) { return m_keyboards[keyboard].m_buttons; }
	KbData& GetData(const u32 keyboard) { return m_keyboards[keyboard].m_data; }
	KbConfig& GetConfig(const u32 keyboard) { return m_keyboards[keyboard].m_config; }

	stx::init_mutex init;
};
