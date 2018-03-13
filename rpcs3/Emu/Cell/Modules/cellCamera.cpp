#include "stdafx.h"
#include "cellCamera.h"

#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/IdManager.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/System.h"

#include <thread>

logs::channel cellCamera("cellCamera");

// **************
// * Prototypes *
// **************

s32 cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2);

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

/**
 * \brief Sets up notify event queue supplied and immediately sends an ATTACH event to it
 * \param key Event queue key to add
 * \param source Event source port
 * \param flag Event flag (CELL_CAMERA_EFLAG_*)
 * \return True on success, false if camera_thead hasn't been initialized
 */
bool add_queue_and_send_attach(u64 key, u64 source, u64 flag)
{
	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return false;
	}

	semaphore_lock lock(g_camera->mutex);
	{
		semaphore_lock lock_data_map(g_camera->mutex_notify_data_map);

		g_camera->notify_data_map[key] = { source, flag };
	}

	// send ATTACH event - HACKY
	g_camera->send_attach_state(true);
	return true;
}

/**
 * \brief Unsets/removes event queue specified
 * \param key Event queue key to remove
 * \return True on success, false if camera_thead hasn't been initialized
 */
bool remove_queue(u64 key)
{
	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return false;
	}

	semaphore_lock lock(g_camera->mutex);
	{
		semaphore_lock lock_data_map(g_camera->mutex_notify_data_map);

		g_camera->notify_data_map.erase(key);
	}
	return true;
}

/**
 * \brief Sets read mode attribute (used for deciding how image data is passed to games)
 *		  Also sends it to the camera thread
 *	      NOTE: thread-safe (uses camera_thread::mutex)
 * \param dev_num Device number (always 0)
 * \param read_mode Either CELL_CAMERA_READ_FUNCCALL or CELL_CAMERA_READ_DIRECT
 * \return CELL error code or CELL_OK
 */
u32 set_and_send_read_mode(s32 dev_num, const s32 read_mode)
{
	if (read_mode == CELL_CAMERA_READ_FUNCCALL ||
		read_mode == CELL_CAMERA_READ_DIRECT)
	{
		if (const auto status = cellCameraSetAttribute(dev_num, CELL_CAMERA_READMODE, read_mode, 0))
		{
			return status;
		}
	}
	else
	{
		cellCamera.error("Unknown read mode set: %d", read_mode);
	}

	// Send read mode to camera thread
	const auto g_camera = fxm::get<camera_thread>();

	g_camera->read_mode.exchange(read_mode);

	return CELL_OK;
}

std::pair<u32, u32> get_video_resolution(const CellCameraInfoEx& info)
{
	std::pair<u32, u32> res;
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

s32 cellCameraInit()
{
	cellCamera.todo("cellCameraInit()");

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	// Start camera thread
	const auto g_camera = fxm::make<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_ALREADY_INIT;
	}

	semaphore_lock lock(g_camera->mutex);

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

	return CELL_OK;
}

s32 cellCameraEnd()
{
	cellCamera.todo("cellCameraEnd()");

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (!fxm::remove<camera_thread>())
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellCameraOpen() // seems unused
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraOpenEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraOpenEx(dev_num=%d, type=*0x%x)", dev_num, info);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	s32 read_mode = info->read_mode;

	u32 status = set_and_send_read_mode(dev_num, read_mode);
	if (status != CELL_OK)
	{
		return status;
	}

	status = cellCameraSetAttribute(dev_num, CELL_CAMERA_GAMEPID, status, 0); // yup, that's what libGem does
	if (status != CELL_OK)
	{
		return status;
	}

	const auto vbuf_size = get_video_buffer_size(*info);

	if (info->read_mode == CELL_CAMERA_READ_FUNCCALL && !info->buffer)
	{
		info->buffer = vm::cast(vm::alloc(vbuf_size, vm::memory_location_t::main));
		info->bytesize = vbuf_size;
	}

	std::tie(info->width, info->height) = get_video_resolution(*info);

	semaphore_lock lock(g_camera->mutex);

	g_camera->is_open = true;
	g_camera->info = *info;

	return CELL_OK;
}

s32 cellCameraClose(s32 dev_num)
{
	cellCamera.todo("cellCameraClose(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	semaphore_lock lock(g_camera->mutex);

	vm::dealloc(g_camera->info.buffer.addr(), vm::memory_location_t::main);
	g_camera->is_open = false;

	return CELL_OK;
}

s32 cellCameraGetDeviceGUID(s32 dev_num, vm::ptr<u32> guid)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetType(s32 dev_num, vm::ptr<s32> type)
{
	cellCamera.todo("cellCameraGetType(dev_num=%d, type=*0x%x)", dev_num, type);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!check_dev_num(dev_num) || !type )
	{
		return CELL_CAMERA_ERROR_PARAM;
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
	cellCamera.todo("cellCameraIsAvailable(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera || !check_dev_num(dev_num))
	{
		return false;
	}

	return true;
}

s32 cellCameraIsAttached(s32 dev_num)
{
	cellCamera.todo("cellCameraIsAttached(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return false;
	}

	semaphore_lock lock(g_camera->mutex);

	bool is_attached = g_camera->is_attached;

	// "attach" camera here
	// normally should be attached immediately after event queue is registered, but just to be sure
	if (!is_attached)
	{
		g_camera->send_attach_state(true);
		is_attached = g_camera->is_attached;
	}

	return is_attached;
}

s32 cellCameraIsOpen(s32 dev_num)
{
	cellCamera.todo("cellCameraIsOpen(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = fxm::get<camera_thread>();

	semaphore_lock lock(g_camera->mutex);

	return g_camera->is_open;
}

s32 cellCameraIsStarted(s32 dev_num)
{
	cellCamera.todo("cellCameraIsStarted(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return false;
	}

	const auto g_camera = fxm::get<camera_thread>();

	return g_camera->is_streaming;
}

s32 cellCameraGetAttribute(s32 dev_num, s32 attrib, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	cellCamera.todo("cellCameraGetAttribute: get attrib %s to: 0x%x - 0x%x)", attr_name ? attr_name : "(invalid)", arg1, arg2);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	cellCamera.todo("cellCameraGetAttribute: get attrib %s arg1: %d arg2: %d", attr_name, arg1, arg2);

	semaphore_lock lock(g_camera->mutex);

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

s32 cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2)
{
	const auto attr_name = get_camera_attr_name(attrib);
	cellCamera.todo("cellCameraSetAttribute: set attrib %s to: %d - %d)", attr_name ? attr_name : "(invalid)", arg1, arg2);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	semaphore_lock lock(g_camera->mutex);
	g_camera->attr[attrib] = { arg1, arg2 };

	return CELL_OK;
}

s32 cellCameraGetBufferSize(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraGetBufferSize(dev_num=%d, info=*0x%x)", dev_num, info);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	// TODO: a bunch of arg checks. here's one
	if (!cellCameraIsAttached(dev_num))
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto read_mode = info->read_mode;

	u32 status = set_and_send_read_mode(dev_num, read_mode);
	if (status != CELL_OK)
	{
		return status;
	}

	semaphore_lock lock(g_camera->mutex);

	info->bytesize = get_video_buffer_size(g_camera->info);

	g_camera->info = *info;

	return info->bytesize;
}

s32 cellCameraGetBufferInfo()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetBufferInfoEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	cellCamera.todo("cellCameraGetBufferInfoEx(dev_num=%d, read=0x%x)", dev_num, info);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!info)
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	semaphore_lock lock(g_camera->mutex);

	*info = g_camera->info;

	return CELL_OK;
}

s32 cellCameraPrepExtensionUnit(s32 dev_num, vm::ptr<u8> guidExtensionCode)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraCtrlExtensionUnit(s32 dev_num, u8 request, u16 value, u16 length, vm::ptr<u8> data)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetExtensionUnit(s32 dev_num, u16 value, u16 length, vm::ptr<u8> data)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraSetExtensionUnit(s32 dev_num, u16 value, u16 length, vm::ptr<u8> data)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraReset(s32 dev_num)
{
	cellCamera.todo("cellCameraReset(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	return CELL_OK;
}

s32 cellCameraStart(s32 dev_num)
{
	cellCamera.todo("cellCameraStart(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	semaphore_lock lock(g_camera->mutex);

	g_camera->timer.Start();
	g_camera->is_streaming = true;

	return CELL_OK;
}

s32 cellCameraRead(s32 dev_num, vm::ptr<u32> frame_num, vm::ptr<u32> bytes_read)
{
	cellCamera.todo("cellCameraRead(dev_num=%d, frame_num=*0x%x, bytes_read=*0x%x)", dev_num, frame_num, bytes_read);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	return CELL_OK;
}

s32 cellCameraReadEx(s32 dev_num, vm::ptr<CellCameraReadEx> read)
{
	cellCamera.todo("cellCameraReadEx(dev_num=%d, read=0x%x)", dev_num, read);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	semaphore_lock lock(g_camera->mutex);

	read->timestamp = g_camera->timer.GetElapsedTimeInMicroSec();
	read->frame = g_camera->frame_num;
	read->bytesread = g_camera->is_streaming ?
		get_video_buffer_size(g_camera->info) : 0;

	auto shared_data = fxm::get_always<gem_camera_shared>();

	shared_data->frame_timestamp.exchange(read->timestamp);

	return CELL_OK;
}

s32 cellCameraReadComplete(s32 dev_num, u32 bufnum, u32 arg2)
{
	cellCamera.todo("cellCameraReadComplete(dev_num=%d, bufnum=%d, arg2=%d)", dev_num, bufnum, arg2);

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return cellCameraSetAttribute(dev_num, CELL_CAMERA_READFINISH, bufnum, arg2);
}

s32 cellCameraStop(s32 dev_num)
{
	cellCamera.todo("cellCameraStop(dev_num=%d)", dev_num);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_CAMERA_ERROR_NOT_OPEN;
	}

	const auto g_camera = fxm::get<camera_thread>();

	if (!g_camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	semaphore_lock lock(g_camera->mutex);

	g_camera->is_streaming = false;
	g_camera->timer.Stop();

	return CELL_OK;
}

s32 cellCameraSetNotifyEventQueue(u64 key)
{
	cellCamera.todo("cellCameraSetNotifyEventQueue(key=0x%x)", key);

	if (!add_queue_and_send_attach(key, 0, 0))
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellCameraRemoveNotifyEventQueue(u64 key)
{
	cellCamera.todo("cellCameraRemoveNotifyEventQueue(key=0x%x)", key);

	if (!remove_queue(key))
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellCameraSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	cellCamera.todo("cellCameraSetNotifyEventQueue2(key=0x%x, source=%d, flag=%d)", key, source, flag);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (!add_queue_and_send_attach(key, source, flag))
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellCameraRemoveNotifyEventQueue2(u64 key)
{
	cellCamera.todo("cellCameraRemoveNotifyEventQueue2(key=0x%x)", key);

	if (g_cfg.io.camera == camera_handler::null)
	{
		return CELL_OK;
	}

	if (!remove_queue(key))
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellCamera)("cellCamera", []()
{
	REG_FUNC(cellCamera, cellCameraInit);
	REG_FUNC(cellCamera, cellCameraEnd);
	REG_FUNC(cellCamera, cellCameraOpen); // was "renamed", but exists
	REG_FUNC(cellCamera, cellCameraOpenEx);
	REG_FUNC(cellCamera, cellCameraClose);

	REG_FUNC(cellCamera, cellCameraGetDeviceGUID);
	REG_FUNC(cellCamera, cellCameraGetType);
	REG_FUNC(cellCamera, cellCameraIsAvailable);
	REG_FUNC(cellCamera, cellCameraIsAttached);
	REG_FUNC(cellCamera, cellCameraIsOpen);
	REG_FUNC(cellCamera, cellCameraIsStarted);
	REG_FUNC(cellCamera, cellCameraGetAttribute);
	REG_FUNC(cellCamera, cellCameraSetAttribute);
	REG_FUNC(cellCamera, cellCameraGetBufferSize);
	REG_FUNC(cellCamera, cellCameraGetBufferInfo); // was "renamed", but exists
	REG_FUNC(cellCamera, cellCameraGetBufferInfoEx);

	REG_FUNC(cellCamera, cellCameraPrepExtensionUnit);
	REG_FUNC(cellCamera, cellCameraCtrlExtensionUnit);
	REG_FUNC(cellCamera, cellCameraGetExtensionUnit);
	REG_FUNC(cellCamera, cellCameraSetExtensionUnit);

	REG_FUNC(cellCamera, cellCameraReset);
	REG_FUNC(cellCamera, cellCameraStart);
	REG_FUNC(cellCamera, cellCameraRead);
	REG_FUNC(cellCamera, cellCameraReadEx);
	REG_FUNC(cellCamera, cellCameraReadComplete);
	REG_FUNC(cellCamera, cellCameraStop);

	REG_FUNC(cellCamera, cellCameraSetNotifyEventQueue);
	REG_FUNC(cellCamera, cellCameraRemoveNotifyEventQueue);
	REG_FUNC(cellCamera, cellCameraSetNotifyEventQueue2);
	REG_FUNC(cellCamera, cellCameraRemoveNotifyEventQueue2);
});

void camera_thread::on_task()
{
	while (fxm::check<camera_thread>() && !Emu.IsStopped())
	{
		std::chrono::steady_clock::time_point frame_start = std::chrono::steady_clock::now();

		if (Emu.IsPaused())
		{
			std::this_thread::sleep_for(1ms); // hack
			continue;
		}

		semaphore_lock lock(mutex_notify_data_map);

		for (auto const& notify_data_entry : notify_data_map)
		{
			const auto& key = notify_data_entry.first;
			const auto& evt_data = notify_data_entry.second;

			// handle FRAME_UPDATE
			if (is_streaming &&
				evt_data.flag & CELL_CAMERA_EFLAG_FRAME_UPDATE &&
				info.framerate != 0)
			{
				if (auto queue = lv2_event_queue::find(key))
				{
					u64 data2{ 0 };
					u64 data3{ 0 };

					switch (read_mode.load())
					{
					case CELL_CAMERA_READ_FUNCCALL:
					{
						data2 = 0;	// device id (always 0)
						data3 = 0;	// unused
						break;
					}
					case CELL_CAMERA_READ_DIRECT:
					{
						const u64 image_data_size = static_cast<u64>(info.bytesize);
						const u64 buffer_number = 0;
						const u64 camera_id = 0;

						data2 = image_data_size << 32 | buffer_number << 16 | camera_id;
						data3 = timer.GetElapsedTimeInMicroSec();	// timestamp
						break;
					}
					default:
					{
						cellCamera.error("Unknown read mode set: %d. This should never happen.", read_mode.load());
						return;
					}
					}

					const auto send_status = queue->send(evt_data.source, CELL_CAMERA_FRAME_UPDATE, data2, data3);
					if (LIKELY(send_status))
					{
						++frame_num;
					}
				}
			}

		}

		const std::chrono::microseconds frame_target_time{ static_cast<u32>(1000000.0 / info.framerate) };

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		std::chrono::microseconds frame_processing_time = std::chrono::duration_cast<std::chrono::microseconds>(now - frame_start);

		if (frame_processing_time < frame_target_time)
		{
			std::chrono::microseconds frame_idle_time = frame_target_time - frame_processing_time;
			std::this_thread::sleep_for(frame_idle_time);
		}
	}
}

void camera_thread::on_init(const std::shared_ptr<void>& _this)
{
	named_thread::on_init(_this);
}

void camera_thread::send_attach_state(bool attached)
{
	semaphore_lock lock(mutex_notify_data_map);

	if (!notify_data_map.empty())
	{
		for (auto const& notify_data_entry : notify_data_map)
		{
			const auto& key = notify_data_entry.first;
			const auto& evt_data = notify_data_entry.second;

			if (auto queue = lv2_event_queue::find(key))
			{
				const auto send_result = queue->send(evt_data.source, attached ? CELL_CAMERA_ATTACH : CELL_CAMERA_DETACH, 0, 0);
				if (LIKELY(send_result))
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
