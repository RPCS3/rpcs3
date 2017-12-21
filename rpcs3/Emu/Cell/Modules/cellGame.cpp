#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"

#include <thread>

logs::channel cellGame("cellGame");

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

// If dir is empty:
// contentInfo = "/dev_bdvd/PS3_GAME"
// usrdir = "/dev_bdvd/PS3_GAME/USRDIR"
// Temporary content directory (dir is not empty):
// contentInfo = "/dev_hdd1/game/" + dir
// usrdir = "/dev_hdd1/game/" + dir + "/USRDIR"
// Normal content directory (dir is not empty):
// contentInfo = "/dev_hdd0/game/" + dir
// usrdir = "/dev_hdd0/game/" + dir + "/USRDIR"
struct content_permission final
{
	// Content directory name or path
	const std::string dir;

	// SFO file
	psf::registry sfo;

	// Temporary directory path
	std::string temp;

	template <typename Dir, typename Sfo>
	content_permission(Dir&& dir, Sfo&& sfo)
		: dir(std::forward<Dir>(dir))
		, sfo(std::forward<Sfo>(sfo))
	{
	}

	~content_permission()
	{
		try
		{
			if (temp.size() > 1)
			{
				fs::remove_all(temp);
			}
		}
		catch (...)
		{
			cellGame.fatal("Failed to clean directory '/dev_hdd1/game/%s'", dir);
			catch_all_exceptions();
		}
	}
};

s32 cellHddGameCheck(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.error("cellHddGameCheck(version=%d, dirName=%s, errDialog=%d, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	std::string dir = dirName.get_ptr();

	if (dir.size() != 9)
	{
		return (s32)CELL_HDDGAME_ERROR_PARAM;
	}

	vm::var<CellHddGameCBResult> result;
	vm::var<CellHddGameStatGet> get;
	vm::var<CellHddGameStatSet> set;

	get->hddFreeSizeKB = 40 * 1024 * 1024; // 40 GB, TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	get->isNewData = CELL_HDDGAME_ISNEWDATA_EXIST;
	get->sysSizeKB = 0; // TODO
	get->atime = 0; // TODO
	get->ctime = 0; // TODO
	get->mtime = 0; // TODO
	get->sizeKB = CELL_HDDGAME_SIZEKB_NOTCALC;
	strcpy_trunc(get->contentInfoPath, "/dev_hdd0/game/" + dir);
	strcpy_trunc(get->hddGamePath, "/dev_hdd0/game/" + dir + "/USRDIR");

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
		return (s32)CELL_HDDGAME_ERROR_CBRESULT;
	}

	// TODO ?

	return CELL_OK;
}

s32 cellHddGameCheck2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellHddGameGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellHddGameGetSizeKB(size=*0x%x)", size);

	const std::string local_dir = vfs::get("/dev_hdd0/game/" + Emu.GetTitleID());

	if (!fs::is_dir(local_dir))
	{
		return (s32)CELL_HDDGAME_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir) / 1024);

	return CELL_OK;
}

s32 cellHddGameSetSystemVer()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellHddGameExitBroken()
{
	fmt::throw_exception("Unimplemented" HERE);
}


s32 cellGameDataGetSizeKB(vm::ptr<u32> size)
{
	cellGame.warning("cellGameDataGetSizeKB(size=*0x%x)", size);

	const std::string local_dir = vfs::get("/dev_hdd0/game/" + Emu.GetTitleID());

	if (!fs::is_dir(local_dir))
	{
		return CELL_GAMEDATA_ERROR_FAILURE;
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir) / 1024);

	return CELL_OK;
}

s32 cellGameDataSetSystemVer()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellGameDataExitBroken()
{
	fmt::throw_exception("Unimplemented" HERE);
}


error_code cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame.warning("cellGameBootCheck(type=*0x%x, attributes=*0x%x, size=*0x%x, dirName=*0x%x)", type, attributes, size, dirName);

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for HG and DG games, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	if (!type)
		return CELL_GAME_ERROR_PARAM;
	// According to testing (in debug mode) cellGameBootCheck doesn't return an error code, when PARAM.SFO doesn't exist.
	psf::registry sfo = psf::load_object(fs::file(vfs::get("/app_home/../PARAM.SFO")));

	const std::string& category = psf::get_string(sfo, "CATEGORY");

	if (category == "DG")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		// TODO: dirName might be a read only string when BootCheck is called on a disc game. (e.g. Ben 10 Ultimate Alien: Cosmic Destruction)

		if (!fxm::make<content_permission>("", std::move(sfo)))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else if (category == "AP" || category == "AV" || category == "HG" || category == "AT" || category == "AM" || category == "SF")
	{
		*type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, Emu.GetTitleID());

		if (!fxm::make<content_permission>(Emu.GetTitleID(), std::move(sfo)))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else if (category == "GD")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO
		if (dirName) strcpy_trunc(*dirName, Emu.GetTitleID()); // ???

		if (!fxm::make<content_permission>("", std::move(sfo)))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else
	{
		// Hack: When there is no (or unknown) CATEGORY returned, instead of throwing an exception
		// we assume it's a disk game.
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0;
		cellGame.error("cellGameBootCheck(): Unknown CATEGORY: %s", category);
	}

	return CELL_OK;
}

error_code cellGamePatchCheck(vm::ptr<CellGameContentSize> size, vm::ptr<void> reserved)
{
	cellGame.warning("cellGamePatchCheck(size=*0x%x, reserved=*0x%x)", size, reserved);

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for patch data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	psf::registry sfo = psf::load_object(fs::file(vfs::get("/app_home/../PARAM.SFO")));

	if (psf::get_string(sfo, "CATEGORY") != "GD")
	{
		return CELL_GAME_ERROR_NOTPATCH;
	}

	if (!fxm::make<content_permission>(Emu.GetTitleID(), std::move(sfo)))
	{
		return CELL_GAME_ERROR_BUSY;
	}

	return CELL_OK;
}

error_code cellGameDataCheck(u32 type, vm::cptr<char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame.warning("cellGameDataCheck(type=%d, dirName=%s, size=*0x%x)", type, dirName, size);

	if ((type - 1) >= 3)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; //40 GB

		// TODO: Calculate data size for game data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	// TODO: not sure what should be checked there
	const auto prm = fxm::make<content_permission>(type == CELL_GAME_GAMETYPE_DISC ? "" : dirName.get_ptr(), psf::registry{});

	if (!prm)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	const std::string dir = prm->dir.empty() ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + prm->dir;

	if (!fs::is_dir(vfs::get(dir)))
	{
		cellGame.warning("cellGameDataCheck(): directory '%s' not found", dir);
		return not_an_error(CELL_GAME_RET_NONE);
	}

	prm->sfo = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	return CELL_OK;
}

error_code cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath && !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = fxm::get<content_permission>();

	if (!prm)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string dir = prm->dir.empty() ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + prm->dir;

	if (!prm->temp.empty())
	{
		// Make temporary directory persistent
		const auto vdir = vfs::get(dir);

		if (fs::exists(vdir))
		{
			fmt::throw_exception("cellGameContentPermit(): epic fail: directory '%s' already exists", dir);
		}

		if (fs::rename(prm->temp, vdir, false))
		{
			cellGame.success("cellGameContentPermit(): directory '%s' has been created", dir);
		}
		else
		{
			fmt::throw_exception("cellGameContentPermit(): failed to initialize directory '%s'", dir);
		}

		// Create PARAM.SFO
		psf::save_object(fs::file(vdir + "/PARAM.SFO", fs::rewrite), prm->sfo);

		// Disable deletion
		prm->temp.clear();
	}

	strcpy_trunc(*contentInfoPath, dir);
	strcpy_trunc(*usrdirPath, dir + "/USRDIR");
	verify(HERE), fxm::remove<content_permission>();

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

	const std::string& dir = "/dev_hdd0/game/"s + dirName.get_ptr();

	vm::var<CellGameDataCBResult> cbResult;
	vm::var<CellGameDataStatGet>  cbGet;
	vm::var<CellGameDataStatSet>  cbSet;
	cbGet->isNewData = fs::is_dir(vfs::get(dir)) ? CELL_GAMEDATA_ISNEWDATA_NO : CELL_GAMEDATA_ISNEWDATA_YES;

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	cbGet->hddFreeSizeKB = 40000000; //40 GB


	strcpy_trunc(cbGet->contentInfoPath, dir);
	strcpy_trunc(cbGet->gameDataPath, dir + "/USRDIR");

	// TODO: set correct time
	cbGet->st_atime_ = 0;
	cbGet->st_ctime_ = 0;
	cbGet->st_mtime_ = 0;

	// TODO: calculate data size, if necessary
	cbGet->sizeKB = CELL_GAMEDATA_SIZEKB_NOTCALC;
	cbGet->sysSizeKB = 0;

	psf::registry sfo = psf::load_object(fs::file(vfs::get("/app_home/../PARAM.SFO")));

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


	switch ((s32)cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
		// TODO: do not process game data(directory)
		cellGame.warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");
		return CELL_OK;


	case CELL_GAMEDATA_CBRESULT_OK:
		//game confirmed that it wants to create directory
		if (!fs::is_dir(vfs::get(dir + "/USRDIR")) && !fs::create_path(vfs::get(dir + "/USRDIR")))
			{
				cellGame.error("cellGameDataCheckCreate2(): folder creation failed");
				return CELL_GAME_ERROR_NOSPACE;	//don't know which error. picked one at random
			}
		if (cbSet->setParam)
		{
			const auto vdir = vfs::get(dir);

			//older SDK does not define not settable values, hopefully it doesn't just change some values(overwrite)
			psf::registry sfo_write =
			{
				{ "TITLE_ID", psf::string(CELL_GAME_SYSP_TITLEID_SIZE, cbSet->setParam->titleId) },
				{ "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, cbSet->setParam->title) },
				{ "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, cbSet->setParam->dataVersion) },
				{ "PARENTAL_LEVEL", cbSet->setParam->parentalLevel.value()							}
			};
			//sfo_write.emplace("PARENTAL_LEVEL", cbSet->setParam->parentalLevel.value()); // I don't care about age restrictions.
			for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
			{
				sfo_write.emplace(fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, cbSet->setParam->titleLang[i]));
			}
			if (!fs::is_dir(vdir))
			{
				cellGame.fatal("directory where param.sfo is to be created does not exist");
				return CELL_GAME_ERROR_INTERNAL;
			}
			psf::save_object(fs::file(vdir + "/PARAM.SFO", fs::rewrite), sfo_write);

		}
		return CELL_OK;

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

s32 cellGameDataCheckCreate(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellGameDataCheckCreate(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(ppu, version, dirName, errDialog, funcStat, container);
}

error_code cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_usrdirPath)
{
	cellGame.error("cellGameCreateGameData(init=*0x%x, tmp_contentInfoPath=*0x%x, tmp_usrdirPath=*0x%x)", init, tmp_contentInfoPath, tmp_usrdirPath);

	const auto prm = fxm::get<content_permission>();

	if (!prm || prm->dir.empty())
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	std::string tmp_contentInfo = "/dev_hdd1/game/" + prm->dir;
	std::string tmp_usrdir = "/dev_hdd1/game/" + prm->dir + "/USRDIR";

	if (!fs::create_dir(vfs::get(tmp_contentInfo)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s'", tmp_contentInfo);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (!fs::create_dir(vfs::get(tmp_usrdir)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s'", tmp_usrdir);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	strcpy_trunc(*tmp_contentInfoPath, tmp_contentInfo);
	strcpy_trunc(*tmp_usrdirPath, tmp_usrdir);

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

s32 cellGameDeleteGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

error_code cellGameGetParamInt(s32 id, vm::ptr<s32> value)
{
	cellGame.warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	const auto prm = fxm::get<content_permission>();

	if (!prm)
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

	*value = psf::get_integer(prm->sfo, key, 0);
	return CELL_OK;
}

static const char* get_param_string_key(s32 id)
{
	switch (id)
	{
	case CELL_GAME_PARAMID_TITLE:                    return "TITLE"; // TODO: Is this value correct?
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            return "TITLE";
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           return "TITLE_00";
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            return "TITLE_01";
	case CELL_GAME_PARAMID_TITLE_FRENCH:             return "TITLE_02";
	case CELL_GAME_PARAMID_TITLE_SPANISH:            return "TITLE_03";
	case CELL_GAME_PARAMID_TITLE_GERMAN:             return "TITLE_04";
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            return "TITLE_05";
	case CELL_GAME_PARAMID_TITLE_DUTCH:              return "TITLE_06";
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         return "TITLE_07";
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            return "TITLE_08";
	case CELL_GAME_PARAMID_TITLE_KOREAN:             return "TITLE_09";
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          return "TITLE_10";
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          return "TITLE_11";
	case CELL_GAME_PARAMID_TITLE_FINNISH:            return "TITLE_12";
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            return "TITLE_13";
	case CELL_GAME_PARAMID_TITLE_DANISH:             return "TITLE_14";
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          return "TITLE_15";
	case CELL_GAME_PARAMID_TITLE_POLISH:             return "TITLE_16";
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  return "TITLE_17";
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         return "TITLE_18";
	case CELL_GAME_PARAMID_TITLE_TURKISH:            return "TITLE_19";

	case CELL_GAME_PARAMID_TITLE_ID:                 return "TITLE_ID";
	case CELL_GAME_PARAMID_VERSION:                  return "VERSION";
	case CELL_GAME_PARAMID_PS3_SYSTEM_VER:           return "PS3_SYSTEM_VER";
	case CELL_GAME_PARAMID_APP_VER:                  return "APP_VER";
	}

	return nullptr;
}

error_code cellGameGetParamString(s32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame.warning("cellGameGetParamString(id=%d, buf=*0x%x, bufsize=%d)", id, buf, bufsize);

	if (!buf || bufsize == 0)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = fxm::get<content_permission>();

	if (!prm)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (!key)
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	std::string value = psf::get_string(prm->sfo, key);
	value.resize(bufsize - 1);

	std::memcpy(buf.get_ptr(), value.c_str(), bufsize);

	return CELL_OK;
}

error_code cellGameSetParamString(s32 id, vm::cptr<char> buf)
{
	cellGame.warning("cellGameSetParamString(id=%d, buf=*0x%x)", id, buf);

	if (!buf)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto prm = fxm::get<content_permission>();

	if (!prm)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (!key)
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	u32 max_size = CELL_GAME_SYSP_TITLE_SIZE;

	switch (id)
	{
	case CELL_GAME_PARAMID_TITLE_ID:       max_size = CELL_GAME_SYSP_TITLEID_SIZE; break;
	case CELL_GAME_PARAMID_VERSION:        max_size = CELL_GAME_SYSP_VERSION_SIZE; break;
	case CELL_GAME_PARAMID_PS3_SYSTEM_VER: max_size = CELL_GAME_SYSP_PS3_SYSTEM_VER_SIZE; break;
	case CELL_GAME_PARAMID_APP_VER:        max_size = CELL_GAME_SYSP_APP_VER_SIZE; break;
	}

	prm->sfo.emplace(key, psf::string(max_size, buf.get_ptr()));

	return CELL_OK;
}

error_code cellGameGetSizeKB(vm::ptr<s32> size)
{
	cellGame.warning("cellGameGetSizeKB(size=*0x%x)", size);

	const auto prm = fxm::get<content_permission>();

	if (!prm)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string local_dir = !prm->temp.empty() ? prm->temp : vfs::get("/dev_hdd0/game/" + prm->dir);

	if (!fs::is_dir(local_dir))
	{
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	*size = ::narrow<u32>(fs::get_dir_size(local_dir) / 1024);

	return CELL_OK;
}

s32 cellGameGetDiscContentInfoUpdatePath()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameGetLocalWebContentPath()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

error_code cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, vm::cptr<char> dirName)
{
	cellGame.warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName=%s)", type, errNeedSizeKB, dirName);

	std::string errorName;
	switch (type)
	{
	case CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA:      errorName = "Game data is corrupted (can be continued).";          break;
	case CELL_GAME_ERRDIALOG_BROKEN_HDDGAME:       errorName = "HDD boot game is corrupted (can be continued).";      break;
	case CELL_GAME_ERRDIALOG_NOSPACE:              errorName = "Not enough available space (can be continued).";      break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA: errorName = "Game data is corrupted (terminate application).";     break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME:  errorName = "HDD boot game is corrupted (terminate application)."; break;
	case CELL_GAME_ERRDIALOG_NOSPACE_EXIT:         errorName = "Not enough available space (terminate application)."; break;
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
		errorMsg += fmt::format("\nDirectory name: %s", dirName);
	}

	const auto dlg = Emu.GetCallbacks().get_msg_dialog();

	dlg->type.bg_invisible = true;
	dlg->type.button_type = 2; // OK
	dlg->type.disable_cancel = true;

	atomic_t<bool> result(false);

	dlg->on_close = [&](s32 status)
	{
		result = true;
	};

	Emu.CallAfter([&]()
	{
		dlg->Create(errorMsg);
	});

	while (!result)
	{
		thread_ctrl::wait_for(1000);
	}

	return CELL_OK;
}

s32 cellGameThemeInstall()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameThemeInstallFromBuffer()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}


s32 cellDiscGameGetBootDiscInfo(vm::ptr<CellDiscGameSystemFileParam> getParam)
{
	cellGame.warning("cellDiscGameGetBootDiscInfo(getParam=*0x%x)", getParam);

	// This is also called by non-disc games, see NPUB90029
	const std::string dir = "/dev_bdvd/PS3_GAME"s;

	if (!fs::is_dir(vfs::get(dir)))
	{
		// Not a disc game. TODO: Fetch PARAM.SFO from proper game dir
		cellGame.warning("cellDiscGameGetBootDiscInfo(): directory '%s' not found", dir);
		getParam->parentalLevel = 0;
		strcpy_trunc(getParam->titleId, "0");
		return CELL_OK;
	}

	const auto& psf = psf::load_object(fs::file(vfs::get(dir + "/PARAM.SFO")));

	if (psf.count("PARENTAL_LEVEL") != 0) getParam->parentalLevel = psf.at("PARENTAL_LEVEL").as_integer();
	if (psf.count("TITLE_ID") != 0) strcpy_trunc(getParam->titleId, psf.at("TITLE_ID").as_string());

	return CELL_OK;
}

s32 cellDiscGameRegisterDiscChangeCallback()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellDiscGameUnregisterDiscChangeCallback()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameRegisterDiscChangeCallback()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameUnregisterDiscChangeCallback()
{
	UNIMPLEMENTED_FUNC(cellGame);
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
