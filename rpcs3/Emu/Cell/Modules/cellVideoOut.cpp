#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"

#include "cellVideoOut.h"

LOG_CHANNEL(cellSysutil);

const extern std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map
{
	{ video_resolution::_1080,      { 1920, 1080 } },
	{ video_resolution::_720,       { 1280, 720 } },
	{ video_resolution::_480,       { 720, 480 } },
	{ video_resolution::_576,       { 720, 576 } },
	{ video_resolution::_1600x1080, { 1600, 1080 } },
	{ video_resolution::_1440x1080, { 1440, 1080 } },
	{ video_resolution::_1280x1080, { 1280, 1080 } },
	{ video_resolution::_960x1080,  { 960, 1080 } },
};

const extern std::unordered_map<video_resolution, CellVideoOutResolutionId, value_hash<video_resolution>> g_video_out_resolution_id
{
	{ video_resolution::_1080,      CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ video_resolution::_720,       CELL_VIDEO_OUT_RESOLUTION_720 },
	{ video_resolution::_480,       CELL_VIDEO_OUT_RESOLUTION_480 },
	{ video_resolution::_576,       CELL_VIDEO_OUT_RESOLUTION_576 },
	{ video_resolution::_1600x1080, CELL_VIDEO_OUT_RESOLUTION_1600x1080 },
	{ video_resolution::_1440x1080, CELL_VIDEO_OUT_RESOLUTION_1440x1080 },
	{ video_resolution::_1280x1080, CELL_VIDEO_OUT_RESOLUTION_1280x1080 },
	{ video_resolution::_960x1080,  CELL_VIDEO_OUT_RESOLUTION_960x1080 },
};

const extern std::unordered_map<video_aspect, CellVideoOutDisplayAspect, value_hash<video_aspect>> g_video_out_aspect_id
{
	{ video_aspect::_16_9, CELL_VIDEO_OUT_ASPECT_16_9 },
	{ video_aspect::_4_3,  CELL_VIDEO_OUT_ASPECT_4_3 },
};

template<>
void fmt_class_string<CellVideoOutError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_VIDEO_OUT_ERROR_NOT_IMPLEMENTED);
		STR_CASE(CELL_VIDEO_OUT_ERROR_ILLEGAL_CONFIGURATION);
		STR_CASE(CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER);
		STR_CASE(CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE);
		STR_CASE(CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND);
		STR_CASE(CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT);
		STR_CASE(CELL_VIDEO_OUT_ERROR_UNSUPPORTED_DISPLAY_MODE);
		STR_CASE(CELL_VIDEO_OUT_ERROR_CONDITION_BUSY);
		STR_CASE(CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET);
		}

		return unknown;
	});
}

error_code cellVideoOutGetNumberOfDevice(u32 videoOut);

error_code cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.trace("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if (!state)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	const auto device_count = cellVideoOutGetNumberOfDevice(videoOut);

	if (device_count < 0 || deviceIndex >= static_cast<u32>(device_count))
	{
		return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;
	}

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
		state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
		state->displayMode.resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution); // TODO
		state->displayMode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
		state->displayMode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
		state->displayMode.aspect = g_video_out_aspect_id.at(g_cfg.video.aspect_ratio); // TODO
		state->displayMode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;
		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
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

error_code cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.todo("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", videoOut, config, option, waitForEvent);

	if (!config)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (!config->pitch)
	{
		return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	if (config->resolutionId == 0x92 || config->resolutionId == 0xa1)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_CONFIGURATION;
	}

	bool found = false;
	video_resolution res;

	for (const auto& ares : g_video_out_resolution_id)
	{
		if (ares.second == config->resolutionId)
		{
			found = true;
			res = ares.first;
			break;
		}
	}

	if (!found)
	{
		cellSysutil.error("Unusual resolution requested: 0x%x", config->resolutionId);
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_DISPLAY_MODE;
	}

	auto& res_info = g_video_out_resolution_map.at(res);

	auto conf = g_fxo->get<rsx::avconf>();
	conf->aspect = config->aspect;
	conf->format = config->format;
	conf->scanline_pitch = config->pitch;
	conf->resolution_x = u32(res_info.first);
	conf->resolution_y = u32(res_info.second);
	conf->state = 1;

	return CELL_OK;
}

error_code cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	if (!config)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (option) *option = {};
	*config = {};

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config->resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution);
		config->format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		config->aspect = g_video_out_aspect_id.at(g_cfg.video.aspect_ratio);
		config->pitch = 4 * g_video_out_resolution_map.at(g_cfg.video.resolution).first;

		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	if (!info)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	const auto device_count = cellVideoOutGetNumberOfDevice(videoOut);

	if (device_count < 0 || deviceIndex >= static_cast<u32>(device_count))
	{
		return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;
	}

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
	info->latency = 100;
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
	info->availableModes[0].aspect = g_video_out_aspect_id.at(g_cfg.video.aspect_ratio);
	info->availableModes[0].conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
	info->availableModes[0].refreshRates =  CELL_VIDEO_OUT_REFRESH_RATE_60HZ | CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;
	info->availableModes[0].resolutionId = g_video_out_resolution_id.at(g_cfg.video.resolution);
	info->availableModes[0].scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
	return CELL_OK;
}

error_code cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return not_an_error(1);
	case CELL_VIDEO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return not_an_error(
		resolutionId == g_video_out_resolution_id.at(g_cfg.video.resolution)
		&& (aspect == CELL_VIDEO_OUT_ASPECT_AUTO || aspect == g_video_out_aspect_id.at(g_cfg.video.aspect_ratio))
	);
	case CELL_VIDEO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutConfigure2()
{
	cellSysutil.todo("cellVideoOutConfigure2()");
	return CELL_OK;
}

error_code cellVideoOutGetResolutionAvailability2()
{
	cellSysutil.todo("cellVideoOutGetResolutionAvailability2()");
	return CELL_OK;
}

error_code cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange)
{
	cellSysutil.todo("cellVideoOutGetConvertCursorColorInfo()");
	return CELL_OK;
}

error_code cellVideoOutDebugSetMonitorType(u32 videoOut, u32 monitorType)
{
	cellSysutil.todo("cellVideoOutDebugSetMonitorType()");
	return CELL_OK;
}

error_code cellVideoOutRegisterCallback(u32 slot, vm::ptr<CellVideoOutCallback> function, vm::ptr<void> userData)
{
	cellSysutil.todo("cellVideoOutRegisterCallback()");
	return CELL_OK;
}

error_code cellVideoOutUnregisterCallback(u32 slot)
{
	cellSysutil.todo("cellVideoOutUnregisterCallback()");
	return CELL_OK;
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution).flag(MFF_PERFECT);
	REG_FUNC(cellSysutil, cellVideoOutConfigure);
	REG_FUNC(cellSysutil, cellVideoOutConfigure2);
	REG_FUNC(cellSysutil, cellVideoOutGetConfiguration);
	REG_FUNC(cellSysutil, cellVideoOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellVideoOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability2);
	REG_FUNC(cellSysutil, cellVideoOutGetConvertCursorColorInfo);
	REG_FUNC(cellSysutil, cellVideoOutDebugSetMonitorType);
	REG_FUNC(cellSysutil, cellVideoOutRegisterCallback);
	REG_FUNC(cellSysutil, cellVideoOutUnregisterCallback);
}
