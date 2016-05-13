#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellOvis("cellOvis", logs::level::notice);

// Return Codes
enum
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

s32 cellOvisGetOverlayTableSize(vm::cptr<char> elf)
{
	cellOvis.todo("cellOvisGetOverlayTableSize(elf=*0x%x)", elf);
	return CELL_OK;
}

s32 cellOvisInitializeOverlayTable()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

s32 cellOvisFixSpuSegments()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

s32 cellOvisInvalidateOverlappedSegments()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellOvis)("cellOvis", []()
{
	REG_FUNC(cellOvis, cellOvisGetOverlayTableSize);
	REG_FUNC(cellOvis, cellOvisInitializeOverlayTable);
	REG_FUNC(cellOvis, cellOvisFixSpuSegments);
	REG_FUNC(cellOvis, cellOvisInvalidateOverlappedSegments);
});
