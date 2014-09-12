#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellCamera.h"

void cellCamera_init();
//Module cellCamera(0x0023, cellCamera_init);
Module *cellCamera = nullptr;

struct cellCameraInternal
{
	bool m_bInitialized;

	cellCameraInternal()
		: m_bInitialized(false)
	{
	}
};

cellCameraInternal cellCameraInstance;

int cellCameraInit()
{
	cellCamera->Warning("cellCameraInit()");

	if (cellCameraInstance.m_bInitialized)
		return CELL_CAMERA_ERROR_ALREADY_INIT;

	// TODO: Check if camera is connected, if not return CELL_CAMERA_ERROR_DEVICE_NOT_FOUND
	cellCameraInstance.m_bInitialized = true;

	return CELL_OK;
}

int cellCameraEnd()
{
	cellCamera->Warning("cellCameraEnd()");

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

int cellCameraGetType(s32 dev_num, CellCameraType type)
{
	cellCamera->Warning("cellCameraGetType(dev_num=%d, type_addr=0x%x)", dev_num, type);

	if (Ini.CameraType.GetValue() == 1)
		type = CELL_CAMERA_EYETOY;
	else if (Ini.CameraType.GetValue() == 2)
		type = CELL_CAMERA_EYETOY2;
	else if (Ini.CameraType.GetValue() == 3)
		type == CELL_CAMERA_USBVIDEOCLASS;
	else
		type = CELL_CAMERA_TYPE_UNKNOWN;

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
	cellCamera->AddFunc(0xbf47c5dd, cellCameraInit);
	cellCamera->AddFunc(0x5ad46570, cellCameraEnd);
	cellCamera->AddFunc(0x85e1b8da, cellCameraOpen);
	cellCamera->AddFunc(0x5d25f866, cellCameraOpenEx);
	cellCamera->AddFunc(0x379c5dd6, cellCameraClose);

	cellCamera->AddFunc(0x602e2052, cellCameraGetDeviceGUID);
	cellCamera->AddFunc(0x58bc5870, cellCameraGetType);
	cellCamera->AddFunc(0x8ca53dde, cellCameraIsAvailable);
	cellCamera->AddFunc(0x7e063bbc, cellCameraIsAttached);
	cellCamera->AddFunc(0xfa160f24, cellCameraIsOpen);
	cellCamera->AddFunc(0x5eebf24e, cellCameraIsStarted);
	cellCamera->AddFunc(0x532b8aaa, cellCameraGetAttribute);
	cellCamera->AddFunc(0x8cd56eee, cellCameraSetAttribute);
	cellCamera->AddFunc(0x7dac520c, cellCameraGetBufferSize);
	cellCamera->AddFunc(0x10697d7f, cellCameraGetBufferInfo);
	cellCamera->AddFunc(0x0e63c444, cellCameraGetBufferInfoEx);

	cellCamera->AddFunc(0x61dfbe83, cellCameraPrepExtensionUnit);
	cellCamera->AddFunc(0xeb6f95fb, cellCameraCtrlExtensionUnit);
	cellCamera->AddFunc(0xb602e328, cellCameraGetExtensionUnit);
	cellCamera->AddFunc(0x2dea3e9b, cellCameraSetExtensionUnit);

	cellCamera->AddFunc(0x81f83db9, cellCameraReset);
	cellCamera->AddFunc(0x456dc4aa, cellCameraStart);
	cellCamera->AddFunc(0x3845d39b, cellCameraRead);
	cellCamera->AddFunc(0x21fc151f, cellCameraReadEx);
	cellCamera->AddFunc(0xe28b206b, cellCameraReadComplete);
	cellCamera->AddFunc(0x02f5ced0, cellCameraStop);

	cellCamera->AddFunc(0xb0647e5a, cellCameraSetNotifyEventQueue);
	cellCamera->AddFunc(0x9b98d258, cellCameraRemoveNotifyEventQueue);
	cellCamera->AddFunc(0xa7fd2f5b, cellCameraSetNotifyEventQueue2);
	cellCamera->AddFunc(0x44673f07, cellCameraRemoveNotifyEventQueue2);
}
