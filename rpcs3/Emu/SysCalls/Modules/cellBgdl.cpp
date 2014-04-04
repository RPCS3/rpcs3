#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellBgdl_init();
Module cellBgdl(0x003f, cellBgdl_init);

// Return Codes
enum
{
	CELL_BGDL_UTIL_RET_OK             = 0x00000000,
	CELL_BGDL_UTIL_ERROR_BUSY         = 0x8002ce01,
	CELL_BGDL_UTIL_ERROR_INTERNAL     = 0x8002ce02,
	CELL_BGDL_UTIL_ERROR_PARAM        = 0x8002ce03,
	CELL_BGDL_UTIL_ERROR_ACCESS_ERROR = 0x8002ce04,
	CELL_BGDL_UTIL_ERROR_INITIALIZE   = 0x8002ce05,
};

int cellBGDLGetInfo()
{
	UNIMPLEMENTED_FUNC(cellBgdl);
	return CELL_OK;
}

int cellBGDLGetInfo2()
{
	UNIMPLEMENTED_FUNC(cellBgdl);
	return CELL_OK;
}

int cellBGDLSetMode()
{
	UNIMPLEMENTED_FUNC(cellBgdl);
	return CELL_OK;
}

int cellBGDLGetMode()
{
	UNIMPLEMENTED_FUNC(cellBgdl);
	return CELL_OK;
}

void cellBgdl_init()
{
	cellBgdl.AddFunc(0x4e9bb95b, cellBGDLGetInfo);
	cellBgdl.AddFunc(0x2ab0d183, cellBGDLGetInfo2);
	cellBgdl.AddFunc(0x7e134a90, cellBGDLSetMode);
	cellBgdl.AddFunc(0x74e57bdf, cellBGDLGetMode);
}