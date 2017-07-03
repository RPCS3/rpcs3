#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAudioenc.h"

logs::channel sceAudioenc("sceAudioenc");

s32 sceAudioencInitLibrary(u32 codecType, vm::ptr<SceAudioencInitParam> pInitParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencTermLibrary(u32 codecType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencCreateEncoder(vm::ptr<SceAudioencCtrl> pCtrl, u32 codecType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencDeleteEncoder(vm::ptr<SceAudioencCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencEncode(vm::ptr<SceAudioencCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencClearContext(vm::ptr<SceAudioencCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencGetOptInfo(vm::ptr<SceAudioencCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioencGetInternalError(vm::ptr<SceAudioencCtrl> pCtrl, vm::ptr<s32> pInternalError)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceAudioencUser, nid, name)

DECLARE(arm_module_manager::SceAudioenc)("SceAudioencUser", []()
{
	REG_FUNC(0x76EE4DC6, sceAudioencInitLibrary);
	REG_FUNC(0xAB32D022, sceAudioencTermLibrary);
	REG_FUNC(0x64C04AE8, sceAudioencCreateEncoder);
	REG_FUNC(0xC6BA5EE6, sceAudioencDeleteEncoder);
	REG_FUNC(0xD85DB29C, sceAudioencEncode);
	REG_FUNC(0x9386F42D, sceAudioencClearContext);
	REG_FUNC(0xD01C63A3, sceAudioencGetOptInfo);
	REG_FUNC(0x452246D0, sceAudioencGetInternalError);
});
