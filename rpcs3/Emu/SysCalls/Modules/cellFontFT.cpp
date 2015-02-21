#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFont.h"
#include "cellFontFT.h"

extern Module cellFontFT;

CCellFontFTInternal* s_fontFtInternalInstance = nullptr;

int cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, vm::ptr<CellFontLibraryConfigFT> config, u32 lib_addr_addr)
{
	cellFontFT.Warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config_addr=0x%x, lib_addr_addr=0x%x",
		revisionFlags, config.addr(), lib_addr_addr);

	//if (s_fontInternalInstance->m_bInitialized)
		//return CELL_FONT_ERROR_UNINITIALIZED;

	vm::write32(lib_addr_addr, (u32)Memory.Alloc(sizeof(CellFontLibrary), 1));

	return CELL_OK;
}

int cellFontFTGetRevisionFlags()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

int cellFontFTGetInitializedRevisionFlags()
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
