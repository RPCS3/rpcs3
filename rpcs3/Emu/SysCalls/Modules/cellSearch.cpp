#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSearch_init();
Module cellSearch(0xf02f, cellSearch_init);

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

int cellSearchInitialize()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchFinalize()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchStartListSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchStartContentSearchInList()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchStartContentSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchStartSceneSearchInVideo()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchStartSceneSearch()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoByOffset()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoByContentId()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetOffsetByContentId()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentIdByOffset()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoGameComment()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetMusicSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetMusicSelectionContextOfSingleTrack()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoPath()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoPathMovieThumb()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchPrepareFile()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchGetContentInfoDeveloperData()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchCancel()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

int cellSearchEnd()
{
	UNIMPLEMENTED_FUNC(cellSearch);
	return CELL_OK;
}

void cellSearch_init()
{
	cellSearch.AddFunc(0xc81ccf8a, cellSearchInitialize);
	cellSearch.AddFunc(0xbfab7616, cellSearchFinalize);
	cellSearch.AddFunc(0x0a4c8295, cellSearchStartListSearch);
	cellSearch.AddFunc(0x64fb0b76, cellSearchStartContentSearchInList);
	cellSearch.AddFunc(0x0591826f, cellSearchStartContentSearch);
	cellSearch.AddFunc(0xc0ed0522, cellSearchStartSceneSearchInVideo);
	cellSearch.AddFunc(0x13524faa, cellSearchStartSceneSearch);
	cellSearch.AddFunc(0x3b210319, cellSearchGetContentInfoByOffset);
	cellSearch.AddFunc(0x9663a44b, cellSearchGetContentInfoByContentId);
	cellSearch.AddFunc(0x540d9068, cellSearchGetOffsetByContentId);
	cellSearch.AddFunc(0x94e21701, cellSearchGetContentIdByOffset);
	cellSearch.AddFunc(0xd7a7a433, cellSearchGetContentInfoGameComment);
	cellSearch.AddFunc(0x025ce169, cellSearchGetMusicSelectionContext);
	cellSearch.AddFunc(0xed20e079, cellSearchGetMusicSelectionContextOfSingleTrack);
	cellSearch.AddFunc(0xffb28491, cellSearchGetContentInfoPath);
	cellSearch.AddFunc(0x37b5ba0c, cellSearchGetContentInfoPathMovieThumb);
	cellSearch.AddFunc(0xe73cb0d2, cellSearchPrepareFile);
	cellSearch.AddFunc(0x35cda406, cellSearchGetContentInfoDeveloperData);
	cellSearch.AddFunc(0x8fe376a6, cellSearchCancel);
	cellSearch.AddFunc(0x774033d6, cellSearchEnd);
}