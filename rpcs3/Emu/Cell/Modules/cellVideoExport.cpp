#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellVideoExport("cellVideoExport");

s32 cellVideoExportProgress()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoExportInitialize2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoExportInitialize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoExportFromFileWithCopy()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoExportFromFile()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoExportFinalize()
{
	fmt::throw_exception("Unimplemented" HERE);
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
