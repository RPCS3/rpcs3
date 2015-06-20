#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceCtrl.h"

s32 sceCtrlSetSamplingMode(u32 uiMode)
{
	throw __FUNCTION__;
}

s32 sceCtrlGetSamplingMode(vm::ptr<u32> puiMode)
{
	throw __FUNCTION__;
}

s32 sceCtrlPeekBufferPositive(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceCtrlPeekBufferNegative(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceCtrlReadBufferPositive(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceCtrlReadBufferNegative(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceCtrlSetRapidFire(s32 port, s32 idx, vm::ptr<const SceCtrlRapidFireRule> pRule)
{
	throw __FUNCTION__;
}

s32 sceCtrlClearRapidFire(s32 port, s32 idx)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceCtrl, #name, name)

psv_log_base sceCtrl("SceCtrl", []()
{
	sceCtrl.on_load = nullptr;
	sceCtrl.on_unload = nullptr;
	sceCtrl.on_stop = nullptr;
	sceCtrl.on_error = nullptr;

	REG_FUNC(0xA497B150, sceCtrlSetSamplingMode);
	REG_FUNC(0xEC752AAF, sceCtrlGetSamplingMode);
	REG_FUNC(0xA9C3CED6, sceCtrlPeekBufferPositive);
	REG_FUNC(0x104ED1A7, sceCtrlPeekBufferNegative);
	REG_FUNC(0x67E7AB83, sceCtrlReadBufferPositive);
	REG_FUNC(0x15F96FB0, sceCtrlReadBufferNegative);
	REG_FUNC(0xE9CB69C8, sceCtrlSetRapidFire);
	REG_FUNC(0xD8294C9C, sceCtrlClearRapidFire);
});
