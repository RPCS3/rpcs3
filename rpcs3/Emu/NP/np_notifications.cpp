#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/IdManager.h"
#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/np_structs_extra.h"

LOG_CHANNEL(rpcn_log, "rpcn");

namespace np
{
	void np_handler::notif_user_joined_room(std::vector<u8>& data)
	{
		vec_stream noti(data);
		u64 room_id          = noti.get<u64>();
		auto update_info_raw = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty UserJoinedRoom notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* update_info = flatbuffers::GetRoot<RoomMemberUpdateInfo>(update_info_raw.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMemberUpdateInfo, sizeof(SceNpMatching2RoomMemberUpdateInfo));
		auto* notif_data  = reinterpret_cast<SceNpMatching2RoomMemberUpdateInfo*>(edata.data());
		RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(edata, update_info, notif_data);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.add_member(room_id, notif_data->roomMemberDataInternal.get_ptr());

		rpcn_log.notice("Received notification that user %s(%d) joined the room(%d)", notif_data->roomMemberDataInternal->userInfo.npId.handle.data, notif_data->roomMemberDataInternal->memberId, room_id);
		extra_nps::print_room_member_data_internal(notif_data->roomMemberDataInternal.get_ptr());

		sysutil_register_cb([room_event_cb = this->room_event_cb, room_id, event_key, room_event_cb_ctx = this->room_event_cb_ctx, room_event_cb_arg = this->room_event_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
			{
				room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined, event_key, 0, size, room_event_cb_arg);
				return 0;
			});
	}

	void np_handler::notif_user_left_room(std::vector<u8>& data)
	{
		vec_stream noti(data);
		u64 room_id          = noti.get<u64>();
		auto update_info_raw = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty UserLeftRoom notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* update_info = flatbuffers::GetRoot<RoomMemberUpdateInfo>(update_info_raw.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMemberUpdateInfo, sizeof(SceNpMatching2RoomMemberUpdateInfo));
		auto* notif_data  = reinterpret_cast<SceNpMatching2RoomMemberUpdateInfo*>(edata.data());
		RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(edata, update_info, notif_data);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.del_member(room_id, notif_data->roomMemberDataInternal->memberId);

		rpcn_log.notice("Received notification that user %s(%d) left the room(%d)", notif_data->roomMemberDataInternal->userInfo.npId.handle.data, notif_data->roomMemberDataInternal->memberId, room_id);
		extra_nps::print_room_member_data_internal(notif_data->roomMemberDataInternal.get_ptr());

		sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
			{
				room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft, event_key, 0, size, room_event_cb_arg);
				return 0;
			});
	}

	void np_handler::notif_room_destroyed(std::vector<u8>& data)
	{
		vec_stream noti(data);
		u64 room_id          = noti.get<u64>();
		auto update_info_raw = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty RoomDestroyed notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* update_info = flatbuffers::GetRoot<RoomUpdateInfo>(update_info_raw.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomUpdateInfo, sizeof(SceNpMatching2RoomUpdateInfo));
		auto* notif_data  = reinterpret_cast<SceNpMatching2RoomUpdateInfo*>(edata.data());
		RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(update_info, notif_data);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		rpcn_log.notice("Received notification that room(%d) was destroyed", room_id);

		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.disconnect_sig2_users(room_id);

		sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
			{
				room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed, event_key, 0, size, room_event_cb_arg);
				return 0;
			});
	}

	void np_handler::notif_updated_room_data_internal(std::vector<u8>& data)
	{
		vec_stream noti(data);
		SceNpMatching2RoomId room_id = noti.get<u64>();
		auto update_info_raw         = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty UpdatedRoomDataInternal notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* update_info = flatbuffers::GetRoot<RoomDataInternalUpdateInfo>(update_info_raw.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomDataInternalUpdateInfo, sizeof(SceNpMatching2RoomDataInternalUpdateInfo));
		auto* notif_data  = reinterpret_cast<SceNpMatching2RoomDataInternalUpdateInfo*>(edata.data());
		RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(edata, update_info, notif_data, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(notif_data->newRoomDataInternal.get_ptr());

		extra_nps::print_room_data_internal(notif_data->newRoomDataInternal.get_ptr());

		rpcn_log.notice("Received notification that room(%d)'s data was updated", room_id);

		sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
			{
				room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal, event_key, 0, size, room_event_cb_arg);
				return 0;
			});
	}

	void np_handler::notif_updated_room_member_data_internal(std::vector<u8>& data)
	{
		vec_stream noti(data);
		SceNpMatching2RoomId room_id = noti.get<u64>();
		auto update_info_raw         = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty UpdatedRoomMemberDataInternal notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* update_info = flatbuffers::GetRoot<RoomMemberDataInternalUpdateInfo>(update_info_raw.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMemberDataInternalUpdateInfo, sizeof(SceNpMatching2RoomMemberDataInternalUpdateInfo));
		auto* notif_data  = reinterpret_cast<SceNpMatching2RoomMemberDataInternalUpdateInfo*>(edata.data());
		RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(edata, update_info, notif_data);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.add_member(room_id, notif_data->newRoomMemberDataInternal.get_ptr());

		rpcn_log.notice("Received notification that user's %s(%d) room (%d) data was updated", notif_data->newRoomMemberDataInternal->userInfo.npId.handle.data, notif_data->newRoomMemberDataInternal->memberId, room_id);
		extra_nps::print_room_member_data_internal(notif_data->newRoomMemberDataInternal.get_ptr());

		sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
			{
				room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal, event_key, 0, size, room_event_cb_arg);
				return 0;
			});
	}

	void np_handler::notif_room_message_received(std::vector<u8>& data)
	{
		vec_stream noti(data);
		u64 room_id           = noti.get<u64>();
		u16 member_id         = noti.get<u16>();
		auto message_info_raw = noti.get_rawdata();

		if (noti.is_error())
		{
			rpcn_log.error("Received faulty RoomMessageReceived notification");
			return;
		}

		u32 event_key = get_event_key();

		auto* message_info = flatbuffers::GetRoot<RoomMessageInfo>(message_info_raw.data());
		auto& edata        = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMessageInfo, sizeof(SceNpMatching2RoomMessageInfo));
		auto* notif_data   = reinterpret_cast<SceNpMatching2RoomMessageInfo*>(edata.data());
		RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(edata, message_info, notif_data);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		rpcn_log.notice("Received notification of a room message from member(%d) in room(%d)", member_id, room_id);

		if (room_msg_cb)
		{
			sysutil_register_cb([room_msg_cb = this->room_msg_cb, room_msg_cb_ctx = this->room_msg_cb_ctx, room_id, member_id, event_key, room_msg_cb_arg = this->room_msg_cb_arg, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					room_msg_cb(cb_ppu, room_msg_cb_ctx, room_id, member_id, SCE_NP_MATCHING2_ROOM_MSG_EVENT_Message, event_key, 0, size, room_msg_cb_arg);
					return 0;
				});
		}
	}

	void np_handler::notif_p2p_connect(std::vector<u8>& data)
	{
		if (data.size() != 16)
		{
			rpcn_log.error("Notification data for SignalP2PConnect != 14");
			return;
		}

		const u64 room_id   = reinterpret_cast<le_t<u64>&>(data[0]);
		const u16 member_id = reinterpret_cast<le_t<u16>&>(data[8]);
		const u16 port_p2p  = reinterpret_cast<be_t<u16>&>(data[10]);
		const u32 addr_p2p  = reinterpret_cast<le_t<u32>&>(data[12]);

		rpcn_log.notice("Received notification to connect to member(%d) of room(%d): %s:%d", member_id, room_id, ip_to_string(addr_p2p), port_p2p);

		// Attempt Signaling
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.set_sig2_infos(room_id, member_id, SCE_NP_SIGNALING_CONN_STATUS_PENDING, addr_p2p, port_p2p);
		sigh.start_sig2(room_id, member_id);
	}
} // namespace np
