#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceNpUtility.h"

logs::channel sceNpUtility("sceNpUtility");

s32 sceNpLookupInit(s32 usesAsync, s32 threadPriority, s32 cpuAffinityMask, vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupTerm(ARMv7Thread&)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupCreateTitleCtx(vm::cptr<SceNpCommunicationId> titleId, vm::cptr<SceNpId> selfNpId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupDeleteTitleCtx(s32 titleCtxId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupCreateRequest(s32 titleCtxId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupDeleteRequest(s32 reqId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupAbortRequest(s32 reqId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupSetTimeout(s32 id, s32 resolveRetry, u32 resolveTimeout, u32 connTimeout, u32 sendTimeout, u32 recvTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupWaitAsync(s32 reqId, vm::ptr<s32> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupPollAsync(s32 reqId, vm::ptr<s32> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupNpId(s32 reqId, vm::cptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupNpIdAsync(s32 reqId, vm::cptr<SceNpOnlineId> onlineId, vm::ptr<SceNpId> npId, vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupUserProfile(
	s32 reqId,
	s32 avatarSizeType,
	vm::cptr<SceNpId> npId,
	vm::ptr<SceNpUserInformation> userInfo,
	vm::ptr<SceNpAboutMe> aboutMe,
	vm::ptr<SceNpMyLanguages> languages,
	vm::ptr<SceNpCountryCode> countryCode,
	vm::ptr<void> avatarImageData,
	u32 avatarImageDataMaxSize,
	vm::ptr<u32> avatarImageDataSize,
	vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupUserProfileAsync(
	s32 reqId,
	s32 avatarSizeType,
	vm::cptr<SceNpId> npId,
	vm::ptr<SceNpUserInformation> userInfo,
	vm::ptr<SceNpAboutMe> aboutMe,
	vm::ptr<SceNpMyLanguages> languages,
	vm::ptr<SceNpCountryCode> countryCode,
	vm::ptr<void> avatarImageData,
	u32 avatarImageDataMaxSize,
	vm::ptr<u32> avatarImageDataSize,
	vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupAvatarImage(s32 reqId, vm::cptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpLookupAvatarImageAsync(s32 reqId, vm::cptr<SceNpAvatarUrl> avatarUrl, vm::ptr<SceNpAvatarImage> avatarImage, vm::ptr<void> option)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBandwidthTestInitStart(s32 initPriority, s32 cpuAffinityMask)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBandwidthTestGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBandwidthTestShutdown(vm::ptr<SceNpBandwidthTestResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBandwidthTestAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceNpUtility, nid, name)

DECLARE(arm_module_manager::SceNpUtility)("SceNpUtility", []()
{
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
