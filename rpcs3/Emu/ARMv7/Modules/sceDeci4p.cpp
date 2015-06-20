#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceDeci4p.h"

s32 sceKernelDeci4pOpen(vm::ptr<const char> protoname, u32 protonum, u32 bufsize)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pClose(s32 socketid)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pRead(s32 socketid, vm::ptr<void> buffer, u32 size, u32 reserved)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pWrite(s32 socketid, vm::ptr<const void> buffer, u32 size, u32 reserved)
{
	throw __FUNCTION__;
}

s32 sceKernelDeci4pRegisterCallback(s32 socketid, s32 cbid)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDeci4p, #name, name)

psv_log_base sceDeci4p("SceDeci4pUserp", []()
{
	sceDeci4p.on_load = nullptr;
	sceDeci4p.on_unload = nullptr;
	sceDeci4p.on_stop = nullptr;
	sceDeci4p.on_error = nullptr;

	REG_FUNC(0x28578FE8, sceKernelDeci4pOpen);
	REG_FUNC(0x63B0C50F, sceKernelDeci4pClose);
	REG_FUNC(0x971E1C66, sceKernelDeci4pRead);
	REG_FUNC(0xCDA3AAAC, sceKernelDeci4pWrite);
	REG_FUNC(0x73371F35, sceKernelDeci4pRegisterCallback);
});
