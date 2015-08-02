#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFontFT.h"

extern Module cellFontFT;

s32 cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, vm::ptr<CellFontLibraryConfigFT> config, vm::pptr<CellFontLibrary> lib)
{
	cellFontFT.Warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config=*0x%x, lib=**0x%x)", revisionFlags, config, lib);

	lib->set(vm::alloc(sizeof(CellFontLibrary), vm::main));

	return CELL_OK;
}

s32 cellFontFTGetRevisionFlags()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 cellFontFTGetInitializedRevisionFlags()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

Module cellFontFT("cellFontFT", []()
{
	REG_FUNC(cellFontFT, cellFontInitLibraryFreeTypeWithRevision);
	REG_FUNC(cellFontFT, cellFontFTGetRevisionFlags);
	REG_FUNC(cellFontFT, cellFontFTGetInitializedRevisionFlags);
});
