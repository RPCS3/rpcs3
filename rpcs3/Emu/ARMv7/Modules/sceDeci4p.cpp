#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceDeci4p;

typedef vm::psv::ptr<s32(s32 notifyId, s32 notifyCount, s32 notifyArg, vm::psv::ptr<void> pCommon)> SceKernelDeci4pCallback;

s32 sceKernelDeci4pOpen(vm::psv::ptr<const char> protoname, u32 protonum, u32 bufsize)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pClose(s32 socketid)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pRead(s32 socketid, vm::psv::ptr<void> buffer, u32 size, u32 reserved)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pWrite(s32 socketid, vm::psv::ptr<const void> buffer, u32 size, u32 reserved)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pRegisterCallback(s32 socketid, s32 cbid)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceDeci4p, #name, name)

psv_log_base sceDeci4p("SceDeci4pUserp", []()
{
	sceDeci4p.on_load = nullptr;
	sceDeci4p.on_unload = nullptr;
	sceDeci4p.on_stop = nullptr;

	REG_FUNC(0x28578FE8, sceKernelDeci4pOpen);
	REG_FUNC(0x63B0C50F, sceKernelDeci4pClose);
	REG_FUNC(0x971E1C66, sceKernelDeci4pRead);
	REG_FUNC(0xCDA3AAAC, sceKernelDeci4pWrite);
	REG_FUNC(0x73371F35, sceKernelDeci4pRegisterCallback);
});
