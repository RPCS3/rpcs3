#include "stdafx.h"
#include "Emu/system_config_types.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"
#include "Emu/RSX/RSXThread.h"

#include "cellVideoOut.h"

LOG_CHANNEL(cellSysutil);

// NOTE: Unused in this module, but used by gs_frame to determine window size
const extern std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map
{
	{ video_resolution::_1080p,      { 1920, 1080 } },
	{ video_resolution::_1080i,      { 1920, 1080 } },
	{ video_resolution::_720p,       { 1280, 720 } },
	{ video_resolution::_480p,       { 720, 480 } },
	{ video_resolution::_480i,       { 720, 480 } },
	{ video_resolution::_576p,       { 720, 576 } },
	{ video_resolution::_576i,       { 720, 576 } },
	{ video_resolution::_1600x1080p, { 1600, 1080 } },
	{ video_resolution::_1440x1080p, { 1440, 1080 } },
	{ video_resolution::_1280x1080p, { 1280, 1080 } },
	{ video_resolution::_960x1080p,  { 960, 1080 } },
};

const extern std::unordered_map<video_resolution, CellVideoOutResolutionId, value_hash<video_resolution>> g_video_out_resolution_id
{
	{ video_resolution::_1080p,      CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ video_resolution::_1080i,      CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ video_resolution::_720p,       CELL_VIDEO_OUT_RESOLUTION_720 },
	{ video_resolution::_480p,       CELL_VIDEO_OUT_RESOLUTION_480 },
	{ video_resolution::_480i,       CELL_VIDEO_OUT_RESOLUTION_480 },
	{ video_resolution::_576p,       CELL_VIDEO_OUT_RESOLUTION_576 },
	{ video_resolution::_576i,       CELL_VIDEO_OUT_RESOLUTION_576 },
	{ video_resolution::_1600x1080p, CELL_VIDEO_OUT_RESOLUTION_1600x1080 },
	{ video_resolution::_1440x1080p, CELL_VIDEO_OUT_RESOLUTION_1440x1080 },
	{ video_resolution::_1280x1080p, CELL_VIDEO_OUT_RESOLUTION_1280x1080 },
	{ video_resolution::_960x1080p,  CELL_VIDEO_OUT_RESOLUTION_960x1080 },
};

const extern std::unordered_map<video_resolution, CellVideoOutScanMode, value_hash<video_resolution>> g_video_out_scan_mode
{
	{ video_resolution::_1080p,      CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_1080i,      CELL_VIDEO_OUT_SCAN_MODE_INTERLACE },
	{ video_resolution::_720p,       CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_480p,       CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_480i,       CELL_VIDEO_OUT_SCAN_MODE_INTERLACE },
	{ video_resolution::_576p,       CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_576i,       CELL_VIDEO_OUT_SCAN_MODE_INTERLACE },
	{ video_resolution::_1600x1080p, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_1440x1080p, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_1280x1080p, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
	{ video_resolution::_960x1080p,  CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE },
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

error_code _IntGetResolutionInfo(u8 resolution_id, CellVideoOutResolution* resolution)
{
	// NOTE: Some resolution IDs that return values on hw have unknown resolution enumerants
	switch (resolution_id)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080:       *resolution = { 0x780, 0x438 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_720:        *resolution = { 0x500, 0x2d0 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_480:        *resolution = { 0x2d0, 0x1e0 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_576:        *resolution = { 0x2d0, 0x240 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_1600x1080:  *resolution = { 0x640, 0x438 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_1440x1080:  *resolution = { 0x5a0, 0x438 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_1280x1080:  *resolution = { 0x500, 0x438 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_960x1080:   *resolution = { 0x3c0, 0x438 }; break;
	case 0x64:                                 *resolution = { 0x550, 0x300 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING:       *resolution = { 0x500, 0x5be }; break;
	case 0x82:                                                 *resolution = { 0x780, 0x438 }; break;
	case 0x83:                                                 *resolution = { 0x780, 0x89d }; break;
	case CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING:   *resolution = { 0x280, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING:   *resolution = { 0x320, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING:   *resolution = { 0x3c0, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING:  *resolution = { 0x400, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_720_DUALVIEW_FRAME_PACKING: *resolution = { 0x500, 0x5be }; break;
	case 0x92:                                                 *resolution = { 0x780, 0x438 }; break;
	case CELL_VIDEO_OUT_RESOLUTION_640x720_DUALVIEW_FRAME_PACKING:  *resolution = { 0x280, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_800x720_DUALVIEW_FRAME_PACKING:  *resolution = { 0x320, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_960x720_DUALVIEW_FRAME_PACKING:  *resolution = { 0x3c0, 0x5be }; break;
	case CELL_VIDEO_OUT_RESOLUTION_1024x720_DUALVIEW_FRAME_PACKING: *resolution = { 0x400, 0x5be }; break;
	case 0xa1:                                                      *resolution = { 0x780, 0x438 }; break;

	default: return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

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
	{
		const auto& conf = g_fxo->get<rsx::avconf>();
		state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
		state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
		state->displayMode.resolutionId = conf.state ? conf.resolution_id : ::at32(g_video_out_resolution_id, g_cfg.video.resolution);
		state->displayMode.scanMode = conf.state ? conf.scan_mode : ::at32(g_video_out_scan_mode, g_cfg.video.resolution);
		state->displayMode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
		state->displayMode.aspect = conf.state ? conf.aspect : ::at32(g_video_out_aspect_id, g_cfg.video.aspect_ratio);
		state->displayMode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;

		return CELL_OK;
	}

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

	CellVideoOutResolution res;
	error_code result;

	if (result = _IntGetResolutionInfo(resolutionId, &res); result == CELL_OK)
	{
		*resolution = res;
	}

	return result;
}

error_code cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.warning("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", videoOut, config, option, waitForEvent);

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

	CellVideoOutResolution res;
	if (_IntGetResolutionInfo(config->resolutionId, &res) != CELL_OK ||
		(config->resolutionId >= CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING && g_cfg.video.stereo_render_mode == stereo_render_mode_options::disabled))
	{
		// Resolution not supported
		cellSysutil.error("Unusual resolution requested: 0x%x", config->resolutionId);
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_DISPLAY_MODE;
	}

	auto& conf = g_fxo->get<rsx::avconf>();
	conf.resolution_id = config->resolutionId;
	conf.stereo_mode = (config->resolutionId >= CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING) ? g_cfg.video.stereo_render_mode.get() : stereo_render_mode_options::disabled;
	conf.aspect = config->aspect;
	conf.format = config->format;
	conf.scanline_pitch = config->pitch;
	conf.resolution_x = res.width;
	conf.resolution_y = res.height;
	conf.state = 1;

	// TODO: What happens if the aspect is unknown? Let's treat it as auto for now.
	if (conf.aspect != CELL_VIDEO_OUT_ASPECT_4_3 && conf.aspect != CELL_VIDEO_OUT_ASPECT_16_9)
	{
		if (conf.aspect != CELL_VIDEO_OUT_ASPECT_AUTO)
		{
			cellSysutil.error("Selected unknown aspect 0x%x. Falling back to aspect %s.", conf.aspect, g_cfg.video.aspect_ratio.get());
		}

		// Resolve 'auto' or unknown options to actual aspect ratio
		conf.aspect = ::at32(g_video_out_aspect_id, g_cfg.video.aspect_ratio);
	}

	cellSysutil.notice("Selected video configuration: resolutionId=0x%x, aspect=0x%x=>0x%x, format=0x%x", config->resolutionId, config->aspect, conf.aspect, config->format);

	// This function resets VSYNC to be enabled
	rsx::get_current_renderer()->requested_vsync = true;
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
	{
		if (const auto& conf = g_fxo->get<rsx::avconf>(); conf.state)
		{
			config->resolutionId = conf.resolution_id;
			config->format = conf.format;
			config->aspect = conf.aspect;
			config->pitch = conf.scanline_pitch;
		}
		else
		{
			config->resolutionId = ::at32(g_video_out_resolution_id, g_cfg.video.resolution);
			config->format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
			config->aspect = ::at32(g_video_out_aspect_id, g_cfg.video.aspect_ratio);

			CellVideoOutResolution res;
			ensure(_IntGetResolutionInfo(config->resolutionId, &res) == CELL_OK); // "Invalid video configuration"

			config->pitch = 4 * res.width;
		}

		return CELL_OK;
	}

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

	u32 mode_count = 0;

	const auto add_mode = [&](u8 resolutionId, u8 scanMode, u8 conversion, u8 aspect)
	{
		info->availableModes[mode_count].resolutionId = resolutionId;
		info->availableModes[mode_count].scanMode = scanMode;
		info->availableModes[mode_count].conversion = conversion;
		info->availableModes[mode_count].aspect = aspect;
		info->availableModes[mode_count].refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_60HZ | CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;
		mode_count++;
	};

	switch (g_cfg.video.resolution.get())
	{
	case video_resolution::_1080p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1600x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1440x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1280x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_960x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_1080i:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1080, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1600x1080, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1440x1080, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1280x1080, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_960x1080, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_720p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_720, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_480p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		if (g_cfg.video.aspect_ratio == video_aspect::_16_9)
		{
			add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		}
		break;
	case video_resolution::_480i:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		if (g_cfg.video.aspect_ratio == video_aspect::_16_9)
		{
			add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		}
		break;
	case video_resolution::_576p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_576, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		if (g_cfg.video.aspect_ratio == video_aspect::_16_9)
		{
			add_mode(CELL_VIDEO_OUT_RESOLUTION_576, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		}
		break;
	case video_resolution::_576i:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_576, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_4_3);
		if (g_cfg.video.aspect_ratio == video_aspect::_16_9)
		{
			add_mode(CELL_VIDEO_OUT_RESOLUTION_576, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_480, CELL_VIDEO_OUT_SCAN_MODE_INTERLACE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
		}
		break;
	case video_resolution::_1600x1080p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1600x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_1440x1080p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1440x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_1280x1080p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_1280x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	case video_resolution::_960x1080p:
		add_mode(CELL_VIDEO_OUT_RESOLUTION_960x1080, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080, CELL_VIDEO_OUT_ASPECT_16_9);
		break;
	}

	if (g_cfg.video.stereo_render_mode != stereo_render_mode_options::disabled && g_cfg.video.resolution == video_resolution::_720p)
	{
		// Register 3D-capable display mode
		if (true) // TODO
		{
			// 3D stereo
			add_mode(CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
		}
		else
		{
			// SimulView
			add_mode(CELL_VIDEO_OUT_RESOLUTION_720_SIMULVIEW_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_1024x720_SIMULVIEW_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_960x720_SIMULVIEW_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_800x720_SIMULVIEW_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
			add_mode(CELL_VIDEO_OUT_RESOLUTION_640x720_SIMULVIEW_FRAME_PACKING, CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE, CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING, CELL_VIDEO_OUT_ASPECT_16_9);
		}
	}

	info->availableModeCount = mode_count;

	return CELL_OK;
}

error_code cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.trace("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

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
	case CELL_VIDEO_OUT_PRIMARY:
	{
		// NOTE: Result is boolean
		if (aspect != CELL_VIDEO_OUT_ASPECT_AUTO && aspect != static_cast<u32>(::at32(g_video_out_aspect_id, g_cfg.video.aspect_ratio)))
		{
			return not_an_error(0);
		}

		if (resolutionId == static_cast<u32>(::at32(g_video_out_resolution_id, g_cfg.video.resolution)))
		{
			// Perfect match
			return not_an_error(1);
		}

		if ((g_cfg.video.stereo_render_mode != stereo_render_mode_options::disabled) && g_cfg.video.resolution == video_resolution::_720p)
		{
			switch (resolutionId)
			{
			case CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING:
			case CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING:
			case CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING:
			case CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING:
			case CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING:
				return not_an_error(1);
			default:
				break;
			}
		}

		return not_an_error(0);
	}

	case CELL_VIDEO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

error_code cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange)
{
	cellSysutil.todo("cellVideoOutGetConvertCursorColorInfo(rgbOutputRange=*0x%x)", rgbOutputRange);

	if (!rgbOutputRange)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER; // TODO: Speculative
	}

	*rgbOutputRange = CELL_VIDEO_OUT_RGB_OUTPUT_RANGE_FULL; // Or CELL_VIDEO_OUT_RGB_OUTPUT_RANGE_LIMITED

	return CELL_OK;
}

error_code cellVideoOutDebugSetMonitorType(u32 videoOut, u32 monitorType)
{
	cellSysutil.todo("cellVideoOutDebugSetMonitorType(videoOut=%d, monitorType=%d)", videoOut, monitorType);
	return CELL_OK;
}

error_code cellVideoOutRegisterCallback(u32 slot, vm::ptr<CellVideoOutCallback> function, vm::ptr<void> userData)
{
	cellSysutil.todo("cellVideoOutRegisterCallback(slot=%d, function=*0x%x, userData=*0x%x)", slot, function, userData);
	return CELL_OK;
}

error_code cellVideoOutUnregisterCallback(u32 slot)
{
	cellSysutil.todo("cellVideoOutUnregisterCallback(slot=%d)", slot);
	return CELL_OK;
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution).flag(MFF_PERFECT);
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
