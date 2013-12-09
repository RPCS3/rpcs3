#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellFont.h"

void cellFontFT_init();
Module cellFontFT(0x001a, cellFontFT_init);

struct CellFontLibraryConfigFT
{
	u32 library_addr; //void*
	CellFontMemoryInterface MemoryIF;
};

struct CellFontRendererConfigFT
{
	struct {
		u32 buffer_addr; //void*
		u32 initSize;
		u32 maxSize;
		u32 expandSize;
		u32 resetSize;
	} BufferingPolicy;
};

struct CCellFontFTInternal
{
	bool m_bInitialized;

	CCellFontFTInternal()
		: m_bInitialized(false)
	{
	}
};

CCellFontFTInternal* s_fontFtInternalInstance = new CCellFontFTInternal();

int cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, mem_ptr_t<CellFontLibraryConfigFT> config, u32 lib_addr_addr)
{
	cellFontFT.Warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config_addr=0x%x, lib_addr_addr=0x%x",
		revisionFlags, config.GetAddr(), lib_addr_addr);

	if (!config.IsGood() || !Memory.IsGoodAddr(lib_addr_addr))
		return CELL_FONT_ERROR_INVALID_PARAMETER;
	//if (s_fontInternalInstance->m_bInitialized)
		//return CELL_FONT_ERROR_UNINITIALIZED;

	Memory.Write32(lib_addr_addr, Memory.Alloc(sizeof(CellFontLibrary), 1));

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

void cellFontFT_init()
{
	cellFontFT.AddFunc(0x7a0a83c4, cellFontInitLibraryFreeTypeWithRevision);
	cellFontFT.AddFunc(0xec89a187, cellFontFTGetRevisionFlags);
	cellFontFT.AddFunc(0xfa0c2de0, cellFontFTGetInitializedRevisionFlags);
}