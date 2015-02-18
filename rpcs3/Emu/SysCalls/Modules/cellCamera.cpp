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

	if (attrib == 0)
		*arg1 = cellCameraInstance.m_camera.attributes.GAIN;
	else if (attrib == 1)
		*arg1 = cellCameraInstance.m_camera.attributes.REDBLUEGAIN;
	else if (attrib == 2)
		*arg1 = cellCameraInstance.m_camera.attributes.SATURATION;
	else if (attrib == 3)
		*arg1 = cellCameraInstance.m_camera.attributes.EXPOSURE;
	else if (attrib == 4)
		*arg1 = cellCameraInstance.m_camera.attributes.BRIGHTNESS;
	else if (attrib == 5)
		*arg1 = cellCameraInstance.m_camera.attributes.AEC;
	else if (attrib == 6)
		*arg1 = cellCameraInstance.m_camera.attributes.AGC;
	else if (attrib == 7)
		*arg1 = cellCameraInstance.m_camera.attributes.AWB;
	else if (attrib == 8)
		*arg1 = cellCameraInstance.m_camera.attributes.ABC;
	else if (attrib == 9)
		*arg1 = cellCameraInstance.m_camera.attributes.LED;
	else if (attrib == 10)
		*arg1 = cellCameraInstance.m_camera.attributes.AUDIOGAIN;
	else if (attrib == 11)
		*arg1 = cellCameraInstance.m_camera.attributes.QS;
	else if (attrib == 12)
	{
		*arg1 = cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[0];
		*arg2 = cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[1];
	}
	else if (attrib == 13)
		*arg1 = cellCameraInstance.m_camera.attributes.YUVFLAG;
	else if (attrib == 14)
		*arg1 = cellCameraInstance.m_camera.attributes.JPEGFLAG;
	else if (attrib == 15)
		*arg1 = cellCameraInstance.m_camera.attributes.BACKLIGHTCOMP;
	else if (attrib == 16)
		*arg1 = cellCameraInstance.m_camera.attributes.MIRRORFLAG;
	else if (attrib == 17)
		*arg1 = cellCameraInstance.m_camera.attributes.MEASUREDQS;
	else if (attrib == 18)
		*arg1 = cellCameraInstance.m_camera.attributes._422FLAG;
	else if (attrib == 19)
		*arg1 = cellCameraInstance.m_camera.attributes.USBLOAD;
	else if (attrib == 20)
		*arg1 = cellCameraInstance.m_camera.attributes.GAMMA;
	else if (attrib == 21)
		*arg1 = cellCameraInstance.m_camera.attributes.GREENGAIN;
	else if (attrib == 22)
		*arg1 = cellCameraInstance.m_camera.attributes.AGCLIMIT;
	else if (attrib == 23)
		*arg1 = cellCameraInstance.m_camera.attributes.DENOISE;
	else if (attrib == 24)
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMERATEADJUST;
	else if (attrib == 25)
		*arg1 = cellCameraInstance.m_camera.attributes.PIXELOUTLIERFILTER;
	else if (attrib == 26)
		*arg1 = cellCameraInstance.m_camera.attributes.AGCLOW;
	else if (attrib == 27)
		*arg1 = cellCameraInstance.m_camera.attributes.AGCHIGH;
	else if (attrib == 28)
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICELOCATION;
	else if (attrib == 29)
		*arg1 = cellCameraInstance.m_camera.attributes.FORMATCAP;
	else if (attrib == 30)
		*arg1 = cellCameraInstance.m_camera.attributes.FORMATINDEX;
	else if (attrib == 31)
		*arg1 = cellCameraInstance.m_camera.attributes.NUMFRAME;
	else if (attrib == 32)
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMEINDEX;
	else if (attrib == 33)
		*arg1 = cellCameraInstance.m_camera.attributes.FRAMESIZE;
	else if (attrib == 34)
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALTYPE;
	else if (attrib == 35)
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALINDEX;
	else if (attrib == 36)
		*arg1 = cellCameraInstance.m_camera.attributes.INTERVALVALUE;
	else if (attrib == 37)
		*arg1 = cellCameraInstance.m_camera.attributes.COLORMATCHING;
	else if (attrib == 38)
		*arg1 = cellCameraInstance.m_camera.attributes.PLFREQ;
	else if (attrib == 39)
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICEID;
	else if (attrib == 40)
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICECAP;
	else if (attrib == 41)
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICESPEED;
	else if (attrib == 42)
		*arg1 = cellCameraInstance.m_camera.attributes.UVCREQCODE;
	else if (attrib == 43)
		*arg1 = cellCameraInstance.m_camera.attributes.UVCREQDATA;
	else if (attrib == 44)
		*arg1 = cellCameraInstance.m_camera.attributes.DEVICEID2;
	else if (attrib == 45)
		*arg1 = cellCameraInstance.m_camera.attributes.READMODE;
	else if (attrib == 46)
		*arg1 = cellCameraInstance.m_camera.attributes.GAMEPID;
	else if (attrib == 47)
		*arg1 = cellCameraInstance.m_camera.attributes.PBUFFER;
	else if (attrib == 48)
		*arg1 = cellCameraInstance.m_camera.attributes.READFINISH;
	else if (attrib == 49)
		*arg1 = cellCameraInstance.m_camera.attributes.ATTRIBUTE_UNKNOWN;

	return CELL_OK;
}

int cellCameraSetAttribute(s32 dev_num, CellCameraAttribute attrib, u32 arg1, u32 arg2)
{
	cellCamera.Warning("cellCameraSetAttribute(dev_num=%d, attrib=%s(%d), arg1=%d, arg2=%d)", dev_num, attributes[attrib], attrib, arg1, arg2);

	if (!cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_NOT_INIT;

	if (attrib == 0)
		cellCameraInstance.m_camera.attributes.GAIN = arg1;
	else if (attrib == 1)
		cellCameraInstance.m_camera.attributes.REDBLUEGAIN = arg1;
	else if (attrib == 2)
		cellCameraInstance.m_camera.attributes.SATURATION = arg1;
	else if (attrib == 3)
		cellCameraInstance.m_camera.attributes.EXPOSURE = arg1;
	else if (attrib == 4)
		cellCameraInstance.m_camera.attributes.BRIGHTNESS = arg1;
	else if (attrib == 5)
		cellCameraInstance.m_camera.attributes.AEC = arg1;
	else if (attrib == 6)
		cellCameraInstance.m_camera.attributes.AGC = arg1;
	else if (attrib == 7)
		cellCameraInstance.m_camera.attributes.AWB = arg1;
	else if (attrib == 8)
		cellCameraInstance.m_camera.attributes.ABC = arg1;
	else if (attrib == 9)
		cellCameraInstance.m_camera.attributes.LED = arg1;
	else if (attrib == 10)
		cellCameraInstance.m_camera.attributes.AUDIOGAIN = arg1;
	else if (attrib == 11)
		cellCameraInstance.m_camera.attributes.QS = arg1;
	else if (attrib == 12)
	{
		cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[0] = arg1;
		cellCameraInstance.m_camera.attributes.NONZEROCOEFFS[1] = arg2;
	}
	else if (attrib == 13)
		cellCameraInstance.m_camera.attributes.YUVFLAG = arg1;
	else if (attrib == 14)
		cellCameraInstance.m_camera.attributes.JPEGFLAG = arg1;
	else if (attrib == 15)
		cellCameraInstance.m_camera.attributes.BACKLIGHTCOMP = arg1;
	else if (attrib == 16)
		cellCameraInstance.m_camera.attributes.MIRRORFLAG = arg1;
	else if (attrib == 17)
		return CELL_CAMERA_ERROR_PARAM;
	else if (attrib == 18)
		cellCameraInstance.m_camera.attributes._422FLAG = arg1;
	else if (attrib == 19)
		cellCameraInstance.m_camera.attributes.USBLOAD = arg1;
	else if (attrib == 20)
		cellCameraInstance.m_camera.attributes.GAMMA = arg1;
	else if (attrib == 21)
		cellCameraInstance.m_camera.attributes.GREENGAIN = arg1;
	else if (attrib == 22)
		cellCameraInstance.m_camera.attributes.AGCLIMIT = arg1;
	else if (attrib == 23)
		cellCameraInstance.m_camera.attributes.DENOISE = arg1;
	else if (attrib == 24)
		cellCameraInstance.m_camera.attributes.FRAMERATEADJUST = arg1;
	else if (attrib == 25)
		cellCameraInstance.m_camera.attributes.PIXELOUTLIERFILTER = arg1;
	else if (attrib == 26)
		cellCameraInstance.m_camera.attributes.AGCLOW = arg1;
	else if (attrib == 27)
		cellCameraInstance.m_camera.attributes.AGCHIGH = arg1;
	else if (attrib == 28)
		cellCameraInstance.m_camera.attributes.DEVICELOCATION = arg1;
	else if (attrib == 29)
		cellCamera.Error("Tried to write to read-only (?) value: FORMATCAP");
	else if (attrib == 30)
		cellCameraInstance.m_camera.attributes.FORMATINDEX = arg1;
	else if (attrib == 31)
		cellCameraInstance.m_camera.attributes.NUMFRAME = arg1;
	else if (attrib == 32)
		cellCameraInstance.m_camera.attributes.FRAMEINDEX = arg1;
	else if (attrib == 33)
		cellCameraInstance.m_camera.attributes.FRAMESIZE = arg1;
	else if (attrib == 34)
		cellCameraInstance.m_camera.attributes.INTERVALTYPE = arg1;
	else if (attrib == 35)
		cellCameraInstance.m_camera.attributes.INTERVALINDEX = arg1;
	else if (attrib == 36)
		cellCameraInstance.m_camera.attributes.INTERVALVALUE = arg1;
	else if (attrib == 37)
		cellCameraInstance.m_camera.attributes.COLORMATCHING = arg1;
	else if (attrib == 38)
		cellCameraInstance.m_camera.attributes.PLFREQ = arg1;
	else if (attrib == 39)
		return CELL_CAMERA_ERROR_PARAM;
	else if (attrib == 40)
		cellCameraInstance.m_camera.attributes.DEVICECAP = arg1;
	else if (attrib == 41)
		cellCameraInstance.m_camera.attributes.DEVICESPEED = arg1;
	else if (attrib == 42)
		cellCameraInstance.m_camera.attributes.UVCREQCODE = arg1;
	else if (attrib == 43)
		cellCameraInstance.m_camera.attributes.UVCREQDATA = arg1;
	else if (attrib == 44)
		return CELL_CAMERA_ERROR_PARAM;
	else if (attrib == 45)
		cellCamera.Error("Tried to write to read-only (?) value: READMODE");
	else if (attrib == 46)
		cellCameraInstance.m_camera.attributes.GAMEPID = arg1;
	else if (attrib == 47)
		cellCameraInstance.m_camera.attributes.PBUFFER = arg1;
	else if (attrib == 48)
		cellCameraInstance.m_camera.attributes.READFINISH = arg1;
	else if (attrib == 49)
		cellCamera.Error("Tried to write to read-only (?) value: ATTRIBUTE_UNKNOWN");

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

void cellCamera_unload()
{
	cellCameraInstance.m_bInitialized = false;
}

Module cellCamera("cellCamera", []()
{
	cellCamera.AddFunc(0xbf47c5dd, cellCameraInit);
	cellCamera.AddFunc(0x5ad46570, cellCameraEnd);
	cellCamera.AddFunc(0x85e1b8da, cellCameraOpen);
	cellCamera.AddFunc(0x5d25f866, cellCameraOpenEx);
	cellCamera.AddFunc(0x379c5dd6, cellCameraClose);

	cellCamera.AddFunc(0x602e2052, cellCameraGetDeviceGUID);
	cellCamera.AddFunc(0x58bc5870, cellCameraGetType);
	cellCamera.AddFunc(0x8ca53dde, cellCameraIsAvailable);
	cellCamera.AddFunc(0x7e063bbc, cellCameraIsAttached);
	cellCamera.AddFunc(0xfa160f24, cellCameraIsOpen);
	cellCamera.AddFunc(0x5eebf24e, cellCameraIsStarted);
	cellCamera.AddFunc(0x532b8aaa, cellCameraGetAttribute);
	cellCamera.AddFunc(0x8cd56eee, cellCameraSetAttribute);
	cellCamera.AddFunc(0x7dac520c, cellCameraGetBufferSize);
	cellCamera.AddFunc(0x10697d7f, cellCameraGetBufferInfo);
	cellCamera.AddFunc(0x0e63c444, cellCameraGetBufferInfoEx);

	cellCamera.AddFunc(0x61dfbe83, cellCameraPrepExtensionUnit);
	cellCamera.AddFunc(0xeb6f95fb, cellCameraCtrlExtensionUnit);
	cellCamera.AddFunc(0xb602e328, cellCameraGetExtensionUnit);
	cellCamera.AddFunc(0x2dea3e9b, cellCameraSetExtensionUnit);

	cellCamera.AddFunc(0x81f83db9, cellCameraReset);
	cellCamera.AddFunc(0x456dc4aa, cellCameraStart);
	cellCamera.AddFunc(0x3845d39b, cellCameraRead);
	cellCamera.AddFunc(0x21fc151f, cellCameraReadEx);
	cellCamera.AddFunc(0xe28b206b, cellCameraReadComplete);
	cellCamera.AddFunc(0x02f5ced0, cellCameraStop);

	cellCamera.AddFunc(0xb0647e5a, cellCameraSetNotifyEventQueue);
	cellCamera.AddFunc(0x9b98d258, cellCameraRemoveNotifyEventQueue);
	cellCamera.AddFunc(0xa7fd2f5b, cellCameraSetNotifyEventQueue2);
	cellCamera.AddFunc(0x44673f07, cellCameraRemoveNotifyEventQueue2);
});
