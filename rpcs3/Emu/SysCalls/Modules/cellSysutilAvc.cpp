#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysutilAvc;

s32 cellSysutilAvcByeRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcCancelByeRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcCancelJoinRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcEnumPlayers()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetLayoutMode()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetShowStatus()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetSpeakerVolumeLevel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetVideoMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcGetVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcHidePanel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcJoinRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcLoadAsync()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcSetAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcSetLayoutMode()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcSetSpeakerVolumeLevel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcSetVideoMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcSetVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcShowPanel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcUnloadAsync()
{
	throw EXCEPTION("");
}


s32 cellSysutilAvcExtInitOptionParam()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetHideNamePlate()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetShowNamePlate()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtHideWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtShowWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetWindowAlpha()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetWindowSize()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetWindowRotation()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetWindowPosition()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetWindowAlpha()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetWindowSize()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetWindowRotation()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetWindowPosition()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtLoadAsyncEx()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtHidePanelEx()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtShowPanelEx()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetNamePlateShowStatus()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtStopVoiceDetection()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtStartVoiceDetection()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetSurfacePointer()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtSetWindowZorder()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvcExtGetWindowShowStatus()
{
	throw EXCEPTION("");
}


void cellSysutil_SysutilAvc_init()
{
	extern Module cellSysutil;

	REG_FUNC(cellSysutil, cellSysutilAvcByeRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcCancelByeRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcCancelJoinRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcEnumPlayers);
	REG_FUNC(cellSysutil, cellSysutilAvcGetAttribute);
	REG_FUNC(cellSysutil, cellSysutilAvcGetLayoutMode);
	REG_FUNC(cellSysutil, cellSysutilAvcGetShowStatus);
	REG_FUNC(cellSysutil, cellSysutilAvcGetSpeakerVolumeLevel);
	REG_FUNC(cellSysutil, cellSysutilAvcGetVideoMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcGetVoiceMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcHidePanel);
	REG_FUNC(cellSysutil, cellSysutilAvcJoinRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcLoadAsync);
	REG_FUNC(cellSysutil, cellSysutilAvcSetAttribute);
	REG_FUNC(cellSysutil, cellSysutilAvcSetLayoutMode);
	REG_FUNC(cellSysutil, cellSysutilAvcSetSpeakerVolumeLevel);
	REG_FUNC(cellSysutil, cellSysutilAvcSetVideoMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcSetVoiceMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcShowPanel);
	REG_FUNC(cellSysutil, cellSysutilAvcUnloadAsync);
}

Module cellSysutilAvc("cellSysutilAvc", []()
{
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtInitOptionParam);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetHideNamePlate);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetShowNamePlate);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtHideWindow);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtShowWindow);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetWindowAlpha);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetWindowSize);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetWindowRotation);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetWindowPosition);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetWindowAlpha);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetWindowSize);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetWindowRotation);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetWindowPosition);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtLoadAsyncEx);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtHidePanelEx);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtShowPanelEx);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetNamePlateShowStatus);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtStopVoiceDetection);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtStartVoiceDetection);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetSurfacePointer);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtSetWindowZorder);
	REG_FUNC(cellSysutilAvc, cellSysutilAvcExtGetWindowShowStatus);
});
