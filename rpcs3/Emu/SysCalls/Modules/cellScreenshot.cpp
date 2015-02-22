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
	cellScreenshot.AddFunc(0xd3ad63e4, cellScreenShotSetParameter);
	cellScreenshot.AddFunc(0x7a9c2243, cellScreenShotSetOverlayImage);
	cellScreenshot.AddFunc(0x9e33ab8f, cellScreenShotEnable);
	cellScreenshot.AddFunc(0xfc6f4e74, cellScreenShotDisable);
});
