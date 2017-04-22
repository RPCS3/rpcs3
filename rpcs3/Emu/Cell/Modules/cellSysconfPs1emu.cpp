#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellSysconfPs1emu("cellSysconfPs1emu", logs::level::notice);

s32 cellSysconfPs1emu_639ABBDE()
{
	UNIMPLEMENTED_FUNC(cellSysconfPs1emu);
	return CELL_OK;
}

s32 cellSysconfPs1emu_6A12D11F()
{
	UNIMPLEMENTED_FUNC(cellSysconfPs1emu);
	return CELL_OK;
}

s32 cellSysconfPs1emu_83E79A23()
{
	UNIMPLEMENTED_FUNC(cellSysconfPs1emu);
	return CELL_OK;
}

s32 cellSysconfPs1emu_EFDDAF6C()
{
	UNIMPLEMENTED_FUNC(cellSysconfPs1emu);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysconfPs1emu)("cellSysconfPs1emu", []()
{
	REG_FNID(cellSysconfPs1emu, 0x639ABBDE, cellSysconfPs1emu_639ABBDE);
	REG_FNID(cellSysconfPs1emu, 0x6A12D11F, cellSysconfPs1emu_6A12D11F);
	REG_FNID(cellSysconfPs1emu, 0x83E79A23, cellSysconfPs1emu_83E79A23);
	REG_FNID(cellSysconfPs1emu, 0xEFDDAF6C, cellSysconfPs1emu_EFDDAF6C);
});
