#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum CellScreenShotError : u32
{
	CELL_SCREENSHOT_ERROR_INTERNAL                 = 0x8002d101,
	CELL_SCREENSHOT_ERROR_PARAM                    = 0x8002d102,
	CELL_SCREENSHOT_ERROR_DECODE                   = 0x8002d103,
	CELL_SCREENSHOT_ERROR_NOSPACE                  = 0x8002d104,
	CELL_SCREENSHOT_ERROR_UNSUPPORTED_COLOR_FORMAT = 0x8002d105,
};

enum CellScreenShotParamSize
{
	CELL_SCREENSHOT_PHOTO_TITLE_MAX_LENGTH = 64,
	CELL_SCREENSHOT_GAME_TITLE_MAX_LENGTH  = 64,
	CELL_SCREENSHOT_GAME_COMMENT_MAX_SIZE  = 1024,
};

struct CellScreenShotSetParam
{
	vm::bcptr<char> photo_title;
	vm::bcptr<char> game_title;
	vm::bcptr<char> game_comment;
	vm::bptr<void> reserved;
};

struct screenshot_manager
{
	atomic_t<bool> is_enabled{ false };

	std::string photo_title;
	std::string game_title;
	std::string game_comment;

	atomic_t<s32> overlay_offset_x{ 0 };
	atomic_t<s32> overlay_offset_y{ 0 };
	std::string overlay_dir_name;
	std::string overlay_file_name;

	std::string get_overlay_path() const
	{
		return vfs::get(overlay_dir_name + overlay_file_name);
	}

	std::string get_photo_title() const
	{
		std::string photo = photo_title;
		if (photo.empty())
			photo = Emu.GetTitle();
		return photo;
	}

	std::string get_game_title() const
	{
		std::string game = game_title;
		if (game.empty())
			game = Emu.GetTitle();
		return game;
	}

	std::string get_screenshot_path() const
	{
		// TODO: make sure the file can be saved, add suffix and increase counter if file exists
		// TODO: maybe find a proper home for these
		return fs::get_config_dir() + "/screenshots/cell/" + get_photo_title() + ".png";
	}
};
