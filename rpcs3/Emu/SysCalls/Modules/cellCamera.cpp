#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellCamera.h"

extern Module cellCamera;

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

// Custom struct to keep track of cameras
struct camera_t
{
	struct attr_t
	{
		u32 v1, v2;
	};

	attr_t attr[500]{};
};

s32 cellCameraInit()
{
	cellCamera.Warning("cellCameraInit()");

	if (Ini.Camera.GetValue() == 0)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	const auto camera = idm::make_fixed<camera_t>();

	if (!camera)
	{
		return CELL_CAMERA_ERROR_ALREADY_INIT;
	}

	if (Ini.CameraType.GetValue() == 1)
	{
		camera->attr[CELL_CAMERA_SATURATION] = { 164 };
		camera->attr[CELL_CAMERA_BRIGHTNESS] = { 96 };
		camera->attr[CELL_CAMERA_AEC] = { 1 };
		camera->attr[CELL_CAMERA_AGC] = { 1 };
		camera->attr[CELL_CAMERA_AWB] = { 1 };
		camera->attr[CELL_CAMERA_ABC] = { 0 };
		camera->attr[CELL_CAMERA_LED] = { 1 };
		camera->attr[CELL_CAMERA_QS] = { 0 };
		camera->attr[CELL_CAMERA_NONZEROCOEFFS] = { 32, 32 };
		camera->attr[CELL_CAMERA_YUVFLAG] = { 0 };
		camera->attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		camera->attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		camera->attr[CELL_CAMERA_422FLAG] = { 1 };
		camera->attr[CELL_CAMERA_USBLOAD] = { 4 };
	}
	else if (Ini.CameraType.GetValue() == 2)
	{
		camera->attr[CELL_CAMERA_SATURATION] = { 64 };
		camera->attr[CELL_CAMERA_BRIGHTNESS] = { 8 };
		camera->attr[CELL_CAMERA_AEC] = { 1 };
		camera->attr[CELL_CAMERA_AGC] = { 1 };
		camera->attr[CELL_CAMERA_AWB] = { 1 };
		camera->attr[CELL_CAMERA_LED] = { 1 };
		camera->attr[CELL_CAMERA_BACKLIGHTCOMP] = { 0 };
		camera->attr[CELL_CAMERA_MIRRORFLAG] = { 1 };
		camera->attr[CELL_CAMERA_GAMMA] = { 1 };
		camera->attr[CELL_CAMERA_AGCLIMIT] = { 4 };
		camera->attr[CELL_CAMERA_DENOISE] = { 0 };
		camera->attr[CELL_CAMERA_FRAMERATEADJUST] = { 0 };
		camera->attr[CELL_CAMERA_PIXELOUTLIERFILTER] = { 1 };
		camera->attr[CELL_CAMERA_AGCLOW] = { 48 };
		camera->attr[CELL_CAMERA_AGCHIGH] = { 64 };
	}
	// TODO: Some other default attributes? Need to check the actual behaviour on a real PS3.

	return CELL_OK;
}

s32 cellCameraEnd()
{
	cellCamera.Warning("cellCameraEnd()");

	if (!idm::remove_fixed<camera_t>())
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellCameraOpen()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraOpenEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraClose(s32 dev_num)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetDeviceGUID(s32 dev_num, vm::ptr<u32> guid)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetType(s32 dev_num, vm::ptr<s32> type)
{
	cellCamera.Warning("cellCameraGetType(dev_num=%d, type=*0x%x)", dev_num, type);

	const auto camera = idm::get_fixed<camera_t>();

	if (!camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	switch (Ini.CameraType.GetValue())
	{
	case 1: *type = CELL_CAMERA_EYETOY; break;
	case 2: *type = CELL_CAMERA_EYETOY2; break;
	case 3: *type = CELL_CAMERA_USBVIDEOCLASS; break;
	default: *type = CELL_CAMERA_TYPE_UNKNOWN; break;
	}

	return CELL_OK;
}

s32 cellCameraIsAvailable(s32 dev_num)
{
	cellCamera.Todo("cellCameraIsAvailable(dev_num=%d)", dev_num);
	return CELL_OK;
}

s32 cellCameraIsAttached(s32 dev_num)
{
	cellCamera.Warning("cellCameraIsAttached(dev_num=%d)", dev_num);

	if (Ini.Camera.GetValue() == 1)
	{
		return 1;
	}

	return CELL_OK; // CELL_OK means that no camera is attached
}

s32 cellCameraIsOpen(s32 dev_num)
{
	cellCamera.Todo("cellCameraIsOpen(dev_num=%d)", dev_num);
	return CELL_OK;
}

s32 cellCameraIsStarted(s32 dev_num)
{
	cellCamera.Todo("cellCameraIsStarted(dev_num=%d)", dev_num);
	return CELL_OK;
}

s32 cellCameraGetAttribute(s32 dev_num, s32 attrib, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	cellCamera.Warning("cellCameraGetAttribute(dev_num=%d, attrib=%d, arg1=*0x%x, arg2=*0x%x)", dev_num, attrib, arg1, arg2);

	const auto attr_name = get_camera_attr_name(attrib);

	const auto camera = idm::get_fixed<camera_t>();

	if (!camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	*arg1 = camera->attr[attrib].v1;
	*arg2 = camera->attr[attrib].v2;

	return CELL_OK;
}

s32 cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2)
{
	cellCamera.Warning("cellCameraSetAttribute(dev_num=%d, attrib=%d, arg1=%d, arg2=%d)", dev_num, attrib, arg1, arg2);

	const auto attr_name = get_camera_attr_name(attrib);

	const auto camera = idm::get_fixed<camera_t>();

	if (!camera)
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	camera->attr[attrib] = { arg1, arg2 };

	return CELL_OK;
}

s32 cellCameraGetBufferSize(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetBufferInfo()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraGetBufferInfoEx(s32 dev_num, vm::ptr<CellCameraInfoEx> info)
{
	UNIMPLEMENTED_FUNC(cellCamera);
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
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraStart(s32 dev_num)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraRead(s32 dev_num, vm::ptr<u32> frame_num, vm::ptr<u32> bytes_read)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraReadEx(s32 dev_num, vm::ptr<CellCameraReadEx> read)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraReadComplete(s32 dev_num, u32 bufnum, u32 arg2)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraStop(s32 dev_num)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraSetNotifyEventQueue(u64 key)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraRemoveNotifyEventQueue(u64 key)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

s32 cellCameraRemoveNotifyEventQueue2(u64 key)
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

Module cellCamera("cellCamera", []()
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
