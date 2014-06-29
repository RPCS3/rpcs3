#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/FS/vfsFile.h"

#include "Loader/PSF.h"

//void cellGame_init();
//Module cellGame(0x003e, cellGame_init);
extern Module *cellGame = nullptr;

// Return Codes
enum
{
	CELL_GAME_RET_OK                   = 0,
	CELL_GAME_RET_CANCEL               = 1,
	CELL_GAME_RET_NONE                 = 2,
	CELL_GAME_ERROR_NOTFOUND           = 0x8002cb04,
	CELL_GAME_ERROR_BROKEN             = 0x8002cb05,
	CELL_GAME_ERROR_INTERNAL           = 0x8002cb06,
	CELL_GAME_ERROR_PARAM              = 0x8002cb07,
	CELL_GAME_ERROR_NOAPP              = 0x8002cb08,
	CELL_GAME_ERROR_ACCESS_ERROR       = 0x8002cb09,
	CELL_GAME_ERROR_NOSPACE            = 0x8002cb20,
	CELL_GAME_ERROR_NOTSUPPORTED       = 0x8002cb21,
	CELL_GAME_ERROR_FAILURE            = 0x8002cb22,
	CELL_GAME_ERROR_BUSY               = 0x8002cb23,
	CELL_GAME_ERROR_IN_SHUTDOWN        = 0x8002cb24,
	CELL_GAME_ERROR_INVALID_ID         = 0x8002cb25,
	CELL_GAME_ERROR_EXIST              = 0x8002cb26,
	CELL_GAME_ERROR_NOTPATCH           = 0x8002cb27,
	CELL_GAME_ERROR_INVALID_THEME_FILE = 0x8002cb28,
	CELL_GAME_ERROR_BOOTPATH           = 0x8002cb50,
};

// Definitions
enum
{
	CELL_GAME_PATH_MAX              = 128,
	CELL_GAME_DIRNAME_SIZE          = 32,
	CELL_GAME_THEMEFILENAME_SIZE    = 48,
	CELL_GAME_SYSP_TITLE_SIZE       = 128,
	CELL_GAME_SYSP_TITLEID_SIZE     = 10,
	CELL_GAME_SYSP_VERSION_SIZE     = 6,
	CELL_GAME_SYSP_APP_VER_SIZE     = 6,

	CELL_GAME_GAMETYPE_DISC         = 1,
	CELL_GAME_GAMETYPE_HDD          = 2,
	
	CELL_GAME_GAMETYPE_GAMEDATA     = 3,

	CELL_GAME_SIZEKB_NOTCALC        = -1,

	CELL_GAME_ATTRIBUTE_PATCH               = 0x1,
	CELL_GAME_ATTRIBUTE_APP_HOME            = 0x2,
	CELL_GAME_ATTRIBUTE_DEBUG               = 0x4,
	CELL_GAME_ATTRIBUTE_XMBBUY              = 0x8,
	CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER   = 0x10,
	CELL_GAME_ATTRIBUTE_INVITE_MESSAGE      = 0x20,
	CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE = 0x40,
	CELL_GAME_ATTRIBUTE_WEB_BROWSER         = 0x100,
};

//Parameter IDs of PARAM.SFO
enum
{
	//Integers
	CELL_GAME_PARAMID_PARENTAL_LEVEL          = 102,
	CELL_GAME_PARAMID_RESOLUTION              = 103,
	CELL_GAME_PARAMID_SOUND_FORMAT            = 104,

	//Strings
	CELL_GAME_PARAMID_TITLE                   = 0,
	CELL_GAME_PARAMID_TITLE_DEFAULT           = 1,
	CELL_GAME_PARAMID_TITLE_JAPANESE          = 2,
	CELL_GAME_PARAMID_TITLE_ENGLISH           = 3,
	CELL_GAME_PARAMID_TITLE_FRENCH            = 4,
	CELL_GAME_PARAMID_TITLE_SPANISH           = 5,
	CELL_GAME_PARAMID_TITLE_GERMAN            = 6,
	CELL_GAME_PARAMID_TITLE_ITALIAN           = 7,
	CELL_GAME_PARAMID_TITLE_DUTCH             = 8,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE        = 9,
	CELL_GAME_PARAMID_TITLE_RUSSIAN           = 10,
	CELL_GAME_PARAMID_TITLE_KOREAN            = 11,
	CELL_GAME_PARAMID_TITLE_CHINESE_T         = 12,
	CELL_GAME_PARAMID_TITLE_CHINESE_S         = 13,
	CELL_GAME_PARAMID_TITLE_FINNISH           = 14,
	CELL_GAME_PARAMID_TITLE_SWEDISH           = 15,
	CELL_GAME_PARAMID_TITLE_DANISH            = 16,
	CELL_GAME_PARAMID_TITLE_NORWEGIAN         = 17,
	CELL_GAME_PARAMID_TITLE_POLISH            = 18,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL = 19,
	CELL_GAME_PARAMID_TITLE_ENGLISH_UK        = 20,
	CELL_GAME_PARAMID_TITLE_ID                = 100,
	CELL_GAME_PARAMID_VERSION                 = 101,
	CELL_GAME_PARAMID_APP_VER                 = 106,
};

//Error dialog types
enum
{
	CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA      =   0,
	CELL_GAME_ERRDIALOG_BROKEN_HDDGAME       =   1,
	CELL_GAME_ERRDIALOG_NOSPACE              =   2,
	CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA = 100,
	CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME  = 101,
	CELL_GAME_ERRDIALOG_NOSPACE_EXIT         = 102,
};

struct CellGameContentSize
{
	be_t<s32> hddFreeSizeKB;
	be_t<s32> sizeKB;
	be_t<s32> sysSizeKB;
};

struct CellGameSetInitParams
{
	char title[CELL_GAME_SYSP_TITLE_SIZE];
	char titleId[CELL_GAME_SYSP_TITLEID_SIZE];
	char reserved0[2];
	char version[CELL_GAME_SYSP_VERSION_SIZE];
	char reserved1[66];
};

std::string contentInfo = "";
std::string usrdir = "";

int cellGameBootCheck(mem32_t type, mem32_t attributes, mem_ptr_t<CellGameContentSize> size, mem_list_ptr_t<u8> dirName)
{
	cellGame->Warning("cellGameBootCheck(type_addr=0x%x, attributes_addr=0x%x, size_addr=0x%x, dirName_addr=0x%x)",
		type.GetAddr(), attributes.GetAddr(), size.GetAddr(), dirName.GetAddr());

	if (!type.IsGood() || !attributes.IsGood() || !size.IsGood() || !dirName.IsGood())
	{
		cellGame->Error("cellGameBootCheck(): CELL_GAME_ERROR_PARAM");
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	size->hddFreeSizeKB = 40000000; // 40 GB

	// TODO: Calculate data size for HG and DG games, if necessary.
	size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
	size->sysSizeKB = 0;

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
		type = CELL_GAME_GAMETYPE_DISC;
		attributes = 0; // TODO
		Memory.WriteString(dirName.GetAddr(), ""); // ???
		contentInfo = "/dev_bdvd/PS3_GAME";
		usrdir = "/dev_bdvd/PS3_GAME/USRDIR";
	}
	else if (category.substr(0, 2) == "HG")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		type = CELL_GAME_GAMETYPE_HDD;
		attributes = 0; // TODO
		Memory.WriteString(dirName.GetAddr(), titleId);
		contentInfo = "/dev_hdd0/game/" + titleId;
		usrdir = "/dev_hdd0/game/" + titleId + "/USRDIR";
	}
	else if (category.substr(0, 2) == "GD")
	{
		std::string titleId = psf.GetString("TITLE_ID");
		type = CELL_GAME_GAMETYPE_DISC;
		attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO
		Memory.WriteString(dirName.GetAddr(), titleId); // ???
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

int cellGamePatchCheck(mem_ptr_t<CellGameContentSize> size, u32 reserved_addr)
{
	cellGame->Warning("cellGamePatchCheck(size_addr=0x%x, reserved_addr=0x%x)", size.GetAddr(), reserved_addr);

	if (!size.IsGood() || reserved_addr != 0)
	{
		cellGame->Error("cellGamePatchCheck(): CELL_GAME_ERROR_PARAM");
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	size->hddFreeSizeKB = 40000000; // 40 GB

	// TODO: Calculate data size for patch data, if necessary.
	size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
	size->sysSizeKB = 0;

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

int cellGameDataCheck(u32 type, const mem_list_ptr_t<u8> dirName, mem_ptr_t<CellGameContentSize> size)
{
	cellGame->Warning("cellGameDataCheck(type=0x%x, dirName_addr=0x%x, size_addr=0x%x)", type, dirName.GetAddr(), size.GetAddr());

	if ((type - 1) >= 3 || !size.IsGood() || !dirName.IsGood())
	{
		cellGame->Error("cellGameDataCheck(): CELL_GAME_ERROR_PARAM");
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	size->hddFreeSizeKB = 40000000; //40 GB

	// TODO: Calculate data size for game data, if necessary.
	size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
	size->sysSizeKB = 0;

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
		std::string dir = "/dev_hdd0/game/" + std::string(dirName.GetString());

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

int cellGameContentPermit(mem_list_ptr_t<u8> contentInfoPath, mem_list_ptr_t<u8> usrdirPath)
{
	cellGame->Warning("cellGameContentPermit(contentInfoPath_addr=0x%x, usrdirPath_addr=0x%x)",
		contentInfoPath.GetAddr(), usrdirPath.GetAddr());
	
	if (!contentInfoPath.IsGood() || !usrdirPath.IsGood())
	{
		cellGame->Error("cellGameContentPermit(): CELL_GAME_ERROR_PARAM");
		return CELL_GAME_ERROR_PARAM;
	}

	if (contentInfo == "" && usrdir == "")
	{
		cellGame->Warning("cellGameContentPermit(): CELL_GAME_ERROR_FAILURE (no permission given)");
		return CELL_GAME_ERROR_FAILURE;
	}

	// TODO: make it better
	Memory.WriteString(contentInfoPath.GetAddr(), contentInfo);
	Memory.WriteString(usrdirPath.GetAddr(), usrdir);

	contentInfo = "";
	usrdir = "";
	
	return CELL_GAME_RET_OK;
}

int cellGameDataCheckCreate2(u32 version, u32 dirName_addr, u32 errDialog, u32 funcStat_addr, u32 container)
{
	cellGame->Error("cellGameDataCheckCreate2(version=0x%x, dirName_addr=0x%x, errDialog=0x%x, funcStat_addr=0x%x, container=%d)",
		version, dirName_addr, errDialog, funcStat_addr, container);
	return CELL_OK;
}

int cellGameDataCheckCreate(u32 version, u32 dirName_addr, u32 errDialog, u32 funcStat_addr, u32 container)
{
	// probably identical
	return cellGameDataCheckCreate2(version, dirName_addr, errDialog, funcStat_addr, container);
}

int cellGameCreateGameData(mem_ptr_t<CellGameSetInitParams> init, mem_list_ptr_t<u8> tmp_contentInfoPath, mem_list_ptr_t<u8> tmp_usrdirPath)
{
	cellGame->Error("cellGameCreateGameData(init_addr=0x%x, tmp_contentInfoPath_addr=0x%x, tmp_usrdirPath_addr=0x%x)",
		init.GetAddr(), tmp_contentInfoPath.GetAddr(), tmp_usrdirPath.GetAddr());

	// TODO: create temporary game directory, set initial PARAM.SFO parameters
	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	return CELL_OK;
}

int cellGameDeleteGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetParamInt(u32 id, mem32_t value)
{
	cellGame->Warning("cellGameGetParamInt(id=%d, value_addr=0x%x)", id, value.GetAddr());

	if(!value.IsGood())
		return CELL_GAME_ERROR_PARAM;

	// TODO: Access through cellGame***Check functions
	// TODO: Locate the PARAM.SFO. The following path may be wrong.
	vfsFile f("/app_home/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return CELL_GAME_ERROR_FAILURE;

	switch(id)
	{
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:  value = psf.GetInteger("PARENTAL_LEVEL"); break;
	case CELL_GAME_PARAMID_RESOLUTION:      value = psf.GetInteger("RESOLUTION");     break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:    value = psf.GetInteger("SOUND_FORMAT");   break;

	default:
		return CELL_GAME_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

int cellGameGetParamString(u32 id, u32 buf_addr, u32 bufsize)
{
	cellGame->Warning("cellGameGetParamString(id=%d, buf_addr=0x%x, bufsize=%d)", id, buf_addr, bufsize);

	if(!Memory.IsGoodAddr(buf_addr))
		return CELL_GAME_ERROR_PARAM;

	// TODO: Access through cellGame***Check functions
	// TODO: Locate the PARAM.SFO. The following path may be wrong.
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

	data.resize(bufsize-1);
	Memory.WriteString(buf_addr, data.c_str());
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

int cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, u32 dirName_addr)
{
	cellGame->Warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName_addr=0x%x)", type, errNeedSizeKB, dirName_addr);

	if (Memory.IsGoodAddr(dirName_addr))
		return CELL_GAME_ERROR_PARAM;

	char* dirName = (char*)Memory.VirtualToRealAddr(dirName_addr);
	std::string errorName;
	switch(type)
	{
		case CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA:      errorName = "Game data is corrupted (can be continued).";          break;
		case CELL_GAME_ERRDIALOG_BROKEN_HDDGAME:       errorName = "HDD boot game is corrupted (can be continued).";      break;
		case CELL_GAME_ERRDIALOG_NOSPACE:              errorName = "Not enough available space (can be continued).";      break;
		case CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA: errorName = "Game data is corrupted (terminate application).";     break;
		case CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME:  errorName = "HDD boot game is corrupted (terminate application)."; break;
		case CELL_GAME_ERRDIALOG_NOSPACE_EXIT:         errorName = "Not enough available space (terminate application)."; break;
		default: return CELL_GAME_ERROR_PARAM;
	}

	std::string errorMsg = fmt::Format("%s\nSpace needed: %d KB\nDirectory name: %s",
		errorName.c_str(), errNeedSizeKB, dirName);
	rMessageBox(errorMsg, rGetApp().GetAppName(), rICON_ERROR | rOK);
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

void cellGame_init()
{
	// (TODO: Disc Exchange functions missing)

	cellGame->AddFunc(0xf52639ea, cellGameBootCheck);
	cellGame->AddFunc(0xce4374f6, cellGamePatchCheck);
	cellGame->AddFunc(0xdb9819f3, cellGameDataCheck);
	cellGame->AddFunc(0x70acec67, cellGameContentPermit);

	cellGame->AddFunc(0x42a2e133, cellGameCreateGameData);
	cellGame->AddFunc(0xb367c6e3, cellGameDeleteGameData);

	cellGame->AddFunc(0xe7951dee, cellGameDataCheckCreate);
	cellGame->AddFunc(0xc9645c41, cellGameDataCheckCreate2);

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
