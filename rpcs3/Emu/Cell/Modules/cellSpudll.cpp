#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

namespace vm { using namespace ps3; }

logs::channel cellSpudll("cellSpudll", logs::level::notice);

s32 cellSpudllGetImageSize(vm::ptr<u32> psize, vm::cptr<void> so_elf, vm::cptr<struct CellSpudllHandleConfig> config)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSpudllHandleConfigSetDefaultValues(vm::ptr<struct CellSpudllHandleConfig> config)
{
	fmt::throw_exception("Unimplemented" HERE);
}

DECLARE(ppu_module_manager::cellSpudll)("cellSpudll", []()
{
	REG_FUNC(cellSpudll, cellSpudllGetImageSize);
	REG_FUNC(cellSpudll, cellSpudllHandleConfigSetDefaultValues);
});
