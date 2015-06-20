#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceMotion.h"

s32 sceMotionGetState(vm::ptr<SceMotionState> motionState)
{
	throw __FUNCTION__;
}

s32 sceMotionGetSensorState(vm::ptr<SceMotionSensorState> sensorState, s32 numRecords)
{
	throw __FUNCTION__;
}

s32 sceMotionGetBasicOrientation(vm::ptr<SceFVector3> basicOrientation)
{
	throw __FUNCTION__;
}

s32 sceMotionRotateYaw(const float radians)
{
	throw __FUNCTION__;
}

s32 sceMotionGetTiltCorrection()
{
	throw __FUNCTION__;
}

s32 sceMotionSetTiltCorrection(s32 setValue)
{
	throw __FUNCTION__;
}

s32 sceMotionGetDeadband()
{
	throw __FUNCTION__;
}

s32 sceMotionSetDeadband(s32 setValue)
{
	throw __FUNCTION__;
}

s32 sceMotionSetAngleThreshold(const float angle)
{
	throw __FUNCTION__;
}

float sceMotionGetAngleThreshold()
{
	throw __FUNCTION__;
}

s32 sceMotionReset()
{
	throw __FUNCTION__;
}

s32 sceMotionMagnetometerOn()
{
	throw __FUNCTION__;
}

s32 sceMotionMagnetometerOff()
{
	throw __FUNCTION__;
}

s32 sceMotionGetMagnetometerState()
{
	throw __FUNCTION__;
}

s32 sceMotionStartSampling()
{
	throw __FUNCTION__;
}

s32 sceMotionStopSampling()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceMotion, #name, name)

psv_log_base sceMotion("SceMotion", []()
{
	sceMotion.on_load = nullptr;
	sceMotion.on_unload = nullptr;
	sceMotion.on_stop = nullptr;
	sceMotion.on_error = nullptr;

	REG_FUNC(0xBDB32767, sceMotionGetState);
	REG_FUNC(0x47D679EA, sceMotionGetSensorState);
	REG_FUNC(0xC1652201, sceMotionGetTiltCorrection);
	REG_FUNC(0xAF09FCDB, sceMotionSetTiltCorrection);
	REG_FUNC(0x112E0EAE, sceMotionGetDeadband);
	REG_FUNC(0x917EA390, sceMotionSetDeadband);
	//REG_FUNC(0x20F00078, sceMotionRotateYaw);
	REG_FUNC(0x0FD2CDA2, sceMotionReset);
	REG_FUNC(0x28034AC9, sceMotionStartSampling);
	REG_FUNC(0xAF32CB1D, sceMotionStopSampling);
	//REG_FUNC(0xDACB2A41, sceMotionSetAngleThreshold);
	//REG_FUNC(0x499B6C87, sceMotionGetAngleThreshold);
	REG_FUNC(0x4F28BFE0, sceMotionGetBasicOrientation);
	REG_FUNC(0x122A79F8, sceMotionMagnetometerOn);
	REG_FUNC(0xC1A7395A, sceMotionMagnetometerOff);
	REG_FUNC(0x3D4813AE, sceMotionGetMagnetometerState);
});
