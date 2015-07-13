#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellCamera.h"

extern Module cellCamera;
const char* attributes[] = {"GAIN", "REDBLUEGAIN", "SATURATION", "EXPOSURE", "BRIGHTNESS", "AEC", "AGC", "AWB", "ABC", "LED", "AUDIOGAIN", "QS", "NONZEROCOEFFS", "YUVFLAG",
                            "JPEGFLAG", "BACKLIGHTCOMP", "MIRRORFLAG", "MEASUREDQS", "422FLAG", "USBLOAD", "GAMMA", "GREENGAIN", "AGCLIMIT", "DENOISE", "FRAMERATEADJUST",
                            "PIXELOUTLIERFILTER", "AGCLOW", "AGCHIGH", "DEVICELOCATION", "FORMATCAP", "FORMATINDEX", "NUMFRAME", "FRAMEINDEX", "FRAMESIZE", "INTERVALTYPE",
                            "INTERVALINDEX", "INTERVALVALUE", "COLORMATCHING", "PLFREQ", "DEVICEID", "DEVICECAP", "DEVICESPEED", "UVCREQCODE", "UVCREQDATA", "DEVICEID2",
                            "READMODE", "GAMEPID", "PBUFFER", "READFINISH", "ATTRIBUTE_UNKNOWN"};

struct cellCameraInternal
{
	bool m_bInitialized;
	CellCamera m_camera;

	cellCameraInternal()
		: m_bInitialized(false)
	{
	}
};

cellCameraInternal cellCameraInstance;

int cellCameraInit()
{
	cellCamera.Warning("cellCameraInit()");

	if (cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_ALREADY_INIT;

	if (Ini.Camera.GetValue() == 0)
		return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND;

	if (Ini.CameraType.GetValue() == 1)
	{
		CellCamera camera;
		camera.attributes.SATURATION = 164;
		camera.attributes.BRIGHTNESS = 96;
		camera.attributes.AEC = 1;
		camera.attributes.AGC = 1;
		camera.attributes.AWB = 1;
		camera.attributes.ABC = 0;
		camera.attributes.LED = 1;
		camera.attributes.QS = 0;
		camera.attributes.NONZEROCOEFFS[0] = 32;
		camera.attributes.NONZEROCOEFFS[1] = 32;
		camera.attributes.YUVFLAG = 0;
		camera.attributes.BACKLIGHTCOMP = 0;
		camera.attributes.MIRRORFLAG = 1;
		camera.attributes._422FLAG = 1;
		camera.attributes.USBLOAD = 4;
		cellCameraInstance.m_camera = camera;
	}
	else if (Ini.CameraType.GetValue() == 2)
	{
		CellCamera camera;
		camera.attributes.SATURATION = 64;
		camera.attributes.BRIGHTNESS = 8;
		camera.attributes.AEC = 1;
		camera.attributes.AGC = 1;
		camera.attributes.AWB = 1;
		camera.attributes.LED = 1;
		camera.attributes.BACKLIGHTCOMP = 0;
		camera.attributes.MIRRORFLAG = 1;
		camera.attributes.GAMMA = 1;
		camera.attributes.AGCLIMIT = 4;
		camera.attributes.DENOISE = 0;
		camera.attributes.FRAMERATEADJUST = 0;
		camera.attributes.PIXELOUTLIERFILTER = 1;
		camera.attributes.AGCLOW = 48;
		camera.attributes.AGCHIGH = 64;
		cellCameraInstance.m_camera = camera;
	}
	// TODO: Some other default attributes? Need to check the actual behaviour on a real PS3.

	cellCameraInstance.m_bInitialized = true;

	return CELL_OK;
}

int cellCameraEnd()
{
	cellCamera.Warning("cellCameraEnd()");

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	cellCameraInstance.m_bInitialized = false;

	return CELL_OK;
}

int cellCameraOpen()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraOpenEx()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraClose()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraGetDeviceGUID()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraGetType(s32 dev_num, vm::ptr<CellCameraType> type)
{
	cellCamera.Warning("cellCameraGetType(dev_num=%d, type_addr=0x%x)", dev_num, type.addr());

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	switch (Ini.CameraType.GetValue())
	{
	case 1: *type = CELL_CAMERA_EYETOY; break;
	case 2: *type = CELL_CAMERA_EYETOY2; break;
	case 3: *type = CELL_CAMERA_USBVIDEOCLASS; break;
	default: *type = CELL_CAMERA_TYPE_UNKNOWN; break;
	}

	return CELL_OK;
}

int cellCameraIsAvailable()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraIsAttached()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraIsOpen()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraIsStarted()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetAttribute(s32 dev_num, CellCameraAttribute attrib, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	cellCamera.Warning("cellCameraGetAttribute(dev_num=%d, attrib=%s(%d), arg1=0x%x, arg2=0x%x)", dev_num, attributes[attrib], arg1.addr(), arg2.addr());

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	switch (attrib)
	{
	case 0:
		*arg1 = cellCameraInstance.m_camera.attributes.GAIN; break;
	case 1:
		*arg1 = cellCameraInstance.m_camera.attributes.REDBLUEGAIN; break;
	case 2:
		*arg1 = cellCameraInstance.m_camera.attributes.SATURATION; break;
	case 3:
		*arg1 = cellCameraInstance.m_camera.attributes.EXPOSURE; break;
	case 4:
		*arg1 = cellCameraInstance.m_camera.attributes.BRIGHTNESS; break;
	case 5:
		*arg1 = cellCameraInstance.m_camera.attributes.AEC; break;
	case 6:
		*arg1 = cellCameraInstance.m_camera.attributes.AGC; break;
	case 7:
		*arg1 = cellCameraInstance.m_camera.attributes.AWB; break;
	case 8:
		*arg1 = cellCameraInstance.m_camera.attributes.ABC; break;
	case 9:
		*arg1 = cellCameraInstance.m_camera.attributes.LED; break;
	case 10:
		*arg1 = cellCameraInstance.m_camera.attributes.AUDIOGAIN; break;
	case 11:
		*arg1 = cellCameraInstance.m_camera.attributes.QS; break;
	case 12:
	{
		*arg1 = cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[0];
		*arg2 = cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[1];
		break;
	}
	case 13:
		*arg1 = cellCameraInstance.m_camera.attributes.YUVFLAG; break;
	case 14:
		*arg1 = cellCameraInstance.m_camera.attributes.JPEGFLAG; break;
	case 15:
		*arg1 = cellCameraInstance.m_camera.attributes.BACKLIGHTCOMP; break;
	case 16:
		*arg1 = cellCameraInstance.m_camera.attributes.MIRRORFLAG; break;
	case 17:
		*arg1 = cellCameraInstance.m_camera.attributes.MEASUREDQS; break;
	case 18:
		*arg1 = cellCameraInstance.m_camera.attributes._422FLAG; break;
	case 19:
		*arg1 = cellCameraInstance.m_camera.attributes.USBLOAD; break;
	case 20:
		*arg1 = cellCameraInstance.m_camera.attributes.GAMMA; break;
	case 21:
		*arg1 = cellCameraInstance.m_camera.attributes.GREENGAIN; break;
	case 22:
		*arg1 = cellCameraInstance.m_camera.attributes.AGCLIMIT; break;
	case 23:
		*arg1 = cellCameraInstance.m_camera.attributes.DENOISE; break;
	case 24:
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMERATEADJUST; break;
	case 25:
		*arg1 = cellCameraInstance.m_camera.attributes.PIXELOUTLIERFILTER; break;
	case 26:
		*arg1 = cellCameraInstance.m_camera.attributes.AGCLOW; break;
	case 27:
		*arg1 = cellCameraInstance.m_camera.attributes.AGCHIGH; break;
	case 28:
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICELOCATION; break;
	case 29:
		*arg1 = cellCameraInstance.m_camera.attributes.FORMATCAP; break;
	case 30:
		*arg1 = cellCameraInstance.m_camera.attributes.FORMATINDEX; break;
	case 31:
		*arg1 = cellCameraInstance.m_camera.attributes.NUMFRAME; break;
	case 32:
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMEINDEX; break;
	case 33:
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMESIZE; break;
	case 34:
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALTYPE; break;
	case 35:
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALINDEX; break;
	case 36:
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALVALUE; break;
	case 37:
		*arg1 = cellCameraInstance.m_camera.attributes.COLORMATCHING; break;
	case 38:
		*arg1 = cellCameraInstance.m_camera.attributes.PLFREQ; break;
	case 39:
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICEID; break;
	case 40:
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICECAP; break;
	case 41:
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICESPEED; break;
	case 42:
		*arg1 = cellCameraInstance.m_camera.attributes.UVCREQCODE; break;
	case 43:
		*arg1 = cellCameraInstance.m_camera.attributes.UVCREQDATA; break;
	case 44:
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICEID2; break;
	case 45:
		*arg1 = cellCameraInstance.m_camera.attributes.READMODE; break;
	case 46:
		*arg1 = cellCameraInstance.m_camera.attributes.GAMEPID; break;
	case 47:
		*arg1 = cellCameraInstance.m_camera.attributes.PBUFFER; break;
	case 48:
		*arg1 = cellCameraInstance.m_camera.attributes.READFINISH; break;
	case 49:
		*arg1 = cellCameraInstance.m_camera.attributes.ATTRIBUTE_UNKNOWN; break;
	default:
		cellCamera.Error("Unexpected cellCameraGetAttribute attribute: %d", attrib); break;
	}

	return CELL_OK;
}

int cellCameraSetAttribute(s32 dev_num, CellCameraAttribute attrib, u32 arg1, u32 arg2)
{
	cellCamera.Warning("cellCameraSetAttribute(dev_num=%d, attrib=%s(%d), arg1=%d, arg2=%d)", dev_num, attributes[attrib], attrib, arg1, arg2);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	switch (attrib)
	{
	case 0:
		cellCameraInstance.m_camera.attributes.GAIN = arg1; break;
	case 1:
		cellCameraInstance.m_camera.attributes.REDBLUEGAIN = arg1; break;
	case 2:
		cellCameraInstance.m_camera.attributes.SATURATION = arg1; break;
	case 3:
		cellCameraInstance.m_camera.attributes.EXPOSURE = arg1; break;
	case 4:
		cellCameraInstance.m_camera.attributes.BRIGHTNESS = arg1; break;
	case 5:
		cellCameraInstance.m_camera.attributes.AEC = arg1; break;
	case 6:
		cellCameraInstance.m_camera.attributes.AGC = arg1; break;
	case 7:
		cellCameraInstance.m_camera.attributes.AWB = arg1; break;
	case 8:
		cellCameraInstance.m_camera.attributes.ABC = arg1; break;
	case 9:
		cellCameraInstance.m_camera.attributes.LED = arg1; break;
	case 10:
		cellCameraInstance.m_camera.attributes.AUDIOGAIN = arg1; break;
	case 11:
		cellCameraInstance.m_camera.attributes.QS = arg1; break;
	case 12:
	{
		cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[0] = arg1;
		cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[1] = arg2;
		break;
	}
	case 13:
		cellCameraInstance.m_camera.attributes.YUVFLAG = arg1; break;
	case 14:
		cellCameraInstance.m_camera.attributes.JPEGFLAG = arg1; break;
	case 15:
		cellCameraInstance.m_camera.attributes.BACKLIGHTCOMP = arg1; break;
	case 16:
		cellCameraInstance.m_camera.attributes.MIRRORFLAG = arg1; break;
	case 17:
		return CELL_CAMERA_ERROR_PARAM; break;
	case 18:
		cellCameraInstance.m_camera.attributes._422FLAG = arg1; break;
	case 19:
		cellCameraInstance.m_camera.attributes.USBLOAD = arg1; break;
	case 20:
		cellCameraInstance.m_camera.attributes.GAMMA = arg1; break;
	case 21:
		cellCameraInstance.m_camera.attributes.GREENGAIN = arg1; break;
	case 22:
		cellCameraInstance.m_camera.attributes.AGCLIMIT = arg1; break;
	case 23:
		cellCameraInstance.m_camera.attributes.DENOISE = arg1; break;
	case 24:
		cellCameraInstance.m_camera.attributes.FRAMERATEADJUST = arg1; break;
	case 25:
		cellCameraInstance.m_camera.attributes.PIXELOUTLIERFILTER = arg1; break;
	case 26:
		cellCameraInstance.m_camera.attributes.AGCLOW = arg1; break;
	case 27:
		cellCameraInstance.m_camera.attributes.AGCHIGH = arg1; break;
	case 28:
		cellCameraInstance.m_camera.attributes.DEVICELOCATION = arg1; break;
	case 29:
		cellCamera.Error("Tried to write to read-only (?) value: FORMATCAP"); break;
	case 30:
		cellCameraInstance.m_camera.attributes.FORMATINDEX = arg1; break;
	case 31:
		cellCameraInstance.m_camera.attributes.NUMFRAME = arg1; break;
	case 32:
		cellCameraInstance.m_camera.attributes.FRAMEINDEX = arg1; break;
	case 33:
		cellCameraInstance.m_camera.attributes.FRAMESIZE = arg1; break;
	case 34:
		cellCameraInstance.m_camera.attributes.INTERVALTYPE = arg1; break;
	case 35:
		cellCameraInstance.m_camera.attributes.INTERVALINDEX = arg1; break;
	case 36:
		cellCameraInstance.m_camera.attributes.INTERVALVALUE = arg1; break;
	case 37:
		cellCameraInstance.m_camera.attributes.COLORMATCHING = arg1; break;
	case 38:
		cellCameraInstance.m_camera.attributes.PLFREQ = arg1; break;
	case 39:
		return CELL_CAMERA_ERROR_PARAM; break;
	case 40:
		cellCameraInstance.m_camera.attributes.DEVICECAP = arg1; break;
	case 41:
		cellCameraInstance.m_camera.attributes.DEVICESPEED = arg1; break;
	case 42:
		cellCameraInstance.m_camera.attributes.UVCREQCODE = arg1; break;
	case 43:
		cellCameraInstance.m_camera.attributes.UVCREQDATA = arg1; break;
	case 44:
		return CELL_CAMERA_ERROR_PARAM; break;
	case 45:
		cellCamera.Error("Tried to write to read-only (?) value: READMODE"); break;
	case 46:
		cellCameraInstance.m_camera.attributes.GAMEPID = arg1; break;
	case 47:
		cellCameraInstance.m_camera.attributes.PBUFFER = arg1; break;
	case 48:
		cellCameraInstance.m_camera.attributes.READFINISH = arg1; break;
	case 49:
		cellCamera.Error("Tried to write to read-only (?) value: ATTRIBUTE_UNKNOWN"); break;
	default:
		cellCamera.Error("Unexpected cellCameraGetAttribute attribute: %d", attrib); break;
	}

	return CELL_OK;
}

int cellCameraGetBufferSize()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraGetBufferInfo()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetBufferInfoEx()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraPrepExtensionUnit()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraCtrlExtensionUnit()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetExtensionUnit()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraSetExtensionUnit()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraReset()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraStart()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraRead()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraReadEx()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraReadComplete()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraStop()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraSetNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

int cellCameraRemoveNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellCamera);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	return CELL_OK;
}

Module cellCamera("cellCamera", []()
{
	cellCameraInstance.m_bInitialized = false;

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
