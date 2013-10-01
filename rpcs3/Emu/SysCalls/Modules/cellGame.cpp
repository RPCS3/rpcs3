#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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

int cellGameGetParamInt()
{
	UNIMPLEMENTED_FUNC(cellGame);
	return CELL_OK;
}

int cellGameGetParamString()
{
	UNIMPLEMENTED_FUNC(cellGame);
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