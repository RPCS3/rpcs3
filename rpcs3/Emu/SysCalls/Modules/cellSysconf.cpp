#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysconf;

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
	extern Module cellSysutil;

	REG_FUNC(cellSysutil, cellSysconfAbort);
	REG_FUNC(cellSysutil, cellSysconfOpen);
}

Module cellSysconf("cellSysconf", []()
{
	REG_FUNC(cellSysconf, cellSysconfBtGetDeviceList);
});
