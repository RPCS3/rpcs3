#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellCamera_init();
Module cellCamera(0x0023, cellCamera_init);

// Error Codes
enum
{
	CELL_CAMERA_ERROR_ALREADY_INIT       = 0x80140801,
	CELL_CAMERA_ERROR_NOT_INIT           = 0x80140803,
	CELL_CAMERA_ERROR_PARAM              = 0x80140804,
	CELL_CAMERA_ERROR_ALREADY_OPEN       = 0x80140805,
	CELL_CAMERA_ERROR_NOT_OPEN           = 0x80140806,
	CELL_CAMERA_ERROR_DEVICE_NOT_FOUND   = 0x80140807,
	CELL_CAMERA_ERROR_DEVICE_DEACTIVATED = 0x80140808,
	CELL_CAMERA_ERROR_NOT_STARTED        = 0x80140809,
	CELL_CAMERA_ERROR_FORMAT_UNKNOWN     = 0x8014080a,
	CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN = 0x8014080b,
	CELL_CAMERA_ERROR_BAD_FRAMERATE      = 0x8014080c,
	CELL_CAMERA_ERROR_TIMEOUT            = 0x8014080d,
	CELL_CAMERA_ERROR_FATAL              = 0x8014080f,
};

int cellCameraInit()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraEnd()
{
	UNIMPLEMENTED_FUNC(cellCamera);
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
	return CELL_OK;
}

int cellCameraClose()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetDeviceGUID()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetType()
{
	UNIMPLEMENTED_FUNC(cellCamera);
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

int cellCameraGetAttribute()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraSetAttribute()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraGetBufferSize()
{
	UNIMPLEMENTED_FUNC(cellCamera);
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
	return CELL_OK;
}

int cellCameraStart()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraRead()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraReadEx()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraReadComplete()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraStop()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraSetNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

int cellCameraRemoveNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellCamera);
	return CELL_OK;
}

void cellCamera_init()
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
}