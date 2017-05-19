#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceSas.h"

logs::channel sceSas("sceSas");

s32 sceSasGetNeededMemorySize(vm::cptr<char> config, vm::ptr<u32> outSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasInit(vm::cptr<char> config, vm::ptr<void> buffer, u32 bufferSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasInitWithGrain(vm::cptr<char> config, u32 grain, vm::ptr<void> buffer, u32 bufferSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasExit(vm::pptr<void> outBuffer, vm::ptr<u32> outBufferSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetGrain(u32 grain)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasGetGrain()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetOutputmode(u32 outputmode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasGetOutputmode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasCore(vm::ptr<s16> out)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasCoreWithMix(vm::ptr<s16> inOut, s32 lvol, s32 rvol)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetVoice(s32 iVoiceNum, vm::cptr<void> vagBuf, u32 size, u32 loopflag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetVoicePCM(s32 iVoiceNum, vm::cptr<void> pcmBuf, u32 size, s32 loopsize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetNoise(s32 iVoiceNum, u32 uClk)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetVolume(s32 iVoiceNum, s32 l, s32 r, s32 wl, s32 wr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetPitch(s32 iVoiceNum, s32 pitch)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetADSR(s32 iVoiceNum, u32 flag, u32 ar, u32 dr, u32 sr, u32 rr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetADSRmode(s32 iVoiceNum, u32 flag, u32 am, u32 dm, u32 sm, u32 rm)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetSL(s32 iVoiceNum, u32 sl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetSimpleADSR(s32 iVoiceNum, u16 adsr1, u16 adsr2)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetKeyOn(s32 iVoiceNum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetKeyOff(s32 iVoiceNum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetPause(s32 iVoiceNum, u32 pauseFlag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasGetPauseState(s32 iVoiceNum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasGetEndState(s32 iVoiceNum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasGetEnvelope(s32 iVoiceNum)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetEffect(s32 drySwitch, s32 wetSwitch)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetEffectType(s32 type)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetEffectVolume(s32 valL, s32 valR)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSasSetEffectParam(u32 delayTime, u32 feedback)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceSas, nid, name)

DECLARE(arm_module_manager::SceSas)("SceSas", []()
{
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
