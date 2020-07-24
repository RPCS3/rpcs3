#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutilAvcExt);

error_code cellSysutilAvcExtIsMicAttached()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStopCameraDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowRotation()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowPosition()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetHideNamePlate()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowPosition()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowSize()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStartCameraDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowShowStatus()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetChatMode()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetNamePlateShowStatus()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowAlpha()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowSize()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtShowPanelEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtLoadAsyncEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetShowNamePlate()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStopVoiceDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtShowWindow()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtIsCameraAttached()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtHidePanelEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtHideWindow()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetChatGroup()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowRotation()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStartMicDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowAlpha()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStartVoiceDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetSurfacePointer()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtStopMicDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtInitOptionParam()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowZorder()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysutilAvcExt)("cellSysutilAvcExt", []()
{
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtIsMicAttached);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStopCameraDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetWindowRotation);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetWindowPosition);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetHideNamePlate);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetWindowPosition);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetWindowSize);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStartCameraDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetWindowShowStatus);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetChatMode);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetNamePlateShowStatus);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetWindowAlpha);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetWindowSize);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtShowPanelEx);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtLoadAsyncEx);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetShowNamePlate);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStopVoiceDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtShowWindow);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtIsCameraAttached);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtHidePanelEx);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtHideWindow);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetChatGroup);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetWindowRotation);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStartMicDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetWindowAlpha);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStartVoiceDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtGetSurfacePointer);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtStopMicDetection);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtInitOptionParam);
	REG_FUNC(cellSysutilAvcExt, cellSysutilAvcExtSetWindowZorder);
});
