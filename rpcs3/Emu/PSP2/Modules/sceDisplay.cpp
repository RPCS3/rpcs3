#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceDisplay.h"

logs::channel sceDisplay("sceDisplay");

s32 sceDisplayGetRefreshRate(vm::ptr<float> pFps)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplaySetFrameBuf(vm::cptr<SceDisplayFrameBuf> pFrameBuf, s32 iUpdateTimingMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayGetFrameBuf(vm::ptr<SceDisplayFrameBuf> pFrameBuf, s32 iUpdateTimingMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayGetVcount()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitVblankStart()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitVblankStartCB()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitVblankStartMulti(u32 vcount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitVblankStartMultiCB(u32 vcount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitSetFrameBuf()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitSetFrameBufCB()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitSetFrameBufMulti(u32 vcount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayWaitSetFrameBufMultiCB(u32 vcount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayRegisterVblankStartCallback(s32 uid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDisplayUnregisterVblankStartCallback(s32 uid)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceDisplay, nid, name)

DECLARE(arm_module_manager::SceDisplayUser)("SceDisplayUser", []()
{
	REG_FNID(SceDisplayUser, 0x7A410B64, sceDisplaySetFrameBuf);
	REG_FNID(SceDisplayUser, 0x42AE6BBC, sceDisplayGetFrameBuf);
});

DECLARE(arm_module_manager::SceDisplay)("SceDisplay", []()
{
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
