#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/NP/rpcn_types.h"
#include "Utilities/StrFmt.h"
#include "stdafx.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/IdManager.h"
#include "np_handler.h"
#include "np_contexts.h"
#include "np_helpers.h"
#include "np_structs_extra.h"
#include "fb_helpers.h"
#include "Emu/NP/signaling_handler.h"
#include "Emu/NP/ip_address.h"

LOG_CHANNEL(rpcn_log, "rpcn");

namespace np
{
	std::vector<SceNpMatching2ServerId> np_handler::get_match2_server_list(SceNpMatching2ContextId ctx_id)
	{
		std::vector<SceNpMatching2ServerId> server_list{};

		if (g_cfg.net.psn_status != np_psn_status::psn_rpcn)
		{
			return server_list;
		}

		if (!get_rpcn()->get_server_list(get_req_id(REQUEST_ID_HIGH::MISC), get_match2_context(ctx_id)->communicationId, server_list))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return server_list;
	}

	u64 np_handler::get_network_time()
	{
		// If network time hasn't been set we need to sync time with the rpcn server
		auto get_local_timestamp = []() -> u64
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(steady_clock::now().time_since_epoch()).count();
		};

		if (!network_time_offset)
		{
			// Could be improved with multiple requests to increase latency determination accuracy
			const u64 req_timestamp = get_local_timestamp();
			const u64 server_timestamp = get_rpcn()->get_network_time(get_req_id(REQUEST_ID_HIGH::MISC));
			if (!server_timestamp)
			{
				rpcn_log.error("Disconnecting from RPCN!");
				is_psn_active = false;
				return 0;
			}
			const u64 reply_timestamp = get_local_timestamp();
			const u64 latency = (reply_timestamp - req_timestamp) / 2;
			network_time_offset = reply_timestamp - (server_timestamp + latency);
		}

		return get_local_timestamp() - network_time_offset;
	}

	u32 np_handler::get_server_status(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		// TODO: actually implement interaction with server for this?
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo);
		const u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetServerInfo, sizeof(SceNpMatching2GetServerInfoResponse));
		SceNpMatching2GetServerInfoResponse* serv_info = reinterpret_cast<SceNpMatching2GetServerInfoResponse*>(edata.data());
		serv_info->server.serverId = server_id;
		serv_info->server.status = SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE;
		np_memory.shrink_allocation(edata.addr(), edata.size());

		const auto cb_info_opt = take_pending_request(req_id);
		ensure(cb_info_opt);
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return req_id;
	}

	u32 np_handler::create_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 /*server_id*/)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext);

		const auto cb_info_opt = take_pending_request(req_id);
		ensure(cb_info_opt);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return req_id;
	}

	u32 np_handler::delete_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 /*server_id*/)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_DeleteServerContext);

		const auto cb_info_opt = take_pending_request(req_id);
		ensure(cb_info_opt);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return req_id;
	}

	u32 np_handler::get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList);

		if (!get_rpcn()->get_world_list(req_id, get_match2_context(ctx_id)->communicationId, server_id))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_get_world_list(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in GetWorldList reply");

		std::vector<u32> world_list;
		u32 num_worlds = reply.get<u32>();
		for (u32 i = 0; i < num_worlds; i++)
		{
			world_list.push_back(reply.get<u32>());
		}

		ensure(!reply.is_error(), "Malformed reply to GetWorldList command");

		const u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetWorldInfoList, sizeof(SceNpMatching2GetWorldInfoListResponse));
		auto* world_info = reinterpret_cast<SceNpMatching2GetWorldInfoListResponse*>(edata.data());
		world_info->worldNum = ::size32(world_list);

		if (!world_list.empty())
		{
			auto* worlds = edata.allocate<SceNpMatching2World>(::narrow<u32>(sizeof(SceNpMatching2World) * world_list.size()), world_info->world);
			for (usz i = 0; i < world_list.size(); i++)
			{
				worlds[i].worldId = world_list[i];
			}
		}

		np_memory.shrink_allocation(edata.addr(), edata.size());
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom);

		extra_nps::print_SceNpMatching2CreateJoinRoomRequest(req);

		if (!get_rpcn()->createjoin_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		// More elegant solution would be to send back password with creation reply
		cached_cj_password = req->roomPassword ? std::optional<SceNpMatching2SessionPassword>{*req->roomPassword} : std::nullopt;

		return req_id;
	}

	void np_handler::reply_create_join_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomGroupMaxSlotMismatch: error_code = SCE_NP_MATCHING2_SERVER_ERROR_MAX_OVER_SLOT_GROUP; break;
		case rpcn::ErrorType::RoomPasswordMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_PASSWORD; break;
		case rpcn::ErrorType::RoomGroupNoJoinLabel: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_JOIN_GROUP_LABEL; break;
		default: fmt::throw_exception("Unexpected error in reply to CreateRoom: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		const auto* resp = reply.get_flatbuffer<RoomDataInternal>();
		ensure(!reply.is_error(), "Malformed reply to CreateRoom command");

		const u32 event_key = get_event_key();
		auto [include_onlinename, include_avatarurl] = get_match2_context_options(cb_info_opt->ctx_id);

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_CreateJoinRoom, sizeof(SceNpMatching2CreateJoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2CreateJoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid, include_onlinename, include_avatarurl);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);
		np_cache.update_password(room_resp->roomDataInternal->roomId, cached_cj_password);

		extra_nps::print_SceNpMatching2CreateJoinRoomResponse(room_resp);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom);

		extra_nps::print_SceNpMatching2JoinRoomRequest(req);

		if (!get_rpcn()->join_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_join_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = 0;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::RoomAlreadyJoined: error_code = SCE_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED; break;
		case rpcn::ErrorType::RoomFull: error_code = SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL; break;
		case rpcn::ErrorType::RoomPasswordMismatch: error_code = SCE_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH; break;
		case rpcn::ErrorType::RoomGroupFull: error_code = SCE_NP_MATCHING2_SERVER_ERROR_GROUP_FULL; break;
		case rpcn::ErrorType::RoomGroupJoinLabelNotFound: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_GROUP; break;
		default: fmt::throw_exception("Unexpected error in reply to JoinRoom: %d", static_cast<u8>(error));
		}

		if (error_code != 0)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		const auto* resp = reply.get_flatbuffer<JoinRoomResponse>();
		ensure(!reply.is_error(), "Malformed reply to JoinRoom command");
		ensure(resp->room_data());

		const u32 event_key = get_event_key();
		const auto [include_onlinename, include_avatarurl] = get_match2_context_options(cb_info_opt->ctx_id);

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_JoinRoom, sizeof(SceNpMatching2JoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2JoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp->room_data(), room_info, npid, include_onlinename, include_avatarurl);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_SceNpMatching2RoomDataInternal(room_info);

		// We initiate signaling if necessary
		if (const auto* signaling_data = resp->signaling_data())
		{
			const u64 room_id = resp->room_data()->roomId();

			for (unsigned int i = 0; i < signaling_data->size(); i++)
			{
				const auto* signaling_info = signaling_data->Get(i);
				ensure(signaling_info->addr());

				const u32 addr_p2p = register_ip(signaling_info->addr()->ip());
				const u16 port_p2p = signaling_info->addr()->port();

				const u16 member_id = signaling_info->member_id();
				const auto [npid_res, npid_p2p] = np_cache.get_npid(room_id, member_id);

				if (npid_res != CELL_OK)
					continue;

				rpcn_log.notice("JoinRoomResult told to connect to member(%d=%s) of room(%d): %s:%d", member_id, reinterpret_cast<const char*>(npid_p2p->handle.data), room_id, ip_to_string(addr_p2p), port_p2p);

				// Attempt Signaling
				auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
				const u32 conn_id = sigh.init_sig2(*npid_p2p, room_id, member_id);
				sigh.start_sig(conn_id, addr_p2p, port_p2p);
			}
		}

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom);

		if (!get_rpcn()->leave_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_leave_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break; // Unsure if this should return another error(missing user in room has no appropriate error code)
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in reply to LeaveRoom: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		u64 room_id = reply.get<u64>();
		ensure(!reply.is_error(), "Malformed reply to LeaveRoom command");

		// Disconnect all users from that room
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.disconnect_sig2_users(room_id);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);
	}

	u32 np_handler::search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom);

		extra_nps::print_SceNpMatching2SearchRoomRequest(req);

		if (!get_rpcn()->search_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_search_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in SearchRoom reply");

		const auto* resp = reply.get_flatbuffer<SearchRoomResponse>();
		ensure(!reply.is_error(), "Malformed reply to SearchRoom command");

		const u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SearchRoom, sizeof(SceNpMatching2SearchRoomResponse));
		auto* search_resp = reinterpret_cast<SceNpMatching2SearchRoomResponse*>(edata.data());
		// The online_name and avatar_url are naturally filtered by the reply from the server
		SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(edata, resp, search_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatching2SearchRoomResponse(search_resp);
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::get_roomdata_external_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataExternalList);

		extra_nps::print_SceNpMatching2GetRoomDataExternalListRequest(req);

		if (!get_rpcn()->get_roomdata_external_list(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_get_roomdata_external_list(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in GetRoomDataExternalList reply");

		const auto* resp = reply.get_flatbuffer<GetRoomDataExternalListResponse>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomDataExternalList command");

		const u32 event_key = get_event_key();
		auto [include_onlinename, include_avatarurl] = get_match2_context_options(cb_info_opt->ctx_id);

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataExternalList, sizeof(SceNpMatching2GetRoomDataExternalListResponse));
		auto* sce_get_room_ext_resp = reinterpret_cast<SceNpMatching2GetRoomDataExternalListResponse*>(edata.data());
		GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(edata, resp, sce_get_room_ext_resp, include_onlinename, include_avatarurl);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_SceNpMatching2GetRoomDataExternalListResponse(sce_get_room_ext_resp);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataExternal);

		extra_nps::print_SceNpMatching2SetRoomDataExternalRequest(req);

		if (!get_rpcn()->set_roomdata_external(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_set_roomdata_external(u32 req_id, rpcn::ErrorType error)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::Unauthorized: error_code = SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN; break;
		default: fmt::throw_exception("Unexpected error in reply to SetRoomDataExternal: %d", static_cast<u8>(error));
		}

		cb_info_opt->queue_callback(req_id, 0, error_code, 0);
	}

	u32 np_handler::get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataInternal);

		if (!get_rpcn()->get_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_get_roomdata_internal(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in reply to GetRoomDataInternal: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		const auto* resp = reply.get_flatbuffer<RoomDataInternal>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomDataInternal command");

		const u32 event_key = get_event_key();
		auto [include_onlinename, include_avatarurl] = get_match2_context_options(cb_info_opt->ctx_id);

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataInternal, sizeof(SceNpMatching2GetRoomDataInternalResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2GetRoomDataInternalResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid, include_onlinename, include_avatarurl);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_SceNpMatching2RoomDataInternal(room_info);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataInternal);

		extra_nps::print_SceNpMatching2SetRoomDataInternalRequest(req);

		if (!get_rpcn()->set_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_set_roomdata_internal(u32 req_id, rpcn::ErrorType error)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in reply to GetRoomDataInternal: %d", static_cast<u8>(error));
		}

		cb_info_opt->queue_callback(req_id, 0, error_code, 0);
	}

	u32 np_handler::get_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomMemberDataInternalRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomMemberDataInternal);
		extra_nps::print_SceNpMatching2GetRoomMemberDataInternalRequest(req);

		if (!get_rpcn()->get_roommemberdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_get_roommemberdata_internal(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::NotFound: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER; break;
		default: fmt::throw_exception("Unexpected error in reply to GetRoomMemberDataInternal: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		const auto* resp = reply.get_flatbuffer<RoomMemberDataInternal>();
		ensure(!reply.is_error(), "Malformed reply to GetRoomMemberDataInternal command");

		const u32 event_key = get_event_key();
		auto [include_onlinename, include_avatarurl] = get_match2_context_options(cb_info_opt->ctx_id);

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomMemberDataInternal, sizeof(SceNpMatching2GetRoomMemberDataInternalResponse));
		auto* mdata_resp = reinterpret_cast<SceNpMatching2GetRoomMemberDataInternalResponse*>(edata.data());
		auto* mdata_info = edata.allocate<SceNpMatching2RoomMemberDataInternal>(sizeof(SceNpMatching2RoomMemberDataInternal), mdata_resp->roomMemberDataInternal);
		RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(edata, resp, nullptr, mdata_info, include_onlinename, include_avatarurl);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::set_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomMemberDataInternal);

		extra_nps::print_SceNpMatching2SetRoomMemberDataInternalRequest(req);

		if (!get_rpcn()->set_roommemberdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_set_roommemberdata_internal(u32 req_id, rpcn::ErrorType error)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::NotFound: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER; break;
		case rpcn::ErrorType::Unauthorized: error_code = SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN; break;
		default: fmt::throw_exception("Unexpected error in reply to SetRoomMemberDataInternal: %d", static_cast<u8>(error));
		}

		cb_info_opt->queue_callback(req_id, 0, error_code, 0);
	}

	u32 np_handler::set_userinfo(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetUserInfoRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetUserInfo);

		if (!get_rpcn()->set_userinfo(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_set_userinfo(u32 req_id, rpcn::ErrorType error)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		default: fmt::throw_exception("Unexpected error in reply to SetUserInfo: %d", static_cast<u8>(error));
		}

		cb_info_opt->queue_callback(req_id, 0, 0, 0);
	}

	u32 np_handler::get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SignalingGetPingInfo);

		if (!get_rpcn()->ping_room_owner(req_id, get_match2_context(ctx_id)->communicationId, req->roomId))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_get_ping_info(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		default: fmt::throw_exception("Unexpected error in reply to PingRoomOwner: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return;
		}

		const auto* resp = reply.get_flatbuffer<GetPingInfoResponse>();
		ensure(!reply.is_error(), "Malformed reply to PingRoomOwner command");

		const u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SignalingGetPingInfo, sizeof(SceNpMatching2SignalingGetPingInfoResponse));
		auto* final_ping_resp = reinterpret_cast<SceNpMatching2SignalingGetPingInfoResponse*>(edata.data());
		GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(resp, final_ping_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());
	}

	u32 np_handler::send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req)
	{
		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomMessage);

		if (!get_rpcn()->send_room_message(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	void np_handler::reply_send_room_message(u32 req_id, rpcn::ErrorType error)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return;

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
		case rpcn::ErrorType::Unauthorized: error_code = SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN; break;
		default: fmt::throw_exception("Unexpected error in reply to SendRoomMessage: %d", static_cast<u8>(error));
		}

		cb_info_opt->queue_callback(req_id, 0, error_code, 0);
	}

	void np_handler::req_sign_infos(const std::string& npid, u32 conn_id)
	{
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::MISC);
		{
			std::lock_guard lock(mutex_pending_sign_infos_requests);
			pending_sign_infos_requests[req_id] = conn_id;
		}

		if (!get_rpcn()->req_sign_infos(req_id, npid))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}
	}

	void np_handler::reply_req_sign_infos(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		u32 conn_id;
		{
			std::lock_guard lock(mutex_pending_sign_infos_requests);
			conn_id = ::at32(pending_sign_infos_requests, req_id);
			pending_sign_infos_requests.erase(req_id);
		}

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound:
		{
			rpcn_log.error("Signaling information was requested for a user that doesn't exist or is not online");
			return;
		}
		default: fmt::throw_exception("Unexpected error in reply to RequestSignalingInfos: %d", static_cast<u8>(error));
		}

		const auto* resp = reply.get_flatbuffer<SignalingAddr>();
		ensure(!reply.is_error() && resp->ip(), "Malformed reply to RequestSignalingInfos command");
		u32 addr = register_ip(resp->ip());

		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.start_sig(conn_id, addr, resp->port());
	}

	u32 np_handler::get_lobby_info_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetLobbyInfoListRequest* req)
	{
		// Hack
		// Note that this is fake and needs to be properly implemented both on server and on rpcs3

		extra_nps::print_SceNpMatching2GetLobbyInfoListRequest(req);

		const u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetLobbyInfoList);
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		const u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetLobbyInfoList, sizeof(SceNpMatching2GetLobbyInfoListResponse));
		auto* resp = reinterpret_cast<SceNpMatching2GetLobbyInfoListResponse*>(edata.data());

		resp->range.size = 1;
		resp->range.startIndex = 1;
		resp->range.total = 1;
		auto lobby_data = edata.allocate<SceNpMatching2LobbyDataExternal>(sizeof(SceNpMatching2LobbyDataExternal), resp->lobbyDataExternal);
		lobby_data->serverId = 1;
		lobby_data->worldId = req->worldId;
		lobby_data->lobbyId = 1;
		lobby_data->maxSlot = 64;
		lobby_data->curMemberNum = 0;
		lobby_data->flagAttr = SCE_NP_MATCHING2_LOBBY_FLAG_ATTR_PERMANENT;

		np_memory.shrink_allocation(edata.addr(), edata.size());
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return req_id;
	}

	void np_handler::req_ticket([[maybe_unused]] u32 version, [[maybe_unused]] const SceNpId* npid, const char* service_id, const u8* cookie, u32 cookie_size, [[maybe_unused]] const char* entitlement_id, [[maybe_unused]] u32 consumed_count)
	{
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::MISC);

		std::string service_id_str(service_id);

		std::vector<u8> cookie_vec;
		if (cookie && cookie_size)
		{
			cookie_vec.assign(cookie, cookie + cookie_size);
		}

		if (!get_rpcn()->req_ticket(req_id, service_id_str, cookie_vec))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return;
	}

	const ticket& np_handler::get_ticket() const
	{
		return current_ticket;
	}

	void np_handler::reply_req_ticket(u32 /* req_id */, rpcn::ErrorType error, vec_stream& reply)
	{
		ensure(error == rpcn::ErrorType::NoError, "Unexpected error in reply to RequestTicket");
		auto ticket_raw = reply.get_rawdata();
		ensure(!reply.is_error(), "Malformed reply to RequestTicket command");

		current_ticket = ticket(std::move(ticket_raw));
		auto ticket_size = static_cast<s32>(current_ticket.size());

		if (manager_cb)
		{
			sysutil_register_cb([manager_cb = this->manager_cb, ticket_size, manager_cb_arg = this->manager_cb_arg](ppu_thread& cb_ppu) -> s32
				{
					manager_cb(cb_ppu, SCE_NP_MANAGER_EVENT_GOT_TICKET, ticket_size, manager_cb_arg);
					return 0;
				});
		}
	}

	void np_handler::transaction_async_handler(std::unique_lock<shared_mutex> lock, const shared_ptr<generic_async_transaction_context>& trans_ctx, u32 req_id, bool async)
	{
		auto worker_function = [trans_ctx = trans_ctx, req_id, this](std::unique_lock<shared_mutex> lock)
		{
			thread_base::set_name("NP Trans Worker");

			auto res = trans_ctx->wake_cond.wait_for(lock, std::chrono::microseconds(trans_ctx->timeout));
			{
				std::lock_guard lock_threads(this->mutex_async_transactions);
				this->async_transactions.erase(req_id);
			}

			if (res == std::cv_status::timeout)
			{
				trans_ctx->result = SCE_NP_COMMUNITY_ERROR_TIMEOUT;
				return;
			}

			// Only 2 cases should be timeout or caller setting result
			ensure(trans_ctx->result, "transaction_async_handler: trans_ctx->result is not set");
			trans_ctx->completion_cond.notify_one();
		};

		{
			std::lock_guard lock_score(mutex_async_transactions);
			ensure(async_transactions.insert({req_id, trans_ctx}).second, "transaction_async_handler: async_transactions insert failed");
		}

		if (async)
		{
			trans_ctx->thread = std::thread(worker_function, std::move(lock));
			return;
		}

		auto& cpu_thread = *get_current_cpu_thread();
		lv2_obj::sleep(cpu_thread);
		worker_function(std::move(lock));
		cpu_thread.check_state();
	}

	void np_handler::get_board_infos(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
		trans_ctx->tdata = tdata_get_board_infos{.boardInfo = boardInfo};
		get_rpcn()->get_board_infos(req_id, trans_ctx->communicationId, boardId);

		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_get_board_infos(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto score_trans = idm::get_unlocked<score_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(score_trans);

		s32 error_code = CELL_OK;

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: error_code = SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND; break;
		default: fmt::throw_exception("Unexpected error in reply to GetBoardInfos: %d", static_cast<u8>(error));
		}

		if (error_code != CELL_OK)
		{
			std::lock_guard lock(score_trans->mutex);
			score_trans->result = error_code;
			score_trans->wake_cond.notify_one();
			return;
		}

		const auto* resp = reply.get_flatbuffer<BoardInfo>();
		ensure(!reply.is_error(), "Malformed reply to GetBoardInfos command");

		const SceNpScoreBoardInfo board_info{
			.rankLimit = resp->rankLimit(),
			.updateMode = resp->updateMode(),
			.sortMode = resp->sortMode(),
			.uploadNumLimit = resp->uploadNumLimit(),
			.uploadSizeLimit = resp->uploadSizeLimit()
		};

		std::lock_guard lock(score_trans->mutex);

		const auto* tdata = std::get_if<tdata_get_board_infos>(&score_trans->tdata);
		ensure(tdata);

		memcpy(reinterpret_cast<u8*>(tdata->boardInfo.get_ptr()), &board_info, sizeof(SceNpScoreBoardInfo));
		score_trans->result = CELL_OK;
		score_trans->wake_cond.notify_one();
	}

	void np_handler::record_score(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, const u8* data, u32 data_size, vm::ptr<SceNpScoreRankNumber> tmpRank, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
		std::optional<std::string> str_comment = scoreComment ? std::optional(std::string(reinterpret_cast<const char*>(scoreComment->data))) : std::nullopt;
		std::optional<std::vector<u8>> vec_data;

		if (data)
		{
			vec_data = std::vector<u8>(data, data + data_size);
		}

		trans_ctx->tdata = tdata_record_score{.tmpRank = tmpRank};

		get_rpcn()->record_score(req_id, trans_ctx->communicationId, boardId, trans_ctx->pcId, score, str_comment, vec_data);

		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_record_score(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto score_trans = idm::get_unlocked<score_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(score_trans);

		std::lock_guard lock(score_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::ScoreNotBest:
		{
			score_trans->result = SCE_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE;
			score_trans->wake_cond.notify_one();
			return;
		}
		default: fmt::throw_exception("Unexpected error in reply_record_score: %d", static_cast<u8>(error));
		}

		auto tmp_rank = reply.get<u32>();
		ensure(!reply.is_error(), "Error parsing response in reply_record_score");

		const auto* tdata = std::get_if<tdata_record_score>(&score_trans->tdata);
		ensure(tdata);

		if (tdata->tmpRank)
		{
			*tdata->tmpRank = tmp_rank;
		}

		score_trans->result = CELL_OK;
		score_trans->wake_cond.notify_one();
	}

	void np_handler::record_score_data(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, const u8* score_data, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		auto* tdata = std::get_if<tdata_record_score_data>(&trans_ctx->tdata);
		if (!tdata)
		{
			trans_ctx->tdata = tdata_record_score_data{.game_data_size = totalSize};
			tdata = std::get_if<tdata_record_score_data>(&trans_ctx->tdata);
			tdata->game_data.reserve(totalSize);
		}

		std::copy(score_data, score_data + sendSize, std::back_inserter(tdata->game_data));

		if (tdata->game_data.size() == tdata->game_data_size)
		{
			trans_ctx->result = std::nullopt;
			const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
			get_rpcn()->record_score_data(req_id, trans_ctx->communicationId, trans_ctx->pcId, boardId, score, tdata->game_data);
			transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
		}
		else
		{
			trans_ctx->result = CELL_OK;
		}
	}

	void np_handler::reply_record_score_data(u32 req_id, rpcn::ErrorType error)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto trans = ::at32(async_transactions, req_id);
		std::lock_guard lock(trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: trans->set_result_and_wake(CELL_OK); break;
		case rpcn::ErrorType::NotFound: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_STORE_NOT_FOUND); break;
		case rpcn::ErrorType::ScoreInvalid: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE); break;
		case rpcn::ErrorType::ScoreHasData: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_DATA_ALREADY_EXISTS); break;
		default: fmt::throw_exception("Unexpected error in reply to RecordScoreData: %d", static_cast<u8>(error));
		}
	}

	void np_handler::get_score_data(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const SceNpId& npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> score_data, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		auto* tdata = std::get_if<tdata_get_score_data>(&trans_ctx->tdata);
		if (!tdata)
		{
			trans_ctx->tdata = tdata_get_score_data{.totalSize = totalSize, .recvSize = recvSize, .score_data = score_data};

			const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
			get_rpcn()->get_score_data(req_id, trans_ctx->communicationId, trans_ctx->pcId, boardId, npId);
			transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
			return;
		}

		// Check if the transaction has actually completed, otherwise adjust tdata parameters
		if (!trans_ctx->result)
		{
			tdata->totalSize = totalSize;
			tdata->recvSize = recvSize;
			tdata->score_data = score_data;
			return;
		}

		// If here the data has already been acquired and the client is just asking for part of it
		usz to_copy = std::min(tdata->game_data.size(), static_cast<usz>(recvSize));
		std::memcpy(score_data.get_ptr(), tdata->game_data.data(), to_copy);
		tdata->game_data.erase(tdata->game_data.begin(), tdata->game_data.begin() + to_copy);
		*totalSize = tdata->game_data_size;
		trans_ctx->result = not_an_error(to_copy);
	}

	void np_handler::reply_get_score_data(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto score_trans = idm::get_unlocked<score_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(score_trans);
		std::lock_guard lock(score_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: score_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_GAME_DATA_MASTER_NOT_FOUND); return;
		default: fmt::throw_exception("Unexpected error in reply to GetScoreData: %d", static_cast<u8>(error));
		}

		auto* tdata = std::get_if<tdata_get_score_data>(&score_trans->tdata);
		ensure(tdata);

		tdata->game_data = reply.get_rawdata();
		ensure(!reply.is_error(), "Error parsing response in reply_get_score_data");

		tdata->game_data_size = ::size32(tdata->game_data);

		usz to_copy = std::min(tdata->game_data.size(), static_cast<usz>(tdata->recvSize));
		std::memcpy(tdata->score_data.get_ptr(), tdata->game_data.data(), to_copy);
		tdata->game_data.erase(tdata->game_data.begin(), tdata->game_data.begin() + to_copy);
		*tdata->totalSize = tdata->game_data_size;

		score_trans->set_result_and_wake(not_an_error(to_copy));
	}

	void np_handler::get_score_range(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);

		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
			.player_rank_data = false,
			.deprecated = deprecated,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_range(req_id, trans_ctx->communicationId, boardId, startSerialRank, arrayNum, with_comments, with_gameinfo);

		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	template <typename T>
	void set_rankdata_values(T& cur_rank, const ScoreRankData* fb_rankdata)
	{
		string_to_npid(fb_rankdata->npId()->string_view(), cur_rank.npId);
		string_to_online_name(fb_rankdata->onlineName()->string_view(), cur_rank.onlineName);

		static_assert(std::is_same_v<T, SceNpScoreRankData> || std::is_same_v<T, SceNpScoreRankData_deprecated>);

		if constexpr (std::is_same_v<T, SceNpScoreRankData>)
			cur_rank.pcId = fb_rankdata->pcId();

		cur_rank.serialRank = fb_rankdata->rank();
		cur_rank.rank = fb_rankdata->rank();
		cur_rank.highestRank = fb_rankdata->rank();
		cur_rank.scoreValue = fb_rankdata->score();
		cur_rank.hasGameData = fb_rankdata->hasGameData();
		cur_rank.recordDate.tick = fb_rankdata->recordDate();
	}

	void np_handler::handle_GetScoreResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply, bool simple_result)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto score_trans = idm::get_unlocked<score_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(score_trans);
		std::lock_guard lock(score_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		default: fmt::throw_exception("Unexpected error in GetScoreResponse: %d", static_cast<u8>(error));
		}

		const auto* resp = reply.get_flatbuffer<GetScoreResponse>();
		ensure(!reply.is_error(), "Error parsing response in handle_GetScoreResponse");

		const auto* tdata = std::get_if<tdata_get_score_generic>(&score_trans->tdata);
		ensure(tdata);
		ensure(resp->rankArray() && resp->rankArray()->size() <= tdata->arrayNum);

		memset(tdata->rankArray.get_ptr(), 0, tdata->rankArraySize);
		auto* fb_rankarray = resp->rankArray();

		vm::ptr<SceNpScorePlayerRankData> rankPlayerArray = vm::static_ptr_cast<SceNpScorePlayerRankData>(tdata->rankArray);
		vm::ptr<SceNpScorePlayerRankData_deprecated> rankPlayerArray_deprecated = vm::static_ptr_cast<SceNpScorePlayerRankData_deprecated>(tdata->rankArray);
		vm::ptr<SceNpScoreRankData> rankArray = vm::static_ptr_cast<SceNpScoreRankData>(tdata->rankArray);
		vm::ptr<SceNpScoreRankData_deprecated> rankArray_deprecated = vm::static_ptr_cast<SceNpScoreRankData_deprecated>(tdata->rankArray);

		u32 num_scores_registered = 0;

		for (flatbuffers::uoffset_t i = 0; i < fb_rankarray->size(); i++)
		{
			const auto* fb_rankdata = fb_rankarray->Get(i);
			ensure(fb_rankdata->npId() && fb_rankdata->onlineName());

			if (fb_rankdata->recordDate() == 0)
				continue;

			num_scores_registered++;

			if (tdata->player_rank_data)
			{
				if (tdata->deprecated)
				{
					rankPlayerArray_deprecated[i].hasData = 1;
					set_rankdata_values(rankPlayerArray_deprecated[i].rankData, fb_rankdata);
				}
				else
				{
					rankPlayerArray[i].hasData = 1;
					set_rankdata_values(rankPlayerArray[i].rankData, fb_rankdata);
				}
			}
			else
			{
				if (tdata->deprecated)
				{
					set_rankdata_values(rankArray_deprecated[i], fb_rankdata);
				}
				else
				{
					set_rankdata_values(rankArray[i], fb_rankdata);
				}
			}
		}

		if (tdata->commentArray)
		{
			ensure(resp->commentArray() && resp->commentArray()->size() <= tdata->arrayNum);
			memset(tdata->commentArray.get_ptr(), 0, sizeof(SceNpScoreComment) * tdata->arrayNum);

			auto* fb_commentarray = resp->commentArray();
			for (flatbuffers::uoffset_t i = 0; i < fb_commentarray->size(); i++)
			{
				const auto* fb_comment = fb_commentarray->Get(i);
				strcpy_trunc(tdata->commentArray[i].data, fb_comment->string_view());
			}
		}

		if (tdata->infoArray)
		{
			ensure(resp->infoArray() && resp->infoArray()->size() <= tdata->arrayNum);
			auto* fb_infoarray = resp->infoArray();

			if ((tdata->arrayNum * sizeof(SceNpScoreGameInfo)) == tdata->infoArraySize)
			{
				vm::ptr<SceNpScoreGameInfo> ptr_gameinfo = vm::static_ptr_cast<SceNpScoreGameInfo>(tdata->infoArray);
				memset(ptr_gameinfo.get_ptr(), 0, sizeof(SceNpScoreGameInfo) * tdata->arrayNum);
				for (flatbuffers::uoffset_t i = 0; i < fb_infoarray->size(); i++)
				{
					const auto* fb_info = fb_infoarray->Get(i);
					ensure(fb_info->data()->size() <= SCE_NP_SCORE_GAMEINFO_SIZE);
					memcpy(ptr_gameinfo[i].nativeData, fb_info->data()->data(), fb_info->data()->size());
				}
			}
			else
			{
				vm::ptr<SceNpScoreVariableSizeGameInfo> ptr_vargameinfo = vm::static_ptr_cast<SceNpScoreVariableSizeGameInfo>(tdata->infoArray);
				memset(ptr_vargameinfo.get_ptr(), 0, sizeof(SceNpScoreVariableSizeGameInfo) * tdata->arrayNum);
				for (flatbuffers::uoffset_t i = 0; i < fb_infoarray->size(); i++)
				{
					const auto* fb_info = fb_infoarray->Get(i);
					ensure(fb_info->data()->size() <= SCE_NP_SCORE_VARIABLE_SIZE_GAMEINFO_MAXSIZE);
					ptr_vargameinfo[i].infoSize = fb_info->data()->size();
					memcpy(ptr_vargameinfo[i].data, fb_info->data(), fb_info->data()->size());
				}
			}
		}

		tdata->lastSortDate->tick = resp->lastSortDate();
		*tdata->totalRecord = resp->totalRecord();

		if (num_scores_registered)
			score_trans->result = simple_result ? CELL_OK : not_an_error(fb_rankarray->size());
		else
			score_trans->result = SCE_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND;

		score_trans->wake_cond.notify_one();
	}

	void np_handler::reply_get_score_range(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		handle_GetScoreResponse(req_id, error, reply);
	}

	void np_handler::get_score_friend(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, bool include_self, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
			.player_rank_data = false,
			.deprecated = deprecated,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_friend(req_id, trans_ctx->communicationId, boardId, include_self, with_comments, with_gameinfo, arrayNum);

		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	void np_handler::reply_get_score_friends(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		handle_GetScoreResponse(req_id, error, reply);
	}

	void np_handler::get_score_npid(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const std::vector<std::pair<SceNpId, s32>>& npid_vec, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
			.player_rank_data = true,
			.deprecated = deprecated,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_npid(req_id, trans_ctx->communicationId, boardId, npid_vec, with_comments, with_gameinfo);

		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	void np_handler::reply_get_score_npid(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		handle_GetScoreResponse(req_id, error, reply, true);
	}

	void np_handler::handle_tus_no_data(u32 req_id, rpcn::ErrorType error)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto trans = ::at32(async_transactions, req_id);
		std::lock_guard lock(trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: trans->set_result_and_wake(CELL_OK); break;
		case rpcn::ErrorType::NotFound: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED); break;
		case rpcn::ErrorType::Unauthorized: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN); break;
		case rpcn::ErrorType::CondFail: trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED); break;
		default: fmt::throw_exception("Unexpected error in handle_tus_no_data: %d", static_cast<u8>(error));
		}
	}

	void np_handler::handle_TusVarResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto tus_trans = idm::get_unlocked<tus_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(tus_trans);
		std::lock_guard lock(tus_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED);
		case rpcn::ErrorType::Unauthorized: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN);
		case rpcn::ErrorType::CondFail: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED);
		default: fmt::throw_exception("Unexpected error in handle_TusVarResponse: %d", static_cast<u8>(error));
		}

		const auto* resp = reply.get_flatbuffer<TusVarResponse>();
		ensure(!reply.is_error(), "Error parsing response in handle_TusVarResponse");

		const auto* tdata = std::get_if<tdata_tus_get_variables_generic>(&tus_trans->tdata);
		ensure(tdata);
		ensure(resp->vars() && resp->vars()->size() <= static_cast<usz>(tdata->arrayNum));

		const auto* fb_vars = resp->vars();

		memset(tdata->variableArray.get_ptr(), 0, sizeof(SceNpTusVariable) * tdata->arrayNum);
		for (flatbuffers::uoffset_t i = 0; i < fb_vars->size(); i++)
		{
			auto* cur_var = &tdata->variableArray[i];
			const auto* cur_fb_var = fb_vars->Get(i);

			ensure(cur_fb_var->ownerId());
			string_to_npid(cur_fb_var->ownerId()->string_view(), cur_var->ownerId);

			if (!cur_fb_var->hasData())
			{
				continue;
			}

			ensure(cur_fb_var->lastChangedAuthorId());

			cur_var->hasData = 1;
			cur_var->lastChangedDate.tick = cur_fb_var->lastChangedDate();
			string_to_npid(cur_fb_var->lastChangedAuthorId()->string_view(), cur_var->lastChangedAuthorId);
			cur_var->variable = cur_fb_var->variable();
			cur_var->oldVariable = cur_fb_var->oldVariable();
		}

		tus_trans->result = not_an_error(fb_vars->size());
		tus_trans->wake_cond.notify_one();
	}

	void np_handler::handle_TusVariable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto tus_trans = idm::get_unlocked<tus_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(tus_trans);
		std::lock_guard lock(tus_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED);
		case rpcn::ErrorType::Unauthorized: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN);
		case rpcn::ErrorType::CondFail: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED);
		default: fmt::throw_exception("Unexpected error in handle_TusVariable: %d", static_cast<u8>(error));
		}

		const auto* fb_var = reply.get_flatbuffer<TusVariable>();
		ensure(!reply.is_error(), "Error parsing response in handle_TusVariable");

		const auto* tdata = std::get_if<tdata_tus_get_variable_generic>(&tus_trans->tdata);
		ensure(tdata);

		auto* var = tdata->outVariable.get_ptr();
		memset(var, 0, sizeof(SceNpTusVariable));

		ensure(fb_var->ownerId());
		string_to_npid(fb_var->ownerId()->string_view(), var->ownerId);

		if (fb_var->hasData())
		{
			ensure(fb_var->lastChangedAuthorId());
			var->hasData = 1;
			var->lastChangedDate.tick = fb_var->lastChangedDate();
			string_to_npid(fb_var->lastChangedAuthorId()->string_view(), var->lastChangedAuthorId);
			var->variable = fb_var->variable();
			var->oldVariable = fb_var->oldVariable();
		}

		tus_trans->result = CELL_OK;
		tus_trans->wake_cond.notify_one();
	}

	void np_handler::handle_TusDataStatusResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto tus_trans = idm::get_unlocked<tus_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(tus_trans);
		std::lock_guard lock(tus_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED);
		case rpcn::ErrorType::Unauthorized: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN);
		case rpcn::ErrorType::CondFail: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED);
		default: fmt::throw_exception("Unexpected error in handle_TusDataStatusResponse: %d", static_cast<u8>(error));
		}

		const auto* resp = reply.get_flatbuffer<TusDataStatusResponse>();
		ensure(!reply.is_error(), "Error parsing response in handle_TusDataStatusReponse");

		const auto* tdata = std::get_if<tdata_tus_get_datastatus_generic>(&tus_trans->tdata);
		ensure(tdata);
		ensure(resp->status() && resp->status()->size() <= static_cast<usz>(tdata->arrayNum));

		const auto* fb_status = resp->status();

		memset(tdata->statusArray.get_ptr(), 0, sizeof(SceNpTusDataStatus) * tdata->arrayNum);
		for (flatbuffers::uoffset_t i = 0; i < fb_status->size(); i++)
		{
			auto* cur_status = &tdata->statusArray[i];
			const auto* cur_fb_status = fb_status->Get(i);

			ensure(cur_fb_status->ownerId());
			string_to_npid(cur_fb_status->ownerId()->string_view(), cur_status->ownerId);

			if (!cur_fb_status->hasData())
			{
				continue;
			}

			ensure(cur_fb_status->lastChangedAuthorId());

			cur_status->hasData = 1;
			cur_status->lastChangedDate.tick = cur_fb_status->lastChangedDate();
			string_to_npid(cur_fb_status->lastChangedAuthorId()->string_view(), cur_status->lastChangedAuthorId);
			cur_status->info.infoSize = cur_fb_status->info() ? cur_fb_status->info()->size() : 0;
			for (flatbuffers::uoffset_t i = 0; i < cur_status->info.infoSize; i++)
			{
				cur_status->info.data[i] = cur_fb_status->info()->Get(i);
			}
		}

		tus_trans->result = not_an_error(fb_status->size());
		tus_trans->wake_cond.notify_one();
	}

	void np_handler::tus_set_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		get_rpcn()->tus_set_multislot_variable(req_id, trans_ctx->communicationId, targetNpId, slotIdArray, variableArray, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_set_multislot_variable(u32 req_id, rpcn::ErrorType error)
	{
		return handle_tus_no_data(req_id, error);
	}

	void np_handler::tus_get_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_variables_generic {
			.variableArray = variableArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_multislot_variable(req_id, trans_ctx->communicationId, targetNpId, slotIdArray, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_multislot_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusVarResponse(req_id, error, reply);
	}

	void np_handler::tus_get_multiuser_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, std::vector<SceNpOnlineId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_variables_generic {
			.variableArray = variableArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_multiuser_variable(req_id, trans_ctx->communicationId, targetNpIdArray, slotId, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_multiuser_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusVarResponse(req_id, error, reply);
	}

	void np_handler::tus_get_friends_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray,s32 arrayNum, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_variables_generic {
			.variableArray = variableArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_friends_variable(req_id, trans_ctx->communicationId, slotId, !!includeSelf, sortType, arrayNum);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_friends_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusVarResponse(req_id, error, reply);
	}

	void np_handler::tus_add_and_get_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, vm::ptr<SceNpTusAddAndGetVariableOptParam> option, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_variable_generic {
			.outVariable = outVariable,
		};

		get_rpcn()->tus_add_and_get_variable(req_id, trans_ctx->communicationId, targetNpId, slotId, inVariable, option, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_add_and_get_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusVariable(req_id, error, reply);
	}

	void np_handler::tus_try_and_set_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, vm::ptr<SceNpTusTryAndSetVariableOptParam> option, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_variable_generic {
			.outVariable = resultVariable,
		};

		get_rpcn()->tus_try_and_set_variable(req_id, trans_ctx->communicationId, targetNpId, slotId, opeType, variable, option, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_try_and_set_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusVariable(req_id, error, reply);
	}

	void np_handler::tus_delete_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		get_rpcn()->tus_delete_multislot_variable(req_id, trans_ctx->communicationId, targetNpId, slotIdArray, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_delete_multislot_variable(u32 req_id, rpcn::ErrorType error)
	{
		return handle_tus_no_data(req_id, error);
	}

	void np_handler::tus_set_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, vm::ptr<SceNpTusSetDataOptParam> option, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		auto* tdata = std::get_if<tdata_tus_set_data>(&trans_ctx->tdata);
		if (!tdata)
		{
			trans_ctx->tdata = tdata_tus_set_data{.tus_data_size = totalSize};
			tdata = std::get_if<tdata_tus_set_data>(&trans_ctx->tdata);
			tdata->tus_data.reserve(totalSize);
		}

		const u8* ptr = static_cast<const u8*>(data.get_ptr());

		std::copy(ptr, ptr + sendSize, std::back_inserter(tdata->tus_data));

		if (tdata->tus_data.size() == tdata->tus_data_size)
		{
			trans_ctx->result = std::nullopt;
			const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);
			get_rpcn()->tus_set_data(req_id, trans_ctx->communicationId, targetNpId, slotId, tdata->tus_data, info, option, vuser);
			transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
		}
		else
		{
			trans_ctx->result = CELL_OK;
		}
	}

	void np_handler::reply_tus_set_data(u32 req_id, rpcn::ErrorType error)
	{
		return handle_tus_no_data(req_id, error);
	}

	void np_handler::tus_get_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, vm::ptr<void> data, u32 recvSize, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		auto* tdata = std::get_if<tdata_tus_get_data>(&trans_ctx->tdata);
		if (!tdata)
		{
			trans_ctx->tdata = tdata_tus_get_data{.recvSize = recvSize, .dataStatus = dataStatus, .data = data};
			const u32 req_id = get_req_id(REQUEST_ID_HIGH::SCORE);
			get_rpcn()->tus_get_data(req_id, trans_ctx->communicationId, targetNpId, slotId, vuser);
			transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
			return;
		}

		// Check if the transaction has actually completed, otherwise adjust tdata parameters
		if (!trans_ctx->result)
		{
			tdata->recvSize = recvSize;
			tdata->dataStatus = dataStatus;
			tdata->data = data;
			return;
		}

		// If here the data has already been acquired and the client is just asking for part of it
		usz to_copy = std::min(tdata->tus_data.size(), static_cast<usz>(recvSize));
		std::memcpy(data.get_ptr(), tdata->tus_data.data(), to_copy);
		tdata->tus_data.erase(tdata->tus_data.begin(), tdata->tus_data.begin() + to_copy);
		trans_ctx->result = not_an_error(to_copy);
	}

	void np_handler::reply_tus_get_data(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		std::lock_guard lock_trans(mutex_async_transactions);
		if (!async_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return;
		}

		auto tus_trans = idm::get_unlocked<tus_transaction_ctx>(::at32(async_transactions, req_id)->idm_id);
		ensure(tus_trans);
		std::lock_guard lock(tus_trans->mutex);

		switch (error)
		{
		case rpcn::ErrorType::NoError: break;
		case rpcn::ErrorType::NotFound: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED);
		case rpcn::ErrorType::Unauthorized: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN);
		case rpcn::ErrorType::CondFail: return tus_trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED);
		default: fmt::throw_exception("Unexpected error in reply to TusGetData: %d", static_cast<u8>(error));
		}

		const auto* fb_data = reply.get_flatbuffer<TusData>();
		ensure(!reply.is_error(), "Error parsing response in reply_tus_get_data");

		auto* tdata = std::get_if<tdata_tus_get_data>(&tus_trans->tdata);
		ensure(tdata);

		const auto* fb_status = fb_data->status();
		ensure(fb_status && fb_status->ownerId());
		if (!fb_status)
			return; // Sanity check to make compiler happy

		auto* data_status = tdata->dataStatus.get_ptr();
		auto* data = static_cast<u8 *>(tdata->data.get_ptr());

		memset(data_status, 0, sizeof(SceNpTusDataStatus));
		string_to_npid(fb_status->ownerId()->string_view(), data_status->ownerId);

		if (fb_status->hasData())
		{
			data_status->hasData = 1;
			data_status->lastChangedDate.tick = fb_status->lastChangedDate();
			string_to_npid(fb_status->lastChangedAuthorId()->string_view(), data_status->lastChangedAuthorId);
			data_status->data = tdata->data;
			data_status->dataSize = fb_data->data() ? fb_data->data()->size() : 0;
			data_status->info.infoSize = fb_status->info() ? fb_status->info()->size() : 0;
			
			const u32 to_copy = std::min<u32>(data_status->dataSize, tdata->recvSize);
			for (flatbuffers::uoffset_t i = 0; i < to_copy; i++)
			{
				data[i] = fb_data->data()->Get(i);
			}
			const u32 bytes_left = data_status->dataSize - to_copy;
			tdata->tus_data.reserve(bytes_left);
			for (flatbuffers::uoffset_t i = to_copy; i < bytes_left; i++)
			{
				tdata->tus_data.push_back(fb_data->data()->Get(i));
			}

			for (flatbuffers::uoffset_t i = 0; i < data_status->info.infoSize; i++)
			{
				fb_status->info()->Get(i);
			}
			tus_trans->result = not_an_error(to_copy);
		}
		else
		{
			tus_trans->result = SCE_NP_COMMUNITY_SERVER_ERROR_USER_STORAGE_DATA_NOT_FOUND;
		}

		tus_trans->wake_cond.notify_one();
	}

	void np_handler::tus_get_multislot_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_datastatus_generic {
			.statusArray = statusArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_multislot_data_status(req_id, trans_ctx->communicationId, targetNpId, slotIdArray, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_multislot_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusDataStatusResponse(req_id, error, reply);
	}

	void np_handler::tus_get_multiuser_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, std::vector<SceNpOnlineId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_datastatus_generic {
			.statusArray = statusArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_multiuser_data_status(req_id, trans_ctx->communicationId, targetNpIdArray, slotId, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_multiuser_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusDataStatusResponse(req_id, error, reply);
	}

	void np_handler::tus_get_friends_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		trans_ctx->tdata = tdata_tus_get_datastatus_generic {
			.statusArray = statusArray,
			.arrayNum = arrayNum,
		};

		get_rpcn()->tus_get_friends_data_status(req_id, trans_ctx->communicationId, slotId, !!includeSelf, sortType, arrayNum);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_get_friends_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply)
	{
		return handle_TusDataStatusResponse(req_id, error, reply);
	}

	void np_handler::tus_delete_multislot_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		const u32 req_id = get_req_id(REQUEST_ID_HIGH::TUS);

		get_rpcn()->tus_delete_multislot_data(req_id, trans_ctx->communicationId, targetNpId, slotIdArray, arrayNum, vuser);
		transaction_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	void np_handler::reply_tus_delete_multislot_data(u32 req_id, rpcn::ErrorType error)
	{
		return handle_tus_no_data(req_id, error);
	}
} // namespace np
