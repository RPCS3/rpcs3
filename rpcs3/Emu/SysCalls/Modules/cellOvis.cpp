#if 0
#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellOvis_init();
Module cellOvis(0x000b, cellOvis_init);

// Return Codes
enum
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

int cellOvisGetOverlayTableSize()
{
	UNIMPLEMENTED_FUNC(cellOvis);
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

void cellOvis_init()
{
	cellOvis.AddFunc(0x82f294b2, cellOvisGetOverlayTableSize);
	cellOvis.AddFunc(0xa876c911, cellOvisInitializeOverlayTable);
	cellOvis.AddFunc(0xce6cb776, cellOvisFixSpuSegments);
	cellOvis.AddFunc(0x629ba0c0, cellOvisInvalidateOverlappedSegments);
}
#endif 
