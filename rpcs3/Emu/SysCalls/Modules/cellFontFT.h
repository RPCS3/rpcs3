#pragma once

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