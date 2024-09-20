#pragma once

#include "Emu/Cell/Modules/sceNp2.h"

namespace extra_nps
{
	void print_SceNpUserInfo2(const SceNpUserInfo2* user);
	void print_SceNpMatching2SignalingOptParam(const SceNpMatching2SignalingOptParam* opt);
	void print_SceNpMatching2BinAttr(const SceNpMatching2BinAttr* bin);
	void print_SceNpMatching2PresenceOptionData(const SceNpMatching2PresenceOptionData* opt);
	void print_SceNpMatching2RangeFilter(const SceNpMatching2RangeFilter* filt);
	void print_SceNpMatching2RoomDataInternal(const SceNpMatching2RoomDataInternal* room);
	void print_SceNpMatching2RoomDataExternal(const SceNpMatching2RoomDataExternal* room);
	void print_SceNpMatching2RoomMemberDataInternal(const SceNpMatching2RoomMemberDataInternal* member);

	void print_SceNpMatching2CreateJoinRoomRequest(const SceNpMatching2CreateJoinRoomRequest* req);
	void print_SceNpMatching2CreateJoinRoomResponse(const SceNpMatching2CreateJoinRoomResponse* resp);
	void print_SceNpMatching2JoinRoomRequest(const SceNpMatching2JoinRoomRequest* req);
	void print_SceNpMatching2SearchRoomRequest(const SceNpMatching2SearchRoomRequest* req);
	void print_SceNpMatching2SearchRoomResponse(const SceNpMatching2SearchRoomResponse* resp);
	void print_SceNpMatching2SetRoomDataExternalRequest(const SceNpMatching2SetRoomDataExternalRequest* req);
	void print_SceNpMatching2SetRoomDataInternalRequest(const SceNpMatching2SetRoomDataInternalRequest* req);
	void print_SceNpMatching2GetRoomMemberDataInternalRequest(const SceNpMatching2GetRoomMemberDataInternalRequest* req);
	void print_SceNpMatching2SetRoomMemberDataInternalRequest(const SceNpMatching2SetRoomMemberDataInternalRequest* req);
	void print_SceNpMatching2GetRoomDataExternalListRequest(const SceNpMatching2GetRoomDataExternalListRequest* req);
	void print_SceNpMatching2GetRoomDataExternalListResponse(const SceNpMatching2GetRoomDataExternalListResponse* resp);

	void print_SceNpMatching2GetLobbyInfoListRequest(const SceNpMatching2GetLobbyInfoListRequest* resp);

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
