#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellGame.h"

LOG_CHANNEL(cellGameExec);

error_code cellGameSetExitParam(u32 execdata)
{
	cellGameExec.todo("cellGameSetExitParam(execdata=0x%x)", execdata);
	return CELL_OK;
}

error_code cellGameGetHomeDataExportPath(vm::ptr<char> exportPath)
{
	cellGameExec.warning("cellGameGetHomeDataExportPath(exportPath=*0x%x)", exportPath);

	if (!exportPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: PlayStation home is defunct.

	return CELL_GAME_ERROR_NOAPP;
}

error_code cellGameGetHomePath()
{
	UNIMPLEMENTED_FUNC(cellGameExec);
	return CELL_OK;
}

error_code cellGameGetHomeDataImportPath(vm::ptr<char> importPath)
{
	cellGameExec.warning("cellGameGetHomeDataImportPath(importPath=*0x%x)", importPath);

	if (!importPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: PlayStation home is defunct.

	return CELL_GAME_ERROR_NOAPP;
}

error_code cellGameGetHomeLaunchOptionPath(vm::ptr<char> commonPath, vm::ptr<char> personalPath)
{
	cellGameExec.todo("cellGameGetHomeLaunchOptionPath(commonPath=%s, personalPath=%s)", commonPath, personalPath);

	if (!commonPath || !personalPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: PlayStation home is not supported atm.
	return CELL_GAME_ERROR_NOAPP;
}

error_code cellGameExecGame()
{
	UNIMPLEMENTED_FUNC(cellGameExec);
	return CELL_OK;
}

error_code cellGameGetBootGameInfo(vm::ptr<u32> type, vm::ptr<char> dirName, vm::ptr<u32> execdata)
{
	cellGameExec.todo("cellGameGetBootGameInfo(type=*0x%x, dirName=%s, execdata=*0x%x)", type, dirName, execdata);

	if (!type || !dirName) // execdata can be NULL
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// TODO: Support more boot types
	*type = CELL_GAME_GAMETYPE_SYS;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellGameExec)("cellGameExec", []()
{
	REG_FUNC(cellGameExec, cellGameSetExitParam);
	REG_FUNC(cellGameExec, cellGameGetHomeDataExportPath);
	REG_FUNC(cellGameExec, cellGameGetHomePath);
	REG_FUNC(cellGameExec, cellGameGetHomeDataImportPath);
	REG_FUNC(cellGameExec, cellGameGetHomeLaunchOptionPath);
	REG_FUNC(cellGameExec, cellGameExecGame);
	REG_FUNC(cellGameExec, cellGameGetBootGameInfo);
});
