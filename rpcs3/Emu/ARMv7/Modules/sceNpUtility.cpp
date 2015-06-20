#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpUtility.h"

s32 sceNpLookupInit(s32 usesAsync, s32 threadPriority, s32 cpuAffinityMask, vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupTerm(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpLookupCreateTitleCtx(vm::ptr<const SceNpCommunicationId> titleId, vm::ptr<const SceNpId> selfNpId)
{
	throw __FUNCTION__;
}

s32 sceNpLookupDeleteTitleCtx(s32 titleCtxId)
{
	throw __FUNCTION__;
}

s32 sceNpLookupCreateRequest(s32 titleCtxId)
{
	throw __FUNCTION__;
}

s32 sceNpLookupDeleteRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceNpLookupAbortRequest(s32 reqId)
{
	throw __FUNCTION__;
}

s32 sceNpLookupSetTimeout(s32 id, s32 resolveRetry, u32 resolveTimeout, u32 connTimeout, u32 sendTimeout, u32 recvTimeout)
{
	throw __FUNCTION__;
}

s32 sceNpLookupWaitAsync(s32 reqId, vm::ptr<s32> result)
{
	throw __FUNCTION__;
}

s32 sceNpLookupPollAsync(s32 reqId, vm::ptr<s32> result)
{
	throw __FUNCTION__;
}

s32 sceNpLookupNpId(s32 reqId, vm::ptr<const SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupNpIdAsync(s32 reqId, vm::ptr<const SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupUserProfile(
	s32 reqId,
	s32 avatarSizeType,
	vm::ptr<const SceNpId> npId,
	vm::ptr<SceNpUserInformation> userInfo,
	vm::ptr<SceNpAboutMe> aboutMe,
	vm::ptr<SceNpMyLanguages> languages,
	vm::ptr<SceNpCountryCode> countryCode,
	vm::ptr<void> avatarImageData,
	u32 avatarImageDataMaxSize,
	vm::ptr<u32> avatarImageDataSize,
	vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupUserProfileAsync(
	s32 reqId,
	s32 avatarSizeType,
	vm::ptr<const SceNpId> npId,
	vm::ptr<SceNpUserInformation> userInfo,
	vm::ptr<SceNpAboutMe> aboutMe,
	vm::ptr<SceNpMyLanguages> languages,
	vm::ptr<SceNpCountryCode> countryCode,
	vm::ptr<void> avatarImageData,
	u32 avatarImageDataMaxSize,
	vm::ptr<u32> avatarImageDataSize,
	vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupAvatarImage(s32 reqId, vm::ptr<const SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpLookupAvatarImageAsync(s32 reqId, vm::ptr<const SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	throw __FUNCTION__;
}

s32 sceNpBandwidthTestInitStart(s32 initPriority, s32 cpuAffinityMask)
{
	throw __FUNCTION__;
}

s32 sceNpBandwidthTestGetStatus()
{
	throw __FUNCTION__;
}

s32 sceNpBandwidthTestShutdown(vm::ptr<SceNpBandwidthTestResult> result)
{
	throw __FUNCTION__;
}

s32 sceNpBandwidthTestAbort()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpUtility, #name, name)

psv_log_base sceNpUtility("SceNpUtility", []()
{
	sceNpUtility.on_load = nullptr;
	sceNpUtility.on_unload = nullptr;
	sceNpUtility.on_stop = nullptr;
	sceNpUtility.on_error = nullptr;

	REG_FUNC(0x9246A673, sceNpLookupInit);
	REG_FUNC(0x0158B61B, sceNpLookupTerm);
	REG_FUNC(0x5110E17E, sceNpLookupCreateTitleCtx);
	REG_FUNC(0x33B64699, sceNpLookupDeleteTitleCtx);
	REG_FUNC(0x9E42E922, sceNpLookupCreateRequest);
	REG_FUNC(0x8B608BF6, sceNpLookupDeleteRequest);
	REG_FUNC(0x027587C4, sceNpLookupAbortRequest);
	REG_FUNC(0xB0C9DC45, sceNpLookupSetTimeout);
	REG_FUNC(0xCF956F23, sceNpLookupWaitAsync);
	REG_FUNC(0xFCDBA234, sceNpLookupPollAsync);
	REG_FUNC(0xB1A14879, sceNpLookupNpId);
	REG_FUNC(0x5387BABB, sceNpLookupNpIdAsync);
	REG_FUNC(0x6A1BF429, sceNpLookupUserProfile);
	REG_FUNC(0xE5285E0F, sceNpLookupUserProfileAsync);
	REG_FUNC(0xFDB0AE47, sceNpLookupAvatarImage);
	REG_FUNC(0x282BD43C, sceNpLookupAvatarImageAsync);
	REG_FUNC(0x081FA13C, sceNpBandwidthTestInitStart);
	REG_FUNC(0xE0EBFBF6, sceNpBandwidthTestGetStatus);
	REG_FUNC(0x58D92EFD, sceNpBandwidthTestShutdown);
	REG_FUNC(0x32B068C4, sceNpBandwidthTestAbort);
});
