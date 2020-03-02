#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutil);

s32 cellSysutilAvcByeRequest()
{
	cellSysutil.todo("cellSysutilAvcByeRequest()");
	return CELL_OK;
}

s32 cellSysutilAvcCancelByeRequest()
{
	cellSysutil.todo("cellSysutilAvcCancelByeRequest()");
	return CELL_OK;
}

s32 cellSysutilAvcCancelJoinRequest()
{
	cellSysutil.todo("cellSysutilAvcCancelJoinRequest()");
	return CELL_OK;
}

s32 cellSysutilAvcEnumPlayers()
{
	cellSysutil.todo("cellSysutilAvcEnumPlayers()");
	return CELL_OK;
}

s32 cellSysutilAvcGetAttribute()
{
	cellSysutil.todo("cellSysutilAvcGetAttribute()");
	return CELL_OK;
}

s32 cellSysutilAvcGetLayoutMode()
{
	cellSysutil.todo("cellSysutilAvcGetLayoutMode()");
	return CELL_OK;
}

s32 cellSysutilAvcGetShowStatus()
{
	cellSysutil.todo("cellSysutilAvcGetShowStatus()");
	return CELL_OK;
}

s32 cellSysutilAvcGetSpeakerVolumeLevel()
{
	cellSysutil.todo("cellSysutilAvcGetSpeakerVolumeLevel()");
	return CELL_OK;
}

s32 cellSysutilAvcGetVideoMuting()
{
	cellSysutil.todo("cellSysutilAvcGetVideoMuting()");
	return CELL_OK;
}

s32 cellSysutilAvcGetVoiceMuting()
{
	cellSysutil.todo("cellSysutilAvcGetVoiceMuting()");
	return CELL_OK;
}

s32 cellSysutilAvcHidePanel()
{
	cellSysutil.todo("cellSysutilAvcHidePanel()");
	return CELL_OK;
}

s32 cellSysutilAvcJoinRequest()
{
	cellSysutil.todo("cellSysutilAvcJoinRequest()");
	return CELL_OK;
}

s32 cellSysutilAvcLoadAsync()
{
	cellSysutil.todo("cellSysutilAvcLoadAsync()");
	return CELL_OK;
}

s32 cellSysutilAvcSetAttribute()
{
	cellSysutil.todo("cellSysutilAvcSetAttribute()");
	return CELL_OK;
}

s32 cellSysutilAvcSetLayoutMode()
{
	cellSysutil.todo("cellSysutilAvcSetLayoutMode()");
	return CELL_OK;
}

s32 cellSysutilAvcSetSpeakerVolumeLevel()
{
	cellSysutil.todo("cellSysutilAvcSetSpeakerVolumeLevel()");
	return CELL_OK;
}

s32 cellSysutilAvcSetVideoMuting()
{
	cellSysutil.todo("cellSysutilAvcSetVideoMuting()");
	return CELL_OK;
}

s32 cellSysutilAvcSetVoiceMuting()
{
	cellSysutil.todo("cellSysutilAvcSetVoiceMuting()");
	return CELL_OK;
}

s32 cellSysutilAvcShowPanel()
{
	cellSysutil.todo("cellSysutilAvcShowPanel()");
	return CELL_OK;
}

s32 cellSysutilAvcUnloadAsync()
{
	cellSysutil.todo("cellSysutilAvcUnloadAsync()");
	return CELL_OK;
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
