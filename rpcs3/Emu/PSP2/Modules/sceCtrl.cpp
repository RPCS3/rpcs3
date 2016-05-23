#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceCtrl.h"

logs::channel sceCtrl("sceCtrl", logs::level::notice);

s32 sceCtrlSetSamplingMode(u32 uiMode)
{
	throw EXCEPTION("");
}

s32 sceCtrlGetSamplingMode(vm::ptr<u32> puiMode)
{
	throw EXCEPTION("");
}

s32 sceCtrlPeekBufferPositive(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw EXCEPTION("");
}

s32 sceCtrlPeekBufferNegative(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw EXCEPTION("");
}

s32 sceCtrlReadBufferPositive(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw EXCEPTION("");
}

s32 sceCtrlReadBufferNegative(s32 port, vm::ptr<SceCtrlData> pData, s32 nBufs)
{
	throw EXCEPTION("");
}

s32 sceCtrlSetRapidFire(s32 port, s32 idx, vm::cptr<SceCtrlRapidFireRule> pRule)
{
	throw EXCEPTION("");
}

s32 sceCtrlClearRapidFire(s32 port, s32 idx)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceCtrl, nid, name)

DECLARE(arm_module_manager::SceCtrl)("SceCtrl", []()
{
	REG_FUNC(0xA497B150, sceCtrlSetSamplingMode);
	REG_FUNC(0xEC752AAF, sceCtrlGetSamplingMode);
	REG_FUNC(0xA9C3CED6, sceCtrlPeekBufferPositive);
	REG_FUNC(0x104ED1A7, sceCtrlPeekBufferNegative);
	REG_FUNC(0x67E7AB83, sceCtrlReadBufferPositive);
	REG_FUNC(0x15F96FB0, sceCtrlReadBufferNegative);
	REG_FUNC(0xE9CB69C8, sceCtrlSetRapidFire);
	REG_FUNC(0xD8294C9C, sceCtrlClearRapidFire);
});
