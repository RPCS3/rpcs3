#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellScreenshot.h"

logs::channel cellScreenshot("cellScreenshot", logs::level::notice);

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

DECLARE(ppu_module_manager::cellScreenShot)("cellScreenShotUtility", []()
{
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetParameter);
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetOverlayImage);
	REG_FUNC(cellScreenShotUtility, cellScreenShotEnable);
	REG_FUNC(cellScreenShotUtility, cellScreenShotDisable);
});
