#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpClans.h"

extern Module sceNpClans;

struct sceNpClansInternal
{
	bool m_bSceNpClansInitialized;

	sceNpClansInternal()
		: m_bSceNpClansInitialized(false)
	{
	}
};

sceNpClansInternal sceNpClansInstance;

int sceNpClansInit(vm::ptr<SceNpCommunicationId> commId, vm::ptr<SceNpCommunicationPassphrase> passphrase, vm::ptr<void> pool, vm::ptr<u32> poolSize, u32 flags)
{
	sceNpClans.Warning("sceNpClansInit(commId_addr=0x%x, passphrase_addr=0x%x, pool_addr=0x%x,poolSize_addr=0x%x, flags=%d)", commId.addr(), passphrase.addr(), pool.addr(), poolSize.addr(), flags);

	if (sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED;

	if (flags != 0)
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;

	sceNpClansInstance.m_bSceNpClansInitialized = true;

	return CELL_OK;
}

int sceNpClansTerm()
{
	sceNpClans.Warning("sceNpClansTerm()");

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	sceNpClansInstance.m_bSceNpClansInitialized = false;

	return CELL_OK;
}

int sceNpClansCreateRequest(vm::ptr<SceNpClansRequestHandle> handle,u64 flags)
{
	sceNpClans.Todo("sceNpClansCreateRequest(handle_addr=0x%x, flags=0x%llx)", handle.addr(), flags);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	if (flags != 0)
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;

	return CELL_OK;
}

int sceNpClansDestroyRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansAbortRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansCreateClan(vm::ptr<SceNpClansRequestHandle> handle, vm::ptr<const char> name, vm::ptr<const char> tag, vm::ptr<u32> clanId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansDisbandClan(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetClanList(vm::ptr<SceNpClansRequestHandle> handle, vm::ptr<const SceNpClansPagingRequest> paging, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetClanListByNpId()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSearchByProfile()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSearchByName()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansUpdateClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetMemberList()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansUpdateMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansChangeMemberRole()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansUpdateAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansJoinClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansLeaveClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansKickMember(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<u32> npid, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSendInvitation(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<u32> npid, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansCancelInvitation()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSendInvitationResponse(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<SceNpClansMessage> message, bool accept)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSendMembershipRequest(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansCancelMembershipRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansSendMembershipResponse()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansGetBlacklist()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansAddBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansRemoveBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansRetrieveAnnouncements()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansPostAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansRemoveAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansPostChallenge(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, u32 targetClan, vm::ptr<SceNpClansMessage> message, vm::ptr<SceNpClansMessageData> data, u32 duration, vm::ptr<u32> mId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	if (data)
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;

	//todo

	return CELL_OK;
}

int sceNpClansRetrievePostedChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	//todo

	return CELL_OK;
}

int sceNpClansRemovePostedChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansRetrieveChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpClansRemoveChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!sceNpClansInstance.m_bSceNpClansInitialized)
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

Module sceNpClans("sceNpClans", []()
{
	sceNpClansInstance.m_bSceNpClansInitialized = false;

	REG_FUNC(sceNpClans, sceNpClansInit);
	REG_FUNC(sceNpClans, sceNpClansTerm);
	REG_FUNC(sceNpClans, sceNpClansCreateRequest);
	REG_FUNC(sceNpClans, sceNpClansDestroyRequest);
	REG_FUNC(sceNpClans, sceNpClansAbortRequest);
	REG_FUNC(sceNpClans, sceNpClansCreateClan);
	REG_FUNC(sceNpClans, sceNpClansDisbandClan);
	REG_FUNC(sceNpClans, sceNpClansGetClanList);
	REG_FUNC(sceNpClans, sceNpClansGetClanListByNpId);
	REG_FUNC(sceNpClans, sceNpClansSearchByProfile);
	REG_FUNC(sceNpClans, sceNpClansSearchByName);
	REG_FUNC(sceNpClans, sceNpClansGetClanInfo);
	REG_FUNC(sceNpClans, sceNpClansUpdateClanInfo);
	REG_FUNC(sceNpClans, sceNpClansGetMemberList);
	REG_FUNC(sceNpClans, sceNpClansGetMemberInfo);
	REG_FUNC(sceNpClans, sceNpClansUpdateMemberInfo);
	REG_FUNC(sceNpClans, sceNpClansChangeMemberRole);
	REG_FUNC(sceNpClans, sceNpClansGetAutoAcceptStatus);
	REG_FUNC(sceNpClans, sceNpClansUpdateAutoAcceptStatus);
	REG_FUNC(sceNpClans, sceNpClansJoinClan);
	REG_FUNC(sceNpClans, sceNpClansLeaveClan);
	REG_FUNC(sceNpClans, sceNpClansKickMember);
	REG_FUNC(sceNpClans, sceNpClansSendInvitation);
	REG_FUNC(sceNpClans, sceNpClansCancelInvitation);
	REG_FUNC(sceNpClans, sceNpClansSendInvitationResponse);
	REG_FUNC(sceNpClans, sceNpClansSendMembershipRequest);
	REG_FUNC(sceNpClans, sceNpClansCancelMembershipRequest);
	REG_FUNC(sceNpClans, sceNpClansSendMembershipResponse);
	REG_FUNC(sceNpClans, sceNpClansGetBlacklist);
	REG_FUNC(sceNpClans, sceNpClansAddBlacklistEntry);
	REG_FUNC(sceNpClans, sceNpClansRemoveBlacklistEntry);
	REG_FUNC(sceNpClans, sceNpClansRetrieveAnnouncements);
	REG_FUNC(sceNpClans, sceNpClansPostAnnouncement);
	REG_FUNC(sceNpClans, sceNpClansRemoveAnnouncement);
	REG_FUNC(sceNpClans, sceNpClansPostChallenge);
	REG_FUNC(sceNpClans, sceNpClansRetrievePostedChallenges);
	REG_FUNC(sceNpClans, sceNpClansRemovePostedChallenge);
	REG_FUNC(sceNpClans, sceNpClansRetrieveChallenges);
	REG_FUNC(sceNpClans, sceNpClansRemoveChallenge);
});
