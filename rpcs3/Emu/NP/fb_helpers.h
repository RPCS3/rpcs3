#pragma once

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "np_event_data.h"
#include "generated/np2_structs_generated.h"

namespace np
{
	void BinAttr_to_SceNpMatching2BinAttr(event_data& edata, const BinAttr* bin_attr, SceNpMatching2BinAttr* binattr_info);
	void BinAttrs_to_SceNpMatching2BinAttrs(event_data& edata, const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, SceNpMatching2BinAttr* binattr_info);
	void RoomMemberBinAttrInternal_to_SceNpMatching2RoomMemberBinAttrInternal(event_data& edata, const RoomMemberBinAttrInternal* fb_attr, SceNpMatching2RoomMemberBinAttrInternal* binattr_info);
	void RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(event_data& edata, const BinAttrInternal* fb_attr, SceNpMatching2RoomBinAttrInternal* binattr_info);
	void RoomGroup_to_SceNpMatching2RoomGroup(const RoomGroup* fb_group, SceNpMatching2RoomGroup* sce_group);
	void RoomGroups_to_SceNpMatching2RoomGroups(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_groups, SceNpMatching2RoomGroup* sce_groups);
	void UserInfo_to_SceNpUserInfo(const UserInfo* user, SceNpUserInfo* user_info);
	void UserInfo_to_SceNpUserInfo2(event_data& edata, const UserInfo* user, SceNpUserInfo2* user_info, bool include_onlinename, bool include_avatarurl);
	void RoomDataExternal_to_SceNpMatching2RoomDataExternal(event_data& edata, const RoomDataExternal* room, SceNpMatching2RoomDataExternal* room_info, bool include_onlinename, bool include_avatarurl);
	void SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(event_data& edata, const SearchRoomResponse* resp, SceNpMatching2SearchRoomResponse* search_resp);
	void GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(event_data& edata, const GetRoomDataExternalListResponse* resp, SceNpMatching2GetRoomDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl);
	u16 RoomDataInternal_to_SceNpMatching2RoomDataInternal(event_data& edata, const RoomDataInternal* resp, SceNpMatching2RoomDataInternal* room_resp, const SceNpId& npid, bool include_onlinename, bool include_avatarurl);
	void RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(event_data& edata, const RoomMemberDataInternal* member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data, bool include_onlinename, bool include_avatarurl);
	void RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(event_data& edata, const RoomMemberUpdateInfo* resp, SceNpMatching2RoomMemberUpdateInfo* room_info, bool include_onlinename, bool include_avatarurl);
	void RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const RoomUpdateInfo* update_info, SceNpMatching2RoomUpdateInfo* sce_update_info);
	void GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const GetPingInfoResponse* resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp);
	void RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(event_data& edata, const RoomMessageInfo* mi, SceNpMatching2RoomMessageInfo* sce_mi, bool include_onlinename, bool include_avatarurl);
	void RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(event_data& edata, const RoomDataInternalUpdateInfo* update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl);
	void RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(event_data& edata, const RoomMemberDataInternalUpdateInfo* update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl);
	void MatchingRoomStatus_to_SceNpMatchingRoomStatus(event_data& edata, const MatchingRoomStatus* resp, SceNpMatchingRoomStatus* room_status);
	void MatchingRoomStatus_to_SceNpMatchingJoinedRoomInfo(event_data& edata, const MatchingRoomStatus* resp, SceNpMatchingJoinedRoomInfo* room_info);
	void MatchingRoom_to_SceNpMatchingRoom(event_data& edata, const MatchingRoom* resp, SceNpMatchingRoom* room);
	void MatchingRoomList_to_SceNpMatchingRoomList(event_data& edata, const MatchingRoomList* resp, SceNpMatchingRoomList* room_list);
	void MatchingSearchJoinRoomInfo_to_SceNpMatchingSearchJoinRoomInfo(event_data& edata, const MatchingSearchJoinRoomInfo* resp, SceNpMatchingSearchJoinRoomInfo* room_info);
} // namespace np
