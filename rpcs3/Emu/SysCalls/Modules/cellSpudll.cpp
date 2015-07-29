#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSpudll;

s32 cellSpudllGetImageSize(vm::ptr<u32> psize, vm::cptr<void> so_elf, vm::cptr<struct CellSpudllHandleConfig> config)
{
	throw EXCEPTION("");
}

s32 cellSpudllHandleConfigSetDefaultValues(vm::ptr<struct CellSpudllHandleConfig> config)
{
	throw EXCEPTION("");
}

Module cellSpudll("cellSpudll", []()
{
	REG_FUNC(cellSpudll, cellSpudllGetImageSize);
	REG_FUNC(cellSpudll, cellSpudllHandleConfigSetDefaultValues);
});
