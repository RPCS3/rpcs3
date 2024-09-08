#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellSysutilAvc.h"

LOG_CHANNEL(cellSysutilAvcExt);

error_code cellSysutilAvcSetAttribute(CellSysUtilAvcAttribute attr_id, vm::ptr<void> param);
error_code cellSysutilAvcLoadAsync(vm::ptr<CellSysutilAvcCallback> func, vm::ptr<void> userdata, sys_memory_container_t container, CellSysUtilAvcMediaType media, CellSysUtilAvcVideoQuality videoQuality, CellSysUtilAvcVoiceQuality voiceQuality, vm::ptr<CellSysutilAvcRequestId> request_id);

error_code cellSysutilAvcExtIsMicAttached(vm::ptr<s32> status)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtIsMicAttached(status=*0x%x)", status);

	ensure(!!status); // Not actually checked

	return CELL_OK;
}

error_code cellSysutilAvcExtStopCameraDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStopCameraDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowRotation(vm::ptr<SceNpId> player_id, f32 rotation_x, f32 rotation_y, f32 rotation_z, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetWindowRotation(player_id=*0x%x, rotation_x=%f, rotation_y=%f, rotation_z=%f, transition_type=0x%x)", player_id, rotation_x, rotation_y, rotation_z, +transition_type);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowPosition(vm::ptr<SceNpId> player_id, vm::ptr<f32> position_x, vm::ptr<f32> position_y, vm::ptr<f32> position_z)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetWindowPosition(player_id=*0x%x, position_x=*0x%x, position_y=*0x%x, position_z=*0x%x)", player_id, position_x, position_y, position_z);

	if (!player_id || !position_x || !position_y || !position_z)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetHideNamePlate()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetHideNamePlate()");
	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowPosition(vm::ptr<SceNpId> player_id, f32 position_x, f32 position_y, f32 position_z, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetWindowPosition(player_id=*0x%x, position_x=%f, position_y=%f, position_z=%f, transition_type=0x%x)", player_id, position_x, position_y, position_z, +transition_type);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowSize(vm::ptr<SceNpId> player_id, vm::ptr<f32> size_x, vm::ptr<f32> size_y)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetWindowSize(player_id=*0x%x, size_x=*0x%x, size_y=*0x%x)", player_id, size_x, size_y);

	if (!player_id || !size_x || !size_y)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtStartCameraDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStartCameraDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowShowStatus(vm::ptr<SceNpId> player_id, vm::ptr<b8> is_visible)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetWindowShowStatus(player_id=*0x%x, is_visible=*0x%x)", player_id, is_visible);

	if (!player_id || !is_visible)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetChatMode(u32 mode)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetChatMode(mode=0x%x)", mode);
	return CELL_OK;
}

error_code cellSysutilAvcExtGetNamePlateShowStatus(vm::ptr<b8> is_visible)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetNamePlateShowStatus(is_visible=*0x%x)", is_visible);

	if (!is_visible)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowAlpha(vm::ptr<SceNpId> player_id, f32 alpha, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetWindowAlpha(player_id=*0x%x, alpha=%f, transition_type=0x%x)", player_id, alpha, +transition_type);

	if (!player_id || transition_type > CELL_SYSUTIL_AVC_TRANSITION_EXPONENT)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowSize(vm::ptr<SceNpId> player_id, f32 size_x, f32 size_y, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetWindowSize(player_id=*0x%x, size_x=%f, size_y=%f, transition_type=0x%x)", player_id, size_x, size_y, +transition_type);

	if (!player_id || transition_type > CELL_SYSUTIL_AVC_TRANSITION_EXPONENT)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtShowPanelEx(CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtShowPanelEx(transition_type=0x%x)", +transition_type);
	return CELL_OK;
}

error_code cellSysutilAvcExtLoadAsyncEx(vm::ptr<CellSysutilAvcCallback> func, vm::ptr<void> userdata, sys_memory_container_t container,
	CellSysUtilAvcMediaType media, CellSysUtilAvcVideoQuality videoQuality, CellSysUtilAvcVoiceQuality voiceQuality, vm::ptr<CellSysutilAvcOptionParam> option, vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtLoadAsyncEx(func=*0x%x, userdata=*0x%x, container=0x%x, media=0x%x, videoQuality=0x%x, voiceQuality=0x%x, option=*0x%x, request_id=*0x%x)",
		func, userdata, container, +media, +videoQuality, +voiceQuality, option, request_id);

	if (!option)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	switch (option->avcOptionParamVersion)
	{
	case CELL_SYSUTIL_AVC_OPTION_PARAM_VERSION:
		if (option->sharingVideoBuffer && media == CELL_SYSUTIL_AVC_VOICE_CHAT)
			return CELL_AVC_ERROR_INVALID_ARGUMENT;

		//cellSysutilAvcSetAttribute(0x10000100, &option->sharingVideoBuffer);
		break;
	case 180:
		if (option->sharingVideoBuffer && media == CELL_SYSUTIL_AVC_VOICE_CHAT)
			return CELL_AVC_ERROR_INVALID_ARGUMENT;

		//cellSysutilAvcSetAttribute(0x10000100, &option->sharingVideoBuffer);
		//cellSysutilAvcSetAttribute(0x10000102, &option->maxPlayers);
		break;
	default:
		return CELL_AVC_ERROR_UNKNOWN;
	}

	return cellSysutilAvcLoadAsync(func, userdata, container, media, videoQuality, voiceQuality, request_id);
}

error_code cellSysutilAvcExtSetShowNamePlate()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetShowNamePlate()");
	return CELL_OK;
}

error_code cellSysutilAvcExtStopVoiceDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStopVoiceDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtShowWindow(vm::ptr<SceNpId> player_id, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStopVoiceDetection(player_id=*0x%x, transition_type=0x%x)", player_id, +transition_type);

	if (!player_id || transition_type > CELL_SYSUTIL_AVC_TRANSITION_EXPONENT)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtIsCameraAttached(vm::ptr<s32> status)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtIsCameraAttached(status=*0x%x)", status);

	ensure(!!status); // Not actually checked

	return CELL_OK;
}

error_code cellSysutilAvcExtHidePanelEx(CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtHidePanelEx(transition_type=0x%x)", +transition_type);
	return CELL_OK;
}

error_code cellSysutilAvcExtHideWindow(vm::ptr<SceNpId> player_id, CellSysutilAvcTransitionType transition_type)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtHideWindow(player_id=*0x%x, transition_type=0x%x)", player_id, +transition_type);

	if (!player_id || transition_type > CELL_SYSUTIL_AVC_TRANSITION_EXPONENT)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetChatGroup()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetChatGroup()");
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowRotation(vm::ptr<SceNpId> player_id, vm::ptr<f32> rotation_x, vm::ptr<f32> rotation_y, vm::ptr<f32> rotation_z)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetWindowRotation(player_id=*0x%x, rotation_x=*0x%x, rotation_y=*0x%x, rotation_z=*0x%x)", player_id, rotation_x, rotation_y, rotation_z);

	if (!player_id || !rotation_x || !rotation_y || !rotation_z)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtStartMicDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStartMicDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtGetWindowAlpha(vm::ptr<SceNpId> player_id, vm::ptr<f32> alpha)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetWindowAlpha(player_id=*0x%x, alpha=*0x%x)", player_id, alpha);

	if (!player_id || !alpha)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtStartVoiceDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStartVoiceDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtGetSurfacePointer(vm::ptr<SceNpId> player_id, vm::pptr<void> surface_ptr, vm::ptr<s32> surface_size, vm::ptr<s32> surface_size_x, vm::ptr<s32> surface_size_y)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtGetSurfacePointer(player_id=*0x%x, surface_ptr=*0x%x, surface_size=*0x%x, surface_size_x=*0x%x, surface_size_y=*0x%x)", player_id, surface_ptr, surface_size, surface_size_x, surface_size_y);

	if (!player_id || !surface_ptr || !surface_size || !surface_size_x || !surface_size_y)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcExtStopMicDetection()
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtStopMicDetection()");
	return CELL_OK;
}

error_code cellSysutilAvcExtInitOptionParam(s32 avcOptionParamVersion, vm::ptr<CellSysutilAvcOptionParam> option)
{
	cellSysutilAvcExt.notice("cellSysutilAvcExtInitOptionParam(avcOptionParamVersion=0x%x, option=*0x%x)", avcOptionParamVersion, option);

	if (!option)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	option->avcOptionParamVersion = avcOptionParamVersion;

	switch (option->avcOptionParamVersion)
	{
	case CELL_SYSUTIL_AVC_OPTION_PARAM_VERSION:
		break;
	case 180:
		option->maxPlayers = 16;
		break;
	default:
		return CELL_AVC_ERROR_UNKNOWN;
	}

	option->sharingVideoBuffer = false;

	return CELL_OK;
}

error_code cellSysutilAvcExtSetWindowZorder(vm::ptr<SceNpId> player_id,	u32 zorder)
{
	cellSysutilAvcExt.todo("cellSysutilAvcExtSetWindowZorder(player_id=*0x%x, zorder=0x%x)", player_id, zorder);

	if (!player_id || zorder < CELL_SYSUTIL_AVC_ZORDER_FORWARD_MOST || zorder > CELL_SYSUTIL_AVC_ZORDER_BEHIND_MOST)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

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
