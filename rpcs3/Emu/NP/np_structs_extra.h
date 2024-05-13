#pragma once

#include "Emu/Cell/Modules/sceNp2.h"

namespace extra_nps
{
	void print_userinfo2(const SceNpUserInfo2* user);
	void print_sigoptparam(const SceNpMatching2SignalingOptParam* opt);
	void print_bin_attr(const SceNpMatching2BinAttr* bin);
	void print_presence_data(const SceNpMatching2PresenceOptionData* opt);
	void print_range_filter(const SceNpMatching2RangeFilter* filt);
	void print_room_data_internal(const SceNpMatching2RoomDataInternal* room);
	void print_room_data_external(const SceNpMatching2RoomDataExternal* room);
	void print_room_member_data_internal(const SceNpMatching2RoomMemberDataInternal* member);

	void print_createjoinroom(const SceNpMatching2CreateJoinRoomRequest* req);
	void print_create_room_resp(const SceNpMatching2CreateJoinRoomResponse* resp);
	void print_joinroom(const SceNpMatching2JoinRoomRequest* req);
	void print_search_room(const SceNpMatching2SearchRoomRequest* req);
	void print_search_room_resp(const SceNpMatching2SearchRoomResponse* resp);
	void print_set_roomdata_ext_req(const SceNpMatching2SetRoomDataExternalRequest* req);
	void print_set_roomdata_int_req(const SceNpMatching2SetRoomDataInternalRequest* req);
	void print_get_roommemberdata_int_req(const SceNpMatching2GetRoomMemberDataInternalRequest* req);
	void print_set_roommemberdata_int_req(const SceNpMatching2SetRoomMemberDataInternalRequest* req);
	void print_get_roomdata_external_list_req(const SceNpMatching2GetRoomDataExternalListRequest* req);
	void print_get_roomdata_external_list_resp(const SceNpMatching2GetRoomDataExternalListResponse* resp);

	void print_SceNpBasicAttachmentData(const SceNpBasicAttachmentData* data);
	void print_SceNpBasicExtendedAttachmentData(const SceNpBasicExtendedAttachmentData* data);

	void print_SceNpScoreRankData(const SceNpScoreRankData* data);
	void print_SceNpScoreRankData_deprecated(const SceNpScoreRankData_deprecated* data);

	void print_SceNpRoomId(const SceNpRoomId& room_id);
	void print_SceNpMatchingAttr(const SceNpMatchingAttr* data);
	void print_SceNpMatchingSearchCondition(const SceNpMatchingSearchCondition* data);
	void print_SceNpMatchingRoom(const SceNpMatchingRoom* data);
	void print_SceNpMatchingRoomList(const SceNpMatchingRoomList* data);
	void print_SceNpMatchingRoomStatus(const SceNpMatchingRoomStatus* data);
	void print_SceNpMatchingJoinedRoomInfo(const SceNpMatchingJoinedRoomInfo* data);
	void print_SceNpMatchingSearchJoinRoomInfo(const SceNpMatchingSearchJoinRoomInfo* data);
} // namespace extra_nps
