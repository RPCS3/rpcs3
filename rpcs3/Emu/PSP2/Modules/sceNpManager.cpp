#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceNpManager.h"

logs::channel sceNpManager("sceNpManager");

s32 sceNpInit(vm::cptr<SceNpCommunicationConfig> commConf, vm::ptr<SceNpOptParam> opt)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpTerm(ARMv7Thread&)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpCheckCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpGetServiceState(vm::ptr<s32> state)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpRegisterServiceStateCallback(vm::ptr<SceNpServiceStateCallback> callback, vm::ptr<void> userdata)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpUnregisterServiceStateCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpManagerGetNpId(vm::ptr<SceNpId> npId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpManagerGetAccountRegion(vm::ptr<SceNpCountryCode> countryCode, vm::ptr<s32> languageCode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpManagerGetContentRatingFlag(vm::ptr<s32> isRestricted, vm::ptr<s32> age)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpManagerGetChatRestrictionFlag(vm::ptr<s32> isRestricted)
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceNpManager, nid, name)

DECLARE(arm_module_manager::SceNpManager)("SceNpManager", []()
{
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
