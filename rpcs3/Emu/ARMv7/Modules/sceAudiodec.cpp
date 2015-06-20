#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceAudiodec.h"

s32 sceAudiodecInitLibrary(u32 codecType, vm::ptr<SceAudiodecInitParam> pInitParam)
{
	throw __FUNCTION__;
}

s32 sceAudiodecTermLibrary(u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudiodecCreateDecoder(vm::ptr<SceAudiodecCtrl> pCtrl, u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudiodecDeleteDecoder(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecDecode(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecClearContext(vm::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecGetInternalError(vm::ptr<SceAudiodecCtrl> pCtrl, vm::ptr<s32> pInternalError)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAudiodec, #name, name)

psv_log_base sceAudiodec("SceAudiodec", []()
{
	sceAudiodec.on_load = nullptr;
	sceAudiodec.on_unload = nullptr;
	sceAudiodec.on_stop = nullptr;
	sceAudiodec.on_error = nullptr;

	REG_FUNC(0x445C2CEF, sceAudiodecInitLibrary);
	REG_FUNC(0x45719B9D, sceAudiodecTermLibrary);
	REG_FUNC(0x4DFD3AAA, sceAudiodecCreateDecoder);
	REG_FUNC(0xE7A24E16, sceAudiodecDeleteDecoder);
	REG_FUNC(0xCCDABA04, sceAudiodecDecode);
	REG_FUNC(0xF72F9B64, sceAudiodecClearContext);
	REG_FUNC(0x883B0CF5, sceAudiodecGetInternalError);
});
