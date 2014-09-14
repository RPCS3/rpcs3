#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFont.h"
#include "cellFontFT.h"

Module *cellFontFT = nullptr;

CCellFontFTInternal* s_fontFtInternalInstance = nullptr;

int cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, vm::ptr<CellFontLibraryConfigFT> config, u32 lib_addr_addr)
{
	cellFontFT->Warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config_addr=0x%x, lib_addr_addr=0x%x",
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

void cellFontFT_init(Module *pxThis)
{
	cellFontFT = pxThis;

	cellFontFT->AddFunc(0x7a0a83c4, cellFontInitLibraryFreeTypeWithRevision);
	cellFontFT->AddFunc(0xec89a187, cellFontFTGetRevisionFlags);
	cellFontFT->AddFunc(0xfa0c2de0, cellFontFTGetInitializedRevisionFlags);
}

void cellFontFT_load()
{
	s_fontFtInternalInstance = new CCellFontFTInternal();
}

void cellFontFT_unload()
{
	delete s_fontFtInternalInstance;
}
