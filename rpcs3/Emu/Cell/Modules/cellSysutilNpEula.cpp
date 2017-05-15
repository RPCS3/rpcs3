#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellSysutilNpEula("cellSysutilNpEula");

s32 cellSysutilNpEula_59D1629A() // Resistance 3, Uncharted 2
{
	cellSysutilNpEula.todo("cellSysutilNpEula_59D1629A");
	return CELL_OK;
}

s32 cellSysutilNpEula_5EC05AD8() // Resistance 3
{
	cellSysutilNpEula.todo("cellSysutilNpEula_5EC05AD8");
	return CELL_OK;
}

s32 cellSysutilNpEula_6599500D() // Resistance 3, Uncharted 2
{
	cellSysutilNpEula.todo("cellSysutilNpEula_6599500D");
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellSysutilNpEula)("cellSysutilNpEula", []()
{
	REG_FNID(cellSysutilNpEula, 0x59D1629A, cellSysutilNpEula_59D1629A);
	REG_FNID(cellSysutilNpEula, 0x5EC05AD8, cellSysutilNpEula_5EC05AD8);
	REG_FNID(cellSysutilNpEula, 0x6599500D, cellSysutilNpEula_6599500D);
});
