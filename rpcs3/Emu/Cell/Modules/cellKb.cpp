#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Io/KeyboardHandler.h"
#include "cellKb.h"



extern logs::channel sys_io;

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

error_code cellKbInit(u32 max_connect)
{
	sys_io.warning("cellKbInit(max_connect=%d)", max_connect);

	auto handler = fxm::get<KeyboardHandlerBase>();

	if (handler)
		return CELL_KB_ERROR_ALREADY_INITIALIZED;

	if (max_connect == 0 || max_connect > CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	handler = fxm::import<KeyboardHandlerBase>(Emu.GetCallbacks().get_kb_handler);
	handler->Init(std::min(max_connect, 7u));

	return CELL_OK;
}

error_code cellKbEnd()
{
	sys_io.notice("cellKbEnd()");

	if (!fxm::remove<KeyboardHandlerBase>())
		return CELL_KB_ERROR_UNINITIALIZED;

	return CELL_OK;
}

error_code cellKbClearBuf(u32 port_no)
{
	sys_io.trace("cellKbClearBuf(port_no=%d)", port_no);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	const KbInfo& current_info = handler->GetInfo();

	if (port_no >= handler->GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return CELL_KB_ERROR_NO_DEVICE;

	KbData& current_data = handler->GetData(port_no);
	current_data.len = 0;
	current_data.led = 0;
	current_data.mkey = 0;

	for (int i = 0; i < CELL_KB_MAX_KEYCODES; i++)
	{
		current_data.keycode[i] = { 0, 0 };
	}

	return CELL_OK;
}

u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode)
{
	sys_io.trace("cellKbCnvRawCode(arrange=%d, mkey=%d, led=%d, rawcode=%d)", arrange, mkey, led, rawcode);

	// CELL_KB_RAWDAT
	if (rawcode <= 0x03 || rawcode == 0x29 || rawcode == 0x35 || (rawcode >= 0x39 && rawcode <= 0x53) || rawcode == 0x65 || rawcode == 0x88 || rawcode == 0x8A || rawcode == 0x8B)
	{
		return rawcode | 0x8000;
	}

	// CELL_KB_NUMPAD
	if (rawcode >= 0x59 && rawcode <= 0x61) return (rawcode - 0x28) | 0x4000; // '1' - '9'
	if (rawcode == 0x62) return 0x30 | 0x4000;                                // '0'
	if (rawcode == 0x53) return 0x00 | 0x4000;                                // 'Num Lock'
	if (rawcode == 0x54) return 0x2F | 0x4000;                                // '/'
	if (rawcode == 0x55) return 0x2A | 0x4000;                                // '*'
	if (rawcode == 0x56) return 0x2D | 0x4000;                                // '-'
	if (rawcode == 0x57) return 0x2B | 0x4000;                                // '+'
	if (rawcode == 0x58) return 0x0A | 0x4000;                                // '\n'

	// ASCII

	const bool is_shift = mkey & (CELL_KB_MKEY_L_SHIFT | CELL_KB_MKEY_R_SHIFT);
	const bool is_caps_lock = led & (CELL_KB_LED_CAPS_LOCK);

	auto get_ascii = [is_shift, is_caps_lock](u16 lower, u16 upper)
	{
		return is_shift || is_caps_lock ? upper : lower;
	};

	if (arrange == CELL_KB_MAPPING_106) // (Japanese)
	{
		if (rawcode == 0x1E) return get_ascii(rawcode + 0x13, 0x21);            // '1' or '!'
		if (rawcode == 0x1F) return get_ascii(rawcode + 0x13, 0x22);            // '2' or '"'
		if (rawcode == 0x20) return get_ascii(rawcode + 0x13, 0x23);            // '3' or '#'
		if (rawcode == 0x21) return get_ascii(rawcode + 0x13, 0x24);            // '4' or '$'
		if (rawcode == 0x22) return get_ascii(rawcode + 0x13, 0x25);            // '5' or '%'
		if (rawcode == 0x23) return get_ascii(rawcode + 0x13, 0x26);            // '6' or '&'
		if (rawcode == 0x24) return get_ascii(rawcode + 0x13, 0x27);            // '7' or '''
		if (rawcode == 0x25) return get_ascii(rawcode + 0x13, 0x28);            // '8' or '('
		if (rawcode == 0x26) return get_ascii(rawcode + 0x13, 0x29);            // '9' or ')'
		if (rawcode == 0x27) return get_ascii(0x303, 0x7E);                     // '0' or '~'

		if (rawcode == 0x2E) return get_ascii(0x5E, 0x7E);                      // '^' or '~'
		if (rawcode == 0x2F) return get_ascii(0x40, 0x60);                      // '@' or '`'
		if (rawcode == 0x30) return get_ascii(0x5B, 0x7B);                      // '[' or '{'
		if (rawcode == 0x32) return get_ascii(0x5D, 0x7D);                      // ']' or '}'
		if (rawcode == 0x33) return get_ascii(0x3B, 0x2B);                      // ';' or '+'
		if (rawcode == 0x34) return get_ascii(0x3A, 0x2A);                      // ':' or '*'
		if (rawcode == 0x87) return get_ascii(rawcode, 0x5F);                   // '\' or '_'
		if (rawcode == 0x36) return get_ascii(0x2C, 0x3C);                      // ',' or '<'
		if (rawcode == 0x37) return get_ascii(0x2E, 0x3E);                      // '.' or '>'
		if (rawcode == 0x38) return get_ascii(0x2F, 0x3F);                      // '/' or '?'
		if (rawcode == 0x89) return get_ascii(0xBE, 0x7C);                      // '&yen;' or '|'
	}
	else if (arrange == CELL_KB_MAPPING_101) // (US)
	{
		if (rawcode == 0x1E) return get_ascii(rawcode + 0x13, 0x21);            // '1' or '!'
		if (rawcode == 0x1F) return get_ascii(rawcode + 0x13, 0x40);            // '2' or '@'
		if (rawcode == 0x20) return get_ascii(rawcode + 0x13, 0x23);            // '3' or '#'
		if (rawcode == 0x21) return get_ascii(rawcode + 0x13, 0x24);            // '4' or '$'
		if (rawcode == 0x22) return get_ascii(rawcode + 0x13, 0x25);            // '5' or '%'
		if (rawcode == 0x23) return get_ascii(rawcode + 0x13, 0x5E);            // '6' or '^'
		if (rawcode == 0x24) return get_ascii(rawcode + 0x13, 0x26);            // '7' or '&'
		if (rawcode == 0x25) return get_ascii(rawcode + 0x13, 0x2A);            // '8' or '*'
		if (rawcode == 0x26) return get_ascii(rawcode + 0x13, 0x28);            // '9' or '('
		if (rawcode == 0x27) return get_ascii(0x303, 0x29);                     // '0' or ')'

		if (rawcode == 0x2D) return get_ascii(0x2D, 0x5F);                      // '-' or '_'
		if (rawcode == 0x2E) return get_ascii(0x3D, 0x2B);                      // '=' or '+'
		if (rawcode == 0x2F) return get_ascii(0x5B, 0x7B);                      // '[' or '{'
		if (rawcode == 0x30) return get_ascii(0x5D, 0x7D);                      // ']' or '}'
		if (rawcode == 0x31) return get_ascii(0x5C, 0x7C);                      // '\' or '|'
		if (rawcode == 0x33) return get_ascii(0x3B, 0x3A);                      // ';' or ':'
		if (rawcode == 0x34) return get_ascii(0x27, 0x22);                      // ''' or '"'
		if (rawcode == 0x36) return get_ascii(0x2C, 0x3C);                      // ',' or '<'
		if (rawcode == 0x37) return get_ascii(0x2E, 0x3E);                      // '.' or '>'
		if (rawcode == 0x38) return get_ascii(0x2F, 0x3F);                      // '/' or '?'
	}
	else if (arrange == CELL_KB_MAPPING_GERMAN_GERMANY)
	{
		if (rawcode == 0x1E) return get_ascii(rawcode + 0x13, 0x21);            // '1' or '!'
		if (rawcode == 0x1F) return get_ascii(rawcode + 0x13, 0x22);            // '2' or '"'
		if (rawcode == 0x20) return rawcode + 0x13;                             // '3' (or '�')
		if (rawcode == 0x21) return get_ascii(rawcode + 0x13, 0x24);            // '4' or '$'
		if (rawcode == 0x22) return get_ascii(rawcode + 0x13, 0x25);            // '5' or '%'
		if (rawcode == 0x23) return get_ascii(rawcode + 0x13, 0x26);            // '6' or '&'
		if (rawcode == 0x24) return get_ascii(rawcode + 0x13, 0x2F);            // '7' or '/'
		if (rawcode == 0x25) return get_ascii(rawcode + 0x13, 0x28);            // '8' or '('
		if (rawcode == 0x26) return get_ascii(rawcode + 0x13, 0x29);            // '9' or ')'
		if (rawcode == 0x27) return get_ascii(0x303, 0x3D);                     // '0' or '='

		if (rawcode == 0x2D) return get_ascii(0x2D, 0x5F);                      // '-' or '_'
		if (rawcode == 0x2E) return 0x5E;                                       // '^' (or '�')
		if (rawcode == 0x36) return get_ascii(0x2C, 0x3B);                      // ',' or ';'
		if (rawcode == 0x37) return get_ascii(0x2E, 0x3A);                      // '.' or ':'

		// TODO: <>#'+*~[]{}\|
	}

	if (rawcode >= 0x04 && rawcode <= 0x1D)                                   // 'A' - 'Z'
	{
		rawcode -=
			(is_shift)
				? ((led & (CELL_KB_LED_CAPS_LOCK)) ? 0 : 0x20)
				: ((led & (CELL_KB_LED_CAPS_LOCK)) ? 0x20 : 0);
		return rawcode + 0x5D;
	}
	if (rawcode >= 0x1E && rawcode <= 0x26) return rawcode + 0x13;            // '1' - '9'
	if (rawcode == 0x27) return 0x30;                                         // '0'
	if (rawcode == 0x28) return 0x0A;                                         // '\n'
	if (rawcode == 0x29) return 0x1B;                                         // 'ESC'
	if (rawcode == 0x2A) return 0x08;                                         // '\b'
	if (rawcode == 0x2B) return 0x09;                                         // '\t'
	if (rawcode == 0x2C) return 0x20;                                         // 'space'

	// TODO: Add more cases (e.g. what about '`' and '~' on english layouts) and layouts (e.g. german)

	return 0x0000;
}

error_code cellKbGetInfo(vm::ptr<CellKbInfo> info)
{
	sys_io.trace("cellKbGetInfo(info=*0x%x)", info);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (!info)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	const KbInfo& current_info = handler->GetInfo();
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

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || !data)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	const KbInfo& current_info = handler->GetInfo();

	if (port_no >= handler->GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return CELL_KB_ERROR_NO_DEVICE;

	KbData& current_data = handler->GetData(port_no);
	data->led = current_data.led;
	data->mkey = current_data.mkey;
	data->len = std::min((s32)CELL_KB_MAX_KEYCODES, current_data.len);

	for (s32 i = 0; i < current_data.len; i++)
	{
		data->keycode[i] = current_data.keycode[i].first;
	}

	return CELL_OK;
}

error_code cellKbSetCodeType(u32 port_no, u32 type)
{
	sys_io.trace("cellKbSetCodeType(port_no=%d, type=%d)", port_no, type);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || type > CELL_KB_CODETYPE_ASCII)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	if (port_no >= handler->GetKeyboards().size())
		return CELL_OK;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	KbConfig& current_config = handler->GetConfig(port_no);
	current_config.code_type = type;

	// can also return CELL_KB_ERROR_SYS_SETTING_FAILED

	return CELL_OK;
}

error_code cellKbSetLEDStatus(u32 port_no, u8 led)
{
	sys_io.trace("cellKbSetLEDStatus(port_no=%d, led=%d)", port_no, led);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	if (led > 7)
		return CELL_KB_ERROR_SYS_SETTING_FAILED;

	if (port_no >= handler->GetKeyboards().size() || handler->GetInfo().status[port_no] != CELL_KB_STATUS_CONNECTED)
		return CELL_KB_ERROR_FATAL;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	KbData& current_data = handler->GetData(port_no);
	current_data.led = static_cast<u32>(led);

	return CELL_OK;
}

error_code cellKbSetReadMode(u32 port_no, u32 rmode)
{
	sys_io.trace("cellKbSetReadMode(port_no=%d, rmode=%d)", port_no, rmode);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS || rmode > CELL_KB_RMODE_PACKET)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	if (port_no >= handler->GetKeyboards().size())
		return CELL_OK;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	KbConfig& current_config = handler->GetConfig(port_no);
	current_config.read_mode = rmode;

	// can also return CELL_KB_ERROR_SYS_SETTING_FAILED

	return CELL_OK;
}

error_code cellKbGetConfiguration(u32 port_no, vm::ptr<CellKbConfig> config)
{
	sys_io.trace("cellKbGetConfiguration(port_no=%d, config=*0x%x)", port_no, config);

	const auto handler = fxm::get<KeyboardHandlerBase>();

	if (!handler)
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= CELL_KB_MAX_KEYBOARDS)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	std::lock_guard<std::mutex> lock(handler->m_mutex);

	const KbInfo& current_info = handler->GetInfo();

	if (port_no >= handler->GetKeyboards().size() || current_info.status[port_no] != CELL_KB_STATUS_CONNECTED)
		return CELL_KB_ERROR_NO_DEVICE;

	// tests show that config is checked only after the device's status
	if (!config)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	const KbConfig& current_config = handler->GetConfig(port_no);
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
