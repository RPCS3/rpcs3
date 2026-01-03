#include "stdafx.h"

#include <string_view>
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

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNpClans.h"
#include "Emu/NP/clans_client.h"
#include "Emu/NP/clans_config.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/system_config.h"

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
			case clan::ClanRequestAction::CreateClan:               return "create_clan";
			case clan::ClanRequestAction::DisbandClan:              return "disband_clan";
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

	size_t clans_client::curl_write_callback(void* data, size_t size, size_t nmemb, void* clientp)
	{
		const size_t realsize = size * nmemb;
		std::vector<char>* mem = static_cast<std::vector<char>*>(clientp);

		const size_t old_size = mem->size();
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
		static constexpr u32 SCE_NP_CLANS_MAX_CTX_NUM = 16;

		static constexpr u32 id_base = 0xA001;
		static constexpr u32 id_step = 1;
		static constexpr u32 id_count = SCE_NP_CLANS_MAX_CTX_NUM;
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

	SceNpClansError clans_client::create_request(s32& req_id)
	{
		if (!g_cfg.net.clans_enabled)
		{
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;
		}

		const s32 id = idm::make<clan_request_ctx>();

		if (id == id_manager::id_traits<clan_request_ctx>::invalid)
		{
			return SceNpClansError::SCE_NP_CLANS_ERROR_EXCEEDS_MAX;
		}

		const auto ctx = idm::get_unlocked<clan_request_ctx>(id);
		if (!ctx || !ctx->curl)
		{
			idm::remove<clan_request_ctx>(id);
			return SceNpClansError::SCE_NP_CLANS_ERROR_NOT_INITIALIZED;
		}

		req_id = id;

		return SceNpClansError::SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::destroy_request(u32 req_id)
	{
		if (idm::remove<clan_request_ctx>(req_id))
			return SceNpClansError::SCE_NP_CLANS_SUCCESS;

		return SceNpClansError::SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;
	}

	SceNpClansError clans_client::send_request(u32 req_id, ClanRequestAction action, ClanManagerOperationType op_type, const pugi::xml_document& xml_body, pugi::xml_document& out_response)
	{
		auto ctx = idm::get_unlocked<clan_request_ctx>(req_id);

		if (!ctx || !ctx->curl)
			return SCE_NP_CLANS_ERROR_NOT_INITIALIZED;

		CURL* curl = ctx->curl;

		ClanRequestType req_type = ClanRequestType::FUNC;
		const pugi::xml_node clan = xml_body.child("clan");
		if (clan && clan.child("ticket"))
		{
			req_type = ClanRequestType::SEC;
		}

		const std::string host = g_cfg_clans.get_host();
		const std::string protocol = g_cfg_clans.get_use_https() ? "https" : "http";
		const std::string url = fmt::format("%s://%s/clan_manager_%s/%s/%s", protocol, host, op_type, req_type, action);

		std::ostringstream oss;
		xml_body.save(oss, "\t", 8U);

		const std::string xml = oss.str();

		char err_buf[CURL_ERROR_SIZE];
		err_buf[0] = '\0';

		std::vector<char> response_buffer;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buf);

		// WARN: This disables certificate verification!
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, xml.size());

		const CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			clan_log.error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			clan_log.error("Error buffer: %s", err_buf);
			return SCE_NP_CLANS_ERROR_BAD_REQUEST;
		}

		response_buffer.push_back('\0');

		const pugi::xml_parse_result xml_res = out_response.load_string(response_buffer.data());
		if (!xml_res)
		{
			clan_log.error("XML parsing failed: %s", xml_res.description());
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;
		}

		const pugi::xml_node clan_result = out_response.child("clan");
		if (!clan_result)
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;

		const pugi::xml_attribute result = clan_result.attribute("result");
		if (!result)
			return SCE_NP_CLANS_ERROR_BAD_RESPONSE;

		const std::string result_str = result.as_string();
		if (result_str != "00")
			return static_cast<SceNpClansError>(0x80022800 | std::stoul(result_str, nullptr, 16));

		return SCE_NP_CLANS_SUCCESS;
	}

	std::string clans_client::get_clan_ticket(np::np_handler& nph)
	{
		// Pretend we failed to get a ticket if the emulator isn't
		// connected to RPCN.
		if (nph.get_psn_status() != SCE_NP_MANAGER_STATUS_ONLINE)
			return "";

		const auto& npid = nph.get_npid();

		const char* service_id = CLANS_SERVICE_ID;
		const unsigned char* cookie = nullptr;
		const u32 cookie_size = 0;
		const char* entitlement_id = CLANS_ENTITLEMENT_ID;
		const u32 consumed_count = 0;

		// Use the cached ticket if available
		np::ticket clan_ticket;
		if (!nph.get_clan_ticket_ready())
		{
			// If not cached, request a new ticket
			nph.req_ticket(0x00020001, &npid, service_id, cookie, cookie_size, entitlement_id, consumed_count);
		}

		clan_ticket = nph.get_clan_ticket();
		if (clan_ticket.empty())
		{
			clan_log.error("Failed to get clan ticket");
			return "";
		}

		std::vector<byte> ticket_bytes(1024);
		uint32_t ticket_size = UINT32_MAX;

		Base64_Encode_NoNl(clan_ticket.data(), static_cast<u32>(clan_ticket.size()), ticket_bytes.data(), &ticket_size);
		const std::string ticket_str = std::string(reinterpret_cast<char*>(ticket_bytes.data()), ticket_size);

		return ticket_str;
	}

#pragma region Outgoing API Requests
	SceNpClansError clans_client::get_clan_list(np::np_handler& nph, u32 req_id, const SceNpClansPagingRequest& paging, std::vector<SceNpClansEntry>& clan_list, SceNpClansPagingResult& page_result)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");

		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("start").text().set(paging.startPos);
		clan.append_child("max").text().set(paging.max);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::GetClanList, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node list = clan_result.child("list");

		const pugi::xml_attribute results = list.attribute("results");
		const uint32_t results_count = results.as_uint();

		const pugi::xml_attribute total = list.attribute("total");
		const uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node info = list.child("info"); info; info = info.next_sibling("info"), i++)
		{
			const pugi::xml_attribute id = info.attribute("id");
			const uint32_t clan_id = id.as_uint();

			const pugi::xml_node name = info.child("name");
			const std::string name_str = name.text().as_string();

			const pugi::xml_node tag = info.child("tag");
			const std::string tag_str = tag.text().as_string();

			const pugi::xml_node role = info.child("role");
			const int32_t role_int = role.text().as_uint();

			const pugi::xml_node status = info.child("status");
			const uint32_t status_int = status.text().as_uint();

			const pugi::xml_node onlinename = info.child("onlinename");
			const std::string onlinename_str = onlinename.text().as_string();

			const pugi::xml_node members = info.child("members");
			const uint32_t members_int = members.text().as_uint();

			SceNpClansEntry entry = SceNpClansEntry{
				.info = SceNpClansClanBasicInfo{
					.clanId = clan_id,
					.numMembers = members_int,
					.name = "",
					.tag = "",
					.reserved = {0, 0},
				},
				.role = static_cast<SceNpClansMemberRole>(role_int),
				.status = static_cast<SceNpClansMemberStatus>(status_int)};

			strcpy_trunc(entry.info.name, name_str);
			strcpy_trunc(entry.info.tag, tag_str);

			::at32(clan_list, i) = std::move(entry);
			i++;
		}

		page_result = SceNpClansPagingResult{
			.count = results_count,
			.total = total_count};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::get_clan_info(u32 req_id, SceNpClanId clan_id, SceNpClansClanInfo& clan_info)
	{
		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::GetClanInfo, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node info = clan_result.child("info");

		const std::string name_str = info.child("name").text().as_string();
		const std::string tag_str = info.child("tag").text().as_string();
		const uint32_t members_int = info.child("members").text().as_uint();
		const std::string date_created_str = info.child("date-created").text().as_string();
		const std::string description_str = info.child("description").text().as_string();

		clan_info = SceNpClansClanInfo{
			.info = SceNpClansClanBasicInfo{
				.clanId = clan_id,
				.numMembers = members_int,
				.name = "",
				.tag = "",
			},
			.updatable = SceNpClansUpdatableClanInfo{
				.description = "",
			}};

		strcpy_trunc(clan_info.info.name, name_str);
		strcpy_trunc(clan_info.info.tag, tag_str);
		strcpy_trunc(clan_info.updatable.description, description_str);

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::get_member_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMemberEntry& mem_info)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::GetMemberInfo, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node member_info = clan_result.child("info");

		const pugi::xml_attribute member_jid = member_info.attribute("jid");
		const std::string member_jid_str = member_jid.as_string();
		const std::string member_username = fmt::split(member_jid_str, {"@"})[0];

		SceNpId member_npid;
		if (strncmp(member_username.c_str(), nph.get_npid().handle.data, 16) == 0)
		{
			member_npid = nph.get_npid();
		}
		else
		{
			np::string_to_npid(member_username, member_npid);
		}

		const pugi::xml_node role = member_info.child("role");
		const uint32_t role_int = role.text().as_uint();

		const pugi::xml_node status = member_info.child("status");
		const uint32_t status_int = status.text().as_uint();

		const pugi::xml_node description = member_info.child("description");
		const std::string description_str = description.text().as_string();

		mem_info = SceNpClansMemberEntry
		{
			.npid = std::move(member_npid),
			.role = static_cast<SceNpClansMemberRole>(role_int),
			.status = static_cast<SceNpClansMemberStatus>(status_int),
			.updatable = SceNpClansUpdatableMemberInfo{
				.description = "",
			}
		};

		strcpy_trunc(mem_info.updatable.description, description_str);

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::get_member_list(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, SceNpClansMemberStatus /*status*/, std::vector<SceNpClansMemberEntry>& mem_list, SceNpClansPagingResult& page_result)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);
		clan.append_child("start").text().set(paging.startPos);
		clan.append_child("max").text().set(paging.max);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::GetMemberList, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node list = clan_result.child("list");

		const pugi::xml_attribute results = list.attribute("results");
		const uint32_t results_count = results.as_uint();

		const pugi::xml_attribute total = list.attribute("total");
		const uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node member_info = list.child("info"); member_info; member_info = member_info.next_sibling("info"))
		{
			const std::string member_jid = member_info.attribute("jid").as_string();
			const std::string member_username = fmt::split(member_jid, {"@"})[0];

			SceNpId member_npid;
			if (strncmp(member_username.c_str(), nph.get_npid().handle.data, SCE_NET_NP_ONLINEID_MAX_LENGTH) == 0)
			{
				member_npid = nph.get_npid();
			}
			else
			{
				np::string_to_npid(member_username, member_npid);
			}

			const uint32_t role_int = member_info.child("role").text().as_uint();
			const uint32_t status_int = member_info.child("status").text().as_uint();
			const std::string description_str = member_info.child("description").text().as_string();

			SceNpClansMemberEntry entry = SceNpClansMemberEntry
			{
				.npid = std::move(member_npid),
				.role = static_cast<SceNpClansMemberRole>(role_int),
				.status = static_cast<SceNpClansMemberStatus>(status_int),
			};

			strcpy_trunc(entry.updatable.description, description_str);

			::at32(mem_list, i) = std::move(entry);
			i++;
		}

		page_result = SceNpClansPagingResult
		{
			.count = results_count,
			.total = total_count
		};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::get_blacklist(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, std::vector<SceNpClansBlacklistEntry>& bl, SceNpClansPagingResult& page_result)
	{
		const std::string ticket = get_clan_ticket(nph);

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);
		clan.append_child("start").text().set(paging.startPos);
		clan.append_child("max").text().set(paging.max);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::GetBlacklist, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node list = clan_result.child("list");

		const pugi::xml_attribute results = list.attribute("results");
		const uint32_t results_count = results.as_uint();

		const pugi::xml_attribute total = list.attribute("total");
		const uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node member = list.child("entry"); member; member = member.next_sibling("entry"))
		{
			const pugi::xml_node member_jid = member.child("jid");
			const std::string member_jid_str = member_jid.text().as_string();
			const std::string member_username = fmt::split(member_jid_str, {"@"})[0];

			SceNpId member_npid = {};
			if (strncmp(member_username.c_str(), nph.get_npid().handle.data, SCE_NET_NP_ONLINEID_MAX_LENGTH) == 0)
			{
				member_npid = nph.get_npid();
			}
			else
			{
				np::string_to_npid(member_username.c_str(), member_npid);
			}

			SceNpClansBlacklistEntry entry = SceNpClansBlacklistEntry
			{
				.entry = std::move(member_npid),
			};

			::at32(bl, i) = std::move(entry);
			i++;
		}

		page_result = SceNpClansPagingResult
		{
			.count = results_count,
			.total = total_count
		};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::add_blacklist_entry(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::RecordBlacklistEntry, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::remove_blacklist_entry(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id)
	{
		std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::DeleteBlacklistEntry, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::clan_search(u32 req_id, const SceNpClansPagingRequest& paging, const SceNpClansSearchableName& search, std::vector<SceNpClansClanBasicInfo>& clan_list, SceNpClansPagingResult& page_result)
	{
		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("start").text().set(paging.startPos);
		clan.append_child("max").text().set(paging.max);

		pugi::xml_node filter = clan.append_child("filter");
		pugi::xml_node name = filter.append_child("name");

		const std::string op_name = fmt::format("%s", static_cast<ClanSearchFilterOperator>(static_cast<s32>(search.nameSearchOp)));
		name.append_attribute("op").set_value(op_name.c_str());
		name.append_attribute("value").set_value(search.name);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::ClanSearch, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node list = clan_result.child("list");

		const pugi::xml_attribute results = list.attribute("results");
		const uint32_t results_count = results.as_uint();

		const pugi::xml_attribute total = list.attribute("total");
		const uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node node = list.child("info"); node; node = node.next_sibling("info"))
		{
			const uint32_t clan_id = node.attribute("id").as_uint();
			const std::string name_str = node.child("name").text().as_string();
			const std::string tag_str = node.child("tag").text().as_string();
			const uint32_t members_int = node.child("members").text().as_uint();

			SceNpClansClanBasicInfo entry = SceNpClansClanBasicInfo
			{
				.clanId = clan_id,
				.numMembers = members_int,
				.name = "",
				.tag = "",
				.reserved = {0, 0},
			};

			strcpy_trunc(entry.name, name_str);
			strcpy_trunc(entry.tag, tag_str);

			::at32(clan_list, i) = std::move(entry);
			i++;
		}

		page_result = SceNpClansPagingResult
		{
			.count = results_count,
			.total = total_count
		};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::create_clan(np::np_handler& nph, u32 req_id, std::string_view name, std::string_view tag, vm::ptr<SceNpClanId> clan_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());

		clan.append_child("name").text().set(name.data());
		clan.append_child("tag").text().set(tag.data());

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::CreateClan, ClanManagerOperationType::UPDATE, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_attribute id = clan_result.attribute("id");
		const uint32_t clan_id_int = id.as_uint();

		*clan_id = clan_id_int;

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::disband_dlan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::DisbandClan, ClanManagerOperationType::UPDATE, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::request_membership(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessage& /*message*/)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::RequestMembership, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::cancel_request_membership(np::np_handler& nph, u32 req_id, SceNpClanId clan_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::CancelRequestMembership, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::send_membership_response(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& /*message*/, b8 allow)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, allow ? ClanRequestAction::AcceptMembershipRequest : ClanRequestAction::DeclineMembershipRequest, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::send_invitation(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& /*message*/)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::SendInvitation, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::cancel_invitation(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::CancelInvitation, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::send_invitation_response(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessage& /*message*/, b8 accept)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, accept ? ClanRequestAction::AcceptInvitation : ClanRequestAction::DeclineInvitation, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::update_member_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansUpdatableMemberInfo& info)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_node role = clan.append_child("onlinename");
		role.text().set(nph.get_npid().handle.data);

		pugi::xml_node description = clan.append_child("description");
		description.text().set(info.description);

		pugi::xml_node status = clan.append_child("bin-attr1");

		byte bin_attr_1[SCE_NP_CLANS_MEMBER_BINARY_ATTRIBUTE1_MAX_SIZE * 2 + 1] = {0};
		uint32_t bin_attr_1_size = UINT32_MAX;
		Base64_Encode_NoNl(info.binAttr1, info.binData1Size, bin_attr_1, &bin_attr_1_size);

		if (bin_attr_1_size == UINT32_MAX)
			return SCE_NP_CLANS_ERROR_INVALID_ARGUMENT;

		// `reinterpret_cast` used to let the compiler select the correct overload of `set`
		status.text().set(reinterpret_cast<const char*>(bin_attr_1));

		pugi::xml_node allow_msg = clan.append_child("allow-msg");
		allow_msg.text().set(static_cast<uint32_t>(info.allowMsg));

		pugi::xml_node size = clan.append_child("size");
		size.text().set(info.binData1Size);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::UpdateMemberInfo, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::update_clan_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansUpdatableClanInfo& info)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		// TODO: implement binary and integer attributes (not implemented in server yet)

		pugi::xml_node description = clan.append_child("description");
		description.text().set(info.description);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::UpdateClanInfo, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::join_clan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::JoinClan, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::leave_clan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::LeaveClan, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::kick_member(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& /*message*/)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::KickMember, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::change_member_role(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMemberRole role)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		const std::string jid_str = fmt::format(JID_FORMAT, np_id.handle.data);
		clan.append_child("jid").text().set(jid_str.c_str());

		pugi::xml_node role_node = clan.append_child("role");
		role_node.text().set(static_cast<uint32_t>(role));

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::ChangeMemberRole, ClanManagerOperationType::UPDATE, doc, response);
	}

	SceNpClansError clans_client::retrieve_announcements(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, std::vector<SceNpClansMessageEntry>& announcements, SceNpClansPagingResult& page_result)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);
		clan.append_child("start").text().set(paging.startPos);
		clan.append_child("max").text().set(paging.max);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::RetrieveAnnouncements, ClanManagerOperationType::VIEW, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node list = clan_result.child("list");

		const pugi::xml_attribute results = list.attribute("results");
		const uint32_t results_count = results.as_uint();

		const pugi::xml_attribute total = list.attribute("total");
		const uint32_t total_count = total.as_uint();

		int i = 0;
		for (pugi::xml_node node = list.child("msg-info"); node; node = node.next_sibling("msg-info"))
		{
			const pugi::xml_attribute id = node.attribute("id");
			const uint32_t msg_id = id.as_uint();

			const std::string subject_str = node.child("subject").text().as_string();
			const std::string msg_str = node.child("msg").text().as_string();
			const std::string author_jid = node.child("jid").text().as_string();
			const std::string msg_date = node.child("msg-date").text().as_string();

			SceNpId author_npid;
			const std::string author_username = fmt::split(author_jid, {"@"})[0];

			if (strncmp(author_username.c_str(), nph.get_npid().handle.data, SCE_NET_NP_ONLINEID_MAX_LENGTH) == 0)
			{
				author_npid = nph.get_npid();
			}
			else
			{
				np::string_to_npid(author_username, author_npid);
			}

			// TODO: implement `binData` and `fromId`

			SceNpClansMessageEntry entry = SceNpClansMessageEntry
			{
				.mId = msg_id,
				.message = SceNpClansMessage {
					.subject = "",
					.body = "",
				},
				.npid = std::move(author_npid),
				.postedBy = clan_id,
			};

			strcpy_trunc(entry.message.subject, subject_str);
			strcpy_trunc(entry.message.body, msg_str);

			::at32(announcements, i) = std::move(entry);
			i++;
		}

		page_result = SceNpClansPagingResult
		{
			.count = results_count,
			.total = total_count
		};

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::post_announcement(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansMessage& announcement, const SceNpClansMessageData& /*data*/, u32 duration, SceNpClansMessageId& msg_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);

		pugi::xml_node subject = clan.append_child("subject");
		subject.text().set(announcement.subject);

		pugi::xml_node msg = clan.append_child("msg");
		msg.text().set(announcement.body);

		pugi::xml_node expire_date = clan.append_child("expire-date");
		expire_date.text().set(duration);

		pugi::xml_document response = pugi::xml_document();
		const SceNpClansError clan_res = send_request(req_id, ClanRequestAction::PostAnnouncement, ClanManagerOperationType::UPDATE, doc, response);

		if (clan_res != SCE_NP_CLANS_SUCCESS)
			return clan_res;

		const pugi::xml_node clan_result = response.child("clan");
		const pugi::xml_node msg_id_node = clan_result.child("id");
		msg_id = msg_id_node.text().as_uint();

		return SCE_NP_CLANS_SUCCESS;
	}

	SceNpClansError clans_client::delete_announcement(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessageId announcement_id)
	{
		const std::string ticket = get_clan_ticket(nph);
		if (ticket.empty())
			return SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE;

		pugi::xml_document doc = pugi::xml_document();
		pugi::xml_node clan = doc.append_child("clan");
		clan.append_child("ticket").text().set(ticket.c_str());
		clan.append_child("id").text().set(clan_id);
		clan.append_child("msg-id").text().set(announcement_id);

		pugi::xml_document response = pugi::xml_document();
		return send_request(req_id, ClanRequestAction::DeleteAnnouncement, ClanManagerOperationType::UPDATE, doc, response);
	}
}
#pragma endregion
