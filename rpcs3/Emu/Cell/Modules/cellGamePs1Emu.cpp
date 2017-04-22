#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellGamePs1Emu("cellGamePs1Emu", logs::level::notice);

s32 cellGamePs1Emu_61CE2BCD()
{
	UNIMPLEMENTED_FUNC(cellGamePs1Emu);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellGamePs1Emu)("cellGamePs1Emu", []()
{
	REG_FNID(cellGamePs1Emu, 0x61CE2BCD, cellGamePs1Emu_61CE2BCD);
});
