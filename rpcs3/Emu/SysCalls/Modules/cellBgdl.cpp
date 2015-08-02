#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellBGDL;

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

s32 cellBGDLGetInfo()
{
	UNIMPLEMENTED_FUNC(cellBGDL);
	return CELL_OK;
}

s32 cellBGDLGetInfo2()
{
	UNIMPLEMENTED_FUNC(cellBGDL);
	return CELL_OK;
}

s32 cellBGDLSetMode()
{
	UNIMPLEMENTED_FUNC(cellBGDL);
	return CELL_OK;
}

s32 cellBGDLGetMode()
{
	UNIMPLEMENTED_FUNC(cellBGDL);
	return CELL_OK;
}

Module cellBGDL("cellBGDL", []()
{
	REG_FUNC(cellBGDL, cellBGDLGetInfo);
	REG_FUNC(cellBGDL, cellBGDLGetInfo2);
	REG_FUNC(cellBGDL, cellBGDLSetMode);
	REG_FUNC(cellBGDL, cellBGDLGetMode);
});
