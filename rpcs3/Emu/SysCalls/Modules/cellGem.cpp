#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellGem.h"

extern Module cellGem;

struct cellGemInternal
{
	bool m_bInitialized;
	CellGemAttribute attribute;

	cellGemInternal()
		: m_bInitialized(false)
	{
	}
};

cellGemInternal cellGemInstance;

int cellGemCalibrate()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemClearStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemConvertVideoFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemConvertVideoStart()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemEnableCameraPitchAngleCorrection()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemEnableMagnetometer()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemEnd()
{
	cellGem.Warning("cellGemEnd()");

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	cellGemInstance.m_bInitialized = false;

	return CELL_OK;
}

int cellGemFilterState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemForceRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetAccelerometerPositionInDevice()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetAllTrackableHues()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetCameraState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetEnvironmentLightingColor()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetHuePixels()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetImageState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetInertialState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetInfo(vm::ptr<CellGemInfo> info)
{
	cellGem.Warning("cellGemGetInfo(info=0x%x)", info.addr());

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	info->max_connect = cellGemInstance.attribute.max_connect;
	// TODO: Support many controllers to be connected
	info->now_connect = 1;
	info->status[0] = CELL_GEM_STATUS_READY;
	info->port[0] = 7;

	return CELL_OK;
}

s32 cellGemGetMemorySize(s32 max_connect)
{
	cellGem.Warning("cellGemGetMemorySize(max_connect=%d)", max_connect);

	if (max_connect > CELL_GEM_MAX_NUM)
		return CELL_GEM_ERROR_INVALID_PARAMETER;

	return 1024 * 1024 * max_connect; // 1 MB * max_connect
}

int cellGemGetRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemGetTrackerHue()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemHSVtoRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

int cellGemInit(vm::ptr<CellGemAttribute> attribute)
{
	cellGem.Warning("cellGemInit(attribute_addr=0x%x)", attribute.addr());

	if (cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;

	cellGemInstance.m_bInitialized = true;
	cellGemInstance.attribute = *attribute;

	return CELL_OK;
}

int cellGemInvalidateCalibration()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

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

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemPrepareVideoConvert()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemReset()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemSetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemSetYaw()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemTrackHues()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemUpdateFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemUpdateStart()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

int cellGemWriteExternalPort()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

void cellGem_unload()
{
	cellGemInstance.m_bInitialized = false;
}

Module cellGem("cellGem", []()
{
	//cellGem.AddFunc(, cellGemAttributeInit);
	REG_FUNC(cellGem, cellGemCalibrate);
	REG_FUNC(cellGem, cellGemClearStatusFlags);
	REG_FUNC(cellGem, cellGemConvertVideoFinish);
	REG_FUNC(cellGem, cellGemConvertVideoStart);
	REG_FUNC(cellGem, cellGemEnableCameraPitchAngleCorrection);
	REG_FUNC(cellGem, cellGemEnableMagnetometer);
	REG_FUNC(cellGem, cellGemEnd);
	REG_FUNC(cellGem, cellGemFilterState);
	REG_FUNC(cellGem, cellGemForceRGB);
	REG_FUNC(cellGem, cellGemGetAccelerometerPositionInDevice);
	REG_FUNC(cellGem, cellGemGetAllTrackableHues);
	REG_FUNC(cellGem, cellGemGetCameraState);
	REG_FUNC(cellGem, cellGemGetEnvironmentLightingColor);
	REG_FUNC(cellGem, cellGemGetHuePixels);
	REG_FUNC(cellGem, cellGemGetImageState);
	REG_FUNC(cellGem, cellGemGetInertialState);
	REG_FUNC(cellGem, cellGemGetInfo);
	REG_FUNC(cellGem, cellGemGetMemorySize);
	REG_FUNC(cellGem, cellGemGetRGB);
	REG_FUNC(cellGem, cellGemGetRumble);
	REG_FUNC(cellGem, cellGemGetState);
	REG_FUNC(cellGem, cellGemGetStatusFlags);
	REG_FUNC(cellGem, cellGemGetTrackerHue);
	//cellGem.AddFunc(, cellGemGetVideoConvertSize);
	REG_FUNC(cellGem, cellGemHSVtoRGB);
	REG_FUNC(cellGem, cellGemInit);
	REG_FUNC(cellGem, cellGemInvalidateCalibration);
	REG_FUNC(cellGem, cellGemIsTrackableHue);
	REG_FUNC(cellGem, cellGemPrepareCamera);
	REG_FUNC(cellGem, cellGemPrepareVideoConvert);
	//cellGem.AddFunc(, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(cellGem, cellGemReset);
	REG_FUNC(cellGem, cellGemSetRumble);
	REG_FUNC(cellGem, cellGemSetYaw);
	REG_FUNC(cellGem, cellGemTrackHues);
	REG_FUNC(cellGem, cellGemUpdateFinish);
	REG_FUNC(cellGem, cellGemUpdateStart);
	//cellGem.AddFunc(, cellGemVideoConvertAttributeInit);
	//cellGem.AddFunc(, cellGemVideoConvertAttributeInitRgba);
	REG_FUNC(cellGem, cellGemWriteExternalPort);
});
