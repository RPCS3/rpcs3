#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Io/KeyboardHandler.h"
#include "cellKb.h"

error_code sys_config_start(ppu_thread& ppu);
error_code sys_config_stop(ppu_thread& ppu);

extern bool is_input_allowed();

LOG_CHANNEL(sys_io);

template<>
void fmt_class_string<CellKbError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_KB_ERROR_FATAL);
			STR_CASE(CELL_KB_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_KB_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_KB_ERROR_UNINITIALIZED);
			STR_CASE(CELL_KB_ERROR_RESOURCE_ALLOCATION_FAILED);
			STR_CASE(CELL_KB_ERROR_READ_FAILED);
			STR_CASE(CELL_KB_ERROR_NO_DEVICE);
			STR_CASE(CELL_KB_ERROR_SYS_SETTING_FAILED);
		}

		return unknown;
	});
}


KeyboardHandlerBase::KeyboardHandlerBase(utils::serial* ar)
{
	if (!ar)
	{
		return;
	}

	u32 max_connect = 0;

	(*ar)(max_connect);

	if (max_connect)
	{
		Emu.PostponeInitCode([this, max_connect]()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			AddConsumer(keyboard_consumer::identifier::cellKb, max_connect);
			auto lk = init.init();
		});
	}
}

void KeyboardHandlerBase::save(utils::serial& ar)
{
	u32 max_connect = 0;
	const auto inited = init.access();

	if (inited)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (auto it = m_consumers.find(keyboard_consumer::identifier::cellKb); it != m_consumers.end())
		{
			max_connect = it->second.GetInfo().max_connect;
		}
	}

	ar(max_connect);
}

error_code cellKbInit(ppu_thread& ppu, u32 max_connect)
{
	sys_io.warning("cellKbInit(max_connect=%d)", max_connect);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	auto init = handler.init.init();

	if (!init)
		return CELL_KB_ERROR_ALREADY_INITIALIZED;

	if (max_connect == 0 || max_connect > CELL_KB_MAX_KEYBOARDS)
	{
		init.cancel();
		return CELL_KB_ERROR_INVALID_PARAMETER;
	}

	sys_config_start(ppu);

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	handler.AddConsumer(keyboard_consumer::identifier::cellKb, std::min(max_connect, 7u));

	return CELL_OK;
}

error_code cellKbEnd(ppu_thread& ppu)
{
	sys_io.notice("cellKbEnd()");

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.reset();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	{
		std::lock_guard<std::mutex> lock(handler.m_mutex);
		handler.RemoveConsumer(keyboard_consumer::identifier::cellKb);
	}

	// TODO
	sys_config_stop(ppu);
	return CELL_OK;
}

error_code cellKbClearBuf(u32 port_no)
{
	sys_io.trace("cellKbClearBuf(port_no=%d)", port_no);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler.m_mutex);

	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);
	const KbInfo& current_info = consumer.GetInfo();

	if (port_no >= consumer.GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return not_an_error(CELL_KB_ERROR_NO_DEVICE);

	KbData& current_data = consumer.GetData(port_no);
	current_data.len = 0;
	current_data.led = 0;
	current_data.mkey = 0;

	for (int i = 0; i < CELL_KB_MAX_KEYCODES; i++)
	{
		current_data.buttons[i] = KbButton(CELL_KEYC_NO_EVENT, 0, false);
	}

	return CELL_OK;
}

u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode)
{
	sys_io.trace("cellKbCnvRawCode(arrange=%d, mkey=%d, led=%d, rawcode=0x%x)", arrange, mkey, led, rawcode);

	// CELL_KB_RAWDAT
	if (rawcode <= CELL_KEYC_E_UNDEF ||
		rawcode == CELL_KEYC_ESCAPE ||
		rawcode == CELL_KEYC_106_KANJI ||
		(rawcode >= CELL_KEYC_CAPS_LOCK && rawcode <= CELL_KEYC_NUM_LOCK) ||
		rawcode == CELL_KEYC_APPLICATION ||
		rawcode == CELL_KEYC_KANA ||
		rawcode == CELL_KEYC_HENKAN ||
		rawcode == CELL_KEYC_MUHENKAN)
	{
		return rawcode | CELL_KB_RAWDAT;
	}

	const bool is_alt = mkey & (CELL_KB_MKEY_L_ALT | CELL_KB_MKEY_R_ALT);
	const bool is_shift = mkey & (CELL_KB_MKEY_L_SHIFT | CELL_KB_MKEY_R_SHIFT);
	const bool is_caps_lock = led & (CELL_KB_LED_CAPS_LOCK);
	const bool is_num_lock = led & (CELL_KB_LED_NUM_LOCK);

	// CELL_KB_NUMPAD

	if (is_num_lock)
	{
		//if (rawcode == CELL_KEYC_KPAD_NUMLOCK)  return 0x00 | CELL_KB_KEYPAD;                                     // 'Num Lock' (unreachable)
		if (rawcode == CELL_KEYC_KPAD_SLASH)    return 0x2F | CELL_KB_KEYPAD;                                     // '/'
		if (rawcode == CELL_KEYC_KPAD_ASTERISK) return 0x2A | CELL_KB_KEYPAD;                                     // '*'
		if (rawcode == CELL_KEYC_KPAD_MINUS)    return 0x2D | CELL_KB_KEYPAD;                                     // '-'
		if (rawcode == CELL_KEYC_KPAD_PLUS)     return 0x2B | CELL_KB_KEYPAD;                                     // '+'
		if (rawcode == CELL_KEYC_KPAD_ENTER)    return 0x0A | CELL_KB_KEYPAD;                                     // '\n'
		if (rawcode == CELL_KEYC_KPAD_0)        return 0x30 | CELL_KB_KEYPAD;                                     // '0'
		if (rawcode >= CELL_KEYC_KPAD_1 && rawcode <= CELL_KEYC_KPAD_9) return (rawcode - 0x28) | CELL_KB_KEYPAD; // '1' - '9'
	}

	// ASCII

	const auto get_ascii = [is_alt, is_shift, is_caps_lock](u16 raw, u16 shifted = 0, u16 altered = 0)
	{
		if ((is_shift || is_caps_lock) && shifted)
		{
			return shifted;
		}
		else if (is_alt && altered)
		{
			return altered;
		}
		return raw;
	};

	if (arrange == CELL_KB_MAPPING_106) // (Japanese)
	{
		if (rawcode == CELL_KEYC_1) return get_ascii('1', '!');
		if (rawcode == CELL_KEYC_2) return get_ascii('2', '"');
		if (rawcode == CELL_KEYC_3) return get_ascii('3', '#');
		if (rawcode == CELL_KEYC_4) return get_ascii('4', '$');
		if (rawcode == CELL_KEYC_5) return get_ascii('5', '%');
		if (rawcode == CELL_KEYC_6) return get_ascii('6', '&');
		if (rawcode == CELL_KEYC_7) return get_ascii('7', '\'');
		if (rawcode == CELL_KEYC_8) return get_ascii('8', '(');
		if (rawcode == CELL_KEYC_9) return get_ascii('9', ')');
		if (rawcode == CELL_KEYC_0) return get_ascii('0', '~');

		if (rawcode == CELL_KEYC_ACCENT_CIRCONFLEX_106) return get_ascii('^', '~');
		if (rawcode == CELL_KEYC_ATMARK_106)            return get_ascii('@', '`');
		if (rawcode == CELL_KEYC_LEFT_BRACKET_106)      return get_ascii('[', '{');
		if (rawcode == CELL_KEYC_RIGHT_BRACKET_106)     return get_ascii(']', '}');
		if (rawcode == CELL_KEYC_SEMICOLON)             return get_ascii(';', '+');
		if (rawcode == CELL_KEYC_COLON_106)             return get_ascii(':', '*');
		if (rawcode == CELL_KEYC_COMMA)                 return get_ascii(',', '<');
		if (rawcode == CELL_KEYC_PERIOD)                return get_ascii('.', '>');
		if (rawcode == CELL_KEYC_SLASH)                 return get_ascii('/', '?');
		if (rawcode == CELL_KEYC_BACKSLASH_106)         return get_ascii('\\', '_');
		if (rawcode == CELL_KEYC_YEN_106)               return get_ascii(190, '|'); // ¥
	}
	else if (arrange == CELL_KB_MAPPING_101) // (US)
	{
		if (rawcode == CELL_KEYC_1) return get_ascii('1', '!');
		if (rawcode == CELL_KEYC_2) return get_ascii('2', '@');
		if (rawcode == CELL_KEYC_3) return get_ascii('3', '#');
		if (rawcode == CELL_KEYC_4) return get_ascii('4', '$');
		if (rawcode == CELL_KEYC_5) return get_ascii('5', '%');
		if (rawcode == CELL_KEYC_6) return get_ascii('6', '^');
		if (rawcode == CELL_KEYC_7) return get_ascii('7', '&');
		if (rawcode == CELL_KEYC_8) return get_ascii('8', '*');
		if (rawcode == CELL_KEYC_9) return get_ascii('9', '(');
		if (rawcode == CELL_KEYC_0) return get_ascii('0', ')');

		if (rawcode == CELL_KEYC_MINUS)             return get_ascii('-', '_');
		if (rawcode == CELL_KEYC_EQUAL_101)         return get_ascii('=', '+');
		if (rawcode == CELL_KEYC_LEFT_BRACKET_101)  return get_ascii('[', '{');
		if (rawcode == CELL_KEYC_RIGHT_BRACKET_101) return get_ascii(']', '}');
		if (rawcode == CELL_KEYC_BACKSLASH_101)     return get_ascii('\\', '|');
		if (rawcode == CELL_KEYC_SEMICOLON)         return get_ascii(';', ':');
		if (rawcode == CELL_KEYC_QUOTATION_101)     return get_ascii('\'', '"');
		if (rawcode == CELL_KEYC_COMMA)             return get_ascii(',', '<');
		if (rawcode == CELL_KEYC_PERIOD)            return get_ascii('.', '>');
		if (rawcode == CELL_KEYC_SLASH)             return get_ascii('/', '?');
		if (rawcode == CELL_KEYC_BACK_QUOTE)        return get_ascii('`', '~');
	}
	else if (arrange == CELL_KB_MAPPING_GERMAN_GERMANY)
	{
		if (rawcode == CELL_KEYC_1) return get_ascii('1', '!');
		if (rawcode == CELL_KEYC_2) return get_ascii('2', '"');
		if (rawcode == CELL_KEYC_3) return get_ascii('3', 245); // §
		if (rawcode == CELL_KEYC_4) return get_ascii('4', '$');
		if (rawcode == CELL_KEYC_5) return get_ascii('5', '%');
		if (rawcode == CELL_KEYC_6) return get_ascii('6', '&');
		if (rawcode == CELL_KEYC_7) return get_ascii('7', '/', '{');
		if (rawcode == CELL_KEYC_8) return get_ascii('8', '(', '[');
		if (rawcode == CELL_KEYC_9) return get_ascii('9', ')', ']');
		if (rawcode == CELL_KEYC_0) return get_ascii('0', '=', '}');

		if (rawcode == CELL_KEYC_MINUS)                 return get_ascii('-', '_');
		if (rawcode == CELL_KEYC_ACCENT_CIRCONFLEX_106) return get_ascii('^', 248); // °
		if (rawcode == CELL_KEYC_COMMA)                 return get_ascii(',', ';');
		if (rawcode == CELL_KEYC_PERIOD)                return get_ascii('.', ':');
		if (rawcode == CELL_KEYC_KPAD_PLUS)             return get_ascii('+', '*', '~');
		if (rawcode == CELL_KEYC_LESS)                  return get_ascii('<', '>', '|');
		if (rawcode == CELL_KEYC_HASHTAG)               return get_ascii('#', '\'');
		if (rawcode == CELL_KEYC_SSHARP)                return get_ascii(225, '?', '\\'); // ß
		if (rawcode == CELL_KEYC_BACK_QUOTE)            return get_ascii(239, '`'); // ´
		if (rawcode == CELL_KEYC_Q)                     return get_ascii('q', 'Q', '@');
	}

	if (rawcode >= CELL_KEYC_A && rawcode <= CELL_KEYC_Z) // 'A' - 'Z'
	{
		rawcode -=
			(is_shift)
				? ((led & (CELL_KB_LED_CAPS_LOCK)) ? 0 : 0x20)
				: ((led & (CELL_KB_LED_CAPS_LOCK)) ? 0x20 : 0);
		return rawcode + 0x5D;
	}
	if (rawcode >= CELL_KEYC_1 && rawcode <= CELL_KEYC_9) return rawcode + 0x13; // '1' - '9'
	if (rawcode == CELL_KEYC_0) return 0x30;                                     // '0'
	if (rawcode == CELL_KEYC_ENTER) return 0x0A;                                 // '\n'
	//if (rawcode == CELL_KEYC_ESC) return 0x1B;                                   // 'ESC' (unreachable)
	if (rawcode == CELL_KEYC_BS) return 0x08;                                    // '\b'
	if (rawcode == CELL_KEYC_TAB) return 0x09;                                   // '\t'
	if (rawcode == CELL_KEYC_SPACE) return 0x20;                                 // 'space'

	// TODO: Add more keys and layouts

	return 0x0000;
}

error_code cellKbGetInfo(vm::ptr<CellKbInfo> info)
{
	sys_io.trace("cellKbGetInfo(info=*0x%x)", info);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (!info)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::memset(info.get_ptr(), 0, info.size());

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);

	const KbInfo& current_info = consumer.GetInfo();
	info->max_connect = current_info.max_connect;
	info->now_connect = current_info.now_connect;
	info->info = current_info.info;

	for (u32 i = 0; i < CELL_KB_MAX_KEYBOARDS; i++)
	{
		info->status[i] = current_info.status[i];
	}

	return CELL_OK;
}

error_code cellKbRead(u32 port_no, vm::ptr<CellKbData> data)
{
	sys_io.trace("cellKbRead(port_no=%d, data=*0x%x)", port_no, data);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || !data)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);
	const KbInfo& current_info = consumer.GetInfo();

	if (port_no >= consumer.GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return not_an_error(CELL_KB_ERROR_NO_DEVICE);

	KbData& current_data = consumer.GetData(port_no);

	if (current_info.is_null_handler || (current_info.info & CELL_KB_INFO_INTERCEPTED) || !is_input_allowed())
	{
		data->led = 0;
		data->mkey = 0;
		data->len = 0;
	}
	else
	{
		data->led = current_data.led;
		data->mkey = current_data.mkey;
		data->len = std::min<s32>(CELL_KB_MAX_KEYCODES, current_data.len);
	}

	if (current_data.len > 0)
	{
		for (s32 i = 0; i < current_data.len; i++)
		{
			data->keycode[i] = current_data.buttons[i].m_keyCode;
		}

		KbConfig& current_config = consumer.GetConfig(port_no);

		// For single character mode to work properly we need to "flush" the buffer after reading or else we'll constantly get the same key presses with each call.
		// Actual key repeats are handled by adding a new key code to the buffer periodically. Key releases are handled in a similar fashion.
		// Warning: Don't do this in packet mode, which is basically the mouse and keyboard gaming mode. Otherwise games like Unreal Tournament will be unplayable.
		if (current_config.read_mode == CELL_KB_RMODE_INPUTCHAR)
		{
			current_data.len = 0;
		}
	}

	return CELL_OK;
}

error_code cellKbSetCodeType(u32 port_no, u32 type)
{
	sys_io.trace("cellKbSetCodeType(port_no=%d, type=%d)", port_no, type);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || type > CELL_KB_CODETYPE_ASCII)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);

	if (port_no >= consumer.GetKeyboards().size())
		return CELL_OK;

	KbConfig& current_config = consumer.GetConfig(port_no);
	current_config.code_type = type;

	// can also return CELL_KB_ERROR_SYS_SETTING_FAILED

	return CELL_OK;
}

error_code cellKbSetLEDStatus(u32 port_no, u8 led)
{
	sys_io.trace("cellKbSetLEDStatus(port_no=%d, led=%d)", port_no, led);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	if (led > 7)
		return CELL_KB_ERROR_SYS_SETTING_FAILED;

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);

	if (port_no >= consumer.GetKeyboards().size() || consumer.GetInfo().status[port_no] != CELL_KB_STATUS_CONNECTED)
		return CELL_KB_ERROR_FATAL;

	KbData& current_data = consumer.GetData(port_no);
	current_data.led = static_cast<u32>(led);

	return CELL_OK;
}

error_code cellKbSetReadMode(u32 port_no, u32 rmode)
{
	sys_io.trace("cellKbSetReadMode(port_no=%d, rmode=%d)", port_no, rmode);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || rmode > CELL_KB_RMODE_PACKET)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler.m_mutex);
	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);

	if (port_no >= consumer.GetKeyboards().size())
		return CELL_OK;

	KbConfig& current_config = consumer.GetConfig(port_no);
	current_config.read_mode = rmode;

	// Key repeat must be disabled in packet mode. But let's just always enable it otherwise.
	Keyboard& keyboard = consumer.GetKeyboards()[port_no];
	keyboard.m_key_repeat = rmode != CELL_KB_RMODE_PACKET;

	// can also return CELL_KB_ERROR_SYS_SETTING_FAILED

	return CELL_OK;
}

error_code cellKbGetConfiguration(u32 port_no, vm::ptr<CellKbConfig> config)
{
	sys_io.trace("cellKbGetConfiguration(port_no=%d, config=*0x%x)", port_no, config);

	auto& handler = g_fxo->get<KeyboardHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler.m_mutex);

	keyboard_consumer& consumer = handler.GetConsumer(keyboard_consumer::identifier::cellKb);

	const KbInfo& current_info = consumer.GetInfo();

	if (port_no >= consumer.GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return not_an_error(CELL_KB_ERROR_NO_DEVICE);

	// tests show that config is checked only after the device's status
	if (!config)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	const KbConfig& current_config = consumer.GetConfig(port_no);
	config->arrange = current_config.arrange;
	config->read_mode = current_config.read_mode;
	config->code_type = current_config.code_type;

	// can also return CELL_KB_ERROR_SYS_SETTING_FAILED

	return CELL_OK;
}

void cellKb_init()
{
	REG_FUNC(sys_io, cellKbInit);
	REG_FUNC(sys_io, cellKbEnd);
	REG_FUNC(sys_io, cellKbClearBuf);
	REG_FUNC(sys_io, cellKbCnvRawCode);
	REG_FUNC(sys_io, cellKbGetInfo);
	REG_FUNC(sys_io, cellKbRead);
	REG_FUNC(sys_io, cellKbSetCodeType);
	REG_FUNC(sys_io, cellKbSetLEDStatus);
	REG_FUNC(sys_io, cellKbSetReadMode);
	REG_FUNC(sys_io, cellKbGetConfiguration);
}
