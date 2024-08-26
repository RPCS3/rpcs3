#include "stdafx.h"
#include <bit>

#include "Emu/NP/np_allocator.h"
#include "Emu/NP/np_cache.h"
#include "Emu/NP/np_helpers.h"

LOG_CHANNEL(np_cache);

namespace np
{
	memberbin_cache::memberbin_cache(const SceNpMatching2RoomMemberBinAttrInternal* sce_memberbin)
	{
		ensure(sce_memberbin && (sce_memberbin->data.ptr || !sce_memberbin->data.size));

		id = sce_memberbin->data.id;
		updateDate.tick = sce_memberbin->updateDate.tick;
		data = std::vector<u8>(sce_memberbin->data.ptr.get_ptr(), sce_memberbin->data.ptr.get_ptr() + sce_memberbin->data.size);
	}

	member_cache::member_cache(const SceNpMatching2RoomMemberDataInternal* sce_member)
	{
		userInfo.npId = sce_member->userInfo.npId;
		if (sce_member->userInfo.onlineName)
		{
			userInfo.onlineName = *sce_member->userInfo.onlineName;
		}
		if (sce_member->userInfo.avatarUrl)
		{
			userInfo.avatarUrl = *sce_member->userInfo.avatarUrl;
		}

		joinDate.tick = sce_member->joinDate.tick;
		memberId      = sce_member->memberId;
		teamId        = sce_member->teamId;
		if (sce_member->roomGroup)
		{
			group_id = sce_member->roomGroup->groupId;
		}
		else
		{
			group_id = 0;
		}

		natType  = sce_member->natType;
		flagAttr = sce_member->flagAttr;

		for (u32 i = 0; i < sce_member->roomMemberBinAttrInternalNum; i++)
		{
			bins.insert_or_assign(sce_member->roomMemberBinAttrInternal[i].data.id, memberbin_cache(&sce_member->roomMemberBinAttrInternal[i]));
		}
	}

	void room_cache::update(const SceNpMatching2RoomDataInternal* sce_roomdata)
	{
		num_slots     = sce_roomdata->maxSlot;
		mask_password = sce_roomdata->passwordSlotMask;

		groups.clear();
		for (u32 i = 0; i < sce_roomdata->roomGroupNum && sce_roomdata->roomGroup; i++)
		{
			const SceNpMatching2RoomGroup* sce_group = &sce_roomdata->roomGroup[i];
			memcpy(&groups[sce_group->groupId], sce_group, sizeof(SceNpMatching2RoomGroup));
		}

		members.clear();
		vm::bptr<SceNpMatching2RoomMemberDataInternal> sce_member = sce_roomdata->memberList.members;
		while (sce_member)
		{
			members.insert_or_assign(sce_member->memberId, member_cache(sce_member.get_ptr()));
			sce_member = sce_member->next;
		}

		if (sce_roomdata->memberList.me == sce_roomdata->memberList.owner)
		{
			owner = true;
		}
	}

	void cache_manager::insert_room(const SceNpMatching2RoomDataInternal* sce_roomdata)
	{
		std::lock_guard lock(mutex);

		rooms[sce_roomdata->roomId].update(sce_roomdata);
	}

	bool cache_manager::add_member(SceNpMatching2RoomId room_id, const SceNpMatching2RoomMemberDataInternal* sce_roommemberdata)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_cache.error("np_cache::add_member cache miss: room_id(%d)", room_id);
			return false;
		}

		rooms[room_id].members.insert_or_assign(sce_roommemberdata->memberId, member_cache(sce_roommemberdata));
		return true;
	}

	bool cache_manager::del_member(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_cache.error("np_cache::del_member cache miss: room_id(%d)/member_id(%d)", room_id, member_id);
			return false;
		}

		rooms.erase(member_id);
		return true;
	}

	void cache_manager::update_password(SceNpMatching2RoomId room_id, const std::optional<SceNpMatching2SessionPassword>& password)
	{
		std::lock_guard lock(mutex);

		rooms[room_id].password = password;
	}

	std::pair<error_code, std::optional<SceNpMatching2RoomSlotInfo>> cache_manager::get_slots(SceNpMatching2RoomId room_id)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			return {SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND, {}};
		}

		const auto& room = rooms[room_id];

		SceNpMatching2RoomSlotInfo slots{};

		slots.roomId = room_id;

		SceNpMatching2RoomJoinedSlotMask join_mask = 0;
		for (const auto& member : room.members)
		{
			join_mask |= (1ull << ((member.first >> 4) - 1));
		}
		slots.joinedSlotMask   = join_mask;
		slots.passwordSlotMask = room.mask_password;

		u64 joinable_slot_mask = (static_cast<u64>(1) << room.num_slots) - 1;
		u16 num_private_slots  = std::popcount(room.mask_password & joinable_slot_mask);
		u16 num_public_slots   = room.num_slots - num_private_slots;

		slots.publicSlotNum  = num_public_slots;
		slots.privateSlotNum = num_private_slots;

		u16 open_private_slots = num_private_slots - std::popcount(join_mask & room.mask_password);
		u16 open_public_slots  = num_public_slots - std::popcount(join_mask & (~room.mask_password));

		slots.openPublicSlotNum  = open_public_slots;
		slots.openPrivateSlotNum = open_private_slots;

		return {CELL_OK, slots};
	}

	std::pair<error_code, std::vector<SceNpMatching2RoomMemberId>> cache_manager::get_memberids(u64 room_id, s32 sort_method)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			return {SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND, {}};
		}

		const auto& room = rooms[room_id];

		std::vector<SceNpMatching2RoomMemberId> vec_memberids;

		switch (sort_method)
		{
		case SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER:
		{
			for (const auto& member : room.members)
			{
				vec_memberids.push_back(member.first);
			}
			break;
		}
		case SCE_NP_MATCHING2_SORT_METHOD_JOIN_DATE:
		{
			std::map<u64, u16> map_joindate_id;

			for (const auto& member : room.members)
			{
				map_joindate_id.insert(std::make_pair(member.second.joinDate.tick, member.first));
			}

			for (const auto& member : map_joindate_id)
			{
				vec_memberids.push_back(member.second);
			}

			break;
		}
		default: fmt::throw_exception("Unreachable");
		}

		return {CELL_OK, vec_memberids};
	}

	std::pair<error_code, std::optional<SceNpMatching2SessionPassword>> cache_manager::get_password(SceNpMatching2RoomId room_id)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			return {SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND, {}};
		}

		if (!rooms[room_id].owner)
		{
			return {SCE_NP_MATCHING2_ERROR_NOT_ALLOWED, {}};
		}

		return {CELL_OK, rooms[room_id].password};
	}

	error_code cache_manager::get_member_and_attrs(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data, bool include_onlinename, bool include_avatarurl)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			return SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND;
		}

		if (!rooms[room_id].members.contains(member_id))
		{
			return SCE_NP_MATCHING2_ERROR_ROOM_MEMBER_NOT_FOUND;
		}

		const auto& room = ::at32(rooms, room_id);
		const auto& member = ::at32(room.members, member_id);

		if (ptr_member)
		{
			memset(ptr_member, 0, sizeof(SceNpMatching2RoomMemberDataInternal));
			memcpy(&ptr_member->userInfo.npId, &member.userInfo.npId, sizeof(SceNpId));
			ptr_member->joinDate.tick = member.joinDate.tick;
			ptr_member->memberId      = member.memberId;
			ptr_member->teamId        = member.teamId;
			ptr_member->natType       = member.natType;
			ptr_member->flagAttr      = member.flagAttr;
		}

		u32 needed_data_size = 0;

		if (include_onlinename && member.userInfo.onlineName)
			needed_data_size += sizeof(SceNpOnlineName);

		if (include_avatarurl && member.userInfo.avatarUrl)
			needed_data_size += sizeof(SceNpAvatarUrl);

		if (member.group_id)
			needed_data_size += sizeof(SceNpMatching2RoomGroup);

		for (usz i = 0; i < binattrs_list.size(); i++)
		{
			if (member.bins.contains(binattrs_list[i]))
			{
				needed_data_size += ::narrow<u32>(sizeof(SceNpMatching2RoomMemberBinAttrInternal) + ::at32(member.bins, binattrs_list[i]).data.size());
			}
		}

		if (!addr_data || !ptr_member)
		{
			return not_an_error(needed_data_size);
		}

		if (size_data < needed_data_size)
		{
			return SCE_NP_MATCHING2_ERROR_INSUFFICIENT_BUFFER;
		}

		memory_allocator mem;
		mem.setup(vm::ptr<void>(vm::cast(addr_data)), size_data);

		if (include_onlinename && member.userInfo.onlineName)
		{
			ptr_member->userInfo.onlineName.set(mem.allocate(sizeof(SceNpOnlineName)));
			memcpy(ptr_member->userInfo.onlineName.get_ptr(), &member.userInfo.onlineName.value(), sizeof(SceNpOnlineName));
		}

		if (include_avatarurl && member.userInfo.avatarUrl)
		{
			ptr_member->userInfo.avatarUrl.set(mem.allocate(sizeof(SceNpAvatarUrl)));
			memcpy(ptr_member->userInfo.avatarUrl.get_ptr(), &member.userInfo.avatarUrl.value(), sizeof(SceNpAvatarUrl));
		}

		if (member.group_id)
		{
			ptr_member->roomGroup.set(mem.allocate(sizeof(SceNpMatching2RoomGroup)));
			memcpy(ptr_member->roomGroup.get_ptr(), &::at32(room.groups, member.group_id), sizeof(SceNpMatching2RoomGroup));
		}

		u32 num_binattrs = 0;
		for (u32 i = 0; i < binattrs_list.size(); i++)
		{
			if (member.bins.contains(binattrs_list[i]))
			{
				num_binattrs++;
			}
		}

		if (num_binattrs)
		{
			ptr_member->roomMemberBinAttrInternal.set(mem.allocate(sizeof(SceNpMatching2RoomMemberBinAttrInternal) * num_binattrs));
			ptr_member->roomMemberBinAttrInternalNum         = num_binattrs;
			SceNpMatching2RoomMemberBinAttrInternal* bin_ptr = ptr_member->roomMemberBinAttrInternal.get_ptr();

			u32 actual_cnt = 0;
			for (u32 i = 0; i < binattrs_list.size(); i++)
			{
				if (member.bins.contains(binattrs_list[i]))
				{
					const auto& bin = ::at32(member.bins, binattrs_list[i]);
					bin_ptr[actual_cnt].updateDate.tick = bin.updateDate.tick;
					bin_ptr[actual_cnt].data.id         = bin.id;
					bin_ptr[actual_cnt].data.size       = ::size32(bin.data);
					bin_ptr[actual_cnt].data.ptr.set(mem.allocate(::size32(bin.data)));
					std::memcpy(bin_ptr[actual_cnt].data.ptr.get_ptr(), bin.data.data(), bin.data.size());
					actual_cnt++;
				}
			}
		}

		return not_an_error(needed_data_size);
	}

	std::pair<error_code, std::optional<SceNpId>> cache_manager::get_npid(u64 room_id, u16 member_id)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_cache.error("np_cache::get_npid cache miss room_id: room_id(%d)/member_id(%d)", room_id, member_id);
			return {SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID, std::nullopt};
		}

		if (!::at32(rooms, room_id).members.contains(member_id))
		{
			np_cache.error("np_cache::get_npid cache miss member_id: room_id(%d)/member_id(%d)", room_id, member_id);
			return {SCE_NP_MATCHING2_ERROR_INVALID_MEMBER_ID, std::nullopt};
		}

		return {CELL_OK, ::at32(::at32(rooms, room_id).members, member_id).userInfo.npId};
	}

	std::optional<u16> cache_manager::get_memberid(u64 room_id, const SceNpId& npid)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_cache.error("np_cache::get_memberid cache miss room_id: room_id(%d)/npid(%s)", room_id, static_cast<const char*>(npid.handle.data));
			return std::nullopt;
		}

		const auto& members = ::at32(rooms, room_id).members;

		for (const auto& [id, member_cache] : members)
		{
			if (np::is_same_npid(member_cache.userInfo.npId, npid))
				return id;
		}

		np_cache.error("np_cache::get_memberid cache miss member_id: room_id(%d)/npid(%s)", room_id, static_cast<const char*>(npid.handle.data));
		return std::nullopt;
	}

} // namespace np
