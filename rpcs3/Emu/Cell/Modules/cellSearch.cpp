#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSearch.h"

logs::channel cellSearch("cellSearch");

s32 cellSearchInitialize(CellSearchMode mode, u32 container, vm::ptr<CellSearchSystemCallback> func, vm::ptr<u32> userData)
{
	cellSearch.warning("cellSearchInitialize()");

	// TODO: Store the arguments somewhere so we can use them later.

	return CELL_OK;
}

s32 cellSearchFinalize()
{
	cellSearch.warning("cellSearchFinalize()");

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

s32 cellSearchGetContentInfoSharable()
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

DECLARE(ppu_module_manager::cellSearch)("cellSearchUtility", []()
{
	REG_FUNC(cellSearchUtility, cellSearchInitialize);
	REG_FUNC(cellSearchUtility, cellSearchFinalize);
	REG_FUNC(cellSearchUtility, cellSearchStartListSearch);
	REG_FUNC(cellSearchUtility, cellSearchStartContentSearchInList);
	REG_FUNC(cellSearchUtility, cellSearchStartContentSearch);
	REG_FUNC(cellSearchUtility, cellSearchStartSceneSearchInVideo);
	REG_FUNC(cellSearchUtility, cellSearchStartSceneSearch);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoByOffset);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoByContentId);
	REG_FUNC(cellSearchUtility, cellSearchGetOffsetByContentId);
	REG_FUNC(cellSearchUtility, cellSearchGetContentIdByOffset);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoGameComment);
	REG_FUNC(cellSearchUtility, cellSearchGetMusicSelectionContext);
	REG_FUNC(cellSearchUtility, cellSearchGetMusicSelectionContextOfSingleTrack);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoPath);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoPathMovieThumb);
	REG_FUNC(cellSearchUtility, cellSearchPrepareFile);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoDeveloperData);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoSharable);
	REG_FUNC(cellSearchUtility, cellSearchCancel);
	REG_FUNC(cellSearchUtility, cellSearchEnd);
});
