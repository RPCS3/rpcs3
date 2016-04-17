#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

logs::channel libad_async("libad_async");

s32 libad_async_44946174() //Pain
{
	libad_async.todo("libad_async_44946174");
	return CELL_OK;
}


DECLARE(ppu_module_manager::libad_async)("libad_async", []()
{
	REG_FNID(libad_async, 0x44946174, libad_async_44946174);
});
