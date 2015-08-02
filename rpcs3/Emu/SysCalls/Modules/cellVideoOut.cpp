#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Ini.h"
#include "Emu/RSX/GSManager.h"
#include "cellVideoOut.h"

extern Module cellSysutil;

s32 cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.Log("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = Emu.GetGSManager().GetState();
		state->colorSpace = Emu.GetGSManager().GetColorSpace();
		state->displayMode.resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
		state->displayMode.scanMode = Emu.GetGSManager().GetInfo().mode.scanMode;
		state->displayMode.conversion = Emu.GetGSManager().GetInfo().mode.conversion;
		state->displayMode.aspect = Emu.GetGSManager().GetInfo().mode.aspect;
		state->displayMode.refreshRates = Emu.GetGSManager().GetInfo().mode.refreshRates;
		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
{
	cellSysutil.Log("cellVideoOutGetResolution(resolutionId=%d, resolution=*0x%x)", resolutionId, resolution);

	u32 num = ResolutionIdToNum(resolutionId);
	if (!num)
		return CELL_EINVAL;

	resolution->width = ResolutionTable[num].width;
	resolution->height = ResolutionTable[num].height;

	return CELL_VIDEO_OUT_SUCCEEDED;
}

s32 cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.Warning("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=0x%x)", videoOut, config, option, waitForEvent);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		if (config->resolutionId)
		{
			Emu.GetGSManager().GetInfo().mode.resolutionId = config->resolutionId;
		}

		Emu.GetGSManager().GetInfo().mode.format = config->format;

		if (config->aspect)
		{
			Emu.GetGSManager().GetInfo().mode.aspect = config->aspect;
		}

		if (config->pitch)
		{
			Emu.GetGSManager().GetInfo().mode.pitch = config->pitch;
		}

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.Warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	if (option) *option = {};
	*config = {};

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config->resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
		config->format = Emu.GetGSManager().GetInfo().mode.format;
		config->aspect = Emu.GetGSManager().GetInfo().mode.aspect;
		config->pitch = Emu.GetGSManager().GetInfo().mode.pitch;

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.Warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = Emu.GetGSManager().GetColorSpace();
	info->latency = 1000;
	info->availableModeCount = 1;
	info->state = CELL_VIDEO_OUT_DEVICE_STATE_AVAILABLE;
	info->rgbOutputRange = 1;
	info->colorInfo.blueX = 0xFFFF;
	info->colorInfo.blueY = 0xFFFF;
	info->colorInfo.greenX = 0xFFFF;
	info->colorInfo.greenY = 0xFFFF;
	info->colorInfo.redX = 0xFFFF;
	info->colorInfo.redY = 0xFFFF;
	info->colorInfo.whiteX = 0xFFFF;
	info->colorInfo.whiteY = 0xFFFF;
	info->colorInfo.gamma = 100;
	info->availableModes[0].aspect = 0;
	info->availableModes[0].conversion = 0;
	info->availableModes[0].refreshRates = 0xF;
	info->availableModes[0].resolutionId = 1;
	info->availableModes[0].scanMode = 0;

	return CELL_OK;
}

s32 cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.Warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.Warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);

	if (!Ini.GS3DTV.GetValue() && (resolutionId == CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING || resolutionId == CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING ||
		resolutionId == CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING || resolutionId == CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING ||
		resolutionId == CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING))
		return 0;

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConvertCursorColorInfo()
{
	throw EXCEPTION("");
}

s32 cellVideoOutDebugSetMonitorType()
{
	throw EXCEPTION("");
}

s32 cellVideoOutRegisterCallback()
{
	throw EXCEPTION("");
}

s32 cellVideoOutUnregisterCallback()
{
	throw EXCEPTION("");
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution);
	REG_FUNC(cellSysutil, cellVideoOutConfigure);
	REG_FUNC(cellSysutil, cellVideoOutGetConfiguration);
	REG_FUNC(cellSysutil, cellVideoOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellVideoOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability);
	REG_FUNC(cellSysutil, cellVideoOutGetConvertCursorColorInfo);
	REG_FUNC(cellSysutil, cellVideoOutDebugSetMonitorType);
	REG_FUNC(cellSysutil, cellVideoOutRegisterCallback);
	REG_FUNC(cellSysutil, cellVideoOutUnregisterCallback);
}
