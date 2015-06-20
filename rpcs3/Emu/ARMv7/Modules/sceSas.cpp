#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceSas.h"

s32 sceSasGetNeededMemorySize(vm::ptr<const char> config, vm::ptr<u32> outSize)
{
	throw __FUNCTION__;
}

s32 sceSasInit(vm::ptr<const char> config, vm::ptr<void> buffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

s32 sceSasInitWithGrain(vm::ptr<const char> config, u32 grain, vm::ptr<void> buffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

s32 sceSasExit(vm::pptr<void> outBuffer, vm::ptr<u32> outBufferSize)
{
	throw __FUNCTION__;
}

s32 sceSasSetGrain(u32 grain)
{
	throw __FUNCTION__;
}

s32 sceSasGetGrain()
{
	throw __FUNCTION__;
}

s32 sceSasSetOutputmode(u32 outputmode)
{
	throw __FUNCTION__;
}

s32 sceSasGetOutputmode()
{
	throw __FUNCTION__;
}

s32 sceSasCore(vm::ptr<s16> out)
{
	throw __FUNCTION__;
}

s32 sceSasCoreWithMix(vm::ptr<s16> inOut, s32 lvol, s32 rvol)
{
	throw __FUNCTION__;
}

s32 sceSasSetVoice(s32 iVoiceNum, vm::ptr<const void> vagBuf, u32 size, u32 loopflag)
{
	throw __FUNCTION__;
}

s32 sceSasSetVoicePCM(s32 iVoiceNum, vm::ptr<const void> pcmBuf, u32 size, s32 loopsize)
{
	throw __FUNCTION__;
}

s32 sceSasSetNoise(s32 iVoiceNum, u32 uClk)
{
	throw __FUNCTION__;
}

s32 sceSasSetVolume(s32 iVoiceNum, s32 l, s32 r, s32 wl, s32 wr)
{
	throw __FUNCTION__;
}

s32 sceSasSetPitch(s32 iVoiceNum, s32 pitch)
{
	throw __FUNCTION__;
}

s32 sceSasSetADSR(s32 iVoiceNum, u32 flag, u32 ar, u32 dr, u32 sr, u32 rr)
{
	throw __FUNCTION__;
}

s32 sceSasSetADSRmode(s32 iVoiceNum, u32 flag, u32 am, u32 dm, u32 sm, u32 rm)
{
	throw __FUNCTION__;
}

s32 sceSasSetSL(s32 iVoiceNum, u32 sl)
{
	throw __FUNCTION__;
}

s32 sceSasSetSimpleADSR(s32 iVoiceNum, u16 adsr1, u16 adsr2)
{
	throw __FUNCTION__;
}

s32 sceSasSetKeyOn(s32 iVoiceNum)
{
	throw __FUNCTION__;
}

s32 sceSasSetKeyOff(s32 iVoiceNum)
{
	throw __FUNCTION__;
}

s32 sceSasSetPause(s32 iVoiceNum, u32 pauseFlag)
{
	throw __FUNCTION__;
}

s32 sceSasGetPauseState(s32 iVoiceNum)
{
	throw __FUNCTION__;
}

s32 sceSasGetEndState(s32 iVoiceNum)
{
	throw __FUNCTION__;
}

s32 sceSasGetEnvelope(s32 iVoiceNum)
{
	throw __FUNCTION__;
}

s32 sceSasSetEffect(s32 drySwitch, s32 wetSwitch)
{
	throw __FUNCTION__;
}

s32 sceSasSetEffectType(s32 type)
{
	throw __FUNCTION__;
}

s32 sceSasSetEffectVolume(s32 valL, s32 valR)
{
	throw __FUNCTION__;
}

s32 sceSasSetEffectParam(u32 delayTime, u32 feedback)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSas, #name, name)

psv_log_base sceSas("SceSas", []()
{
	sceSas.on_load = nullptr;
	sceSas.on_unload = nullptr;
	sceSas.on_stop = nullptr;
	sceSas.on_error = nullptr;

	//REG_FUNC(0xA2209C58, sceAsSetRegisterReportHandler);
	//REG_FUNC(0xBB635544, sceAsSetUnregisterReportHandler);
	//REG_FUNC(0xF578F0EF, sceAsGetSystemNeededMemory);
	//REG_FUNC(0xAA8D4541, sceAsCreateSystem);
	//REG_FUNC(0x139D29C0, sceAsDestroySystem);
	//REG_FUNC(0xBE843EEC, sceAsLockParam);
	//REG_FUNC(0xFF2380C4, sceAsUnlockParam);
	//REG_FUNC(0x2549F436, sceAsSetEvent);
	//REG_FUNC(0xDC26B9F2, sceAsGetState);
	//REG_FUNC(0xB6220E73, sceAsSetBuss);
	//REG_FUNC(0x1E608068, sceAsSetRacks);
	//REG_FUNC(0x5835B473, sceAsSetGranularity);
	//REG_FUNC(0xDFE6502F, sceAsGetGranularity);
	//REG_FUNC(0xC72F1EEF, sceAsRender);
	//REG_FUNC(0xCE23F057, sceAsLockUpdate);
	//REG_FUNC(0x8BEF3C92, sceAsUnlockUpdate);
	REG_FUNC(0x180C6824, sceSasGetNeededMemorySize);
	REG_FUNC(0x449B5974, sceSasInit);
	REG_FUNC(0x820D5F82, sceSasInitWithGrain);
	REG_FUNC(0xBB7D6790, sceSasExit);
	REG_FUNC(0x2B4A207C, sceSasSetGrain);
	REG_FUNC(0x2BEA45BC, sceSasGetGrain);
	REG_FUNC(0x44DDB3C4, sceSasSetOutputmode);
	REG_FUNC(0x2C36E150, sceSasGetOutputmode);
	REG_FUNC(0x7A4672B2, sceSasCore);
	REG_FUNC(0xBD496983, sceSasCoreWithMix);
	REG_FUNC(0x2B75F9BC, sceSasSetVoice);
	REG_FUNC(0xB1756EFC, sceSasSetVoicePCM);
	REG_FUNC(0xF1C63CB9, sceSasSetNoise);
	REG_FUNC(0x0BE8204D, sceSasSetVolume);
	//REG_FUNC(0x011788BE, sceSasSetDistortion);
	REG_FUNC(0x2C48A08C, sceSasSetPitch);
	REG_FUNC(0x18A5EFA2, sceSasSetADSR);
	REG_FUNC(0x5207F9D2, sceSasSetADSRmode);
	REG_FUNC(0xDE6227B8, sceSasSetSL);
	REG_FUNC(0xECCE0DB8, sceSasSetSimpleADSR);
	REG_FUNC(0xC838DB6F, sceSasSetKeyOn);
	REG_FUNC(0x5E42ADAB, sceSasSetKeyOff);
	REG_FUNC(0x59C7A9DF, sceSasSetPause);
	REG_FUNC(0x007E63E6, sceSasGetEndState);
	REG_FUNC(0xFD1A0CBF, sceSasGetPauseState);
	REG_FUNC(0x296A9910, sceSasGetEnvelope);
	REG_FUNC(0xB0444E69, sceSasSetEffect);
	REG_FUNC(0xCDF2DDD5, sceSasSetEffectType);
	REG_FUNC(0x55EDDBFA, sceSasSetEffectVolume);
	REG_FUNC(0xBAD546A0, sceSasSetEffectParam);
	//REG_FUNC(0xB6642276, sceSasGetDryPeak);
	//REG_FUNC(0x4314F0E9, sceSasGetWetPeak);
	//REG_FUNC(0x1568017A, sceSasGetPreMasterPeak);
});
