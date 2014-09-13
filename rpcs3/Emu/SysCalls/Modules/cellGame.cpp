#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "Utilities/rMsgBox.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/PSF.h"
#include "cellGame.h"

Module *cellGame = nullptr;

std::string contentInfo = "";
std::string usrdir = "";

int cellGameBootCheck(vm::ptr<be_t<u32>> type, vm::ptr<be_t<u32>> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame->Warning("cellGameBootCheck(type_addr=0x%x, attributes_addr=0x%x, size_addr=0x%x, dirName_addr=0x%x)",
		type.addr(), attributes.addr(), size.addr(), dirName.addr());

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for HG and DG games, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	vfsFile f("/app_home/PARAM.SFO");
	if (!f.IsOpened())
	{
		cellGame->Error("cellGameBootCheck(): CELL_GAME_ERROR_ACCESS_ERROR (cannot open PARAM.SFO)");
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	PSFLoader psf(f);
	if (!psf.Load(false))
	{
		cellGame->Error("cellGameBootCheck(): CELL_GAME_ERROR_ACCESS_ERROR (cannot read PARAM.SFO)");
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	std::string category = psf.GetString("CATEGORY");
	if (category.substr(0, 2) == "DG")
	{
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, ""); // ???
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
	}
	else if (category.substr(0, 2) == "HG")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		*type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId);
		contentInfo = "/dev_hdd0/game/" + titleId;
		usrdir = "/dev_hdd0/game/" + titleId + "/USRDIR";
	}
	else if (category.substr(0, 2) == "GD")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		*type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO
		if (dirName) strcpy_trunc(*dirName, titleId); // ???
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
	}
	else
	{
		cellGame->Error("cellGameBootCheck(): CELL_GAME_ERROR_FAILURE (unknown CATEGORY)");
		return CELL_GAME_ERROR_FAILURE;
	}

	return CELL_GAME_RET_OK;
}

int cellGamePatchCheck(vm::ptr<CellGameContentSize> size, u32 reserved_addr)
{
	cellGame->Warning("cellGamePatchCheck(size_addr=0x%x, reserved_addr=0x%x)", size.addr(), reserved_addr);

	if (reserved_addr != 0)
	{
		cellGame->Error("cellGamePatchCheck(): CELL_GAME_ERROR_PARAM");
		return CELL_GAME_ERROR_PARAM;
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40000000; // 40 GB

		// TODO: Calculate data size for patch data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0;
	}

	vfsFile f("/app_home/PARAM.SFO");
	if (!f.IsOpened())
	{
		cellGame->Error("cellGamePatchCheck(): CELL_GAME_ERROR_ACCESS_ERROR (cannot open PARAM.SFO)");
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	PSFLoader psf(f);
	if (!psf.Load(false))
	{
		cellGame->Error("cellGamePatchCheck(): CELL_GAME_ERROR_ACCESS_ERROR (cannot read PARAM.SFO)");
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	std::string category = psf.GetString("CATEGORY");
	if (category.substr(0, 2) != "GD")
	{
		cellGame->Error("cellGamePatchCheck(): CELL_GAME_ERROR_NOTPATCH");
		return CELL_GAME_ERROR_NOTPATCH;
	}

	std::string titleId = psf.GetString("TITLE_ID");
	contentInfo = "/dev_hdd0/game/" + titleId;
	usrdir = "/dev_hdd0/game/" + titleId + "/USRDIR";
	
	return CELL_GAME_RET_OK;
}

int cellGameDataCheck(u32 type, vm::ptr<const char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame->Warning("cellGameDataCheck(type=0x%x, dirName_addr=0x%x, size_addr=0x%x)", type, dirName.addr(), size.addr());

	if ((type - 1) >= 3)
	{
		cellGame->Error("cellGameDataCheck(): CELL_GAME_ERROR_PARAM");
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
			cellGame->Warning("cellGameDataCheck(): /dev_bdvd/PS3_GAME not found");
			return CELL_GAME_RET_NONE;
		}
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
	}
	else
	{
		const std::string dir = std::string("/dev_hdd0/game/") + dirName.get_ptr();

		if (!Emu.GetVFS().ExistsDir(dir))
		{
			cellGame->Warning("cellGameDataCheck(): '%s' directory not found", dir.c_str());
			return CELL_GAME_RET_NONE;
		}
		contentInfo = dir;
		usrdir = dir + "/USRDIR";
	}

	return CELL_GAME_RET_OK;
}

int cellGameContentPermit(vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame->Warning("cellGameContentPermit(contentInfoPath_addr=0x%x, usrdirPath_addr=0x%x)",
		contentInfoPath.addr(), usrdirPath.addr());
	
	if (contentInfo == "" && usrdir == "")
	{
		cellGame->Warning("cellGameContentPermit(): CELL_GAME_ERROR_FAILURE (no permission given)");
		return CELL_GAME_ERROR_FAILURE;
	}

	// TODO: make it better
	strcpy_trunc(*contentInfoPath, contentInfo);
	strcpy_trunc(*usrdirPath, usrdir);

	contentInfo = "";
	usrdir = "";
	
	return CELL_GAME_RET_OK;
}

int cellGameDataCheckCreate2(u32 version, vm::ptr<const char> dirName, u32 errDialog,
	vm::ptr<void(*)(vm::ptr<CellGameDataCBResult> cbResult, vm::ptr<CellGameDataStatGet> get, vm::ptr<CellGameDataStatSet> set)> funcStat, u32 container)
{
	cellGame->Warning("cellGameDataCheckCreate2(version=0x%x, dirName_addr=0x%x, errDialog=0x%x, funcStat_addr=0x%x, container=%d)",
		version, dirName.addr(), errDialog, funcStat.addr(), container);

	if (version != CELL_GAMEDATA_VERSION_CURRENT || errDialog > 1)
	{
		cellGame->Error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_PARAM");
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	// TODO: output errors (errDialog)

	const std::string dir = std::string("/dev_hdd0/game/") + dirName.get_ptr();

	if (!Emu.GetVFS().ExistsDir(dir))
	{
		cellGame->Todo("cellGameDataCheckCreate2(): creating directory '%s'", dir.c_str());
		// TODO: create data
		return CELL_GAMEDATA_RET_OK;
	}

	vfsFile f(dir + "/PARAM.SFO");
	if (!f.IsOpened())
	{
		cellGame->Error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_BROKEN (cannot open PARAM.SFO)");
		return CELL_GAMEDATA_ERROR_BROKEN;
	}

	PSFLoader psf(f);
	if (!psf.Load(false))
	{
		cellGame->Error("cellGameDataCheckCreate2(): CELL_GAMEDATA_ERROR_BROKEN (cannot read PARAM.SFO)");
		return CELL_GAMEDATA_ERROR_BROKEN;
	}

	// TODO: use memory container
	vm::var<CellGameDataCBResult> cbResult;
	vm::var<CellGameDataStatGet> cbGet;
	vm::var<CellGameDataStatSet> cbSet;

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

	funcStat(cbResult, cbGet, cbSet);

	if (cbSet->setParam)
	{
		// TODO: write PARAM.SFO from cbSet
		cellGame->Todo("cellGameDataCheckCreate2(): writing PARAM.SFO parameters (addr=0x%x)", cbSet->setParam.addr());
	}

	switch ((s32)cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
		// TODO: do not process game data
		cellGame->Warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");

	case CELL_GAMEDATA_CBRESULT_OK:
		return CELL_GAMEDATA_RET_OK;

	case CELL_GAMEDATA_CBRESULT_ERR_NOSPACE: // TODO: process errors, error message and needSizeKB result
		cellGame->Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NOSPACE");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_BROKEN:
		cellGame->Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_BROKEN");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_NODATA:
		cellGame->Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NODATA");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	case CELL_GAMEDATA_CBRESULT_ERR_INVALID:
		cellGame->Error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_INVALID");
		return CELL_GAMEDATA_ERROR_CBRESULT;

	default:
		cellGame->Error("cellGameDataCheckCreate2(): callback returned unknown error (code=0x%x)");
		return CELL_GAMEDATA_ERROR_CBRESULT;
	}
}

int cellGameDataCheckCreate(u32 version, vm::ptr<const char> dirName, u32 errDialog, 
	vm::ptr<void(*)(vm::ptr<CellGameDataCBResult> cbResult, vm::ptr<CellGameDataStatGet> get, vm::ptr<CellGameDataStatSet> set)> funcStat, u32 container)
{
	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(version, dirName, errDialog, funcStat, container);
}

int cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char> tmp_contentInfoPath, vm::ptr<char> tmp_usrdirPath)
{
	cellGame->Todo("cellGameCreateGameData(init_addr=0x%x, tmp_contentInfoPath_addr=0x%x, tmp_usrdirPath_addr=0x%x)",
		init.addr(), tmp_contentInfoPath.addr(), tmp_usrdirPath.addr());

	// TODO: create temporary game directory, set initial PARAM.SFO parameters
	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	return CELL_OK;
}

int cellGameDeleteGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetParamInt(u32 id, vm::ptr<be_t<u32>> value)
{
	cellGame->Warning("cellGameGetParamInt(id=%d, value_addr=0x%x)", id, value.addr());

	// TODO: Access through cellGame***Check functions
	vfsFile f("/app_home/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return CELL_GAME_ERROR_FAILURE;

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

int cellGameGetParamString(u32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame->Warning("cellGameGetParamString(id=%d, buf_addr=0x%x, bufsize=%d)", id, buf.addr(), bufsize);

	// TODO: Access through cellGame***Check functions
	vfsFile f("/app_home/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return CELL_GAME_ERROR_FAILURE;

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

int cellGameSetParamString()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetSizeKB()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetDiscContentInfoUpdatePath()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetLocalWebContentPath()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, vm::ptr<const char> dirName)
{
	cellGame->Warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName_addr=0x%x)", type, errNeedSizeKB, dirName.addr());

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
		errorMsg = fmt::Format("ERROR: %s\nSpace needed: %d KB", errorName.c_str(), errNeedSizeKB, dirName);
	}
	else
	{
		errorMsg = fmt::Format("ERROR: %s", errorName.c_str());
	}

	if (dirName)
	{
		errorMsg += fmt::Format("\nDirectory name: %s", dirName.get_ptr());
	}

	rMessageBox(errorMsg, "Error", rICON_ERROR | rOK);

	return CELL_OK;
}

int cellGameThemeInstall()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameThemeInstallFromBuffer()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

void cellGame_init(Module *pxThis)
{
	cellGame = pxThis;

	// (TODO: Disc Exchange functions missing)

	cellGame->AddFunc(0xf52639ea, cellGameBootCheck);
	cellGame->AddFunc(0xce4374f6, cellGamePatchCheck);
	cellGame->AddFunc(0xdb9819f3, cellGameDataCheck);
	cellGame->AddFunc(0x70acec67, cellGameContentPermit);

	cellGame->AddFunc(0x42a2e133, cellGameCreateGameData);
	cellGame->AddFunc(0xb367c6e3, cellGameDeleteGameData);

	cellGame->AddFunc(0xb7a45caf, cellGameGetParamInt);
	//cellGame->AddFunc(, cellGameSetParamInt);
	cellGame->AddFunc(0x3a5d726a, cellGameGetParamString);
	cellGame->AddFunc(0xdaa5cd20, cellGameSetParamString);
	cellGame->AddFunc(0xef9d42d5, cellGameGetSizeKB);
	cellGame->AddFunc(0x2a8e6b92, cellGameGetDiscContentInfoUpdatePath);
	cellGame->AddFunc(0xa80bf223, cellGameGetLocalWebContentPath);

	cellGame->AddFunc(0xb0a1f8c6, cellGameContentErrorDialog);

	cellGame->AddFunc(0xd24e3928, cellGameThemeInstall);
	cellGame->AddFunc(0x87406734, cellGameThemeInstallFromBuffer);
	//cellGame->AddFunc(, CellGameThemeInstallCallback);
}
