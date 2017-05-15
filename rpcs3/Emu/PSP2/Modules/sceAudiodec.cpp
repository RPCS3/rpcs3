#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAudiodec.h"

logs::channel sceAudiodec("sceAudiodec");

s32 sceAudiodecInitLibrary(u32 codecType, vm::ptr<SceAudiodecInitParam> pInitParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecTermLibrary(u32 codecType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecCreateDecoder(vm::ptr<SceAudiodecCtrl> pCtrl, u32 codecType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecDeleteDecoder(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecDecode(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecClearContext(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudiodecGetInternalError(vm::ptr<SceAudiodecCtrl> pCtrl, vm::ptr<s32> pInternalError)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceAudiodecUser, nid, name)

DECLARE(arm_module_manager::SceAudiodec)("SceAudiodecUser", []()
{
	REG_FUNC(0x445C2CEF, sceAudiodecInitLibrary);
	REG_FUNC(0x45719B9D, sceAudiodecTermLibrary);
	REG_FUNC(0x4DFD3AAA, sceAudiodecCreateDecoder);
	REG_FUNC(0xE7A24E16, sceAudiodecDeleteDecoder);
	REG_FUNC(0xCCDABA04, sceAudiodecDecode);
	REG_FUNC(0xF72F9B64, sceAudiodecClearContext);
	REG_FUNC(0x883B0CF5, sceAudiodecGetInternalError);
});
