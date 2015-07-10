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

s32 cellGemCalibrate()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemClearStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemConvertVideoFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemConvertVideoStart()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemEnableCameraPitchAngleCorrection()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemEnableMagnetometer()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemEnd()
{
	cellGem.Warning("cellGemEnd()");

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	cellGemInstance.m_bInitialized = false;

	return CELL_OK;
}

s32 cellGemFilterState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemForceRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetAccelerometerPositionInDevice()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetAllTrackableHues()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetCameraState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetEnvironmentLightingColor()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetHuePixels()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetImageState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetInertialState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetInfo(vm::ptr<CellGemInfo> info)
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

s32 cellGemGetRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetState()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemGetTrackerHue()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemHSVtoRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemInit(vm::ptr<CellGemAttribute> attribute)
{
	cellGem.Warning("cellGemInit(attribute=*0x%x)", attribute);

	if (cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;

	cellGemInstance.m_bInitialized = true;
	cellGemInstance.attribute = *attribute;

	return CELL_OK;
}

s32 cellGemInvalidateCalibration()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemIsTrackableHue()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemPrepareCamera()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemPrepareVideoConvert()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemReset()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemSetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemSetYaw()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemTrackHues()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemUpdateFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemUpdateStart()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellGemWriteExternalPort()
{
	UNIMPLEMENTED_FUNC(cellGem);

	if (!cellGemInstance.m_bInitialized)
		return CELL_GEM_ERROR_UNINITIALIZED;

	return CELL_OK;
}

Module cellGem("cellGem", []()
{
	cellGemInstance.m_bInitialized = false;

	//REG_FUNC(cellGem, cellGemAttributeInit);
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
	//REG_FUNC(cellGem, cellGemGetVideoConvertSize);
	REG_FUNC(cellGem, cellGemHSVtoRGB);
	REG_FUNC(cellGem, cellGemInit);
	REG_FUNC(cellGem, cellGemInvalidateCalibration);
	REG_FUNC(cellGem, cellGemIsTrackableHue);
	REG_FUNC(cellGem, cellGemPrepareCamera);
	REG_FUNC(cellGem, cellGemPrepareVideoConvert);
	//REG_FUNC(cellGem, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(cellGem, cellGemReset);
	REG_FUNC(cellGem, cellGemSetRumble);
	REG_FUNC(cellGem, cellGemSetYaw);
	REG_FUNC(cellGem, cellGemTrackHues);
	REG_FUNC(cellGem, cellGemUpdateFinish);
	REG_FUNC(cellGem, cellGemUpdateStart);
	//REG_FUNC(cellGem, cellGemVideoConvertAttributeInit);
	//REG_FUNC(cellGem, cellGemVideoConvertAttributeInitRgba);
	REG_FUNC(cellGem, cellGemWriteExternalPort);
});
