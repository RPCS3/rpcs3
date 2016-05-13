#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellVideoExport("cellVideoExport", logs::level::notice);

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


DECLARE(ppu_module_manager::cellVideoExport)("cellVideoExportUtility", []()
{
	REG_FUNC(cellVideoExportUtility, cellVideoExportProgress);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize2);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFileWithCopy);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFile);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFinalize);
});
