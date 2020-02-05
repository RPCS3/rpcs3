#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"
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
	atomic_t<bool> restrict_sfo_params = true;

	content_permission() = default;

	content_permission(const content_permission&) = delete;

	content_permission& operator=(const content_permission&) = delete;

	~content_permission()
	{
		bool success = false;
		fs::g_tls_error = fs::error::ok;

		try
		{
			if (temp.size() <= 1 || fs::remove_all(temp))
			{
				success = true;
			}
		}
		catch (...)
		{
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

	std::string dir = dirName.get_ptr();

	// TODO: Find error code
	verify(HERE), dir.size() == 9;

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
	strcpy_trunc(get->contentInfoPath, "/dev_hdd0/game/" + dir);
	strcpy_trunc(get->hddGamePath, "/dev_hdd0/game/" + dir + "/USRDIR");

	vm::var<CellHddGameSystemFileParam> setParam;
	set->setParam = setParam;

	const std::string& local_dir = vfs::get("/dev_hdd0/game/" + dir);

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

	if (result->result != CELL_HDDGAME_CBRESULT_OK && result->result != CELL_HDDGAME_CBRESULT_OK_CANCEL)
	{
		return CELL_HDDGAME_ERROR_CBRESULT;
	}

	// TODO ?

	return CELL_OK;
}

error_code cellHddGameCheck2()
{
	cellGame.todo("cellHddGameCheck2()");
	return CELL_OK;
}

error_code cellHddGameGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellHddGameGetSizeKB(size=*0x%x)", size);

	const std::string local_dir = vfs::get(Emu.GetDir());

	if (!fs::is_dir(local_dir))
	{
		return CELL_HDDGAME_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir, 1024) / 1024);

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
	return open_exit_dialog("There has been an error!\n\nPlease reinstall the HDD boot game.", true);
}

error_code cellGameDataGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellGameDataGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	const std::string local_dir = vfs::get(Emu.GetDir());

	if (!fs::is_dir(local_dir))
	{
		return CELL_GAMEDATA_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir, 1024) / 1024);

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
	return open_exit_dialog("There has been an error!\n\nPlease remove the game data for this title.", true);
}

error_code cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame.warning("cellGameBootCheck(type=*0x%x, attributes=*0x%x, size=*0x%x, dirName=*0x%x)", type, attributes, size, dirName);

	if (!type || !attributes)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto perm = g_fxo->get<content_permission>();

	const auto init = perm->init.init();

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

	if (*type == CELL_GAME_GAMETYPE_HDD && dirName)
	{
		strcpy_trunc(*dirName, Emu.GetTitleID());
	}

	perm->dir = std::move(dir);
	perm->sfo = std::move(sfo);

	perm->temp.clear();
	perm->can_create = 0;

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

	const auto perm = g_fxo->get<content_permission>();

	const auto init = perm->init.init();

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

	perm->dir = Emu.GetTitleID();
	perm->sfo = std::move(sfo);

	perm->temp.clear();
	perm->can_create = 0;

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

	const auto perm = g_fxo->get<content_permission>();

	const auto init = perm->init.init();

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for game data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0; // TODO
	}

	perm->dir = std::move(name);
	perm->sfo.clear();
	perm->temp.clear();

	if (type == CELL_GAME_GAMETYPE_GAMEDATA)
	{
		perm->can_create = true;
	}

	if (!fs::is_dir(vfs::get(dir)))
	{
		cellGame.warning("cellGameDataCheck(): directory '%s' not found", dir);
		return not_an_error(CELL_GAME_RET_NONE);
	}

	perm->restrict_sfo_params = false;
	perm->sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));
	return CELL_OK;
}

error_code cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath || !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto perm = g_fxo->get<content_permission>();

	const auto init = perm->init.reset();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string dir = perm->dir.empty() ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + perm->dir;

	if (perm->can_create && perm->temp.empty() && !fs::is_dir(vfs::get(dir)))
	{
		strcpy_trunc(*contentInfoPath, "");
		strcpy_trunc(*usrdirPath, "");
		return CELL_OK;
	}

	if (!perm->temp.empty())
	{
		// Create PARAM.SFO
		psf::save_object(fs::file(perm->temp + "/PARAM.SFO", fs::rewrite), perm->sfo);

		// Make temporary directory persistent (atomically)
		if (vfs::host::rename(perm->temp, vfs::get(dir), false))
		{
			cellGame.success("cellGameContentPermit(): directory '%s' has been created", dir);

			// Prevent cleanup
			perm->temp.clear();
		}
		else
		{
			cellGame.error("cellGameContentPermit(): failed to initialize directory '%s' (%s)", dir, fs::g_tls_error);
		}
	}

	strcpy_trunc(*contentInfoPath, dir);
	strcpy_trunc(*usrdirPath, dir + "/USRDIR");
	return CELL_OK;
}

error_code cellGameDataCheckCreate2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.error("cellGameDataCheckCreate2(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	//older sdk. it might not care about game type.

	if (version != CELL_GAMEDATA_VERSION_CURRENT || errDialog > 1)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	// TODO: output errors (errDialog)

	const std::string dir = "/dev_hdd0/game/"s + dirName.get_ptr();

	vm::var<CellGameDataCBResult> cbResult;
	vm::var<CellGameDataStatGet>  cbGet;
	vm::var<CellGameDataStatSet>  cbSet;
	cbGet->isNewData = fs::is_dir(vfs::get(dir)) ? CELL_GAMEDATA_ISNEWDATA_NO : CELL_GAMEDATA_ISNEWDATA_YES;


	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	cbGet->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck


	strcpy_trunc(cbGet->contentInfoPath, dir);
	strcpy_trunc(cbGet->gameDataPath, dir + "/USRDIR");

	// TODO: set correct time
	cbGet->st_atime_ = 0;
	cbGet->st_ctime_ = 0;
	cbGet->st_mtime_ = 0;

	// TODO: calculate data size, if necessary
	cbGet->sizeKB = CELL_GAMEDATA_SIZEKB_NOTCALC;
	cbGet->sysSizeKB = 0; // TODO

	psf::registry sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	cbGet->getParam.attribute = CELL_GAMEDATA_ATTR_NORMAL;
	cbGet->getParam.parentalLevel = psf::get_integer(sfo, "PARENTAL_LEVEL", 0);
	strcpy_trunc(cbGet->getParam.dataVersion, psf::get_string(sfo, "APP_VER", ""));
	strcpy_trunc(cbGet->getParam.titleId, psf::get_string(sfo, "TITLE_ID", ""));
	strcpy_trunc(cbGet->getParam.title, psf::get_string(sfo, "TITLE", ""));
	for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
	{
		strcpy_trunc(cbGet->getParam.titleLang[i], psf::get_string(sfo, fmt::format("TITLE_%02d", i)));
	}

	vm::var<CellGameDataSystemFileParam> setParam;
	*setParam = cbGet->getParam;
	cbSet->setParam = setParam;

	funcStat(ppu, cbResult, cbGet, cbSet);

	switch (cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
	{
		// TODO: do not process game data(directory)
		cellGame.warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");
		return CELL_OK;
	}

	case CELL_GAMEDATA_CBRESULT_OK:
	{
		// Game confirmed that it wants to create directory
		const std::string usrdir = dir + "/USRDIR";
		const std::string vusrdir = vfs::get(usrdir);

		if (!fs::is_dir(vusrdir) && !fs::create_path(vusrdir))
		{
			return {CELL_GAME_ERROR_ACCESS_ERROR, usrdir};
		}

		if (cbSet->setParam)
		{
			psf::assign(sfo, "CATEGORY", psf::string(3, "GD"));
			psf::assign(sfo, "TITLE_ID", psf::string(CELL_GAME_SYSP_TITLEID_SIZE, cbSet->setParam->titleId));
			psf::assign(sfo, "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, cbSet->setParam->title));
			psf::assign(sfo, "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, cbSet->setParam->dataVersion));
			psf::assign(sfo, "PARENTAL_LEVEL", cbSet->setParam->parentalLevel.value());

			for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
			{
				if (!cbSet->setParam->titleLang[i][0])
				{
					continue;
				}

				psf::assign(sfo, fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, cbSet->setParam->titleLang[i]));
			}

			const auto vdir = vfs::get(dir);

			if (!fs::is_dir(vdir))
			{
				return {CELL_GAME_ERROR_INTERNAL, dir};
			}

			psf::save_object(fs::file(vdir + "/PARAM.SFO", fs::rewrite), sfo);
		}

		return CELL_OK;
	}
	case CELL_GAMEDATA_CBRESULT_ERR_NOSPACE: // TODO: process errors, error message and needSizeKB result
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NOSPACE");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_BROKEN:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_BROKEN");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_NODATA:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NODATA");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_INVALID:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_INVALID");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	default:
		cellGame.error("cellGameDataCheckCreate2(): callback returned unknown error (code=0x%x)");
		return CELL_GAMEDATA_ERROR_CBRESULT;
	}
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

	const auto prm = g_fxo->get<content_permission>();

	const auto _init = prm->init.access();

	if (!_init || prm->dir.empty())
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	if (!prm->can_create)
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
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

	prm->temp = vfs::get(tmp_contentInfo);
	cellGame.success("cellGameCreateGameData(): temporary directory '%s' has been created", tmp_contentInfo);

	// Initial PARAM.SFO parameters (overwrite)
	prm->sfo =
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
	cellGame.todo("cellGameDeleteGameData(dirName=%s)", dirName);

	if (!dirName)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameGetParamInt(s32 id, vm::ptr<s32> value)
{
	cellGame.warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	if (!value)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = g_fxo->get<content_permission>();

	const auto init = prm->init.access();

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

	if (!prm->sfo.count(key))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamInt(): id=%d was not found", id);
	}

	*value = psf::get_integer(prm->sfo, key, 0);
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
	bs_t<strkey_flag> flags;
};

static string_key_info get_param_string_key(s32 id)
{
	switch (id)
	{
	case CELL_GAME_PARAMID_TITLE:                    return {"TITLE", strkey_flag::set}; // TODO: Is this value correct?
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            return {"TITLE", strkey_flag::set};
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           return {"TITLE_00", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            return {"TITLE_01", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_FRENCH:             return {"TITLE_02", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_SPANISH:            return {"TITLE_03", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_GERMAN:             return {"TITLE_04", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            return {"TITLE_05", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_DUTCH:              return {"TITLE_06", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         return {"TITLE_07", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            return {"TITLE_08", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_KOREAN:             return {"TITLE_09", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          return {"TITLE_10", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          return {"TITLE_11", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_FINNISH:            return {"TITLE_12", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            return {"TITLE_13", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_DANISH:             return {"TITLE_14", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          return {"TITLE_15", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_POLISH:             return {"TITLE_16", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  return {"TITLE_17", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         return {"TITLE_18", strkey_flag::set + strkey_flag::get};
	case CELL_GAME_PARAMID_TITLE_TURKISH:            return {"TITLE_19", strkey_flag::set + strkey_flag::get};

	case CELL_GAME_PARAMID_TITLE_ID:                 return {"TITLE_ID", strkey_flag::read_only};
	case CELL_GAME_PARAMID_VERSION:                  return {"VERSION", strkey_flag::get + strkey_flag::read_only};
	case CELL_GAME_PARAMID_PS3_SYSTEM_VER:           return {"PS3_SYSTEM_VER"}; // TODO
	case CELL_GAME_PARAMID_APP_VER:                  return {"APP_VER", strkey_flag::read_only};
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

	const auto prm = g_fxo->get<content_permission>();

	const auto init = prm->init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (key.flags & strkey_flag::get && prm->restrict_sfo_params)
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	const std::string value = psf::get_string(prm->sfo, std::string(key.name));
	const auto value_size = value.size() + 1;

	if (value.empty() && !prm->sfo.count(std::string(key.name)))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamString(): id=%d was not found", id);
	}

	const auto pbuf = buf.get_ptr();
	const bool to_pad = bufsize > value_size;
	std::memcpy(pbuf, value.c_str(), to_pad ? value_size : bufsize);

	if (to_pad)
	{
		std::memset(pbuf + value_size, 0, bufsize - value_size);
	}

	return CELL_OK;
}

error_code cellGameSetParamString(s32 id, vm::cptr<char> buf)
{
	cellGame.warning("cellGameSetParamString(id=%d, buf=*0x%x)", id, buf);

	if (!buf)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = g_fxo->get<content_permission>();

	const auto init = prm->init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (key.flags & strkey_flag::read_only || (key.flags & strkey_flag::set && prm->restrict_sfo_params))
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	u32 max_size = CELL_GAME_SYSP_TITLE_SIZE;

	switch (id)
	{
	case CELL_GAME_PARAMID_VERSION:        max_size = CELL_GAME_SYSP_VERSION_SIZE; break; // ??
	}

	psf::assign(prm->sfo, std::string(key.name), psf::string(max_size, buf.get_ptr()));

	return CELL_OK;
}

error_code cellGameGetSizeKB(vm::ptr<s32> size)
{
	cellGame.warning("cellGameGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = g_fxo->get<content_permission>();

	const auto init = prm->init.access();

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string local_dir = !prm->temp.empty() ? prm->temp : vfs::get("/dev_hdd0/game/" + prm->dir);

	if (!fs::is_dir(local_dir))
	{
		if (fs::g_tls_error == fs::error::noent)
		{
			*size = 0;
			return CELL_OK;
		}
		else
		{
			cellGame.error("cellGameGetSizeKb(): unexpexcted error on path '%s' (%s)", local_dir, fs::g_tls_error);
			return CELL_GAME_ERROR_ACCESS_ERROR;
		}
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir, 1024) / 1024);

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

	std::string errorName;
	switch (type)
	{
	case CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA:      errorName = "Game data is corrupted. The application will continue.";          break;
	case CELL_GAME_ERRDIALOG_BROKEN_HDDGAME:       errorName = "HDD boot game is corrupted. The application will continue.";      break;
	case CELL_GAME_ERRDIALOG_NOSPACE:              errorName = "Not enough available space. The application will continue.";      break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA: errorName = "Game data is corrupted. The application will be terminated.";     break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME:  errorName = "HDD boot game is corrupted. The application will be terminated."; break;
	case CELL_GAME_ERRDIALOG_NOSPACE_EXIT:         errorName = "Not enough available space. The application will be terminated."; break;
	default: return CELL_GAME_ERROR_PARAM;
	}

	std::string errorMsg;
	if (type == CELL_GAME_ERRDIALOG_NOSPACE || type == CELL_GAME_ERRDIALOG_NOSPACE_EXIT)
	{
		errorMsg = fmt::format("ERROR: %s\nSpace needed: %d KB", errorName, errNeedSizeKB);
	}
	else
	{
		errorMsg = fmt::format("ERROR: %s", errorName);
	}

	if (dirName)
	{
		if (!memchr(dirName.get_ptr(), '\0', CELL_GAME_DIRNAME_SIZE))
		{
			return CELL_GAME_ERROR_PARAM;
		}

		errorMsg += fmt::format("\nDirectory name: %s", dirName);
	}

	return open_exit_dialog(errorMsg, type > CELL_GAME_ERRDIALOG_NOSPACE);
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
		return CELL_DISCGAME_ERROR_PARAM;

	// This is also called by non-disc games, see NPUB90029
	const std::string dir = "/dev_bdvd/PS3_GAME"s;

	if (!fs::is_dir(vfs::get(dir)))
	{
		getParam->parentalLevel = 0;
		strcpy_trunc(getParam->titleId, "0");

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
