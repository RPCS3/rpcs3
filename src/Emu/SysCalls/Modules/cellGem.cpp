#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellGem.h"

extern Module<> cellGem;

struct gem_t
{
	CellGemAttribute attribute;
};

s32 cellGemCalibrate()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemClearStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemConvertVideoFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemConvertVideoStart()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemEnableCameraPitchAngleCorrection()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemEnableMagnetometer()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemEnd()
{
	cellGem.warning("cellGemEnd()");

	if (!fxm::remove<gem_t>())
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

s32 cellGemFilterState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemForceRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetAccelerometerPositionInDevice()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetAllTrackableHues()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetCameraState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetEnvironmentLightingColor()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetHuePixels()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetImageState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetInertialState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetInfo(vm::ptr<CellGemInfo> info)
{
	cellGem.todo("cellGemGetInfo(info=*0x%x)", info);

	const auto gem = fxm::get<gem_t>();

	if (!gem)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	// TODO: Support connecting PlayStation Move controllers
	info->max_connect = gem->attribute.max_connect;
	info->now_connect = 0;

	for (int i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		info->status[i] = CELL_GEM_STATUS_DISCONNECTED;
	}

	return CELL_OK;
}

s32 cellGemGetMemorySize(s32 max_connect)
{
	cellGem.warning("cellGemGetMemorySize(max_connect=%d)", max_connect);

	if (max_connect > CELL_GEM_MAX_NUM || max_connect <= 0)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return max_connect <= 2 ? 0x120000 : 0x140000;
}

s32 cellGemGetRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetState()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetStatusFlags()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemGetTrackerHue()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemHSVtoRGB()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemInit(vm::cptr<CellGemAttribute> attribute)
{
	cellGem.warning("cellGemInit(attribute=*0x%x)", attribute);

	const auto gem = fxm::make<gem_t>();

	if (!gem)
	{
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;
	}

	gem->attribute = *attribute;

	return CELL_OK;
}

s32 cellGemInvalidateCalibration()
{
	UNIMPLEMENTED_FUNC(cellGem);
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
	return CELL_OK;
}

s32 cellGemPrepareVideoConvert()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemReadExternalPortDeviceInfo()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemReset()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemSetRumble()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemSetYaw()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemTrackHues()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemUpdateFinish()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemUpdateStart()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

s32 cellGemWriteExternalPort()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

Module<> cellGem("cellGem", []()
{
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
	REG_FUNC(cellGem, cellGemHSVtoRGB);
	REG_FUNC(cellGem, cellGemInit);
	REG_FUNC(cellGem, cellGemInvalidateCalibration);
	REG_FUNC(cellGem, cellGemIsTrackableHue);
	REG_FUNC(cellGem, cellGemPrepareCamera);
	REG_FUNC(cellGem, cellGemPrepareVideoConvert);
	REG_FUNC(cellGem, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(cellGem, cellGemReset);
	REG_FUNC(cellGem, cellGemSetRumble);
	REG_FUNC(cellGem, cellGemSetYaw);
	REG_FUNC(cellGem, cellGemTrackHues);
	REG_FUNC(cellGem, cellGemUpdateFinish);
	REG_FUNC(cellGem, cellGemUpdateStart);
	REG_FUNC(cellGem, cellGemWriteExternalPort);
});
