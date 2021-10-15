#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_spu.h"



LOG_CHANNEL(cellOvis);

// Return Codes
enum CellOvisError : u32
{
	CELL_OVIS_ERROR_INVAL = 0x80410402,
	CELL_OVIS_ERROR_ABORT = 0x8041040C,
	CELL_OVIS_ERROR_ALIGN = 0x80410410,
};

template<>
void fmt_class_string<CellOvisError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_OVIS_ERROR_INVAL);
			STR_CASE(CELL_OVIS_ERROR_ABORT);
			STR_CASE(CELL_OVIS_ERROR_ALIGN);
		}

		return unknown;
	});
}

error_code cellOvisGetOverlayTableSize(vm::cptr<char> elf)
{
	cellOvis.todo("cellOvisGetOverlayTableSize(elf=%s)", elf);
	return CELL_OK;
}

error_code cellOvisInitializeOverlayTable(vm::ptr<void> ea_ovly_table, vm::cptr<char> elf)
{
	cellOvis.todo("cellOvisInitializeOverlayTable(ea_ovly_table=*0x%x, elf=%s)", ea_ovly_table, elf);
	return CELL_OK;
}

void cellOvisFixSpuSegments(vm::ptr<sys_spu_image> image)
{
	cellOvis.todo("cellOvisFixSpuSegments(image=*0x%x)", image);
}

void cellOvisInvalidateOverlappedSegments(vm::ptr<sys_spu_segment> segs, vm::ptr<int> nsegs)
{
	cellOvis.todo("cellOvisInvalidateOverlappedSegments(segs=*0x%x, nsegs=*0x%x)", segs, nsegs);
}

DECLARE(ppu_module_manager::cellOvis)("cellOvis", []()
{
	REG_FUNC(cellOvis, cellOvisGetOverlayTableSize);
	REG_FUNC(cellOvis, cellOvisInitializeOverlayTable);
	REG_FUNC(cellOvis, cellOvisFixSpuSegments);
	REG_FUNC(cellOvis, cellOvisInvalidateOverlappedSegments);
});
