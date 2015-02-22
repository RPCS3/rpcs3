#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSearch;

// Error Codes
enum
{
	CELL_SEARCH_OK                          = 0,
	CELL_SEARCH_CANCELED                    = 1,
	CELL_SEARCH_ERROR_PARAM                 = 0x8002C801,
	CELL_SEARCH_ERROR_BUSY                  = 0x8002C802,
	CELL_SEARCH_ERROR_NO_MEMORY             = 0x8002C803,
	CELL_SEARCH_ERROR_UNKNOWN_MODE          = 0x8002C804,
	CELL_SEARCH_ERROR_ALREADY_INITIALIZED   = 0x8002C805,
	CELL_SEARCH_ERROR_NOT_INITIALIZED       = 0x8002C806,
	CELL_SEARCH_ERROR_FINALIZING            = 0x8002C807,
	CELL_SEARCH_ERROR_NOT_SUPPORTED_SEARCH  = 0x8002C808,
	CELL_SEARCH_ERROR_CONTENT_OBSOLETE      = 0x8002C809,
	CELL_SEARCH_ERROR_CONTENT_NOT_FOUND     = 0x8002C80A,
	CELL_SEARCH_ERROR_NOT_LIST              = 0x8002C80B,
	CELL_SEARCH_ERROR_OUT_OF_RANGE          = 0x8002C80C,
	CELL_SEARCH_ERROR_INVALID_SEARCHID      = 0x8002C80D,
	CELL_SEARCH_ERROR_ALREADY_GOT_RESULT    = 0x8002C80E,
	CELL_SEARCH_ERROR_NOT_SUPPORTED_CONTEXT = 0x8002C80F,
	CELL_SEARCH_ERROR_INVALID_CONTENTTYPE   = 0x8002C810,
	CELL_SEARCH_ERROR_DRM                   = 0x8002C811,
	CELL_SEARCH_ERROR_TAG                   = 0x8002C812,
	CELL_SEARCH_ERROR_GENERIC               = 0x8002C8FF,
};

s32 cellSearchInitialize()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchFinalize()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchStartListSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchStartContentSearchInList()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchStartContentSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchStartSceneSearchInVideo()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchStartSceneSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoByOffset()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoByContentId()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetOffsetByContentId()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentIdByOffset()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoGameComment()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetMusicSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetMusicSelectionContextOfSingleTrack()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoPath()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoPathMovieThumb()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchPrepareFile()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchGetContentInfoDeveloperData()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchCancel()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

s32 cellSearchEnd()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

Module cellSearch("cellSearch", []()
{
	REG_FUNC(cellSearch, cellSearchInitialize);
	REG_FUNC(cellSearch, cellSearchFinalize);
	REG_FUNC(cellSearch, cellSearchStartListSearch);
	REG_FUNC(cellSearch, cellSearchStartContentSearchInList);
	REG_FUNC(cellSearch, cellSearchStartContentSearch);
	REG_FUNC(cellSearch, cellSearchStartSceneSearchInVideo);
	REG_FUNC(cellSearch, cellSearchStartSceneSearch);
	REG_FUNC(cellSearch, cellSearchGetContentInfoByOffset);
	REG_FUNC(cellSearch, cellSearchGetContentInfoByContentId);
	REG_FUNC(cellSearch, cellSearchGetOffsetByContentId);
	REG_FUNC(cellSearch, cellSearchGetContentIdByOffset);
	REG_FUNC(cellSearch, cellSearchGetContentInfoGameComment);
	REG_FUNC(cellSearch, cellSearchGetMusicSelectionContext);
	REG_FUNC(cellSearch, cellSearchGetMusicSelectionContextOfSingleTrack);
	REG_FUNC(cellSearch, cellSearchGetContentInfoPath);
	REG_FUNC(cellSearch, cellSearchGetContentInfoPathMovieThumb);
	REG_FUNC(cellSearch, cellSearchPrepareFile);
	REG_FUNC(cellSearch, cellSearchGetContentInfoDeveloperData);
	REG_FUNC(cellSearch, cellSearchCancel);
	REG_FUNC(cellSearch, cellSearchEnd);
}
