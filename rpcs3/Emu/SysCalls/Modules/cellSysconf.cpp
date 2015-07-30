#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysconf;

s32 cellSysconfBtGetDeviceList()
{
	throw EXCEPTION("");
}

Module cellSysconf("cellSysconf", []()
{
	REG_FUNC(cellSysconf, cellSysconfBtGetDeviceList);
});
