#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

logs::channel libad_billboard_util("libad_billboard_util");

s32 libad_billboard_util_47BC7BED() //Pain
{
	libad_billboard_util.todo("libad_billboard_util_47BC7BED");
	return CELL_OK;
}


DECLARE(ppu_module_manager::libad_billboard_util)("libad_billboard_util", []()
{
	REG_FNID(libad_billboard_util, 0x47BC7BED, libad_billboard_util_47BC7BED);
});
