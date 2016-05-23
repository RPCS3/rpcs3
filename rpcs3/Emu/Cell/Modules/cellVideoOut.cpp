#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellVideoOut.h"

extern logs::channel cellSysutil;

cfg::map_entry<u8> g_cfg_video_out_resolution(cfg::root.video, "Resolution", "1280x720",
{
	{ "1920x1080", CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ "1280x720", CELL_VIDEO_OUT_RESOLUTION_720 },
	{ "720x480", CELL_VIDEO_OUT_RESOLUTION_480 },
	{ "720x576", CELL_VIDEO_OUT_RESOLUTION_576 },
	{ "1600x1080", CELL_VIDEO_OUT_RESOLUTION_1600x1080 },
	{ "1440x1080", CELL_VIDEO_OUT_RESOLUTION_1440x1080 },
	{ "1280x1080", CELL_VIDEO_OUT_RESOLUTION_1280x1080 },
	{ "960x1080", CELL_VIDEO_OUT_RESOLUTION_960x1080 },
});

cfg::map_entry<u8> g_cfg_video_out_aspect_ratio(cfg::root.video, "Aspect ratio", "16x9",
{
	{ "4x3", CELL_VIDEO_OUT_ASPECT_4_3 },
	{ "16x9", CELL_VIDEO_OUT_ASPECT_16_9 },
});

const extern std::unordered_map<u8, std::pair<int, int>> g_video_out_resolution_map
{
	{ CELL_VIDEO_OUT_RESOLUTION_1080,      { 1920, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_720,       { 1280, 720 } },
	{ CELL_VIDEO_OUT_RESOLUTION_480,       { 720, 480 } },
	{ CELL_VIDEO_OUT_RESOLUTION_576,       { 720, 576 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1600x1080, { 1600, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1440x1080, { 1440, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1280x1080, { 1280, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_960x1080,  { 960, 1080 } },
};

ppu_error_code cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.trace("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
		state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
		state->displayMode.resolutionId = g_cfg_video_out_resolution.get(); // TODO
		state->displayMode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
		state->displayMode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
		state->displayMode.aspect = g_cfg_video_out_aspect_ratio.get(); // TODO
		state->displayMode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;
		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

ppu_error_code cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
{
	cellSysutil.trace("cellVideoOutGetResolution(resolutionId=0x%x, resolution=*0x%x)", resolutionId, resolution);

	if (!resolution)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	switch (resolutionId)
	{
	case 0x01: *resolution = { 0x780, 0x438 }; break;
	case 0x02: *resolution = { 0x500, 0x2d0 }; break;
	case 0x04: *resolution = { 0x2d0, 0x1e0 }; break;
	case 0x05: *resolution = { 0x2d0, 0x240 }; break;
	case 0x0a: *resolution = { 0x640, 0x438 }; break;
	case 0x0b: *resolution = { 0x5a0, 0x438 }; break;
	case 0x0c: *resolution = { 0x500, 0x438 }; break;
	case 0x0d: *resolution = { 0x3c0, 0x438 }; break;
	case 0x64: *resolution = { 0x550, 0x300 }; break;
	case 0x81: *resolution = { 0x500, 0x5be }; break;
	case 0x82: *resolution = { 0x780, 0x438 }; break;
	case 0x83: *resolution = { 0x780, 0x89d }; break;
	case 0x8b: *resolution = { 0x280, 0x5be }; break;
	case 0x8a: *resolution = { 0x320, 0x5be }; break;
	case 0x89: *resolution = { 0x3c0, 0x5be }; break;
	case 0x88: *resolution = { 0x400, 0x5be }; break;
	case 0x91: *resolution = { 0x500, 0x5be }; break;
	case 0x92: *resolution = { 0x780, 0x438 }; break;
	case 0x9b: *resolution = { 0x280, 0x5be }; break;
	case 0x9a: *resolution = { 0x320, 0x5be }; break;
	case 0x99: *resolution = { 0x3c0, 0x5be }; break;
	case 0x98: *resolution = { 0x400, 0x5be }; break;
	case 0xa1: *resolution = { 0x780, 0x438 }; break;
		
	default: return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

ppu_error_code cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.warning("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", videoOut, config, option, waitForEvent);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		// TODO
		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

ppu_error_code cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	if (option) *option = {};
	*config = {};

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config->resolutionId = g_cfg_video_out_resolution.get();
		config->format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		config->aspect = g_cfg_video_out_aspect_ratio.get();
		config->pitch = 4 * g_video_out_resolution_map.at(g_cfg_video_out_resolution.get()).first;

		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

ppu_error_code cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
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

ppu_error_code cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return NOT_AN_ERROR(1);
	case CELL_VIDEO_OUT_SECONDARY: return NOT_AN_ERROR(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

ppu_error_code cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return NOT_AN_ERROR(1);
	case CELL_VIDEO_OUT_SECONDARY: return NOT_AN_ERROR(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange)
{
	throw EXCEPTION("");
}

s32 cellVideoOutDebugSetMonitorType(u32 videoOut, u32 monitorType)
{
	throw EXCEPTION("");
}

s32 cellVideoOutRegisterCallback(u32 slot, vm::ptr<CellVideoOutCallback> function, vm::ptr<void> userData)
{
	throw EXCEPTION("");
}

s32 cellVideoOutUnregisterCallback(u32 slot)
{
	throw EXCEPTION("");
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution, MFF_PERFECT);
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
