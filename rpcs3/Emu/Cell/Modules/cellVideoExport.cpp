#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellVideoExport);

enum CellVideoExportUtilError : u32
{
	CELL_VIDEO_EXPORT_UTIL_ERROR_BUSY         = 0x8002ca01,
	CELL_VIDEO_EXPORT_UTIL_ERROR_INTERNAL     = 0x8002ca02,
	CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM        = 0x8002ca03,
	CELL_VIDEO_EXPORT_UTIL_ERROR_ACCESS_ERROR = 0x8002ca04,
	CELL_VIDEO_EXPORT_UTIL_ERROR_DB_INTERNAL  = 0x8002ca05,
	CELL_VIDEO_EXPORT_UTIL_ERROR_DB_REGIST    = 0x8002ca06,
	CELL_VIDEO_EXPORT_UTIL_ERROR_SET_META     = 0x8002ca07,
	CELL_VIDEO_EXPORT_UTIL_ERROR_FLUSH_META   = 0x8002ca08,
	CELL_VIDEO_EXPORT_UTIL_ERROR_MOVE         = 0x8002ca09,
	CELL_VIDEO_EXPORT_UTIL_ERROR_INITIALIZE   = 0x8002ca0a,
};

enum
{
	CELL_VIDEO_EXPORT_UTIL_RET_OK     = 0,
	CELL_VIDEO_EXPORT_UTIL_RET_CANCEL = 1,
};

enum
{
	CELL_VIDEO_EXPORT_UTIL_VERSION_CURRENT        = 0,
	CELL_VIDEO_EXPORT_UTIL_HDD_PATH_MAX           = 1055,
	CELL_VIDEO_EXPORT_UTIL_VIDEO_TITLE_MAX_LENGTH = 64,
	CELL_VIDEO_EXPORT_UTIL_GAME_TITLE_MAX_LENGTH  = 64,
	CELL_VIDEO_EXPORT_UTIL_GAME_COMMENT_MAX_SIZE  = 1024,
};

template<>
void fmt_class_string<CellVideoExportUtilError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_BUSY);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_INTERNAL);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_DB_INTERNAL);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_DB_REGIST);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_SET_META);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_FLUSH_META);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_MOVE);
			STR_CASE(CELL_VIDEO_EXPORT_UTIL_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

struct CellVideoExportSetParam
{
	vm::bptr<char> title;
	vm::bptr<char> game_title;
	vm::bptr<char> game_comment;
	be_t<s32> editable;
	vm::bptr<void> reserved2;
};

using CellVideoExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);


struct video_export
{
	atomic_t<s32> progress = 0; // 0x0-0xFFFF for 0-100%
};


bool check_movie_path(const std::string& file_path)
{
	if (file_path.size() >= CELL_VIDEO_EXPORT_UTIL_HDD_PATH_MAX)
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

std::string get_available_movie_path(const std::string& filename)
{
	// TODO: Find out how to build this path properly. Apparently real hardware doesn't add a suffix,
	//       but just randomly puts each video into a separate 2-Letter subdirectory like /video/hd/ or /video/ee/
	const std::string movie_dir = "/dev_hdd0/video/";
	std::string dst_path = vfs::get(movie_dir + filename);

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

		dst_path = vfs::get(movie_dir + new_filename);
	}

	return dst_path;
}


error_code cellVideoExportInitialize2(u32 version, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.notice("cellVideoExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	if (version != CELL_VIDEO_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	if (!funcFinish)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportInitialize(u32 version, u32 container, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.notice("cellVideoExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	if (version != CELL_VIDEO_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	// Check container (same sub-function as cellVideoExportInitialize2, so we have to check this parameter anyway)
	if (container != 0xfffffffe)
	{
		if (false) // invalid container or container size < 0x500000
		{
			return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
		}
	}

	if (!funcFinish)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportProgress(vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		// Set the status as 0x0-0xFFFF (0-100%) depending on the copy status.
		// Only the copy or move of the movie and metadata files is considered for the progress.
		const auto& vexp = g_fxo->get<video_export>();

		funcFinish(ppu, vexp.progress, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFromFileWithCopy(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellVideoExportSetParam> param, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportFromFileWithCopy(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	if (!param || !funcFinish || !srcHddDir || !srcHddDir[0] || !srcHddFile || !srcHddFile[0])
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	// TODO: check param members ?

	cellVideoExport.notice("cellVideoExportFromFileWithCopy: param: title=%s, game_title=%s, game_comment=%s, editable=%d", param->title, param->game_title, param->game_comment, param->editable);

	const std::string file_path = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());

	if (!check_movie_path(file_path))
	{
		return { CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	std::string filename;

	if (srcHddFile)
	{
		fmt::append(filename, "%s", srcHddFile.get_ptr());
	}

	if (filename.empty())
	{
		return { CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& vexp = g_fxo->get<video_export>();
		vexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_movie_path(filename);

		cellVideoExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellVideoExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_VIDEO_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		// TODO: write metadata file sometime in the far future
		// const std::string metadata_file = dst_path + ".vmd";

		// TODO: track progress during file copy

		vexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellVideoExportSetParam> param, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	if (!param || !funcFinish || !srcHddDir || !srcHddDir[0] || !srcHddFile || !srcHddFile[0])
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	// TODO: check param members ?

	cellVideoExport.notice("cellVideoExportFromFile: param: title=%s, game_title=%s, game_comment=%s, editable=%d", param->title, param->game_title, param->game_comment, param->editable);

	const std::string file_path = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());

	if (!check_movie_path(file_path))
	{
		return { CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	std::string filename;

	if (srcHddFile)
	{
		fmt::append(filename, "%s", srcHddFile.get_ptr());
	}

	if (filename.empty())
	{
		return { CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& vexp = g_fxo->get<video_export>();
		vexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_movie_path(filename);

		cellVideoExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellVideoExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_VIDEO_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		if (!file_path.starts_with("/dev_bdvd"sv))
		{
			cellVideoExport.notice("Removing file '%s'", src_path);

			if (!fs::remove_file(src_path))
			{
				// TODO: find out if an error is used here
				cellVideoExport.error("Failed to remove file '%s' (%s)", src_path, fs::g_tls_error);
			}
		}

		// TODO: write metadata file sometime in the far future
		// const std::string metadata_file = dst_path + ".vmd";

		// TODO: track progress during file copy

		vexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFinalize(vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.notice("cellVideoExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_VIDEO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellVideoExport)("cellVideoExportUtility", []()
{
	REG_FUNC(cellVideoExportUtility, cellVideoExportProgress);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize2);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFileWithCopy);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFile);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFinalize);
});
