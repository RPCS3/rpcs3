#include "stdafx.h"
#include "np_handler.h"

LOG_CHANNEL(rpcn_log, "rpcn");

void np_handler::BinAttr_to_SceNpMatching2BinAttr(const BinAttr* bin_attr, SceNpMatching2BinAttr* binattr_info)
{
	binattr_info->id   = bin_attr->id();
	binattr_info->size = bin_attr->data()->size();
	binattr_info->ptr  = allocate(binattr_info->size);
	for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < bin_attr->data()->size(); tmp_index++)
	{
		binattr_info->ptr[tmp_index] = bin_attr->data()->Get(tmp_index);
	}
}
void np_handler::BinAttrs_to_SceNpMatching2BinAttr(const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, vm::ptr<SceNpMatching2BinAttr> binattr_info)
{
	for (flatbuffers::uoffset_t i = 0; i < fb_attr->size(); i++)
	{
		auto fb_attr_this      = fb_attr->Get(i);
		auto binattr_info_this = binattr_info + i;

		BinAttr_to_SceNpMatching2BinAttr(fb_attr_this, binattr_info_this.get_ptr());
	}
}

void np_handler::MemberBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(const MemberBinAttrInternal* fb_attr, vm::ptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info)
{
	binattr_info->updateDate.tick = fb_attr->updateDate();
	BinAttr_to_SceNpMatching2BinAttr(fb_attr->data(), &binattr_info->data);
}

void np_handler::RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(const BinAttrInternal* fb_attr, vm::ptr<SceNpMatching2RoomBinAttrInternal> binattr_info)
{
	binattr_info->updateDate.tick = fb_attr->updateDate();
	binattr_info->updateMemberId = fb_attr->updateMemberId();
	BinAttr_to_SceNpMatching2BinAttr(fb_attr->data(), &binattr_info->data);
}

void np_handler::RoomGroups_to_SceNpMatching2RoomGroup(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_group, vm::ptr<SceNpMatching2RoomGroup> group_info)
{
	for (flatbuffers::uoffset_t i = 0; i < fb_group->size(); i++)
	{
		auto group                 = fb_group->Get(i);
		group_info[i].groupId      = group->groupId();
		group_info[i].withPassword = group->withPassword();
		group_info[i].withLabel    = group->withLabel();
		if (group->label())
		{
			for (flatbuffers::uoffset_t l_index = 0; l_index < group->label()->size(); l_index++)
			{
				group_info[i].label.data[l_index] = group->label()->Get(l_index);
			}
		}
		group_info[i].slotNum           = group->slotNum();
		group_info[i].curGroupMemberNum = group->curGroupMemberNum();
	}
}

void np_handler::UserInfo2_to_SceNpUserInfo2(const UserInfo2* user, SceNpUserInfo2* user_info)
{
	if (user->npId())
		std::memcpy(user_info->npId.handle.data, user->npId()->c_str(), std::min<usz>(16, user->npId()->size()));

	if (user->onlineName())
	{
		user_info->onlineName.set(allocate(sizeof(SceNpOnlineName)));
		std::memcpy(user_info->onlineName->data, user->onlineName()->c_str(), std::min<usz>(48, user->onlineName()->size()));
	}
	if (user->avatarUrl())
	{
		user_info->avatarUrl.set(allocate(sizeof(SceNpAvatarUrl)));
		std::memcpy(user_info->avatarUrl->data, user->avatarUrl()->c_str(), std::min<usz>(127, user->avatarUrl()->size()));
	}
}

void np_handler::SearchRoomReponse_to_SceNpMatching2SearchRoomResponse(const SearchRoomResponse* resp, SceNpMatching2SearchRoomResponse* search_resp)
{
	search_resp->range.size       = resp->size();
	search_resp->range.startIndex = resp->startIndex();
	search_resp->range.total      = resp->total();

	if (resp->rooms() && resp->rooms()->size() != 0)
	{
		vm::addr_t previous_next = vm::cast<u32>(0);
		for (flatbuffers::uoffset_t i = 0; i < resp->rooms()->size(); i++)
		{
			auto room = resp->rooms()->Get(i);
			vm::ptr<SceNpMatching2RoomDataExternal> room_info(allocate(sizeof(SceNpMatching2RoomDataExternal)));

			if (i > 0)
			{
				vm::ptr<SceNpMatching2RoomDataExternal> prev_room(previous_next);
				prev_room->next.set(room_info.addr());
			}
			else
			{
				search_resp->roomDataExternal = room_info;
			}

			previous_next = vm::cast(room_info.addr());

			room_info->serverId           = room->serverId();
			room_info->worldId            = room->worldId();
			room_info->publicSlotNum      = room->publicSlotNum();
			room_info->privateSlotNum     = room->privateSlotNum();
			room_info->lobbyId            = room->lobbyId();
			room_info->roomId             = room->roomId();
			room_info->openPublicSlotNum  = room->openPublicSlotNum();
			room_info->maxSlot            = room->maxSlot();
			room_info->openPrivateSlotNum = room->openPrivateSlotNum();
			room_info->curMemberNum       = room->curMemberNum();
			room_info->passwordSlotMask   = room->curMemberNum();
			if (auto owner = room->owner())
			{
				vm::ptr<SceNpUserInfo2> owner_info(allocate(sizeof(SceNpUserInfo2)));
				UserInfo2_to_SceNpUserInfo2(owner, owner_info.get_ptr());
				room_info->owner = owner_info;
			}

			if (room->roomGroup() && room->roomGroup()->size() != 0)
			{
				room_info->roomGroupNum = room->roomGroup()->size();
				vm::ptr<SceNpMatching2RoomGroup> group_info(allocate(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum));
				RoomGroups_to_SceNpMatching2RoomGroup(room->roomGroup(), group_info);
				room_info->roomGroup = group_info;
			}

			room_info->flagAttr = room->flagAttr();

			if (room->roomSearchableIntAttrExternal() && room->roomSearchableIntAttrExternal()->size() != 0)
			{
				room_info->roomSearchableIntAttrExternalNum = room->roomSearchableIntAttrExternal()->size();
				vm::ptr<SceNpMatching2IntAttr> intattr_info(allocate(sizeof(SceNpMatching2IntAttr) * room_info->roomSearchableIntAttrExternalNum));
				for (flatbuffers::uoffset_t a_index = 0; a_index < room->roomSearchableIntAttrExternal()->size(); a_index++)
				{
					auto int_attr             = room->roomSearchableIntAttrExternal()->Get(a_index);
					intattr_info[a_index].id  = int_attr->id();
					intattr_info[a_index].num = int_attr->num();
				}
				room_info->roomSearchableIntAttrExternal = intattr_info;
			}

			if (room->roomSearchableBinAttrExternal() && room->roomSearchableBinAttrExternal()->size() != 0)
			{
				room_info->roomSearchableBinAttrExternalNum = room->roomSearchableBinAttrExternal()->size();
				vm::ptr<SceNpMatching2BinAttr> binattr_info(allocate(sizeof(SceNpMatching2BinAttr) * room_info->roomSearchableBinAttrExternalNum));
				BinAttrs_to_SceNpMatching2BinAttr(room->roomSearchableBinAttrExternal(), binattr_info);
				room_info->roomSearchableBinAttrExternal = binattr_info;
			}

			if (room->roomBinAttrExternal() && room->roomBinAttrExternal()->size() != 0)
			{
				room_info->roomBinAttrExternalNum = room->roomBinAttrExternal()->size();
				vm::ptr<SceNpMatching2BinAttr> binattr_info(allocate(sizeof(SceNpMatching2BinAttr) * room_info->roomBinAttrExternalNum));
				BinAttrs_to_SceNpMatching2BinAttr(room->roomBinAttrExternal(), binattr_info);
				room_info->roomBinAttrExternal = binattr_info;
			}
		}
	}
	else
	{
		search_resp->roomDataExternal.set(0);
	}
}

u16 np_handler::RoomDataInternal_to_SceNpMatching2RoomDataInternal(const RoomDataInternal* resp, SceNpMatching2RoomDataInternal* room_info, const SceNpId& npid)
{
	u16 member_id                    = 0;
	room_info->serverId              = resp->serverId();
	room_info->worldId               = resp->worldId();
	room_info->lobbyId               = resp->lobbyId();
	room_info->roomId                = resp->roomId();
	room_info->passwordSlotMask      = resp->passwordSlotMask();
	room_info->maxSlot               = resp->maxSlot();
	room_info->memberList.membersNum = resp->memberList()->size();

	if (resp->roomGroup() && resp->roomGroup()->size() != 0)
	{
		room_info->roomGroupNum = resp->roomGroup()->size();
		vm::ptr<SceNpMatching2RoomGroup> group_info(allocate(sizeof(SceNpMatching2RoomGroup) * room_info->roomGroupNum));
		RoomGroups_to_SceNpMatching2RoomGroup(resp->roomGroup(), group_info);
		room_info->roomGroup = group_info;
	}

	vm::ptr<SceNpMatching2RoomMemberDataInternal> prev_member;
	for (flatbuffers::uoffset_t i = 0; i < resp->memberList()->size(); i++)
	{
		auto member = resp->memberList()->Get(i);
		vm::ptr<SceNpMatching2RoomMemberDataInternal> member_info(allocate(sizeof(SceNpMatching2RoomMemberDataInternal)));

		if (i > 0)
		{
			prev_member->next = member_info;
		}
		else
		{
			room_info->memberList.members    = member_info;
			room_info->memberList.membersNum = static_cast<u32>(resp->memberList()->size());
		}

		prev_member = member_info;

		RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(member, room_info, member_info.get_ptr());
	}

	vm::ptr<SceNpMatching2RoomMemberDataInternal> ptr = room_info->memberList.members;
	while (ptr)
	{
		if (strcmp(ptr->userInfo.npId.handle.data, npid.handle.data) == 0)
		{
			room_info->memberList.me = ptr;
			member_id                = ptr->memberId;
			break;
		}
		ptr = ptr->next;
	}

	ptr = room_info->memberList.members;
	while (ptr)
	{
		if (ptr->memberId == resp->ownerId())
		{
			room_info->memberList.owner = ptr;
			break;
		}
		ptr = ptr->next;
	}

	room_info->flagAttr = resp->flagAttr();

	if (resp->roomBinAttrInternal() && resp->roomBinAttrInternal()->size() != 0)
	{
		room_info->roomBinAttrInternalNum = resp->roomBinAttrInternal()->size();
		vm::ptr<SceNpMatching2RoomBinAttrInternal> binattrint_info(allocate(sizeof(SceNpMatching2RoomBinAttrInternal) * room_info->roomBinAttrInternalNum));

		for (u32 b_index = 0; b_index < room_info->roomBinAttrInternalNum; b_index++)
		{
			auto battr                               = resp->roomBinAttrInternal()->Get(b_index);
			binattrint_info[b_index].updateDate.tick = battr->updateDate();
			binattrint_info[b_index].updateMemberId  = battr->updateMemberId();

			binattrint_info[b_index].data.id   = battr->data()->id();
			binattrint_info[b_index].data.size = battr->data()->data()->size();
			binattrint_info[b_index].data.ptr  = allocate(binattrint_info[b_index].data.size);
			for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < binattrint_info[b_index].data.size; tmp_index++)
			{
				binattrint_info[b_index].data.ptr[tmp_index] = battr->data()->data()->Get(tmp_index);
			}
		}

		room_info->roomBinAttrInternal = binattrint_info;
	}

	return member_id;
}

void np_handler::RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(const RoomMemberDataInternal* member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data)
{
	UserInfo2_to_SceNpUserInfo2(member_data->userInfo(), &sce_member_data->userInfo);
	sce_member_data->joinDate.tick = member_data->joinDate();
	sce_member_data->memberId      = member_data->memberId();
	sce_member_data->teamId        = member_data->teamId();

	// Look for id
	if (member_data->roomGroup() != 0 && room_info)
	{
		bool found = false;
		for (u32 g_index = 0; g_index < room_info->roomGroupNum; g_index++)
		{
			if (room_info->roomGroup[g_index].groupId == member_data->roomGroup())
			{
				sce_member_data->roomGroup = vm::cast(room_info->roomGroup.addr() + (u32{sizeof(SceNpMatching2RoomGroup)} * g_index));
				found = true;
				break;
			}
		}
		ensure(found);
	}

	sce_member_data->natType  = member_data->natType();
	sce_member_data->flagAttr = member_data->flagAttr();

	if (member_data->roomMemberBinAttrInternal() && member_data->roomMemberBinAttrInternal()->size() != 0)
	{
		sce_member_data->roomMemberBinAttrInternalNum = member_data->roomMemberBinAttrInternal()->size();
		vm::ptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info(allocate(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * sce_member_data->roomMemberBinAttrInternalNum));
		for (u32 b_index = 0; b_index < sce_member_data->roomMemberBinAttrInternalNum; b_index++)
		{
			const auto battr                      = member_data->roomMemberBinAttrInternal()->Get(b_index);
			binattr_info[b_index].updateDate.tick = battr->updateDate();

			binattr_info[b_index].data.id   = battr->data()->id();
			binattr_info[b_index].data.size = battr->data()->data()->size();
			binattr_info[b_index].data.ptr  = allocate(binattr_info[b_index].data.size);
			for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < binattr_info[b_index].data.size; tmp_index++)
			{
				binattr_info[b_index].data.ptr[tmp_index] = battr->data()->data()->Get(tmp_index);
			}
		}
		sce_member_data->roomMemberBinAttrInternal = binattr_info;
	}
}

void np_handler::RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(const RoomMemberUpdateInfo* update_info, SceNpMatching2RoomMemberUpdateInfo* sce_update_info)
{
	sce_update_info->eventCause = 0;
	if (update_info->optData())
	{
		sce_update_info->optData.length = update_info->optData()->data()->size();
		for (usz i = 0; i < 16; i++)
		{
			sce_update_info->optData.data[i] = update_info->optData()->data()->Get(i);
		}
	}

	if (update_info->roomMemberDataInternal())
	{
		auto member = update_info->roomMemberDataInternal();

		vm::ptr<SceNpMatching2RoomMemberDataInternal> member_info(allocate(sizeof(SceNpMatching2RoomMemberDataInternal)));
		sce_update_info->roomMemberDataInternal = member_info;

		// TODO: Pass room_info
		RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(member, nullptr, member_info.get_ptr());
	}
}

void np_handler::RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const RoomUpdateInfo* update_info, SceNpMatching2RoomUpdateInfo* sce_update_info)
{
	sce_update_info->errorCode  = 0;
	sce_update_info->eventCause = 0;
	if (update_info->optData())
	{
		sce_update_info->optData.length = update_info->optData()->data()->size();
		for (usz i = 0; i < 16; i++)
		{
			sce_update_info->optData.data[i] = update_info->optData()->data()->Get(i);
		}
	}
}

void np_handler::RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(const RoomDataInternalUpdateInfo* update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid)
{
	vm::ptr<SceNpMatching2RoomDataInternal> room_data(allocate(sizeof(SceNpMatching2RoomDataInternal)));
	sce_update_info->newRoomDataInternal = room_data;
	RoomDataInternal_to_SceNpMatching2RoomDataInternal(update_info->newRoomDataInternal(), sce_update_info->newRoomDataInternal.get_ptr(), npid);

	if (update_info->newFlagAttr() == update_info->prevFlagAttr())
	{
		sce_update_info->newFlagAttr.set(0);
		sce_update_info->prevFlagAttr.set(0);
	}
	else
	{
		sce_update_info->newFlagAttr = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::flagAttr);

		vm::ptr<SceNpMatching2FlagAttr> prev_flag_attr(allocate(sizeof(SceNpMatching2FlagAttr)));
		*prev_flag_attr               = update_info->prevFlagAttr();
		sce_update_info->prevFlagAttr = prev_flag_attr;
	}

	if (update_info->newRoomPasswordSlotMask() == update_info->prevRoomPasswordSlotMask())
	{
		sce_update_info->newRoomPasswordSlotMask.set(0);
		sce_update_info->prevRoomPasswordSlotMask.set(0);
	}
	else
	{
		sce_update_info->newRoomPasswordSlotMask = sce_update_info->newRoomDataInternal.ptr(&SceNpMatching2RoomDataInternal::passwordSlotMask);

		vm::ptr<SceNpMatching2RoomPasswordSlotMask> prev_room_password_slot_mask(allocate(sizeof(SceNpMatching2RoomPasswordSlotMask)));
		*prev_room_password_slot_mask             = update_info->prevRoomPasswordSlotMask();
		sce_update_info->prevRoomPasswordSlotMask = prev_room_password_slot_mask;
	}

	if (update_info->newRoomGroup() && update_info->newRoomGroup()->size() != 0)
	{
		rpcn_log.todo("RoomDataInternalUpdateInfo::newRoomGroup");
		// TODO
		//sce_update_info->newRoomGroupNum = update_info->newRoomGroup()->size();
		//vm::ptr<SceNpMatching2RoomGroup> group_info(allocate(sizeof(SceNpMatching2RoomGroup) * sce_update_info->newRoomGroupNum));
		//RoomGroups_to_SceNpMatching2RoomGroup(update_info->newRoomGroup(), group_info);
		//sce_update_info->newRoomGroup = group_info;
	}

	if (update_info->newRoomBinAttrInternal() && update_info->newRoomBinAttrInternal()->size() != 0)
	{
		sce_update_info->newRoomBinAttrInternalNum = update_info->newRoomBinAttrInternal()->size();
		vm::bpptr<SceNpMatching2RoomBinAttrInternal> binattr_info_array(allocate(4 * sce_update_info->newRoomBinAttrInternalNum));
		for (uint i = 0; i < sce_update_info->newRoomBinAttrInternalNum; ++i)
		{
			vm::ptr<SceNpMatching2RoomBinAttrInternal> binattr_info = allocate(sizeof(SceNpMatching2RoomBinAttrInternal) * sce_update_info->newRoomBinAttrInternalNum);
			binattr_info_array[i]                               = binattr_info;
			RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(update_info->newRoomBinAttrInternal()->Get(i), binattr_info);
		}
		sce_update_info->newRoomBinAttrInternal = binattr_info_array;
	}
}

void np_handler::RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(const RoomMemberDataInternalUpdateInfo* update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info)
{
	vm::ptr<SceNpMatching2RoomMemberDataInternal> room_member_data(allocate(sizeof(SceNpMatching2RoomMemberDataInternal)));
	sce_update_info->newRoomMemberDataInternal = room_member_data;
	RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(update_info->newRoomMemberDataInternal(), nullptr, sce_update_info->newRoomMemberDataInternal.get_ptr());

	if (update_info->newFlagAttr() == update_info->prevFlagAttr())
	{
		sce_update_info->newFlagAttr.set(0);
		sce_update_info->prevFlagAttr.set(0);
	}
	else
	{
		sce_update_info->newFlagAttr = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::flagAttr);

		vm::ptr<SceNpMatching2FlagAttr> prev_flag_attr(allocate(sizeof(SceNpMatching2FlagAttr)));
		*prev_flag_attr               = update_info->prevFlagAttr();
		sce_update_info->prevFlagAttr = prev_flag_attr;
	}

	sce_update_info->newTeamId = sce_update_info->newRoomMemberDataInternal.ptr(&SceNpMatching2RoomMemberDataInternal::teamId);

	if (update_info->newRoomMemberBinAttrInternal() && update_info->newRoomMemberBinAttrInternal()->size() != 0)
	{
		sce_update_info->newRoomMemberBinAttrInternalNum = update_info->newRoomMemberBinAttrInternal()->size();
		vm::bpptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info_array(allocate(4 * sce_update_info->newRoomMemberBinAttrInternalNum));
		for (uint i = 0; i < sce_update_info->newRoomMemberBinAttrInternalNum; ++i)
		{
			vm::ptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info = allocate(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * sce_update_info->newRoomMemberBinAttrInternalNum);
			binattr_info_array[i] = binattr_info;
			MemberBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(update_info->newRoomMemberBinAttrInternal()->Get(i), binattr_info);
		}
		sce_update_info->newRoomMemberBinAttrInternal = binattr_info_array;
	}
}

void np_handler::GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const GetPingInfoResponse* resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp)
{
	sce_resp->serverId = resp->serverId();
	sce_resp->worldId  = resp->worldId();
	sce_resp->roomId   = resp->roomId();
	sce_resp->rtt      = resp->rtt();
}

void np_handler::RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(const RoomMessageInfo* mi, SceNpMatching2RoomMessageInfo* sce_mi)
{
	sce_mi->filtered = mi->filtered();
	sce_mi->castType = mi->castType();

	if (sce_mi->castType != SCE_NP_MATCHING2_CASTTYPE_BROADCAST)
	{
		vm::ptr<SceNpMatching2RoomMessageDestination> dst_info(allocate(sizeof(SceNpMatching2RoomMessageDestination)));
		sce_mi->dst = dst_info;
	}

	switch (sce_mi->castType)
	{
	case SCE_NP_MATCHING2_CASTTYPE_BROADCAST: break;
	case SCE_NP_MATCHING2_CASTTYPE_UNICAST: sce_mi->dst->unicastTarget = mi->dst()->Get(0); break;
	case SCE_NP_MATCHING2_CASTTYPE_MULTICAST:
	{
		sce_mi->dst->multicastTarget.memberIdNum = mi->dst()->size();
		vm::ptr<be_t<u16>> member_list(allocate(sizeof(u16) * mi->dst()->size()));
		sce_mi->dst->multicastTarget.memberId = member_list;
		for (u32 i = 0; i < mi->dst()->size(); i++)
		{
			sce_mi->dst->multicastTarget.memberId[i] = mi->dst()->Get(i);
		}
		break;
	}
	case SCE_NP_MATCHING2_CASTTYPE_MULTICAST_TEAM: sce_mi->dst->multicastTargetTeamId = mi->dst()->Get(0); break;
	default: ensure(false);
	}

	if (auto src_member = mi->srcMember())
	{
		vm::ptr<SceNpUserInfo2> src_info(allocate(sizeof(SceNpUserInfo2)));
		UserInfo2_to_SceNpUserInfo2(src_member, src_info.get_ptr());
		sce_mi->srcMember = src_info;
	}

	if (auto msg = mi->msg())
	{
		sce_mi->msgLen = msg->size();
		vm::ptr<u8> msg_data(allocate(msg->size()));
		for (u32 i = 0; i < msg->size(); i++)
		{
			msg_data[i] = msg->Get(i);
		}
		sce_mi->msg = msg_data;
	}
}
