#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceCamera.h"

logs::channel sceCamera("sceCamera");

s32 sceCameraOpen(s32 devnum, vm::ptr<SceCameraInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraClose(s32 devnum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraStart(s32 devnum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraStop(s32 devnum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraRead(s32 devnum, vm::ptr<SceCameraRead> pRead)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraIsActive(s32 devnum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetSaturation(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetSaturation(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetBrightness(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetBrightness(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetContrast(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetContrast(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetSharpness(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetSharpness(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetReverse(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetReverse(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetEffect(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetEffect(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetEV(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetEV(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetZoom(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetZoom(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetAntiFlicker(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetAntiFlicker(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetISO(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetISO(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetGain(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetGain(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetWhiteBalance(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetWhiteBalance(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetBacklight(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetBacklight(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetNightmode(s32 devnum, vm::ptr<s32> pMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetNightmode(s32 devnum, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraLedSwitch(s32 devnum, s32 iSwitch)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraLedBlink(s32 devnum, s32 iOnCount, s32 iOffCount, s32 iBlinkCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetNoiseReductionForDebug(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetNoiseReductionForDebug(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraGetSharpnessOffForDebug(s32 devnum, vm::ptr<s32> pLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCameraSetSharpnessOffForDebug(s32 devnum, s32 level)
{
	fmt::throw_exception("Unimplemented" HERE);
}

void sceCameraUseCacheMemoryForTrial(s32 isCache)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceCamera, nid, name)

DECLARE(arm_module_manager::SceCamera)("SceCamera", []()
{
	REG_FUNC(0xA462F801, sceCameraOpen);
	REG_FUNC(0xCD6E1CFC, sceCameraClose);
	REG_FUNC(0xA8FEAE35, sceCameraStart);
	REG_FUNC(0x1DD9C9CE, sceCameraStop);
	REG_FUNC(0x79B5C2DE, sceCameraRead);
	REG_FUNC(0x103A75B8, sceCameraIsActive);
	REG_FUNC(0x624F7653, sceCameraGetSaturation);
	REG_FUNC(0xF9F7CA3D, sceCameraSetSaturation);
	REG_FUNC(0x85D5951D, sceCameraGetBrightness);
	REG_FUNC(0x98D71588, sceCameraSetBrightness);
	REG_FUNC(0x8FBE84BE, sceCameraGetContrast);
	REG_FUNC(0x06FB2900, sceCameraSetContrast);
	REG_FUNC(0xAA72C3DC, sceCameraGetSharpness);
	REG_FUNC(0xD1A5BB0B, sceCameraSetSharpness);
	REG_FUNC(0x44F6043F, sceCameraGetReverse);
	REG_FUNC(0x1175F477, sceCameraSetReverse);
	REG_FUNC(0x7E8EF3B2, sceCameraGetEffect);
	REG_FUNC(0xE9D2CFB1, sceCameraSetEffect);
	REG_FUNC(0x8B5E6147, sceCameraGetEV);
	REG_FUNC(0x62AFF0B8, sceCameraSetEV);
	REG_FUNC(0x06D3816C, sceCameraGetZoom);
	REG_FUNC(0xF7464216, sceCameraSetZoom);
	REG_FUNC(0x9FDACB99, sceCameraGetAntiFlicker);
	REG_FUNC(0xE312958A, sceCameraSetAntiFlicker);
	REG_FUNC(0x4EBD5C68, sceCameraGetISO);
	REG_FUNC(0x3CF630A1, sceCameraSetISO);
	REG_FUNC(0x2C36D6F3, sceCameraGetGain);
	REG_FUNC(0xE65CFE86, sceCameraSetGain);
	REG_FUNC(0xDBFFA1DA, sceCameraGetWhiteBalance);
	REG_FUNC(0x4D4514AC, sceCameraSetWhiteBalance);
	REG_FUNC(0x8DD1292B, sceCameraGetBacklight);
	REG_FUNC(0xAE071044, sceCameraSetBacklight);
	REG_FUNC(0x12B6FF26, sceCameraGetNightmode);
	REG_FUNC(0x3F26233E, sceCameraSetNightmode);
	REG_FUNC(0xD02CFA5C, sceCameraLedSwitch);
	REG_FUNC(0x89B16030, sceCameraLedBlink);
	REG_FUNC(0x7670474C, sceCameraUseCacheMemoryForTrial);
	REG_FUNC(0x27BB0528, sceCameraGetNoiseReductionForDebug);
	REG_FUNC(0x233C9E27, sceCameraSetNoiseReductionForDebug);
	REG_FUNC(0xC387F4DC, sceCameraGetSharpnessOffForDebug);
	REG_FUNC(0xE22C2375, sceCameraSetSharpnessOffForDebug);
});
