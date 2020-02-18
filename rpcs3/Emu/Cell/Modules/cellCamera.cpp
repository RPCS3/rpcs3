#include "stdafx.h"
#include "cellCamera.h"

#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/IdManager.h"

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
		STR_CASE(CELL_CAMERA_ERROR_FATAL);
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

template <>
void fmt_class_string<camera_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case camera_handler::null: return "Null";
		case camera_handler::fake: return "Fake";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<fake_camera_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case fake_camera_type::unknown: return "Unknown";
		case fake_camera_type::eyetoy: return "EyeToy";
		case fake_camera_type::eyetoy2: return "PS Eye";
		case fake_camera_type::uvc1_1: return "UVC 1.1";
		}

		return unknown;
	});
}

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

static bool check_dev_num(s32 dev_num)
{
	return dev_num == 0;
}

static error_code check_camera_info(const CellCameraInfoEx& info)
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

	auto check_fps = [fps = info.framerate](const std::vector<s32>& range)
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

u32 get_video_buffer_size(const CellCameraInfoEx& info)
{
	u32 width, height;
	std::tie(width, height) = get_video_resolution(info);

	const auto bpp = 4;
	return width * height * bpp;
}

// ************************
// * cellCamera functions *
// ************************

error_code cellCameraInit()
{
	cellCamera.todo("cellCameraInit()");

	// Start camera thread
	const auto g_camera = g_fxo->get<camera_thread>();

	std::lock_guard lock(g_camera->mutex);

	if (g_camera->init)
	{
		return CELL_CAMERA_ERROR_ALREADY_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		g_camera->init = 1;
		return CELL_OK;
	}

	switch (g_cfg.io.camera_type)
	{
	case fake_camera_type::eyetoy:
	{
		g_camera->attr[CELL_CAMERA_SATURATION] = { 164 };
		g_camera->attr[CELL_CAMERA_BRIGHTNESS] = { 96 };
		g_camera->attr[CELL_CAMERA_AEC] = { 1 };
		g_camera->attr[CELL_CAMERA_AGC] = { 1 };
		g_camera->attr[CELL_CAMERA_AWB] = { 1 };
		g_camera->attr[CELL_CAMERA_ABC] = { 0 };
		g_camera->attr[CELL_CAMERA_LED] = { 1 };
		g_camera->attr[CELL_CAMERA_QS] = { 0 };
		g_camera->attr[CELL_CAMERA_NONZEROCOEFFS] = { 32, 32 };
		g_camera->attr[CELL_CAMERA_YUVFLAG] = { 0 };
		g_camera->attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		g_camera->attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		g_camera->attr[CELL_CAMERA_422FLAG] = { 1 };
		g_camera->attr[CELL_CAMERA_USBLOAD] = { 4 };
		break;
	}

	case fake_camera_type::eyetoy2:
	{
		g_camera->attr[CELL_CAMERA_SATURATION] = { 64 };
		g_camera->attr[CELL_CAMERA_BRIGHTNESS] = { 8 };
		g_camera->attr[CELL_CAMERA_AEC] = { 1 };
		g_camera->attr[CELL_CAMERA_AGC] = { 1 };
		g_camera->attr[CELL_CAMERA_AWB] = { 1 };
		g_camera->attr[CELL_CAMERA_LED] = { 1 };
		g_camera->attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		g_camera->attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		g_camera->attr[CELL_CAMERA_GAMMA] = { 1 };
		g_camera->attr[CELL_CAMERA_AGCLIMIT] = { 4 };
		g_camera->attr[CELL_CAMERA_DENOISE] = { 0 };
		g_camera->attr[CELL_CAMERA_FRAMERATEADJUST] = { 0 };
		g_camera->attr[CELL_CAMERA_PIXELOUTLIERFILTER] = { 1 };
		g_camera->attr[CELL_CAMERA_AGCLOW] = { 48 };
		g_camera->attr[CELL_CAMERA_AGCHIGH] = { 64 };
		break;
	}
	default:
		cellCamera.todo("Trying to init cellCamera with un-researched camera type.");
	}

	// TODO: Some other default attributes? Need to check the actual behaviour on a real PS3.

	if (g_cfg.io.camera == camera_handler::fake)
	{
		g_camera->is_attached = true;
	}

	g_camera->init = 1;
	return CELL_OK;
}

error_code cellCameraEnd()
{
	cellCamera.todo("cellCameraEnd()");

	const auto g_camera = g_fxo->get<camera_thread>();

	std::lock_guard lock(g_camera->mutex);

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	// TODO: My tests hinted to this behavior, but I'm not sure, so I'll leave this commented
	//if (auto res = cellCameraClose(0))
	//{
	//	return res;
	//}

	// TODO
	g_camera->init = 0;
	g_camera->reset_state();
	return CELL_OK;
}

error_code cellCameraOpen() // seems unused
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraOpenAsync()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraOpenEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraOpenEx(dev_num=%d, type=*0x%x)", dev_num, info);

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

	const auto g_camera = g_fxo->get<camera_thread>();
	// we know g_camera is valid here (cellCameraSetAttribute above checks for it)

	if (g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_ALREADY_OPEN;
	}

	if (auto res = check_camera_info(*info))
	{
		return res;
	}

	// calls cellCameraGetAttribute(dev_num, CELL_CAMERA_PBUFFER) at some point

	const auto vbuf_size = get_video_buffer_size(*info);

	std::lock_guard lock(g_camera->mutex);

	if (info->read_mode != CELL_CAMERA_READ_DIRECT && !info->buffer)
	{
		info->buffer = vm::cast(vm::alloc(vbuf_size, vm::memory_location_t::main));
		info->bytesize = vbuf_size;
	}

	std::tie(info->width, info->height) = get_video_resolution(*info);

	g_camera->is_open = true;
	g_camera->info = *info;

	return CELL_OK;
}

error_code cellCameraOpenPost()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraClose(s32 dev_num)
{
	cellCamera.todo("cellCameraClose(dev_num=%d)", dev_num);

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	std::lock_guard lock(g_camera->mutex);

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	vm::dealloc(g_camera->info.buffer.addr(), vm::memory_location_t::main);
	g_camera->is_open = false;

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
	cellCamera.todo("cellCameraGetDeviceGUID(dev_num=%d, guid=*0x%x)", dev_num, guid);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	// Does not check params or is_open (maybe attached?)

	*guid = 0; // apparently always 0

	return CELL_OK;
}

error_code cellCameraGetType(s32 dev_num, vm::ptr<s32> type)
{
	cellCamera.todo("cellCameraGetType(dev_num=%d, type=*0x%x)", dev_num, type);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	if (!check_dev_num(dev_num) || !type )
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (!g_camera->is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	switch (g_cfg.io.camera_type)
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
	cellCamera.warning("cellCameraIsAvailable(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	return true;
}

s32 cellCameraIsAttached(s32 dev_num)
{
	cellCamera.warning("cellCameraIsAttached(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	std::lock_guard lock(g_camera->mutex);

	bool is_attached = g_camera->is_attached;

	if (g_cfg.io.camera == camera_handler::fake)
	{
		// "attach" camera here
		// normally should be attached immediately after event queue is registered, but just to be sure
		if (!is_attached)
		{
			g_camera->send_attach_state(true);
			is_attached = g_camera->is_attached;
		}
	}

	return is_attached;
}

s32 cellCameraIsOpen(s32 dev_num)
{
	cellCamera.warning("cellCameraIsOpen(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	std::lock_guard lock(g_camera->mutex);

	return g_camera->is_open;
}

s32 cellCameraIsStarted(s32 dev_num)
{
	cellCamera.warning("cellCameraIsStarted(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return false;
	}

	if (!check_dev_num(dev_num))
	{
		return false;
	}

	std::lock_guard lock(g_camera->mutex);

	return g_camera->is_streaming;
}

error_code cellCameraGetAttribute(s32 dev_num, s32 attrib, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	cellCamera.todo("cellCameraGetAttribute(dev_num=%d, attrib=%d=%s, arg1=*0x%x, arg2=*0x%x)", dev_num, attrib, attr_name, arg1, arg2);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	if (!check_dev_num(dev_num) || !attr_name || !arg1) // invalid attributes don't have a name and at least arg1 should not be NULL
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	// actually compares <= 0x63 which is equivalent
	if (attrib < CELL_CAMERA_FORMATCAP && !g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	std::lock_guard lock(g_camera->mutex);

	if (!g_camera->is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (arg1)
	{
		*arg1 = g_camera->attr[attrib].v1;
	}
	if (arg2)
	{
		*arg2 = g_camera->attr[attrib].v2;
	}

	return CELL_OK;
}

error_code cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	cellCamera.todo("cellCameraSetAttribute(dev_num=%d, attrib=%d=%s, arg1=%d, arg2=%d)", dev_num, attrib, attr_name, arg1, arg2);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	if (!check_dev_num(dev_num) || !attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	// actually compares <= 0x63 which is equivalent
	if (attrib < CELL_CAMERA_FORMATCAP && !g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	g_camera->set_attr(attrib, arg1, arg2);

	return CELL_OK;
}

error_code cellCameraResetAttribute()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraGetBufferSize(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraGetBufferSize(dev_num=%d, info=*0x%x)", dev_num, info);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_DEVICE_NOT_FOUND);
	}

	// the next few checks have a strange order, if I can trust the tests

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	if (g_camera->is_open)
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

	std::lock_guard lock(g_camera->mutex);

	info->bytesize = get_video_buffer_size(g_camera->info);
	g_camera->info = *info;

	return info->bytesize;
}

error_code cellCameraGetBufferInfo()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	// called by cellCameraGetBufferInfoEx

	return CELL_OK;
}

error_code cellCameraGetBufferInfoEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraGetBufferInfoEx(dev_num=%d, read=0x%x)", dev_num, info);

	// the following should be moved to cellCameraGetBufferInfo

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
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

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!info)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	std::lock_guard lock(g_camera->mutex);
	*info = g_camera->info;

	return CELL_OK;
}

error_code cellCameraPrepExtensionUnit(s32 dev_num, vm::ptr<u8> guidExtensionCode)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

error_code cellCameraCtrlExtensionUnit(s32 dev_num, u8 request, u16 value, u16 length, vm::ptr<u8> data)
{
	UNIMPLEMENTED_FUNC(cellCamera);
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

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!g_camera->is_attached)
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
	cellCamera.todo("cellCameraStart(dev_num=%d)", dev_num);

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	std::lock_guard lock(g_camera->mutex);

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!g_camera->is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	g_camera->timer.Start();
	g_camera->is_streaming = true;

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
	cellCamera.todo("cellCameraRead(dev_num=%d, frame_num=*0x%x, bytes_read=*0x%x)", dev_num, frame_num, bytes_read);

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
	cellCamera.todo("cellCameraReadEx(dev_num=%d, read=0x%x)", dev_num, read);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
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

	std::lock_guard lock(g_camera->mutex);

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!g_camera->is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!g_camera->is_streaming)
	{
		return CELL_CAMERA_ERROR_NOT_STARTED;
	}

	// can call cellCameraReset() and cellCameraStop() in some cases

	if (read) // NULL returns CELL_OK
	{
		read->timestamp = g_camera->timer.GetElapsedTimeInMicroSec();
		read->frame = g_camera->frame_num;
		read->bytesread = g_camera->is_streaming ? get_video_buffer_size(g_camera->info) : 0;

		auto shared_data = g_fxo->get<gem_camera_shared>();

		shared_data->frame_timestamp.exchange(read->timestamp);
	}

	return CELL_OK;
}

error_code cellCameraReadComplete(s32 dev_num, u32 bufnum, u32 arg2)
{
	cellCamera.todo("cellCameraReadComplete(dev_num=%d, bufnum=%d, arg2=%d)", dev_num, bufnum, arg2);

	return cellCameraSetAttribute(dev_num, CELL_CAMERA_READFINISH, bufnum, arg2);
}

error_code cellCameraStop(s32 dev_num)
{
	cellCamera.todo("cellCameraStop(dev_num=%d)", dev_num);

	if (!check_dev_num(dev_num))
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return not_an_error(CELL_CAMERA_ERROR_NOT_OPEN);
	}

	if (!g_camera->is_open)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	if (!g_camera->is_attached)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (!g_camera->is_streaming)
	{
		return CELL_CAMERA_ERROR_NOT_STARTED;
	}

	g_camera->is_streaming = false;

	std::lock_guard lock(g_camera->mutex);
	g_camera->timer.Stop();

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
	cellCamera.todo("cellCameraSetNotifyEventQueue(key=0x%x)", key);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	g_camera->add_queue(key, 0, 0);

	return CELL_OK;
}

error_code cellCameraRemoveNotifyEventQueue(u64 key)
{
	cellCamera.todo("cellCameraRemoveNotifyEventQueue(key=0x%x)", key);

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	g_camera->remove_queue(key);

	return CELL_OK;
}

error_code cellCameraSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	cellCamera.todo("cellCameraSetNotifyEventQueue2(key=0x%x, source=%d, flag=%d)", key, source, flag);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	const auto g_camera = g_fxo->get<camera_thread>();

	if (!g_camera->init)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	g_camera->add_queue(key, source, flag);

	return CELL_OK;
}

error_code cellCameraRemoveNotifyEventQueue2(u64 key)
{
	cellCamera.todo("cellCameraRemoveNotifyEventQueue2(key=0x%x)", key);

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
		const s32 fps = info.framerate;

		if (!fps || Emu.IsPaused() || g_cfg.io.camera == camera_handler::null)
		{
			thread_ctrl::wait_for(1000); // hack
			continue;
		}

		const u64 frame_start = get_guest_system_time();

		std::unique_lock lock(mutex_notify_data_map);

		for (auto const& notify_data_entry : notify_data_map)
		{
			const auto& key = notify_data_entry.first;
			const auto& evt_data = notify_data_entry.second;

			// handle FRAME_UPDATE
			if (is_streaming && evt_data.flag & CELL_CAMERA_EFLAG_FRAME_UPDATE)
			{
				if (auto queue = lv2_event_queue::find(key))
				{
					u64 data2 = 0;
					u64 data3 = 0;

					if (read_mode.load() == CELL_CAMERA_READ_DIRECT)
					{
						const u64 image_data_size = static_cast<u64>(info.bytesize);
						const u64 buffer_number = 0;
						const u64 camera_id = 0;

						data2 = image_data_size << 32 | buffer_number << 16 | camera_id;
						data3 = timer.GetElapsedTimeInMicroSec();	// timestamp
					}
					else // CELL_CAMERA_READ_FUNCCALL, also default
					{
						data2 = 0;	// device id (always 0)
						data3 = 0;	// unused
					}

					if (queue->send(evt_data.source, CELL_CAMERA_FRAME_UPDATE, data2, data3)) [[likely]]
					{
						++frame_num;
					}
				}
			}
		}

		lock.unlock();

		for (const u64 frame_target_time = 1000000u / fps;;)
		{
			const u64 time_passed = get_guest_system_time() - frame_start;
			if (time_passed >= frame_target_time)
				break;

			lv2_obj::wait_timeout(frame_target_time - time_passed);
		}
	}
}

void camera_context::reset_state()
{
	read_mode = CELL_CAMERA_READ_FUNCCALL;
	is_streaming = false;
	is_attached = false;
	is_open = false;
	info.framerate = 0;
	std::memset(&attr, 0, sizeof(attr));

	std::scoped_lock lock(mutex_notify_data_map);
	notify_data_map.clear();
}

void camera_context::send_attach_state(bool attached)
{
	std::lock_guard lock(mutex_notify_data_map);

	if (!notify_data_map.empty())
	{
		for (auto const& notify_data_entry : notify_data_map)
		{
			const auto& key = notify_data_entry.first;
			const auto& evt_data = notify_data_entry.second;

			if (auto queue = lv2_event_queue::find(key))
			{
				if (queue->send(evt_data.source, attached ? CELL_CAMERA_ATTACH : CELL_CAMERA_DETACH, 0, 0)) [[likely]]
				{
					is_attached = attached;
				}
			}
		}
	}
	else
	{
		// We're not expected to send any events for attaching/detaching
		is_attached = attached;
	}
}

void camera_context::set_attr(s32 attrib, u32 arg1, u32 arg2)
{
	if (attrib == CELL_CAMERA_READMODE)
	{
		if (arg1 != CELL_CAMERA_READ_FUNCCALL && arg1 != CELL_CAMERA_READ_DIRECT)
		{
			cellCamera.warning("Unknown read mode set: %d", arg1);
			arg1 = CELL_CAMERA_READ_FUNCCALL;
		}
		read_mode.exchange(arg1);
	}

	std::lock_guard lock(mutex);
	attr[attrib] = {arg1, arg2};
}

void camera_context::add_queue(u64 key, u64 source, u64 flag)
{
	std::lock_guard lock(mutex);
	{
		std::lock_guard lock_data_map(mutex_notify_data_map);

		notify_data_map[key] = { source, flag };
	}

	// send ATTACH event - HACKY
	send_attach_state(true);
}

void camera_context::remove_queue(u64 key)
{
	std::lock_guard lock(mutex);
	{
		std::lock_guard lock_data_map(mutex_notify_data_map);

		notify_data_map.erase(key);
	}
}
