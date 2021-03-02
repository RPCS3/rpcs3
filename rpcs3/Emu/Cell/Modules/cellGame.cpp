#include "stdafx.h"
#include "Emu/localized_string.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"
#include "Utilities/span.h"
#include "util/init_mutex.hpp"

#include <thread>

LOG_CHANNEL(cellGame);

template<>
void fmt_class_string<CellGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_GAME_ERROR_NOTFOUND);
		STR_CASE(CELL_GAME_ERROR_BROKEN);
		STR_CASE(CELL_GAME_ERROR_INTERNAL);
		STR_CASE(CELL_GAME_ERROR_PARAM);
		STR_CASE(CELL_GAME_ERROR_NOAPP);
		STR_CASE(CELL_GAME_ERROR_ACCESS_ERROR);
		STR_CASE(CELL_GAME_ERROR_NOSPACE);
		STR_CASE(CELL_GAME_ERROR_NOTSUPPORTED);
		STR_CASE(CELL_GAME_ERROR_FAILURE);
		STR_CASE(CELL_GAME_ERROR_BUSY);
		STR_CASE(CELL_GAME_ERROR_IN_SHUTDOWN);
		STR_CASE(CELL_GAME_ERROR_INVALID_ID);
		STR_CASE(CELL_GAME_ERROR_EXIST);
		STR_CASE(CELL_GAME_ERROR_NOTPATCH);
		STR_CASE(CELL_GAME_ERROR_INVALID_THEME_FILE);
		STR_CASE(CELL_GAME_ERROR_BOOTPATH);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellGameDataError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_GAMEDATA_ERROR_CBRESULT);
		STR_CASE(CELL_GAMEDATA_ERROR_ACCESS_ERROR);
		STR_CASE(CELL_GAMEDATA_ERROR_INTERNAL);
		STR_CASE(CELL_GAMEDATA_ERROR_PARAM);
		STR_CASE(CELL_GAMEDATA_ERROR_NOSPACE);
		STR_CASE(CELL_GAMEDATA_ERROR_BROKEN);
		STR_CASE(CELL_GAMEDATA_ERROR_FAILURE);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellDiscGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_DISCGAME_ERROR_INTERNAL);
			STR_CASE(CELL_DISCGAME_ERROR_NOT_DISCBOOT);
			STR_CASE(CELL_DISCGAME_ERROR_PARAM);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellHddGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_HDDGAME_ERROR_CBRESULT);
			STR_CASE(CELL_HDDGAME_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_HDDGAME_ERROR_INTERNAL);
			STR_CASE(CELL_HDDGAME_ERROR_PARAM);
			STR_CASE(CELL_HDDGAME_ERROR_NOSPACE);
			STR_CASE(CELL_HDDGAME_ERROR_BROKEN);
			STR_CASE(CELL_HDDGAME_ERROR_FAILURE);
		}

		return unknown;
	});
}

// If dir is empty:
// contentInfo = "/dev_bdvd/PS3_GAME"
// usrdir = "/dev_bdvd/PS3_GAME/USRDIR"
// Temporary content directory (dir is not empty):
// contentInfo = "/dev_hdd0/game/_GDATA_" + time_since_epoch
// usrdir = "/dev_hdd0/game/_GDATA_" + time_since_epoch + "/USRDIR"
// Normal content directory (dir is not empty):
// contentInfo = "/dev_hdd0/game/" + dir
// usrdir = "/dev_hdd0/game/" + dir + "/USRDIR"
struct content_permission final
{
	// Content directory name or path
	std::string dir;

	// SFO file
	psf::registry sfo;

	// Temporary directory path
	std::string temp;

	stx::init_mutex init;

	atomic_t<u32> can_create = 0;
	atomic_t<bool> exists = false;
	atomic_t<bool> restrict_sfo_params = true;

	content_permission() = default;

	content_permission(const content_permission&) = delete;

	content_permission& operator=(const content_permission&) = delete;

	void reset()
	{
		dir.clear();
		sfo.clear();
		temp.clear();
		can_create = 0;
		exists = false;
		restrict_sfo_params = true;
	}

	~content_permission()
	{
		bool success = false;
		fs::g_tls_error = fs::error::ok;

		if (temp.size() <= 1 || fs::remove_all(temp))
		{
			success = true;
		}

		if (!success)
		{
			cellGame.fatal("Failed to clean directory '%s' (%s)", temp, fs::g_tls_error);
		}
	}
};

error_code cellHddGameCheck(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.error("cellHddGameCheck(version=%d, dirName=%s, errDialog=%d, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	if (!dirName || !funcStat || sysutil_check_name_string(dirName.get_ptr(), 1, CELL_GAME_DIRNAME_SIZE) != 0)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	std::string game_dir = dirName.get_ptr();

	// TODO: Find error code
	ensure(game_dir.size() == 9);

	const std::string dir = "/dev_hdd0/game/" + game_dir;

	psf::registry sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	const u32 new_data = sfo.empty() && !fs::is_file(vfs::get(dir + "/PARAM.SFO")) ? CELL_GAMEDATA_ISNEWDATA_YES : CELL_GAMEDATA_ISNEWDATA_NO;

	if (!new_data)
	{
		const auto cat = psf::get_string(sfo, "CATEGORY", "");
		if (cat != "HG")
		{
			return CELL_GAMEDATA_ERROR_BROKEN;
		}
	}

	const std::string usrdir = dir + "/USRDIR";

	vm::var<CellHddGameCBResult> result;
	vm::var<CellHddGameStatGet> get;
	vm::var<CellHddGameStatSet> set;

	// 40 GB - 1 kilobyte. The reasoning is that many games take this number and multiply it by 1024, to get the amount of bytes. With 40GB exactly,
	// this will result in an overflow, and the size would be 0, preventing the game from running. By reducing 1 kilobyte, we make sure that even
	// after said overflow, the number would still be high enough to contain the game's data.
	get->hddFreeSizeKB = 40 * 1024 * 1024 - 1;
	get->isNewData = CELL_HDDGAME_ISNEWDATA_EXIST;
	get->sysSizeKB = 0; // TODO
	get->atime = 0; // TODO
	get->ctime = 0; // TODO
	get->mtime = 0; // TODO
	get->sizeKB = CELL_HDDGAME_SIZEKB_NOTCALC;
	strcpy_trunc(get->contentInfoPath, dir);
	strcpy_trunc(get->hddGamePath, usrdir);

	vm::var<CellHddGameSystemFileParam> setParam;
	set->setParam = setParam;

	const std::string& local_dir = vfs::get(dir);

	if (!fs::is_dir(local_dir))
	{
		get->isNewData = CELL_HDDGAME_ISNEWDATA_NODIR;
		get->getParam = {};
	}
	else
	{
		// TODO: Is cellHddGameCheck really responsible for writing the information in get->getParam ? (If not, delete this else)
		const auto& psf = psf::load_object(fs::file(local_dir +"/PARAM.SFO"));

		// Some following fields may be zero in old FW 1.00 version PARAM.SFO
		if (psf.count("PARENTAL_LEVEL") != 0) get->getParam.parentalLevel = psf.at("PARENTAL_LEVEL").as_integer();
		if (psf.count("ATTRIBUTE") != 0) get->getParam.attribute = psf.at("ATTRIBUTE").as_integer();
		if (psf.count("RESOLUTION") != 0) get->getParam.resolution = psf.at("RESOLUTION").as_integer();
		if (psf.count("SOUND_FORMAT") != 0) get->getParam.soundFormat = psf.at("SOUND_FORMAT").as_integer();
		if (psf.count("TITLE") != 0) strcpy_trunc(get->getParam.title, psf.at("TITLE").as_string());
		if (psf.count("APP_VER") != 0) strcpy_trunc(get->getParam.dataVersion, psf.at("APP_VER").as_string());
		if (psf.count("TITLE_ID") != 0) strcpy_trunc(get->getParam.titleId, psf.at("TITLE_ID").as_string());

		for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
		{
			strcpy_trunc(get->getParam.titleLang[i], psf::get_string(psf, fmt::format("TITLE_%02d", i)));
		}
	}

	// TODO ?

	funcStat(ppu, result, get, set);

	std::string error_msg;

	switch (result->result)
	{
	case CELL_HDDGAME_CBRESULT_OK:
	{
		// Game confirmed that it wants to create directory
		const auto setParam = set->setParam;

		if (new_data)
		{
			if (!setParam)
			{
				return CELL_GAMEDATA_ERROR_PARAM;
			}

			if (!fs::create_path(vfs::get(usrdir)))
			{
				return {CELL_GAME_ERROR_ACCESS_ERROR, usrdir};
			}
		}

		// Nuked until correctly reversed engineered
		// if (setParam)
		// {
		// 	if (new_data)
		// 	{
		// 		psf::assign(sfo, "CATEGORY", psf::string(3, "HG"));
		// 	}

		// 	psf::assign(sfo, "TITLE_ID", psf::string(CELL_GAME_SYSP_TITLEID_SIZE, setParam->titleId));
		// 	psf::assign(sfo, "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->title));
		// 	psf::assign(sfo, "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, setParam->dataVersion));
		// 	psf::assign(sfo, "PARENTAL_LEVEL", +setParam->parentalLevel);
		// 	psf::assign(sfo, "RESOLUTION", +setParam->resolution);
		// 	psf::assign(sfo, "SOUND_FORMAT", +setParam->soundFormat);

		// 	for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
		// 	{
		// 		if (!setParam->titleLang[i][0])
		// 		{
		// 			continue;
		// 		}

		// 		psf::assign(sfo, fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->titleLang[i]));
		// 	}

		// 	psf::save_object(fs::file(vfs::get(dir + "/PARAM.SFO"), fs::rewrite), sfo);
		// }
		return CELL_OK;
	}
	case CELL_HDDGAME_CBRESULT_OK_CANCEL:
		cellGame.warning("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_OK_CANCEL");
		return CELL_OK;

	case CELL_HDDGAME_CBRESULT_ERR_NOSPACE:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_NOSPACE. Space Needed: %d KB", result->errNeedSizeKB);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_NOSPACE, fmt::format("%d", result->errNeedSizeKB).c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_BROKEN:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_BROKEN");
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_BROKEN, game_dir.c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_NODATA:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_NODATA");
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_NODATA, game_dir.c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_INVALID:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_INVALID. Error message: %s", result->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_INVALID, fmt::format("%s", result->invalidMsg).c_str());
		break;

	default:
		cellGame.error("cellHddGameCheck(): callback returned unknown error (code=0x%x). Error message: %s", result->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_INVALID, fmt::format("%s", result->invalidMsg).c_str());
		break;
	}

	if (errDialog == CELL_GAMEDATA_ERRDIALOG_ALWAYS) // Maybe != CELL_GAMEDATA_ERRDIALOG_NONE
	{
		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Get user confirmation by opening a blocking dialog
		error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON, vm::make_str(error_msg));

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		if (res != CELL_OK)
		{
			return CELL_GAMEDATA_ERROR_INTERNAL;
		}
	}

	return CELL_HDDGAME_ERROR_CBRESULT;
}

error_code cellHddGameCheck2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.trace("cellHddGameCheck2()");

	// Identical function
	return cellHddGameCheck(ppu, version, dirName, errDialog, funcStat, container);
}

error_code cellHddGameGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellHddGameGetSizeKB(size=*0x%x)", size);

	const std::string local_dir = vfs::get(Emu.GetDir());

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (fs::exists(local_dir))
		{
			cellGame.error("cellHddGameGetSizeKB(): Unknown failure on calculating directory '%s' size (%s)", local_dir, error);
		}

		return CELL_HDDGAME_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellHddGameSetSystemVer(vm::cptr<char> systemVersion)
{
	cellGame.todo("cellHddGameSetSystemVer(systemVersion=%s)", systemVersion);

	if (!systemVersion)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellHddGameExitBroken()
{
	cellGame.warning("cellHddGameExitBroken()");
	return open_exit_dialog(get_localized_string(localized_string_id::CELL_HDD_GAME_EXIT_BROKEN), true);
}

error_code cellGameDataGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellGameDataGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	const std::string local_dir = vfs::get(Emu.GetDir());

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (fs::exists(local_dir))
		{
			cellGame.error("cellGameDataGetSizeKB(): Unknown failure on calculating directory '%s' size (%s)", local_dir, error);
		}

		return CELL_GAMEDATA_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellGameDataSetSystemVer(vm::cptr<char> systemVersion)
{
	cellGame.todo("cellGameDataSetSystemVer(systemVersion=%s)", systemVersion);

	if (!systemVersion)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameDataExitBroken()
{
	cellGame.warning("cellGameDataExitBroken()");
	return open_exit_dialog(get_localized_string(localized_string_id::CELL_GAME_DATA_EXIT_BROKEN), true);
}

error_code cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame.warning("cellGameBootCheck(type=*0x%x, attributes=*0x%x, size=*0x%x, dirName=*0x%x)", type, attributes, size, dirName);

	if (!type || !attributes)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.init();

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	std::string dir;
	psf::registry sfo;

	if (Emu.GetCat() == "DG")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		// TODO: dirName might be a read only string when BootCheck is called on a disc game. (e.g. Ben 10 Ultimate Alien: Cosmic Destruction)

		sfo = psf::load_object(fs::file(vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO")));
	}
	else if (Emu.GetCat() == "GD")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO

		sfo = psf::load_object(fs::file(vfs::get(Emu.GetDir() + "PARAM.SFO")));
	}
	else
	{
		*type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO

		sfo = psf::load_object(fs::file(vfs::get(Emu.GetDir() + "PARAM.SFO")));
		dir = Emu.GetTitleID();
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for HG and DG games, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 4;
	}

	if (*type == u32{CELL_GAME_GAMETYPE_HDD} && dirName)
	{
		strcpy_trunc(*dirName, Emu.GetTitleID());
	}

	perm.dir = std::move(dir);
	perm.sfo = std::move(sfo);
	perm.restrict_sfo_params = *type == u32{CELL_GAME_GAMETYPE_HDD}; // Ratchet & Clank: All 4 One (PSN versions) rely on this error checking (TODO: Needs proper hw tests)
	perm.exists = true;

	return CELL_OK;
}

error_code cellGamePatchCheck(vm::ptr<CellGameContentSize> size, vm::ptr<void> reserved)
{
	cellGame.warning("cellGamePatchCheck(size=*0x%x, reserved=*0x%x)", size, reserved);

	if (Emu.GetCat() != "GD")
	{
		return CELL_GAME_ERROR_NOTPATCH;
	}

	psf::registry sfo = psf::load_object(fs::file(vfs::get(Emu.GetDir() + "PARAM.SFO")));

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.init();

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for patch data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0; // TODO
	}

	perm.restrict_sfo_params = false;
	perm.dir = Emu.GetTitleID();
	perm.sfo = std::move(sfo);
	perm.exists = true;

	return CELL_OK;
}

error_code cellGameDataCheck(u32 type, vm::cptr<char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame.warning("cellGameDataCheck(type=%d, dirName=%s, size=*0x%x)", type, dirName, size);

	if ((type - 1) >= 3 || (type != CELL_GAME_GAMETYPE_DISC && !dirName))
	{
		return {CELL_GAME_ERROR_PARAM, type};
	}

	std::string name;

	if (type != CELL_GAME_GAMETYPE_DISC)
	{
		name = dirName.get_ptr();
	}

	const std::string dir = type == CELL_GAME_GAMETYPE_DISC ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + name;

	// TODO: not sure what should be checked there

	auto& perm = g_fxo->get<content_permission>();

	auto init = perm.init.init();

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	auto sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	if (psf::get_string(sfo, "CATEGORY") != [&]()
	{
		switch (type)
		{
		case CELL_GAME_GAMETYPE_HDD: return "HG"sv;
		case CELL_GAME_GAMETYPE_GAMEDATA: return "GD"sv;
		case CELL_GAME_GAMETYPE_DISC: return "DG"sv;
		default: fmt::throw_exception("Unreachable");
		}
	}())
	{
		if (fs::is_file(vfs::get(dir + "/PARAM.SFO")))
		{
			init.cancel();
			return CELL_GAME_ERROR_BROKEN;
		}
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for game data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0; // TODO
	}

	perm.dir = std::move(name);

	if (type == CELL_GAME_GAMETYPE_GAMEDATA)
	{
		perm.can_create = true;
	}

	perm.restrict_sfo_params = false;

	if (sfo.empty())
	{
		cellGame.warning("cellGameDataCheck(): directory '%s' not found", dir);
		return not_an_error(CELL_GAME_RET_NONE);
	}

	perm.exists = true;
	perm.sfo = std::move(sfo);
	return CELL_OK;
}

error_code cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath || !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.reset();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string dir = perm.dir.empty() ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + perm.dir;

	if (perm.temp.empty() && !perm.exists)
	{
		perm.reset();
		strcpy_trunc(*contentInfoPath, "");
		strcpy_trunc(*usrdirPath, "");
		return CELL_OK;
	}

	if (!perm.temp.empty())
	{
		// Create PARAM.SFO
		fs::pending_file temp(perm.temp + "/PARAM.SFO");
		temp.file.write(psf::save_object(perm.sfo));
		ensure(temp.commit());

		// Make temporary directory persistent (atomically)
		if (vfs::host::rename(perm.temp, vfs::get(dir), &g_mp_sys_dev_hdd0, false))
		{
			cellGame.success("cellGameContentPermit(): directory '%s' has been created", dir);

			// Prevent cleanup
			perm.temp.clear();
		}
		else
		{
			cellGame.error("cellGameContentPermit(): failed to initialize directory '%s' (%s)", dir, fs::g_tls_error);
		}
	}
	else if (perm.can_create)
	{
		// Update PARAM.SFO
		fs::pending_file temp(vfs::get(dir + "/PARAM.SFO"));
		temp.file.write(psf::save_object(perm.sfo));
		ensure(temp.commit());
	}

	// Cleanup
	perm.reset();

	strcpy_trunc(*contentInfoPath, dir);
	strcpy_trunc(*usrdirPath, dir + "/USRDIR");
	return CELL_OK;
}

error_code cellGameDataCheckCreate2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.error("cellGameDataCheckCreate2(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	//older sdk. it might not care about game type.

	if (version != CELL_GAMEDATA_VERSION_CURRENT || errDialog > 1 || !funcStat || sysutil_check_name_string(dirName.get_ptr(), 1, CELL_GAME_DIRNAME_SIZE) != 0)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	const std::string game_dir = dirName.get_ptr();
	const std::string dir = "/dev_hdd0/game/"s + game_dir;

	psf::registry sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	const u32 new_data = sfo.empty() && !fs::is_file(vfs::get(dir + "/PARAM.SFO")) ? CELL_GAMEDATA_ISNEWDATA_YES : CELL_GAMEDATA_ISNEWDATA_NO;

	if (!new_data)
	{
		const auto cat = psf::get_string(sfo, "CATEGORY", "");
		if (cat != "GD" && cat != "DG")
		{
			return CELL_GAMEDATA_ERROR_BROKEN;
		}
	}

	const std::string usrdir = dir + "/USRDIR";

	vm::var<CellGameDataCBResult> cbResult;
	vm::var<CellGameDataStatGet> cbGet;
	vm::var<CellGameDataStatSet> cbSet;

	cbGet->isNewData = new_data;

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	cbGet->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck

	strcpy_trunc(cbGet->contentInfoPath, dir);
	strcpy_trunc(cbGet->gameDataPath, usrdir);

	// TODO: set correct time
	cbGet->st_atime_ = 0;
	cbGet->st_ctime_ = 0;
	cbGet->st_mtime_ = 0;

	// TODO: calculate data size, if necessary
	cbGet->sizeKB = CELL_GAMEDATA_SIZEKB_NOTCALC;
	cbGet->sysSizeKB = 0; // TODO

	cbGet->getParam.attribute = CELL_GAMEDATA_ATTR_NORMAL;
	cbGet->getParam.parentalLevel = psf::get_integer(sfo, "PARENTAL_LEVEL", 0);
	strcpy_trunc(cbGet->getParam.dataVersion, psf::get_string(sfo, "APP_VER", ""));
	strcpy_trunc(cbGet->getParam.titleId, psf::get_string(sfo, "TITLE_ID", ""));
	strcpy_trunc(cbGet->getParam.title, psf::get_string(sfo, "TITLE", ""));
	for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
	{
		strcpy_trunc(cbGet->getParam.titleLang[i], psf::get_string(sfo, fmt::format("TITLE_%02d", i)));
	}

	funcStat(ppu, cbResult, cbGet, cbSet);

	std::string error_msg;

	switch (cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
	{
		cellGame.warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");
		return CELL_OK;
	}
	case CELL_GAMEDATA_CBRESULT_OK:
	{
		// Game confirmed that it wants to create directory
		const auto setParam = cbSet->setParam;

		if (new_data)
		{
			if (!setParam)
			{
				return CELL_GAMEDATA_ERROR_PARAM;
			}

			if (!fs::create_path(vfs::get(usrdir)))
			{
				return {CELL_GAME_ERROR_ACCESS_ERROR, usrdir};
			}
		}

		if (setParam)
		{
			if (new_data)
			{
				psf::assign(sfo, "CATEGORY", psf::string(3, "GD"));
			}

			psf::assign(sfo, "TITLE_ID", psf::string(CELL_GAME_SYSP_TITLEID_SIZE, setParam->titleId));
			psf::assign(sfo, "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->title));
			psf::assign(sfo, "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, setParam->dataVersion));
			psf::assign(sfo, "PARENTAL_LEVEL", +setParam->parentalLevel);

			for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
			{
				if (!setParam->titleLang[i][0])
				{
					continue;
				}

				psf::assign(sfo, fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->titleLang[i]));
			}

			fs::pending_file temp(vfs::get(dir + "/PARAM.SFO"));
			temp.file.write(psf::save_object(sfo));
			ensure(temp.commit());
		}

		return CELL_OK;
	}
	case CELL_GAMEDATA_CBRESULT_ERR_NOSPACE:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NOSPACE. Space Needed: %d KB", cbResult->errNeedSizeKB);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_NOSPACE, fmt::format("%d", cbResult->errNeedSizeKB).c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_BROKEN:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_BROKEN");
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_BROKEN, game_dir.c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_NODATA:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NODATA");
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_NODATA, game_dir.c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_INVALID:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_INVALID. Error message: %s", cbResult->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_INVALID, fmt::format("%s", cbResult->invalidMsg).c_str());
		break;

	default:
		cellGame.error("cellGameDataCheckCreate2(): callback returned unknown error (code=0x%x). Error message: %s", cbResult->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_INVALID, fmt::format("%s", cbResult->invalidMsg).c_str());
		break;
	}

	if (errDialog == CELL_GAMEDATA_ERRDIALOG_ALWAYS) // Maybe != CELL_GAMEDATA_ERRDIALOG_NONE
	{
		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Get user confirmation by opening a blocking dialog
		error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON, vm::make_str(error_msg));

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		if (res != CELL_OK)
		{
			return CELL_GAMEDATA_ERROR_INTERNAL;
		}
	}

	return CELL_GAMEDATA_ERROR_CBRESULT;
}

error_code cellGameDataCheckCreate(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellGameDataCheckCreate(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(ppu, version, dirName, errDialog, funcStat, container);
}

error_code cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_usrdirPath)
{
	cellGame.error("cellGameCreateGameData(init=*0x%x, tmp_contentInfoPath=*0x%x, tmp_usrdirPath=*0x%x)", init, tmp_contentInfoPath, tmp_usrdirPath);

	if (!init)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto _init = perm.init.access();

	if (!_init || perm.dir.empty())
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	if (!perm.can_create)
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	if (perm.exists)
	{
		return CELL_GAME_ERROR_EXIST;
	}

	std::string dirname = "_GDATA_" + std::to_string(steady_clock::now().time_since_epoch().count());
	std::string tmp_contentInfo = "/dev_hdd0/game/" + dirname;
	std::string tmp_usrdir = "/dev_hdd0/game/" + dirname + "/USRDIR";

	if (!fs::create_dir(vfs::get(tmp_contentInfo)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s' (%s)", tmp_contentInfo, fs::g_tls_error);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	if (tmp_contentInfoPath) strcpy_trunc(*tmp_contentInfoPath, tmp_contentInfo);

	if (!fs::create_dir(vfs::get(tmp_usrdir)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s' (%s)", tmp_usrdir, fs::g_tls_error);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (tmp_usrdirPath) strcpy_trunc(*tmp_usrdirPath, tmp_usrdir);

	perm.temp = vfs::get(tmp_contentInfo);
	cellGame.success("cellGameCreateGameData(): temporary directory '%s' has been created", tmp_contentInfo);

	// Initial PARAM.SFO parameters (overwrite)
	perm.sfo =
	{
		{ "CATEGORY", psf::string(3, "GD") },
		{ "TITLE_ID", psf::string(CELL_GAME_SYSP_TITLEID_SIZE, init->titleId) },
		{ "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, init->title) },
		{ "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, init->version) },
	};

	return CELL_OK;
}

error_code cellGameDeleteGameData(vm::cptr<char> dirName)
{
	cellGame.warning("cellGameDeleteGameData(dirName=%s)", dirName);

	if (!dirName)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const std::string name = dirName.get_ptr();
	const std::string dir = vfs::get("/dev_hdd0/game/"s + name);

	auto& perm = g_fxo->get<content_permission>();

	auto remove_gd = [&]() -> error_code
	{
		if (Emu.GetCat() == "GD" && Emu.GetDir().substr(Emu.GetDir().find_last_of('/') + 1) == vfs::escape(name))
		{
			// Boot patch cannot delete its own directory
			return CELL_GAME_ERROR_NOTSUPPORTED;
		}

		psf::registry sfo = psf::load_object(fs::file(dir + "/PARAM.SFO"));

		if (psf::get_string(sfo, "CATEGORY") != "GD" && fs::is_file(dir + "/PARAM.SFO"))
		{
			return CELL_GAME_ERROR_NOTSUPPORTED;
		}

		if (sfo.empty())
		{
			// Nothing to remove
			return CELL_GAME_ERROR_NOTFOUND;
		}

		if (auto id = psf::get_string(sfo, "TITLE_ID"); !id.empty() && id != Emu.GetTitleID())
		{
			cellGame.error("cellGameDeleteGameData(%s): Attempts to delete GameData with TITLE ID which does not match the program's (%s)", id, Emu.GetTitleID());
		}

		// Actually remove game data
		if (!vfs::host::remove_all(dir, Emu.GetHddDir(), &g_mp_sys_dev_hdd0, true))
		{
			return {CELL_GAME_ERROR_ACCESS_ERROR, dir};
		}

		return CELL_OK;
	};

	while (true)
	{
		// Obtain exclusive lock and cancel init
		auto _init = perm.init.init();

		if (!_init)
		{
			// Or access it
			if (auto access = perm.init.access(); access)
			{
				// Cannot remove it when it is accessed by cellGameDataCheck
				// If it is HG data then resort to remove_gd for ERROR_BROKEN
				if (perm.dir == name && perm.can_create)
				{
					return CELL_GAME_ERROR_NOTSUPPORTED;
				}

				return remove_gd();
			}
			else
			{
				// Reacquire lock
				continue;
			}
		}

		auto err = remove_gd();
		_init.cancel();
		return err;
	}
}

error_code cellGameGetParamInt(s32 id, vm::ptr<s32> value)
{
	cellGame.warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	if (!value)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	std::string key;

	switch(id)
	{
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:  key = "PARENTAL_LEVEL"; break;
	case CELL_GAME_PARAMID_RESOLUTION:      key = "RESOLUTION";     break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:    key = "SOUND_FORMAT";   break;
	default:
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}
	}

	if (!perm.sfo.count(key))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamInt(): id=%d was not found", id);
	}

	*value = psf::get_integer(perm.sfo, key, 0);
	return CELL_OK;
}

// String key restriction flags
enum class strkey_flag : u32
{
	get, // reading is restricted
	set, // writing is restricted
	read_only, // writing is disallowed (don't mind set flag in this case)

	__bitset_enum_max
};

struct string_key_info
{
	std::string_view name;
	u32 max_size = 0;
	bs_t<strkey_flag> flags;
};

static string_key_info get_param_string_key(s32 id)
{
	switch (id)
	{
	case CELL_GAME_PARAMID_TITLE:                    return {"TITLE", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set}; // TODO: Is this value correct?
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            return {"TITLE", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set};
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           return {"TITLE_00", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            return {"TITLE_01", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_FRENCH:             return {"TITLE_02", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_SPANISH:            return {"TITLE_03", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_GERMAN:             return {"TITLE_04", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            return {"TITLE_05", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_DUTCH:              return {"TITLE_06", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         return {"TITLE_07", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            return {"TITLE_08", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_KOREAN:             return {"TITLE_09", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          return {"TITLE_10", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          return {"TITLE_11", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_FINNISH:            return {"TITLE_12", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            return {"TITLE_13", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_DANISH:             return {"TITLE_14", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          return {"TITLE_15", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_POLISH:             return {"TITLE_16", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  return {"TITLE_17", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         return {"TITLE_18", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_TURKISH:            return {"TITLE_19", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::set + strkey_flag::get};

	case CELL_GAME_PARAMID_TITLE_ID:                 return {"TITLE_ID", CELL_GAME_SYSP_TITLEID_SIZE, strkey_flag::read_only};
	case CELL_GAME_PARAMID_VERSION:                  return {"VERSION", CELL_GAME_SYSP_VERSION_SIZE, strkey_flag::get + strkey_flag::read_only};
	case CELL_GAME_PARAMID_PS3_SYSTEM_VER:           return {"PS3_SYSTEM_VER", CELL_GAME_SYSP_PS3_SYSTEM_VER_SIZE}; // TODO
	case CELL_GAME_PARAMID_APP_VER:                  return {"APP_VER", CELL_GAME_SYSP_APP_VER_SIZE, strkey_flag::read_only};
	}

	return {};
}

error_code cellGameGetParamString(s32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame.warning("cellGameGetParamString(id=%d, buf=*0x%x, bufsize=%d)", id, buf, bufsize);

	if (!buf || bufsize == 0)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (key.flags & strkey_flag::get && perm.restrict_sfo_params)
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	const auto value = psf::get_string(perm.sfo, std::string(key.name));

	if (value.empty() && !perm.sfo.count(std::string(key.name)))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamString(): id=%d was not found", id);
	}

	gsl::span dst(buf.get_ptr(), bufsize);
	strcpy_trunc(dst, value);
	return CELL_OK;
}

error_code cellGameSetParamString(s32 id, vm::cptr<char> buf)
{
	cellGame.warning("cellGameSetParamString(id=%d, buf=*0x%x %s)", id, buf, buf);

	if (!buf)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (!perm.can_create || key.flags & strkey_flag::read_only || (key.flags & strkey_flag::set && perm.restrict_sfo_params))
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	psf::assign(perm.sfo, std::string(key.name), psf::string(key.max_size, buf.get_ptr()));

	return CELL_OK;
}

error_code cellGameGetSizeKB(vm::ptr<s32> size)
{
	cellGame.warning("cellGameGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// Always reset to 0 at start
	*size = 0;

	auto& perm = g_fxo->get<content_permission>();

	const auto init = perm.init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string local_dir = !perm.temp.empty() ? perm.temp : vfs::get("/dev_hdd0/game/" + perm.dir);

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (!fs::exists(local_dir))
		{
			return CELL_OK;
		}
		else
		{
			cellGame.error("cellGameGetSizeKb(): Unknown failure on calculating directory size '%s' (%s)", local_dir, error);
			return CELL_GAME_ERROR_ACCESS_ERROR;
		}
	}

	*size = ::narrow<u32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellGameGetDiscContentInfoUpdatePath(vm::ptr<char> updatePath)
{
	cellGame.todo("cellGameGetDiscContentInfoUpdatePath(updatePath=*0x%x)", updatePath);

	if (!updatePath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameGetLocalWebContentPath(vm::ptr<char> contentPath)
{
	cellGame.todo("cellGameGetLocalWebContentPath(contentPath=*0x%x)", contentPath);

	if (!contentPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, vm::cptr<char> dirName)
{
	cellGame.warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName=%s)", type, errNeedSizeKB, dirName);

	std::string error_msg;

	switch (type)
	{
	case CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA:
		// Game data is corrupted. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_GAMEDATA);
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_HDDGAME:
		// HDD boot game is corrupted. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_HDDGAME);
		break;
	case CELL_GAME_ERRDIALOG_NOSPACE:
		// Not enough available space. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_NOSPACE, fmt::format("%d", errNeedSizeKB).c_str());
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA:
		// Game data is corrupted. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_GAMEDATA);
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME:
		// HDD boot game is corrupted. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_HDDGAME);
		break;
	case CELL_GAME_ERRDIALOG_NOSPACE_EXIT:
		// Not enough available space. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_NOSPACE_EXIT, fmt::format("%d", errNeedSizeKB).c_str());
		break;
	default:
		return CELL_GAME_ERROR_PARAM;
	}

	if (dirName)
	{
		if (!memchr(dirName.get_ptr(), '\0', CELL_GAME_DIRNAME_SIZE))
		{
			return CELL_GAME_ERROR_PARAM;
		}

		error_msg += "\n" + get_localized_string(localized_string_id::CELL_GAME_ERROR_DIR_NAME, fmt::format("%s", dirName).c_str());
	}

	return open_exit_dialog(error_msg, type > CELL_GAME_ERRDIALOG_NOSPACE);
}

error_code cellGameThemeInstall(vm::cptr<char> usrdirPath, vm::cptr<char> fileName, u32 option)
{
	cellGame.todo("cellGameThemeInstall(usrdirPath=%s, fileName=%s, option=0x%x)", usrdirPath, fileName, option);

	if (!usrdirPath || !fileName || !memchr(usrdirPath.get_ptr(), '\0', CELL_GAME_PATH_MAX) || option > CELL_GAME_THEME_OPTION_APPLY)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameThemeInstallFromBuffer(u32 fileSize, u32 bufSize, vm::ptr<void> buf, vm::ptr<CellGameThemeInstallCallback> func, u32 option)
{
	cellGame.todo("cellGameThemeInstallFromBuffer(fileSize=%d, bufSize=%d, buf=*0x%x, func=*0x%x, option=0x%x)", fileSize, bufSize, buf, func, option);

	if (!buf || !fileSize || (fileSize > bufSize && !func) || bufSize <= 4095 || option > CELL_GAME_THEME_OPTION_APPLY)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellDiscGameGetBootDiscInfo(vm::ptr<CellDiscGameSystemFileParam> getParam)
{
	cellGame.warning("cellDiscGameGetBootDiscInfo(getParam=*0x%x)", getParam);

	if (!getParam)
	{
		return CELL_DISCGAME_ERROR_PARAM;
	}

	// Always sets 0 at first dword
	reinterpret_cast<nse_t<u32, 1>*>(getParam->titleId)[0] = 0;

	// This is also called by non-disc games, see NPUB90029
	static const std::string dir = "/dev_bdvd/PS3_GAME"s;

	if (!fs::is_dir(vfs::get(dir)))
	{
		return CELL_DISCGAME_ERROR_NOT_DISCBOOT;
	}

	const auto& psf = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	if (psf.count("PARENTAL_LEVEL") != 0) getParam->parentalLevel = psf.at("PARENTAL_LEVEL").as_integer();
	if (psf.count("TITLE_ID") != 0) strcpy_trunc(getParam->titleId, psf.at("TITLE_ID").as_string());

	return CELL_OK;
}

error_code cellDiscGameRegisterDiscChangeCallback(vm::ptr<CellDiscGameDiscEjectCallback> funcEject, vm::ptr<CellDiscGameDiscInsertCallback> funcInsert)
{
	cellGame.todo("cellDiscGameRegisterDiscChangeCallback(funcEject=*0x%x, funcInsert=*0x%x)", funcEject, funcInsert);

	return CELL_OK;
}

error_code cellDiscGameUnregisterDiscChangeCallback()
{
	cellGame.todo("cellDiscGameUnregisterDiscChangeCallback()");

	return CELL_OK;
}

error_code cellGameRegisterDiscChangeCallback(vm::ptr<CellGameDiscEjectCallback> funcEject, vm::ptr<CellGameDiscInsertCallback> funcInsert)
{
	cellGame.todo("cellGameRegisterDiscChangeCallback(funcEject=*0x%x, funcInsert=*0x%x)", funcEject, funcInsert);

	return CELL_OK;
}

error_code cellGameUnregisterDiscChangeCallback()
{
	cellGame.todo("cellGameUnregisterDiscChangeCallback()");

	return CELL_OK;
}

void cellSysutil_GameData_init()
{
	REG_FUNC(cellSysutil, cellHddGameCheck);
	REG_FUNC(cellSysutil, cellHddGameCheck2);
	REG_FUNC(cellSysutil, cellHddGameGetSizeKB);
	REG_FUNC(cellSysutil, cellHddGameSetSystemVer);
	REG_FUNC(cellSysutil, cellHddGameExitBroken);

	REG_FUNC(cellSysutil, cellGameDataGetSizeKB);
	REG_FUNC(cellSysutil, cellGameDataSetSystemVer);
	REG_FUNC(cellSysutil, cellGameDataExitBroken);

	REG_FUNC(cellSysutil, cellGameDataCheckCreate);
	REG_FUNC(cellSysutil, cellGameDataCheckCreate2);

	REG_FUNC(cellSysutil, cellDiscGameGetBootDiscInfo);
	REG_FUNC(cellSysutil, cellDiscGameRegisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellDiscGameUnregisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellGameRegisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellGameUnregisterDiscChangeCallback);
}

DECLARE(ppu_module_manager::cellGame)("cellGame", []()
{
	REG_FUNC(cellGame, cellGameBootCheck);
	REG_FUNC(cellGame, cellGamePatchCheck);
	REG_FUNC(cellGame, cellGameDataCheck);
	REG_FUNC(cellGame, cellGameContentPermit);

	REG_FUNC(cellGame, cellGameCreateGameData);
	REG_FUNC(cellGame, cellGameDeleteGameData);

	REG_FUNC(cellGame, cellGameGetParamInt);
	REG_FUNC(cellGame, cellGameGetParamString);
	REG_FUNC(cellGame, cellGameSetParamString);
	REG_FUNC(cellGame, cellGameGetSizeKB);
	REG_FUNC(cellGame, cellGameGetDiscContentInfoUpdatePath);
	REG_FUNC(cellGame, cellGameGetLocalWebContentPath);

	REG_FUNC(cellGame, cellGameContentErrorDialog);

	REG_FUNC(cellGame, cellGameThemeInstall);
	REG_FUNC(cellGame, cellGameThemeInstallFromBuffer);
});
