#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceFiber;

typedef vm::psv::ptr<void(u32 argOnInitialize, u32 argOnRun)> SceFiberEntry;

struct SceFiber
{
	static const uint size = 128;
	static const uint align = 8;
	u64 padding[size / sizeof(u64)];
};

struct SceFiberOptParam 
{
	static const uint size = 128;
	static const uint align = 8;
	u64 padding[size / sizeof(u64)];
};

struct SceFiberInfo
{
	static const uint size = 128;
	static const uint align = 8;

	union
	{
		u64 padding[size / sizeof(u64)];

		struct
		{
			SceFiberEntry entry;
			u32 argOnInitialize;
			vm::psv::ptr<void> addrContext;
			s32 sizeContext;
			char name[32];
		};
	};
};

s32 _sceFiberInitializeImpl(vm::psv::ptr<SceFiber> fiber, vm::psv::ptr<const char> name, SceFiberEntry entry, u32 argOnInitialize, vm::psv::ptr<void> addrContext, u32 sizeContext, vm::psv::ptr<const SceFiberOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceFiberOptParamInitialize(vm::psv::ptr<SceFiberOptParam> optParam)
{
	throw __FUNCTION__;
}

s32 sceFiberFinalize(vm::psv::ptr<SceFiber> fiber)
{
	throw __FUNCTION__;
}

s32 sceFiberRun(vm::psv::ptr<SceFiber> fiber, u32 argOnRunTo, vm::psv::ptr<u32> argOnReturn)
{
	throw __FUNCTION__;
}

s32 sceFiberSwitch(vm::psv::ptr<SceFiber> fiber, u32 argOnRunTo, vm::psv::ptr<u32> argOnRun)
{
	throw __FUNCTION__;
}

s32 sceFiberGetSelf(vm::psv::ptr<vm::psv::ptr<SceFiber>> fiber)
{
	throw __FUNCTION__;
}

s32 sceFiberReturnToThread(u32 argOnReturn, vm::psv::ptr<u32> argOnRun)
{
	throw __FUNCTION__;
}

s32 sceFiberGetInfo(vm::psv::ptr<SceFiber> fiber, vm::psv::ptr<SceFiberInfo> fiberInfo)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceFiber, #name, name)

psv_log_base sceFiber("SceFiber", []()
{
	sceFiber.on_load = nullptr;
	sceFiber.on_unload = nullptr;
	sceFiber.on_stop = nullptr;

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
