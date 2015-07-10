#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFont.h"
#include "cellFontFT.h"

extern Module cellFontFT;

CCellFontFTInternal* s_fontFtInternalInstance = nullptr;

s32 cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, vm::ptr<CellFontLibraryConfigFT> config, vm::pptr<CellFontLibrary> lib)
{
	cellFontFT.Warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config=*0x%x, lib=**0x%x)", revisionFlags, config, lib);

	//if (s_fontInternalInstance->m_bInitialized)
		//return CELL_FONT_ERROR_UNINITIALIZED;

	lib->set(Memory.Alloc(sizeof(CellFontLibrary), 1));

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
	s_fontFtInternalInstance = new CCellFontFTInternal();

	cellFontFT.on_stop = []()
	{
		delete s_fontFtInternalInstance;
	};

	REG_FUNC(cellFontFT, cellFontInitLibraryFreeTypeWithRevision);
	REG_FUNC(cellFontFT, cellFontFTGetRevisionFlags);
	REG_FUNC(cellFontFT, cellFontFTGetInitializedRevisionFlags);
});
