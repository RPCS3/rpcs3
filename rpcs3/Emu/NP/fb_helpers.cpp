#include "stdafx.h"
#include "np_handler.h"

LOG_CHANNEL(rpcn_log, "rpcn");

void np_handler::BinAttr_to_SceNpMatching2BinAttr(const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, vm::ptr<SceNpMatching2BinAttr> binattr_info)
{
	for (flatbuffers::uoffset_t i = 0; i < fb_attr->size(); i++)
	{
		auto bin_attr        = fb_attr->Get(i);
		binattr_info[i].id   = bin_attr->id();
		binattr_info[i].size = bin_attr->data()->size();
		binattr_info[i].ptr  = allocate(binattr_info[i].size);
		for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < bin_attr->data()->size(); tmp_index++)
		{
			binattr_info[i].ptr[tmp_index] = bin_attr->data()->Get(tmp_index);
		}
	}
}

void np_handler::RoomGroup_to_SceNpMatching2RoomGroup(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_group, vm::ptr<SceNpMatching2RoomGroup> group_info)
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
		std::memcpy(user_info->npId.handle.data, user->npId()->c_str(), std::min<usz>(9, user->npId()->size()));

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
				RoomGroup_to_SceNpMatching2RoomGroup(room->roomGroup(), group_info);
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
				BinAttr_to_SceNpMatching2BinAttr(room->roomSearchableBinAttrExternal(), binattr_info);
				room_info->roomSearchableBinAttrExternal = binattr_info;
			}

			if (room->roomBinAttrExternal() && room->roomBinAttrExternal()->size() != 0)
			{
				room_info->roomBinAttrExternalNum = room->roomBinAttrExternal()->size();
				vm::ptr<SceNpMatching2BinAttr> binattr_info(allocate(sizeof(SceNpMatching2BinAttr) * room_info->roomBinAttrExternalNum));
				BinAttr_to_SceNpMatching2BinAttr(room->roomBinAttrExternal(), binattr_info);
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
		RoomGroup_to_SceNpMatching2RoomGroup(resp->roomGroup(), group_info);
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

		UserInfo2_to_SceNpUserInfo2(member->userInfo(), &member_info->userInfo);
		member_info->joinDate.tick = member->joinDate();
		member_info->memberId      = member->memberId();
		member_info->teamId        = member->teamId();

		// Look for id
		if (member->roomGroup() != 0)
		{
			bool found = false;
			for (u32 g_index = 0; g_index < room_info->roomGroupNum; g_index++)
			{
				if (room_info->roomGroup[g_index].groupId == member->roomGroup())
				{
					member_info->roomGroup = vm::cast(room_info->roomGroup.addr() + (sizeof(SceNpMatching2RoomGroup) * g_index));
					found                  = true;
				}
			}
			ensure(found);
		}

		member_info->natType  = member->natType();
		member_info->flagAttr = member->flagAttr();

		if (member->roomMemberBinAttrInternal() && member->roomMemberBinAttrInternal()->size() != 0)
		{
			member_info->roomMemberBinAttrInternalNum = member->roomMemberBinAttrInternal()->size();
			vm::ptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info(allocate(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * member_info->roomMemberBinAttrInternalNum));
			for (u32 b_index = 0; b_index < member_info->roomMemberBinAttrInternalNum; b_index++)
			{
				const auto battr                      = member->roomMemberBinAttrInternal()->Get(b_index);
				binattr_info[b_index].updateDate.tick = battr->updateDate();

				binattr_info[b_index].data.id   = battr->data()->id();
				binattr_info[b_index].data.size = battr->data()->data()->size();
				binattr_info[b_index].data.ptr  = allocate(binattr_info[b_index].data.size);
				for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < binattr_info[b_index].data.size; tmp_index++)
				{
					binattr_info[b_index].data.ptr[tmp_index] = battr->data()->data()->Get(tmp_index);
				}
			}
			member_info->roomMemberBinAttrInternal = binattr_info;
		}
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

		UserInfo2_to_SceNpUserInfo2(member->userInfo(), &member_info->userInfo);
		member_info->joinDate.tick = member->joinDate();
		member_info->memberId      = member->memberId();
		member_info->teamId        = member->teamId();

		// Look for id
		// TODO

		member_info->natType  = member->natType();
		member_info->flagAttr = member->flagAttr();

		if (member->roomMemberBinAttrInternal() && member->roomMemberBinAttrInternal()->size() != 0)
		{
			member_info->roomMemberBinAttrInternalNum = member->roomMemberBinAttrInternal()->size();
			vm::ptr<SceNpMatching2RoomMemberBinAttrInternal> binattr_info(allocate(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * member_info->roomMemberBinAttrInternalNum));
			for (u32 b_index = 0; b_index < member_info->roomMemberBinAttrInternalNum; b_index++)
			{
				const auto battr                      = member->roomMemberBinAttrInternal()->Get(b_index);
				binattr_info[b_index].updateDate.tick = battr->updateDate();

				binattr_info[b_index].data.id   = battr->data()->id();
				binattr_info[b_index].data.size = battr->data()->data()->size();
				binattr_info[b_index].data.ptr  = allocate(binattr_info[b_index].data.size);
				for (flatbuffers::uoffset_t tmp_index = 0; tmp_index < binattr_info[b_index].data.size; tmp_index++)
				{
					binattr_info[b_index].data.ptr[tmp_index] = battr->data()->data()->Get(tmp_index);
				}
			}
			member_info->roomMemberBinAttrInternal = binattr_info;
		}
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
