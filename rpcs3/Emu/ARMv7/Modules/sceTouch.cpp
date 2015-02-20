#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceTouch.h"

s32 sceTouchGetPanelInfo(u32 port, vm::psv::ptr<SceTouchPanelInfo> pPanelInfo)
{
	throw __FUNCTION__;
}

s32 sceTouchRead(u32 port, vm::psv::ptr<SceTouchData> pData, u32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceTouchPeek(u32 port, vm::psv::ptr<SceTouchData> pData, u32 nBufs)
{
	throw __FUNCTION__;
}

s32 sceTouchSetSamplingState(u32 port, u32 state)
{
	throw __FUNCTION__;
}

s32 sceTouchGetSamplingState(u32 port, vm::psv::ptr<u32> pState)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceTouch, #name, name)

psv_log_base sceTouch("SceTouch", []()
{
	sceTouch.on_load = nullptr;
	sceTouch.on_unload = nullptr;
	sceTouch.on_stop = nullptr;

	REG_FUNC(0x169A1D58, sceTouchRead);
	REG_FUNC(0xFF082DF0, sceTouchPeek);
	REG_FUNC(0x1B9C5D14, sceTouchSetSamplingState);
	REG_FUNC(0x26531526, sceTouchGetSamplingState);
	REG_FUNC(0x10A2CA25, sceTouchGetPanelInfo);
});
