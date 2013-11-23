#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "Loader/PSF.h"

void cellGame_init();
Module cellGame(0x003e, cellGame_init);

// Return Codes
enum
{
	CELL_GAME_RET_OK					= 0,
	CELL_GAME_RET_CANCEL				= 1,
	CELL_GAME_RET_NONE					= 2,
	CELL_GAME_ERROR_NOTFOUND			= 0x8002cb04,
	CELL_GAME_ERROR_BROKEN				= 0x8002cb05,
	CELL_GAME_ERROR_INTERNAL			= 0x8002cb06,
	CELL_GAME_ERROR_PARAM				= 0x8002cb07,
	CELL_GAME_ERROR_NOAPP				= 0x8002cb08,
	CELL_GAME_ERROR_ACCESS_ERROR		= 0x8002cb09,
	CELL_GAME_ERROR_NOSPACE				= 0x8002cb20,
	CELL_GAME_ERROR_NOTSUPPORTED		= 0x8002cb21,
	CELL_GAME_ERROR_FAILURE				= 0x8002cb22,
	CELL_GAME_ERROR_BUSY				= 0x8002cb23,
	CELL_GAME_ERROR_IN_SHUTDOWN			= 0x8002cb24,
	CELL_GAME_ERROR_INVALID_ID			= 0x8002cb25,
	CELL_GAME_ERROR_EXIST				= 0x8002cb26,
	CELL_GAME_ERROR_NOTPATCH			= 0x8002cb27,
	CELL_GAME_ERROR_INVALID_THEME_FILE	= 0x8002cb28,
	CELL_GAME_ERROR_BOOTPATH			= 0x8002cb50,
};

//Parameter IDs of PARAM.SFO
enum
{
	//Integers
	CELL_GAME_PARAMID_PARENTAL_LEVEL	= 102,
	CELL_GAME_PARAMID_RESOLUTION		= 103,
	CELL_GAME_PARAMID_SOUND_FORMAT		= 104,

	//Strings
	CELL_GAME_PARAMID_TITLE				= 0,
	CELL_GAME_PARAMID_TITLE_DEFAULT		= 1,
	CELL_GAME_PARAMID_TITLE_JAPANESE	= 2,
	CELL_GAME_PARAMID_TITLE_ENGLISH		= 3,
	CELL_GAME_PARAMID_TITLE_FRENCH		= 4,
	CELL_GAME_PARAMID_TITLE_SPANISH		= 5,
	CELL_GAME_PARAMID_TITLE_GERMAN		= 6,
	CELL_GAME_PARAMID_TITLE_ITALIAN		= 7,
	CELL_GAME_PARAMID_TITLE_DUTCH		= 8,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE	= 9,
	CELL_GAME_PARAMID_TITLE_RUSSIAN		= 10,
	CELL_GAME_PARAMID_TITLE_KOREAN		= 11,
	CELL_GAME_PARAMID_TITLE_CHINESE_T	= 12,
	CELL_GAME_PARAMID_TITLE_CHINESE_S	= 13,
	CELL_GAME_PARAMID_TITLE_FINNISH		= 14,
	CELL_GAME_PARAMID_TITLE_SWEDISH		= 15,
	CELL_GAME_PARAMID_TITLE_DANISH		= 16,
	CELL_GAME_PARAMID_TITLE_NORWEGIAN	= 17,
	CELL_GAME_PARAMID_TITLE_POLISH		= 18,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL = 19,
	CELL_GAME_PARAMID_TITLE_ENGLISH_UK	= 20,
	CELL_GAME_PARAMID_TITLE_ID			= 100,
	CELL_GAME_PARAMID_VERSION			= 101,
	CELL_GAME_PARAMID_APP_VER			= 106,
};

int cellGameBootCheck()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGamePatchCheck()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameDataCheck()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameContentPermit()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameCreateGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameDeleteGameData()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetParamInt(u32 id, mem32_t value)
{
	cellGame.Warning("cellGameGetParamInt(id=%d, value_addr=0x%x)", id, value.GetAddr());

	if(!value.IsGood())
		return CELL_GAME_ERROR_PARAM;

	// TODO: Locate the PARAM.SFO. The following path is in most cases wrong.
	vfsStream* f = Emu.GetVFS().Open("/app_home/PARAM.SFO", vfsRead);
	PSFLoader psf(*f);
	if(!psf.Load(false))
		return CELL_GAME_ERROR_FAILURE;
	psf.Close();

	switch(id)
	{										// TODO: Is the endianness right?
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:	value = psf.m_info.parental_lvl;	break;
	case CELL_GAME_PARAMID_RESOLUTION:		value = psf.m_info.resolution;		break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:	value = psf.m_info.sound_format;	break;

	default:
		return CELL_GAME_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

int cellGameGetParamString(u32 id, mem_list_ptr_t<u8> buf, u32 bufsize)
{
	cellGame.Warning("cellGameGetParamString(id=%d, buf_addr=0x%x, bufsize=%d)", id, buf.GetAddr(), bufsize);

	if(!buf.IsGood())
		return CELL_GAME_ERROR_PARAM;

	// TODO: Locate the PARAM.SFO. The following path is in most cases wrong.
	vfsStream* f = Emu.GetVFS().Open("/app_home/PARAM.SFO", vfsRead);
	PSFLoader psf(*f);
	if(!psf.Load(false))
		return CELL_GAME_ERROR_FAILURE;
	psf.Close();

	switch(id)
	{
	// WARNING: Is there any difference between all these "CELL_GAME_PARAMID_TITLE*" IDs?
	case CELL_GAME_PARAMID_TITLE:
	case CELL_GAME_PARAMID_TITLE_DEFAULT:
	case CELL_GAME_PARAMID_TITLE_JAPANESE:
	case CELL_GAME_PARAMID_TITLE_ENGLISH:
	case CELL_GAME_PARAMID_TITLE_FRENCH:
	case CELL_GAME_PARAMID_TITLE_SPANISH:
	case CELL_GAME_PARAMID_TITLE_GERMAN:
	case CELL_GAME_PARAMID_TITLE_ITALIAN:
	case CELL_GAME_PARAMID_TITLE_DUTCH:
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:
	case CELL_GAME_PARAMID_TITLE_KOREAN:
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:
	case CELL_GAME_PARAMID_TITLE_FINNISH:
	case CELL_GAME_PARAMID_TITLE_SWEDISH:
	case CELL_GAME_PARAMID_TITLE_DANISH:
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:
	case CELL_GAME_PARAMID_TITLE_POLISH:
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:
		Memory.WriteString(buf.GetAddr(), psf.m_info.name.Left(bufsize));
		break;
	case CELL_GAME_PARAMID_TITLE_ID:
		Memory.WriteString(buf.GetAddr(), psf.m_info.serial.Left(bufsize));
		break;
	case CELL_GAME_PARAMID_VERSION:
		Memory.WriteString(buf.GetAddr(), psf.m_info.fw.Left(bufsize));
		break;
	case CELL_GAME_PARAMID_APP_VER:
		Memory.WriteString(buf.GetAddr(), psf.m_info.app_ver.Left(bufsize));
		break;

	default:
		return CELL_GAME_ERROR_INVALID_ID;
	}

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

int cellGameContentErrorDialog()
{
	UNIMPLEMENTED_FUNC(cellGame);
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

	cellGame.AddFunc(0xf52639ea, cellGameBootCheck);
	cellGame.AddFunc(0xce4374f6, cellGamePatchCheck);
	cellGame.AddFunc(0xdb9819f3, cellGameDataCheck);
	cellGame.AddFunc(0x70acec67, cellGameContentPermit);

	cellGame.AddFunc(0x42a2e133, cellGameCreateGameData);
	cellGame.AddFunc(0xb367c6e3, cellGameDeleteGameData);

	cellGame.AddFunc(0xb7a45caf, cellGameGetParamInt);
	//cellGame.AddFunc(, cellGameSetParamInt);
	cellGame.AddFunc(0x3a5d726a, cellGameGetParamString);
	cellGame.AddFunc(0xdaa5cd20, cellGameSetParamString);
	cellGame.AddFunc(0xef9d42d5, cellGameGetSizeKB);
	cellGame.AddFunc(0x2a8e6b92, cellGameGetDiscContentInfoUpdatePath);
	cellGame.AddFunc(0xa80bf223, cellGameGetLocalWebContentPath);

	cellGame.AddFunc(0xb0a1f8c6, cellGameContentErrorDialog);

	cellGame.AddFunc(0xd24e3928, cellGameThemeInstall);
	cellGame.AddFunc(0x87406734, cellGameThemeInstallFromBuffer);
	//cellGame.AddFunc(, CellGameThemeInstallCallback);
}