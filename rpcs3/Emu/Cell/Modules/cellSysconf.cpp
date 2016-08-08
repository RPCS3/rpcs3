#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellSysconf("cellSysconf", logs::level::notice);

s32 cellSysconfAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysconfOpen()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysconfBtGetDeviceList()
{
	fmt::throw_exception("Unimplemented" HERE);
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
