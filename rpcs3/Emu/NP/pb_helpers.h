#pragma once

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "np_event_data.h"
#include "generated/np2_structs.pb.h"

namespace np
{
	void BinAttr_to_SceNpMatching2BinAttr(event_data& edata, const np2_structs::BinAttr& bin_attr, SceNpMatching2BinAttr* binattr_info);
	void BinAttrs_to_SceNpMatching2BinAttrs(event_data& edata, const google::protobuf::RepeatedPtrField<np2_structs::BinAttr>& pb_attrs, SceNpMatching2BinAttr* binattr_info);
	void RoomMemberBinAttrInternal_to_SceNpMatching2RoomMemberBinAttrInternal(event_data& edata, const np2_structs::RoomMemberBinAttrInternal& pb_attr, SceNpMatching2RoomMemberBinAttrInternal* binattr_info);
	void RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(event_data& edata, const np2_structs::BinAttrInternal& pb_attr, SceNpMatching2RoomBinAttrInternal* binattr_info);
	void RoomGroup_to_SceNpMatching2RoomGroup(const np2_structs::RoomGroup& pb_group, SceNpMatching2RoomGroup* sce_group);
	void RoomGroups_to_SceNpMatching2RoomGroups(const google::protobuf::RepeatedPtrField<np2_structs::RoomGroup>& pb_groups, SceNpMatching2RoomGroup* sce_groups);
	void UserInfo_to_SceNpUserInfo(const np2_structs::UserInfo& user, SceNpUserInfo* user_info);
	void UserInfo_to_SceNpUserInfo2(event_data& edata, const np2_structs::UserInfo& user, SceNpUserInfo2* user_info, bool include_onlinename, bool include_avatarurl);
	void RoomDataExternal_to_SceNpMatching2RoomDataExternal(event_data& edata, const np2_structs::RoomDataExternal& room, SceNpMatching2RoomDataExternal* room_info, bool include_onlinename, bool include_avatarurl);
	void RoomMemberDataExternal_to_SceNpMatching2RoomMemberDataExternal(event_data& edata, const np2_structs::RoomMemberDataExternal& member, SceNpMatching2RoomMemberDataExternal* member_info, bool include_onlinename, bool include_avatarurl);
	void SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(event_data& edata, const np2_structs::SearchRoomResponse& resp, SceNpMatching2SearchRoomResponse* search_resp);
	void GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(event_data& edata, const np2_structs::GetRoomDataExternalListResponse& resp, SceNpMatching2GetRoomDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl);
	void GetRoomMemberDataExternalListResponse_to_SceNpMatching2GetRoomMemberDataExternalListResponse(event_data& edata, const np2_structs::GetRoomMemberDataExternalListResponse& resp, SceNpMatching2GetRoomMemberDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl);
	u16 RoomDataInternal_to_SceNpMatching2RoomDataInternal(event_data& edata, const np2_structs::RoomDataInternal& resp, SceNpMatching2RoomDataInternal* room_resp, const SceNpId& npid, bool include_onlinename, bool include_avatarurl);
	void RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(event_data& edata, const np2_structs::RoomMemberDataInternal& member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data, bool include_onlinename, bool include_avatarurl);
	void RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(event_data& edata, const np2_structs::RoomMemberUpdateInfo& resp, SceNpMatching2RoomMemberUpdateInfo* room_info, bool include_onlinename, bool include_avatarurl);
	void RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const np2_structs::RoomUpdateInfo& update_info, SceNpMatching2RoomUpdateInfo* sce_update_info);
	void GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const np2_structs::GetPingInfoResponse& resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp);
	void RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(event_data& edata, const np2_structs::RoomMessageInfo& mi, SceNpMatching2RoomMessageInfo* sce_mi, bool include_onlinename, bool include_avatarurl);
	void RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(event_data& edata, const np2_structs::RoomDataInternalUpdateInfo& update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl);
	void RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(event_data& edata, const np2_structs::RoomMemberDataInternalUpdateInfo& update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl);
	void MatchingRoomStatus_to_SceNpMatchingRoomStatus(event_data& edata, const np2_structs::MatchingRoomStatus& resp, SceNpMatchingRoomStatus* room_status);
	void MatchingRoomStatus_to_SceNpMatchingJoinedRoomInfo(event_data& edata, const np2_structs::MatchingRoomStatus& resp, SceNpMatchingJoinedRoomInfo* room_info);
	void MatchingRoom_to_SceNpMatchingRoom(event_data& edata, const np2_structs::MatchingRoom& resp, SceNpMatchingRoom* room);
	void MatchingRoomList_to_SceNpMatchingRoomList(event_data& edata, const np2_structs::MatchingRoomList& resp, SceNpMatchingRoomList* room_list);
	void MatchingSearchJoinRoomInfo_to_SceNpMatchingSearchJoinRoomInfo(event_data& edata, const np2_structs::MatchingSearchJoinRoomInfo& resp, SceNpMatchingSearchJoinRoomInfo* room_info);
	void MatchingAttr_to_SceNpMatchingAttr(event_data& edata, const google::protobuf::RepeatedPtrField<np2_structs::MatchingAttr>& attr_list, vm::bptr<SceNpMatchingAttr>& first_attr);
	void OptParam_to_SceNpMatching2SignalingOptParam(const np2_structs::OptParam& resp, SceNpMatching2SignalingOptParam* opt_param);
} // namespace np
