#pragma once

#include <map>
#include "Utilities/mutex.h"
#include "Emu/Cell/Modules/sceNp.h"

template <>
struct std::less<SceNpRoomId>
{
	bool operator()(const SceNpRoomId& a, const SceNpRoomId& b) const
	{
		return (std::memcmp(a.opt, b.opt, sizeof(a.opt)) < 0);
	}
};

template <>
struct std::less<SceNpId>
{
	bool operator()(const SceNpId& a, const SceNpId& b) const
	{
		return (std::memcmp(a.handle.data, b.handle.data, sizeof(a.handle.data)) < 0);
	}
};

namespace np
{
	struct gui_room_member
	{
		SceNpUserInfo info;
		bool owner;
	};

	struct gui_room_cache
	{
		std::map<SceNpId, gui_room_member> members;
	};

	class gui_cache_manager
	{
	public:
		gui_cache_manager() = default;

		void add_room(const SceNpRoomId& room_id);
		void del_room(const SceNpRoomId& room_id);
		void add_member(const SceNpRoomId& room_id, const SceNpMatchingRoomMember* user_info, bool new_member);
		void del_member(const SceNpRoomId& room_id, const SceNpMatchingRoomMember* user_info);

		error_code get_room_member_list(const SceNpRoomId& room_id, u32 buf_len, vm::ptr<void> data);

	private:
		shared_mutex mutex;
		std::map<SceNpRoomId, gui_room_cache> rooms;
	};
} // namespace np
