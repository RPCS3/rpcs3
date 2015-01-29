#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceCamera;

struct SceCameraInfo
{
	u32 sizeThis;
	u32 wPriority;
	u32 wFormat;
	u32 wResolution;
	u32 wFramerate;
	u32 wWidth;
	u32 wHeight;
	u32 wRange;
	u32 _padding_0;
	u32 sizeIBase;
	u32 sizeUBase;
	u32 sizeVBase;
	vm::psv::ptr<void> pvIBase;
	vm::psv::ptr<void> pvUBase;
	vm::psv::ptr<void> pvVBase;
	u32 wPitch;
	u32 wBuffer;
};

struct SceCameraRead
{
	u32 sizeThis;
	s32 dwMode;
	s32 _padding_0;
	s32 dwStatus;
	u32 qwFrame;
	u32 qwTimestamp;
	u32 sizeIBase;
	u32 sizeUBase;
	u32 sizeVBase;
	vm::psv::ptr<void> pvIBase;
	vm::psv::ptr<void> pvUBase;
	vm::psv::ptr<void> pvVBase;
};

s32 sceCameraOpen(s32 devnum, vm::psv::ptr<SceCameraInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceCameraClose(s32 devnum)
{
	throw __FUNCTION__;
}

s32 sceCameraStart(s32 devnum)
{
	throw __FUNCTION__;
}

s32 sceCameraStop(s32 devnum)
{
	throw __FUNCTION__;
}

s32 sceCameraRead(s32 devnum, vm::psv::ptr<SceCameraRead> pRead)
{
	throw __FUNCTION__;
}

s32 sceCameraIsActive(s32 devnum)
{
	throw __FUNCTION__;
}

s32 sceCameraGetSaturation(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetSaturation(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetBrightness(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetBrightness(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetContrast(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetContrast(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetSharpness(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetSharpness(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetReverse(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetReverse(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetEffect(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetEffect(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetEV(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetEV(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetZoom(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetZoom(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetAntiFlicker(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetAntiFlicker(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetISO(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetISO(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetGain(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetGain(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetWhiteBalance(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetWhiteBalance(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetBacklight(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetBacklight(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraGetNightmode(s32 devnum, vm::psv::ptr<s32> pMode)
{
	throw __FUNCTION__;
}

s32 sceCameraSetNightmode(s32 devnum, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceCameraLedSwitch(s32 devnum, s32 iSwitch)
{
	throw __FUNCTION__;
}

s32 sceCameraLedBlink(s32 devnum, s32 iOnCount, s32 iOffCount, s32 iBlinkCount)
{
	throw __FUNCTION__;
}

s32 sceCameraGetNoiseReductionForDebug(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetNoiseReductionForDebug(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

s32 sceCameraGetSharpnessOffForDebug(s32 devnum, vm::psv::ptr<s32> pLevel)
{
	throw __FUNCTION__;
}

s32 sceCameraSetSharpnessOffForDebug(s32 devnum, s32 level)
{
	throw __FUNCTION__;
}

void sceCameraUseCacheMemoryForTrial(s32 isCache)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceCamera, #name, name)

psv_log_base sceCamera("SceCamera", []()
{
	sceCamera.on_load = nullptr;
	sceCamera.on_unload = nullptr;
	sceCamera.on_stop = nullptr;

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
