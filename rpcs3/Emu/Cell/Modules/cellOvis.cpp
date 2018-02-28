#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_spu.h"



logs::channel cellOvis("cellOvis");

// Return Codes
enum
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

s32 cellOvisGetOverlayTableSize(vm::cptr<char> elf)
{
	cellOvis.todo("cellOvisGetOverlayTableSize(elf=%s)", elf);
	return CELL_OK;
}

s32 cellOvisInitializeOverlayTable(vm::ptr<void> ea_ovly_table, vm::cptr<char> elf)
{
	cellOvis.todo("cellOvisInitializeOverlayTable(ea_ovly_table=*0x%x, elf=%s)", ea_ovly_table, elf);
	return CELL_OK;
}

void cellOvisFixSpuSegments(vm::ptr<sys_spu_image> image)
{
	cellOvis.todo("cellOvisFixSpuSegments(image=*0x%x)", image);
}

void cellOvisInvalidateOverlappedSegments(vm::ptr<sys_spu_segment> segs, vm::ptr<int> nsegs)
{
	cellOvis.todo("cellOvisInvalidateOverlappedSegments(segs=*0x%x, nsegs=*0x%x)", segs, nsegs);
}

DECLARE(ppu_module_manager::cellOvis)("cellOvis", []()
{
	REG_FUNC(cellOvis, cellOvisGetOverlayTableSize);
	REG_FUNC(cellOvis, cellOvisInitializeOverlayTable);
	REG_FUNC(cellOvis, cellOvisFixSpuSegments);
	REG_FUNC(cellOvis, cellOvisInvalidateOverlappedSegments);
});
