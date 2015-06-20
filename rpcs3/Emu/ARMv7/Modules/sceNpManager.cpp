#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpManager.h"

s32 sceNpInit(vm::ptr<const SceNpCommunicationConfig> commConf, vm::ptr<SceNpOptParam> opt)
{
	throw __FUNCTION__;
}

s32 sceNpTerm(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpCheckCallback()
{
	throw __FUNCTION__;
}

s32 sceNpGetServiceState(vm::ptr<SceNpServiceState> state)
{
	throw __FUNCTION__;
}

s32 sceNpRegisterServiceStateCallback(vm::ptr<SceNpServiceStateCallback> callback, vm::ptr<void> userdata)
{
	throw __FUNCTION__;
}

s32 sceNpUnregisterServiceStateCallback()
{
	throw __FUNCTION__;
}

s32 sceNpManagerGetNpId(vm::ptr<SceNpId> npId)
{
	throw __FUNCTION__;
}

s32 sceNpManagerGetAccountRegion(vm::ptr<SceNpCountryCode> countryCode, vm::ptr<s32> languageCode)
{
	throw __FUNCTION__;
}

s32 sceNpManagerGetContentRatingFlag(vm::ptr<s32> isRestricted, vm::ptr<s32> age)
{
	throw __FUNCTION__;
}

s32 sceNpManagerGetChatRestrictionFlag(vm::ptr<s32> isRestricted)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpManager, #name, name)

psv_log_base sceNpManager("SceNpManager", []()
{
	sceNpManager.on_load = nullptr;
	sceNpManager.on_unload = nullptr;
	sceNpManager.on_stop = nullptr;
	sceNpManager.on_error = nullptr;

	REG_FUNC(0x04D9F484, sceNpInit);
	REG_FUNC(0x19E40AE1, sceNpTerm);
	REG_FUNC(0x3C94B4B4, sceNpManagerGetNpId);
	REG_FUNC(0x54060DF6, sceNpGetServiceState);
	REG_FUNC(0x44239C35, sceNpRegisterServiceStateCallback);
	REG_FUNC(0xD9E6E56C, sceNpUnregisterServiceStateCallback);
	REG_FUNC(0x3B0AE9A9, sceNpCheckCallback);
	REG_FUNC(0xFE835967, sceNpManagerGetAccountRegion);
	REG_FUNC(0xAF0073B2, sceNpManagerGetContentRatingFlag);
	REG_FUNC(0x60C575B1, sceNpManagerGetChatRestrictionFlag);
});
