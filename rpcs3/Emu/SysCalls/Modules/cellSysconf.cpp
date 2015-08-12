#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSysconf.h"

extern Module cellSysconf;

s32 cellSysconfOpen(u32 type, vm::ptr<CellSysconfCallback>, vm::ptr<u32> userdata, vm::ptr<u32> extparam, u32 id)
{
	throw EXCEPTION("");
}

s32 cellSysconfAbort()
{
	throw EXCEPTION("");
}

s32 cellSysconfBtGetDeviceList()
{
	throw EXCEPTION("");
}

void cellSysutil_Sysconf_init()
{
	extern Module cellSysutil;

	REG_FUNC(cellSysutil, cellSysconfAbort);
	REG_FUNC(cellSysutil, cellSysconfOpen);
}

Module cellSysconf("cellSysconf", []()
{
	REG_FUNC(cellSysconf, cellSysconfBtGetDeviceList);
});
