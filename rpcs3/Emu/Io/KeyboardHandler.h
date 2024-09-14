#pragma once

#include "Keyboard.h"

#include <mutex>
#include <vector>
#include <set>
#include <unordered_map>

#include "util/init_mutex.hpp"

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

// See https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
enum native_key : u32
{
#ifdef _WIN32
	ctrl_l = 0x001D,
	ctrl_r = 0xE01D,
	shift_l = 0x002A,
	shift_r = 0x0036,
	alt_l = 0x0038,
	alt_r = 0xE038,
	meta_l = 0xE05B,
	meta_r = 0xE05C,
#elif defined (__APPLE__)
	ctrl_l = 0x3B,  // kVK_Control
	ctrl_r = 0x3E,  // kVK_RightControl
	shift_l = 0x38, // kVK_Shift
	shift_r = 0x3C, // kVK_RightShift
	alt_l = 0x3A,   // kVK_Option
	alt_r = 0x3D,   // kVK_RightOption
	meta_l = 0x37,  // kVK_Command
	meta_r = 0x36,  // kVK_RightCommand
#else
	ctrl_l = 0x0025,
	ctrl_r = 0x0069,
	shift_l = 0x0032,
	shift_r = 0x003E,
	alt_l = 0x0040,
	alt_r = 0x006C,
	meta_l = 0x0085,
	meta_r = 0x0086,
#endif
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

struct KbExtraData
{
	std::set<std::u32string> pressed_keys{};
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
	KbExtraData m_extra_data{};
	KbConfig m_config{};
	std::unordered_map<u32, KbButton> m_keys;
};

class keyboard_consumer
{
public:
	enum class identifier
	{
		unknown,
		overlays,
		cellKb,
	};

	keyboard_consumer() {}
	keyboard_consumer(identifier id) : m_id(id) {}

	bool ConsumeKey(u32 qt_code, u32 native_code, bool pressed, bool is_auto_repeat, const std::u32string& key);
	void SetIntercepted(bool intercepted);

	static bool IsMetaKey(u32 code);

	KbInfo& GetInfo() { return m_info; }
	std::vector<Keyboard>& GetKeyboards() { return m_keyboards; }
	KbData& GetData(const u32 keyboard) { return m_keyboards[keyboard].m_data; }
	KbExtraData& GetExtraData(const u32 keyboard) { return m_keyboards[keyboard].m_extra_data; }
	KbConfig& GetConfig(const u32 keyboard) { return m_keyboards[keyboard].m_config; }
	identifier id() const { return m_id; }

	void ReleaseAllKeys();

protected:
	u32 get_out_key_code(u32 qt_code, u32 native_code, u32 out_key_code);

	identifier m_id = identifier::unknown;
	KbInfo m_info{};
	std::vector<Keyboard> m_keyboards;
};

class KeyboardHandlerBase
{
public:
	std::mutex m_mutex;

	virtual void Init(keyboard_consumer& consumer, const u32 max_connect) = 0;

	virtual ~KeyboardHandlerBase() = default;
	KeyboardHandlerBase(utils::serial* ar);
	KeyboardHandlerBase(utils::serial& ar) : KeyboardHandlerBase(&ar) {}
	void save(utils::serial& ar);

	SAVESTATE_INIT_POS(19);

	keyboard_consumer& AddConsumer(keyboard_consumer::identifier id, u32 max_connect);
	keyboard_consumer& GetConsumer(keyboard_consumer::identifier id);
	void RemoveConsumer(keyboard_consumer::identifier id);

	bool HandleKey(u32 qt_code, u32 native_code, bool pressed, bool is_auto_repeat, const std::u32string& key);
	void SetIntercepted(bool intercepted);

	stx::init_mutex init;

protected:
	void ReleaseAllKeys();

	bool m_keys_released = false;
	std::unordered_map<keyboard_consumer::identifier, keyboard_consumer> m_consumers;
};
