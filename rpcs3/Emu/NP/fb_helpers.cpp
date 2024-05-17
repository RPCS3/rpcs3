#include "stdafx.h"
#include "np_handler.h"
#include "Emu/Cell/lv2/sys_process.h"

LOG_CHANNEL(rpcn_log, "rpcn");

namespace np
{
	void np_handler::BinAttr_to_SceNpMatching2BinAttr(event_data& edata, const BinAttr* bin_attr, SceNpMatching2BinAttr* binattr_info)
	{
		binattr_info->id   = bin_attr->id();
		binattr_info->size = bin_attr->data()->size();
		auto* ptr          = edata.allocate<u8>(binattr_info->size, binattr_info->ptr);
		for (flatbuffers::uoffset_t i = 0; i < bin_attr->data()->size(); i++)
		{
			ptr[i] = bin_attr->data()->Get(i);
		}
	}

	void np_handler::BinAttrs_to_SceNpMatching2BinAttrs(event_data& edata, const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, SceNpMatching2BinAttr* binattr_info)
	{
		for (flatbuffers::uoffset_t i = 0; i < fb_attr->size(); i++)
		{
			auto cur_fb_attr = fb_attr->Get(i);
			auto cur_binattr = binattr_info + i;

			BinAttr_to_SceNpMatching2BinAttr(edata, cur_fb_attr, cur_binattr);
		}
	}

	void np_handler::RoomMemberBinAttrInternal_to_SceNpMatching2RoomMemberBinAttrInternal(event_data& edata, const RoomMemberBinAttrInternal* fb_attr, SceNpMatching2RoomMemberBinAttrInternal* binattr_info)
	{
		binattr_info->updateDate.tick = fb_attr->updateDate();
		BinAttr_to_SceNpMatching2BinAttr(edata, fb_attr->data(), &binattr_info->data);
	}

	void np_handler::RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(event_data& edata, const BinAttrInternal* fb_attr, SceNpMatching2RoomBinAttrInternal* binattr_info)
	{
		binattr_info->updateDate.tick = fb_attr->updateDate();
		binattr_info->updateMemberId  = fb_attr->updateMemberId();
		BinAttr_to_SceNpMatching2BinAttr(edata, fb_attr->data(), &binattr_info->data);
	}

	void np_handler::RoomGroup_to_SceNpMatching2RoomGroup(const RoomGroup* fb_group, SceNpMatching2RoomGroup* sce_group)
	{
		sce_group->groupId      = fb_group->groupId();
		sce_group->withPassword = fb_group->withPassword();
		sce_group->withLabel    = fb_group->withLabel();
		if (fb_group->label())
		{
			for (flatbuffers::uoffset_t l_index = 0; l_index < fb_group->label()->size(); l_index++)
			{
				sce_group->label.data[l_index] = fb_group->label()->Get(l_index);
			}
		}
		sce_group->slotNum           = fb_group->slotNum();
		sce_group->curGroupMemberNum = fb_group->curGroupMemberNum();
	}

	void np_handler::RoomGroups_to_SceNpMatching2RoomGroups(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_groups, SceNpMatching2RoomGroup* sce_groups)
	{
		for (flatbuffers::uoffset_t i = 0; i < fb_groups->size(); i++)
		{
			const auto* fb_group               = fb_groups->Get(i);
			SceNpMatching2RoomGroup* sce_group = &sce_groups[i];
			RoomGroup_to_SceNpMatching2RoomGroup(fb_group, sce_group);
		}
	}

	void np_handler::UserInfo2_to_SceNpUserInfo2(event_data& edata, const UserInfo2* user, SceNpUserInfo2* user_info, bool include_onlinename, bool include_avatarurl)
	{
		if (user->npId())
			std::memcpy(user_info->npId.handle.data, user->npId()->c_str(), std::min<usz>(16, user->npId()->size()));

		if (include_onlinename && user->onlineName())
		{
			auto* ptr = edata.allocate<SceNpOnlineName>(sizeof(SceNpOnlineName), user_info->onlineName);
			std::memcpy(ptr->data, user->onlineName()->c_str(), std::min<usz>(48, user->onlineName()->size()));
		}
		if (include_avatarurl && user->avatarUrl())
		{
			auto* ptr = edata.allocate<SceNpAvatarUrl>(sizeof(SceNpAvatarUrl), user_info->avatarUrl);
			std::memcpy(ptr->data, user->avatarUrl()->c_str(), std::min<usz>(127, user->avatarUrl()->size()));
		}
	}

	void np_handler::RoomDataExternal_to_SceNpMatching2RoomDataExternal(event_data& edata, const RoomDataExternal* room, SceNpMatching2RoomDataExternal* room_info, bool include_onlinename, bool include_avatarurl)
	{
		room_info->serverId           = room->serverId();
		room_info->worldId            = room->worldId();
		room_info->lobbyId            = room->lobbyId();
		room_info->roomId             = room->roomId();
		room_info->maxSlot            = room->maxSlot();
		room_info->curMemberNum       = room->curMemberNum();
		room_info->passwordSlotMask   = room->passwordSlotMask();

		s32 sdk_ver;
		process_get_sdk_version(process_getpid(), sdk_ver);

		// Structure changed in sdk 3.3.0
		if (sdk_ver >= 0x330000)
		{
			room_info->publicSlotNum = room->publicSlotNum();
			room_info->privateSlotNum = room->privateSlotNum();
			room_info->openPublicSlotNum = room->openPublicSlotNum();
			room_info->openPrivateSlotNum = room->openPrivateSlotNum();
		}
		else
		{
			room_info->publicSlotNum = 0;
			room_info->privateSlotNum = 0;
			room_info->openPublicSlotNum = 0;
			room_info->openPrivateSlotNum = 0;
		}

		if (auto owner = room->owner())
		{
			auto* ptr_owner = edata.allocate<SceNpUserInfo2>(sizeof(SceNpUserInfo2), room_info->owner);
			UserInfo2_to_SceNpUserInfo2(edata, owner, ptr_owner, include_onlinename, include_avatarurl);
		}

		if (room->roomGroup() && room->roomGroup()->size() != 0)
		{
			room_info->roomGroupNum = room->roomGroup()->size();
			auto* ptr_groups        = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum, room_info->roomGroup);
			RoomGroups_to_SceNpMatching2RoomGroups(room->roomGroup(), ptr_groups);
		}

		room_info->flagAttr = room->flagAttr();

		if (room->roomSearchableIntAttrExternal() && room->roomSearchableIntAttrExternal()->size() != 0)
		{
			room_info->roomSearchableIntAttrExternalNum = room->roomSearchableIntAttrExternal()->size();
			auto* ptr_int_attr                          = edata.allocate<SceNpMatching2IntAttr>(sizeof(SceNpMatching2IntAttr) * room_info->roomSearchableIntAttrExternalNum, room_info->roomSearchableIntAttrExternal);
			for (flatbuffers::uoffset_t a_index = 0; a_index < room->roomSearchableIntAttrExternal()->size(); a_index++)
			{
				auto fb_int_attr          = room->roomSearchableIntAttrExternal()->Get(a_index);
				ptr_int_attr[a_index].id  = fb_int_attr->id();
				ptr_int_attr[a_index].num = fb_int_attr->num();
			}
		}

		if (room->roomSearchableBinAttrExternal() && room->roomSearchableBinAttrExternal()->size() != 0)
		{
			room_info->roomSearchableBinAttrExternalNum = room->roomSearchableBinAttrExternal()->size();
			auto* ptr_bin_attr                          = edata.allocate<SceNpMatching2BinAttr>(sizeof(SceNpMatching2BinAttr) * room_info->roomSearchableBinAttrExternalNum, room_info->roomSearchableBinAttrExternal);
			BinAttrs_to_SceNpMatching2BinAttrs(edata, room->roomSearchableBinAttrExternal(), ptr_bin_attr);
		}

		if (room->roomBinAttrExternal() && room->roomBinAttrExternal()->size() != 0)
		{
			room_info->roomBinAttrExternalNum = room->roomBinAttrExternal()->size();
			auto* ptr_bin_attr                = edata.allocate<SceNpMatching2BinAttr>(sizeof(SceNpMatching2BinAttr) * room_info->roomBinAttrExternalNum, room_info->roomBinAttrExternal);
			BinAttrs_to_SceNpMatching2BinAttrs(edata, room->roomBinAttrExternal(), ptr_bin_attr);
		}
	}

	void np_handler::SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(event_data& edata, const SearchRoomResponse* resp, SceNpMatching2SearchRoomResponse* search_resp)
	{
		search_resp->range.size       = resp->size();
		search_resp->range.startIndex = resp->startIndex();
		search_resp->range.total      = resp->total();

		if (resp->rooms() && resp->rooms()->size() != 0)
		{
			SceNpMatching2RoomDataExternal* prev_room = nullptr;
			for (flatbuffers::uoffset_t i = 0; i < resp->rooms()->size(); i++)
			{
				auto* fb_room = resp->rooms()->Get(i);
				SceNpMatching2RoomDataExternal* cur_room;

				cur_room = (i > 0) ? edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), prev_room->next) :
				                     edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), search_resp->roomDataExternal);

				RoomDataExternal_to_SceNpMatching2RoomDataExternal(edata, fb_room, cur_room, true, true);
				prev_room = cur_room;
			}
		}
	}

	void np_handler::GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(event_data& edata, const GetRoomDataExternalListResponse* resp, SceNpMatching2GetRoomDataExternalListResponse* get_resp, bool include_onlinename, bool include_avatarurl)
	{
		get_resp->roomDataExternalNum = resp->rooms() ? resp->rooms()->size() : 0;

		SceNpMatching2RoomDataExternal* prev_room = nullptr;
		for (flatbuffers::uoffset_t i = 0; i < get_resp->roomDataExternalNum; i++)
		{
			auto* fb_room = resp->rooms()->Get(i);
			SceNpMatching2RoomDataExternal* cur_room;

			cur_room = (i > 0) ? edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), prev_room->next) :
			                     edata.allocate<SceNpMatching2RoomDataExternal>(sizeof(SceNpMatching2RoomDataExternal), get_resp->roomDataExternal);

			RoomDataExternal_to_SceNpMatching2RoomDataExternal(edata, fb_room, cur_room, include_onlinename, include_avatarurl);
			prev_room = cur_room;
		}
	}

	u16 np_handler::RoomDataInternal_to_SceNpMatching2RoomDataInternal(event_data& edata, const RoomDataInternal* resp, SceNpMatching2RoomDataInternal* room_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl)
	{
		u16 member_id               = 0;
		room_info->serverId         = resp->serverId();
		room_info->worldId          = resp->worldId();
		room_info->lobbyId          = resp->lobbyId();
		room_info->roomId           = resp->roomId();
		room_info->passwordSlotMask = resp->passwordSlotMask();
		room_info->maxSlot          = resp->maxSlot();

		if (resp->roomGroup() && resp->roomGroup()->size() != 0)
		{
			room_info->roomGroupNum = resp->roomGroup()->size();
			auto* ptr_groups        = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum, room_info->roomGroup);
			RoomGroups_to_SceNpMatching2RoomGroups(resp->roomGroup(), ptr_groups);
		}

		room_info->memberList.membersNum = static_cast<u32>(resp->memberList()->size());
		edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal) * room_info->memberList.membersNum, room_info->memberList.members);

		for (flatbuffers::uoffset_t i = 0; i < resp->memberList()->size(); i++)
		{
			auto fb_member                                   = resp->memberList()->Get(i);
			SceNpMatching2RoomMemberDataInternal* sce_member = &room_info->memberList.members[i];

			if (i < (resp->memberList()->size() - 1))
			{
				sce_member->next = room_info->memberList.members + i + 1;
				edata.add_relocation<SceNpMatching2RoomMemberDataInternal>(sce_member->next);
			}

			RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, fb_member, room_info, sce_member, include_onlinename, include_avatarurl);
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
			if (sce_member->memberId == resp->ownerId())
			{
				room_info->memberList.owner = room_info->memberList.members + i;
				edata.add_relocation<SceNpMatching2RoomMemberDataInternal>(room_info->memberList.owner);
				break;
			}
		}

		room_info->flagAttr = resp->flagAttr();

		if (resp->roomBinAttrInternal() && resp->roomBinAttrInternal()->size() != 0)
		{
			room_info->roomBinAttrInternalNum = resp->roomBinAttrInternal()->size();
			auto* ptr_bin_attr                = edata.allocate<SceNpMatching2RoomBinAttrInternal>(sizeof(SceNpMatching2RoomBinAttrInternal) * room_info->roomBinAttrInternalNum, room_info->roomBinAttrInternal);

			for (u32 b_index = 0; b_index < room_info->roomBinAttrInternalNum; b_index++)
			{
				auto fb_bin_attr                      = resp->roomBinAttrInternal()->Get(b_index);
				ptr_bin_attr[b_index].updateDate.tick = fb_bin_attr->updateDate();
				ptr_bin_attr[b_index].updateMemberId  = fb_bin_attr->updateMemberId();

				ptr_bin_attr[b_index].data.id   = fb_bin_attr->data()->id();
				ptr_bin_attr[b_index].data.size = fb_bin_attr->data()->data()->size();
				auto* ptr_bin_attr_data         = edata.allocate<u8>(ptr_bin_attr[b_index].data.size, ptr_bin_attr[b_index].data.ptr);
				for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < ptr_bin_attr[b_index].data.size; tmp_index++)
				{
					ptr_bin_attr_data[tmp_index] = fb_bin_attr->data()->data()->Get(tmp_index);
				}
			}
		}

		return member_id;
	}

	void np_handler::RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(event_data& edata, const RoomMemberDataInternal* member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data, bool include_onlinename, bool include_avatarurl)
	{
		UserInfo2_to_SceNpUserInfo2(edata, member_data->userInfo(), &sce_member_data->userInfo, include_onlinename, include_avatarurl);
		sce_member_data->joinDate.tick = member_data->joinDate();
		sce_member_data->memberId      = member_data->memberId();
		sce_member_data->teamId        = member_data->teamId();

		if (const auto* fb_roomgroup = member_data->roomGroup())
		{
			if (room_info)
			{
				// If we have SceNpMatching2RoomDataInternal available we point the pointers to the group there
				sce_member_data->roomGroup = room_info->roomGroup + (fb_roomgroup->groupId() - 1);
				edata.add_relocation<SceNpMatching2RoomGroup>(sce_member_data->roomGroup);
			}
			else
			{
				// Otherwise we allocate for it
				auto* ptr_group = edata.allocate<SceNpMatching2RoomGroup>(sizeof(SceNpMatching2RoomGroup), sce_member_data->roomGroup);
				RoomGroup_to_SceNpMatching2RoomGroup(fb_roomgroup, ptr_group);
			}
		}

		sce_member_data->natType  = member_data->natType();
		sce_member_data->flagAttr = member_data->flagAttr();

		if (member_data->roomMemberBinAttrInternal() && member_data->roomMemberBinAttrInternal()->size() != 0)
		{
			sce_member_data->roomMemberBinAttrInternalNum = member_data->roomMemberBinAttrInternal()->size();
			auto* sce_binattrs                            = edata.allocate<SceNpMatching2RoomMemberBinAttrInternal>(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * sce_member_data->roomMemberBinAttrInternalNum, sce_member_data->roomMemberBinAttrInternal);
			for (u32 b_index = 0; b_index < sce_member_data->roomMemberBinAttrInternalNum; b_index++)
			{
				const auto fb_battr                   = member_data->roomMemberBinAttrInternal()->Get(b_index);
				sce_binattrs[b_index].updateDate.tick = fb_battr->updateDate();

				sce_binattrs[b_index].data.id   = fb_battr->data()->id();
				sce_binattrs[b_index].data.size = fb_battr->data()->data()->size();
				auto* sce_binattr_data          = edata.allocate<u8>(sce_binattrs[b_index].data.size, sce_binattrs[b_index].data.ptr);
				for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < sce_binattrs[b_index].data.size; tmp_index++)
				{
					sce_binattr_data[tmp_index] = fb_battr->data()->data()->Get(tmp_index);
				}
			}
		}
	}

	void np_handler::RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(event_data& edata, const RoomMemberUpdateInfo* update_info, SceNpMatching2RoomMemberUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl)
	{
		sce_update_info->eventCause = 0;
		if (update_info->optData())
		{
			sce_update_info->optData.length = update_info->optData()->data()->size();
			for (flatbuffers::uoffset_t i = 0; i < 16; i++)
			{
				sce_update_info->optData.data[i] = update_info->optData()->data()->Get(i);
			}
		}

		if (update_info->roomMemberDataInternal())
		{
			auto fb_member              = update_info->roomMemberDataInternal();
			auto* ptr_roomemberinternal = edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal), sce_update_info->roomMemberDataInternal);

			// TODO: Pass room_info
			RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, fb_member, nullptr, ptr_roomemberinternal, include_onlinename, include_avatarurl);
		}
	}

	void np_handler::RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const RoomUpdateInfo* update_info, SceNpMatching2RoomUpdateInfo* sce_update_info)
	{
		sce_update_info->errorCode  = 0;
		sce_update_info->eventCause = 0;
		if (update_info->optData())
		{
			sce_update_info->optData.length = update_info->optData()->data()->size();
			for (flatbuffers::uoffset_t i = 0; i < 16; i++)
			{
				sce_update_info->optData.data[i] = update_info->optData()->data()->Get(i);
			}
		}
	}

	void np_handler::RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(event_data& edata, const RoomDataInternalUpdateInfo* update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid, bool include_onlinename, bool include_avatarurl)
	{
		auto* sce_room_data = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), sce_update_info->newRoomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, update_info->newRoomDataInternal(), sce_room_data, npid, include_onlinename, include_avatarurl);

		if (sce_room_data->flagAttr != update_info->prevFlagAttr())
		{
			sce_update_info->newFlagAttr = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::flagAttr);
			edata.add_relocation<u32>(sce_update_info->newFlagAttr);
			auto* ptr_sce_prevflag = edata.allocate<SceNpMatching2FlagAttr>(sizeof(SceNpMatching2FlagAttr), sce_update_info->prevFlagAttr);
			*ptr_sce_prevflag      = update_info->prevFlagAttr();
		}

		if (sce_room_data->passwordSlotMask != update_info->prevRoomPasswordSlotMask())
		{
			sce_update_info->newRoomPasswordSlotMask = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::passwordSlotMask);
			edata.add_relocation<u64>(sce_update_info->newRoomPasswordSlotMask);
			auto* ptr_sce_prevpass = edata.allocate<SceNpMatching2RoomPasswordSlotMask>(sizeof(SceNpMatching2RoomPasswordSlotMask), sce_update_info->prevRoomPasswordSlotMask);
			*ptr_sce_prevpass      = update_info->prevRoomPasswordSlotMask();
		}

		if (update_info->newRoomGroup() && update_info->newRoomGroup()->size() != 0)
		{
			rpcn_log.todo("RoomDataInternalUpdateInfo::newRoomGroup");
			// TODO
			// sce_update_info->newRoomGroupNum = update_info->newRoomGroup()->size();
			// vm::ptr<SceNpMatching2RoomGroup> group_info(allocate(sizeof(SceNpMatching2RoomGroup) * sce_update_info->newRoomGroupNum));
			// RoomGroups_to_SceNpMatching2RoomGroup(update_info->newRoomGroup(), group_info);
			// sce_update_info->newRoomGroup = group_info;
		}

		if (update_info->newRoomBinAttrInternal() && update_info->newRoomBinAttrInternal()->size() != 0)
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

			sce_update_info->newRoomBinAttrInternalNum = update_info->newRoomBinAttrInternal()->size();
			edata.allocate_ptr_array<SceNpMatching2RoomBinAttrInternal>(sce_update_info->newRoomBinAttrInternalNum, sce_update_info->newRoomBinAttrInternal);
			for (u32 i = 0; i < sce_update_info->newRoomBinAttrInternalNum; i++)
			{
				sce_update_info->newRoomBinAttrInternal[i] = get_ptr_for_binattr(update_info->newRoomBinAttrInternal()->Get(i));
			}
		}
	}

	void np_handler::RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(event_data& edata, const RoomMemberDataInternalUpdateInfo* update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info, bool include_onlinename, bool include_avatarurl)
	{
		auto* sce_room_member_data = edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal), sce_update_info->newRoomMemberDataInternal);
		RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, update_info->newRoomMemberDataInternal(), nullptr, sce_room_member_data, include_onlinename, include_avatarurl);

		if (sce_update_info->newRoomMemberDataInternal->flagAttr != update_info->prevFlagAttr())
		{
			sce_update_info->newFlagAttr = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::flagAttr);
			edata.add_relocation<u32>(sce_update_info->newFlagAttr);
			auto* ptr_sce_prevflag = edata.allocate<SceNpMatching2FlagAttr>(sizeof(SceNpMatching2FlagAttr), sce_update_info->prevFlagAttr);
			*ptr_sce_prevflag      = update_info->prevFlagAttr();
		}

		if (sce_update_info->newRoomMemberDataInternal->teamId != update_info->prevTeamId())
		{
			sce_update_info->newTeamId = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::teamId);
			edata.add_relocation<u8>(sce_update_info->newTeamId);
		}

		if (update_info->newRoomMemberBinAttrInternal() && update_info->newRoomMemberBinAttrInternal()->size() != 0)
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

			sce_update_info->newRoomMemberBinAttrInternalNum = update_info->newRoomMemberBinAttrInternal()->size();
			edata.allocate_ptr_array<SceNpMatching2RoomMemberBinAttrInternal>(sce_update_info->newRoomMemberBinAttrInternalNum, sce_update_info->newRoomMemberBinAttrInternal);
			for (u32 i = 0; i < sce_update_info->newRoomMemberBinAttrInternalNum; i++)
			{
				sce_update_info->newRoomMemberBinAttrInternal[i] = get_ptr_for_binattr(update_info->newRoomMemberBinAttrInternal()->Get(i));
			}
		}
	}

	void np_handler::GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const GetPingInfoResponse* resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp)
	{
		sce_resp->serverId = resp->serverId();
		sce_resp->worldId  = resp->worldId();
		sce_resp->roomId   = resp->roomId();
		sce_resp->rtt      = resp->rtt();
	}

	void np_handler::RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(event_data& edata, const RoomMessageInfo* mi, SceNpMatching2RoomMessageInfo* sce_mi, bool include_onlinename, bool include_avatarurl)
	{
		sce_mi->filtered = mi->filtered();
		sce_mi->castType = mi->castType();

		if (sce_mi->castType != SCE_NP_MATCHING2_CASTTYPE_BROADCAST)
		{
			edata.allocate<SceNpMatching2RoomMessageDestination>(sizeof(SceNpMatching2RoomMessageDestination), sce_mi->dst);
		}

		switch (sce_mi->castType)
		{
		case SCE_NP_MATCHING2_CASTTYPE_BROADCAST: break;
		case SCE_NP_MATCHING2_CASTTYPE_UNICAST: sce_mi->dst->unicastTarget = mi->dst()->Get(0); break;
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST:
		{
			sce_mi->dst->multicastTarget.memberIdNum = mi->dst()->size();
			edata.allocate<u16>(sizeof(u16) * mi->dst()->size(), sce_mi->dst->multicastTarget.memberId);
			for (u32 i = 0; i < mi->dst()->size(); i++)
			{
				sce_mi->dst->multicastTarget.memberId[i] = mi->dst()->Get(i);
			}
			break;
		}
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST_TEAM: sce_mi->dst->multicastTargetTeamId = ::narrow<SceNpMatching2TeamId>(mi->dst()->Get(0)); break;
		default: ensure(false);
		}

		if (auto src_member = mi->srcMember())
		{
			auto* ptr_sce_userinfo = edata.allocate<SceNpUserInfo2>(sizeof(SceNpUserInfo2), sce_mi->srcMember);
			UserInfo2_to_SceNpUserInfo2(edata, src_member, ptr_sce_userinfo, include_onlinename, include_avatarurl);
		}

		if (auto msg = mi->msg())
		{
			sce_mi->msgLen     = msg->size();
			auto* ptr_msg_data = static_cast<u8*>(edata.allocate<void>(msg->size(), sce_mi->msg));
			for (u32 i = 0; i < msg->size(); i++)
			{
				ptr_msg_data[i] = msg->Get(i);
			}
		}
	}
} // namespace np
