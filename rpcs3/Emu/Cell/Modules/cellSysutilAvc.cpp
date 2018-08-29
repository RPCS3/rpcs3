#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutilAvc);

s32 cellSysutilAvcByeRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcCancelByeRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcCancelJoinRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcEnumPlayers()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetLayoutMode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetShowStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetSpeakerVolumeLevel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetVideoMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcGetVoiceMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcHidePanel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcJoinRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcLoadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcSetAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcSetLayoutMode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcSetSpeakerVolumeLevel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcSetVideoMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcSetVoiceMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcShowPanel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcUnloadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}


s32 cellSysutilAvcExtInitOptionParam()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetHideNamePlate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetShowNamePlate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtHideWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtShowWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetWindowAlpha()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetWindowSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetWindowRotation()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetWindowPosition()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetWindowAlpha()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetWindowSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetWindowRotation()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetWindowPosition()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtLoadAsyncEx()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtHidePanelEx()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtShowPanelEx()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetNamePlateShowStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtStopVoiceDetection()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtStartVoiceDetection()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetSurfacePointer()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtSetWindowZorder()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvcExtGetWindowShowStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}


void cellSysutil_SysutilAvc_init()
{
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

DECLARE(ppu_module_manager::cellSysutilAvc)("cellSysutilAvc", []()
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
