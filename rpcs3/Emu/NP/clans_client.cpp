#include "stdafx.h"

#include <util/types.hpp>

// wolfssl uses old-style casts which break clang builds
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wextern-c-compat"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif

#include <wolfssl/wolfcrypt/coding.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <Crypto/utils.h>
#include <Utilities/StrUtil.h>
#include <Utilities/StrFmt.h>

#include "Emu/Cell/Modules/sceNpClans.h"
#include "Emu/NP/clans_client.h"
#include "Emu/NP/clans_config.h"
#include "Emu/NP/np_helpers.h"

LOG_CHANNEL(clan_log, "clans");


const char* REQ_TYPE_FUNC = "func";
const char* REQ_TYPE_SEC = "sec";

constexpr const char JID_FORMAT[] = "%s@un.br.np.playstation.net";

template <>
void fmt_class_string<clan::ClanRequestType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case clan::ClanRequestType::FUNC: return "func";
			case clan::ClanRequestType::SEC:  return "sec";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<clan::ClanManagerOperationType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case clan::ClanManagerOperationType::VIEW:   return "view";
			case clan::ClanManagerOperationType::UPDATE: return "update";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<clan::ClanSearchFilterOperator>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case clan::ClanSearchFilterOperator::Equal:              return "eq";
			case clan::ClanSearchFilterOperator::NotEqual:           return "ne";
			case clan::ClanSearchFilterOperator::GreaterThan:        return "gt";
			case clan::ClanSearchFilterOperator::GreaterThanOrEqual: return "ge";
			case clan::ClanSearchFilterOperator::LessThan:           return "lt";
			case clan::ClanSearchFilterOperator::LessThanOrEqual:    return "le";
			case clan::ClanSearchFilterOperator::Like:               return "lk";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<clan::ClanRequestAction>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case clan::ClanRequestAction::GetClanList:              return "get_clan_list";
			case clan::ClanRequestAction::GetClanInfo:              return "get_clan_info";
			case clan::ClanRequestAction::GetMemberInfo:            return "get_member_info";
			case clan::ClanRequestAction::GetMemberList:            return "get_member_list";
			case clan::ClanRequestAction::GetBlacklist:             return "get_blacklist";
			case clan::ClanRequestAction::RecordBlacklistEntry:     return "record_blacklist_entry";
			case clan::ClanRequestAction::DeleteBlacklistEntry:     return "delete_blacklist_entry";
			case clan::ClanRequestAction::ClanSearch:               return "clan_search";
			case clan::ClanRequestAction::RequestMembership:        return "request_membership";
			case clan::ClanRequestAction::CancelRequestMembership:  return "cancel_request_membership";
			case clan::ClanRequestAction::AcceptMembershipRequest:  return "accept_membership_request";
			case clan::ClanRequestAction::DeclineMembershipRequest: return "decline_membership_request";
			case clan::ClanRequestAction::SendInvitation:           return "send_invitation";
			case clan::ClanRequestAction::CancelInvitation:         return "cancel_invitation";
			case clan::ClanRequestAction::AcceptInvitation:         return "accept_invitation";
			case clan::ClanRequestAction::DeclineInvitation:        return "decline_invitation";
			case clan::ClanRequestAction::UpdateMemberInfo:         return "update_member_info";
			case clan::ClanRequestAction::UpdateClanInfo:           return "update_clan_info";
			case clan::ClanRequestAction::JoinClan:                 return "join_clan";
			case clan::ClanRequestAction::LeaveClan:                return "leave_clan";
			case clan::ClanRequestAction::KickMember:               return "kick_member";
			case clan::ClanRequestAction::ChangeMemberRole:         return "change_member_role";
			case clan::ClanRequestAction::RetrieveAnnouncements:    return "retrieve_announcements";
			case clan::ClanRequestAction::PostAnnouncement:         return "post_announcement";
			case clan::ClanRequestAction::DeleteAnnouncement:       return "delete_announcement";
			}

			return unknown;
		});
}

namespace clan
{
    struct curl_memory
    {
		char* response;
		size_t size;
    };

	size_t clans_client::curlWriteCallback(void* data, size_t size, size_t nmemb, void* clientp)
	{
		size_t realsize = size * nmemb;
		std::vector<char>* mem = static_cast<std::vector<char>*>(clientp);

		size_t old_size = mem->size();
		mem->resize(old_size + realsize);
		memcpy(mem->data() + old_size, data, realsize);

		return realsize;
	}

	struct clan_request_ctx
	{
		clan_request_ctx()
		{
			curl = curl_easy_init();
			if (curl)
			{
				curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
			}
		}

		~clan_request_ctx()
		{
			if (curl)
			{
				curl_easy_cleanup(curl);
				curl = nullptr;
			}
		}

		CURL* curl = nullptr;

		// TODO: this was arbitrarily chosen -- see if there's a real amount
		static const u32 SCE_NP_CLANS_MAX_CTX_NUM = 16;
		
		static const u32 id_base = 0xA001;
		static const u32 id_step = 1;
		static const u32 id_count = SCE_NP_CLANS_MAX_CTX_NUM;
		SAVESTATE_INIT_POS(55);
	};

	clans_client::clans_client()
	{
		g_cfg_clans.load();
	}

	clans_client::~clans_client()
	{
		idm::clear<clan_request_ctx>();
	}

	SceNpClansError clans_client::createRequest(s32* reqId)
	{
		const s32 id = idm::make<clan_request_ctx>();

		if (id == id_manager::id_traits<clan_request_ctx>::invalid)
		{
			return SceNpClansError::SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}

		auto ctx = idm::get_unlocked<clan_request_ctx>(id);
		if (!ctx || !ctx->curl)
		{
			idm::remove<clan_request_ctx>(id);
			return SceNpClansError::SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
		}

		*reqId = id;

		return SceNpClansError::SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::destroyRequest(u32 reqId)
	{
		if (idm::remove<clan_request_ctx>(reqId))
			return SceNpClansError::SCE_NP_CLANS_SUCCESS;

		return SceNpClansError::SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	SceNpClansError clans_client::sendRequest(u32 reqId, ClanRequestAction action, ClanManagerOperationType opType, pugi::xml_document* xmlBody, pugi::xml_document* outResponse)
	{
		auto ctx = idm::get_unlocked<clan_request_ctx>(reqId);

		if (!ctx || !ctx->curl)
			return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

		CURL* curl = ctx->curl;

		ClanRequestType reqType = ClanRequestType::FUNC;
		pugi::xml_node clan = xmlBody->child("clan");
		if (clan && clan.child("ticket"))
		{
			reqType = ClanRequestType::SEC;
		}

		std::string host = g_cfg_clans.get_host();
		std::string protocol = g_cfg_clans.get_use_https() ? "https" : "http";
		std::string url = fmt::format("%s://%s/clan_manager_%s/%s/%s", protocol, host, opType, reqType, action);

		std::ostringstream oss;
		xmlBody->save(oss, "\t", 8U);

		std::string xml = oss.str();

		char err_buf[CURL_ERROR_SIZE];
		err_buf[0] = '\0';

		std::vector<char> response_buffer;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buf);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, xml.size());

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			outResponse = nullptr;
			clan_log.error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			clan_log.error("Error buffer: %s", err_buf);
			return SCE_NP_CLANS_ERROR_BAD_REQUEST;
		}

		response_buffer.push_back('\0');

		pugi::xml_parse_result xml_res = outResponse->load_string(response_buffer.data());
		if (!xml_res)
		{
			clan_log.error("XML parsing failed: %s", xml_res.description());
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;
		}

		pugi::xml_node clanResult = outResponse->child("clan");
		if (!clanResult)
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;

		pugi::xml_attribute result = clanResult.attribute("result");
		if (!result)
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;

		std::string result_str = result.as_string();
		if (result_str != "00")
			return static_cast<SceNpClansError>(0x80022800 | std::stoul(result_str, nullptr, 16));

		return SCE_NP_CLANS_SUCCESS;
	}

	std::string clans_client::getClanTicket(np::np_handler& nph)
	{
		const auto& npid = nph.get_npid();

		const char* service_id = CLANS_SERVICE_ID;
		const unsigned char* cookie = nullptr;
		const u32 cookie_size = 0;
		const char* entitlement_id = CLANS_ENTITLEMENT_ID;
		const u32 consumed_count = 0;

		nph.req_ticket(0x00020001, &npid, service_id, cookie, cookie_size, entitlement_id, consumed_count);

		np::ticket ticket = nph.get_clan_ticket();
		if (ticket.empty())
		{
			clan_log.error("Failed to get clan ticket");
			return "";
		}

		std::vector<byte> ticket_bytes(1024);
		uint32_t ticket_size = UINT32_MAX;

		Base64_Encode_NoNl(ticket.data(), ticket.size(), ticket_bytes.data(), &ticket_size);
		std::string ticket_str = std::string(reinterpret_cast<char*>(ticket_bytes.data()), ticket_size);

		return ticket_str;
	}

#pragma region Outgoing API Requests
	SceNpClansError clans_client::getClanList(np::np_handler& nph, u32 reqId, SceNpClansPagingRequest* paging, SceNpClansEntry* clanList, SceNpClansPagingResult* pageResult)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");

		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("start").text().set(paging->startPos);
		clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::GetClanList, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node list = clanResult.child("list");

		pugi::xml_attribute results = list.attribute("results");
		uint32_t results_count = results.as_uint();

		pugi::xml_attribute total = list.attribute("total");
		uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node info = list.child("info"); info; info = info.next_sibling("info"), i++)
		{
			pugi::xml_attribute id = info.attribute("id");
			uint32_t clanId = id.as_uint();

			pugi::xml_node name = info.child("name");
			std::string name_str = name.text().as_string();

			pugi::xml_node tag = info.child("tag");
			std::string tag_str = tag.text().as_string();

			pugi::xml_node role = info.child("role");
			int32_t role_int = role.text().as_uint();

			pugi::xml_node status = info.child("status");
			uint32_t status_int = status.text().as_uint();

			pugi::xml_node onlinename = info.child("onlinename");
			std::string onlinename_str = onlinename.text().as_string();

			pugi::xml_node members = info.child("members");
			uint32_t members_int = members.text().as_uint();

			SceNpClansEntry entry = SceNpClansEntry{
				.info = SceNpClansClanBasicInfo{
					.clanId = clanId,
					.numMembers = members_int,
					.name = "",
					.tag = "",
					.reserved = {0, 0},
				},
				.role = static_cast<SceNpClansMemberRole>(role_int),
				.status = static_cast<SceNpClansMemberStatus>(status_int)};

			strcpy_trunc(entry.info.name, name_str);
            strcpy_trunc(entry.info.tag, tag_str);

			clanList[i] = entry;
			i++;
		}

		*pageResult = SceNpClansPagingResult{
			.count = results_count,
			.total = total_count};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::getClanInfo(u32 reqId, SceNpClanId clanId, SceNpClansClanInfo* clanInfo)
	{
		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::GetClanInfo, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node info = clanResult.child("info");

		std::string name_str = info.child("name").text().as_string();
		std::string tag_str = info.child("tag").text().as_string();
		uint32_t members_int = info.child("members").text().as_uint();
		std::string date_created_str = info.child("date-created").text().as_string();
		std::string description_str = info.child("description").text().as_string();

		*clanInfo = SceNpClansClanInfo{
			.info = SceNpClansClanBasicInfo{
				.clanId = clanId,
				.numMembers = members_int,
				.name = "",
				.tag = "",
			},
			.updatable = SceNpClansUpdatableClanInfo{
				.description = "",
			}};

		strcpy_trunc(clanInfo->info.name, name_str);
		strcpy_trunc(clanInfo->info.tag, tag_str);
		strcpy_trunc(clanInfo->updatable.description, description_str);

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::getMemberInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMemberEntry* memInfo)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::GetMemberInfo, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node info = clanResult.child("info");

		pugi::xml_attribute jid = info.attribute("jid");
		std::string npid_str = jid.as_string();
		std::string username = fmt::split(npid_str, {"@"})[0];

		SceNpId npid;
		if (!strcmp(username.c_str(), nph.get_npid().handle.data))
		{
			npid = nph.get_npid();
		}
		else
		{
			np::string_to_npid(npid_str, npid);
		}

		pugi::xml_node role = info.child("role");
		uint32_t role_int = role.text().as_uint();

		pugi::xml_node status = info.child("status");
		uint32_t status_int = status.text().as_uint();

		pugi::xml_node description = info.child("description");
		std::string description_str = description.text().as_string();

		*memInfo = SceNpClansMemberEntry
		{
			.npid = npid,
			.role = static_cast<SceNpClansMemberRole>(role_int),
			.status = static_cast<SceNpClansMemberStatus>(status_int),
			.updatable = SceNpClansUpdatableMemberInfo{
				.description = "",
			}
		};

		strcpy_trunc(memInfo->updatable.description, description_str);

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::getMemberList(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMemberStatus /*status*/, SceNpClansMemberEntry* memList, SceNpClansPagingResult* pageResult)
	{	
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

        pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::GetMemberList, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
        pugi::xml_node list = clanResult.child("list");

        pugi::xml_attribute results = list.attribute("results");
        uint32_t results_count = results.as_uint();

        pugi::xml_attribute total = list.attribute("total");
        uint32_t total_count = total.as_uint();
        
        int i = 0;
        for (pugi::xml_node info = list.child("info"); info; info = info.next_sibling("info"))
        {
            std::string npid_str = info.attribute("jid").as_string();
			std::string username = fmt::split(npid_str, {"@"})[0];

			SceNpId npid;
			if (!strcmp(username.c_str(), nph.get_npid().handle.data))
			{
				npid = nph.get_npid();
			}
			else
			{
				np::string_to_npid(npid_str, npid);
			}

            uint32_t role_int = info.child("role").text().as_uint();
            uint32_t status_int = info.child("status").text().as_uint();
            std::string description_str = info.child("description").text().as_string();

            SceNpClansMemberEntry entry = SceNpClansMemberEntry
            {
                .npid = npid,
                .role = static_cast<SceNpClansMemberRole>(role_int),
                .status = static_cast<SceNpClansMemberStatus>(status_int),
            };

			strcpy_trunc(entry.updatable.description, description_str);
            
			memList[i] = entry;
            i++;
        }

		*pageResult = SceNpClansPagingResult
        {
			.count = results_count,
			.total = total_count
        };

		return SCE_NP_CLANS_SUCCESS;
	}

    SceNpClansError clans_client::getBlacklist(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansBlacklistEntry* bl, SceNpClansPagingResult* pageResult)
    {
        std::string ticket = getClanTicket(nph);

        pugi::xml_document doc = pugi::xml_document();
        pugi::xml_node clan = doc.append_child("clan");
        clan.append_child("ticket").text().set(ticket.c_str());
        clan.append_child("id").text().set(clanId);
        clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

        pugi::xml_document response = pugi::xml_document();
        SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::GetBlacklist, ClanManagerOperationType::VIEW, &doc, &response);

        if (clanRes != SCE_NP_CLANS_SUCCESS)
            return clanRes;

		pugi::xml_node clanResult = response.child("clan");
        pugi::xml_node list = clanResult.child("list");

        pugi::xml_attribute results = list.attribute("results");
        uint32_t results_count = results.as_uint();

        pugi::xml_attribute total = list.attribute("total");
        uint32_t total_count = total.as_uint();

        int i = 0;
        for (pugi::xml_node node = list.child("entry"); node; node = node.next_sibling("entry"))
        {
            pugi::xml_node id = node.child("jid");
            std::string npid_str = id.text().as_string();

			SceNpId npid = {};
            np::string_to_npid(npid_str.c_str(), npid);

            SceNpClansBlacklistEntry entry = SceNpClansBlacklistEntry
            {
                .entry = npid,
            };

            bl[i] = entry;
            i++;
        }

        *pageResult = SceNpClansPagingResult
        {
            .count = results_count,
            .total = total_count
        };

        return SCE_NP_CLANS_SUCCESS;
    }

	SceNpClansError clans_client::addBlacklistEntry(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::RecordBlacklistEntry, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::removeBlacklistEntry(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::DeleteBlacklistEntry, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::clanSearch(u32 reqId, SceNpClansPagingRequest* paging, SceNpClansSearchableName* search, SceNpClansClanBasicInfo* clanList, SceNpClansPagingResult* pageResult)
	{
		pugi::xml_document doc = pugi::xml_document();
        pugi::xml_node clan = doc.append_child("clan");
        clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

		pugi::xml_node filter = clan.append_child("filter");
		pugi::xml_node name = filter.append_child("name");
		
		std::string op_name = fmt::format("%s", static_cast<ClanSearchFilterOperator>(static_cast<s32>(search->nameSearchOp)));
		name.append_attribute("op").set_value(op_name.c_str());
		name.append_attribute("value").set_value(search->name);

        pugi::xml_document response = pugi::xml_document();
        SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::ClanSearch, ClanManagerOperationType::VIEW, &doc, &response);

        if (clanRes != SCE_NP_CLANS_SUCCESS)
            return clanRes;

		pugi::xml_node clanResult = response.child("clan");
        pugi::xml_node list = clanResult.child("list");

        pugi::xml_attribute results = list.attribute("results");
        uint32_t results_count = results.as_uint();

        pugi::xml_attribute total = list.attribute("total");
        uint32_t total_count = total.as_uint();

        int i = 0;
        for (pugi::xml_node node = list.child("info"); node; node = node.next_sibling("info"))
        {
            uint32_t clanId = node.attribute("id").as_uint();
            std::string name_str = node.child("name").text().as_string();
			std::string tag_str = node.child("tag").text().as_string();
			uint32_t members_int = node.child("members").text().as_uint();

			SceNpClansClanBasicInfo entry = SceNpClansClanBasicInfo
			{
				.clanId = clanId,
				.numMembers = members_int,
				.name = "",
				.tag = "",
				.reserved = {0, 0},
			};

			strcpy_trunc(entry.name, name_str);
			strcpy_trunc(entry.tag, tag_str);

            clanList[i] = entry;
            i++;
        }

        *pageResult = SceNpClansPagingResult
        {
            .count = results_count,
            .total = total_count
        };

        return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::requestMembership(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::RequestMembership, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::cancelRequestMembership(np::np_handler& nph, u32 reqId, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::CancelRequestMembership, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::sendMembershipResponse(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/, b8 allow)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, allow ? ClanRequestAction::AcceptMembershipRequest : ClanRequestAction::DeclineMembershipRequest, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::sendInvitation(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::SendInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::cancelInvitation(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::CancelInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::sendInvitationResponse(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* /*message*/, b8 accept)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, accept ? ClanRequestAction::AcceptInvitation : ClanRequestAction::DeclineInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::updateMemberInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansUpdatableMemberInfo* info)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_node role = clan.append_child("onlinename");
		role.text().set(nph.get_npid().handle.data);

		pugi::xml_node description = clan.append_child("description");
		description.text().set(info->description);
		
		pugi::xml_node status = clan.append_child("bin-attr1");

		byte binAttr1[SCE_NP_CLANS_MEMBER_BINARY_ATTRIBUTE1_MAX_SIZE * 2 + 1] = {0};
		uint32_t binAttr1Size = UINT32_MAX;
		Base64_Encode_NoNl(info->binAttr1, info->binData1Size, binAttr1, &binAttr1Size);

		if (binAttr1Size == UINT32_MAX)
			return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
		
        // `reinterpret_cast` used to let the compiler select the correct overload of `set`
		status.text().set(reinterpret_cast<const char*>(binAttr1));

		pugi::xml_node allowMsg = clan.append_child("allow-msg");
		allowMsg.text().set(static_cast<uint32_t>(info->allowMsg));

		pugi::xml_node size = clan.append_child("size");
		size.text().set(info->binData1Size);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::UpdateMemberInfo, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::updateClanInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansUpdatableClanInfo* info)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		// TODO: implement binary and integer attributes (not implemented in server yet)

		pugi::xml_node description = clan.append_child("description");
		description.text().set(info->description);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::UpdateClanInfo, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::joinClan(np::np_handler& nph, u32 reqId, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::JoinClan, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::leaveClan(np::np_handler& nph, u32 reqId, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::LeaveClan, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::kickMember(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::KickMember, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::changeMemberRole(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMemberRole role)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = fmt::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_node roleNode = clan.append_child("role");
		roleNode.text().set(static_cast<uint32_t>(role));

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::ChangeMemberRole, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clans_client::retrieveAnnouncements(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMessageEntry* announcements, SceNpClansPagingResult* pageResult)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("start").text().set(paging->startPos);
		clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::RetrieveAnnouncements, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node list = clanResult.child("list");

		pugi::xml_attribute results = list.attribute("results");
		uint32_t results_count = results.as_uint();

		pugi::xml_attribute total = list.attribute("total");
		uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node node = list.child("msg-info"); node; node = node.next_sibling("msg-info"))
		{
			pugi::xml_attribute id = node.attribute("id");
			uint32_t msgId = id.as_uint();

			std::string subject_str = node.child("subject").text().as_string();
			std::string msg_str = node.child("msg").text().as_string();
			std::string npid_str = node.child("jid").text().as_string();
			std::string msg_date = node.child("msg-date").text().as_string();

			SceNpId npid;
			std::string username = fmt::split(npid_str, {"@"})[0];

			if (!strcmp(username.c_str(), nph.get_npid().handle.data))
			{
				npid = nph.get_npid();
			}
			else
			{
				np::string_to_npid(npid_str, npid);
			}

			// TODO: implement `binData` and `fromId`

			SceNpClansMessageEntry entry = SceNpClansMessageEntry
			{
				.mId = msgId,
				.message = SceNpClansMessage {
					.subject = "",
					.body = "",
				},
				.npid = npid,
				.postedBy = clanId,
			};

			strcpy_trunc(entry.message.subject, subject_str);
			strcpy_trunc(entry.message.body, msg_str);

			announcements[i] = entry;
			i++;
		}

		*pageResult = SceNpClansPagingResult
		{
			.count = results_count,
			.total = total_count
		};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::postAnnouncement(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* announcement, SceNpClansMessageData* /*data*/, u32 duration, SceNpClansMessageId* msgId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_node subject = clan.append_child("subject");
		subject.text().set(announcement->subject);

		pugi::xml_node msg = clan.append_child("msg");
		msg.text().set(announcement->body);

		pugi::xml_node expireDate = clan.append_child("expire-date");
		expireDate.text().set(duration);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(reqId, ClanRequestAction::PostAnnouncement, ClanManagerOperationType::UPDATE, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node msgIdNode = clanResult.child("id");
		*msgId = msgIdNode.text().as_uint();

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::deleteAnnouncement(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessageId announcementId)
	{
		std::string ticket = getClanTicket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("msg-id").text().set(announcementId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(reqId, ClanRequestAction::DeleteAnnouncement, ClanManagerOperationType::UPDATE, &doc, &response);
	}
}
#pragma endregion
