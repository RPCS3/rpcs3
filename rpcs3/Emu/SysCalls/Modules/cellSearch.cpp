#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "cellSearch.h"

extern Module cellSearch;

struct cellSearchInternal
{
	bool m_bInitialized;

	cellSearchInternal()
		: m_bInitialized(false)
	{
	}
};

cellSearchInternal cellSearchInstance;

s32 cellSearchInitialize(CellSearchMode mode, u32 container, CellSearchSystemCallback func, vm::ptr<u32> userData)
{
	cellSearch.Todo("cellSearchInitialize()");

	if (cellSearchInstance.m_bInitialized)
		return CELL_SEARCH_ERROR_ALREADY_INITIALIZED;
	if (mode != 0)
		return CELL_SEARCH_ERROR_UNKNOWN_MODE;
	if (!func)
		return CELL_SEARCH_ERROR_PARAM;

	cellSearchInstance.m_bInitialized = true;

	// TODO: Store the arguments somewhere so we can use them later.

	return CELL_OK;
}

s32 cellSearchFinalize()
{
	cellSearch.Log("cellSearchFinalize()");

	if (!cellSearchInstance.m_bInitialized)
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;

	cellSearchInstance.m_bInitialized = false;

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
});
