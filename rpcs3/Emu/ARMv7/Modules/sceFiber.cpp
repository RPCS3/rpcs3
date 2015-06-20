#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceFiber.h"

s32 _sceFiberInitializeImpl(vm::ptr<SceFiber> fiber, vm::ptr<const char> name, vm::ptr<SceFiberEntry> entry, u32 argOnInitialize, vm::ptr<void> addrContext, u32 sizeContext, vm::ptr<const SceFiberOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceFiberOptParamInitialize(vm::ptr<SceFiberOptParam> optParam)
{
	throw __FUNCTION__;
}

s32 sceFiberFinalize(vm::ptr<SceFiber> fiber)
{
	throw __FUNCTION__;
}

s32 sceFiberRun(vm::ptr<SceFiber> fiber, u32 argOnRunTo, vm::ptr<u32> argOnReturn)
{
	throw __FUNCTION__;
}

s32 sceFiberSwitch(vm::ptr<SceFiber> fiber, u32 argOnRunTo, vm::ptr<u32> argOnRun)
{
	throw __FUNCTION__;
}

s32 sceFiberGetSelf(vm::pptr<SceFiber> fiber)
{
	throw __FUNCTION__;
}

s32 sceFiberReturnToThread(u32 argOnReturn, vm::ptr<u32> argOnRun)
{
	throw __FUNCTION__;
}

s32 sceFiberGetInfo(vm::ptr<SceFiber> fiber, vm::ptr<SceFiberInfo> fiberInfo)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceFiber, #name, name)

psv_log_base sceFiber("SceFiber", []()
{
	sceFiber.on_load = nullptr;
	sceFiber.on_unload = nullptr;
	sceFiber.on_stop = nullptr;
	sceFiber.on_error = nullptr;

	REG_FUNC(0xF24A298C, _sceFiberInitializeImpl);
	//REG_FUNC(0xC6A3F9BB, _sceFiberInitializeWithInternalOptionImpl);
	//REG_FUNC(0x7D0C7DDB, _sceFiberAttachContextAndRun);
	//REG_FUNC(0xE00B9AFE, _sceFiberAttachContextAndSwitch);
	REG_FUNC(0x801AB334, sceFiberOptParamInitialize);
	REG_FUNC(0xE160F844, sceFiberFinalize);
	REG_FUNC(0x7DF23243, sceFiberRun);
	REG_FUNC(0xE4283144, sceFiberSwitch);
	REG_FUNC(0x414D8CA5, sceFiberGetSelf);
	REG_FUNC(0x3B42921F, sceFiberReturnToThread);
	REG_FUNC(0x189599B4, sceFiberGetInfo);
});
