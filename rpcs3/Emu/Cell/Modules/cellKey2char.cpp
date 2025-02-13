#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Io/Keyboard.h"

LOG_CHANNEL(cellKey2char);

// Return Codes
enum CellKey2CharError : u32
{
	CELL_K2C_ERROR_FATAL               = 0x80121301,
	CELL_K2C_ERROR_INVALID_HANDLE      = 0x80121302,
	CELL_K2C_ERROR_INVALID_PARAMETER   = 0x80121303,
	CELL_K2C_ERROR_ALREADY_INITIALIZED = 0x80121304,
	CELL_K2C_ERROR_UNINITIALIZED       = 0x80121305,
	CELL_K2C_ERROR_OTHER               = 0x80121306,
};

// Modes
enum
{
	CELL_KEY2CHAR_MODE_ENGLISH = 0,
	CELL_KEY2CHAR_MODE_NATIVE  = 1,
	CELL_KEY2CHAR_MODE_NATIVE2 = 2
};

// Constants
enum
{
	SCE_KEY2CHAR_HANDLE_SIZE = 128
};

struct CellKey2CharKeyData
{
	be_t<u32> led;
	be_t<u32> mkey;
	be_t<u16> keycode;
};

struct CellKey2CharHandle
{
	u8 data[SCE_KEY2CHAR_HANDLE_SIZE];
};

template<>
void fmt_class_string<CellKey2CharError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_K2C_ERROR_FATAL);
			STR_CASE(CELL_K2C_ERROR_INVALID_HANDLE);
			STR_CASE(CELL_K2C_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_K2C_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_K2C_ERROR_UNINITIALIZED);
			STR_CASE(CELL_K2C_ERROR_OTHER);
		}

		return unknown;
	});
}

error_code cellKey2CharOpen(vm::ptr<CellKey2CharHandle> handle)
{
	cellKey2char.todo("cellKey2CharOpen(handle=*0x%x)", handle);

	if (!handle)
		return CELL_K2C_ERROR_INVALID_HANDLE;

	if (handle->data[8] != 0)
		return CELL_K2C_ERROR_ALREADY_INITIALIZED;

	// TODO

	return CELL_OK;
}

error_code cellKey2CharClose(vm::ptr<CellKey2CharHandle> handle)
{
	cellKey2char.todo("cellKey2CharClose(handle=*0x%x)", handle);

	if (!handle)
		return CELL_K2C_ERROR_INVALID_HANDLE;

	if (handle->data[8] == 0)
		return CELL_K2C_ERROR_UNINITIALIZED;

	// TODO

	return CELL_OK;
}

error_code cellKey2CharGetChar(vm::ptr<CellKey2CharHandle> handle, vm::ptr<CellKey2CharKeyData> kdata, vm::pptr<u16> charCode, vm::ptr<u32> charNum, vm::ptr<b8> processed)
{
	cellKey2char.todo("cellKey2CharGetChar(handle=*0x%x, kdata=*0x%x, charCode=**0x%x, charNum=*0x%x, processed=*0x%x)", handle, kdata, charCode, charNum, processed);

	if (!handle)
		return CELL_K2C_ERROR_INVALID_HANDLE;

	if (handle->data[8] == 0)
		return CELL_K2C_ERROR_UNINITIALIZED;

	if (!charCode || !kdata || !kdata->keycode)
		return CELL_K2C_ERROR_INVALID_PARAMETER;

	if (handle->data[0] == 255)
	{
		if (false /* some check for CELL_OK */)
			return CELL_K2C_ERROR_OTHER;
	}

	return CELL_OK;
}

error_code cellKey2CharSetMode(vm::ptr<CellKey2CharHandle> handle, s32 mode)
{
	cellKey2char.todo("cellKey2CharSetMode(handle=*0x%x, mode=0x%x)", handle, mode);

	if (!handle)
		return CELL_K2C_ERROR_INVALID_HANDLE;

	if (handle->data[8] == 0)
		return CELL_K2C_ERROR_UNINITIALIZED;

	if (mode > CELL_KEY2CHAR_MODE_NATIVE2)
		return CELL_K2C_ERROR_INVALID_PARAMETER;

	if (handle->data[0] == 255)
		return CELL_K2C_ERROR_OTHER;

	const s32 mapping = handle->data[1];

	switch (mode)
	{
	case CELL_KEY2CHAR_MODE_ENGLISH:
		// TODO: set mode to alphanumeric
		break;
	case CELL_KEY2CHAR_MODE_NATIVE:
		switch (mapping)
		{
		case CELL_KB_MAPPING_106: // Japanese
			// TODO: set mode to kana
			break;
		case CELL_KB_MAPPING_RUSSIAN_RUSSIA:
			// TODO: set mode to Cyrillic
			break;
		case CELL_KB_MAPPING_KOREAN_KOREA:
			// TODO: set mode to Hangul
			break;
		case CELL_KB_MAPPING_CHINESE_TRADITIONAL:
			// TODO: set mode to Bopofomo
			break;
		default:
			break;
		}
		break;
	case CELL_KEY2CHAR_MODE_NATIVE2:
		if (mapping == CELL_KB_MAPPING_CHINESE_TRADITIONAL)
		{
			// TODO: set mode to Cangjie
		}
		break;
	default:
		break; // Unreachable
	}

	return CELL_OK;
}

error_code cellKey2CharSetArrangement(vm::ptr<CellKey2CharHandle> handle, s32 arrange)
{
	cellKey2char.todo("cellKey2CharSetArrangement(handle=*0x%x, arrange=0x%x)", handle, arrange);

	if (!handle)
		return CELL_K2C_ERROR_INVALID_HANDLE;

	if (handle->data[8] == 0)
		return CELL_K2C_ERROR_UNINITIALIZED;

	if (arrange < CELL_KB_MAPPING_101 || arrange > CELL_KB_MAPPING_TURKISH_TURKEY)
		return CELL_K2C_ERROR_INVALID_PARAMETER;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellKey2char)("cellKey2char", []()
{
	REG_FUNC(cellKey2char, cellKey2CharOpen);
	REG_FUNC(cellKey2char, cellKey2CharClose);
	REG_FUNC(cellKey2char, cellKey2CharGetChar);
	REG_FUNC(cellKey2char, cellKey2CharSetMode);
	REG_FUNC(cellKey2char, cellKey2CharSetArrangement);
});
