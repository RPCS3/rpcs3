#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Utilities/rMsgBox.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/PSF.h"
#include "cellSysutil.h"
#include "cellGame.h"

extern Module cellGame;

std::string contentInfo;
std::string usrdir;
bool path_set = false;

s32 cellHddGameCheck(PPUThread& CPU, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.Warning("cellHddGameCheck(version=%d, dirName=*0x%x, errDialog=%d, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	std::string dir = dirName.get_ptr();

	if (dir.size() != 9)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	vm::stackvar<CellHddGameSystemFileParam> param(CPU);
	vm::stackvar<CellHddGameCBResult> result(CPU);
	vm::stackvar<CellHddGameStatGet> get(CPU);
	vm::stackvar<CellHddGameStatSet> set(CPU);

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
		vfsFile f("/dev_hdd0/game/" + dir + "/PARAM.SFO");
		const PSFLoader psf(f);
		if (!psf)
		{
			return CELL_HDDGAME_ERROR_BROKEN;
		}

		get->getParam.parentalLevel = psf.GetInteger("PARENTAL_LEVEL");
		get->getParam.attribute = psf.GetInteger("ATTRIBUTE");
		get->getParam.resolution = psf.GetInteger("RESOLUTION");
		get->getParam.soundFormat = psf.GetInteger("SOUND_FORMAT");
		std::string title = psf.GetString("TITLE");
		strcpy_trunc(get->getParam.title, title);
		std::string app_ver = psf.GetString("APP_VER");
		strcpy_trunc(get->getParam.dataVersion, app_ver);
		strcpy_trunc(get->getParam.titleId, dir);

		for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
		{
			char key[16];
			sprintf(key, "TITLE_%02d", i);
			title = psf.GetString(key);
			strcpy_trunc(get->getParam.titleLang[i], title);
		}
	}

	// TODO ?

	//funcStat(result, get, set);

	//if (result->result != CELL_HDDGAME_CBRESULT_OK && result->result != CELL_HDDGAME_CBRESULT_OK_CANCEL)
	//{
	//	return CELL_HDDGAME_ERROR_CBRESULT;
	//}

	// TODO ?

	return CELL_OK;
}

s32 cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame.Warning("cellGameBootCheck(type=*0x%x, attributes=*0x%x, size=*0x%x, dirName=*0x%x)", type, attributes, size, dirName);

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for HG and DG games, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	vfsFile f("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);

	if (!psf)
	{
		// According to testing (in debug mode) cellGameBootCheck doesn't return an error code, when PARAM.SFO doesn't exist.
		cellGame.Error("cellGameBootCheck(): Cannot read PARAM.SFO.");
	}

	std::string category = psf.GetString("CATEGORY");
	if (category.substr(0, 2) == "DG")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, ""); // ???
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
		path_set = true;
	}
	else if (category.substr(0, 2) == "HG")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		*type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId);
		contentInfo = "/dev_hdd0/game/" + titleId;
		usrdir = "/dev_hdd0/game/" + titleId + "/USRDIR";
		path_set = true;
	}
	else if (category.substr(0, 2) == "GD")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId); // ???
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
		path_set = true;
	}
	else if (psf)
	{
		cellGame.Error("cellGameBootCheck(): Unknown CATEGORY.");
	}

	return CELL_GAME_RET_OK;
}

s32 cellGamePatchCheck(vm::ptr<CellGameContentSize> size, vm::ptr<void> reserved)
{
	cellGame.Warning("cellGamePatchCheck(size=*0x%x, reserved=*0x%x)", size, reserved);

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for patch data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	vfsFile f("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	if (!psf)
	{
		cellGame.Error("cellGamePatchCheck(): CELL_GAME_ERROR_ACCESS_ERROR (cannot read PARAM.SFO)");
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	std::string category = psf.GetString("CATEGORY");
	if (category.substr(0, 2) != "GD")
	{
		cellGame.Error("cellGamePatchCheck(): CELL_GAME_ERROR_NOTPATCH");
		return CELL_GAME_ERROR_NOTPATCH;
	}

	std::string titleId = psf.GetString("TITLE_ID");
	contentInfo = "/dev_hdd0/game/" + titleId;
	usrdir = "/dev_hdd0/game/" + titleId + "/USRDIR";
	path_set = true;
	
	return CELL_GAME_RET_OK;
}

s32 cellGameDataCheck(u32 type, vm::cptr<char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame.Warning("cellGameDataCheck(type=%d, dirName=*0x%x, size=*0x%x)", type, dirName, size);

	if ((type - 1) >= 3)
	{
		cellGame.Error("cellGameDataCheck(): CELL_GAME_ERROR_PARAM");
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
			cellGame.Warning("cellGameDataCheck(): /dev_bdvd/PS3_GAME not found");
			contentInfo = "";
			usrdir = "";
			path_set = true;
			return CELL_GAME_RET_NONE;
		}

		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
		path_set = true;
	}
	else
	{
		const std::string dir = std::string("/dev_hdd0/game/") + dirName.get_ptr();

		if (!Emu.GetVFS().ExistsDir(dir))
		{
			cellGame.Warning("cellGameDataCheck(): '%s' directory not found", dir.c_str());
			contentInfo = "";
			usrdir = "";
			path_set = true;
			return CELL_GAME_RET_NONE;
		}

		contentInfo = dir;
		usrdir = dir + "/USRDIR";
		path_set = true;
	}

	return CELL_GAME_RET_OK;
}

s32 cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.Warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath && !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	cellGame.Warning("cellGameContentPermit(): path_set=%d, contentInfo='%s', usrdir='%s'", path_set, contentInfo, usrdir);
	
	if (!path_set)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	if (contentInfo.size() == 9 && usrdir.empty())
	{
		if (Emu.GetVFS().RenameDir("/dev_hdd0/game/TMP_" + contentInfo, "/dev_hdd0/game/" + contentInfo))
		{
			cellGame.Success("cellGameContentPermit(): gamedata directory created ('/dev_hdd0/game/%s')", contentInfo);
		}

		contentInfo = "/dev_hdd0/game/" + contentInfo;
		usrdir = contentInfo + "/USRDIR";
	}

	strcpy_trunc(*contentInfoPath, contentInfo);
	strcpy_trunc(*usrdirPath, usrdir);

	contentInfo = "";
	usrdir = "";
	path_set = false;
	
	return CELL_GAME_RET_OK;
}

s32 cellGameDataCheckCreate2(PPUThread& CPU, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.Warning("cellGameDataCheckCreate2(version=0x%x, dirName=*0x%x, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	if (version != CELL_GAMEDATA_VERSION_CURRENT || errDialog > 1)
	{
		cellGame.Error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_PARAM");
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	// TODO: output errors (errDialog)

	const std::string dir = std::string("/dev_hdd0/game/") + dirName.get_ptr();

	if (!Emu.GetVFS().ExistsDir(dir))
	{
		cellGame.Todo("cellGameDataCheckCreate2(): creating directory '%s'", dir.c_str());
		// TODO: create data
		return CELL_GAMEDATA_RET_OK;
	}

	vfsFile f("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	if (!psf)
	{
		cellGame.Error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_BROKEN (cannot read PARAM.SFO)");
		return CELL_GAMEDATA_ERROR_BROKEN;
	}

	vm::stackvar<CellGameDataCBResult> cbResult(CPU);
	vm::stackvar<CellGameDataStatGet> cbGet(CPU);
	vm::stackvar<CellGameDataStatSet> cbSet(CPU);

	cbGet.value() = {};

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
	cbGet->getParam.parentalLevel = psf.GetInteger("PARENTAL_LEVEL");
	strcpy_trunc(cbGet->getParam.dataVersion, psf.GetString("APP_VER"));
	strcpy_trunc(cbGet->getParam.titleId, psf.GetString("TITLE_ID"));
	strcpy_trunc(cbGet->getParam.title, psf.GetString("TITLE"));
	// TODO: write lang titles

	funcStat(CPU, cbResult, cbGet, cbSet);

	if (cbSet->setParam)
	{
		// TODO: write PARAM.SFO from cbSet
		cellGame.Todo("cellGameDataCheckCreate2(): writing PARAM.SFO parameters (addr=0x%x)", cbSet->setParam);
	}

	switch ((s32)cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
		// TODO: do not process game data
		cellGame.Warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");

	case CELL_GAMEDATA_CBRESULT_OK:
		return CELL_GAMEDATA_RET_OK;

	case CELL_GAMEDATA_CBRESULT_ERR_NOSPACE: // TODO: process errors, error message and needSizeKB result
		cellGame.Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NOSPACE");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_BROKEN:
		cellGame.Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_BROKEN");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_NODATA:
		cellGame.Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NODATA");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_INVALID:
		cellGame.Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_INVALID");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	default:
		cellGame.Error("cellGameDataCheckCreate2(): callback returned unknown error (code=0x%x)");
		return CELL_GAMEDATA_ERROR_CBRESULT;
	}
}

s32 cellGameDataCheckCreate(PPUThread& CPU, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.Warning("cellGameDataCheckCreate(version=0x%x, dirName=*0x%x, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(CPU, version, dirName, errDialog, funcStat, container);
}

s32 cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_usrdirPath)
{
	cellGame.Error("cellGameCreateGameData(init=*0x%x, tmp_contentInfoPath=*0x%x, tmp_usrdirPath=*0x%x)", init, tmp_contentInfoPath, tmp_usrdirPath);

	std::string dir = init->titleId;
	std::string tmp_contentInfo = "/dev_hdd0/game/TMP_" + dir;
	std::string tmp_usrdir = "/dev_hdd0/game/TMP_" + dir + "/USRDIR";

	if (!Emu.GetVFS().CreateDir(tmp_contentInfo))
	{
		cellGame.Error("cellGameCreateGameData(): failed to create content directory ('%s')", tmp_contentInfo);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (!Emu.GetVFS().CreateDir(tmp_usrdir))
	{
		cellGame.Error("cellGameCreateGameData(): failed to create USRDIR directory ('%s')", tmp_usrdir);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	strcpy_trunc(*tmp_contentInfoPath, tmp_contentInfo);
	strcpy_trunc(*tmp_usrdirPath, tmp_usrdir);

	contentInfo = dir;
	usrdir.clear();
	path_set = true;

	cellGame.Success("cellGameCreateGameData(): temporary gamedata directory created ('%s')", tmp_contentInfo);

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
	cellGame.Warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	// TODO: Access through cellGame***Check functions
	vfsFile f("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	if (!psf)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	switch(id)
	{
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:  *value = psf.GetInteger("PARENTAL_LEVEL"); break;
	case CELL_GAME_PARAMID_RESOLUTION:      *value = psf.GetInteger("RESOLUTION");     break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:    *value = psf.GetInteger("SOUND_FORMAT");   break;

	default:
		return CELL_GAME_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

s32 cellGameGetParamString(u32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame.Warning("cellGameGetParamString(id=%d, buf=*0x%x, bufsize=%d)", id, buf, bufsize);

	// TODO: Access through cellGame***Check functions
	vfsFile f("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	if (!psf)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	std::string data;
	switch(id)
	{
	case CELL_GAME_PARAMID_TITLE:                    data = psf.GetString("TITLE");    break; // TODO: Is this value correct?
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            data = psf.GetString("TITLE");    break;
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           data = psf.GetString("TITLE_00"); break;
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            data = psf.GetString("TITLE_01"); break;
	case CELL_GAME_PARAMID_TITLE_FRENCH:             data = psf.GetString("TITLE_02"); break;
	case CELL_GAME_PARAMID_TITLE_SPANISH:            data = psf.GetString("TITLE_03"); break;
	case CELL_GAME_PARAMID_TITLE_GERMAN:             data = psf.GetString("TITLE_04"); break;
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            data = psf.GetString("TITLE_05"); break;
	case CELL_GAME_PARAMID_TITLE_DUTCH:              data = psf.GetString("TITLE_06"); break;
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         data = psf.GetString("TITLE_07"); break;
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            data = psf.GetString("TITLE_08"); break;
	case CELL_GAME_PARAMID_TITLE_KOREAN:             data = psf.GetString("TITLE_09"); break;
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          data = psf.GetString("TITLE_10"); break;
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          data = psf.GetString("TITLE_11"); break;
	case CELL_GAME_PARAMID_TITLE_FINNISH:            data = psf.GetString("TITLE_12"); break;
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            data = psf.GetString("TITLE_13"); break;
	case CELL_GAME_PARAMID_TITLE_DANISH:             data = psf.GetString("TITLE_14"); break;
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          data = psf.GetString("TITLE_15"); break;
	case CELL_GAME_PARAMID_TITLE_POLISH:             data = psf.GetString("TITLE_16"); break;
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  data = psf.GetString("TITLE_17"); break;
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         data = psf.GetString("TITLE_18"); break;

	case CELL_GAME_PARAMID_TITLE_ID:                 data = psf.GetString("TITLE_ID");       break;
	case CELL_GAME_PARAMID_VERSION:                  data = psf.GetString("PS3_SYSTEM_VER"); break;
	case CELL_GAME_PARAMID_APP_VER:                  data = psf.GetString("APP_VER");        break;

	default:
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (data.size() >= bufsize) data.resize(bufsize - 1);
	memcpy(buf.get_ptr(), data.c_str(), data.size() + 1);
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
	cellGame.Warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName=*0x%x)", type, errNeedSizeKB, dirName);

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
		errorMsg += fmt::Format("\nDirectory name: %s", dirName.get_ptr());
	}

	rMessageBox(errorMsg, "Error", rICON_ERROR | rOK);

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

Module cellGame("cellGame", []()
{
	contentInfo = "";
	usrdir = "";
	path_set = false;

	// (TODO: Disc Exchange functions missing)

	REG_FUNC(cellGame, cellGameBootCheck);
	REG_FUNC(cellGame, cellGamePatchCheck);
	REG_FUNC(cellGame, cellGameDataCheck);
	REG_FUNC(cellGame, cellGameContentPermit);

	REG_FUNC(cellGame, cellGameCreateGameData);
	REG_FUNC(cellGame, cellGameDeleteGameData);

	REG_FUNC(cellGame, cellGameGetParamInt);
	//cellGame.AddFunc(, cellGameSetParamInt);
	REG_FUNC(cellGame, cellGameGetParamString);
	REG_FUNC(cellGame, cellGameSetParamString);
	REG_FUNC(cellGame, cellGameGetSizeKB);
	REG_FUNC(cellGame, cellGameGetDiscContentInfoUpdatePath);
	REG_FUNC(cellGame, cellGameGetLocalWebContentPath);

	REG_FUNC(cellGame, cellGameContentErrorDialog);

	REG_FUNC(cellGame, cellGameThemeInstall);
	REG_FUNC(cellGame, cellGameThemeInstallFromBuffer);
	//cellGame.AddFunc(, CellGameThemeInstallCallback);
});

void cellSysutil_GameData_init()
{
	REG_FUNC(cellGame, cellHddGameCheck);
	//REG_FUNC(cellGame, cellHddGameCheck2);
	//REG_FUNC(cellGame, cellHddGameGetSizeKB);
	//REG_FUNC(cellGame, cellHddGameSetSystemVer);
	//REG_FUNC(cellGame, cellHddGameExitBroken);

	REG_FUNC(cellGame, cellGameDataCheckCreate);
	REG_FUNC(cellGame, cellGameDataCheckCreate2);
}
