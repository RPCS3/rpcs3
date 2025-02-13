#include "Emu/NP/rpcn_types.h"
#include "stdafx.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/IdManager.h"
#include "np_handler.h"
#include "np_contexts.h"
#include "np_structs_extra.h"
#include "fb_helpers.h"

LOG_CHANNEL(rpcn_log, "rpcn");

namespace np
{
	std::pair<error_code, shared_ptr<matching_ctx>> gui_prelude(u32 ctx_id, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return {SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND, {}};

		if (!ctx->busy.compare_and_swap_test(0, 1))
			return {SCE_NP_MATCHING_ERROR_CTX_STILL_RUNNING, {}};

		ctx->ctx_id = ctx_id;
		ctx->gui_handler = handler;
		ctx->gui_arg = arg;

		ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_COMMON_LOAD, 0);

		return {CELL_OK, ctx};
	}

	void gui_epilog(const shared_ptr<matching_ctx>& ctx)
	{
		ensure(ctx->busy.compare_and_swap_test(1, 0), "Matching context wasn't busy in gui_epilog");
		ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_COMMON_UNLOAD, 0);
	}

	void np_handler::set_current_gui_ctx_id(u32 id)
	{
		std::lock_guard lock(gui_notifications.mutex);

		gui_notifications.current_gui_ctx_id = id;

		if (id == 0)
		{
			for (const auto& [key, notif_data] : gui_notifications.list)
			{
				np_memory.free(notif_data.edata.addr());
			}

			gui_notifications.list.clear();
		}
	}

	void np_handler::set_gui_result(s32 event, np::event_data data)
	{
		std::lock_guard lock(gui_result.mutex);
		if (gui_result.event)
			np_memory.free(gui_result.data.addr());

		gui_result.data = std::move(data);
		gui_result.event = event;
	}

	error_code np_handler::get_matching_result(u32 ctx_id, u32 req_id, vm::ptr<void> buf, vm::ptr<u32> size, vm::ptr<s32> event)
	{
		std::lock_guard lock(gui_notifications.mutex);

		auto ctx = get_matching_context(ctx_id);

		if (!gui_notifications.current_gui_ctx_id || !ctx)
		{
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;
		}

		if (!gui_notifications.list.contains(std::make_pair(ctx_id, req_id)))
		{
			return SCE_NP_MATCHING_ERROR_INVALID_REQ_ID;
		}

		const auto key = std::make_pair(ctx_id, req_id);

		auto& notif = ::at32(gui_notifications.list, key);

		if (event)
		{
			*event = notif.event;
		}

		if (!buf)
		{
			*size = notif.edata.size();
			return CELL_OK;
		}

		const u32 final_size = std::min(static_cast<u32>(*size), notif.edata.size());
		notif.edata.apply_relocations(buf.addr());
		memcpy(buf.get_ptr(), notif.edata.data(), final_size);
		*size = final_size;

		np_memory.free(notif.edata.addr());
		gui_notifications.list.erase(key);

		return CELL_OK;
	}

	error_code np_handler::get_result_gui(vm::ptr<void> buf, vm::ptr<u32> size, vm::ptr<s32> event)
	{
		std::lock_guard lock(gui_result.mutex);

		if (!gui_result.event)
		{
			return SCE_NP_MATCHING_ERROR_RESULT_NOT_FOUND;
		}

		if (event)
		{
			*event = gui_result.event;
		}

		if (!buf)
		{
			*size = gui_result.data.size();
			return CELL_OK;
		}

		const u32 final_size = std::min(static_cast<u32>(*size), gui_result.data.size());
		gui_result.data.apply_relocations(buf.addr());
		memcpy(buf.get_ptr(), gui_result.data.data(), final_size);
		*size = final_size;

		np_memory.free(gui_result.data.addr());
		gui_result.event = 0;

		return CELL_OK;
	}

	error_code np_handler::create_room_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
	{
		const auto [error, ctx] = gui_prelude(ctx_id, handler, arg);

		if (error)
			return error;

		for (auto cur_attr = attr; cur_attr; cur_attr = cur_attr->next)
		{
			extra_nps::print_SceNpMatchingAttr(cur_attr.get_ptr());
		}

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		get_rpcn()->createjoin_room_gui(req_id, *communicationId, attr.get_ptr());

		return CELL_OK;
	}

	void np_handler::reply_create_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in CreateRoomGUI reply");
		const auto* resp = reply.get_flatbuffer<MatchingRoomStatus>();
		ensure(!reply.is_error(), "Malformed reply to CreateRoomGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingJoinedRoomInfo_SIZE), sizeof(SceNpMatchingJoinedRoomInfo), MAX_SceNpMatchingJoinedRoomInfo_SIZE);
		auto* room_info = reinterpret_cast<SceNpMatchingJoinedRoomInfo*>(edata.data());
		MatchingRoomStatus_to_SceNpMatchingJoinedRoomInfo(edata, resp, room_info);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		gui_cache.add_room(room_info->room_status.id);
		gui_cache.add_member(room_info->room_status.id, room_info->room_status.members.get_ptr(), true);

		set_gui_result(SCE_NP_MATCHING_GUI_EVENT_CREATE_ROOM, std::move(edata));
		ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_CREATE_ROOM, 0);
		gui_epilog(ctx);
	}

	error_code np_handler::join_room_gui(u32 ctx_id, vm::ptr<SceNpRoomId> roomid, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
	{
		auto [error, ctx] = gui_prelude(ctx_id, handler, arg);

		if (error)
			return error;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		get_rpcn()->join_room_gui(req_id, *roomid);

		return CELL_OK;
	}

	void np_handler::reply_join_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing:
			error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM;
			break;
		case rpcn::ErrorType::RoomFull:
			// Might also be SCE_NP_MATCHING_SERVER_ERROR_ACCESS_FORBIDDEN or SCE_NP_MATCHING_SERVER_ERROR_NOT_ALLOWED ?
			error_code = SCE_NP_MATCHING_SERVER_ERROR_ROOM_CLOSED;
			break;
		case rpcn::ErrorType::RoomAlreadyJoined:
			error_code = SCE_NP_MATCHING_SERVER_ERROR_ACCESS_FORBIDDEN;
			break;
		default:
			fmt::throw_exception("Unexpected error in JoinRoomGUI reply: %d", static_cast<u8>(error));
			break;
		}

		if (error_code != 0)
		{
			ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_JOIN_ROOM, error_code);
			gui_epilog(ctx);
			return;
		}

		const auto* resp = reply.get_flatbuffer<MatchingRoomStatus>();
		ensure(!reply.is_error(), "Malformed reply to JoinRoomGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingJoinedRoomInfo_SIZE), sizeof(SceNpMatchingJoinedRoomInfo), MAX_SceNpMatchingJoinedRoomInfo_SIZE);
		auto* room_info = reinterpret_cast<SceNpMatchingJoinedRoomInfo*>(edata.data());
		MatchingRoomStatus_to_SceNpMatchingJoinedRoomInfo(edata, resp, room_info);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingJoinedRoomInfo(room_info);

		gui_cache.add_room(room_info->room_status.id);

		for (auto cur_member = room_info->room_status.members; cur_member; cur_member = cur_member->next)
		{
			gui_cache.add_member(room_info->room_status.id, cur_member.get_ptr(), true);
		}

		set_gui_result(SCE_NP_MATCHING_GUI_EVENT_JOIN_ROOM, std::move(edata));
		ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_JOIN_ROOM, 0);
		gui_epilog(ctx);
	}

	error_code np_handler::leave_room_gui(u32 ctx_id, vm::cptr<SceNpRoomId> roomid)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(roomid);
		extra_nps::print_SceNpRoomId(*roomid);
		get_rpcn()->leave_room_gui(req_id, *roomid);

		return not_an_error(req_id);
	}

	void np_handler::reply_leave_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in LeaveRoomGUI reply: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_LEAVE_ROOM_DONE, error_code);
			return;
		}

		const auto* resp = reply.get_flatbuffer<MatchingRoomStatus>();
		ensure(!reply.is_error(), "Malformed reply to LeaveRoomGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingRoomStatus_SIZE), sizeof(SceNpMatchingRoomStatus), MAX_SceNpMatchingRoomStatus_SIZE);
		auto* room_status = reinterpret_cast<SceNpMatchingRoomStatus*>(edata.data());
		MatchingRoomStatus_to_SceNpMatchingRoomStatus(edata, resp, room_status);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingRoomStatus(room_status);

		gui_cache.del_room(room_status->id);

		gui_notifications.list.emplace(std::make_pair(gui_notifications.current_gui_ctx_id, req_id), gui_notification{.event = SCE_NP_MATCHING_EVENT_LEAVE_ROOM_DONE, .edata = std::move(edata)});
		ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_LEAVE_ROOM_DONE, 0);
	}

	error_code np_handler::get_room_list_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::ptr<SceNpMatchingReqRange> range, vm::ptr<SceNpMatchingSearchCondition> cond, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg, bool limit)
	{
		auto [error, ctx] = gui_prelude(ctx_id, handler, arg);

		if (error)
			return error;

		for (auto cur_cond = cond; cur_cond; cur_cond = cur_cond->next)
		{
			extra_nps::print_SceNpMatchingSearchCondition(cur_cond.get_ptr());
		}

		for (auto cur_attr = attr; cur_attr; cur_attr = cur_attr->next)
		{
			extra_nps::print_SceNpMatchingAttr(cur_attr.get_ptr());
		}

		ctx->get_room_limit_version = limit;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(range);
		get_rpcn()->get_room_list_gui(req_id, *communicationId, range.get_ptr(), cond, attr);

		return CELL_OK;
	}

	void np_handler::reply_get_room_list_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in GetRoomListGUI reply");
		const auto* resp = reply.get_flatbuffer<MatchingRoomList>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomListGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingRoomList_SIZE), sizeof(SceNpMatchingRoomList), MAX_SceNpMatchingRoomList_SIZE);
		auto* room_list = reinterpret_cast<SceNpMatchingRoomList*>(edata.data());
		MatchingRoomList_to_SceNpMatchingRoomList(edata, resp, room_list);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingRoomList(room_list);

		if (ctx->get_room_limit_version)
		{
			set_gui_result(SCE_NP_MATCHING_GUI_EVENT_GET_ROOM_LIST_LIMIT, std::move(edata));
			ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_GET_ROOM_LIST_LIMIT, 0);
		}
		else
		{
			set_gui_result(SCE_NP_MATCHING_GUI_EVENT_GET_ROOM_LIST, std::move(edata));
			ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_GET_ROOM_LIST, 0);
		}

		gui_epilog(ctx);
	}

	error_code np_handler::set_room_search_flag_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> /* lobby_id */, vm::ptr<SceNpRoomId> room_id, s32 flag)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(room_id);
		extra_nps::print_SceNpRoomId(*room_id);
		get_rpcn()->set_room_search_flag_gui(req_id, *room_id, flag);

		return not_an_error(req_id);
	}

	void np_handler::reply_set_room_search_flag_gui(u32 req_id, rpcn::ErrorType error)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::Unauthorized: error_code = SCE_NP_MATCHING_SERVER_ERROR_NOT_ALLOWED; break;
		default: fmt::throw_exception("Unexpected error in SetRoomSearchFlagGUI reply: %d", static_cast<u8>(error));
		}

		ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_SET_ROOM_SEARCH_FLAG_DONE, error_code);
	}

	error_code np_handler::get_room_search_flag_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> /* lobby_id */, vm::ptr<SceNpRoomId> room_id)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(room_id);
		extra_nps::print_SceNpRoomId(*room_id);
		get_rpcn()->get_room_search_flag_gui(req_id, *room_id);

		return not_an_error(req_id);
	}

	void np_handler::reply_get_room_search_flag_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in GetRoomSearchFlagGUI reply: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE, error_code);
			return;
		}

		const auto* resp = reply.get_flatbuffer<MatchingRoom>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomSearchFlagGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingRoom_SIZE), sizeof(SceNpMatchingRoom), MAX_SceNpMatchingRoom_SIZE);
		auto* room_info = reinterpret_cast<SceNpMatchingRoom*>(edata.data());
		MatchingRoom_to_SceNpMatchingRoom(edata, resp, room_info);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingRoom(room_info);

		gui_notifications.list.emplace(std::make_pair(gui_notifications.current_gui_ctx_id, req_id), gui_notification{.event = SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE, .edata = std::move(edata)});
		ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE, 0);
	}

	error_code np_handler::set_room_info_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> /* lobby_id */, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		for (auto cur_attr = attr; cur_attr; cur_attr = cur_attr->next)
		{
			extra_nps::print_SceNpMatchingAttr(cur_attr.get_ptr());
		}

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(room_id && attr);
		// extra_nps::print_SceNpRoomId(*room_id);
		get_rpcn()->set_room_info_gui(req_id, *room_id, attr);

		return not_an_error(req_id);
	}

	void np_handler::reply_set_room_info_gui(u32 req_id, rpcn::ErrorType error)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::Unauthorized: error_code = SCE_NP_MATCHING_SERVER_ERROR_NOT_ALLOWED; break;
		default: fmt::throw_exception("Unexpected error in SetRoomInfoGUI reply: %d", static_cast<u8>(error));
		}

		ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_SET_ROOM_INFO_DONE, error_code);
	}

	error_code np_handler::get_room_info_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> /* lobby_id */, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		ensure(room_id && attr);
		// extra_nps::print_SceNpRoomId(*room_id);
		get_rpcn()->get_room_info_gui(req_id, *room_id, attr);

		return not_an_error(req_id);
	}

	void np_handler::reply_get_room_info_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in GetRoomInfoGUI reply: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_GET_ROOM_INFO_DONE, error_code);
			return;
		}

		const auto* resp = reply.get_flatbuffer<MatchingRoom>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomInfoGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingRoom_SIZE), sizeof(SceNpMatchingRoom), MAX_SceNpMatchingRoom_SIZE);
		auto* room_info = reinterpret_cast<SceNpMatchingRoom*>(edata.data());
		MatchingRoom_to_SceNpMatchingRoom(edata, resp, room_info);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingRoom(room_info);

		gui_notifications.list.emplace(std::make_pair(gui_notifications.current_gui_ctx_id, req_id), gui_notification{.event = SCE_NP_MATCHING_EVENT_GET_ROOM_INFO_DONE, .edata = std::move(edata)});
		ctx->queue_callback(req_id, SCE_NP_MATCHING_EVENT_GET_ROOM_INFO_DONE, 0);
	}

	error_code np_handler::quickmatch_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, s32 available_num, s32 timeout, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
	{
		auto [error, ctx] = gui_prelude(ctx_id, handler, arg);

		if (error)
			return error;

		ctx->timeout = timeout;
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		get_rpcn()->quickmatch_gui(req_id, *communicationId, cond, available_num);

		return CELL_OK;
	}

	void np_handler::reply_quickmatch_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in QuickMatchGUI reply");
		const auto* resp = reply.get_flatbuffer<MatchingGuiRoomId>();
		ensure(!reply.is_error(), "Malformed reply to QuickMatchGUI command");

		SceNpRoomId room_id{};
		ensure(resp->id() && resp->id()->size() == sizeof(SceNpRoomId::opt));
		std::memcpy(room_id.opt, resp->id()->data(), sizeof(SceNpRoomId::opt));
		const auto [_, inserted] = pending_quickmatching.insert_or_assign(room_id, ctx->ctx_id);
		ensure(inserted);

		// Now that the reply has been received, we start the wait for the notification
		ctx->thread = std::make_unique<named_thread<std::function<void(SceNpRoomId)>>>("NP GUI Timeout Worker", [ctx, req_id, this](SceNpRoomId room_id)
			{
				ctx->wakey.wait(0, static_cast<atomic_wait_timeout>(ctx->timeout * 1'000'000'000));

				if (thread_ctrl::state() == thread_state::aborting)
					return;

				{
					std::lock_guard lock(this->mutex_quickmatching);
					if (this->pending_quickmatching.erase(room_id) != 1)
						return;
				}

				if (ctx->wakey == 0)
				{
					// Verify that the context is still valid
					if (!idm::check_unlocked<matching_ctx>(ctx->ctx_id))
						return;

					rpcn_log.notice("QuickMatch timeout");

					const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
					this->get_rpcn()->leave_room_gui(req_id, room_id);

					ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_QUICK_MATCH, SCE_NP_MATCHING_ERROR_TIMEOUT);
					gui_epilog(ctx);
				}
			});

		ctx->wakey = 0;
		auto& thread = *ctx->thread;
		thread(room_id);
	}

	error_code np_handler::searchjoin_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
	{
		auto [error, ctx] = gui_prelude(ctx_id, handler, arg);

		if (error)
			return error;

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::GUI);
		add_gui_request(req_id, ctx_id);

		get_rpcn()->searchjoin_gui(req_id, *communicationId, cond, attr);
		return CELL_OK;
	}

	void np_handler::reply_searchjoin_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto ctx = take_pending_gui_request(req_id);

		if (!ctx)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: error_code = SCE_NP_MATCHING_ERROR_SEARCH_JOIN_ROOM_NOT_FOUND; break;
		default: fmt::throw_exception("Unexpected error in SearchJoinRoomGUI reply: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN, error_code);
			gui_epilog(ctx);
			return;
		}

		const auto* resp = reply.get_flatbuffer<MatchingSearchJoinRoomInfo>();
		ensure(!reply.is_error(), "Malformed reply to SearchJoinRoomGUI command");

		event_data edata(np_memory.allocate(MAX_SceNpMatchingSearchJoinRoomInfo_SIZE), sizeof(SceNpMatchingSearchJoinRoomInfo), MAX_SceNpMatchingSearchJoinRoomInfo_SIZE);
		auto* room_info = reinterpret_cast<SceNpMatchingSearchJoinRoomInfo*>(edata.data());
		MatchingSearchJoinRoomInfo_to_SceNpMatchingSearchJoinRoomInfo(edata, resp, room_info);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatchingSearchJoinRoomInfo(room_info);

		gui_cache.add_room(room_info->room_status.id);

		for (auto cur_member = room_info->room_status.members; cur_member; cur_member = cur_member->next)
		{
			gui_cache.add_member(room_info->room_status.id, cur_member.get_ptr(), true);
		}

		set_gui_result(SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN, std::move(edata));
		ctx->queue_gui_callback(SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN, 0);
		gui_epilog(ctx);
	}

	// Local cache requests

	error_code np_handler::get_room_member_list_local_gui(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<u32> buflen, vm::ptr<void> buf)
	{
		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return SCE_NP_MATCHING_ERROR_CTX_NOT_FOUND;

		if (!room_id)
			return SCE_NP_MATCHING_ERROR_ROOM_NOT_FOUND;

		if (!buf)
		{
			error_code room_size = gui_cache.get_room_member_list(*room_id, 0, {});

			if (room_size < 0)
				return room_size;

			*buflen = room_size;
			return CELL_OK;
		}

		return gui_cache.get_room_member_list(*room_id, *buflen, buf);
	}

} // namespace np
