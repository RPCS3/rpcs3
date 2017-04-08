#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSpudll.h"

logs::channel cellSpudll("cellSpudll", logs::level::notice);

s32 cellSpudllGetImageSize(vm::ptr<u32> psize, vm::cptr<void> so_elf, vm::cptr<CellSpudllHandleConfig> config)
{
	cellSpudll.todo("cellSpudllGetImageSize(psize=*0x%x, so_elf=*0x%x, config=*0x%x)", psize, so_elf, config);

	if (!psize || !so_elf)
	{
		return CELL_SPUDLL_ERROR_NULL_POINTER;
	}

	// todo

	return CELL_OK;
}

s32 cellSpudllHandleConfigSetDefaultValues(vm::ptr<CellSpudllHandleConfig> config)
{
	cellSpudll.trace("cellSpudllHandleConfigSetDefaultValues(config=*0x%x)", config);

	if (!config)
	{
		return CELL_SPUDLL_ERROR_NULL_POINTER;
	}

	config->mode = 0;
	config->dmaTag = 0;
	config->numMaxReferred = 16;
	config->unresolvedSymbolValueForFunc = vm::null;
	config->unresolvedSymbolValueForObject = vm::null;
	config->unresolvedSymbolValueForOther = vm::null;

	std::memset(config->__reserved__, 0, sizeof(config->__reserved__));

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSpudll)("cellSpudll", []()
{
	REG_FUNC(cellSpudll, cellSpudllGetImageSize);
	REG_FUNC(cellSpudll, cellSpudllHandleConfigSetDefaultValues).flags = MFF_PERFECT;
});
