#pragma once

#include "Keyboard.h"

#include <mutex>
#include <vector>

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
	u32 max_connect = 0;
	u32 now_connect = 0;
	u32 info = 0;
	bool is_null_handler = false;
	std::array<u8, CELL_KB_MAX_KEYBOARDS> status{};
};

struct KbButton
{
	u32 m_keyCode = 0;
	u32 m_outKeyCode = 0;
	bool m_pressed = false;
	
	KbButton() = default;
	KbButton(u32 keyCode, u32 outKeyCode, bool pressed = false)
		: m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
		, m_pressed(pressed)
	{
	}
};

struct KbData
{
	u32 led = 0;  // Active led lights     TODO: Set initial state of led
	u32 mkey = 0; // Active key modifiers
	s32 len = 0;  // Number of key codes (0 means no data available)
	std::array<KbButton, CELL_KB_MAX_KEYCODES> buttons{};
};

struct KbConfig
{
	u32 arrange = CELL_KB_MAPPING_101;
	u32 read_mode = CELL_KB_RMODE_INPUTCHAR;
	u32 code_type = CELL_KB_CODETYPE_ASCII;
};

struct Keyboard
{
	bool m_key_repeat = false;
	KbData m_data{};
	KbConfig m_config{};
	std::vector<KbButton> m_buttons;
};

class KeyboardHandlerBase
{
public:
	std::mutex m_mutex;

	virtual void Init(const u32 max_connect) = 0;

	virtual ~KeyboardHandlerBase() = default;

	void Key(u32 code, bool pressed);
	void SetIntercepted(bool intercepted);

	static bool IsMetaKey(u32 code);

	KbInfo& GetInfo() { return m_info; }
	std::vector<Keyboard>& GetKeyboards() { return m_keyboards; }
	std::vector<KbButton>& GetButtons(const u32 keyboard) { return m_keyboards[keyboard].m_buttons; }
	KbData& GetData(const u32 keyboard) { return m_keyboards[keyboard].m_data; }
	KbConfig& GetConfig(const u32 keyboard) { return m_keyboards[keyboard].m_config; }

	stx::init_mutex init;

protected:
	void ReleaseAllKeys();

	KbInfo m_info{};
	std::vector<Keyboard> m_keyboards;
};
