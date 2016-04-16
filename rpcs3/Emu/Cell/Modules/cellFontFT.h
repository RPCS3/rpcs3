#pragma once

#include "cellFont.h"

namespace vm { using namespace ps3; }

struct CellFontLibraryConfigFT
{
	vm::bptr<void> library;
	CellFontMemoryInterface MemoryIF;
};

using CellFontRendererConfigFT = CellFontRendererConfig;
