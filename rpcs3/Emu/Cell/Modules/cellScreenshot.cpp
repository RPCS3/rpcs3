#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include "cellScreenshot.h"

LOG_CHANNEL(cellScreenshot);

template<>
void fmt_class_string<CellScreenShotError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SCREENSHOT_ERROR_INTERNAL);
			STR_CASE(CELL_SCREENSHOT_ERROR_PARAM);
			STR_CASE(CELL_SCREENSHOT_ERROR_DECODE);
			STR_CASE(CELL_SCREENSHOT_ERROR_NOSPACE);
			STR_CASE(CELL_SCREENSHOT_ERROR_UNSUPPORTED_COLOR_FORMAT);
		}

		return unknown;
	});
}

std::string screenshot_info::get_overlay_path() const
{
	return vfs::get(overlay_dir_name + "/" + overlay_file_name);
}

std::string screenshot_info::get_photo_title() const
{
	return photo_title.empty() ? Emu.GetTitle() : photo_title;
}

std::string screenshot_info::get_game_title() const
{
	return game_title.empty() ? Emu.GetTitle() : game_title;
}

std::string screenshot_info::get_game_comment() const
{
	return game_comment;
}


error_code cellScreenShotSetParameter(vm::cptr<CellScreenShotSetParam> param)
{
	cellScreenshot.warning("cellScreenShotSetParameter(param=*0x%x)", param);

	if (!param) // TODO: check if param->reserved must be null
		return CELL_SCREENSHOT_ERROR_PARAM;

	if (param->photo_title && !memchr(param->photo_title.get_ptr(), '\0', CELL_SCREENSHOT_PHOTO_TITLE_MAX_LENGTH))
		return CELL_SCREENSHOT_ERROR_PARAM;

	if (param->game_title && !memchr(param->game_title.get_ptr(), '\0', CELL_SCREENSHOT_GAME_TITLE_MAX_LENGTH))
		return CELL_SCREENSHOT_ERROR_PARAM;

	if (param->game_comment && !memchr(param->game_comment.get_ptr(), '\0', CELL_SCREENSHOT_GAME_COMMENT_MAX_SIZE))
		return CELL_SCREENSHOT_ERROR_PARAM;

	auto& manager = g_fxo->get<screenshot_manager>();
	std::lock_guard lock(manager.mutex);

	if (param->photo_title && param->photo_title[0] != '\0')
		manager.photo_title = std::string(param->photo_title.get_ptr());
	else
		manager.photo_title = "";

	if (param->game_title && param->game_title[0] != '\0')
		manager.game_title = std::string(param->game_title.get_ptr());
	else
		manager.game_title = "";

	if (param->game_comment && param->game_comment[0] != '\0')
		manager.game_comment = std::string(param->game_comment.get_ptr());
	else
		manager.game_comment = "";

	cellScreenshot.notice("cellScreenShotSetParameter(photo_title=%s, game_title=%s, game_comment=%s)", manager.photo_title, manager.game_title, manager.game_comment);

	return CELL_OK;
}

error_code cellScreenShotSetOverlayImage(vm::cptr<char> srcDir, vm::cptr<char> srcFile, s32 offset_x, s32 offset_y)
{
	cellScreenshot.warning("cellScreenShotSetOverlayImage(srcDir=%s, srcFile=%s, offset_x=%d, offset_y=%d)", srcDir, srcFile, offset_x, offset_y);

	if (!srcDir || !srcFile)
		return CELL_SCREENSHOT_ERROR_PARAM;

	// TODO: check srcDir (size 1024) and srcFile (size 64) for '-' or '_' or '.' or '/' in some manner (range checks?)

	// Make sure that srcDir starts with /dev_hdd0, /dev_bdvd, /app_home or /host_root
	if (strncmp(srcDir.get_ptr(), "/dev_hdd0", 9) && strncmp(srcDir.get_ptr(), "/dev_bdvd", 9) && strncmp(srcDir.get_ptr(), "/app_home", 9) && strncmp(srcDir.get_ptr(), "/host_root", 10))
	{
		return CELL_SCREENSHOT_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<screenshot_manager>();
	std::lock_guard lock(manager.mutex);

	manager.overlay_dir_name = std::string(srcDir.get_ptr());
	manager.overlay_file_name = std::string(srcFile.get_ptr());
	manager.overlay_offset_x = offset_x;
	manager.overlay_offset_y = offset_y;

	return CELL_OK;
}

error_code cellScreenShotEnable()
{
	cellScreenshot.trace("cellScreenShotEnable()");

	auto& manager = g_fxo->get<screenshot_manager>();
	std::lock_guard lock(manager.mutex);

	if (!std::exchange(manager.is_enabled, true))
	{
		cellScreenshot.warning("cellScreenShotEnable(): Enabled");
	}

	return CELL_OK;
}

error_code cellScreenShotDisable()
{
	cellScreenshot.trace("cellScreenShotDisable()");

	auto& manager = g_fxo->get<screenshot_manager>();
	std::lock_guard lock(manager.mutex);

	if (std::exchange(manager.is_enabled, false))
	{
		cellScreenshot.warning("cellScreenShotDisable(): Disabled");
	}


	return CELL_OK;
}

DECLARE(ppu_module_manager::cellScreenShot)("cellScreenShotUtility", []()
{
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetParameter);
	REG_FUNC(cellScreenShotUtility, cellScreenShotSetOverlayImage);
	REG_FUNC(cellScreenShotUtility, cellScreenShotEnable);
	REG_FUNC(cellScreenShotUtility, cellScreenShotDisable);
});
