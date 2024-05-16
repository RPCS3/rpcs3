#pragma once

#include <map>
#include <optional>

#include "Utilities/mutex.h"

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

namespace np
{
	struct userinfo_cache
	{
		SceNpId npId{};
		std::optional<SceNpOnlineName> onlineName;
		std::optional<SceNpAvatarUrl> avatarUrl;
	};

	struct memberbin_cache
	{
		memberbin_cache(const SceNpMatching2RoomMemberBinAttrInternal* sce_memberbin);

		SceNpMatching2AttributeId id;
		CellRtcTick updateDate;
		std::vector<u8> data;
	};

	struct member_cache
	{
		member_cache(const SceNpMatching2RoomMemberDataInternal* sce_member);

		userinfo_cache userInfo;
		CellRtcTick joinDate;
		SceNpMatching2RoomMemberId memberId;
		SceNpMatching2TeamId teamId;
		SceNpMatching2RoomGroupId group_id;
		SceNpMatching2NatType natType;
		SceNpMatching2FlagAttr flagAttr;

		std::map<SceNpMatching2AttributeId, memberbin_cache> bins;
	};

	struct password_info_cache
	{
		bool with_password = false;
		SceNpMatching2SessionPassword password{};
	};

	struct room_cache
	{
		void update(const SceNpMatching2RoomDataInternal* sce_roomdata);

		u32 num_slots                                    = 0;
		SceNpMatching2RoomPasswordSlotMask mask_password = 0;
		std::optional<SceNpMatching2SessionPassword> password;

		std::map<SceNpMatching2RoomGroupId, SceNpMatching2RoomGroup> groups;
		std::map<SceNpMatching2RoomMemberId, member_cache> members;

		bool owner = false;
	};

	class cache_manager
	{
	public:
		cache_manager() = default;

		void insert_room(const SceNpMatching2RoomDataInternal* sce_roomdata);
		bool add_member(SceNpMatching2RoomId room_id, const SceNpMatching2RoomMemberDataInternal* sce_roommemberdata);
		bool del_member(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id);
		void update_password(SceNpMatching2RoomId room_id, const std::optional<SceNpMatching2SessionPassword>& password);

		std::pair<error_code, std::optional<SceNpMatching2RoomSlotInfo>> get_slots(SceNpMatching2RoomId room_id);
		std::pair<error_code, std::vector<SceNpMatching2RoomMemberId>> get_memberids(u64 room_id, s32 sort_method);
		std::pair<error_code, std::optional<SceNpMatching2SessionPassword>> get_password(SceNpMatching2RoomId room_id);
		error_code get_member_and_attrs(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data, bool include_onlinename, bool include_avatarurl);
		std::pair<error_code, std::optional<SceNpId>> get_npid(u64 room_id, u16 member_id);
		std::optional<u16> get_memberid(u64 room_id, const SceNpId& npid);

	private:
		shared_mutex mutex;
		std::map<SceNpMatching2RoomId, room_cache> rooms;
	};
} // namespace np
