#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellVideoExport;

s32 cellVideoExportProgress()
{
	throw EXCEPTION("");
}

s32 cellVideoExportInitialize2()
{
	throw EXCEPTION("");
}

s32 cellVideoExportInitialize()
{
	throw EXCEPTION("");
}

s32 cellVideoExportFromFileWithCopy()
{
	throw EXCEPTION("");
}

s32 cellVideoExportFromFile()
{
	throw EXCEPTION("");
}

s32 cellVideoExportFinalize()
{
	throw EXCEPTION("");
}


Module cellVideoExport("cellVideoExport", []()
{
	REG_FUNC(cellVideoExport, cellVideoExportProgress);
	REG_FUNC(cellVideoExport, cellVideoExportInitialize2);
	REG_FUNC(cellVideoExport, cellVideoExportInitialize);
	REG_FUNC(cellVideoExport, cellVideoExportFromFileWithCopy);
	REG_FUNC(cellVideoExport, cellVideoExportFromFile);
	REG_FUNC(cellVideoExport, cellVideoExportFinalize);
});
