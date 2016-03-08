#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceAudioenc.h"

s32 sceAudioencInitLibrary(u32 codecType, vm::ptr<SceAudioencInitParam> pInitParam)
{
	throw EXCEPTION("");
}

s32 sceAudioencTermLibrary(u32 codecType)
{
	throw EXCEPTION("");
}

s32 sceAudioencCreateEncoder(vm::ptr<SceAudioencCtrl> pCtrl, u32 codecType)
{
	throw EXCEPTION("");
}

s32 sceAudioencDeleteEncoder(vm::ptr<SceAudioencCtrl> pCtrl)
{
	throw EXCEPTION("");
}

s32 sceAudioencEncode(vm::ptr<SceAudioencCtrl> pCtrl)
{
	throw EXCEPTION("");
}

s32 sceAudioencClearContext(vm::ptr<SceAudioencCtrl> pCtrl)
{
	throw EXCEPTION("");
}

s32 sceAudioencGetOptInfo(vm::ptr<SceAudioencCtrl> pCtrl)
{
	throw EXCEPTION("");
}

s32 sceAudioencGetInternalError(vm::ptr<SceAudioencCtrl> pCtrl, vm::ptr<s32> pInternalError)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAudioenc, #name, name)

psv_log_base sceAudioenc("SceAudioenc", []()
{
	sceAudioenc.on_load = nullptr;
	sceAudioenc.on_unload = nullptr;
	sceAudioenc.on_stop = nullptr;
	sceAudioenc.on_error = nullptr;

	REG_FUNC(0x76EE4DC6, sceAudioencInitLibrary);
	REG_FUNC(0xAB32D022, sceAudioencTermLibrary);
	REG_FUNC(0x64C04AE8, sceAudioencCreateEncoder);
	REG_FUNC(0xC6BA5EE6, sceAudioencDeleteEncoder);
	REG_FUNC(0xD85DB29C, sceAudioencEncode);
	REG_FUNC(0x9386F42D, sceAudioencClearContext);
	REG_FUNC(0xD01C63A3, sceAudioencGetOptInfo);
	REG_FUNC(0x452246D0, sceAudioencGetInternalError);
});
