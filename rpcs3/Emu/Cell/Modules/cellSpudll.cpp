#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellSpudll("cellSpudll", logs::level::notice);

s32 cellSpudllGetImageSize(vm::ptr<u32> psize, vm::cptr<void> so_elf, vm::cptr<struct CellSpudllHandleConfig> config)
{
	throw EXCEPTION("");
}

s32 cellSpudllHandleConfigSetDefaultValues(vm::ptr<struct CellSpudllHandleConfig> config)
{
	throw EXCEPTION("");
}

DECLARE(ppu_module_manager::cellSpudll)("cellSpudll", []()
{
	REG_FUNC(cellSpudll, cellSpudllGetImageSize);
	REG_FUNC(cellSpudll, cellSpudllHandleConfigSetDefaultValues);
});
