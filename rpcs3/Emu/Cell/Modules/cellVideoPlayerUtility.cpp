#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellVideoPlayerUtility);

s32 cellVideoPlayerInitialize()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerSetStartPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerGetVolume()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerFinalize()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerSetStopPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerClose()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerGetTransferPictureInfo()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerSetDownloadPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerStartThumbnail()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerEndThumbnail()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerOpen()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerSetVolume()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerGetOutputStereoPicture()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerGetPlaybackStatus()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerSetTransferComplete()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerGetOutputPicture()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

s32 cellVideoPlayerPlaybackControl()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVideoPlayerUtility)("cellVideoPlayerUtility", []()
{
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerInitialize);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerSetStartPosition);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerGetVolume);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerFinalize);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerSetStopPosition);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerClose);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerGetTransferPictureInfo);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerSetDownloadPosition);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerStartThumbnail);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerEndThumbnail);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerOpen);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerSetVolume);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerGetOutputStereoPicture);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerGetPlaybackStatus);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerSetTransferComplete);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerGetOutputPicture);
	REG_FUNC(cellVideoPlayerUtility, cellVideoPlayerPlaybackControl);
});
