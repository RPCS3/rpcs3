#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellGem.h"

Module *cellGem = nullptr;

struct cellGemInternal
{
	bool m_bInitialized;

	cellGemInternal()
		: m_bInitialized(false)
	{
	}
};

cellGemInternal cellGemInstance;

int cellGemCalibrate()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemClearStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemConvertVideoFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemConvertVideoStart()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemEnableCameraPitchAngleCorrection()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemEnableMagnetometer()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemEnd()
{
	cellGem->Warning("cellGemEnd()");

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	cellGemInstance.m_bInitialized = false;

	return CELL_OK;
}

int cellGemFilterState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemForceRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetAccelerometerPositionInDevice()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetAllTrackableHues()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetCameraState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetEnvironmentLightingColor()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetHuePixels()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetImageState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetInertialState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetInfo()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

// Should int be used, even when type is int_32t (s32)?
s32 cellGemGetMemorySize(be_t<s32> max_connect)
{
	cellGem->Warning("cellGemGetMemorySize(max_connect=%i)", max_connect);

	if (max_connect > CELL_GEM_MAX_NUM)
		return CELL_GEM_ERROR_INVALID_PARAMETER;

	return 1024 * 1024 * max_connect; // 1 MB * max_connect
}

int cellGemGetRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemGetTrackerHue()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemHSVtoRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemInit(vm::ptr<CellGemAttribute> attribute)
{
	cellGem->Warning("cellGemInit(attribute_addr=0x%x)", attribute.addr());

	if (cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;

	cellGemInstance.m_bInitialized = true;

	return CELL_OK;
}

int cellGemInvalidateCalibration()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemIsTrackableHue()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemPrepareCamera()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemPrepareVideoConvert()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemReset()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemSetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemSetYaw()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemTrackHues()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemUpdateFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemUpdateStart()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemWriteExternalPort()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

void cellGem_init(Module *pxThis)
{
	cellGem = pxThis;

	//cellGem->AddFunc(, cellGemAttributeInit);
	cellGem->AddFunc(0xafa99ead, cellGemCalibrate);
	cellGem->AddFunc(0x9b9714a4, cellGemClearStatusFlags);
	cellGem->AddFunc(0x1a13d010, cellGemConvertVideoFinish);
	cellGem->AddFunc(0x6dce048c, cellGemConvertVideoStart);
	cellGem->AddFunc(0x4219de31, cellGemEnableCameraPitchAngleCorrection);
	cellGem->AddFunc(0x1a2518a2, cellGemEnableMagnetometer);
	cellGem->AddFunc(0xe1f85a80, cellGemEnd);
	cellGem->AddFunc(0x6fc4c791, cellGemFilterState);
	cellGem->AddFunc(0xce6d7791, cellGemForceRGB);
	cellGem->AddFunc(0x6a5b7048, cellGemGetAccelerometerPositionInDevice);
	cellGem->AddFunc(0x2d2c2764, cellGemGetAllTrackableHues);
	cellGem->AddFunc(0x8befac67, cellGemGetCameraState);
	cellGem->AddFunc(0x02eb41bb, cellGemGetEnvironmentLightingColor);
	cellGem->AddFunc(0xb8ef56a6, cellGemGetHuePixels);
	cellGem->AddFunc(0x92cc4b34, cellGemGetImageState);
	cellGem->AddFunc(0xd37b127a, cellGemGetInertialState);
	cellGem->AddFunc(0x9e1dff96, cellGemGetInfo);
	cellGem->AddFunc(0x2e0a170d, cellGemGetMemorySize);
	cellGem->AddFunc(0x1b30cc22, cellGemGetRGB);
	cellGem->AddFunc(0x6db6b007, cellGemGetRumble);
	cellGem->AddFunc(0x6441d38d, cellGemGetState);
	cellGem->AddFunc(0xfee33481, cellGemGetStatusFlags);
	cellGem->AddFunc(0x18ea899a, cellGemGetTrackerHue);
	//cellGem->AddFunc(, cellGemGetVideoConvertSize);
	cellGem->AddFunc(0xc7622586, cellGemHSVtoRGB);
	cellGem->AddFunc(0x13ea7c64, cellGemInit);
	cellGem->AddFunc(0xe3e4f0d6, cellGemInvalidateCalibration);
	cellGem->AddFunc(0xfb5887f9, cellGemIsTrackableHue);
	cellGem->AddFunc(0xa03ef587, cellGemPrepareCamera);
	cellGem->AddFunc(0xc07896f9, cellGemPrepareVideoConvert);
	//cellGem->AddFunc(, cellGemReadExternalPortDeviceInfo);
	cellGem->AddFunc(0xde54e2fc, cellGemReset);
	cellGem->AddFunc(0x49609306, cellGemSetRumble);
	cellGem->AddFunc(0x77e08704, cellGemSetYaw);
	cellGem->AddFunc(0x928ac5f8, cellGemTrackHues);
	cellGem->AddFunc(0x41ae9c31, cellGemUpdateFinish);
	cellGem->AddFunc(0x0ecd2261, cellGemUpdateStart);
	//cellGem->AddFunc(, cellGemVideoConvertAttributeInit);
	//cellGem->AddFunc(, cellGemVideoConvertAttributeInitRgba);
	cellGem->AddFunc(0x1f6328d8, cellGemWriteExternalPort);

}
