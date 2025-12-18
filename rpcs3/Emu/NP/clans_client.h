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


		static size_t curlWriteCallback(void* data, size_t size, size_t nmemb, void* clientp);
		SceNpClansError sendRequest(u32 reqId, ClanRequestAction action, ClanManagerOperationType type, pugi::xml_document* xmlBody, pugi::xml_document* outResponse);

		/// @brief Forge and get a V2.1 Ticket for clan operations
		std::string getClanTicket(np::np_handler& nph);

	public:
		clans_client();
		~clans_client();

		SceNpClansError createRequest(s32* reqId);
		SceNpClansError destroyRequest(u32 reqId);

		SceNpClansError clanSearch(u32 reqId, SceNpClansPagingRequest* paging, SceNpClansSearchableName* search, SceNpClansClanBasicInfo* clanList, SceNpClansPagingResult* pageResult);

		SceNpClansError createClan(np::np_handler& nph, u32 reqId, std::string_view name, std::string_view tag, vm::ptr<SceNpClanId> clanId);
		SceNpClansError disbandClan(np::np_handler& nph, u32 reqId, SceNpClanId clanId);

		SceNpClansError getClanList(np::np_handler& nph, u32 reqId, SceNpClansPagingRequest* paging, SceNpClansEntry* clanList, SceNpClansPagingResult* pageResult);
		SceNpClansError getClanInfo(u32 reqId, SceNpClanId clanId, SceNpClansClanInfo* clanInfo);

		SceNpClansError getMemberInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMemberEntry* memInfo);
		SceNpClansError getMemberList(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMemberStatus status, SceNpClansMemberEntry* memList, SceNpClansPagingResult* pageResult);

		SceNpClansError getBlacklist(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansBlacklistEntry* bl, SceNpClansPagingResult* pageResult);
		SceNpClansError addBlacklistEntry(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId);
		SceNpClansError removeBlacklistEntry(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId);

		SceNpClansError requestMembership(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* message);
		SceNpClansError cancelRequestMembership(np::np_handler& nph, u32 reqId, SceNpClanId clanId);
		SceNpClansError sendMembershipResponse(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message, b8 allow);

		SceNpClansError sendInvitation(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message);
		SceNpClansError cancelInvitation(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId);
		SceNpClansError sendInvitationResponse(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* message, b8 accept);

		SceNpClansError joinClan(np::np_handler& nph, u32 reqId, SceNpClanId clanId);
		SceNpClansError leaveClan(np::np_handler& nph, u32 reqId, SceNpClanId clanId);

		SceNpClansError updateMemberInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansUpdatableMemberInfo* info);
		SceNpClansError updateClanInfo(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansUpdatableClanInfo* info);

		SceNpClansError kickMember(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMessage* message);
		SceNpClansError changeMemberRole(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpId npId, SceNpClansMemberRole role);

		SceNpClansError retrieveAnnouncements(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansPagingRequest* paging, SceNpClansMessageEntry* announcements, SceNpClansPagingResult* pageResult);
		SceNpClansError postAnnouncement(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessage* announcement, SceNpClansMessageData* data, u32 duration, SceNpClansMessageId* announcementId);
		SceNpClansError deleteAnnouncement(np::np_handler& nph, u32 reqId, SceNpClanId clanId, SceNpClansMessageId announcementId);
	};
} // namespace clan

struct sce_np_clans_manager
{
	atomic_t<bool> is_initialized = false;
	std::shared_ptr<clan::clans_client> client;
};
