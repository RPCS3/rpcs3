#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSpudll.h"



LOG_CHANNEL(cellSpudll);

template<>
void fmt_class_string<CellSpudllError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SPUDLL_ERROR_INVAL);
			STR_CASE(CELL_SPUDLL_ERROR_STAT);
			STR_CASE(CELL_SPUDLL_ERROR_ALIGN);
			STR_CASE(CELL_SPUDLL_ERROR_NULL_POINTER);
			STR_CASE(CELL_SPUDLL_ERROR_SRCH);
			STR_CASE(CELL_SPUDLL_ERROR_UNDEF);
			STR_CASE(CELL_SPUDLL_ERROR_FATAL);
		}

		return unknown;
	});
}

error_code cellSpudllGetImageSize(vm::ptr<u32> psize, vm::cptr<void> so_elf, vm::cptr<CellSpudllHandleConfig> config)
{
	cellSpudll.todo("cellSpudllGetImageSize(psize=*0x%x, so_elf=*0x%x, config=*0x%x)", psize, so_elf, config);

	if (!psize || !so_elf)
	{
		return CELL_SPUDLL_ERROR_NULL_POINTER;
	}

	// todo

	return CELL_OK;
}

error_code cellSpudllHandleConfigSetDefaultValues(vm::ptr<CellSpudllHandleConfig> config)
{
	cellSpudll.trace("cellSpudllHandleConfigSetDefaultValues(config=*0x%x)", config);

	if (!config)
	{
		return CELL_SPUDLL_ERROR_NULL_POINTER;
	}

	config->mode = 0;
	config->dmaTag = 0;
	config->numMaxReferred = 16;
	config->numMaxDepend = 16;
	config->unresolvedSymbolValueForFunc = vm::null;
	config->unresolvedSymbolValueForObject = vm::null;
	config->unresolvedSymbolValueForOther = vm::null;

	std::memset(config->__reserved__, 0, sizeof(config->__reserved__));

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSpudll)("cellSpudll", []()
{
	REG_FUNC(cellSpudll, cellSpudllGetImageSize);
	REG_FUNC(cellSpudll, cellSpudllHandleConfigSetDefaultValues).flag(MFF_PERFECT);
});
