#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

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
