#pragma once

#include "Emu/Memory/vm_ptr.h"

enum CellSpudllError : u32
{
	CELL_SPUDLL_ERROR_INVAL        = 0x80410602,
	CELL_SPUDLL_ERROR_STAT         = 0x8041060f,
	CELL_SPUDLL_ERROR_ALIGN        = 0x80410610,
	CELL_SPUDLL_ERROR_NULL_POINTER = 0x80410611,
	CELL_SPUDLL_ERROR_SRCH         = 0x80410605,
	CELL_SPUDLL_ERROR_UNDEF        = 0x80410612,
	CELL_SPUDLL_ERROR_FATAL        = 0x80410613,
};

struct CellSpudllHandleConfig
{
	be_t<u32> mode;
	be_t<u32> dmaTag;
	be_t<u32> numMaxReferred;
	be_t<u32> numMaxDepend;
	vm::bptr<void> unresolvedSymbolValueForFunc;
	vm::bptr<void> unresolvedSymbolValueForObject;
	vm::bptr<void> unresolvedSymbolValueForOther;
	be_t<u32> __reserved__[9];
};
