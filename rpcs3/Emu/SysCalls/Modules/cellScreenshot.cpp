#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellScreenshot;

s32 cellScreenShotSetParameter() //const CellScreenShotSetParam *param
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

s32 cellScreenShotSetOverlayImage() //const char *srcDir, const char *srcFile, s32 offset_x, s32 offset_y
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

s32 cellScreenShotEnable()
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

s32 cellScreenShotDisable()
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

Module cellScreenshot("cellScreenshot", []()
{
	REG_FUNC(cellScreenshot, cellScreenShotSetParameter);
	REG_FUNC(cellScreenshot, cellScreenShotSetOverlayImage);
	REG_FUNC(cellScreenshot, cellScreenShotEnable);
	REG_FUNC(cellScreenshot, cellScreenShotDisable);
}
