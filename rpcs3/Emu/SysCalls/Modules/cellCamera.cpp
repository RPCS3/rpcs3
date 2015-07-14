#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellCamera.h"

extern Module cellCamera;

std::unique_ptr<camera_t> g_camera;

s32 cellCameraInit()
{
	cellCamera.Warning("cellCameraInit()");

	if (Ini.Camera.GetValue() == 0)
	{
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;
	}

	if (g_camera->init.exchange(true))
	{
		return CELL_CAMERA_ERROR_ALREADY_INIT;
	}

	if (Ini.CameraType.GetValue() == 1)
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
	}
	else if (Ini.CameraType.GetValue() == 2)
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
	}
	// TODO: Some other default attributes? Need to check the actual behaviour on a real PS3.

	return CELL_OK;
}

s32 cellCameraEnd()
{
	cellCamera.Warning("cellCameraEnd()");

	if (!g_camera->init.exchange(false))
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

	if (!g_camera->init.load())
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

	const auto attr_name = camera_t::get_attr_name(attrib);

	if (!g_camera->init.load())
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	*arg1 = g_camera->attr[attrib].v1;
	*arg2 = g_camera->attr[attrib].v2;

	return CELL_OK;
}

s32 cellCameraSetAttribute(s32 dev_num, s32 attrib, u32 arg1, u32 arg2)
{
	cellCamera.Warning("cellCameraSetAttribute(dev_num=%d, attrib=%d, arg1=%d, arg2=%d)", dev_num, attrib, arg1, arg2);

	const auto attr_name = camera_t::get_attr_name(attrib);

	if (!g_camera->init.load())
	{
		return CELL_CAMERA_ERROR_NOT_INIT;
	}

	if (!attr_name) // invalid attributes don't have a name
	{
		return CELL_CAMERA_ERROR_PARAM;
	}

	g_camera->attr[attrib] = { arg1, arg2 };

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
	g_camera = std::make_unique<camera_t>();

	REG_FUNC(cellCamera, cellCameraInit);
	REG_FUNC(cellCamera, cellCameraEnd);
	REG_FUNC(cellCamera, cellCameraOpen);
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
	REG_FUNC(cellCamera, cellCameraGetBufferInfo);
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
