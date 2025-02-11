#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellPhotoExport);

// Return Codes
enum CellPhotoExportError : u32
{
	CELL_PHOTO_EXPORT_UTIL_ERROR_BUSY         = 0x8002c201,
	CELL_PHOTO_EXPORT_UTIL_ERROR_INTERNAL     = 0x8002c202,
	CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM        = 0x8002c203,
	CELL_PHOTO_EXPORT_UTIL_ERROR_ACCESS_ERROR = 0x8002c204,
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_INTERNAL  = 0x8002c205,
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_REGIST    = 0x8002c206,
	CELL_PHOTO_EXPORT_UTIL_ERROR_SET_META     = 0x8002c207,
	CELL_PHOTO_EXPORT_UTIL_ERROR_FLUSH_META   = 0x8002c208,
	CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE         = 0x8002c209,
	CELL_PHOTO_EXPORT_UTIL_ERROR_INITIALIZE   = 0x8002c20a,
};

template<>
void fmt_class_string<CellPhotoExportError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_BUSY);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_INTERNAL);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_DB_INTERNAL);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_DB_REGIST);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_SET_META);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_FLUSH_META);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

enum
{
	CELL_PHOTO_EXPORT_UTIL_VERSION_CURRENT = 0
};

enum
{
	CELL_PHOTO_EXPORT_UTIL_HDD_PATH_MAX           = 1055,
	CELL_PHOTO_EXPORT_UTIL_PHOTO_TITLE_MAX_LENGTH = 64,
	CELL_PHOTO_EXPORT_UTIL_GAME_TITLE_MAX_LENGTH  = 64,
	CELL_PHOTO_EXPORT_UTIL_GAME_COMMENT_MAX_SIZE  = 1024,
};

struct CellPhotoExportSetParam
{
	vm::bptr<char> photo_title;
	vm::bptr<char> game_title;
	vm::bptr<char> game_comment;
	vm::bptr<void> reserved;
};

using CellPhotoExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);

struct photo_export
{
	atomic_t<s32> progress = 0; // 0x0-0xFFFF for 0-100%
};


bool check_photo_path(const std::string& file_path)
{
	if (file_path.size() >= CELL_PHOTO_EXPORT_UTIL_HDD_PATH_MAX)
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

std::string get_available_photo_path(const std::string& filename)
{
	const std::string photo_dir = "/dev_hdd0/photo/";
	std::string dst_path = vfs::get(photo_dir + filename);

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

		dst_path = vfs::get(photo_dir + new_filename);
	}

	return dst_path;
}


error_code cellPhotoInitialize(s32 version, u32 container, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoInitialize(version=%d, container=%d, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	if (version != CELL_PHOTO_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	if (container != 0xfffffffe)
	{
		// TODO
	}

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoFinalize(vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoRegistFromFile(vm::cptr<char> path, vm::cptr<char> photo_title, vm::cptr<char> game_title, vm::cptr<char> game_comment, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoRegistFromFile(path=%s, photo_title=%s, game_title=%s, game_comment=%s, funcFinish=*0x%x, userdata=*0x%x)", path, photo_title, game_title, game_comment, funcFinish, userdata);

	if (!path || !funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	const std::string file_path = path.get_ptr();

	if (!check_photo_path(file_path))
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	const std::string filename = file_path.substr(file_path.find_last_of('/') + 1);

	if (filename.empty())
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& pexp = g_fxo->get<photo_export>();
		pexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_photo_path(filename);

		cellPhotoExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		// TODO: We ignore metadata for now

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellPhotoExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		if (file_path.starts_with("/dev_hdd0"sv))
		{
			cellPhotoExport.notice("Removing file '%s'", src_path);

			if (!fs::remove_file(src_path))
			{
				// TODO: find out if an error is used here
				cellPhotoExport.error("Failed to remove file '%s' (%s)", src_path, fs::g_tls_error);
			}
		}

		// TODO: track progress during file copy

		pexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportInitialize(u32 version, u32 container, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	if (version != CELL_PHOTO_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	if (container != 0xfffffffe)
	{
		// TODO
	}

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportInitialize2(u32 version, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	if (version != CELL_PHOTO_EXPORT_UTIL_VERSION_CURRENT)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFinalize(vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoExportSetParam> param, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	if (!param || !funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	// TODO: check param members ?

	cellPhotoExport.notice("cellPhotoExportFromFile: param: photo_title=%s, game_title=%s, game_comment=%s", param->photo_title, param->game_title, param->game_comment);

	const std::string file_path = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());

	if (!check_photo_path(file_path))
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	std::string filename;

	if (srcHddFile)
	{
		fmt::append(filename, "%s", srcHddFile.get_ptr());
	}

	if (filename.empty())
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& pexp = g_fxo->get<photo_export>();
		pexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_photo_path(filename);

		cellPhotoExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		// TODO: We ignore metadata for now

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellPhotoExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		if (file_path.starts_with("/dev_hdd0"sv))
		{
			cellPhotoExport.notice("Removing file '%s'", src_path);

			if (!fs::remove_file(src_path))
			{
				// TODO: find out if an error is used here
				cellPhotoExport.error("Failed to remove file '%s' (%s)", src_path, fs::g_tls_error);
			}
		}

		// TODO: track progress during file copy

		pexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFromFileWithCopy(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoExportSetParam> param, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportFromFileWithCopy(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	if (!param || !funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	// TODO: check param members ?

	cellPhotoExport.notice("cellPhotoExportFromFileWithCopy: param: photo_title=%s, game_title=%s, game_comment=%s", param->photo_title, param->game_title, param->game_comment);

	const std::string file_path = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());

	if (!check_photo_path(file_path))
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, file_path };
	}

	std::string filename;

	if (srcHddFile)
	{
		fmt::append(filename, "%s", srcHddFile.get_ptr());
	}

	if (filename.empty())
	{
		return { CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM, "filename empty" };
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		auto& pexp = g_fxo->get<photo_export>();
		pexp.progress = 0; // 0%

		const std::string src_path = vfs::get(file_path);
		const std::string dst_path = get_available_photo_path(filename);

		cellPhotoExport.notice("Copying file from '%s' to '%s'", file_path, dst_path);

		// TODO: We ignore metadata for now

		if (!fs::create_path(fs::get_parent_dir(dst_path)) || !fs::copy_file(src_path, dst_path, false))
		{
			// TODO: find out which error is used
			cellPhotoExport.error("Failed to copy file from '%s' to '%s' (%s)", src_path, dst_path, fs::g_tls_error);
			funcFinish(ppu, CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE, userdata);
			return CELL_OK;
		}

		// TODO: track progress during file copy

		pexp.progress = 0xFFFF; // 100%

		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportProgress(vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.notice("cellPhotoExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		// Set the status as 0x0-0xFFFF (0-100%) depending on the copy status.
		// Only the copy or move of the movie and metadata files is considered for the progress.
		const auto& pexp = g_fxo->get<photo_export>();

		funcFinish(ppu, pexp.progress, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPhotoExport)("cellPhotoUtility", []()
{
	REG_FUNC(cellPhotoUtility, cellPhotoInitialize);
	REG_FUNC(cellPhotoUtility, cellPhotoFinalize);
	REG_FUNC(cellPhotoUtility, cellPhotoRegistFromFile);
	REG_FUNC(cellPhotoUtility, cellPhotoExportInitialize);
	REG_FUNC(cellPhotoUtility, cellPhotoExportInitialize2);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFinalize);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFromFile);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFromFileWithCopy);
	REG_FUNC(cellPhotoUtility, cellPhotoExportProgress);
});
