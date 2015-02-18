#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellOvis;

// Return Codes
enum
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

int cellOvisGetOverlayTableSize(vm::ptr<const char> elf)
{
	cellOvis.Todo("cellOvisGetOverlayTableSize(elf_addr=0x%x)", elf.addr());
	return CELL_OK;
}

int cellOvisInitializeOverlayTable()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

int cellOvisFixSpuSegments()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

int cellOvisInvalidateOverlappedSegments()
{
	UNIMPLEMENTED_FUNC(cellOvis);
	return CELL_OK;
}

Module cellOvis("cellOvis", []()
{
	cellOvis.AddFunc(0x82f294b2, cellOvisGetOverlayTableSize);
	cellOvis.AddFunc(0xa876c911, cellOvisInitializeOverlayTable);
	cellOvis.AddFunc(0xce6cb776, cellOvisFixSpuSegments);
	cellOvis.AddFunc(0x629ba0c0, cellOvisInvalidateOverlappedSegments);
});
