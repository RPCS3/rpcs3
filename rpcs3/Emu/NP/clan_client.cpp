#include "stdafx.h"
#include "util/types.hpp"

#include <Crypto/utils.h>
#include <Emu/NP/clan_client.h>
#include <format>
#include <wolfssl/wolfcrypt/coding.h>

LOG_CHANNEL(clan_log, "clans");

const char* HOST_VIEW = "clans-view01.ww.np.community.playstation.net";
const char* HOST_UPDATE = "clans-rec01.ww.np.community.playstation.net";

const char* REQ_TYPE_FUNC = "func";
const char* REQ_TYPE_SEC = "sec";

constexpr const char* JID_FORMAT = "%s@un.br.np.playstation.net";

const char* CLANS_SERVICE_ID = "IV0001-NPXS01001_00";
const char* CLANS_ENTITLEMENT_ID = "NPWR00432_00";

namespace std
{
	template <>
	struct formatter<clan::ClanRequestType> : formatter<string>
	{
		auto format(clan::ClanRequestType value, format_context& ctx) const
		{
			string_view name = "unknown";
			switch (value)
			{
			case clan::ClanRequestType::FUNC: name = "func"; break;
			case clan::ClanRequestType::SEC:  name = "sec"; break;
			}
			return formatter<string>::format(string(name), ctx);
		}
	};

	template <>
	struct formatter<clan::ClanManagerOperationType> : formatter<string>
	{
		auto format(clan::ClanManagerOperationType value, format_context& ctx) const
		{
			string_view name = "unknown";
			switch (value)
			{
			case clan::ClanManagerOperationType::VIEW:   name = "view"; break;
			case clan::ClanManagerOperationType::UPDATE: name = "update"; break;
			}
			return formatter<string>::format(string(name), ctx);
		}
	};

	template <>
	struct formatter<clan::ClanSearchFilterOperator> : formatter<string>
	{
		auto format(clan::ClanSearchFilterOperator value, format_context& ctx) const
		{
			string_view name = "unknown";
			switch (value)
			{
			case clan::ClanSearchFilterOperator::Equal:              name = "eq"; break;
			case clan::ClanSearchFilterOperator::NotEqual:           name = "ne"; break;
			case clan::ClanSearchFilterOperator::GreaterThan:        name = "gt"; break;
			case clan::ClanSearchFilterOperator::GreaterThanOrEqual: name = "ge"; break;
			case clan::ClanSearchFilterOperator::LessThan:           name = "lt"; break;
			case clan::ClanSearchFilterOperator::LessThanOrEqual:    name = "le"; break;
			case clan::ClanSearchFilterOperator::Like:               name = "lk"; break;
			}
			return formatter<string>::format(string(name), ctx);
		}
	};

	template <>
	struct formatter<clan::ClanRequestAction> : formatter<string>
	{
		auto format(clan::ClanRequestAction value, format_context& ctx) const
		{
			string_view name = "unknown";
			switch (value)
			{
			case clan::ClanRequestAction::GetClanList:              name = "get_clan_list"; break;
			case clan::ClanRequestAction::GetClanInfo:              name = "get_clan_info"; break;
			case clan::ClanRequestAction::GetMemberInfo:            name = "get_member_info"; break;
			case clan::ClanRequestAction::GetMemberList:            name = "get_member_list"; break;
			case clan::ClanRequestAction::GetBlacklist:             name = "get_blacklist"; break;
			case clan::ClanRequestAction::RecordBlacklistEntry:     name = "record_blacklist_entry"; break;
			case clan::ClanRequestAction::DeleteBlacklistEntry:     name = "delete_blacklist_entry"; break;
			case clan::ClanRequestAction::ClanSearch:               name = "clan_search"; break;
			case clan::ClanRequestAction::RequestMembership:        name = "request_membership"; break;
			case clan::ClanRequestAction::CancelRequestMembership:  name = "cancel_request_membership"; break;
			case clan::ClanRequestAction::AcceptMembershipRequest:  name = "accept_membership_request"; break;
			case clan::ClanRequestAction::DeclineMembershipRequest: name = "decline_membership_request"; break;
			case clan::ClanRequestAction::SendInvitation:           name = "send_invitation"; break;
			case clan::ClanRequestAction::CancelInvitation:         name = "cancel_invitation"; break;
			case clan::ClanRequestAction::AcceptInvitation:         name = "accept_invitation"; break;
			case clan::ClanRequestAction::DeclineInvitation:        name = "decline_invitation"; break;
			case clan::ClanRequestAction::UpdateMemberInfo:         name = "update_member_info"; break;
			case clan::ClanRequestAction::UpdateClanInfo:           name = "update_clan_info"; break;
			case clan::ClanRequestAction::JoinClan:                 name = "join_clan"; break;
			case clan::ClanRequestAction::LeaveClan:                name = "leave_clan"; break;
			case clan::ClanRequestAction::KickMember:               name = "kick_member"; break;
			case clan::ClanRequestAction::ChangeMemberRole:         name = "change_member_role"; break;
			case clan::ClanRequestAction::RetrieveAnnouncements:    name = "retrieve_announcements"; break;
			case clan::ClanRequestAction::PostAnnouncement:         name = "post_announcement"; break;
			case clan::ClanRequestAction::DeleteAnnouncement:       name = "delete_announcement"; break;
			}
			return formatter<string>::format(string(name), ctx);
		}
	};
}

namespace clan
{
    struct curl_memory
    {
	char* response;
	size_t size;
    };

	size_t clan_client::curlWriteCallback(void* data, size_t size, size_t nmemb, void* clientp)
	{
		size_t realsize = size * nmemb;
		std::vector<char>* mem = static_cast<std::vector<char>*>(clientp);

		size_t old_size = mem->size();
		mem->resize(old_size + realsize);
		memcpy(mem->data() + old_size, data, realsize);

		return realsize;
	}

	clan_client::clan_client()
	{
		createRequest();
	}

	clan_client::~clan_client()
	{
		destroyRequest();
	}

	SceNpClansError clan_client::createRequest()
	{
		if (curl)
			return SceNpClansError::SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED;

		curl = curl_easy_init();
		if (!curl)
		{
			return SceNpClansError::SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
		}

		curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

		return SceNpClansError::SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::destroyRequest()
	{
		if (curl)
		{
			curl_easy_cleanup(curl);
			curl = nullptr;
		}

		return SceNpClansError::SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::sendRequest(ClanRequestAction action, ClanManagerOperationType opType, pugi::xml_document* xmlBody, pugi::xml_document* outResponse)
	{
		if (!curl)
			return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

		ClanRequestType reqType = ClanRequestType::FUNC;
		pugi::xml_node clan = xmlBody->child("clan");
		if (clan && clan.child("ticket"))
		{
			reqType = ClanRequestType::SEC;
		}

		std::string host = opType == ClanManagerOperationType::VIEW ? HOST_VIEW : HOST_UPDATE;
		std::string url = std::format("https://{}/clan_manager_{}/{}/{}", host, opType, reqType, action);

		std::ostringstream oss;
		xmlBody->save(oss, "\t", 8U);

		std::string xml = oss.str();

		char err_buf[CURL_ERROR_SIZE];
		err_buf[0] = '\0';

		std::vector<char> response_buffer;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buf);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, xml.size());

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			outResponse = nullptr;
			clan_log.error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			clan_log.error("Error buffer: %s", err_buf);
			return SCE_NP_CLANS_ERROR_BAD_REQUEST;
		}

		response_buffer.push_back('\0');

		pugi::xml_parse_result res = outResponse->load_string(response_buffer.data());
		if (!res)
		{
			clan_log.error("XML parsing failed: %s", res.description());
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

	std::string clan_client::getClanTicket(np::np_handler& nph)
    {
		if (nph.get_ticket().size() > 0) {
			std::vector<byte> ticket_bytes(1024);
			uint32_t ticket_size = UINT32_MAX;

			Base64_Encode_NoNl(nph.get_ticket().data(), nph.get_ticket().size(), ticket_bytes.data(), &ticket_size);
			return std::string(reinterpret_cast<char*>(ticket_bytes.data()), ticket_size);
		}

        const auto& npid = nph.get_npid();

        const char* service_id = CLANS_SERVICE_ID;
        const unsigned char* cookie = nullptr;
        const u32 cookie_size = 0;
        const char* entitlement_id = CLANS_ENTITLEMENT_ID;
        const u32 consumed_count = 0;

        nph.req_ticket(0x00020001, &npid, service_id, cookie, cookie_size, entitlement_id, consumed_count);

        np::ticket ticket;

		// TODO: convert this to use events?
		int retries = 0;
		while (ticket.empty() && retries < 100) 
		{
			ticket = nph.get_ticket();
			if (ticket.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				retries++;
			}
		}

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
	SceNpClansError clan_client::getClanList(np::np_handler& nph, SceNpClansPagingRequest* paging, SceNpClansEntry* clanList, SceNpClansPagingResult* pageResult)
	{
		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");

        std::string ticket = getClanTicket(nph);
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("start").text().set(paging->startPos);
		clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(ClanRequestAction::GetClanList, ClanManagerOperationType::VIEW, &doc, &response);

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

            snprintf(entry.info.name, sizeof(entry.info.name), "%s", name_str.c_str());
            snprintf(entry.info.tag, sizeof(entry.info.tag), "%s", tag_str.c_str());

			clanList[i] = entry;
			i++;
		}

		*pageResult = SceNpClansPagingResult{
			.count = results_count,
			.total = total_count};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::getClanInfo(SceNpClanId clanId, SceNpClansClanInfo* clanInfo)
	{
		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(ClanRequestAction::GetClanInfo, ClanManagerOperationType::VIEW, &doc, &response);

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

		snprintf(clanInfo->info.name, sizeof(clanInfo->info.name), "%s", name_str.c_str());
		snprintf(clanInfo->info.tag, sizeof(clanInfo->info.tag), "%s", tag_str.c_str());
		snprintf(clanInfo->updatable.description, sizeof(clanInfo->updatable.description), "%s", description_str.c_str());

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::getMemberInfo(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMemberEntry* memInfo)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(ClanRequestAction::GetMemberInfo, ClanManagerOperationType::VIEW, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node info = clanResult.child("info");

		pugi::xml_attribute jid = info.attribute("jid");
		std::string npid_str = jid.as_string();

		char username[SCE_NET_NP_ONLINEID_MAX_LENGTH + 1] = {0};

		sscanf(npid_str.c_str(), "%16[^@]", username);

		SceNpId npid;

		if (!strcmp(username, nph.get_npid().handle.data))
		{
			npid = nph.get_npid();
		}
		else
		{
			npid = SceNpId {};
			std::strncpy(npid.handle.data, username, SCE_NET_NP_ONLINEID_MAX_LENGTH + 1);
		}

		pugi::xml_node role = info.child("role");
		uint32_t role_int = role.text().as_uint();

		pugi::xml_node status = info.child("status");
		uint32_t status_int = status.text().as_uint();

		pugi::xml_node description = info.child("description");
		std::string description_str = description.text().as_string();

		char description_char[256] = {0};
		snprintf(description_char, sizeof(description_char), "%s", description_str.c_str());

		*memInfo = SceNpClansMemberEntry
		{
			.npid = npid,
			.role = static_cast<SceNpClansMemberRole>(role_int),
			.status = static_cast<SceNpClansMemberStatus>(status_int),
			.updatable = SceNpClansUpdatableMemberInfo{
				.description = "",
			}
		};

		snprintf(memInfo->npid.handle.data, sizeof(memInfo->npid.handle.data), "%s", username);
		snprintf(memInfo->updatable.description, sizeof(memInfo->updatable.description), "%s", description_char);

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::getMemberList(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMemberStatus /*status*/, SceNpClansMemberEntry* memList, SceNpClansPagingResult* pageResult)
	{	
		std::string ticket = getClanTicket(nph);

        pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(ClanRequestAction::GetMemberList, ClanManagerOperationType::VIEW, &doc, &response);

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

			char username[SCE_NET_NP_ONLINEID_MAX_LENGTH + 1] = {0};

			sscanf(npid_str.c_str(), "%16[^@]", username);

			SceNpId npid;

			if (!strcmp(username, nph.get_npid().handle.data))
			{
				npid = nph.get_npid();
			}
			else
			{
				npid = SceNpId {};
				std::strncpy(npid.handle.data, username, SCE_NET_NP_ONLINEID_MAX_LENGTH + 1);
			}

            uint32_t role_int = info.child("role").text().as_uint();
            uint32_t status_int = info.child("status").text().as_uint();
            std::string description_str = info.child("description").text().as_string();

            char description_char[256] = {0};
            snprintf(description_char, sizeof(description_char), "%s", description_str.c_str());

            SceNpClansMemberEntry entry = SceNpClansMemberEntry
            {
                .npid = npid,
                .role = static_cast<SceNpClansMemberRole>(role_int),
                .status = static_cast<SceNpClansMemberStatus>(status_int),
            };

			snprintf(entry.updatable.description, sizeof(entry.updatable.description), "%s", description_char);
            
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

    SceNpClansError clan_client::getBlacklist(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansBlacklistEntry* bl, SceNpClansPagingResult* pageResult)
    {
        std::string ticket = getClanTicket(nph);

        pugi::xml_document doc = pugi::xml_document();
        pugi::xml_node clan = doc.append_child("clan");
        clan.append_child("ticket").text().set(ticket.c_str());
        clan.append_child("id").text().set(clanId);
        clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

        pugi::xml_document response = pugi::xml_document();
        SceNpClansError clanRes = sendRequest(ClanRequestAction::GetBlacklist, ClanManagerOperationType::VIEW, &doc, &response);

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

            char username[SCE_NET_NP_ONLINEID_MAX_LENGTH + 1] = {0};

            sscanf(npid_str.c_str(), "%16[^@]", username);

            SceNpId npid = SceNpId
            {
                .handle = SceNpOnlineId
                {
                    .data = ""
                }
            };

			snprintf(npid.handle.data, sizeof(npid.handle.data), "%s", username);

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

	SceNpClansError clan_client::addBlacklistEntry(np::np_handler& nph, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::RecordBlacklistEntry, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::removeBlacklistEntry(np::np_handler& nph, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::DeleteBlacklistEntry, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::clanSearch(SceNpClansPagingRequest* paging, SceNpClansSearchableName* search, SceNpClansClanBasicInfo* clanList, SceNpClansPagingResult* pageResult)
	{
		pugi::xml_document doc = pugi::xml_document();
        pugi::xml_node clan = doc.append_child("clan");
        clan.append_child("start").text().set(paging->startPos);
        clan.append_child("max").text().set(paging->max);

		pugi::xml_node filter = clan.append_child("filter");
		pugi::xml_node name = filter.append_child("name");
		
		std::string op_name = std::format("{}", static_cast<ClanSearchFilterOperator>(static_cast<s32>(search->nameSearchOp)));
		name.append_attribute("op").set_value(op_name.c_str());
		name.append_attribute("value").set_value(search->name);

        pugi::xml_document response = pugi::xml_document();
        SceNpClansError clanRes = sendRequest(ClanRequestAction::ClanSearch, ClanManagerOperationType::VIEW, &doc, &response);

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

			snprintf(entry.name, sizeof(entry.name), "%s", name_str.c_str());
			snprintf(entry.tag, sizeof(entry.tag), "%s", tag_str.c_str());

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

	SceNpClansError clan_client::requestMembership(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::RequestMembership, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::cancelRequestMembership(np::np_handler& nph, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::CancelRequestMembership, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::sendMembershipResponse(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/, b8 allow)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(allow ? ClanRequestAction::AcceptMembershipRequest : ClanRequestAction::DeclineMembershipRequest, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::sendInvitation(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::SendInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::cancelInvitation(np::np_handler& nph, SceNpClanId clanId, SceNpId npId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::CancelInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::sendInvitationResponse(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* /*message*/, b8 accept)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(accept ? ClanRequestAction::AcceptInvitation : ClanRequestAction::DeclineInvitation, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::updateMemberInfo(np::np_handler& nph, SceNpClanId clanId, SceNpClansUpdatableMemberInfo* info)
	{
		std::string ticket = getClanTicket(nph);

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
		return sendRequest(ClanRequestAction::UpdateMemberInfo, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::updateClanInfo(np::np_handler& nph, SceNpClanId clanId, SceNpClansUpdatableClanInfo* info)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		// TODO: implement binary and integer attributes (not implemented in server yet)

		pugi::xml_node description = clan.append_child("description");
		description.text().set(info->description);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::UpdateClanInfo, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::joinClan(np::np_handler& nph, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::JoinClan, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::leaveClan(np::np_handler& nph, SceNpClanId clanId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::LeaveClan, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::kickMember(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* /*message*/)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::KickMember, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::changeMemberRole(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMemberRole role)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		
        std::string jid_str = std::format(JID_FORMAT, npId.handle.data);
        clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_node roleNode = clan.append_child("role");
		roleNode.text().set(static_cast<uint32_t>(role));

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::ChangeMemberRole, ClanManagerOperationType::UPDATE, &doc, &response);
	}

	SceNpClansError clan_client::retrieveAnnouncements(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMessageEntry* announcements, SceNpClansPagingResult* pageResult)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("start").text().set(paging->startPos);
		clan.append_child("max").text().set(paging->max);

		pugi::xml_document response = pugi::xml_document();
		SceNpClansError clanRes = sendRequest(ClanRequestAction::RetrieveAnnouncements, ClanManagerOperationType::VIEW, &doc, &response);

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

			char username[SCE_NET_NP_ONLINEID_MAX_LENGTH + 1] = {0};
			sscanf(npid_str.c_str(), "%16[^@]", username);

			SceNpId npid;

			if (!strcmp(username, nph.get_npid().handle.data))
			{
				npid = nph.get_npid();
			}
			else
			{
				npid = SceNpId {};
				std::strncpy(npid.handle.data, username, SCE_NET_NP_ONLINEID_MAX_LENGTH + 1);
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


			strncpy(entry.message.subject, subject_str.c_str(), subject_str.size());
			strncpy(entry.message.body, msg_str.c_str(), msg_str.size());

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

	SceNpClansError clan_client::postAnnouncement(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* announcement, SceNpClansMessageData* /*data*/, u32 duration, SceNpClansMessageId* msgId)
	{
		std::string ticket = getClanTicket(nph);

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
		SceNpClansError clanRes = sendRequest(ClanRequestAction::PostAnnouncement, ClanManagerOperationType::UPDATE, &doc, &response);

		if (clanRes != SCE_NP_CLANS_SUCCESS)
			return clanRes;

		pugi::xml_node clanResult = response.child("clan");
		pugi::xml_node msgIdNode = clanResult.child("id");
		*msgId = msgIdNode.text().as_uint();

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clan_client::deleteAnnouncement(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessageId announcementId)
	{
		std::string ticket = getClanTicket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clanId);
		clan.append_child("msg-id").text().set(announcementId);

		pugi::xml_document response = pugi::xml_document();
		return sendRequest(ClanRequestAction::DeleteAnnouncement, ClanManagerOperationType::UPDATE, &doc, &response);
	}
}
#pragma endregion