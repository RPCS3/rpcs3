#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysutil;

s32 cellStorageDataImportMove()
{
	throw EXCEPTION("");
}

s32 cellStorageDataImport()
{
	throw EXCEPTION("");
}

s32 cellStorageDataExport()
{
	throw EXCEPTION("");
}


void cellSysutil_Storage_init()
{
	REG_FUNC(cellSysutil, cellStorageDataImportMove);
	REG_FUNC(cellSysutil, cellStorageDataImport);
	REG_FUNC(cellSysutil, cellStorageDataExport);
}
