#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

extern logs::channel cellSysutil;

s32 cellStorageDataImportMove()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellStorageDataImport()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellStorageDataExport()
{
	fmt::throw_exception("Unimplemented" HERE);
}


void cellSysutil_Storage_init()
{
	REG_FUNC(cellSysutil, cellStorageDataImportMove);
	REG_FUNC(cellSysutil, cellStorageDataImport);
	REG_FUNC(cellSysutil, cellStorageDataExport);
}
