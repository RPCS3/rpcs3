#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellGem.h"

logs::channel cellGem("cellGem");

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
		info->port[i] = 0;
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

s32 cellGemGetState(u32 gem_num, u32 flag, u64 time_parameter, vm::ptr<CellGemState> gem_state)
{
	cellGem.todo("cellGemGetState(gem_num=%d, flag=0x%x, time=0x%llx, gem_state=*0x%x)", gem_num, flag, time_parameter, gem_state);
	const auto gem = fxm::get<gem_t>();

	if (!gem)
		return CELL_GEM_ERROR_UNINITIALIZED;

	// clear out gem_state so no games get any funny ideas about them being connected...
	std::memset(gem_state.get_ptr(), 0, sizeof(CellGemState));

	return CELL_GEM_NOT_CONNECTED;
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

DECLARE(ppu_module_manager::cellGem)("libgem", []()
{
	REG_FUNC(libgem, cellGemCalibrate);
	REG_FUNC(libgem, cellGemClearStatusFlags);
	REG_FUNC(libgem, cellGemConvertVideoFinish);
	REG_FUNC(libgem, cellGemConvertVideoStart);
	REG_FUNC(libgem, cellGemEnableCameraPitchAngleCorrection);
	REG_FUNC(libgem, cellGemEnableMagnetometer);
	REG_FUNC(libgem, cellGemEnd);
	REG_FUNC(libgem, cellGemFilterState);
	REG_FUNC(libgem, cellGemForceRGB);
	REG_FUNC(libgem, cellGemGetAccelerometerPositionInDevice);
	REG_FUNC(libgem, cellGemGetAllTrackableHues);
	REG_FUNC(libgem, cellGemGetCameraState);
	REG_FUNC(libgem, cellGemGetEnvironmentLightingColor);
	REG_FUNC(libgem, cellGemGetHuePixels);
	REG_FUNC(libgem, cellGemGetImageState);
	REG_FUNC(libgem, cellGemGetInertialState);
	REG_FUNC(libgem, cellGemGetInfo);
	REG_FUNC(libgem, cellGemGetMemorySize);
	REG_FUNC(libgem, cellGemGetRGB);
	REG_FUNC(libgem, cellGemGetRumble);
	REG_FUNC(libgem, cellGemGetState);
	REG_FUNC(libgem, cellGemGetStatusFlags);
	REG_FUNC(libgem, cellGemGetTrackerHue);
	REG_FUNC(libgem, cellGemHSVtoRGB);
	REG_FUNC(libgem, cellGemInit);
	REG_FUNC(libgem, cellGemInvalidateCalibration);
	REG_FUNC(libgem, cellGemIsTrackableHue);
	REG_FUNC(libgem, cellGemPrepareCamera);
	REG_FUNC(libgem, cellGemPrepareVideoConvert);
	REG_FUNC(libgem, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(libgem, cellGemReset);
	REG_FUNC(libgem, cellGemSetRumble);
	REG_FUNC(libgem, cellGemSetYaw);
	REG_FUNC(libgem, cellGemTrackHues);
	REG_FUNC(libgem, cellGemUpdateFinish);
	REG_FUNC(libgem, cellGemUpdateStart);
	REG_FUNC(libgem, cellGemWriteExternalPort);
});
