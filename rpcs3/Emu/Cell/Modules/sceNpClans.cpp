#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/NP/np_handler.h"
#include "Emu/NP/clans_client.h"

#include "sceNp.h"
#include <memory>
#include "sceNpClans.h"


LOG_CHANNEL(sceNpClans);

template<>
void fmt_class_string<SceNpClansError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SCE_NP_CLANS_SUCCESS);
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

	// Allocate space for a client somewhere
	std::shared_ptr<clan::clans_client> client = std::make_shared<clan::clans_client>();
	clans_manager.client = client;
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

	clans_manager.client.reset();
	clans_manager.is_initialized = false;

	return CELL_OK;
}

error_code sceNpClansCreateRequest(vm::ptr<SceNpClansRequestHandle> handle, u64 flags)
{
	sceNpClans.warning("sceNpClansCreateRequest(handle=*0x%x, flags=0x%x)", handle, flags);

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

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	s32 reqId = 0;
	const SceNpClansError res = clans_manager.client->create_request(reqId);
	if (res != SCE_NP_CLANS_SUCCESS)
	{
		return res;
	}

	*handle = reqId;

	return CELL_OK;
}

error_code sceNpClansDestroyRequest(SceNpClansRequestHandle handle)
{
	sceNpClans.warning("sceNpClansDestroyRequest(handle=*0x%x)", handle);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansError res = clans_manager.client->destroy_request(handle);
	if (res != SCE_NP_CLANS_SUCCESS)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpClansAbortRequest(SceNpClansRequestHandle handle)
{
	sceNpClans.warning("sceNpClansAbortRequest(handle=*0x%x)", handle);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();
	clans_manager.client->destroy_request(handle);

	return CELL_OK;
}

error_code sceNpClansCreateClan(SceNpClansRequestHandle handle, vm::cptr<char> name, vm::cptr<char> tag, vm::ptr<SceNpClanId> clanId)
{
	sceNpClans.warning("sceNpClansCreateClan(handle=*0x%x, name=%s, tag=%s, clanId=*0x%x)", handle, name, tag, clanId);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	std::string name_str;
	vm::read_string(name.addr(), SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH, name_str);

	std::string tag_str;
	vm::read_string(tag.addr(), SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH, tag_str);

	const SceNpClansError res = clans_manager.client->create_clan(nph, handle, name_str, tag_str, clanId);
	if (res != SCE_NP_CLANS_SUCCESS)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpClansDisbandClan(SceNpClansRequestHandle handle, SceNpClanId clanId)
{
	sceNpClans.warning("sceNpClansDisbandClan(handle=*0x%x, clanId=*0x%x)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!clanId)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansError res = clans_manager.client->disband_dlan(nph, handle, clanId);
	if (res != SCE_NP_CLANS_SUCCESS)
	{
		return res;
	}

	return CELL_OK;
}

error_code sceNpClansGetClanList(SceNpClansRequestHandle handle, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.warning("sceNpClansGetClanList(handle=*0x%x, paging=*0x%x, clanList=*0x%x, pageResult=*0x%x)", handle, paging, clanList, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !clanList))
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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansPagingRequest host_paging = {};
	if (paging)
	{
		host_paging = *paging;
	}

	std::vector<SceNpClansEntry> host_clanList(SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX);
	SceNpClansPagingResult host_pageResult = {};

	const SceNpClansError ret = clans_manager.client->get_clan_list(nph, handle, host_paging, host_clanList, host_pageResult);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	if (clanList && host_pageResult.count > 0)
	{
		std::memcpy(clanList.get_ptr(), host_clanList.data(), sizeof(SceNpClansEntry) * host_pageResult.count);
	}
	*pageResult = host_pageResult;

	return CELL_OK;
}

// TODO: seems to not be needed, even by the PS3..?
error_code sceNpClansGetClanListByNpId(SceNpClansRequestHandle handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpId> npid, vm::ptr<SceNpClansEntry> clanList, vm::ptr<SceNpClansPagingResult> pageResult)
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

// TODO: seems to not be needed, even by the PS3..?
error_code sceNpClansSearchByProfile(SceNpClansRequestHandle handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpClansSearchableProfile> search, vm::ptr<SceNpClansClanBasicInfo> results, vm::ptr<SceNpClansPagingResult> pageResult)
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

error_code sceNpClansSearchByName(SceNpClansRequestHandle handle, vm::cptr<SceNpClansPagingRequest> paging, vm::cptr<SceNpClansSearchableName> search, vm::ptr<SceNpClansClanBasicInfo> results, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.warning("sceNpClansSearchByName(handle=*0x%x, paging=*0x%x, search=*0x%x, results=*0x%x, pageResult=*0x%x)", handle, paging, search, results, pageResult);

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

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansPagingRequest host_paging = {};
	if (paging)
	{
		host_paging = *paging;
	}

	const SceNpClansSearchableName host_search = *search;

	std::vector<SceNpClansClanBasicInfo> host_results(SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX);
	SceNpClansPagingResult host_pageResult = {};

	const SceNpClansError ret = clans_manager.client->clan_search(handle, host_paging, host_search, host_results, host_pageResult);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	if (results && host_pageResult.count > 0)
	{
		std::memcpy(results.get_ptr(), host_results.data(), sizeof(SceNpClansClanBasicInfo) * host_pageResult.count);
	}
	*pageResult = host_pageResult;

	return CELL_OK;
}

error_code sceNpClansGetClanInfo(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::ptr<SceNpClansClanInfo> info)
{
	sceNpClans.warning("sceNpClansGetClanInfo(handle=*0x%x, clanId=*0x%x, info=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: add more checks for info
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansClanInfo host_info = {};

	const SceNpClansError ret = clans_manager.client->get_clan_info(handle, clanId, host_info);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	*info = host_info;

	return CELL_OK;
}

error_code sceNpClansUpdateClanInfo(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansUpdatableClanInfo> info)
{
	sceNpClans.warning("sceNpClansUpdateClanInfo(handle=*0x%x, clanId=*0x%x, info=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		// TODO: add more checks for info
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansUpdatableClanInfo host_info = *info;

	const SceNpClansError ret = clans_manager.client->update_clan_info(nph, handle, clanId, host_info);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansGetMemberList(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, SceNpClansMemberStatus status, vm::ptr<SceNpClansMemberEntry> memList, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.warning("sceNpClansGetMemberList(handle=*0x%x, clanId=*0x%x, paging=*0x%x, status=0x%x, memList=*0x%x, pageResult=*0x%x)", handle, clanId, paging, status, memList, pageResult);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansPagingRequest host_paging = {};
	if (paging)
	{
		host_paging = *paging;
	}

	std::vector<SceNpClansMemberEntry> host_memList_addr(SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX);
	SceNpClansPagingResult host_pageResult = {};

	const SceNpClansError ret = clans_manager.client->get_member_list(nph, handle, clanId, host_paging, status, host_memList_addr, host_pageResult);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	if (memList && host_pageResult.count > 0)
	{
		std::memcpy(memList.get_ptr(), host_memList_addr.data(), sizeof(SceNpClansMemberEntry) * host_pageResult.count);
	}
	*pageResult = host_pageResult;

	return CELL_OK;
}

error_code sceNpClansGetMemberInfo(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::ptr<SceNpClansMemberEntry> memInfo)
{
	sceNpClans.warning("sceNpClansGetMemberInfo(handle=*0x%x, clanId=*0x%x, npid=*0x%x, memInfo=*0x%x)", handle, clanId, npid, memInfo);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid || !memInfo)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	SceNpClansMemberEntry host_memInfo = {};

	const SceNpClansError ret = clans_manager.client->get_member_info(nph, handle, clanId, host_npid, host_memInfo);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	*memInfo = host_memInfo;

	return CELL_OK;
}

error_code sceNpClansUpdateMemberInfo(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansUpdatableMemberInfo> info)
{
	sceNpClans.warning("sceNpClansUpdateMemberInfo(handle=*0x%x, clanId=*0x%x, info=*0x%x)", handle, clanId, info);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansUpdatableMemberInfo host_info = *info;

	const SceNpClansError ret = clans_manager.client->update_member_info(nph, handle, clanId, host_info);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansChangeMemberRole(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, u32 role)
{
	sceNpClans.warning("sceNpClansChangeMemberRole(handle=*0x%x, clanId=*0x%x, npid=*0x%x, role=0x%x)", handle, clanId, npid, role);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid || role > SCE_NP_CLANS_ROLE_LEADER) // TODO: check if role can be 0
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	const SceNpClansError ret = clans_manager.client->change_member_role(nph, handle, clanId, host_npid, static_cast<SceNpClansMemberRole>(role));
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

// TODO: no struct currently implements `autoAccept` as a field
error_code sceNpClansGetAutoAcceptStatus(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::ptr<b8> enable)
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

// TODO: no struct currently implements `autoAccept` as a field
error_code sceNpClansUpdateAutoAcceptStatus(SceNpClansRequestHandle handle, SceNpClanId clanId, b8 enable)
{
	sceNpClans.todo("sceNpClansUpdateAutoAcceptStatus(handle=*0x%x, clanId=%d, enable=%d)", handle, clanId, enable);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansJoinClan(SceNpClansRequestHandle handle, SceNpClanId clanId)
{
	sceNpClans.warning("sceNpClansJoinClan(handle=*0x%x, clanId=*0x%x)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansError ret = clans_manager.client->join_clan(nph, handle, clanId);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansLeaveClan(SceNpClansRequestHandle handle, SceNpClanId clanId)
{
	sceNpClans.warning("sceNpClansLeaveClan(handle=*0x%x, clanId=*0x%x)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansError ret = clans_manager.client->leave_clan(nph, handle, clanId);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansKickMember(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.warning("sceNpClansKickMember(handle=*0x%x, clanId=*0x%x, npid=*0x%x, message=*0x%x)", handle, clanId, npid, message);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	SceNpClansMessage host_message = {};
	if (message)
	{
		host_message = *message;
	}

	const SceNpClansError ret = clans_manager.client->kick_member(nph, handle, clanId, host_npid, host_message);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansSendInvitation(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.warning("sceNpClansSendInvitation(handle=*0x%x, clanId=*0x%x, npid=*0x%x, message=*0x%x)", handle, clanId, npid, message);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	SceNpClansMessage host_message = {};
	if (message)
	{
		host_message = *message;
	}

	const SceNpClansError ret = clans_manager.client->send_invitation(nph, handle, clanId, host_npid, host_message);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansCancelInvitation(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid)
{
	sceNpClans.warning("sceNpClansCancelInvitation(handle=*0x%x, clanId=*0x%x, npid=*0x%x)", handle, clanId, npid);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!npid)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	const SceNpClansError ret = clans_manager.client->cancel_invitation(nph, handle, clanId, host_npid);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansSendInvitationResponse(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansMessage> message, b8 accept)
{
	sceNpClans.warning("sceNpClansSendInvitationResponse(handle=*0x%x, clanId=*0x%x, message=*0x%x, accept=%d)", handle, clanId, message, accept);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansMessage host_message = {};
	if (message)
	{
		host_message = *message;
	}

	const SceNpClansError ret = clans_manager.client->send_invitation_response(nph, handle, clanId, host_message, accept);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansSendMembershipRequest(SceNpClansRequestHandle handle, u32 clanId, vm::cptr<SceNpClansMessage> message)
{
	sceNpClans.warning("sceNpClansSendMembershipRequest(handle=*0x%x, clanId=*0x%x, message=*0x%x)", handle, clanId, message);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansMessage host_message = {};
	if (message)
	{
		host_message = *message;
	}

	const SceNpClansError ret = clans_manager.client->request_membership(nph, handle, clanId, host_message);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansCancelMembershipRequest(SceNpClansRequestHandle handle, SceNpClanId clanId)
{
	sceNpClans.warning("sceNpClansCancelMembershipRequest(handle=*0x%x, clanId=*0x%x)", handle, clanId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpClansError ret = clans_manager.client->cancel_request_membership(nph, handle, clanId);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansSendMembershipResponse(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> npid, vm::cptr<SceNpClansMessage> message, b8 allow)
{
	sceNpClans.warning("sceNpClansSendMembershipResponse(handle=*0x%x, clanId=*0x%x, npid=*0x%x, message=*0x%x, allow=%d)", handle, clanId, npid, message, allow);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_npid = *npid;

	SceNpClansMessage host_message = {};
	if (message)
	{
		host_message = *message;
	}

	const SceNpClansError ret = clans_manager.client->send_membership_response(nph, handle, clanId, host_npid, host_message, allow);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansGetBlacklist(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansBlacklistEntry> bl, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.warning("sceNpClansGetBlacklist(handle=*0x%x, clanId=*0x%x, paging=*0x%x, bl=*0x%x, pageResult=*0x%x)", handle, clanId, paging, bl, pageResult);

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

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	SceNpClansPagingRequest host_paging = {};
	if (paging)
	{
		host_paging = *paging;
	}

	std::vector<SceNpClansBlacklistEntry> host_blacklist(SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX);
	SceNpClansPagingResult host_pageResult = {};

	const SceNpClansError ret = clans_manager.client->get_blacklist(nph, handle, clanId, host_paging, host_blacklist, host_pageResult);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	if (bl && host_pageResult.count > 0)
	{
		std::memcpy(bl.get_ptr(), host_blacklist.data(), sizeof(SceNpClansBlacklistEntry) * host_pageResult.count);
	}
	*pageResult = host_pageResult;

	return CELL_OK;
}

error_code sceNpClansAddBlacklistEntry(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> member)
{
	sceNpClans.warning("sceNpClansAddBlacklistEntry(handle=*0x%x, clanId=*0x%x, member=*0x%x)", handle, clanId, member);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!member)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_member = *member;

	const SceNpClansError ret = clans_manager.client->add_blacklist_entry(nph, handle, clanId, host_member);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansRemoveBlacklistEntry(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpId> member)
{
	sceNpClans.warning("sceNpClansRemoveBlacklistEntry(handle=*0x%x, clanId=*0x%x, member=*0x%x)", handle, clanId, member);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!member)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();

	const SceNpId host_member = *member;

	const SceNpClansError ret = clans_manager.client->remove_blacklist_entry(nph, handle, clanId, host_member);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansRetrieveAnnouncements(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mlist, vm::ptr<SceNpClansPagingResult> pageResult)
{
	sceNpClans.warning("sceNpClansRetrieveAnnouncements(handle=*0x%x, clanId=*0x%x, paging=*0x%x, mlist=*0x%x, pageResult=*0x%x)", handle, clanId, paging, mlist, pageResult);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!pageResult || (paging && !mlist) || clanId == UINT32_MAX) // TODO: confirm
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

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	SceNpClansPagingRequest host_paging = {};
	if (paging)
	{
		host_paging = *paging;
	}

	std::vector<SceNpClansMessageEntry> host_announcements(SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX);
	SceNpClansPagingResult host_pageResult = {};

	const SceNpClansError ret = clans_manager.client->retrieve_announcements(nph, handle, clanId, host_paging, host_announcements, host_pageResult);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	if (mlist && host_pageResult.count > 0)
	{
		std::memcpy(mlist.get_ptr(), host_announcements.data(), sizeof(SceNpClansMessageEntry) * host_pageResult.count);
	}
	*pageResult = host_pageResult;

	return CELL_OK;
}

error_code sceNpClansPostAnnouncement(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansMessage> message, vm::cptr<SceNpClansMessageData> data, u32 duration, vm::ptr<SceNpClansMessageId> mId)
{
	sceNpClans.warning("sceNpClansPostAnnouncement(handle=*0x%x, clanId=*0x%x, message=*0x%x, data=*0x%x, duration=*0x%x, mId=*0x%x)", handle, clanId, message, data, duration, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	if (!message || !mId || duration == 0)
	{
		return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	if (strlen(message->body) > SCE_NP_CLANS_ANNOUNCEMENT_MESSAGE_BODY_MAX_LENGTH || strlen(message->subject) > SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH) // TODO: correct max?
	{
		return SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
	}

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	const SceNpClansMessage host_announcement = *message;

	SceNpClansMessageData host_data = {};
	if (data)
	{
		host_data = *data;
	}

	SceNpClansMessageId host_announcementId = 0;
	const SceNpClansError ret = clans_manager.client->post_announcement(nph, handle, clanId, host_announcement, host_data, duration, host_announcementId);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	*mId = host_announcementId;

	return CELL_OK;
}

error_code sceNpClansRemoveAnnouncement(SceNpClansRequestHandle handle, SceNpClanId clanId, SceNpClansMessageId mId)
{
	sceNpClans.warning("sceNpClansRemoveAnnouncement(handle=*0x%x, clanId=*0x%x, mId=*0x%x)", handle, clanId, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	auto& clans_manager = g_fxo->get<sce_np_clans_manager>();
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	const SceNpClansError ret = clans_manager.client->delete_announcement(nph, handle, clanId, mId);
	if (ret != SCE_NP_CLANS_SUCCESS)
	{
		return ret;
	}

	return CELL_OK;
}

error_code sceNpClansPostChallenge(SceNpClansRequestHandle handle, SceNpClanId clanId, SceNpClanId targetClan, vm::cptr<SceNpClansMessage> message, vm::cptr<SceNpClansMessageData> data, u32 duration, vm::ptr<SceNpClansMessageId> mId)
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

error_code sceNpClansRetrievePostedChallenges(SceNpClansRequestHandle handle, SceNpClanId clanId, SceNpClanId targetClan,  vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mList, vm::ptr<SceNpClansPagingResult> pageResult)
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

error_code sceNpClansRemovePostedChallenge(SceNpClansRequestHandle handle, SceNpClanId clanId, SceNpClanId targetClan, SceNpClansMessageId mId)
{
	sceNpClans.todo("sceNpClansRemovePostedChallenge(handle=*0x%x, clanId=%d, targetClan=%d, mId=%d)", handle, clanId, targetClan, mId);

	if (!g_fxo->get<sce_np_clans_manager>().is_initialized)
	{
		return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code sceNpClansRetrieveChallenges(SceNpClansRequestHandle handle, SceNpClanId clanId, vm::cptr<SceNpClansPagingRequest> paging, vm::ptr<SceNpClansMessageEntry> mList, vm::ptr<SceNpClansPagingResult> pageResult)
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
