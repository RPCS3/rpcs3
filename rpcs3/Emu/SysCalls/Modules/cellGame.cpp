#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/PSF.h"
#include "cellSysutil.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

extern Module<> cellGame;

// Normal content directory (if is_temporary is not involved):
// contentInfo = dir
// usrdir = dir + "/USRDIR"
// Temporary content directory:
// contentInfo = "/dev_hdd1/game/" + dir
// usrdir = "/dev_hdd1/game/" + dir + "/USRDIR"
// Usual (persistent) content directory (if is_temporary):
// contentInfo = "/dev_hdd0/game/" + dir
// usrdir = "/dev_hdd0/game/" + dir + "/USRDIR"
struct content_permission_t final
{
	// content directory name or path
	const std::string dir;

	// true if temporary directory is created and must be moved or deleted
	bool is_temporary = false;

	content_permission_t(const std::string& dir, bool is_temp)
		: dir(dir)
		, is_temporary(is_temp)
	{
	}

	~content_permission_t()
	{
		if (is_temporary)
		{
			Emu.GetVFS().DeleteAll("/dev_hdd1/game/" + dir);
		}
	}
};

s32 cellHddGameCheck(PPUThread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellHddGameCheck(version=%d, dirName=*0x%x, errDialog=%d, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	std::string dir = dirName.get_ptr();

	if (dir.size() != 9)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	vm::var<CellHddGameSystemFileParam> param;
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

	if (!Emu.GetVFS().ExistsDir("/dev_hdd0/game/" + dir))
	{
		get->isNewData = CELL_HDDGAME_ISNEWDATA_NODIR;
	}
	else
	{
		// TODO: Is cellHddGameCheck really responsible for writing the information in get->getParam ? (If not, delete this else)
		const auto& psf = psf::load(vfsFile("/dev_hdd0/game/" + dir + "/PARAM.SFO").VRead<char>());

		get->getParam.parentalLevel = psf.at("PARENTAL_LEVEL").as_integer();
		get->getParam.attribute = psf.at("ATTRIBUTE").as_integer();
		get->getParam.resolution = psf.at("RESOLUTION").as_integer();
		get->getParam.soundFormat = psf.at("SOUND_FORMAT").as_integer();
		strcpy_trunc(get->getParam.title, psf.at("TITLE").as_string());
		strcpy_trunc(get->getParam.dataVersion, psf.at("APP_VER").as_string());
		strcpy_trunc(get->getParam.titleId, psf.at("TITLE_ID").as_string());

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

s32 cellHddGameCheck2()
{
	throw EXCEPTION("");
}

s32 cellHddGameGetSizeKB()
{
	throw EXCEPTION("");
}

s32 cellHddGameSetSystemVer()
{
	throw EXCEPTION("");
}

s32 cellHddGameExitBroken()
{
	throw EXCEPTION("");
}


s32 cellGameDataGetSizeKB()
{
	throw EXCEPTION("");
}

s32 cellGameDataSetSystemVer()
{
	throw EXCEPTION("");
}

s32 cellGameDataExitBroken()
{
	throw EXCEPTION("");
}


s32 cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
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

	const auto& psf = psf::load(vfsFile("/app_home/../PARAM.SFO").VRead<char>());

	if (psf.empty())
	{
		// According to testing (in debug mode) cellGameBootCheck doesn't return an error code, when PARAM.SFO doesn't exist.
		cellGame.error("cellGameBootCheck(): Cannot read PARAM.SFO.");
		return CELL_GAME_RET_OK;
	}

	const std::string& category = psf.at("CATEGORY").as_string();
	if (category == "DG")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, ""); // ???

		if (!fxm::make<content_permission_t>("/dev_bdvd/PS3_GAME", false))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else if (category == "HG")
	{
		const std::string& titleId = psf.at("TITLE_ID").as_string();
		*type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId); 

		if (!fxm::make<content_permission_t>("/dev_hdd0/game/" + titleId, false))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else if (category == "GD")
	{
		const std::string& titleId = psf.at("TITLE_ID").as_string();
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId); // ???

		if (!fxm::make<content_permission_t>("/dev_bdvd/PS3_GAME", false))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else
	{
		cellGame.error("cellGameBootCheck(): Unknown CATEGORY.");
	}

	return CELL_GAME_RET_OK;
}

s32 cellGamePatchCheck(vm::ptr<CellGameContentSize> size, vm::ptr<void> reserved)
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

	const auto& psf = psf::load(vfsFile("/app_home/../PARAM.SFO").VRead<char>());

	const std::string& category = psf.at("CATEGORY").as_string();
	if (category != "GD")
	{
		cellGame.error("cellGamePatchCheck(): CELL_GAME_ERROR_NOTPATCH");
		return CELL_GAME_ERROR_NOTPATCH;
	}

	if (!fxm::make<content_permission_t>("/dev_hdd0/game/" + psf.at("TITLE_ID").as_string(), false))
	{
		return CELL_GAME_ERROR_BUSY;
	}
	
	return CELL_OK;
}

s32 cellGameDataCheck(u32 type, vm::cptr<char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame.warning("cellGameDataCheck(type=%d, dirName=*0x%x, size=*0x%x)", type, dirName, size);

	if ((type - 1) >= 3)
	{
		cellGame.error("cellGameDataCheck(): CELL_GAME_ERROR_PARAM");
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

	if (type == CELL_GAME_GAMETYPE_DISC)
	{
		// TODO: not sure what should be checked there

		if (!Emu.GetVFS().ExistsDir("/dev_bdvd/PS3_GAME"))
		{
			cellGame.warning("cellGameDataCheck(): /dev_bdvd/PS3_GAME not found");
			return CELL_GAME_RET_NONE;
		}

		if (!fxm::make<content_permission_t>("/dev_bdvd/PS3_GAME", false))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}
	else
	{
		const std::string dir = "/dev_hdd0/game/"s + dirName.get_ptr();

		if (!Emu.GetVFS().ExistsDir(dir))
		{
			cellGame.warning("cellGameDataCheck(): '%s' directory not found", dir.c_str());
			return CELL_GAME_RET_NONE;
		}

		if (!fxm::make<content_permission_t>(dir, false))
		{
			return CELL_GAME_ERROR_BUSY;
		}
	}

	return CELL_GAME_RET_OK;
}

s32 cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath && !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const auto path_set = fxm::withdraw<content_permission_t>();
	
	if (!path_set)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	if (path_set->is_temporary)
	{
		const std::string dir = "/dev_hdd0/game/" + path_set->dir;

		// make temporary directory persistent
		if (Emu.GetVFS().Rename("/dev_hdd1/game/" + path_set->dir, dir))
		{
			cellGame.success("cellGameContentPermit(): '%s' directory created", dir);
		}
		else
		{
			throw EXCEPTION("Cannot create gamedata directory");
		}

		// prevent deleting directory
		path_set->is_temporary = false;

		strcpy_trunc(*contentInfoPath, dir);
		strcpy_trunc(*usrdirPath, dir + "/USRDIR");
	}
	else
	{
		strcpy_trunc(*contentInfoPath, path_set->dir);
		strcpy_trunc(*usrdirPath, path_set->dir + "/USRDIR");
	}
	
	return CELL_OK;
}

s32 cellGameDataCheckCreate2(PPUThread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellGameDataCheckCreate2(version=0x%x, dirName=*0x%x, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	if (version != CELL_GAMEDATA_VERSION_CURRENT || errDialog > 1)
	{
		cellGame.error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_PARAM");
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	// TODO: output errors (errDialog)

	const std::string dir = "/dev_hdd0/game/"s + dirName.get_ptr();

	if (!Emu.GetVFS().ExistsDir(dir))
	{
		cellGame.todo("cellGameDataCheckCreate2(): creating directory '%s'", dir.c_str());
		// TODO: create data
		return CELL_GAMEDATA_RET_OK;
	}

	const auto& psf = psf::load(vfsFile("/app_home/../PARAM.SFO").VRead<char>());

	vm::var<CellGameDataCBResult> cbResult;
	vm::var<CellGameDataStatGet>  cbGet;
	vm::var<CellGameDataStatSet>  cbSet;

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	cbGet->hddFreeSizeKB = 40000000; //40 GB

	cbGet->isNewData = CELL_GAMEDATA_ISNEWDATA_NO;
	strcpy_trunc(cbGet->contentInfoPath, dir);
	strcpy_trunc(cbGet->gameDataPath, dir + "/USRDIR");

	// TODO: set correct time
	cbGet->st_atime_ = 0;
	cbGet->st_ctime_ = 0;
	cbGet->st_mtime_ = 0;

	// TODO: calculate data size, if necessary
	cbGet->sizeKB = CELL_GAMEDATA_SIZEKB_NOTCALC;
	cbGet->sysSizeKB = 0;

	cbGet->getParam.attribute = CELL_GAMEDATA_ATTR_NORMAL;
	cbGet->getParam.parentalLevel = psf.at("PARENTAL_LEVEL").as_integer();
	strcpy_trunc(cbGet->getParam.dataVersion, psf.at("APP_VER").as_string());
	strcpy_trunc(cbGet->getParam.titleId, psf.at("TITLE_ID").as_string());
	strcpy_trunc(cbGet->getParam.title, psf.at("TITLE").as_string());
	// TODO: write lang titles

	funcStat(ppu, cbResult, cbGet, cbSet);

	if (cbSet->setParam)
	{
		// TODO: write PARAM.SFO from cbSet
		cellGame.todo("cellGameDataCheckCreate2(): writing PARAM.SFO parameters (addr=0x%x)", cbSet->setParam);
	}

	switch ((s32)cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
		// TODO: do not process game data
		cellGame.warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");

	case CELL_GAMEDATA_CBRESULT_OK:
		return CELL_GAMEDATA_RET_OK;

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

s32 cellGameDataCheckCreate(PPUThread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellGameDataCheckCreate(version=0x%x, dirName=*0x%x, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(ppu, version, dirName, errDialog, funcStat, container);
}

s32 cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_usrdirPath)
{
	cellGame.error("cellGameCreateGameData(init=*0x%x, tmp_contentInfoPath=*0x%x, tmp_usrdirPath=*0x%x)", init, tmp_contentInfoPath, tmp_usrdirPath);

	std::string dir = init->titleId;
	std::string tmp_contentInfo = "/dev_hdd1/game/" + dir;
	std::string tmp_usrdir = "/dev_hdd1/game/" + dir + "/USRDIR";

	if (!Emu.GetVFS().CreateDir(tmp_contentInfo))
	{
		cellGame.error("cellGameCreateGameData(): failed to create content directory ('%s')", tmp_contentInfo);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (!Emu.GetVFS().CreateDir(tmp_usrdir))
	{
		cellGame.error("cellGameCreateGameData(): failed to create USRDIR directory ('%s')", tmp_usrdir);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (!fxm::make<content_permission_t>(dir, true))
	{
		return CELL_GAME_ERROR_BUSY;
	}

	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	strcpy_trunc(*tmp_contentInfoPath, tmp_contentInfo);
	strcpy_trunc(*tmp_usrdirPath, tmp_usrdir);

	cellGame.success("cellGameCreateGameData(): temporary gamedata directory created ('%s')", tmp_contentInfo);

	// TODO: set initial PARAM.SFO parameters
	
	return CELL_OK;
}

s32 cellGameDeleteGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameGetParamInt(u32 id, vm::ptr<u32> value)
{
	cellGame.warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	const auto& psf = psf::load(vfsFile("/app_home/../PARAM.SFO").VRead<char>());

	std::string key;

	switch(id)
	{
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:  key = "PARENTAL_LEVEL"; break;
	case CELL_GAME_PARAMID_RESOLUTION:      key = "RESOLUTION";     break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:    key = "SOUND_FORMAT";   break;
	default:
		cellGame.error("cellGameGetParamInt(): Unimplemented parameter (%d)", id);
		return CELL_GAME_ERROR_INVALID_ID;
	}

	*value = psf.at(key).as_integer();
	return CELL_OK;
}

s32 cellGameGetParamString(u32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame.warning("cellGameGetParamString(id=%d, buf=*0x%x, bufsize=%d)", id, buf, bufsize);

	const auto& psf = psf::load(vfsFile("/app_home/../PARAM.SFO").VRead<char>());

	std::string key;
	switch(id)
	{
	case CELL_GAME_PARAMID_TITLE:                    key = "TITLE";    break; // TODO: Is this value correct?
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            key = "TITLE";    break;
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           key = "TITLE_00"; break;
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            key = "TITLE_01"; break;
	case CELL_GAME_PARAMID_TITLE_FRENCH:             key = "TITLE_02"; break;
	case CELL_GAME_PARAMID_TITLE_SPANISH:            key = "TITLE_03"; break;
	case CELL_GAME_PARAMID_TITLE_GERMAN:             key = "TITLE_04"; break;
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            key = "TITLE_05"; break;
	case CELL_GAME_PARAMID_TITLE_DUTCH:              key = "TITLE_06"; break;
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         key = "TITLE_07"; break;
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            key = "TITLE_08"; break;
	case CELL_GAME_PARAMID_TITLE_KOREAN:             key = "TITLE_09"; break;
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          key = "TITLE_10"; break;
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          key = "TITLE_11"; break;
	case CELL_GAME_PARAMID_TITLE_FINNISH:            key = "TITLE_12"; break;
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            key = "TITLE_13"; break;
	case CELL_GAME_PARAMID_TITLE_DANISH:             key = "TITLE_14"; break;
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          key = "TITLE_15"; break;
	case CELL_GAME_PARAMID_TITLE_POLISH:             key = "TITLE_16"; break;
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  key = "TITLE_17"; break;
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         key = "TITLE_18"; break;

	case CELL_GAME_PARAMID_TITLE_ID:                 key = "TITLE_ID";       break;
	case CELL_GAME_PARAMID_VERSION:                  key = "PS3_SYSTEM_VER"; break;
	case CELL_GAME_PARAMID_APP_VER:                  key = "APP_VER";        break;

	default:
		cellGame.error("cellGameGetParamString(): Unimplemented parameter (%d)", id);
		return CELL_GAME_ERROR_INVALID_ID;
	}

	const std::string& value = psf.at(key).as_string().substr(0, bufsize - 1);

	std::copy_n(value.c_str(), value.size() + 1, buf.get_ptr());

	return CELL_OK;
}

s32 cellGameSetParamString()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

s32 cellGameGetSizeKB()
{
	UNIMPLEMENTED_FUNC(cellGame);
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

s32 cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, vm::cptr<char> dirName)
{
	cellGame.warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName=*0x%x)", type, errNeedSizeKB, dirName);

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
		errorMsg += fmt::format("\nDirectory name: %s", dirName.get_ptr());
	}

	const auto dlg = Emu.GetCallbacks().get_msg_dialog();

	dlg->type.bg_invisible = true;
	dlg->type.button_type = 2; // OK
	dlg->type.disable_cancel = true;

	const auto p = std::make_shared<std::promise<void>>();
	std::future<void> future = p->get_future();

	dlg->on_close = [=](s32 status)
	{
		p->set_value();
	};

	dlg->Create(errorMsg);

	future.get();

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


s32 cellDiscGameGetBootDiscInfo()
{
	throw EXCEPTION("");
}

s32 cellDiscGameRegisterDiscChangeCallback()
{
	throw EXCEPTION("");
}

s32 cellDiscGameUnregisterDiscChangeCallback()
{
	throw EXCEPTION("");
}

s32 cellGameRegisterDiscChangeCallback()
{
	throw EXCEPTION("");
}

s32 cellGameUnregisterDiscChangeCallback()
{
	throw EXCEPTION("");
}


void cellSysutil_GameData_init()
{
	extern Module<> cellSysutil;

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

Module<> cellGame("cellGame", []()
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
