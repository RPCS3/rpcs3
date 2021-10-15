#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellVideoPlayerUtility);

error_code cellVideoPlayerInitialize()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerSetStartPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerGetVolume()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerFinalize()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerSetStopPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerClose()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerGetTransferPictureInfo()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerSetDownloadPosition()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerStartThumbnail()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerEndThumbnail()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerOpen()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerSetVolume()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerGetOutputStereoPicture()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerGetPlaybackStatus()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerSetTransferComplete()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerGetOutputPicture()
{
	UNIMPLEMENTED_FUNC(cellVideoPlayerUtility);
	return CELL_OK;
}

error_code cellVideoPlayerPlaybackControl()
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
