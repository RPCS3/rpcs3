#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
void cellScreenShotUtility_init ();
Module cellScreenShotUtility ("cellScreenShotUtility", cellScreenShotUtility_init);

//Error codes
enum
{
	CELL_SCREENSHOT_OK								    = 0x0, 
	CELL_SCREENSHOT_ERROR_INTERNAL						= 0x8002d101,
	CELL_SCREENSHOT_ERROR_PARAM							= 0x8002d102, 
	CELL_SCREENSHOT_ERROR_DECODE					    = 0x8002d103,  
	CELL_SCREENSHOT_ERROR_NOSPACE					    = 0x8002d104,  
	CELL_SCREENSHOT_ERROR_UNSUPPORTED_COLOR_FORMAT		= 0x8002d105,
};

//datatyps
struct CellScreenShotSetParam
{
	const char *photo_title; 
	const char *game_title; 
	const char *game_comment; 
	void* reserved;
};

int cellScreenShotSetParameter () //const CellScreenShotSetParam *param
{
	UNIMPLEMENTED_FUNC(cellScreenShotUtility);
	return CELL_SCREENSHOT_OK;
}

int cellScreenShotSetOverlayImage() //const char *srcDir, const char *srcFile, s32 offset_x, s32 offset_y
{
	UNIMPLEMENTED_FUNC(cellScreenShotUtility);
	return CELL_SCREENSHOT_OK;
}

int cellScreenShotEnable (void)
{
	UNIMPLEMENTED_FUNC(cellScreenShotUtility);
	return CELL_SCREENSHOT_OK;
}

int cellScreenShotDisable (void)
{
	UNIMPLEMENTED_FUNC(cellScreenShotUtility);
	return CELL_SCREENSHOT_OK;
}


void cellScreenShotUtility_init ()
{
cellScreenShotUtility.AddFunc (0x9e33ab8f, cellScreenShotEnable);
cellScreenShotUtility.AddFunc (0xd3ad63e4, cellScreenShotSetParameter);
cellScreenShotUtility.AddFunc (0xfc6f4e74, cellScreenShotDisable);
cellScreenShotUtility.AddFunc (0x7a9c2243, cellScreenShotSetOverlayImage);
}
