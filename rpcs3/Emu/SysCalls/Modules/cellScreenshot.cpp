#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellScreenshot_init();
Module cellScreenshot(0x004e, cellScreenshot_init);

// Return Codes
enum
{
	CELL_SCREENSHOT_OK                             = 0x0,
	CELL_SCREENSHOT_ERROR_INTERNAL                 = 0x8002d101,
	CELL_SCREENSHOT_ERROR_PARAM                    = 0x8002d102,
	CELL_SCREENSHOT_ERROR_DECODE                   = 0x8002d103,
	CELL_SCREENSHOT_ERROR_NOSPACE                  = 0x8002d104,
	CELL_SCREENSHOT_ERROR_UNSUPPORTED_COLOR_FORMAT = 0x8002d105,
};

// Datatypes
struct CellScreenShotSetParam
{
	const char *photo_title;
	const char *game_title;
	const char *game_comment;
};

// Functions
int cellScreenShotSetParameter() //const CellScreenShotSetParam *param
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

int cellScreenShotSetOverlayImage() //const char *srcDir, const char *srcFile, s32 offset_x, s32 offset_y
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

int cellScreenShotEnable()
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

int cellScreenShotDisable()
{
	UNIMPLEMENTED_FUNC(cellScreenshot);
	return CELL_OK;
}

void cellScreenshot_init()
{
	cellScreenshot.AddFunc(0xd3ad63e4, cellScreenShotSetParameter);
	cellScreenshot.AddFunc(0x7a9c2243, cellScreenShotSetOverlayImage);
	cellScreenshot.AddFunc(0x9e33ab8f, cellScreenShotEnable);
	cellScreenshot.AddFunc(0xfc6f4e74, cellScreenShotDisable);
}