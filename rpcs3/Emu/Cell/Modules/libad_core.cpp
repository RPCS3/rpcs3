#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

logs::channel libad_core("libad_core");

s32 libad_core_260DB83D() //Pain
{
	libad_core.todo("libad_core_260DB83D");
	return CELL_OK;
}


DECLARE(ppu_module_manager::libad_core)("libad_core", []()
{
	REG_FNID(libad_core, 0x260DB83D, libad_core_260DB83D);
});
