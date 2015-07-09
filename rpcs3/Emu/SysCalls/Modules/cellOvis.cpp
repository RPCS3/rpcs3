#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

namespace vm { using namespace ps3; }

extern Module cellOvis;

// Return Codes
enum
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

s32 cellOvisGetOverlayTableSize(vm::cptr<char> elf)
{
	cellOvis.Todo("cellOvisGetOverlayTableSize(elf_addr=0x%x)", elf.addr());
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

Module cellOvis("cellOvis", []()
{
	REG_FUNC(cellOvis, cellOvisGetOverlayTableSize);
	REG_FUNC(cellOvis, cellOvisInitializeOverlayTable);
	REG_FUNC(cellOvis, cellOvisFixSpuSegments);
	REG_FUNC(cellOvis, cellOvisInvalidateOverlappedSegments);
});
