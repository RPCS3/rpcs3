#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellMusicExport);

// Return Codes
enum CellMusicExportError : u32
{
	CELL_MUSIC_EXPORT_UTIL_ERROR_BUSY         = 0x8002c601,
	CELL_MUSIC_EXPORT_UTIL_ERROR_INTERNAL     = 0x8002c602,
	CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM        = 0x8002c603,
	CELL_MUSIC_EXPORT_UTIL_ERROR_ACCESS_ERROR = 0x8002c604,
	CELL_MUSIC_EXPORT_UTIL_ERROR_DB_INTERNAL  = 0x8002c605,
	CELL_MUSIC_EXPORT_UTIL_ERROR_DB_REGIST    = 0x8002c606,
	CELL_MUSIC_EXPORT_UTIL_ERROR_SET_META     = 0x8002c607,
	CELL_MUSIC_EXPORT_UTIL_ERROR_FLUSH_META   = 0x8002c608,
	CELL_MUSIC_EXPORT_UTIL_ERROR_MOVE         = 0x8002c609,
	CELL_MUSIC_EXPORT_UTIL_ERROR_INITIALIZE   = 0x8002c60a,
};

template<>
void fmt_class_string<CellMusicExportError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_INTERNAL);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_DB_INTERNAL);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_DB_REGIST);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_SET_META);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_FLUSH_META);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_MOVE);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

enum
{
	CELL_MUSIC_EXPORT_UTIL_VERSION_CURRENT        = 0,
	CELL_MUSIC_EXPORT_UTIL_HDD_PATH_MAX           = 1055,
	CELL_MUSIC_EXPORT_UTIL_MUSIC_TITLE_MAX_LENGTH = 64,
	CELL_MUSIC_EXPORT_UTIL_GAME_TITLE_MAX_LENGTH  = 64,
	CELL_MUSIC_EXPORT_UTIL_GAME_COMMENT_MAX_SIZE  = 1024,
};

struct CellMusicExportSetParam
{
	vm::bptr<char> title;
	vm::bptr<char> game_title;
	vm::bptr<char> artist;
	vm::bptr<char> genre;
	vm::bptr<char> game_comment;
	vm::bptr<char> reserved1;
	vm::bptr<void> reserved2;
};

using CellMusicExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);

struct music_export
{
	atomic_t<s32> progress = 0; // 0x0-0xFFFF for 0-100%
};


bool check_music_path(const std::string& file_path)
{
	if (file_path.size() >= CELL_MUSIC_EXPORT_UTIL_HDD_PATH_MAX)
	{
		return false;
	}

	for (char c : file_path)
	{
		if (!((c >= 'a' && c <= 'z') ||
			  (c >= 'A' && c <= 'Z') ||
			  (c >= '0' && c <= '9') ||
			   c == '-' || c == '_' ||
			   c == '/' || c == '.'))
		{
			return false;
		}
	}

	if (!file_path.starts_with("/dev_hdd0"sv) &&
		!file_path.starts_with("/dev_bdvd"sv) &&
		!file_path.starts_with("/dev_hdd1"sv))
	{
		return false;
	}

	if (file_path.find(".."sv) != umax)
	{
		return false;
	}

	return true;
}

std::string get_available_music_path(const std::string& filename)
{
	const std::string music_dir = "/dev_hdd0/music/";
	std::string dst_path = vfs::get(music_dir + filename);

	// Do not overwrite existing files. Add a suffix instead.
	for (u32 i = 0; fs::exists(dst_path); i++)
	{
		const std::string suffix = fmt::format("_%d", i);
		std::string new_filename = filename;

		if (const usz pos = new_filename.find_last_of('.'); pos != std::string::npos)
		{
			new_filename.insert(pos, suffix);
		}
		else
		{
			new_filename.append(suffix);
		}

		dst_path = vfs::get(music_dir + new_filename);
	}

	return dst_path;
}


error_code cellMusicExportInitialize(u32 version, u32 container, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.warning("cellMusicExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	if (version != CELL_MUSIC_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	if (container != 0xfffffffe)
	{
		if (false) // TODO: Check if memory container size >= 0x300000
		{
			return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
		}
	}

	if (!funcFinish)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportInitialize2(u32 version, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.notice("cellMusicExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	if (version != CELL_MUSIC_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	if (!funcFinish)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportFinalize(vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.notice("cellMusicExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellMusicExportSetParam> param, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	if (!param || !funcFinish)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	// TODO: check param members ?

	cellMusicExport.notice("cellMusicExportFromFile: param: title=%s, game_title=%s, artist=%s, genre=%s, game_comment=%s", param->title, param->game_title, param->artist, param->genre, param->game_comment);

	const std::string file_path = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());

	if (!check_music_path(file_path))
	{
		return { CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	std::string filename;

	if (srcHddFile)
	{
		fmt::append(filename, "%s", srcHddFile.get_ptr());
	}

	if (filename.empty())
	{
		return { CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& mexp = g_fxo->get<music_export>();
		mexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_music_path(filename);

		cellMusicExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		// TODO: We ignore metadata for now

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellMusicExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_MUSIC_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		if (!file_path.starts_with("/dev_bdvd"sv))
		{
			cellMusicExport.notice("Removing file '%s'", src_path);

			if (!fs::remove_file(src_path))
			{
				// TODO: find out if an error is used here
				cellMusicExport.error("Failed to remove file '%s' (%s)", src_path, fs::g_tls_error);
			}
		}

		// TODO: track progress during file copy

		mexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportProgress(vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		// Set the status as 0x0-0xFFFF (0-100%) depending on the copy status.
		// Only the copy or move of the movie and metadata files is considered for the progress.
		const auto& mexp = g_fxo->get<music_export>();

		funcFinish(ppu, mexp.progress, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellMusicExport)("cellMusicExportUtility", []()
{
	REG_FUNC(cellMusicExportUtility, cellMusicExportInitialize);
	REG_FUNC(cellMusicExportUtility, cellMusicExportInitialize2);
	REG_FUNC(cellMusicExportUtility, cellMusicExportFinalize);
	REG_FUNC(cellMusicExportUtility, cellMusicExportFromFile);
	REG_FUNC(cellMusicExportUtility, cellMusicExportProgress);
});
