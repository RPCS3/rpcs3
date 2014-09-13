#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpClans.h"

Module *sceNpClans = nullptr;

int sceNpClansInit()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansTerm()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansCreateRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansDestroyRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansAbortRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansCreateClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansDisbandClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetClanList()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetClanListByNpId()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSearchByProfile()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSearchByName()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansUpdateClanInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetMemberList()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansUpdateMemberInfo()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansChangeMemberRole()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansUpdateAutoAcceptStatus()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansJoinClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansLeaveClan()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansKickMember()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSendInvitation()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansCancelInvitation()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSendInvitationResponse()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSendMembershipRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansCancelMembershipRequest()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansSendMembershipResponse()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansGetBlacklist()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansAddBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRemoveBlacklistEntry()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRetrieveAnnouncements()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansPostAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRemoveAnnouncement()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansPostChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRetrievePostedChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRemovePostedChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRetrieveChallenges()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

int sceNpClansRemoveChallenge()
{
	UNIMPLEMENTED_FUNC(sceNpClans);
	return CELL_OK;
}

void sceNpClans_unload()
{
	// TODO: Unload Clans module
}

void sceNpClans_init(Module *pxThis)
{
	sceNpClans = pxThis;

	sceNpClans->AddFunc(0x9b820047, sceNpClansInit);
	sceNpClans->AddFunc(0x42332cb7, sceNpClansTerm);
	sceNpClans->AddFunc(0x9a72232d, sceNpClansCreateRequest);
	sceNpClans->AddFunc(0xd6551cd1, sceNpClansDestroyRequest);
	sceNpClans->AddFunc(0xe82969e2, sceNpClansAbortRequest);
	sceNpClans->AddFunc(0xa6a31a38, sceNpClansCreateClan);
	sceNpClans->AddFunc(0x4826f6d5, sceNpClansDisbandClan);
	sceNpClans->AddFunc(0xca4181b4, sceNpClansGetClanList);
	sceNpClans->AddFunc(0x672399a8, sceNpClansGetClanListByNpId);
	sceNpClans->AddFunc(0x1221a1bf, sceNpClansSearchByProfile);
	sceNpClans->AddFunc(0xace0cfba, sceNpClansSearchByName);
	sceNpClans->AddFunc(0x487de998, sceNpClansGetClanInfo);
	sceNpClans->AddFunc(0x09f9e1a9, sceNpClansUpdateClanInfo);
	sceNpClans->AddFunc(0x856ff5c0, sceNpClansGetMemberList);
	sceNpClans->AddFunc(0x20472da0, sceNpClansGetMemberInfo);
	sceNpClans->AddFunc(0xf4a2d52b, sceNpClansUpdateMemberInfo);
	sceNpClans->AddFunc(0x9cac2085, sceNpClansChangeMemberRole);
	sceNpClans->AddFunc(0x38dadf1f, sceNpClansGetAutoAcceptStatus);
	sceNpClans->AddFunc(0x5da94854, sceNpClansUpdateAutoAcceptStatus);
	sceNpClans->AddFunc(0xdbf300ca, sceNpClansJoinClan);
	sceNpClans->AddFunc(0x560f717b, sceNpClansLeaveClan);
	sceNpClans->AddFunc(0xaa7912b5, sceNpClansKickMember);
	sceNpClans->AddFunc(0xbc05ef31, sceNpClansSendInvitation);
	sceNpClans->AddFunc(0x726dffd5, sceNpClansCancelInvitation);
	sceNpClans->AddFunc(0x095e12c6, sceNpClansSendInvitationResponse);
	sceNpClans->AddFunc(0x59743b2b, sceNpClansSendMembershipRequest);
	sceNpClans->AddFunc(0x299ccc9b, sceNpClansCancelMembershipRequest);
	sceNpClans->AddFunc(0x942dbdc4, sceNpClansSendMembershipResponse);
	sceNpClans->AddFunc(0x56bc5a7c, sceNpClansGetBlacklist);
	sceNpClans->AddFunc(0x4d06aef7, sceNpClansAddBlacklistEntry);
	sceNpClans->AddFunc(0x5bff9da1, sceNpClansRemoveBlacklistEntry);
	sceNpClans->AddFunc(0x727aa7f8, sceNpClansRetrieveAnnouncements);
	sceNpClans->AddFunc(0xada45b84, sceNpClansPostAnnouncement);
	sceNpClans->AddFunc(0xe2590f60, sceNpClansRemoveAnnouncement);
	sceNpClans->AddFunc(0x83d65529, sceNpClansPostChallenge);
	sceNpClans->AddFunc(0x8e785b97, sceNpClansRetrievePostedChallenges);
	sceNpClans->AddFunc(0xd3346dc4, sceNpClansRemovePostedChallenge);
	sceNpClans->AddFunc(0x0df25834, sceNpClansRetrieveChallenges);
	sceNpClans->AddFunc(0xce6dc0f0, sceNpClansRemoveChallenge);
}