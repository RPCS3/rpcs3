#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpUtility;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpUtility, #name, name)

psv_log_base sceNpUtility("SceNpUtility", []()
{
	sceNpUtility.on_load = nullptr;
	sceNpUtility.on_unload = nullptr;
	sceNpUtility.on_stop = nullptr;

	//REG_FUNC(0x9246A673, sceNpLookupInit);
	//REG_FUNC(0x0158B61B, sceNpLookupTerm);
	//REG_FUNC(0x5110E17E, sceNpLookupCreateTitleCtx);
	//REG_FUNC(0x33B64699, sceNpLookupDeleteTitleCtx);
	//REG_FUNC(0x9E42E922, sceNpLookupCreateRequest);
	//REG_FUNC(0x8B608BF6, sceNpLookupDeleteRequest);
	//REG_FUNC(0x027587C4, sceNpLookupAbortRequest);
	//REG_FUNC(0xB0C9DC45, sceNpLookupSetTimeout);
	//REG_FUNC(0xCF956F23, sceNpLookupWaitAsync);
	//REG_FUNC(0xFCDBA234, sceNpLookupPollAsync);
	//REG_FUNC(0xB1A14879, sceNpLookupNpId);
	//REG_FUNC(0x5387BABB, sceNpLookupNpIdAsync);
	//REG_FUNC(0x6A1BF429, sceNpLookupUserProfile);
	//REG_FUNC(0xE5285E0F, sceNpLookupUserProfileAsync);
	//REG_FUNC(0xFDB0AE47, sceNpLookupAvatarImage);
	//REG_FUNC(0x282BD43C, sceNpLookupAvatarImageAsync);
	//REG_FUNC(0x081FA13C, sceNpBandwidthTestInitStart);
	//REG_FUNC(0xE0EBFBF6, sceNpBandwidthTestGetStatus);
	//REG_FUNC(0x58D92EFD, sceNpBandwidthTestShutdown);
	//REG_FUNC(0x32B068C4, sceNpBandwidthTestAbort);
});
