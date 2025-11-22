#include "stdafx.h"
#include "cellCamera.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/timers.hpp"

#include <cmath>

LOG_CHANNEL(cellCamera);

template <>
void fmt_class_string<CellCameraError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCameraError value)
	{
		switch (value)
		{
		STR_CASE(CELL_CAMERA_ERROR_ALREADY_INIT);
		STR_CASE(CELL_CAMERA_ERROR_NOT_INIT);
		STR_CASE(CELL_CAMERA_ERROR_PARAM);
		STR_CASE(CELL_CAMERA_ERROR_ALREADY_OPEN);
		STR_CASE(CELL_CAMERA_ERROR_NOT_OPEN);
		STR_CASE(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
		STR_CASE(CELL_CAMERA_ERROR_DEVICE_DEACTIVATED);
		STR_CASE(CELL_CAMERA_ERROR_NOT_STARTED);
		STR_CASE(CELL_CAMERA_ERROR_FORMAT_UNKNOWN);
		STR_CASE(CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN);
		STR_CASE(CELL_CAMERA_ERROR_BAD_FRAMERATE);
		STR_CASE(CELL_CAMERA_ERROR_TIMEOUT);
		STR_CASE(CELL_CAMERA_ERROR_BUSY);
		STR_CASE(CELL_CAMERA_ERROR_FATAL);
		STR_CASE(CELL_CAMERA_ERROR_MUTEX);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellCameraFormat>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCameraFormat value)
	{
		switch (value)
		{
		STR_CASE(CELL_CAMERA_FORMAT_UNKNOWN);
		STR_CASE(CELL_CAMERA_JPG);
		STR_CASE(CELL_CAMERA_RAW8);
		STR_CASE(CELL_CAMERA_YUV422);
		STR_CASE(CELL_CAMERA_RAW10);
		STR_CASE(CELL_CAMERA_RGBA);
		STR_CASE(CELL_CAMERA_YUV420);
		STR_CASE(CELL_CAMERA_V_Y1_U_Y0);
		}

		return unknown;
	});
}

// **************
// * Prototypes *
// **************

error_code cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2);
error_code cellCameraReadEx(s32 dev_num, vm::ptr<CellCameraReadEx> read);

// ************************
// * HLE helper functions *
// ************************

static const char* get_camera_attr_name(s32 value)
{
	switch (value)
	{
	case CELL_CAMERA_GAIN: return "GAIN";
	case CELL_CAMERA_REDBLUEGAIN: return "REDBLUEGAIN";
	case CELL_CAMERA_SATURATION: return "SATURATION";
	case CELL_CAMERA_EXPOSURE: return "EXPOSURE";
	case CELL_CAMERA_BRIGHTNESS: return "BRIGHTNESS";
	case CELL_CAMERA_AEC: return "AEC";
	case CELL_CAMERA_AGC: return "AGC";
	case CELL_CAMERA_AWB: return "AWB";
	case CELL_CAMERA_ABC: return "ABC";
	case CELL_CAMERA_LED: return "LED";
	case CELL_CAMERA_AUDIOGAIN: return "AUDIOGAIN";
	case CELL_CAMERA_QS: return "QS";
	case CELL_CAMERA_NONZEROCOEFFS: return "NONZEROCOEFFS";
	case CELL_CAMERA_YUVFLAG: return "YUVFLAG";
	case CELL_CAMERA_JPEGFLAG: return "JPEGFLAG";
	case CELL_CAMERA_BACKLIGHTCOMP: return "BACKLIGHTCOMP";
	case CELL_CAMERA_MIRRORFLAG: return "MIRRORFLAG";
	case CELL_CAMERA_MEASUREDQS: return "MEASUREDQS";
	case CELL_CAMERA_422FLAG: return "422FLAG";
	case CELL_CAMERA_USBLOAD: return "USBLOAD";
	case CELL_CAMERA_GAMMA: return "GAMMA";
	case CELL_CAMERA_GREENGAIN: return "GREENGAIN";
	case CELL_CAMERA_AGCLIMIT: return "AGCLIMIT";
	case CELL_CAMERA_DENOISE: return "DENOISE";
	case CELL_CAMERA_FRAMERATEADJUST: return "FRAMERATEADJUST";
	case CELL_CAMERA_PIXELOUTLIERFILTER: return "PIXELOUTLIERFILTER";
	case CELL_CAMERA_AGCLOW: return "AGCLOW";
	case CELL_CAMERA_AGCHIGH: return "AGCHIGH";
	case CELL_CAMERA_DEVICELOCATION: return "DEVICELOCATION";
	case CELL_CAMERA_FORMATCAP: return "FORMATCAP";
	case CELL_CAMERA_FORMATINDEX: return "FORMATINDEX";
	case CELL_CAMERA_NUMFRAME: return "NUMFRAME";
	case CELL_CAMERA_FRAMEINDEX: return "FRAMEINDEX";
	case CELL_CAMERA_FRAMESIZE: return "FRAMESIZE";
	case CELL_CAMERA_INTERVALTYPE: return "INTERVALTYPE";
	case CELL_CAMERA_INTERVALINDEX: return "INTERVALINDEX";
	case CELL_CAMERA_INTERVALVALUE: return "INTERVALVALUE";
	case CELL_CAMERA_COLORMATCHING: return "COLORMATCHING";
	case CELL_CAMERA_PLFREQ: return "PLFREQ";
	case CELL_CAMERA_DEVICEID: return "DEVICEID";
	case CELL_CAMERA_DEVICECAP: return "DEVICECAP";
	case CELL_CAMERA_DEVICESPEED: return "DEVICESPEED";
	case CELL_CAMERA_UVCREQCODE: return "UVCREQCODE";
	case CELL_CAMERA_UVCREQDATA: return "UVCREQDATA";
	case CELL_CAMERA_DEVICEID2: return "DEVICEID2";
	case CELL_CAMERA_READMODE: return "READMODE";
	case CELL_CAMERA_GAMEPID: return "GAMEPID";
	case CELL_CAMERA_PBUFFER: return "PBUFFER";
	case CELL_CAMERA_READFINISH: return "READFINISH";
	}

	return nullptr;
}

camera_context::camera_context(utils::serial& ar)
{
	save(ar);
}

void camera_context::save(utils::serial& ar)
{
	ar(init);

	if (!init)
	{
		return;
	}

	const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellCamera);

	ar(notify_data_map, start_timestamp_us, read_mode, is_streaming, is_attached, is_open, info, attr, frame_num);

	if (ar.is_writing() || version >= 2)
	{
		ar(is_attached_dirty);
	}

	if (!ar.is_writing())
	{
		if (is_open)
		{
			if (!open_camera())
			{
				cellCamera.error("Failed to open camera while loading savestate");
			}
			else if (is_streaming && !start_camera())
			{
				cellCamera.error("Failed to start camera while loading savestate");
			}
		}
	}
}

gem_camera_shared::gem_camera_shared(utils::serial& ar)
{
	save(ar);
}

void gem_camera_shared::save(utils::serial& ar)
{
	const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellCamera);

	if (ar.is_writing() || version >= 2)
	{
		ar(frame_timestamp_us, width, height, size, format);
	}
}

static bool check_dev_num(s32 dev_num)
{
	return dev_num == 0;
}

template <typename VariantOfCellCameraInfo>
static error_code check_camera_info(const VariantOfCellCameraInfo& info)
{
	// TODO: I managed to get 0x80990004 once. :thonkang:

	if (info.format == CELL_CAMERA_FORMAT_UNKNOWN)
	{
		return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
	}

	if (info.resolution == CELL_CAMERA_RESOLUTION_UNKNOWN)
	{
		return CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN;
	}

	if (info.framerate <= 24)
	{
		return CELL_CAMERA_ERROR_BAD_FRAMERATE;
	}

	auto check_fps = [fps = info.framerate](std::initializer_list<s32> range)
	{
		return std::find(range.begin(), range.end(), fps) != range.end();
	};

	switch (g_cfg.io.camera_type)
	{
	case fake_camera_type::eyetoy:
		switch (info.format)
		{
		case CELL_CAMERA_JPG:
			switch (info.resolution)
			{
			case CELL_CAMERA_VGA:
			case CELL_CAMERA_WGA:
				if (!check_fps({ 25, 30 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_QVGA:
				if (!check_fps({ 25, 30, 50, 60 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT:
			default:
				// TODO
				return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
			}
			break;
		case CELL_CAMERA_RAW8:
		case CELL_CAMERA_YUV422:
		case CELL_CAMERA_RAW10:
		case CELL_CAMERA_RGBA:
		case CELL_CAMERA_YUV420:
		case CELL_CAMERA_V_Y1_U_Y0:
		default:
			// TODO (also check those other formats)
			return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
		}
		break;
	case fake_camera_type::eyetoy2:
		switch (info.format)
		{
		case CELL_CAMERA_JPG:
		case CELL_CAMERA_RAW10:
			return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
		case CELL_CAMERA_RAW8:
			switch (info.resolution)
			{
			case CELL_CAMERA_VGA:
			case CELL_CAMERA_WGA:
				if (!check_fps({ 25, 30, 50, 60 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_QVGA:
				if (!check_fps({ 25, 30, 50, 60, 100, 120 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT:
			default:
				// TODO
				return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
			}
			break;
		case CELL_CAMERA_YUV422:
		case CELL_CAMERA_RGBA:      // < TODO check: they all seem to pass the same resolutions as in CELL_CAMERA_YUV422
		case CELL_CAMERA_YUV420:    // <
		case CELL_CAMERA_V_Y1_U_Y0: // <
		default:                    // <
			switch (info.resolution)
			{
			case CELL_CAMERA_VGA:
			case CELL_CAMERA_WGA:
				if (!check_fps({ 25, 30 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_QVGA:
				if (!check_fps({ 25, 30, 50, 60 }))
					return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
				break;
			case CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT:
			default:
				// TODO
				return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
			}
			break;
		}
		break;
	case fake_camera_type::uvc1_1:
		switch (info.format)
		{
		case CELL_CAMERA_JPG:
		case CELL_CAMERA_YUV422:
			break;
		case CELL_CAMERA_RAW8:
		case CELL_CAMERA_RAW10:
		case CELL_CAMERA_RGBA:
		case CELL_CAMERA_YUV420:
		case CELL_CAMERA_V_Y1_U_Y0:
		default:
			// TODO
			return CELL_CAMERA_ERROR_FORMAT_UNKNOWN;
		}
		break;
	case fake_camera_type::unknown:
	default:
		// TODO
		break;
	}
	return CELL_OK;
}

std::pair<u32, u32> get_video_resolution(const CellCameraInfoEx& info)
{
	switch (info.resolution)
	{
	case CELL_CAMERA_VGA: return{ 640, 480 };
	case CELL_CAMERA_QVGA: return { 320, 240 };
	case CELL_CAMERA_WGA: return{ 640, 360 };
	case CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT: return{ info.width, info.height };
	case CELL_CAMERA_RESOLUTION_UNKNOWN:
	default: return{ 0, 0 };
	}
}

u32 get_buffer_size_by_format(s32 format, s32 width, s32 height)
{
	double bytes_per_pixel = 0.0;
	switch (format)
	{
	case CELL_CAMERA_RAW8:
		bytes_per_pixel = 1.0;
		break;
	case CELL_CAMERA_YUV422:
		bytes_per_pixel = 2.0;
		break;
	case CELL_CAMERA_YUV420:
	case CELL_CAMERA_V_Y1_U_Y0:
		bytes_per_pixel = 1.5;
		break;
	case CELL_CAMERA_RAW10:
		bytes_per_pixel = 1.25;
		break;
	case CELL_CAMERA_JPG:
	case CELL_CAMERA_RGBA:
	case CELL_CAMERA_FORMAT_UNKNOWN:
	default:
		bytes_per_pixel = 4.0;
		break;
	}

	return ::narrow<u32>(static_cast<u64>(std::ceil(width * height * bytes_per_pixel)));
}


u32 get_video_buffer_size(const CellCameraInfoEx& info)
{
	u32 width, height;
	std::tie(width, height) = get_video_resolution(info);
	return get_buffer_size_by_format(info.format, width, height);
}

// ************************
// * cellCamera functions *
// ************************

// This represents 4 almost identical subfunctions used by the Start/Stop/Reset/Close functions
error_code check_init_and_open(s32 dev_num)
{
	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}
	// TODO: Yet another CELL_CAMERA_ERROR_BUSY

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}
	if (!g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}
	return CELL_OK;
}

// This represents a recurring subfunction throughout libCamera
error_code check_resolution(s32 /*dev_num*/)
{
	// TODO: Some sort of connection check maybe?
	//if (error == CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN)
	//{
	//	return CELL_CAMERA_ERROR_TIMEOUT;
	//}
	// TODO: Yet another CELL_CAMERA_ERROR_FATAL
	return CELL_OK;
}

// This represents an often used sequence in libCamera (usually the beginning of a subfunction).
// There also exist common sequences for mutex lock/unlock by the way.
error_code check_resolution_ex(s32 dev_num)
{
	// TODO: Yet another CELL_CAMERA_ERROR_BUSY

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}
	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}
	return CELL_OK;
}


error_code cellCameraInit()
{
	cellCamera.todo("cellCameraInit()");

	// Start camera thread
	auto& g_camera = g_fxo->get<camera_thread>();

	std::lock_guard lock(g_camera.mutex);

	if (g_camera.init)
	{
		return CELL_CAMERA_ERROR_ALREADY_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		g_camera.init = 1;
		return CELL_OK;
	}

	switch (g_cfg.io.camera_type)
	{
	case fake_camera_type::eyetoy:
	{
		g_camera.attr[CELL_CAMERA_SATURATION] = { 164 };
		g_camera.attr[CELL_CAMERA_BRIGHTNESS] = { 96 };
		g_camera.attr[CELL_CAMERA_AEC] = { 1 };
		g_camera.attr[CELL_CAMERA_AGC] = { 1 };
		g_camera.attr[CELL_CAMERA_AWB] = { 1 };
		g_camera.attr[CELL_CAMERA_ABC] = { 0 };
		g_camera.attr[CELL_CAMERA_LED] = { 1 };
		g_camera.attr[CELL_CAMERA_QS] = { 0 };
		g_camera.attr[CELL_CAMERA_NONZEROCOEFFS] = { 32, 32 };
		g_camera.attr[CELL_CAMERA_YUVFLAG] = { 0 };
		g_camera.attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		g_camera.attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		g_camera.attr[CELL_CAMERA_422FLAG] = { 1 };
		g_camera.attr[CELL_CAMERA_USBLOAD] = { 4 };
		break;
	}
	case fake_camera_type::eyetoy2:
	{
		g_camera.attr[CELL_CAMERA_SATURATION] = { 64 };
		g_camera.attr[CELL_CAMERA_BRIGHTNESS] = { 8 };
		g_camera.attr[CELL_CAMERA_AEC] = { 1 };
		g_camera.attr[CELL_CAMERA_AGC] = { 1 };
		g_camera.attr[CELL_CAMERA_AWB] = { 1 };
		g_camera.attr[CELL_CAMERA_LED] = { 1 };
		g_camera.attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		g_camera.attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		g_camera.attr[CELL_CAMERA_GAMMA] = { 1 };
		g_camera.attr[CELL_CAMERA_AGCLIMIT] = { 4 };
		g_camera.attr[CELL_CAMERA_DENOISE] = { 0 };
		g_camera.attr[CELL_CAMERA_FRAMERATEADJUST] = { 0 };
		g_camera.attr[CELL_CAMERA_PIXELOUTLIERFILTER] = { 1 };
		g_camera.attr[CELL_CAMERA_AGCLOW] = { 48 };
		g_camera.attr[CELL_CAMERA_AGCHIGH] = { 64 };
		break;
	}
	case fake_camera_type::uvc1_1:
	{
		g_camera.attr[CELL_CAMERA_DEVICEID] = { 0x5ca, 0x18d0 }; // KBCR-S01MU
		g_camera.attr[CELL_CAMERA_FORMATCAP] = { CELL_CAMERA_JPG | CELL_CAMERA_YUV422 };
		g_camera.attr[CELL_CAMERA_NUMFRAME] = { 1 }; // Amount of supported resolutions
		break;
	}
	default:
		cellCamera.todo("Trying to init cellCamera with un-researched camera type.");
		break;
	}

	// TODO: Some other default attributes? Need to check the actual behaviour on a real PS3.

	g_camera.is_attached = g_cfg.io.camera != camera_handler::null;
	g_camera.init = 1;
	return CELL_OK;
}

error_code cellCameraEnd()
{
	cellCamera.todo("cellCameraEnd()");

	auto& g_camera = g_fxo->get<camera_thread>();

	std::lock_guard lock(g_camera.mutex);

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	// TODO: call cellCameraClose(0), ignore errors

	// TODO
	g_camera.init = 0;
	g_camera.reset_state();
	return CELL_OK;
}

error_code cellCameraOpen(s32 dev_num, vm::ptr<CellCameraInfo> info)
{
	cellCamera.notice("cellCameraOpen(dev_num=%d, info=*0x%x)", dev_num, info);

	if (!info)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_ALREADY_OPEN;
	}

	if (auto res = check_camera_info(*info))
	{
		return res;
	}

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	return CELL_OK;
}

error_code cellCameraOpenAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraOpenEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraOpenEx(dev_num=%d, info=*0x%x)", dev_num, info);

	// This function has a very weird order of checking for errors

	if (!info)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	if (auto res = cellCameraSetAttribute(dev_num, CELL_CAMERA_READMODE, info->read_mode, 0))
	{
		return res;
	}

	if (info->read_mode == CELL_CAMERA_READ_DIRECT)
	{
		// Note: arg1 is the return value of previous SetAttribute
		if (auto res = cellCameraSetAttribute(dev_num, CELL_CAMERA_GAMEPID, 0, 0))
		{
			return res;
		}
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_ALREADY_OPEN;
	}

	if (auto res = check_camera_info(*info))
	{
		return res;
	}

	// calls cellCameraGetAttribute(dev_num, CELL_CAMERA_PBUFFER) at some point

	const auto vbuf_size = get_video_buffer_size(*info);

	std::lock_guard lock(g_camera.mutex);

	// TODO: find out if the buffers are also checked for nullptr
	if (info->read_mode == CELL_CAMERA_READ_DIRECT)
	{
		info->pbuf[0] = vm::cast(vm::alloc(vbuf_size, vm::main));
		info->pbuf[1] = vm::cast(vm::alloc(vbuf_size, vm::main));

		// TODO: verify
		info->bytesize = vbuf_size;
	}
	else
	{
		info->buffer = vm::cast(vm::alloc(vbuf_size, vm::main));
		info->bytesize = vbuf_size;
	}

	std::tie(info->width, info->height) = get_video_resolution(*info);

	if (!g_camera.open_camera())
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	g_camera.is_open = true;
	g_camera.info = *info;

	cellCamera.notice("cellCameraOpen info: format=%s, resolution=%d, framerate=%d, bytesize=%d, width=%d, height=%d, dev_num=%d, guid=%d",
		info->format, info->resolution, info->framerate, info->bytesize, info->width, info->height, info->dev_num, info->guid);

	auto& shared_data = g_fxo->get<gem_camera_shared>();
	shared_data.width = info->width > 0 ? +info->width : 640;
	shared_data.height = info->height > 0 ? +info->height : 480;
	shared_data.size = vbuf_size;
	shared_data.format = info->format;

	return CELL_OK;
}

error_code cellCameraOpenPost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraClose(s32 dev_num)
{
	cellCamera.notice("cellCameraClose(dev_num=%d)", dev_num);

	if (error_code error = check_init_and_open(dev_num))
	{
		return error;
	}

	// TODO: Yet another CELL_CAMERA_ERROR_BUSY

	if (dev_num != 0)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();
	std::lock_guard lock(g_camera.mutex);
	g_camera.is_streaming = false;

	if (g_camera.info.buffer)
	{
		vm::dealloc(g_camera.info.buffer.addr(), vm::main);
	}
	if (g_camera.info.pbuf[0])
	{
		vm::dealloc(g_camera.info.pbuf[0].addr(), vm::main);
	}
	if (g_camera.info.pbuf[1])
	{
		vm::dealloc(g_camera.info.pbuf[1].addr(), vm::main);
	}

	g_camera.close_camera();
	g_camera.is_open = false;

	return CELL_OK;
}

error_code cellCameraCloseAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraClosePost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraGetDeviceGUID(s32 dev_num, vm::ptr<u32> guid)
{
	cellCamera.notice("cellCameraGetDeviceGUID(dev_num=%d, guid=*0x%x)", dev_num, guid);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (guid)
	{
		*guid = 0; // apparently always 0
	}

	return CELL_OK;
}

error_code cellCameraGetType(s32 dev_num, vm::ptr<s32> type)
{
	cellCamera.trace("cellCameraGetType(dev_num=%d, type=*0x%x)", dev_num, type);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	if (!check_dev_num(dev_num) || !type)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	switch (g_cfg.io.camera_type.get())
	{
	case fake_camera_type::unknown: *type = CELL_CAMERA_TYPE_UNKNOWN; break;
	case fake_camera_type::eyetoy:  *type = CELL_CAMERA_EYETOY; break;
	case fake_camera_type::eyetoy2: *type = CELL_CAMERA_EYETOY2; break;
	case fake_camera_type::uvc1_1:  *type = CELL_CAMERA_USBVIDEOCLASS; break;
	}

	return CELL_OK;
}

s32 cellCameraIsAvailable(s32 dev_num)
{
	cellCamera.trace("cellCameraIsAvailable(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	vm::var<s32> type;

	if (cellCameraGetType(dev_num, type) != CELL_OK || *type == CELL_CAMERA_TYPE_UNKNOWN)
	{
		return false;
	}

	if (*type > CELL_CAMERA_TYPE_UNKNOWN || *type <= CELL_CAMERA_USBVIDEOCLASS)
	{
		// TODO: checks CELL_CAMERA_DEVICESPEED attribute
	}

	return true;
}

s32 cellCameraIsAttached(s32 dev_num)
{
	cellCamera.trace("cellCameraIsAttached(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return 0;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return 0;
	}

	if (!check_dev_num(dev_num))
	{
		return 0;
	}

	vm::var<s32> type;

	if (cellCameraGetType(dev_num, type) != CELL_OK)
	{
		return 0;
	}

	std::lock_guard lock(g_camera.mutex);

	bool is_attached = g_camera.is_attached;

	if (g_cfg.io.camera == camera_handler::fake)
	{
		// "attach" camera here
		// normally should be attached immediately after event queue is registered, but just to be sure
		if (!is_attached)
		{
			g_camera.is_attached = is_attached = true;
			g_camera.is_attached_dirty = true;
		}
	}

	return is_attached ? 1 : 0;
}

s32 cellCameraIsOpen(s32 dev_num)
{
	cellCamera.trace("cellCameraIsOpen(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	std::lock_guard lock(g_camera.mutex);

	return g_camera.is_open.load();
}

s32 cellCameraIsStarted(s32 dev_num)
{
	cellCamera.trace("cellCameraIsStarted(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	std::lock_guard lock(g_camera.mutex);

	return g_camera.is_streaming.load();
}

error_code cellCameraGetAttribute(s32 dev_num, s32 attrib, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	cellCamera.notice("cellCameraGetAttribute(dev_num=%d, attrib=%d=%s, arg1=*0x%x, arg2=*0x%x)", dev_num, attrib, attr_name, arg1, arg2);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	// actually compares <= 0x63 which is equivalent
	if (attrib < CELL_CAMERA_FORMATCAP && !g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!arg1)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	std::lock_guard lock(g_camera.mutex);

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (arg1)
	{
		*arg1 = g_camera.attr[attrib].v1;
	}

	if (arg2)
	{
		*arg2 = g_camera.attr[attrib].v2;
	}

	cellCamera.todo("cellCameraGetAttribute(attr_name=%s, v1=%d, v2=%d)", attr_name, g_camera.attr[attrib].v1, g_camera.attr[attrib].v2);
	return CELL_OK;
}

error_code cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	(attrib == CELL_CAMERA_LED ? cellCamera.trace : cellCamera.todo)("cellCameraSetAttribute(dev_num=%d, attrib=%d=%s, arg1=%d, arg2=%d)", dev_num, attrib, attr_name, arg1, arg2);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	// actually compares <= 0x63 which is equivalent
	if (attrib < CELL_CAMERA_FORMATCAP && !g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	g_camera.set_attr(attrib, arg1, arg2);

	return CELL_OK;
}

error_code cellCameraResetAttribute()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraGetBufferSize(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.notice("cellCameraGetBufferSize(dev_num=%d, info=*0x%x)", dev_num, info);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_ALREADY_OPEN;
	}

	if (!info)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (auto res = check_camera_info(*info))
	{
		return res;
	}

	if (!cellCameraIsAttached(dev_num))
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (auto status = cellCameraSetAttribute(dev_num, CELL_CAMERA_READMODE, info->read_mode, 0))
	{
		return status;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	std::lock_guard lock(g_camera.mutex);

	g_camera.info = *info;
	info->bytesize = get_video_buffer_size(g_camera.info);

	cellCamera.notice("cellCameraGetBufferSize info: format=%d, resolution=%d, framerate=%d, bytesize=%d, width=%d, height=%d, dev_num=%d, guid=%d",
		info->format, info->resolution, info->framerate, info->bytesize, info->width, info->height, info->dev_num, info->guid);

	return not_an_error(info->bytesize);
}

error_code check_get_camera_info(s32 dev_num, bool is_valid_info_struct)
{
	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	if (!g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!is_valid_info_struct)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellCameraGetBufferInfo(s32 dev_num, vm::ptr<CellCameraInfo> info)
{
	cellCamera.notice("cellCameraGetBufferInfo(dev_num=%d, info=0x%x)", dev_num, info);

	// called by cellCameraGetBufferInfoEx

	if (error_code error = check_get_camera_info(dev_num, !!info))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();
	std::lock_guard lock(g_camera.mutex);

	info->format = g_camera.info.format;
	info->resolution = g_camera.info.resolution;
	info->framerate = g_camera.info.framerate;
	info->buffer = g_camera.info.buffer;
	info->bytesize = g_camera.info.bytesize;
	info->width = g_camera.info.width;
	info->height = g_camera.info.height;
	info->dev_num = g_camera.info.dev_num;
	info->guid = g_camera.info.guid;

	return CELL_OK;
}

error_code cellCameraGetBufferInfoEx(ppu_thread& ppu, s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	ppu.state += cpu_flag::wait;

	cellCamera.notice("cellCameraGetBufferInfoEx(dev_num=%d, info=0x%x)", dev_num, info);

	// calls cellCameraGetBufferInfo

	if (error_code error = check_get_camera_info(dev_num, !!info))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	CellCameraInfoEx info_out;

	{
		std::lock_guard lock(g_camera.mutex);

		info_out = g_camera.info;
	}

	*info = info_out;
	return CELL_OK;
}

error_code cellCameraPrepExtensionUnit(s32 dev_num, vm::ptr<u8> guidExtensionCode)
{
	cellCamera.todo("cellCameraPrepExtensionUnit(dev_num=%d, guidExtensionCode=0x%x)", dev_num, guidExtensionCode);

	if (!check_dev_num(dev_num) || !guidExtensionCode)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (error_code error = check_resolution(dev_num))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	return CELL_OK;
}

error_code cellCameraCtrlExtensionUnit(s32 dev_num, u8 request, u16 value, u16 length, vm::ptr<u8> data)
{
	cellCamera.todo("cellCameraCtrlExtensionUnit(dev_num=%d, request=%d, value=%d, length=%d, data=*0x%x)", dev_num, request, value, length, data);

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!data)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	// TODO: Yet another CELL_CAMERA_ERROR_PARAM

	return CELL_OK;
}

error_code cellCameraGetExtensionUnit(s32 dev_num, u16 value, u16 length, vm::ptr<u8> data)
{
	cellCamera.todo("cellCameraGetExtensionUnit(dev_num=%d, value=%d, length=%d, data=*0x%x)", dev_num, value, length, data);

	return cellCameraCtrlExtensionUnit(dev_num, GET_CUR, value, length, data);
}

error_code cellCameraSetExtensionUnit(s32 dev_num, u16 value, u16 length, vm::ptr<u8> data)
{
	cellCamera.todo("cellCameraSetExtensionUnit(dev_num=%d, value=%d, length=%d, data=*0x%x)", dev_num, value, length, data);

	return cellCameraCtrlExtensionUnit(dev_num, SET_CUR, value, length, data);
}

error_code cellCameraSetContainer()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraReset(s32 dev_num)
{
	cellCamera.todo("cellCameraReset(dev_num=%d)", dev_num);

	if (error_code error = check_init_and_open(dev_num))
	{
		return error;
	}

	if (error_code error = check_resolution_ex(dev_num))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	// TODO reset camera

	return CELL_OK;
}

error_code cellCameraResetAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraResetPost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraStart(s32 dev_num)
{
	cellCamera.notice("cellCameraStart(dev_num=%d)", dev_num);

	if (error_code error = check_init_and_open(dev_num))
	{
		return error;
	}

	if (error_code error = check_resolution_ex(dev_num))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();
	std::lock_guard lock(g_camera.mutex);

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!g_camera.start_camera())
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	// TODO: Yet another CELL_CAMERA_ERROR_TIMEOUT

	g_camera.start_timestamp_us = get_guest_system_time();
	g_camera.is_streaming = true;

	return CELL_OK;
}

error_code cellCameraStartAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraStartPost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraRead(s32 dev_num, vm::ptr<u32> frame_num, vm::ptr<u32> bytes_read)
{
	cellCamera.trace("cellCameraRead(dev_num=%d, frame_num=*0x%x, bytes_read=*0x%x)", dev_num, frame_num, bytes_read);

	vm::ptr<CellCameraReadEx> read_ex = vm::make_var<CellCameraReadEx>({});

	if (auto res = cellCameraReadEx(dev_num, read_ex))
	{
		return res;
	}

	if (frame_num)
	{
		*frame_num = read_ex->frame;
	}

	if (bytes_read)
	{
		*bytes_read = read_ex->bytesread;
	}

	return CELL_OK;
}

error_code cellCameraRead2()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraReadEx(s32 dev_num, vm::ptr<CellCameraReadEx> read)
{
	cellCamera.trace("cellCameraReadEx(dev_num=%d, read=0x%x)", dev_num, read);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	std::lock_guard lock(g_camera.mutex);

	if (!g_camera.is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!g_camera.is_streaming)
	{
		return CELL_CAMERA_ERROR_NOT_STARTED;
	}

	// can call cellCameraReset() and cellCameraStop() in some cases

	const bool has_new_frame = g_camera.has_new_frame.exchange(false);

	if (g_camera.handler)
	{
		if (has_new_frame)
		{
			u32 width{};
			u32 height{};
			u64 frame_number{};
			u64 bytes_read{};

			if (!g_camera.get_camera_frame(g_camera.info.buffer.get_ptr(), width, height, frame_number, bytes_read))
			{
				return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
			}

			g_camera.bytes_read = ::narrow<u32>(bytes_read);

			cellCamera.trace("cellCameraRead: frame_number=%d, width=%d, height=%d. bytes_read=%d (passed to game: frame=%d, bytesread=%d)",
				frame_number, width, height, bytes_read, read ? read->frame.get() : 0, read ? read->bytesread.get() : 0);
		}
	}
	else
	{
		g_camera.bytes_read = g_camera.is_streaming ? get_video_buffer_size(g_camera.info) : 0;
	}

	if (has_new_frame)
	{
		g_camera.frame_timestamp_us = get_guest_system_time() - g_camera.start_timestamp_us;
	}

	if (read) // NULL returns CELL_OK
	{
		read->timestamp = g_camera.frame_timestamp_us;
		read->frame = g_camera.frame_num;
		read->bytesread = g_camera.bytes_read;

		auto& shared_data = g_fxo->get<gem_camera_shared>();

		shared_data.frame_timestamp_us.store(read->timestamp);
	}

	return CELL_OK;
}

error_code cellCameraReadComplete(s32 dev_num, u32 bufnum, u32 arg2)
{
	cellCamera.todo("cellCameraReadComplete(dev_num=%d, bufnum=%d, arg2=%d)", dev_num, bufnum, arg2);

	if (bufnum < 2)
	{
		auto& g_camera = g_fxo->get<camera_thread>();
		std::lock_guard lock(g_camera.mutex);
		g_camera.pbuf_locked[bufnum] = false;
	}

	return cellCameraSetAttribute(dev_num, CELL_CAMERA_READFINISH, bufnum, arg2);
}

error_code cellCameraStop(s32 dev_num)
{
	cellCamera.notice("cellCameraStop(dev_num=%d)", dev_num);

	if (error_code error = check_init_and_open(dev_num))
	{
		return error;
	}

	if (error_code error = check_resolution_ex(dev_num))
	{
		return error;
	}

	auto& g_camera = g_fxo->get<camera_thread>();
	std::lock_guard lock(g_camera.mutex);

	if (!g_camera.is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!g_camera.is_streaming)
	{
		return CELL_CAMERA_ERROR_NOT_STARTED;
	}

	g_camera.stop_camera();
	g_camera.is_streaming = false;

	return CELL_OK;
}

error_code cellCameraStopAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraStopPost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraSetNotifyEventQueue(u64 key)
{
	cellCamera.notice("cellCameraSetNotifyEventQueue(key=0x%x)", key);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (error_code error = check_resolution(0))
	{
		return error;
	}

	g_camera.add_queue(key, 0, 0);

	return CELL_OK;
}

error_code cellCameraRemoveNotifyEventQueue(u64 key)
{
	cellCamera.notice("cellCameraRemoveNotifyEventQueue(key=0x%x)", key);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (error_code error = check_resolution(0))
	{
		return error;
	}

	g_camera.remove_queue(key);

	return CELL_OK;
}

error_code cellCameraSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	cellCamera.notice("cellCameraSetNotifyEventQueue2(key=0x%x, source=%d, flag=%d)", key, source, flag);

	auto& g_camera = g_fxo->get<camera_thread>();

	if (!g_camera.init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (error_code error = check_resolution(0))
	{
		return error;
	}

	g_camera.add_queue(key, source, flag);

	return CELL_OK;
}

error_code cellCameraRemoveNotifyEventQueue2(u64 key)
{
	cellCamera.notice("cellCameraRemoveNotifyEventQueue2(key=0x%x)", key);

	return cellCameraRemoveNotifyEventQueue(key);
}

DECLARE(ppu_module_manager::cellCamera)("cellCamera", []()
{
	REG_FUNC(cellCamera, cellCameraInit);
	REG_FUNC(cellCamera, cellCameraEnd);
	REG_FUNC(cellCamera, cellCameraOpen);
	REG_FUNC(cellCamera, cellCameraOpenAsync);
	REG_FUNC(cellCamera, cellCameraOpenEx);
	REG_FUNC(cellCamera, cellCameraOpenPost);
	REG_FUNC(cellCamera, cellCameraClose);
	REG_FUNC(cellCamera, cellCameraCloseAsync);
	REG_FUNC(cellCamera, cellCameraClosePost);

	REG_FUNC(cellCamera, cellCameraGetDeviceGUID);
	REG_FUNC(cellCamera, cellCameraGetType);
	REG_FUNC(cellCamera, cellCameraIsAvailable);
	REG_FUNC(cellCamera, cellCameraIsAttached);
	REG_FUNC(cellCamera, cellCameraIsOpen);
	REG_FUNC(cellCamera, cellCameraIsStarted);
	REG_FUNC(cellCamera, cellCameraGetAttribute);
	REG_FUNC(cellCamera, cellCameraSetAttribute);
	REG_FUNC(cellCamera, cellCameraResetAttribute);
	REG_FUNC(cellCamera, cellCameraGetBufferSize);
	REG_FUNC(cellCamera, cellCameraGetBufferInfo);
	REG_FUNC(cellCamera, cellCameraGetBufferInfoEx);

	REG_FUNC(cellCamera, cellCameraPrepExtensionUnit);
	REG_FUNC(cellCamera, cellCameraCtrlExtensionUnit);
	REG_FUNC(cellCamera, cellCameraGetExtensionUnit);
	REG_FUNC(cellCamera, cellCameraSetExtensionUnit);
	REG_FUNC(cellCamera, cellCameraSetContainer);

	REG_FUNC(cellCamera, cellCameraReset);
	REG_FUNC(cellCamera, cellCameraResetAsync);
	REG_FUNC(cellCamera, cellCameraResetPost);
	REG_FUNC(cellCamera, cellCameraStart);
	REG_FUNC(cellCamera, cellCameraStartAsync);
	REG_FUNC(cellCamera, cellCameraStartPost);
	REG_FUNC(cellCamera, cellCameraRead);
	REG_FUNC(cellCamera, cellCameraRead2);
	REG_FUNC(cellCamera, cellCameraReadEx);
	REG_FUNC(cellCamera, cellCameraReadComplete);
	REG_FUNC(cellCamera, cellCameraStop);
	REG_FUNC(cellCamera, cellCameraStopAsync);
	REG_FUNC(cellCamera, cellCameraStopPost);

	REG_FUNC(cellCamera, cellCameraSetNotifyEventQueue);
	REG_FUNC(cellCamera, cellCameraRemoveNotifyEventQueue);
	REG_FUNC(cellCamera, cellCameraSetNotifyEventQueue2);
	REG_FUNC(cellCamera, cellCameraRemoveNotifyEventQueue2);
});

// camera_thread members

void camera_context::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting && !Emu.IsStopped())
	{
		// send ATTACH event
		if (init && is_attached_dirty && !Emu.IsPausedOrReady())
		{
			send_attach_state(is_attached);
		}

		const s32 fps = info.framerate;

		if (!init || !fps || Emu.IsPausedOrReady() || g_cfg.io.camera == camera_handler::null)
		{
			thread_ctrl::wait_for(1000); // hack
			continue;
		}

		const u64 frame_start = get_guest_system_time();

		// Get latest frame with CELL_CAMERA_READ_DIRECT.
		// With CELL_CAMERA_READ_FUNCCALL the game fetches the buffer in cellCameraRead.
		const u64 buffer_number = pbuf_write_index;
		bool send_frame_update_event = false;
		bool frame_update_event_sent = false;

		if (is_streaming)
		{
			if (read_mode.load() == CELL_CAMERA_READ_DIRECT)
			{
				std::lock_guard lock(mutex);
				{
					send_frame_update_event = info.pbuf[pbuf_write_index] && !pbuf_locked[pbuf_write_index];
				}

				if (handler && send_frame_update_event)
				{
					u32 width{};
					u32 height{};
					u64 frame_number{};
					u64 bytes_read{};

					send_frame_update_event = get_camera_frame(info.pbuf[pbuf_write_index].get_ptr(), width, height, frame_number, bytes_read);

					if (send_frame_update_event)
					{
						pbuf_write_index = pbuf_next_index();
					}
				}
			}
			else
			{
				std::lock_guard lock(mutex);
				send_frame_update_event = !handler || on_handler_state(handler->get_state());
			}
		}

		std::unique_lock lock(mutex_notify_data_map);

		for (const auto& [key, evt_data] : notify_data_map)
		{
			// handle FRAME_UPDATE
			if (send_frame_update_event && evt_data.flag & CELL_CAMERA_EFLAG_FRAME_UPDATE)
			{
				if (auto queue = lv2_event_queue::find(key))
				{
					u64 data2 = 0;
					u64 data3 = 0;

					if (read_mode.load() == CELL_CAMERA_READ_DIRECT)
					{
						const u64 image_data_size = static_cast<u64>(info.bytesize);
						const u64 camera_id = 0;

						data2 = image_data_size << 32 | buffer_number << 16 | camera_id;
						data3 = get_guest_system_time() - start_timestamp_us; // timestamp
					}
					else // CELL_CAMERA_READ_FUNCCALL, also default
					{
						data2 = 0; // device id (always 0)
						data3 = 0; // unused
					}

					if (CellError err = queue->send(evt_data.source, CELL_CAMERA_FRAME_UPDATE, data2, data3)) [[unlikely]]
					{
						cellCamera.warning("Failed to send frame update event (error=0x%x)", err);
					}

					frame_update_event_sent = true;
				}
			}
		}

		++frame_num;
		has_new_frame = true;

		if (read_mode.load() == CELL_CAMERA_READ_DIRECT && frame_update_event_sent)
		{
			std::lock_guard lock(mutex);
			pbuf_locked[buffer_number] = true;
		}

		lock.unlock();

		for (const u64 frame_target_time = 1000000u / fps; !Emu.IsStopped();)
		{
			const u64 time_passed = get_guest_system_time() - frame_start;
			if (time_passed >= frame_target_time)
				break;

			lv2_obj::wait_timeout(frame_target_time - time_passed);
		}
	}
}

bool camera_context::open_camera()
{
	Emu.BlockingCallFromMainThread([this]()
	{
		handler.reset();
		handler = Emu.GetCallbacks().get_camera_handler();
		if (handler)
		{
			handler->open_camera();
		}
	});

	return !handler || on_handler_state(handler->get_state());
}

bool camera_context::start_camera()
{
	if (handler)
	{
		handler->set_mirrored(!!attr[CELL_CAMERA_MIRRORFLAG].v1);
		handler->set_frame_rate(info.framerate);
		handler->set_resolution(info.width, info.height);
		handler->set_format(info.format, info.bytesize);

		Emu.BlockingCallFromMainThread([this]()
		{
			handler->start_camera();
		});

		return on_handler_state(handler->get_state());
	}

	return true;
}

bool camera_context::get_camera_frame(u8* dst, u32& width, u32& height, u64& frame_number, u64& bytes_read)
{
	if (handler)
	{
		return on_handler_state(handler->get_image(dst, info.bytesize, width, height, frame_number, bytes_read));
	}

	return true;
}

void camera_context::stop_camera()
{
	if (handler)
	{
		Emu.BlockingCallFromMainThread([this]()
		{
			handler->stop_camera();
		});
	}
}

void camera_context::close_camera()
{
	if (handler)
	{
		Emu.BlockingCallFromMainThread([this]()
		{
			handler->close_camera();
		});
	}
}

void camera_context::reset_state()
{
	read_mode = CELL_CAMERA_READ_FUNCCALL;
	is_streaming = false;
	is_attached = false;
	is_attached_dirty = false;
	is_open = false;
	info.framerate = 0;
	std::memset(&attr, 0, sizeof(attr));
	handler.reset();
	pbuf_write_index = 0;
	pbuf_locked[0] = false;
	pbuf_locked[1] = false;
	has_new_frame = false;
	frame_timestamp_us = 0;
	bytes_read = 0;

	if (info.buffer)
	{
		vm::dealloc(info.buffer.addr(), vm::main);
	}
	if (info.pbuf[0])
	{
		vm::dealloc(info.pbuf[0].addr(), vm::main);
	}
	if (info.pbuf[1])
	{
		vm::dealloc(info.pbuf[1].addr(), vm::main);
	}

	std::scoped_lock lock(mutex_notify_data_map);
	notify_data_map.clear();
}

void camera_context::send_attach_state(bool attached)
{
	std::lock_guard lock(mutex_notify_data_map);

	for (const auto& [key, evt_data] : notify_data_map)
	{
		if (auto queue = lv2_event_queue::find(key))
		{
			if (CellError err = queue->send(evt_data.source, attached ? CELL_CAMERA_ATTACH : CELL_CAMERA_DETACH, 0, 0)) [[unlikely]]
			{
				cellCamera.warning("Failed to send attach event (attached=%d, error=0x%x)", attached, err);
			}
		}
	}

	// We're not expected to send any events for attaching/detaching
	is_attached = attached;
	is_attached_dirty = false;
}

void camera_context::set_attr(s32 attrib, u32 arg1, u32 arg2)
{
	switch (attrib)
	{
	case CELL_CAMERA_READMODE:
	{
		if (arg1 != CELL_CAMERA_READ_FUNCCALL && arg1 != CELL_CAMERA_READ_DIRECT)
		{
			cellCamera.warning("Unknown read mode set: %d", arg1);
			arg1 = CELL_CAMERA_READ_FUNCCALL;
		}
		read_mode.exchange(arg1);
		break;
	}
	case CELL_CAMERA_MIRRORFLAG:
	{
		if (handler)
		{
			handler->set_mirrored(!!arg1);
		}
		break;
	}
	default:
		break;
	}

	std::lock_guard lock(mutex);
	attr[attrib] = {arg1, arg2};
}

void camera_context::add_queue(u64 key, u64 source, u64 flag)
{
	{
		std::lock_guard lock_data_map(mutex_notify_data_map);

		notify_data_map[key] = { source, flag };
	}

	is_attached_dirty = true;
}

void camera_context::remove_queue(u64 key)
{
	std::lock_guard lock(mutex);
	{
		std::lock_guard lock_data_map(mutex_notify_data_map);

		notify_data_map.erase(key);
	}
}

u32 camera_context::pbuf_next_index() const
{
	// The read buffer index cannot be the same as the write index
	return (pbuf_write_index + 1u) % 2;
}

bool camera_context::on_handler_state(camera_handler_base::camera_handler_state state)
{
	switch (state)
	{
	case camera_handler_base::camera_handler_state::closed:
	{
		if (is_attached)
		{
			is_attached = false;
			is_attached_dirty = true;
		}
		if (handler)
		{
			if (is_streaming)
			{
				cellCamera.warning("Camera closed or disconnected (state=%d). Trying to start camera...", static_cast<int>(state));
				Emu.BlockingCallFromMainThread([&]()
				{
					handler->open_camera();
					handler->start_camera();
				});
			}
			else if (is_open)
			{
				cellCamera.warning("Camera closed or disconnected (state=%d). Trying to open camera...", static_cast<int>(state));
				Emu.BlockingCallFromMainThread([&]()
				{
					handler->open_camera();
				});
			}
		}
		return false;
	}
	case camera_handler_base::camera_handler_state::open:
	{
		if (handler && is_streaming)
		{
			cellCamera.warning("Camera handler not running (state=%d). Trying to start camera...", static_cast<int>(state));
			Emu.BlockingCallFromMainThread([&]()
			{
				handler->start_camera();
			});
		}
		break;
	}
	case camera_handler_base::camera_handler_state::running:
	{
		if (!is_attached)
		{
			cellCamera.warning("Camera handler not attached. Sending attach event...", static_cast<int>(state));
			is_attached = true;
			is_attached_dirty = true;
		}
		break;
	}
	}

	return true;
}
