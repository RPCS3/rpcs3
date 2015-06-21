#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceDisplay.h"

s32 sceDisplayGetRefreshRate(vm::ptr<float> pFps)
{
	throw __FUNCTION__;
}

s32 sceDisplaySetFrameBuf(vm::cptr<SceDisplayFrameBuf> pFrameBuf, s32 iUpdateTimingMode)
{
	throw __FUNCTION__;
}

s32 sceDisplayGetFrameBuf(vm::ptr<SceDisplayFrameBuf> pFrameBuf, s32 iUpdateTimingMode)
{
	throw __FUNCTION__;
}

s32 sceDisplayGetVcount()
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitVblankStart()
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitVblankStartCB()
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitVblankStartMulti(u32 vcount)
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitVblankStartMultiCB(u32 vcount)
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitSetFrameBuf()
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitSetFrameBufCB()
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitSetFrameBufMulti(u32 vcount)
{
	throw __FUNCTION__;
}

s32 sceDisplayWaitSetFrameBufMultiCB(u32 vcount)
{
	throw __FUNCTION__;
}

s32 sceDisplayRegisterVblankStartCallback(s32 uid)
{
	throw __FUNCTION__;
}

s32 sceDisplayUnregisterVblankStartCallback(s32 uid)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDisplay, #name, name)

psv_log_base sceDisplay("SceDisplay", []()
{
	sceDisplay.on_load = nullptr;
	sceDisplay.on_unload = nullptr;
	sceDisplay.on_stop = nullptr;
	sceDisplay.on_error = nullptr;

	// SceDisplayUser
	REG_FUNC(0x7A410B64, sceDisplaySetFrameBuf);
	REG_FUNC(0x42AE6BBC, sceDisplayGetFrameBuf);

	// SceDisplay
	REG_FUNC(0xA08CA60D, sceDisplayGetRefreshRate);
	REG_FUNC(0xB6FDE0BA, sceDisplayGetVcount);
	REG_FUNC(0x5795E898, sceDisplayWaitVblankStart);
	REG_FUNC(0x78B41B92, sceDisplayWaitVblankStartCB);
	REG_FUNC(0xDD0A13B8, sceDisplayWaitVblankStartMulti);
	REG_FUNC(0x05F27764, sceDisplayWaitVblankStartMultiCB);
	REG_FUNC(0x9423560C, sceDisplayWaitSetFrameBuf);
	REG_FUNC(0x814C90AF, sceDisplayWaitSetFrameBufCB);
	REG_FUNC(0x7D9864A8, sceDisplayWaitSetFrameBufMulti);
	REG_FUNC(0x3E796EF5, sceDisplayWaitSetFrameBufMultiCB);
	REG_FUNC(0x6BDF4C4D, sceDisplayRegisterVblankStartCallback);
	REG_FUNC(0x98436A80, sceDisplayUnregisterVblankStartCallback);
});
