#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellGameExec;

s32 cellGameSetExitParam()
{
	throw EXCEPTION("");
}

s32 cellGameGetHomeDataExportPath()
{
	throw EXCEPTION("");
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

s32 cellGameGetBootGameInfo()
{
	throw EXCEPTION("");
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
