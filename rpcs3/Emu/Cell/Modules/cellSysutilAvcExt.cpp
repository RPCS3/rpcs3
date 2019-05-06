#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutilAvcExt);

s32 cellSysutilAvcExtIsMicAttached()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStopCameraDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetWindowRotation()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetWindowPosition()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetHideNamePlate()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetWindowPosition()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetWindowSize()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStartCameraDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetWindowShowStatus()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetChatMode()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetNamePlateShowStatus()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetWindowAlpha()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetWindowSize()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtShowPanelEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtLoadAsyncEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetShowNamePlate()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStopVoiceDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtShowWindow()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtIsCameraAttached()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtHidePanelEx()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtHideWindow()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetChatGroup()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetWindowRotation()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStartMicDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetWindowAlpha()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStartVoiceDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtGetSurfacePointer()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtStopMicDetection()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtInitOptionParam()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvcExt);
	return CELL_OK;
}

s32 cellSysutilAvcExtSetWindowZorder()
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
