#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"



logs::channel cellKey2char("cellKey2char");

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

struct CellKey2CharKeyData
{
	be_t<u32> led;
	be_t<u32> mkey;
	be_t<u16> keycode;
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

error_code cellKey2CharOpen(vm::ptr<u8> handle)
{
	cellKey2char.todo("cellKey2CharOpen(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellKey2CharClose(vm::ptr<u8> handle)
{
	cellKey2char.todo("cellKey2CharClose(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellKey2CharGetChar(vm::ptr<u8> handle, vm::ptr<CellKey2CharKeyData> kdata, vm::pptr<u16> charCode, vm::ptr<u32> charNum, vm::ptr<b8> processed)
{
	cellKey2char.todo("cellKey2CharGetChar(handle=*0x%x, kdata=*0x%x, charCode=**0x%x, charNum=*0x%x, processed=*0x%x)", handle, kdata, charCode, charNum, processed);
	return CELL_OK;
}

error_code cellKey2CharSetMode(vm::ptr<u8> handle, s32 mode)
{
	cellKey2char.todo("cellKey2CharSetMode(handle=*0x%x, mode=0x%x)", handle, mode);
	return CELL_OK;
}

error_code cellKey2CharSetArrangement(vm::ptr<u8> handle, s32 arrange)
{
	cellKey2char.todo("cellKey2CharSetArrangement(handle=*0x%x, arrange=0x%x)", handle, arrange);
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
