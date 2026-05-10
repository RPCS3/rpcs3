#pragma once

#include "cellFont.h"

#include "Emu/Memory/vm_ptr.h"

struct CellFontLibraryConfigFT
{
	vm::bptr<void> library;
	CellFontMemoryInterface MemoryIF;
};

using CellFontRendererConfigFT = CellFontRendererConfig;
