#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellCrossController("cellCrossController");

s32 cellCrossController_37E1F502() // LittleBigPlanet 2 and 3
{
	cellCrossController.todo("cellCrossController_37E1F502");
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellCrossController)("cellCrossController", []()
{
	REG_FNID(cellCrossController, 0x37E1F502, cellCrossController_37E1F502);
});
