#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceFiber.h"

logs::channel sceFiber("sceFiber");

s32 _sceFiberInitializeImpl(vm::ptr<SceFiber> fiber, vm::cptr<char> name, vm::ptr<SceFiberEntry> entry, u32 argOnInitialize, vm::ptr<void> addrContext, u32 sizeContext, vm::cptr<SceFiberOptParam> optParam, u32 buildVersion)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberOptParamInitialize(vm::ptr<SceFiberOptParam> optParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberFinalize(vm::ptr<SceFiber> fiber)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberRun(vm::ptr<SceFiber> fiber, u32 argOnRunTo, vm::ptr<u32> argOnReturn)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberSwitch(vm::ptr<SceFiber> fiber, u32 argOnRunTo, vm::ptr<u32> argOnRun)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberGetSelf(vm::pptr<SceFiber> fiber)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberReturnToThread(u32 argOnReturn, vm::ptr<u32> argOnRun)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceFiberGetInfo(vm::ptr<SceFiber> fiber, vm::ptr<SceFiberInfo> fiberInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceFiber, nid, name)

DECLARE(arm_module_manager::SceFiber)("SceFiber", []()
{
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
