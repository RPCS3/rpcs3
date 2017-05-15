#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceMotion.h"

logs::channel sceMotion("sceMotion");

s32 sceMotionGetState(vm::ptr<SceMotionState> motionState)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionGetSensorState(vm::ptr<SceMotionSensorState> sensorState, s32 numRecords)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionGetBasicOrientation(vm::ptr<SceFVector3> basicOrientation)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionRotateYaw(const float radians)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionGetTiltCorrection()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionSetTiltCorrection(s32 setValue)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionGetDeadband()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionSetDeadband(s32 setValue)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionSetAngleThreshold(const float angle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

float sceMotionGetAngleThreshold()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionReset()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionMagnetometerOn()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionMagnetometerOff()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionGetMagnetometerState()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionStartSampling()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMotionStopSampling()
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceMotion, nid, name)

DECLARE(arm_module_manager::SceMotion)("SceMotion", []()
{
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
