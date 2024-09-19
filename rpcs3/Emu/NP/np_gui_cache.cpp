#include "stdafx.h"
#include "util/asm.hpp"
#include "np_gui_cache.h"

LOG_CHANNEL(np_gui_cache);

namespace np
{
	void gui_cache_manager::add_room(const SceNpRoomId& room_id)
	{
		std::lock_guard lock(mutex);
		const auto [room, inserted] = rooms.insert_or_assign(room_id, gui_room_cache{});

		if (!inserted)
			np_gui_cache.error("Cache mismatch: tried to insert room but it already existed");
	}

	void gui_cache_manager::del_room(const SceNpRoomId& room_id)
	{
		std::lock_guard lock(mutex);

		if (rooms.erase(room_id) != 1)
			np_gui_cache.error("Cache mismatch: tried to delete a room that wasn't in the cache");
	}

	void gui_cache_manager::add_member(const SceNpRoomId& room_id, const SceNpMatchingRoomMember* user_info, bool new_member)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_gui_cache.error("Cache mismatch: tried to add a member to a non-existing room");
			return;
		}

		auto& room = ::at32(rooms, room_id);
		const auto [_, inserted] = room.members.insert_or_assign(user_info->user_info.userId, gui_room_member{.info = user_info->user_info, .owner = !!user_info->owner});

		if (new_member)
		{
			if (!inserted)
				np_gui_cache.error("Cache mismatch: tried to add a member but it was already in the room");
		}
		else
		{
			if (inserted)
				np_gui_cache.error("Cache mismatch: tried to update a member but it wasn't already in the room");
		}
	}

	void gui_cache_manager::del_member(const SceNpRoomId& room_id, const SceNpMatchingRoomMember* user_info)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
		{
			np_gui_cache.error("Cache mismatch: tried to remove a member from a non-existing room");
			return;
		}

		auto& room = ::at32(rooms, room_id);
		if (room.members.erase(user_info->user_info.userId) != 1)
			np_gui_cache.error("Cache mismatch: tried to remove a member but it wasn't in the room");
	}

	error_code gui_cache_manager::get_room_member_list(const SceNpRoomId& room_id, u32 buf_len, vm::ptr<void> data)
	{
		std::lock_guard lock(mutex);

		if (!rooms.contains(room_id))
			return SCE_NP_MATCHING_ERROR_ROOM_NOT_FOUND;

		const auto& room = ::at32(rooms, room_id);

		const u32 room_size = ::narrow<u32>(utils::align(sizeof(SceNpMatchingRoomStatus), 8) + (utils::align(sizeof(SceNpMatchingRoomMember), 8) * room.members.size()));

		if (!data)
			return not_an_error(room_size);

		if (buf_len < room_size)
			return SCE_NP_MATCHING_ERROR_INSUFFICIENT_BUFFER;

		std::memset(data.get_ptr(), 0, buf_len);

		vm::ptr<SceNpMatchingRoomStatus> room_status = vm::cast(data.addr());
		std::memcpy(&room_status->id, &room_id, sizeof(SceNpRoomId));
		room_status->num = ::narrow<s32>(room.members.size());

		if (!room.members.empty())
		{
			vm::ptr<SceNpMatchingRoomMember> cur_member_ptr{};

			for (const auto& [_, member] : room.members)
			{
				if (!cur_member_ptr)
				{
					room_status->members = vm::cast(data.addr() + utils::align(sizeof(SceNpMatchingRoomStatus), 8));
					cur_member_ptr = room_status->members;
				}
				else
				{
					cur_member_ptr->next = vm::cast(cur_member_ptr.addr() + utils::align(sizeof(SceNpMatchingRoomMember), 8));
					cur_member_ptr = cur_member_ptr->next;
				}

				cur_member_ptr->owner = member.owner ? 1 : 0;
				std::memcpy(&cur_member_ptr->user_info, &member.info, sizeof(SceNpUserInfo));
			}
		}

		return CELL_OK;
	}
} // namespace np
