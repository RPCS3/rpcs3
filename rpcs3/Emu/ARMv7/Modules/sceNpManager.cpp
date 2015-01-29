#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpManager;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpManager, #name, name)

psv_log_base sceNpManager("SceNpManager", []()
{
	sceNpManager.on_load = nullptr;
	sceNpManager.on_unload = nullptr;
	sceNpManager.on_stop = nullptr;

	//REG_FUNC(0x04D9F484, sceNpInit);
	//REG_FUNC(0x19E40AE1, sceNpTerm);
	//REG_FUNC(0x3C94B4B4, sceNpManagerGetNpId);
	//REG_FUNC(0x54060DF6, sceNpGetServiceState);
	//REG_FUNC(0x44239C35, sceNpRegisterServiceStateCallback);
	//REG_FUNC(0xD9E6E56C, sceNpUnregisterServiceStateCallback);
	//REG_FUNC(0x3B0AE9A9, sceNpCheckCallback);
	//REG_FUNC(0xFE835967, sceNpManagerGetAccountRegion);
	//REG_FUNC(0xAF0073B2, sceNpManagerGetContentRatingFlag);
	//REG_FUNC(0x60C575B1, sceNpManagerGetChatRestrictionFlag);
});
