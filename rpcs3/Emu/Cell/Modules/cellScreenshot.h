#pragma once

// Return Codes
enum
{
	CELL_SCREENSHOT_ERROR_INTERNAL = 0x8002d101,
	CELL_SCREENSHOT_ERROR_PARAM = 0x8002d102,
	CELL_SCREENSHOT_ERROR_DECODE = 0x8002d103,
	CELL_SCREENSHOT_ERROR_NOSPACE = 0x8002d104,
	CELL_SCREENSHOT_ERROR_UNSUPPORTED_COLOR_FORMAT = 0x8002d105,
};

struct CellScreenShotSetParam
{
	vm::bcptr<char> photo_title;
	vm::bcptr<char> game_title;
	vm::bcptr<char> game_comment;
};
