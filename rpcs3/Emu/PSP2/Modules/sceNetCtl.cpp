#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceNetCtl.h"

logs::channel sceNetCtl("sceNetCtl");

s32 sceNetCtlInit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

void sceNetCtlTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlCheckCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlInetGetResult(s32 eventType, vm::ptr<s32> errorCode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocGetResult(s32 eventType, vm::ptr<s32> errorCode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlInetGetInfo(s32 code, vm::ptr<SceNetCtlInfo> info)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlInetGetState(vm::ptr<s32> state)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlGetNatInfo(vm::ptr<SceNetCtlNatInfo> natinfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlInetRegisterCallback(vm::ptr<SceNetCtlCallback> func, vm::ptr<void> arg, vm::ptr<s32> cid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlInetUnregisterCallback(s32 cid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocRegisterCallback(vm::ptr<SceNetCtlCallback> func, vm::ptr<void> arg, vm::ptr<s32> cid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocUnregisterCallback(s32 cid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocGetState(vm::ptr<s32> state)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocDisconnect()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocGetPeerList(vm::ptr<u32> buflen, vm::ptr<void> buf)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCtlAdhocGetInAddr(vm::ptr<SceNetInAddr> inaddr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceNetCtl, nid, name)

DECLARE(arm_module_manager::SceNetCtl)("SceNetCtl", []()
{
	REG_FUNC(0x495CA1DB, sceNetCtlInit);
	REG_FUNC(0xCD188648, sceNetCtlTerm);
	REG_FUNC(0xDFFC3ED4, sceNetCtlCheckCallback);
	REG_FUNC(0x6B20EC02, sceNetCtlInetGetResult);
	REG_FUNC(0x7AE0ED19, sceNetCtlAdhocGetResult);
	REG_FUNC(0xB26D07F3, sceNetCtlInetGetInfo);
	REG_FUNC(0x6D26AC68, sceNetCtlInetGetState);
	REG_FUNC(0xEAEE6185, sceNetCtlInetRegisterCallback);
	REG_FUNC(0xD0C3BF3F, sceNetCtlInetUnregisterCallback);
	REG_FUNC(0x4DDD6149, sceNetCtlGetNatInfo);
	REG_FUNC(0x0961A561, sceNetCtlAdhocGetState);
	REG_FUNC(0xFFA9D594, sceNetCtlAdhocRegisterCallback);
	REG_FUNC(0xA4471E10, sceNetCtlAdhocUnregisterCallback);
	REG_FUNC(0xED43B79A, sceNetCtlAdhocDisconnect);
	REG_FUNC(0x77586C59, sceNetCtlAdhocGetPeerList);
	REG_FUNC(0x7118C99D, sceNetCtlAdhocGetInAddr);
});
