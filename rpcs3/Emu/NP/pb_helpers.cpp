#include "stdafx.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "pb_helpers.h"

LOG_CHANNEL(rpcn_log, "rpcn");

namespace np
{
	void BinAttr_to_SceNpMatching2BinAttr(event_data& edata, const np2_structs::BinAttr& bin_attr, SceNpMatching2BinAttr* binattr_info)
	{
		binattr_info->id   = bin_attr.id().value();
		binattr_info->size = ::narrow<u32>(bin_attr.data().size());
		auto* ptr          = edata.allocate<u8>(binattr_info->size, binattr_info->ptr);
		memcpy(ptr, bin_attr.data().data(), bin_attr.data().size());
	}

	void BinAttrs_to_SceNpMatching2BinAttrs(event_data& edata, const google::protobuf::RepeatedPtrField<np2_structs::BinAttr>& pb_attrs, SceNpMatching2BinAttr* binattr_info)
	{
		for (int i = 0; i < pb_attrs.size(); i++)
		{
			const auto& cur_pb_attr = pb_attrs.Get(i);
			auto* cur_binattr       = binattr_info + i;

			BinAttr_to_SceNpMatching2BinAttr(edata, cur_pb_attr, cur_binattr);
		}
	}

	void RoomMemberBinAttrInternal_to_SceNpMatching2RoomMemberBinAttrInternal(event_data& edata, const np2_structs::RoomMemberBinAttrInternal& pb_attr, SceNpMatching2RoomMemberBinAttrInternal* binattr_info)
	{
		binattr_info->updateDate.tick = pb_attr.updatedate();
		BinAttr_to_SceNpMatching2BinAttr(edata, pb_attr.data(), &binattr_info->data);
	}

	void RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(event_data& edata, const np2_structs::BinAttrInternal& pb_attr, SceNpMatching2RoomBinAttrInternal* binattr_info)
	{
		binattr_info->updateDate.tick = pb_attr.updatedate();
		binattr_info->updateMemberId  = pb_attr.updatememberid().value();
		BinAttr_to_SceNpMatching2BinAttr(edata, pb_attr.data(), &binattr_info->data);
	}

	void RoomGroup_to_SceNpMatching2RoomGroup(const np2_structs::RoomGroup& pb_group, SceNpMatching2RoomGroup* sce_group)
	{
		sce_group->groupId      = pb_group.groupid().value();
		sce_group->withPassword = pb_group.withpassword();
		sce_group->withLabel    = !pb_group.label().empty();
		if (!pb_group.label().empty())
		{
			const auto& label = pb_group.label();
			for (usz l_index = 0; l_index < label.size() && l_index < sizeof(sce_group->label.data); l_index++)
			{
				sce_group->label.data[l_index] = static_cast<u8>(label[l_index]);
			}
		}
		sce_group->slotNum           = pb_group.slotnum();
		sce_group->curGroupMemberNum = pb_group.curgroupmembernum();
	}

	void RoomGroups_to_SceNpMatching2RoomGroups(const google::protobuf::RepeatedPtrField<np2_structs::RoomGroup>& pb_groups, SceNpMatching2RoomGroup* sce_groups)
	{
		for (int i = 0; i < pb_groups.size(); i++)
		{
			const auto& pb_group               = pb_groups.Get(i);
			SceNpMatching2RoomGroup* sce_group = &sce_groups[i];
			RoomGroup_to_SceNpMatching2RoomGroup(pb_group, sce_group);
		}
	}

	void UserInfo_to_SceNpUserInfo(const np2_structs::UserInfo& user, SceNpUserInfo* user_info)
	{
		if (!user.npid().empty())
		{
			std::memcpy(user_info->userId.handle.data, user.npid().c_str(), std::min<usz>(16, user.npid().size()));
		}

		if (!user.onlinename().empty())
		{
			std::memcpy(user_info->name.data, user.onlinename().c_str(), std::min<usz>(48, user.onlinename().size()));
		}

		if (!user.avatarurl().empty())
		{
			std::memcpy(user_info->icon.data, user.avatarurl().c_str(), std::min<usz>(127, user.avatarurl().size()));
		}
	}

	void UserInfo_to_SceNpUserInfo2(event_data& edata, const np2_structs::UserInfo& user, SceNpUserInfo2* user_info, bool include_onlinename, bool include_avatarurl)
	{
		if (!user.npid().empty())
			std::memcpy(user_info->npId.handle.data, user.npid().c_str(), std::min<usz>(16, user.npid().size()));

		if (include_onlinename && !user.onlinename().empty())
		{
			auto* ptr = edata.allocate<SceNpOnlineName>(sizeof(SceNpOnlineName), user_info->onlineName);
			std::memcpy(ptr->data, user.onlinename().c_str(), std::min<usz>(48, user.onlinename().size()));
		}
		if (include_avatarurl && !user.avatarurl().empty())
		{
			auto* ptr = edata.allocate<SceNpAvatarUrl>(sizeof(SceNpAvatarUrl), user_info->avatarUrl);
			std::memcpy(ptr->data, user.avatarurl().c_str(), std::min<usz>(127, user.avatarurl().size()));
		}
	}

	void RoomDataExternal_to_SceNpMatching2RoomDataExternal(event_data& edata, const np2_structs::RoomDataExternal& room, SceNpMatching2RoomDataExternal* room_info, bool include_onlinename, bool include_avatarurl)
	{
		room_info->serverId           = room.serverid().value();
		room_info->worldId            = room.worldid();
		room_info->lobbyId            = room.lobbyid();
		room_info->roomId             = room.roomid();
		room_info->maxSlot            = room.maxslot().value();
		room_info->curMemberNum       = room.curmembernum().value();
		room_info->passwordSlotMask   = room.passwordslotmask();

		s32 sdk_ver;
		process_get_sdk_version(process_getpid(), sdk_ver);

		// Structure changed in sdk 3.3.0
		if (sdk_ver >= 0x330000)
		{
			room_info->publicSlotNum      = room.publicslotnum().value();
			room_info->privateSlotNum     = room.privateslotnum().value();
			room_info->openPublicSlotNum  = room.openpublicslotnum().value();
			room_info->openPrivateSlotNum = room.openprivateslotnum().value();
		}
		else
		{
			room_info->publicSlotNum      = 0;
			room_info->privateSlotNum     = 0;
			room_info->openPublicSlotNum  = 0;
			room_info->openPrivateSlotNum = 0;
		}

		if (room.has_owner())
		{
			auto* ptr_owner = edata.allocate<SceNpUserInfo2>(sizeof(SceNpUserInfo2), room_info->owner);
			UserInfo_to_SceNpUserInfo2(edata, room.owner(), ptr_owner, include_onlinename, include_avatarurl);
		}

		if (room.roomgroup_size() != 0)
		{
			room_info->roomGroupNum = room.roomgroup_size();
			auto* ptr_groups        = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum, room_info->roomGroup);
			RoomGroups_to_SceNpMatching2RoomGroups(room.roomgroup(), ptr_groups);
		}

		room_info->flagAttr = room.flagattr();

		if (room.roomsearchableintattrexternal_size() != 0)
		{
			room_info->roomSearchableIntAttrExternalNum = room.roomsearchableintattrexternal_size();
			auto* ptr_int_attr                          = edata.allocate<SceNpMatching2IntAttr>(sizeof(SceNpMatching2IntAttr) * room_info->roomSearchableIntAttrExternalNum, room_info->roomSearchableIntAttrExternal);
			for (int a_index = 0; a_index < room.roomsearchableintattrexternal_size(); a_index++)
			{
				const auto& pb_int_attr   = room.roomsearchableintattrexternal(a_index);
				ptr_int_attr[a_index].id  = pb_int_attr.id().value();
				ptr_int_attr[a_index].num = pb_int_attr.num();
			}
		}

		if (room.roomsearchablebinattrexternal_size() != 0)
		{
			room_info->roomSearchableBinAttrExternalNum = room.roomsearchablebinattrexternal_size();
			auto* ptr_bin_attr                          = edata.allocate<SceNpMatching2BinAttr>(sizeof(SceNpMatching2BinAttr) * room_info->roomSearchableBinAttrExternalNum, room_info->roomSearchableBinAttrExternal);
			BinAttrs_to_SceNpMatching2BinAttrs(edata, room.roomsearchablebinattrexternal(), ptr_bin_attr);
		}

		if (room.roombinattrexternal_size() != 0)
		{
			room_info->roomBinAttrExternalNum = room.roombinattrexternal_size();
			auto* ptr_bin_attr                = edata.allocate<SceNpMatching2BinAttr>(sizeof(SceNpMatching2BinAttr) * room_info->roomBinAttrExternalNum, room_info->roomBinAttrExternal);
			BinAttrs_to_SceNpMatching2BinAttrs(edata, room.roombinattrexternal(), ptr_bin_attr);
		}
	}

	void RoomMemberDataExternal_to_SceNpMatching2RoomMemberDataExternal(event_data& edata, const np2_structs::RoomMemberDataExternal& member, SceNpMatching2RoomMemberDataExternal* member_info, bool include_onlinename, bool include_avatarurl)
	{
		ensure(member.has_userinfo());

		UserInfo_to_SceNpUserInfo2(edata, member.userinfo(), &member_info->userInfo, include_onlinename, include_avatarurl);
		member_info->joinDate.tick = member.joindate();
		member_info->role = member.role().value();
	}

	void SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(event_data& edata, const np2_structs::SearchRoomResponse& resp, SceNpMatching2SearchRoomResponse* search_resp)
	{
		search_resp->range.size       = resp.rooms_size();
		search_resp->range.startIndex = resp.startindex();
		search_resp->range.total      = resp.total();

		SceNpMatching2RoomDataExternal* prev_room = nullptr;
		for (int i = 0; i < resp.rooms_size(); i++)
		{
			const auto& pb_room = resp.rooms(i);
			SceNpMatching2RoomDataExternal* cur_room = edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), (i > 0) ? prev_room->next : search_resp->roomDataExternal);
			RoomDataExternal_to_SceNpMatching2RoomDataExternal(edata, pb_room, cur_room, true, true);
			prev_room = cur_room;
		}
	}

	void GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(event_data& edata, const np2_structs::GetRoomDataExternalListResponse& resp, SceNpMatching2GetRoomDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl)
	{
		get_resp->roomDataExternalNum = resp.rooms_size();

		SceNpMatching2RoomDataExternal* prev_room = nullptr;
		for (int i = 0; i < resp.rooms_size(); i++)
		{
			const auto& pb_room = resp.rooms(i);
			SceNpMatching2RoomDataExternal* cur_room = edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), (i > 0) ? prev_room->next : get_resp->roomDataExternal);

			RoomDataExternal_to_SceNpMatching2RoomDataExternal(edata, pb_room, cur_room, include_onlinename, include_avatarurl);
			prev_room = cur_room;
		}
	}

	void GetRoomMemberDataExternalListResponse_to_SceNpMatching2GetRoomMemberDataExternalListResponse(event_data& edata, const np2_structs::GetRoomMemberDataExternalListResponse& resp, SceNpMatching2GetRoomMemberDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl)
	{
		get_resp->roomMemberDataExternalNum = resp.members_size();

		SceNpMatching2RoomMemberDataExternal* prev_member = nullptr;
		for (int i = 0; i < resp.members_size(); i++)
		{
			const auto& pb_member = resp.members(i);
			SceNpMatching2RoomMemberDataExternal* cur_member = edata.allocate<SceNpMatching2RoomMemberDataExternal>(sizeof(SceNpMatching2RoomMemberDataExternal), (i > 0) ? prev_member->next : get_resp->roomMemberDataExternal);

			RoomMemberDataExternal_to_SceNpMatching2RoomMemberDataExternal(edata, pb_member, cur_member, include_onlinename, include_avatarurl);
			prev_member = cur_member;
		}
	}

	u16 RoomDataInternal_to_SceNpMatching2RoomDataInternal(event_data& edata, const np2_structs::RoomDataInternal& resp, SceNpMatching2RoomDataInternal* room_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl)
	{
		u16 member_id               = 0;
		room_info->serverId         = resp.serverid().value();
		room_info->worldId          = resp.worldid();
		room_info->lobbyId          = resp.lobbyid();
		room_info->roomId           = resp.roomid();
		room_info->passwordSlotMask = resp.passwordslotmask();
		room_info->maxSlot          = resp.maxslot();

		if (resp.roomgroup_size() != 0)
		{
			room_info->roomGroupNum = resp.roomgroup_size();
			auto* ptr_groups        = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum, room_info->roomGroup);
			RoomGroups_to_SceNpMatching2RoomGroups(resp.roomgroup(), ptr_groups);
		}

		room_info->memberList.membersNum = static_cast<u32>(resp.memberlist_size());
		edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal) * room_info->memberList.membersNum, room_info->memberList.members);

		for (int i = 0; i < resp.memberlist_size(); i++)
		{
			const auto& pb_member                            = resp.memberlist(i);
			SceNpMatching2RoomMemberDataInternal* sce_member = &room_info->memberList.members[i];

			if (i < (resp.memberlist_size() - 1))
			{
				sce_member->next = room_info->memberList.members + i + 1;
				edata.add_relocation<SceNpMatching2RoomMemberDataInternal>(sce_member->next);
			}

			RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, pb_member, room_info, sce_member, include_onlinename, include_avatarurl);
		}

		for (u32 i = 0; i < room_info->memberList.membersNum; i++)
		{
			SceNpMatching2RoomMemberDataInternal* sce_member = &room_info->memberList.members[i];
			if (strcmp(sce_member->userInfo.npId.handle.data, npid.handle.data) == 0)
			{
				room_info->memberList.me = room_info->memberList.members + i;
				edata.add_relocation<SceNpMatching2RoomMemberDataInternal>(room_info->memberList.me);
				member_id = sce_member->memberId;
				break;
			}
		}

		for (u32 i = 0; i < room_info->memberList.membersNum; i++)
		{
			SceNpMatching2RoomMemberDataInternal* sce_member = &room_info->memberList.members[i];
			if (sce_member->memberId == resp.ownerid().value())
			{
				room_info->memberList.owner = room_info->memberList.members + i;
				edata.add_relocation<SceNpMatching2RoomMemberDataInternal>(room_info->memberList.owner);
				break;
			}
		}

		room_info->flagAttr = resp.flagattr();

		if (resp.roombinattrinternal_size() != 0)
		{
			room_info->roomBinAttrInternalNum = resp.roombinattrinternal_size();
			auto* ptr_bin_attr                = edata.allocate<SceNpMatching2RoomBinAttrInternal>(sizeof(SceNpMatching2RoomBinAttrInternal) * room_info->roomBinAttrInternalNum, room_info->roomBinAttrInternal);

			for (u32 b_index = 0; b_index < room_info->roomBinAttrInternalNum; b_index++)
			{
				const auto& pb_bin_attr               = resp.roombinattrinternal(b_index);
				ptr_bin_attr[b_index].updateDate.tick = pb_bin_attr.updatedate();
				ptr_bin_attr[b_index].updateMemberId  = pb_bin_attr.updatememberid().value();

				ptr_bin_attr[b_index].data.id   = pb_bin_attr.data().id().value();
				ptr_bin_attr[b_index].data.size = ::narrow<u32>(pb_bin_attr.data().data().size());
				auto* ptr_bin_attr_data         = edata.allocate<u8>(ptr_bin_attr[b_index].data.size, ptr_bin_attr[b_index].data.ptr);
				memcpy(ptr_bin_attr_data, pb_bin_attr.data().data().data(), pb_bin_attr.data().data().size());
			}
		}

		return member_id;
	}

	void RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(event_data& edata, const np2_structs::RoomMemberDataInternal& member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data, bool include_onlinename, bool include_avatarurl)
	{
		UserInfo_to_SceNpUserInfo2(edata, member_data.userinfo(), &sce_member_data->userInfo, include_onlinename, include_avatarurl);
		sce_member_data->joinDate.tick = member_data.joindate();
		sce_member_data->memberId      = member_data.memberid();
		sce_member_data->teamId        = member_data.teamid().value();

		if (member_data.has_roomgroup())
		{
			const auto& pb_roomgroup = member_data.roomgroup();
			if (room_info)
			{
				// If we have SceNpMatching2RoomDataInternal available we point the pointers to the group there
				sce_member_data->roomGroup = room_info->roomGroup + (pb_roomgroup.groupid().value() - 1);
				edata.add_relocation<SceNpMatching2RoomGroup>(sce_member_data->roomGroup);
			}
			else
			{
				// Otherwise we allocate for it
				auto* ptr_group = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup), sce_member_data->roomGroup);
				RoomGroup_to_SceNpMatching2RoomGroup(pb_roomgroup, ptr_group);
			}
		}

		sce_member_data->natType  = member_data.nattype().value();
		sce_member_data->flagAttr = member_data.flagattr();

		if (member_data.roommemberbinattrinternal_size() != 0)
		{
			sce_member_data->roomMemberBinAttrInternalNum = member_data.roommemberbinattrinternal_size();
			auto* sce_binattrs                            = edata.allocate<SceNpMatching2RoomMemberBinAttrInternal>(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * sce_member_data->roomMemberBinAttrInternalNum, sce_member_data->roomMemberBinAttrInternal);
			for (u32 b_index = 0; b_index < sce_member_data->roomMemberBinAttrInternalNum; b_index++)
			{
				const auto& pb_battr                  = member_data.roommemberbinattrinternal(b_index);
				sce_binattrs[b_index].updateDate.tick = pb_battr.updatedate();

				sce_binattrs[b_index].data.id   = pb_battr.data().id().value();
				sce_binattrs[b_index].data.size = ::narrow<u32>(pb_battr.data().data().size());
				auto* sce_binattr_data          = edata.allocate<u8>(sce_binattrs[b_index].data.size, sce_binattrs[b_index].data.ptr);
				memcpy(sce_binattr_data, pb_battr.data().data().data(), pb_battr.data().data().size());
			}
		}
	}

	void RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(event_data& edata, const np2_structs::RoomMemberUpdateInfo& update_info, SceNpMatching2RoomMemberUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl)
	{
		sce_update_info->eventCause = 0;
		if (update_info.has_optdata())
		{
			const auto& opt_data            = update_info.optdata();
			sce_update_info->optData.length = ::narrow<u32>(opt_data.data().size());
			for (usz i = 0; i < 16 && i < opt_data.data().size(); i++)
			{
				sce_update_info->optData.data[i] = static_cast<u8>(opt_data.data()[i]);
			}
		}

		if (update_info.has_roommemberdatainternal())
		{
			const auto& pb_member       = update_info.roommemberdatainternal();
			auto* ptr_roomemberinternal = edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal), sce_update_info->roomMemberDataInternal);

			// TODO: Pass room_info
			RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, pb_member, nullptr, ptr_roomemberinternal, include_onlinename, include_avatarurl);
		}
	}

	void RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const np2_structs::RoomUpdateInfo& update_info, SceNpMatching2RoomUpdateInfo* sce_update_info)
	{
		sce_update_info->errorCode  = 0;
		sce_update_info->eventCause = 0;
		if (update_info.has_optdata())
		{
			const auto& opt_data            = update_info.optdata();
			sce_update_info->optData.length = ::narrow<u32>(opt_data.data().size());
			for (usz i = 0; i < 16 && i < opt_data.data().size(); i++)
			{
				sce_update_info->optData.data[i] = static_cast<u8>(opt_data.data()[i]);
			}
		}
	}

	void RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(event_data& edata, const np2_structs::RoomDataInternalUpdateInfo& update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl)
	{
		auto* sce_room_data = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), sce_update_info->newRoomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, update_info.newroomdatainternal(), sce_room_data, npid, include_onlinename, include_avatarurl);

		if (sce_room_data->flagAttr != update_info.prevflagattr())
		{
			sce_update_info->newFlagAttr = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::flagAttr);
			edata.add_relocation<u32>(sce_update_info->newFlagAttr);
			auto* ptr_sce_prevflag = edata.allocate<SceNpMatching2FlagAttr>(sizeof(SceNpMatching2FlagAttr), sce_update_info->prevFlagAttr);
			*ptr_sce_prevflag      = update_info.prevflagattr();
		}

		if (sce_room_data->passwordSlotMask != update_info.prevroompasswordslotmask())
		{
			sce_update_info->newRoomPasswordSlotMask = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::passwordSlotMask);
			edata.add_relocation<u64>(sce_update_info->newRoomPasswordSlotMask);
			auto* ptr_sce_prevpass = edata.allocate<SceNpMatching2RoomPasswordSlotMask>(sizeof(SceNpMatching2RoomPasswordSlotMask), sce_update_info->prevRoomPasswordSlotMask);
			*ptr_sce_prevpass      = update_info.prevroompasswordslotmask();
		}

		if (!update_info.newroomgroup().empty())
		{
			rpcn_log.todo("RoomDataInternalUpdateInfo::newRoomGroup");
			// TODO
			// sce_update_info->newRoomGroupNum = update_info.newroomgroup().size();
			// vm::ptr<SceNpMatching2RoomGroup> group_info(allocate(sizeof(SceNpMatching2RoomGroup) * sce_update_info->newRoomGroupNum));
			// RoomGroups_to_SceNpMatching2RoomGroup(update_info.newroomgroup(), group_info);
			// sce_update_info->newRoomGroup = group_info;
		}

		if (update_info.newroombinattrinternal_size() != 0)
		{
			const auto get_ptr_for_binattr = [&](u16 binattr_id) -> vm::bptr<SceNpMatching2RoomBinAttrInternal>
			{
				vm::bptr<SceNpMatching2RoomBinAttrInternal> ret_ptr = sce_room_data->roomBinAttrInternal;
				for (u32 i = 0; i < sce_room_data->roomBinAttrInternalNum; i++)
				{
					if (ret_ptr->data.id == binattr_id)
						return ret_ptr;

					ret_ptr++;
				}
				rpcn_log.fatal("RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo: Couldn't find matching roomBinAttrInternal!");
				return vm::null;
			};

			sce_update_info->newRoomBinAttrInternalNum = update_info.newroombinattrinternal_size();
			edata.allocate_ptr_array<SceNpMatching2RoomBinAttrInternal>(sce_update_info->newRoomBinAttrInternalNum, sce_update_info->newRoomBinAttrInternal);
			for (u32 i = 0; i < sce_update_info->newRoomBinAttrInternalNum; i++)
			{
				sce_update_info->newRoomBinAttrInternal[i] = get_ptr_for_binattr(update_info.newroombinattrinternal(i).value());
			}
		}
	}

	void RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(event_data& edata, const np2_structs::RoomMemberDataInternalUpdateInfo& update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl)
	{
		auto* sce_room_member_data = edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal), sce_update_info->newRoomMemberDataInternal);
		RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, update_info.newroommemberdatainternal(), nullptr, sce_room_member_data, include_onlinename, include_avatarurl);

		if (sce_update_info->newRoomMemberDataInternal->flagAttr != update_info.prevflagattr())
		{
			sce_update_info->newFlagAttr = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::flagAttr);
			edata.add_relocation<u32>(sce_update_info->newFlagAttr);
			auto* ptr_sce_prevflag = edata.allocate<SceNpMatching2FlagAttr>(sizeof(SceNpMatching2FlagAttr), sce_update_info->prevFlagAttr);
			*ptr_sce_prevflag      = update_info.prevflagattr();
		}

		if (sce_update_info->newRoomMemberDataInternal->teamId != update_info.prevteamid().value())
		{
			sce_update_info->newTeamId = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::teamId);
			edata.add_relocation<u8>(sce_update_info->newTeamId);
		}

		if (update_info.newroommemberbinattrinternal_size() != 0)
		{
			const auto get_ptr_for_binattr = [&](u16 binattr_id) -> vm::bptr<SceNpMatching2RoomMemberBinAttrInternal>
			{
				vm::bptr<SceNpMatching2RoomMemberBinAttrInternal> ret_ptr = sce_room_member_data->roomMemberBinAttrInternal;
				for (u32 i = 0; i < sce_room_member_data->roomMemberBinAttrInternalNum; i++)
				{
					if (ret_ptr->data.id == binattr_id)
						return ret_ptr;

					ret_ptr++;
				}
				rpcn_log.fatal("RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo: Couldn't find matching roomMemberBinAttrInternal!");
				return vm::null;
			};

			sce_update_info->newRoomMemberBinAttrInternalNum = update_info.newroommemberbinattrinternal_size();
			edata.allocate_ptr_array<SceNpMatching2RoomMemberBinAttrInternal>(sce_update_info->newRoomMemberBinAttrInternalNum, sce_update_info->newRoomMemberBinAttrInternal);
			for (u32 i = 0; i < sce_update_info->newRoomMemberBinAttrInternalNum; i++)
			{
				sce_update_info->newRoomMemberBinAttrInternal[i] = get_ptr_for_binattr(update_info.newroommemberbinattrinternal(i).value());
			}
		}
	}

	void GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const np2_structs::GetPingInfoResponse& resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp)
	{
		sce_resp->serverId = resp.serverid().value();
		sce_resp->worldId  = resp.worldid();
		sce_resp->roomId   = resp.roomid();
		sce_resp->rtt      = resp.rtt();
	}

	void RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(event_data& edata, const np2_structs::RoomMessageInfo& mi, SceNpMatching2RoomMessageInfo* sce_mi, bool include_onlinename, bool include_avatarurl)
	{
		sce_mi->filtered = mi.filtered();
		sce_mi->castType = mi.casttype().value();

		if (sce_mi->castType != SCE_NP_MATCHING2_CASTTYPE_BROADCAST)
		{
			edata.allocate<SceNpMatching2RoomMessageDestination>(sizeof(SceNpMatching2RoomMessageDestination), sce_mi->dst);
		}

		switch (sce_mi->castType)
		{
		case SCE_NP_MATCHING2_CASTTYPE_BROADCAST: break;
		case SCE_NP_MATCHING2_CASTTYPE_UNICAST: sce_mi->dst->unicastTarget = mi.dst(0).value(); break;
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST:
		{
			sce_mi->dst->multicastTarget.memberIdNum = mi.dst_size();
			edata.allocate<u16>(sizeof(u16) * mi.dst_size(), sce_mi->dst->multicastTarget.memberId);
			for (int i = 0; i < mi.dst_size(); i++)
			{
				sce_mi->dst->multicastTarget.memberId[i] = mi.dst(i).value();
			}
			break;
		}
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST_TEAM: sce_mi->dst->multicastTargetTeamId = ::narrow<SceNpMatching2TeamId>(mi.dst(0).value()); break;
		default: ensure(false);
		}

		if (mi.has_srcmember())
		{
			auto* ptr_sce_userinfo = edata.allocate<SceNpUserInfo2>(sizeof(SceNpUserInfo2), sce_mi->srcMember);
			UserInfo_to_SceNpUserInfo2(edata, mi.srcmember(), ptr_sce_userinfo, include_onlinename, include_avatarurl);
		}

		if (!mi.msg().empty())
		{
			sce_mi->msgLen     = ::narrow<u32>(mi.msg().size());
			auto* ptr_msg_data = static_cast<u8*>(edata.allocate<void>(mi.msg().size(), sce_mi->msg));
			memcpy(ptr_msg_data, mi.msg().data(), mi.msg().size());
		}
	}

	void MatchingRoomStatus_to_SceNpMatchingRoomStatus(event_data& edata, const np2_structs::MatchingRoomStatus& resp, SceNpMatchingRoomStatus* room_status)
	{
		const auto& vec_id = resp.id();
		ensure(vec_id.size() == 28, "Invalid room id in MatchingRoomStatus");

		for (usz i = 0; i < 28; i++)
		{
			room_status->id.opt[i] = static_cast<u8>(vec_id[i]);
		}

		// In some events the member list can be empty
		if (resp.members_size() > 0)
		{
			room_status->num = resp.members_size();
			SceNpMatchingRoomMember* prev_member{};

			for (int i = 0; i < resp.members_size(); i++)
			{
				auto* cur_member     = edata.allocate<SceNpMatchingRoomMember>(sizeof(SceNpMatchingRoomMember), (i > 0) ? prev_member->next : room_status->members);
				const auto& member   = resp.members(i);
				ensure(member.has_info(), "Invalid member in MatchingRoomStatus list");

				cur_member->owner = member.owner() ? 1 : 0;
				UserInfo_to_SceNpUserInfo(member.info(), &cur_member->user_info);

				prev_member = cur_member;
			}
		}

		if (!resp.kick_actor().empty())
		{
			auto* npid = edata.allocate<SceNpId>(sizeof(SceNpId), room_status->kick_actor);
			std::memcpy(npid->handle.data, resp.kick_actor().c_str(), std::min<usz>(16, resp.kick_actor().size()));
		}

		if (!resp.opt().empty())
		{
			room_status->opt_len = ::narrow<u32>(resp.opt().size());
			u8* opt_data         = static_cast<u8*>(edata.allocate<void>(resp.opt().size(), room_status->opt));

			memcpy(opt_data, resp.opt().data(), resp.opt().size());
		}
	}

	void MatchingRoomStatus_to_SceNpMatchingJoinedRoomInfo(event_data& edata, const np2_structs::MatchingRoomStatus& resp, SceNpMatchingJoinedRoomInfo* room_info)
	{
		// The use of SceNpLobbyId is unclear as it is never specified by the client except in further operations, so we always set it to a series of 0 and a 1
		room_info->lobbyid.opt[27] = 1;
		MatchingRoomStatus_to_SceNpMatchingRoomStatus(edata, resp, &room_info->room_status);
	}

	void MatchingAttr_to_SceNpMatchingAttr(event_data& edata, const google::protobuf::RepeatedPtrField<np2_structs::MatchingAttr>& attr_list, vm::bptr<SceNpMatchingAttr>& first_attr)
	{
		if (attr_list.size() > 0)
		{
			SceNpMatchingAttr* cur_attr = nullptr;

			for (int i_attr = 0; i_attr < attr_list.size(); i_attr++)
			{
				const auto& attr = attr_list.Get(i_attr);
				cur_attr         = edata.allocate<SceNpMatchingAttr>(sizeof(SceNpMatchingAttr), cur_attr ? cur_attr->next : first_attr);

				cur_attr->type = attr.attr_type();
				cur_attr->id   = attr.attr_id();
				if (!attr.data().empty())
				{
					cur_attr->value.data.size = ::narrow<u32>(attr.data().size());
					u8* data_ptr              = static_cast<u8*>(edata.allocate<void>(attr.data().size(), cur_attr->value.data.ptr));
					memcpy(data_ptr, attr.data().data(), attr.data().size());
				}
				else
				{
					cur_attr->value.num = attr.num();
				}
			}
		}
	}

	void MatchingRoom_to_SceNpMatchingRoom(event_data& edata, const np2_structs::MatchingRoom& resp, SceNpMatchingRoom* room)
	{
		ensure(room && resp.id().size() == sizeof(SceNpRoomId::opt));
		memcpy(room->id.opt, resp.id().data(), sizeof(SceNpRoomId::opt));
		MatchingAttr_to_SceNpMatchingAttr(edata, resp.attr(), room->attr);
	}

	void MatchingRoomList_to_SceNpMatchingRoomList(event_data& edata, const np2_structs::MatchingRoomList& resp, SceNpMatchingRoomList* room_list)
	{
		// The use of SceNpLobbyId is unclear as it is never specified by the client except in further operations, so we always set it to a series of 0 and a 1
		room_list->lobbyid.opt[27] = 1;
		room_list->range.start     = resp.start();
		room_list->range.total     = resp.total();

		if (resp.rooms_size() > 0)
		{
			room_list->range.results = resp.rooms_size();

			SceNpMatchingRoom* cur_room = nullptr;

			for (int i = 0; i < resp.rooms_size(); i++)
			{
				const auto& room = resp.rooms(i);
				cur_room         = edata.allocate<SceNpMatchingRoom>(sizeof(SceNpMatchingRoom), cur_room ? cur_room->next : room_list->head);
				MatchingRoom_to_SceNpMatchingRoom(edata, room, cur_room);
			}
		}
	}

	void MatchingSearchJoinRoomInfo_to_SceNpMatchingSearchJoinRoomInfo(event_data& edata, const np2_structs::MatchingSearchJoinRoomInfo& resp, SceNpMatchingSearchJoinRoomInfo* room_info)
	{
		ensure(resp.has_room());
		room_info->lobbyid.opt[27] = 1;
		MatchingRoomStatus_to_SceNpMatchingRoomStatus(edata, resp.room(), &room_info->room_status);
		MatchingAttr_to_SceNpMatchingAttr(edata, resp.attr(), room_info->attr);
	}

	void OptParam_to_SceNpMatching2SignalingOptParam(const np2_structs::OptParam& resp, SceNpMatching2SignalingOptParam* opt_param)
	{
		opt_param->type = resp.type().value();
		opt_param->flag = resp.flag().value();
		opt_param->hubMemberId = resp.hubmemberid().value();
	}

} // namespace np
