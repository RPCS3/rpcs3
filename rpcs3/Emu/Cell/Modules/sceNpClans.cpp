#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNpClans.h"

LOG_CHANNEL(sceNpClans);

template<>
void fmt_class_string<SceNpClansError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_CLANS_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_CLANS_ERROR_NOT_SUPPORTED);
			STR_CASE(SCE_NP_CLANS_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_CLANS_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_CLANS_ERROR_EXCEEDS_MAX);
			STR_CASE(SCE_NP_CLANS_ERROR_BAD_RESPONSE);
			STR_CASE(SCE_NP_CLANS_ERROR_BAD_DATA);
			STR_CASE(SCE_NP_CLANS_ERROR_BAD_REQUEST);
			STR_CASE(SCE_NP_CLANS_ERROR_INVALID_SIGNATURE);
			STR_CASE(SCE_NP_CLANS_ERROR_INSUFFICIENT);
			STR_CASE(SCE_NP_CLANS_ERROR_INTERNAL_BUFFER);
			STR_CASE(SCE_NP_CLANS_ERROR_SERVER_MAINTENANCE);
			STR_CASE(SCE_NP_CLANS_ERROR_SERVER_END_OF_SERVICE);
			STR_CASE(SCE_NP_CLANS_ERROR_SERVER_BEFORE_START_OF_SERVICE);
			STR_CASE(SCE_NP_CLANS_ERROR_ABORTED);
			STR_CASE(SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_BAD_REQUEST);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_TICKET);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_SIGNATURE);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_TICKET_EXPIRED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_NPID);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_FORBIDDEN);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INTERNAL_SERVER_ERROR);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_BANNED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_BLACKLISTED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_ENVIRONMENT);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_SERVICE);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_MEMBER);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_BEFORE_HOURS);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLOSED_SERVICE);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_PERMISSION_DENIED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_LEADER_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_MEMBER_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_JOINED_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_MEMBER_STATUS_INVALID);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_DUPLICATED_CLAN_NAME);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_LEADER_CANNOT_LEAVE);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_ROLE_PRIORITY);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_ANNOUNCEMENT_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_CONFIG_MASTER_NOT_FOUND);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_DUPLICATED_CLAN_TAG);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_EXCEEDS_CREATE_CLAN_FREQUENCY);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CLAN_PASSPHRASE_INCORRECT);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_CANNOT_RECORD_BLACKLIST_ENTRY);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_ANNOUNCEMENT);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_VULGAR_WORDS_POSTED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_BLACKLIST_LIMIT_REACHED);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_BLACKLIST_ENTRY);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_INVALID_NP_MESSAGE_FORMAT);
			STR_CASE(SCE_NP_CLANS_SERVER_ERROR_FAILED_TO_SEND_NP_MESSAGE);
		}

		return unknown;
	});
}

error_code sceNpClansInit(vm::cptr<SceNpCommunicationId> commId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::ptr<void> pool, vm::ptr<u32> poolSize, u32 flags)
{
	sceNpClans.warning("sceNpClansInit(commId=*0x%x, passphrase=*0x%x, pool=*0x%x, poolSize=*0x%x, flags=0x%x)", commId, passphrase, pool, poolSize, flags);

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	if (clans_manager.is_initialized)
	{
		return SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED;
	}

	if (!commId || !passphrase)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (flags != 0)
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	clans_manager.is_initialized = true;

	return CELL_OK;
}

error_code sceNpClansTerm()
{
	sceNpClans.warning("sceNpClansTerm()");

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	if (!clans_manager.is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	clans_manager.is_initialized = false;

	return CELL_OK;
}

error_code sceNpClansCreateRequest(vm::ptr<SceNpClansRequestHandle> handle, u64 flags)
{
	sceNpClans.todo("sceNpClansCreateRequest(handle=*0x%x, flags=0x%llx)", handle, flags);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!handle)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (flags != 0)
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	return CELL_OK;
}

error_code sceNpClansDestroyRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	sceNpClans.todo("sceNpClansDestroyRequest(handle=*0x%x)", handle);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansAbortRequest(vm::ptr<SceNpClansRequestHandle> handle)
{
	sceNpClans.todo("sceNpClansAbortRequest(handle=*0x%x)", handle);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansCreateClan(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<char> name, vm::cptr<char> tag, vm::ptr<SceNpClanId> clanId)
{
	sceNpClans.todo("sceNpClansCreateClan(handle=*0x%x, name=%s, tag=%s, clanId=*0x%x)", handle, name, tag, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!name || !tag || !clanId)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (strlen(name.get_ptr()) > SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH || strlen(tag.get_ptr()) > SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH)
	{
		return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpClansDisbandClan(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId)
{
	sceNpClans.todo("sceNpClansDisbandClan(handle=*0x%x, clanId=*0x%x)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansGetClanList(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansGetClanList(handle=*0x%x, paging=*0x%x, clanList=*0x%x, pageResult=*0x%x)", handle, paging, clanList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !clanList)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansGetClanListByNpId(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpId> npid, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansGetClanListByNpId(handle=*0x%x, paging=*0x%x, npid=*0x%x, clanList=*0x%x, pageResult=*0x%x)", handle, paging, npid, clanList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid || !pageResult || (paging && !clanList)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansSearchByProfile(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpClansSearchableProfile> search, vm::ptr<SceNpClansClanBasicInfo> results, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansSearchByProfile(handle=*0x%x, paging=*0x%x, search=*0x%x, results=*0x%x, pageResult=*0x%x)", handle, paging, search, results, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!search || !pageResult || (paging && !results)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansSearchByName(vm::ptr<SceNpClansRequestHandle> handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpClansSearchableName> search, vm::ptr<SceNpClansClanBasicInfo> results, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansSearchByName(handle=*0x%x, paging=*0x%x, search=*0x%x, results=*0x%x, pageResult=*0x%x)", handle, paging, search, results, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!search || !pageResult || (paging && !results)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansGetClanInfo(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::ptr<SceNpClansClanInfo> info)
{
	sceNpClans.todo("sceNpClansGetClanInfo(handle=*0x%x, clanId=%d, info=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: add more checks for info
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansUpdateClanInfo(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansUpdatableClanInfo> info)
{
	sceNpClans.todo("sceNpClansUpdateClanInfo(handle=*0x%x, clanId=%d, info=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: add more checks for info
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	//if (info->something > X)
	//{
	//	return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	//}

	return CELL_OK;
}

error_code sceNpClansGetMemberList(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, SceNpClansMemberStatus status, vm::ptr<SceNpClansMemberEntry> memList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansGetMemberList(handle=*0x%x, clanId=%d, paging=*0x%x, status=%d, memList=*0x%x, pageResult=*0x%x)", handle, clanId, paging, status, memList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !memList)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansGetMemberInfo(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::ptr<SceNpClansMemberEntry> memInfo)
{
	sceNpClans.todo("sceNpClansGetMemberInfo(handle=*0x%x, clanId=%d, npid=*0x%x, memInfo=*0x%x)", handle, clanId, npid, memInfo);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid || !memInfo)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansUpdateMemberInfo(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansUpdatableMemberInfo> info)
{
	sceNpClans.todo("sceNpClansUpdateMemberInfo(handle=*0x%x, clanId=%d, memInfo=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: add more checks for info
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	//if (info->something > X)
	//{
	//	return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	//}

	return CELL_OK;
}

error_code sceNpClansChangeMemberRole(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, u32 role)
{
	sceNpClans.todo("sceNpClansChangeMemberRole(handle=*0x%x, clanId=%d, npid=*0x%x, role=%d)", handle, clanId, npid, role);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid || role > SCE_NP_CLANS_ROLE_LEADER) // TODO: check if role can be 0
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansGetAutoAcceptStatus(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::ptr<b8> enable)
{
	sceNpClans.todo("sceNpClansGetAutoAcceptStatus(handle=*0x%x, clanId=%d, enable=*0x%x)", handle, clanId, enable);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!enable)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansUpdateAutoAcceptStatus(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, b8 enable)
{
	sceNpClans.todo("sceNpClansUpdateAutoAcceptStatus(handle=*0x%x, clanId=%d, enable=%d)", handle, clanId, enable);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansJoinClan(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId)
{
	sceNpClans.todo("sceNpClansJoinClan(handle=*0x%x, clanId=%d)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansLeaveClan(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId)
{
	sceNpClans.todo("sceNpClansLeaveClan(handle=*0x%x, clanId=%d)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansKickMember(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.todo("sceNpClansKickMember(handle=*0x%x, clanId=%d, npid=*0x%x, message=*0x%x)", handle, clanId, npid, message);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (message)
	{
		if (strlen(message->body) > SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansSendInvitation(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.todo("sceNpClansSendInvitation(handle=*0x%x, clanId=%d, npid=*0x%x, message=*0x%x)", handle, clanId, npid, message);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (message)
	{
		if (strlen(message->body) > SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansCancelInvitation(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid)
{
	sceNpClans.todo("sceNpClansCancelInvitation(handle=*0x%x, clanId=%d, npid=*0x%x)", handle, clanId, npid);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansSendInvitationResponse(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansMessage> message, b8 accept)
{
	sceNpClans.todo("sceNpClansSendInvitationResponse(handle=*0x%x, clanId=%d, message=*0x%x, accept=%d)", handle, clanId, message, accept);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (message)
	{
		if (strlen(message->body) > SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansSendMembershipRequest(vm::ptr<SceNpClansRequestHandle> handle, u32 clanId, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.todo("sceNpClansSendMembershipRequest(handle=*0x%x, clanId=%d, message=*0x%x)", handle, clanId, message);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (message)
	{
		if (strlen(message->body) > SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansCancelMembershipRequest(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId)
{
	sceNpClans.todo("sceNpClansCancelMembershipRequest(handle=*0x%x, clanId=%d)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansSendMembershipResponse(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message, b8 allow)
{
	sceNpClans.todo("sceNpClansSendMembershipResponse(handle=*0x%x, clanId=%d, npid=*0x%x, message=*0x%x, allow=%d)", handle, clanId, npid, message, allow);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (message)
	{
		if (strlen(message->body) > SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansGetBlacklist(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansBlacklistEntry> bl, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansGetBlacklist(handle=*0x%x, clanId=%d, paging=*0x%x, bl=*0x%x, pageResult=*0x%x)", handle, clanId, paging, bl, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !bl)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansAddBlacklistEntry(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid)
{
	sceNpClans.todo("sceNpClansAddBlacklistEntry(handle=*0x%x, clanId=%d, npid=*0x%x)", handle, clanId, npid);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansRemoveBlacklistEntry(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpId> npid)
{
	sceNpClans.todo("sceNpClansRemoveBlacklistEntry(handle=*0x%x, clanId=%d, npid=*0x%x)", handle, clanId, npid);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code sceNpClansRetrieveAnnouncements(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mlist, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansRetrieveAnnouncements(handle=*0x%x, clanId=%d, paging=*0x%x, mlist=*0x%x, pageResult=*0x%x)", handle, clanId, paging, mlist, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !mlist)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansPostAnnouncement(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansMessage> message, vm::cptr<SceNpClansMessageData> data, u32 duration, vm::ptr<SceNpClansMessageId> mId)
{
	sceNpClans.todo("sceNpClansPostAnnouncement(handle=*0x%x, clanId=%d, message=*0x%x, data=*0x%x, duration=%d, mId=*0x%x)", handle, clanId, message, data, duration, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!message || !mId || duration == 0)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (!data) // TODO verify
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	if (strlen(message->body) > SCE_NP_CLANS_ANNOUNCEMENT_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
	{
		return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpClansRemoveAnnouncement(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, SceNpClansMessageId mId)
{
	sceNpClans.todo("sceNpClansPostAnnouncement(handle=*0x%x, clanId=%d, mId=%d)", handle, clanId, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansPostChallenge(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, SceNpClanId targetClan, vm::cptr<SceNpClansMessage> message, vm::cptr<SceNpClansMessageData> data, u32 duration, vm::ptr<SceNpClansMessageId> mId)
{
	sceNpClans.todo("sceNpClansPostChallenge(handle=*0x%x, clanId=%d, targetClan=%d, message=*0x%x, data=*0x%x, duration=%d, mId=*0x%x)", handle, clanId, targetClan, message, data, duration, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!message || !mId || duration == 0)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (!data) // TODO verify
	{
		return SCE_NP_CLANS_ERROR_NOT_SUPPORTED;
	}

	if (strlen(message->body) > SCE_NP_CLANS_ANNOUNCEMENT_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
	{
		return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	}

	return CELL_OK;
}

error_code sceNpClansRetrievePostedChallenges(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, SceNpClanId targetClan,  vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansRetrievePostedChallenges(handle=*0x%x, clanId=%d, targetClan=%d, paging=*0x%x, mList=*0x%x, pageResult=*0x%x)", handle, clanId, targetClan, paging, mList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !mList)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansRemovePostedChallenge(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, SceNpClanId targetClan, SceNpClansMessageId mId)
{
	sceNpClans.todo("sceNpClansRemovePostedChallenge(handle=*0x%x, clanId=%d, targetClan=%d, mId=%d)", handle, clanId, targetClan, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansRetrieveChallenges(vm::ptr<SceNpClansRequestHandle> handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.todo("sceNpClansRetrieveChallenges(handle=*0x%x, clanId=%d, paging=*0x%x, mList=*0x%x, pageResult=*0x%x)", handle, clanId, paging, mList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !mList)) // TODO: confirm
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (paging)
	{
		if (paging->startPos > SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX || paging->max > SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX)
		{
			return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}
	}

	return CELL_OK;
}

error_code sceNpClansRemoveChallenge(SceNpClansRequestHandle handle, SceNpClanId clanId, SceNpClansMessageId mId)
{
	sceNpClans.todo("sceNpClansRemoveChallenge(handle=*0x%x, clanId=%d, mId=%d)", handle, clanId, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpClans)("sceNpClans", []()
{
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
