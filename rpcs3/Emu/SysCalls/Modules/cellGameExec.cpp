#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellGame.h"

extern Module cellGameExec;

s32 cellGameSetExitParam()
{
	throw EXCEPTION("");
}

s32 cellGameGetHomeDataExportPath(vm::ptr<char> exportPath)
{
	cellGameExec.Warning("cellGameGetHomeDataExportPath(exportPath=0x%x)", exportPath);

	// TODO: PlayStation home is defunct.

	return CELL_GAME_ERROR_NOAPP;
}

s32 cellGameGetHomePath()
{
	throw EXCEPTION("");
}

s32 cellGameGetHomeDataImportPath()
{
	throw EXCEPTION("");
}

s32 cellGameGetHomeLaunchOptionPath()
{
	throw EXCEPTION("");
}

s32 cellGameGetBootGameInfo(vm::ptr<u32> type, vm::ptr<char> dirName, vm::ptr<u32> execData)
{
	cellGameExec.Todo("cellGameGetBootGameInfo(type=*0x%x, dirName=*0x%x, execData=*0x%x)", type, dirName, execData);

	// TODO: Support more boot types
	*type = CELL_GAME_GAMETYPE_SYS;

	return CELL_OK;
}

Module cellGameExec("cellGameExec", []()
{
	REG_FUNC(cellGameExec, cellGameSetExitParam);
	REG_FUNC(cellGameExec, cellGameGetHomeDataExportPath);
	REG_FUNC(cellGameExec, cellGameGetHomePath);
	REG_FUNC(cellGameExec, cellGameGetHomeDataImportPath);
	REG_FUNC(cellGameExec, cellGameGetHomeLaunchOptionPath);
	REG_FUNC(cellGameExec, cellGameGetBootGameInfo);
});
