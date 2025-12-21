#pragma once

#include <curl/curl.h>
#include <pugixml.hpp>

#include <Emu/Cell/Modules/sceNpClans.h>
#include <Emu/NP/np_handler.h>
#include <Emu/IdManager.h>

inline const char* CLANS_ENTITLEMENT_ID = "NPWR00432_00";
inline const char* CLANS_SERVICE_ID = "IV0001-NPXS01001_00";

namespace clan
{
	enum class ClanManagerOperationType
	{
		VIEW,
		UPDATE
	};

	enum class ClanRequestType
	{
		FUNC,
		SEC
	};

	enum class ClanSearchFilterOperator : u8
	{
		Equal,
		NotEqual,
		GreaterThan,
		GreaterThanOrEqual,
		LessThan,
		LessThanOrEqual,
		Like,
	};

	enum class ClanRequestAction
	{
		GetClanList,
		GetClanInfo,
		GetMemberInfo,
		GetMemberList,
		GetBlacklist,
		RecordBlacklistEntry,
		DeleteBlacklistEntry,
		ClanSearch,
		CreateClan,
		DisbandClan,
		RequestMembership,
		CancelRequestMembership,
		AcceptMembershipRequest,
		DeclineMembershipRequest,
		SendInvitation,
		CancelInvitation,
		AcceptInvitation,
		DeclineInvitation,
		UpdateMemberInfo,
		UpdateClanInfo,
		JoinClan,
		LeaveClan,
		KickMember,
		ChangeMemberRole,
		RetrieveAnnouncements,
		PostAnnouncement,
		DeleteAnnouncement
	};

	class clans_client
	{
	private:

		static size_t curl_write_callback(void* data, size_t size, size_t nmemb, void* clientp);
		SceNpClansError send_request(u32 reqId, ClanRequestAction action, ClanManagerOperationType type, const pugi::xml_document& xml_body, pugi::xml_document& out_response);

		/// @brief Forge and get a V2.1 Ticket for clan operations
		std::string get_clan_ticket(np::np_handler& nph);

	public:
		clans_client();
		~clans_client();

		SceNpClansError create_request(s32& req_id);
		SceNpClansError destroy_request(u32 req_id);

		SceNpClansError clan_search(u32 req_id, const SceNpClansPagingRequest& paging, const SceNpClansSearchableName& search, std::vector<SceNpClansClanBasicInfo>& clan_list, SceNpClansPagingResult& page_result);

		SceNpClansError create_clan(np::np_handler& nph, u32 req_id, std::string_view name, std::string_view tag, vm::ptr<SceNpClanId> clan_id);
		SceNpClansError disband_dlan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id);

		SceNpClansError get_clan_list(np::np_handler& nph, u32 req_id, const SceNpClansPagingRequest&, std::vector<SceNpClansEntry>& clan_list, SceNpClansPagingResult& page_result);
		SceNpClansError get_clan_info(u32 req_id, SceNpClanId clan_id, SceNpClansClanInfo& clan_info);

		SceNpClansError get_member_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMemberEntry& mem_info);
		SceNpClansError get_member_list(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, SceNpClansMemberStatus status, std::vector<SceNpClansMemberEntry>& mem_list, SceNpClansPagingResult& page_result);

		SceNpClansError get_blacklist(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, std::vector<SceNpClansBlacklistEntry>& bl, SceNpClansPagingResult& page_result);
		SceNpClansError add_blacklist_entry(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id);
		SceNpClansError remove_blacklist_entry(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id);

		SceNpClansError request_membership(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessage& message);
		SceNpClansError cancel_request_membership(np::np_handler& nph, u32 req_id, SceNpClanId clan_id);
		SceNpClansError send_membership_response(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& message, b8 allow);

		SceNpClansError send_invitation(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& message);
		SceNpClansError cancel_invitation(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id);
		SceNpClansError send_invitation_response(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessage& message, b8 accept);

		SceNpClansError join_clan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id);
		SceNpClansError leave_clan(np::np_handler& nph, u32 req_id, SceNpClanId clan_id);

		SceNpClansError update_member_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansUpdatableMemberInfo& info);
		SceNpClansError update_clan_info(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansUpdatableClanInfo& info);

		SceNpClansError kick_member(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMessage& message);
		SceNpClansError change_member_role(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpId& np_id, SceNpClansMemberRole role);

		SceNpClansError retrieve_announcements(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansPagingRequest& paging, std::vector<SceNpClansMessageEntry>& announcements, SceNpClansPagingResult& page_result);
		SceNpClansError post_announcement(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, const SceNpClansMessage& announcement, const SceNpClansMessageData& data, u32 duration, SceNpClansMessageId& announcement_id);
		SceNpClansError delete_announcement(np::np_handler& nph, u32 req_id, SceNpClanId clan_id, SceNpClansMessageId announcement_id);
	};
} // namespace clan

struct sce_np_clans_manager
{
	atomic_t<bool> is_initialized = false;
	std::shared_ptr<clan::clans_client> client;
};
