#pragma once

#include "cellFont.h"



struct CellFontLibraryConfigFT
{
	vm::bptr<void> library;
	CellFontMemoryInterface MemoryIF;
};

using CellFontRendererConfigFT = CellFontRendererConfig;
