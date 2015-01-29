#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceFiber;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceFiber, #name, name)

psv_log_base sceFiber("SceFiber", []()
{
	sceFiber.on_load = nullptr;
	sceFiber.on_unload = nullptr;
	sceFiber.on_stop = nullptr;

	//REG_FUNC(0xF24A298C, _sceFiberInitializeImpl);
	//REG_FUNC(0xC6A3F9BB, _sceFiberInitializeWithInternalOptionImpl);
	//REG_FUNC(0x7D0C7DDB, _sceFiberAttachContextAndRun);
	//REG_FUNC(0xE00B9AFE, _sceFiberAttachContextAndSwitch);
	//REG_FUNC(0x801AB334, sceFiberOptParamInitialize);
	//REG_FUNC(0xE160F844, sceFiberFinalize);
	//REG_FUNC(0x7DF23243, sceFiberRun);
	//REG_FUNC(0xE4283144, sceFiberSwitch);
	//REG_FUNC(0x414D8CA5, sceFiberGetSelf);
	//REG_FUNC(0x3B42921F, sceFiberReturnToThread);
	//REG_FUNC(0x189599B4, sceFiberGetInfo);
});
