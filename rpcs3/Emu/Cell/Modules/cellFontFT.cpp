#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellFontFT.h"

LOG_CHANNEL(cellFontFT);

s32 cellFontInitLibraryFreeType()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 cellFontInitLibraryFreeTypeWithRevision(u64 revisionFlags, vm::ptr<CellFontLibraryConfigFT> config, vm::pptr<CellFontLibrary> lib)
{
	cellFontFT.warning("cellFontInitLibraryFreeTypeWithRevision(revisionFlags=0x%llx, config=*0x%x, lib=**0x%x)", revisionFlags, config, lib);

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

s32 FTCacheStream_CacheEnd()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTCacheStream_CacheInit()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTCacheStream_CalcCacheIndexSize()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTCacheStream_Init()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_FontFamilyName()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_FontStyleName()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetAscender()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxHeight()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxMaxX()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxMaxY()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxMinX()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxMinY()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetBoundingBoxWidth()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetGlyphImage()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetGlyphMetrics()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetMaxHorizontalAdvance()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetMaxVerticalAdvance()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetRenderBufferSize()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetRenderScale()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_GetRenderScalePoint()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_SetCompositeCodes()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_SetRenderScalePixel()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTFaceH_SetRenderScalePoint()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_CloseFace()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_Done_FreeType()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_Init_FreeType()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_OpenFileFace()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_OpenMemFace()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

s32 FTManager_OpenStreamFace()
{
	UNIMPLEMENTED_FUNC(cellFontFT);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellFontFT)("cellFontFT", []()
{
	REG_FUNC(cellFontFT, cellFontInitLibraryFreeType);
	REG_FUNC(cellFontFT, cellFontInitLibraryFreeTypeWithRevision);
	REG_FUNC(cellFontFT, cellFontFTGetRevisionFlags);
	REG_FUNC(cellFontFT, cellFontFTGetInitializedRevisionFlags);

	REG_FUNC(cellFontFT, FTCacheStream_CacheEnd);
	REG_FUNC(cellFontFT, FTCacheStream_CacheInit);
	REG_FUNC(cellFontFT, FTCacheStream_CalcCacheIndexSize);
	REG_FUNC(cellFontFT, FTCacheStream_Init);

	REG_FUNC(cellFontFT, FTFaceH_FontFamilyName);
	REG_FUNC(cellFontFT, FTFaceH_FontStyleName);
	REG_FUNC(cellFontFT, FTFaceH_GetAscender);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxHeight);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxMaxX);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxMaxY);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxMinX);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxMinY);
	REG_FUNC(cellFontFT, FTFaceH_GetBoundingBoxWidth);
	REG_FUNC(cellFontFT, FTFaceH_GetGlyphImage);
	REG_FUNC(cellFontFT, FTFaceH_GetGlyphMetrics);
	REG_FUNC(cellFontFT, FTFaceH_GetMaxHorizontalAdvance);
	REG_FUNC(cellFontFT, FTFaceH_GetMaxVerticalAdvance);
	REG_FUNC(cellFontFT, FTFaceH_GetRenderBufferSize);
	REG_FUNC(cellFontFT, FTFaceH_GetRenderScale);
	REG_FUNC(cellFontFT, FTFaceH_GetRenderScalePoint);
	REG_FUNC(cellFontFT, FTFaceH_SetCompositeCodes);
	REG_FUNC(cellFontFT, FTFaceH_SetRenderScalePixel);
	REG_FUNC(cellFontFT, FTFaceH_SetRenderScalePoint);

	REG_FUNC(cellFontFT, FTManager_CloseFace);
	REG_FUNC(cellFontFT, FTManager_Done_FreeType);
	REG_FUNC(cellFontFT, FTManager_Init_FreeType);
	REG_FUNC(cellFontFT, FTManager_OpenFileFace);
	REG_FUNC(cellFontFT, FTManager_OpenMemFace);
	REG_FUNC(cellFontFT, FTManager_OpenStreamFace);
});
