#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpClans.h"

extern Module sceNpClans;

std::unique_ptr<SceNpClansInternal> g_sceNpClans;

s32 sceNpClansInit(vm::ptr<SceNpCommunicationId> commId, vm::ptr<SceNpCommunicationPassphrase> passphrase, vm::ptr<void> pool, vm::ptr<u32> poolSize, u32 flags)
{
	sceNpClans.Warning("sceNpClansInit(commId=*0x%x, passphrase=*0x%x, pool=*0x%x, poolSize=*0x%x, flags=0x%x)", commId, passphrase, pool, poolSize, flags);

	if (g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED;
	}

	if (flags != 0)
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	g_sceNpClans->m_bSceNpClansInitialized = true;

	return CELL_OK;
}

s32 sceNpClansTerm()
{
	sceNpClans.Warning("sceNpClansTerm()");

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	g_sceNpClans->m_bSceNpClansInitialized = false;

	return CELL_OK;
}

s32 sceNpClansCreateRequest(vm::ptr<SceNpClansRequestHandle> handle, u64 flags)
{
	sceNpClans.Todo("sceNpClansCreateRequest(handle=*0x%x, flags=0x%llx)", handle, flags);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (flags != 0)
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	return CELL_OK;
}

s32 sceNpClansDestroyRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansAbortRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansCreateClan(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<char> name, vm::cptr<char> tag, vm::ptr<u32> clanId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansDisbandClan(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetClanList(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetClanListByNpId()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSearchByProfile()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSearchByName()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansUpdateClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetMemberList()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansUpdateMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansChangeMemberRole()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansUpdateAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansJoinClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansLeaveClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansKickMember(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<u32> npid, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSendInvitation(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<u32> npid, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansCancelInvitation()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSendInvitationResponse(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<SceNpClansMessage> message, b8 accept)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSendMembershipRequest(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::ptr<SceNpClansMessage> message)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansCancelMembershipRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansSendMembershipResponse()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansGetBlacklist()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansAddBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRemoveBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRetrieveAnnouncements()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansPostAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRemoveAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansPostChallenge(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, u32 targetClan, vm::ptr<SceNpClansMessage> message, vm::ptr<SceNpClansMessageData> data, u32 duration, vm::ptr<u32> mId)
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (data)
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	return CELL_OK;
}

s32 sceNpClansRetrievePostedChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRemovePostedChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRetrieveChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpClansRemoveChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);

	if (!g_sceNpClans->m_bSceNpClansInitialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

Module sceNpClans("sceNpClans", []()
{
	g_sceNpClans = std::make_unique<SceNpClansInternal>();

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
