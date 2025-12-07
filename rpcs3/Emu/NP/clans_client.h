#pragma once

#include <3rdparty/curl/curl/include/curl/curl.h>
#include <Emu/Cell/Modules/sceNpClans.h>
#include <Emu/NP/np_handler.h>
#include <pugixml.hpp>

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
		CURL* curl = nullptr;
		CURLcode res = CURLE_OK;

		static size_t curlWriteCallback(void* data, size_t size, size_t nmemb, void* clientp);
		SceNpClansError sendRequest(ClanRequestAction action, ClanManagerOperationType type, pugi::xml_document* xmlBody, pugi::xml_document* outResponse);

		/// @brief Forge and get a V2.1 Ticket for clan operations
		std::string getClanTicket(np::np_handler& nph);

	public:
		clans_client();
		~clans_client();

		SceNpClansError createRequest();
		SceNpClansError destroyRequest();

		SceNpClansError clanSearch(SceNpClansPagingRequest* paging, SceNpClansSearchableName* search, SceNpClansClanBasicInfo* clanList, SceNpClansPagingResult* pageResult);

		SceNpClansError getClanList(np::np_handler& nph, SceNpClansPagingRequest* paging, SceNpClansEntry* clanList, SceNpClansPagingResult* pageResult);
		SceNpClansError getClanInfo(SceNpClanId clanId, SceNpClansClanInfo* clanInfo);

		SceNpClansError getMemberInfo(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMemberEntry* memInfo);
		SceNpClansError getMemberList(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMemberStatus status, SceNpClansMemberEntry* memList, SceNpClansPagingResult* pageResult);

		SceNpClansError getBlacklist(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansBlacklistEntry* bl, SceNpClansPagingResult* pageResult);
		SceNpClansError addBlacklistEntry(np::np_handler& nph, SceNpClanId clanId, SceNpId npId);
		SceNpClansError removeBlacklistEntry(np::np_handler& nph, SceNpClanId clanId, SceNpId npId);

		SceNpClansError requestMembership(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* message);
		SceNpClansError cancelRequestMembership(np::np_handler& nph, SceNpClanId clanId);
		SceNpClansError sendMembershipResponse(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message, b8 allow);

		SceNpClansError sendInvitation(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message);
		SceNpClansError cancelInvitation(np::np_handler& nph, SceNpClanId clanId, SceNpId npId);
		SceNpClansError sendInvitationResponse(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* message, b8 accept);

		SceNpClansError joinClan(np::np_handler& nph, SceNpClanId clanId);
		SceNpClansError leaveClan(np::np_handler& nph, SceNpClanId clanId);

		SceNpClansError updateMemberInfo(np::np_handler& nph, SceNpClanId clanId, SceNpClansUpdatableMemberInfo* info);
		SceNpClansError updateClanInfo(np::np_handler& nph, SceNpClanId clanId, SceNpClansUpdatableClanInfo* info);

		SceNpClansError kickMember(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message);
		SceNpClansError changeMemberRole(np::np_handler& nph, SceNpClanId clanId, SceNpId npId, SceNpClansMemberRole role);

		SceNpClansError retrieveAnnouncements(np::np_handler& nph, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMessageEntry* announcements, SceNpClansPagingResult* pageResult);
		SceNpClansError postAnnouncement(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessage* announcement, SceNpClansMessageData* data, u32 duration, SceNpClansMessageId* announcementId);
		SceNpClansError deleteAnnouncement(np::np_handler& nph, SceNpClanId clanId, SceNpClansMessageId announcementId);
	};
} // namespace clan

struct sce_np_clans_manager
{
	atomic_t<bool> is_initialized = false;
	clan::clans_client* client;
};