#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellScreenshot.h"



logs::channel cellScreenshot("cellScreenshot");

s32 cellScreenShotSetParameter(vm::cptr<CellScreenShotSetParam> param)
{
	cellScreenshot.todo("cellScreenShotSetParameter(param=*0x%x)", param);
	return CELL_OK;
}

s32 cellScreenShotSetOverlayImage(vm::cptr<char> srcDir, vm::cptr<char> srcFile, s32 offset_x, s32 offset_y)
{
	cellScreenshot.todo("cellScreenShotSetOverlayImage(srcDir=%s, srcFile=%s, offset_x=%d, offset_y=%d)", srcDir, srcFile, offset_x, offset_y);
	return CELL_OK;
}

s32 cellScreenShotEnable()
{
	cellScreenshot.todo("cellScreenShotEnable()");
	return CELL_OK;
}

s32 cellScreenShotDisable()
{
	cellScreenshot.todo("cellScreenShotDisable()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellScreenShot)("cellScreenShotUtility", []()
{
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetParameter);
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetOverlayImage);
	REG_FUNC(cellScreenShotUtility, cellScreenShotEnable);
	REG_FUNC(cellScreenShotUtility, cellScreenShotDisable);
});
