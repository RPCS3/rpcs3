#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellSysconf("cellSysconf", logs::level::notice);

s32 cellSysconfAbort()
{
	throw EXCEPTION("");
}

s32 cellSysconfOpen()
{
	throw EXCEPTION("");
}

s32 cellSysconfBtGetDeviceList()
{
	throw EXCEPTION("");
}

void cellSysutil_Sysconf_init()
{
	REG_FUNC(cellSysutil, cellSysconfAbort);
	REG_FUNC(cellSysutil, cellSysconfOpen);
}

DECLARE(ppu_module_manager::cellSysconf)("cellSysconfExtUtility", []()
{
	REG_FUNC(cellSysconfExtUtility, cellSysconfBtGetDeviceList);
});
