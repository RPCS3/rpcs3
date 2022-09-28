#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/IdManager.h"
#include "np_handler.h"
#include "np_contexts.h"
#include "np_helpers.h"
#include "np_structs_extra.h"

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

		if (!rpcn->get_server_list(get_req_id(0), get_match2_context(ctx_id)->communicationId, server_list))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return server_list;
	}

	u32 np_handler::get_server_status(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		// TODO: actually implement interaction with server for this?
		u32 req_id    = generate_callback_info(ctx_id, optParam);
		u32 event_key = get_event_key();

		auto& edata                                    = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetServerInfo, sizeof(SceNpMatching2GetServerInfoResponse));
		SceNpMatching2GetServerInfoResponse* serv_info = reinterpret_cast<SceNpMatching2GetServerInfoResponse*>(edata.data());
		serv_info->server.serverId                     = server_id;
		serv_info->server.status                       = SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE;
		np_memory.shrink_allocation(edata.addr(), edata.size());

		const auto cb_info = take_pending_request(req_id);

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return req_id;
	}

	u32 np_handler::create_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 /*server_id*/)
	{
		u32 req_id    = generate_callback_info(ctx_id, optParam);
		u32 event_key = get_event_key();

		const auto cb_info = take_pending_request(req_id);

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return req_id;
	}

	u32 np_handler::delete_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 /*server_id*/)
	{
		u32 req_id    = generate_callback_info(ctx_id, optParam);
		u32 event_key = get_event_key();

		const auto cb_info = take_pending_request(req_id);

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_DeleteServerContext, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return req_id;
	}

	u32 np_handler::get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		if (!rpcn->get_world_list(req_id, get_match2_context(ctx_id)->communicationId, server_id))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_world_list(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);

		std::vector<u32> world_list;
		u32 num_worlds = reply.get<u32>();
		for (u32 i = 0; i < num_worlds; i++)
		{
			world_list.push_back(reply.get<u32>());
		}

		if (reply.is_error())
		{
			return error_and_disconnect("Malformed reply to GetWorldList command");
		}

		u32 event_key = get_event_key();

		auto& edata          = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetWorldInfoList, sizeof(SceNpMatching2GetWorldInfoListResponse));
		auto* world_info     = reinterpret_cast<SceNpMatching2GetWorldInfoListResponse*>(edata.data());
		world_info->worldNum = world_list.size();

		if (!world_list.empty())
		{
			auto* worlds = edata.allocate<SceNpMatching2World>(sizeof(SceNpMatching2World) * world_list.size(), world_info->world);
			for (usz i = 0; i < world_list.size(); i++)
			{
				worlds[i].worldId                  = world_list[i];
				worlds[i].numOfLobby               = 1; // TODO
				worlds[i].maxNumOfTotalLobbyMember = 10000;
				worlds[i].curNumOfTotalLobbyMember = 1;
				worlds[i].curNumOfRoom             = 1;
				worlds[i].curNumOfTotalRoomMember  = 1;
			}
		}

		np_memory.shrink_allocation(edata.addr(), edata.size());

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_createjoinroom(req);

		if (!rpcn->createjoin_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		// More elegant solution would be to send back password with creation reply
		cached_cj_password = req->roomPassword ? std::optional<SceNpMatching2SessionPassword>{*req->roomPassword} : std::nullopt;

		return req_id;
	}

	bool np_handler::reply_create_join_room(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);
		auto create_room_resp = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to CreateRoom command");

		u32 event_key = get_event_key();

		auto* resp      = flatbuffers::GetRoot<RoomDataInternal>(create_room_resp.data());
		auto& edata     = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_CreateJoinRoom, sizeof(SceNpMatching2CreateJoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2CreateJoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);
		np_cache.update_password(room_resp->roomDataInternal->roomId, cached_cj_password);

		// Establish Matching2 self signaling info
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.set_self_sig2_info(room_info->roomId, 1);
		sigh.set_sig2_infos(room_info->roomId, 1, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE, rpcn->get_addr_sig(), rpcn->get_port_sig(), get_npid(), true);
		// TODO? Should this send a message to Signaling CB? Is this even necessary?

		extra_nps::print_create_room_resp(room_resp);

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_joinroom(req);

		if (!rpcn->join_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_join_room(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);

		auto join_room_resp = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to JoinRoom command");

		u32 event_key = get_event_key();

		auto resp       = flatbuffers::GetRoot<RoomDataInternal>(join_room_resp.data());
		auto& edata     = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_JoinRoom, sizeof(SceNpMatching2JoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2JoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		u16 member_id   = RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_room_data_internal(room_info);

		// Establish Matching2 self signaling info
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.set_self_sig2_info(room_info->roomId, member_id);
		sigh.set_sig2_infos(room_info->roomId, member_id, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE, rpcn->get_addr_sig(), rpcn->get_port_sig(), get_npid(), true);
		// TODO? Should this send a message to Signaling CB? Is this even necessary?

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		if (!rpcn->leave_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_leave_room(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);
		u64 room_id = reply.get<u64>();
		if (reply.is_error())
			return error_and_disconnect("Malformed reply to LeaveRoom command");

		u32 event_key = get_event_key(); // Unsure if necessary if there is no data

		// Disconnect all users from that room
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.disconnect_sig2_users(room_id);

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_search_room(req);

		if (!rpcn->search_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_search_room(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);
		auto search_room_resp = reply.get_rawdata();
		if (reply.is_error())
			return error_and_disconnect("Malformed reply to SearchRoom command");

		u32 event_key = get_event_key();

		auto* resp        = flatbuffers::GetRoot<SearchRoomResponse>(search_room_resp.data());
		auto& edata       = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SearchRoom, sizeof(SceNpMatching2SearchRoomResponse));
		auto* search_resp = reinterpret_cast<SceNpMatching2SearchRoomResponse*>(edata.data());
		SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(edata, resp, search_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_search_room_resp(search_resp);

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::get_roomdata_external_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_get_roomdata_external_list_req(req);

		if (!rpcn->get_roomdata_external_list(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_roomdata_external_list(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);
		auto get_room_ext_resp = reply.get_rawdata();
		if (reply.is_error())
			return error_and_disconnect("Malformed reply to GetRoomDataExternalList command");

		u32 event_key = get_event_key();

		auto* resp                  = flatbuffers::GetRoot<GetRoomDataExternalListResponse>(get_room_ext_resp.data());
		auto& edata                 = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataExternalList, sizeof(SceNpMatching2GetRoomDataExternalListResponse));
		auto* sce_get_room_ext_resp = reinterpret_cast<SceNpMatching2GetRoomDataExternalListResponse*>(edata.data());
		GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(edata, resp, sce_get_room_ext_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_get_roomdata_external_list_resp(sce_get_room_ext_resp);

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataExternalList, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_set_roomdata_ext_req(req);

		if (!rpcn->set_roomdata_external(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roomdata_external(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		const auto cb_info = take_pending_request(req_id);

		u32 event_key = get_event_key(); // Unsure if necessary if there is no data

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataExternal, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		if (!rpcn->get_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_roomdata_internal(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);

		auto internal_data = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to GetRoomDataInternal command");

		u32 event_key = get_event_key();

		auto* resp      = flatbuffers::GetRoot<RoomDataInternal>(internal_data.data());
		auto& edata     = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataInternal, sizeof(SceNpMatching2GetRoomDataInternalResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2GetRoomDataInternalResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_room_data_internal(room_info);

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataInternal, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		// TODO: extra_nps::print_set_roomdata_req(req);

		extra_nps::print_set_roomdata_int_req(req);

		if (!rpcn->set_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roomdata_internal(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		const auto cb_info = take_pending_request(req_id);

		u32 event_key = get_event_key(); // Unsure if necessary if there is no data

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataInternal, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::set_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		extra_nps::print_set_roommemberdata_int_req(req);

		if (!rpcn->set_roommemberdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roommemberdata_internal(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		const auto cb_info = take_pending_request(req_id);

		u32 event_key = get_event_key(); // Unsure if necessary if there is no data

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomMemberDataInternal, event_key, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		if (!rpcn->ping_room_owner(req_id, get_match2_context(ctx_id)->communicationId, req->roomId))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_ping_info(u32 req_id, std::vector<u8>& reply_data)
	{
		const auto cb_info = take_pending_request(req_id);

		vec_stream reply(reply_data, 1);

		auto ping_resp = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to PingRoomOwner command");

		u32 event_key = get_event_key();

		auto* resp            = flatbuffers::GetRoot<GetPingInfoResponse>(ping_resp.data());
		auto& edata           = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SignalingGetPingInfo, sizeof(SceNpMatching2SignalingGetPingInfoResponse));
		auto* final_ping_resp = reinterpret_cast<SceNpMatching2SignalingGetPingInfoResponse*>(edata.data());
		GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(resp, final_ping_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		if (cb_info.cb)
		{
			sysutil_register_cb([=, size = edata.size()](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SignalingGetPingInfo, event_key, 0, size, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	u32 np_handler::send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam);

		if (!rpcn->send_room_message(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_send_room_message(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		const auto cb_info = take_pending_request(req_id);

		if (cb_info.cb)
		{
			sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
				{
					cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomMessage, 0, 0, 0, cb_info.cb_arg);
					return 0;
				});
		}

		return true;
	}

	void np_handler::req_sign_infos(const std::string& npid, u32 conn_id)
	{
		u32 req_id = get_req_id(0x3333);
		{
			std::lock_guard lock(mutex_pending_sign_infos_requests);
			pending_sign_infos_requests[req_id] = conn_id;
		}

		if (!rpcn->req_sign_infos(req_id, npid))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return;
	}

	bool np_handler::reply_req_sign_infos(u32 req_id, std::vector<u8>& reply_data)
	{
		u32 conn_id;
		{
			std::lock_guard lock(mutex_pending_sign_infos_requests);
			conn_id = ::at32(pending_sign_infos_requests, req_id);
			pending_sign_infos_requests.erase(req_id);
		}

		vec_stream reply(reply_data, 1);
		u32 addr = reply.get<u32>();
		u16 port = reply.get<u16>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to RequestSignalingInfos command");

		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.start_sig(conn_id, addr, port);

		return true;
	}

	void np_handler::req_ticket([[maybe_unused]] u32 version, [[maybe_unused]] const SceNpId* npid, const char* service_id, const u8* cookie, u32 cookie_size, [[maybe_unused]] const char* entitlement_id, [[maybe_unused]] u32 consumed_count)
	{
		u32 req_id = get_req_id(0x3333);

		std::string service_id_str(service_id);

		std::vector<u8> cookie_vec;
		if (cookie && cookie_size)
		{
			cookie_vec.assign(cookie, cookie + cookie_size);
		}

		if (!rpcn->req_ticket(req_id, service_id_str, cookie_vec))
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

	bool np_handler::reply_req_ticket([[maybe_unused]] u32 req_id, std::vector<u8>& reply_data)
	{
		vec_stream reply(reply_data, 1);
		auto ticket_raw = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to RequestTicket command");

		current_ticket   = ticket(std::move(ticket_raw));
		auto ticket_size = static_cast<s32>(current_ticket.size());

		if (manager_cb)
		{
			sysutil_register_cb([manager_cb = this->manager_cb, ticket_size, manager_cb_arg = this->manager_cb_arg](ppu_thread& cb_ppu) -> s32
				{
					manager_cb(cb_ppu, SCE_NP_MANAGER_EVENT_GOT_TICKET, ticket_size, manager_cb_arg);
					return 0;
				});
		}

		return true;
	}

	void np_handler::score_async_handler(std::unique_lock<shared_mutex> lock, const std::shared_ptr<score_transaction_ctx>& trans_ctx, u32 req_id, bool async)
	{
		auto worker_function = [trans_ctx = trans_ctx, req_id, this](std::unique_lock<shared_mutex> lock)
		{
			auto res = trans_ctx->wake_cond.wait_for(lock, std::chrono::microseconds(trans_ctx->timeout));
			{
				std::lock_guard lock_threads(this->mutex_score_transactions);
				this->score_transactions.erase(req_id);
			}

			if (res == std::cv_status::timeout)
			{
				trans_ctx->result = SCE_NP_COMMUNITY_ERROR_TIMEOUT;
				return;
			}

			// Only 2 cases should be timeout or caller setting result
			ensure(trans_ctx->result);

			trans_ctx->completion_cond.notify_one();
		};

		{
			std::lock_guard lock_score(mutex_score_transactions);
			ensure(score_transactions.insert({req_id, trans_ctx}).second);
		}

		if (async)
		{
			trans_ctx->thread = std::thread(worker_function, std::move(lock));
			return;
		}

		return worker_function(std::move(lock));
	}

	void np_handler::get_board_infos(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId board_id, vm::ptr<SceNpScoreBoardInfo> board_info, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		u32 req_id = get_req_id(0x3334);
		std::string comm_id(static_cast<const char*>(trans_ctx->communicationId.data));

		trans_ctx->data.clear();
		trans_ctx->data.push_back(board_info.addr());
		rpcn->get_board_infos(req_id, trans_ctx->communicationId, board_id);

		return score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	bool np_handler::reply_get_board_infos(u32 req_id, std::vector<u8>& reply_data)
	{
		vec_stream reply(reply_data, 1);

		auto raw_board_info = reply.get_rawdata();
		auto* resp          = flatbuffers::GetRoot<BoardInfo>(raw_board_info.data());

		SceNpScoreBoardInfo board_info;

		board_info.rankLimit       = resp->rankLimit();
		board_info.updateMode      = resp->updateMode();
		board_info.sortMode        = resp->sortMode();
		board_info.uploadNumLimit  = resp->uploadNumLimit();
		board_info.uploadSizeLimit = resp->uploadSizeLimit();

		std::lock_guard lock_trans(mutex_score_transactions);
		if (!score_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return false;
		}

		auto trans = ::at32(score_transactions, req_id);
		std::lock_guard lock(trans->mutex);
		ensure(trans->data.size() == 1);
		vm::ptr<SceNpScoreBoardInfo> boardinfo_ptr = vm::cast(trans->data[0]);
		memcpy(reinterpret_cast<u8*>(boardinfo_ptr.get_ptr()), &board_info, sizeof(SceNpScoreBoardInfo));
		trans->result = CELL_OK;
		trans->wake_cond.notify_one();

		return true;
	}

	void np_handler::record_score(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId board_id, SceNpScoreValue score, vm::cptr<SceNpScoreComment> comment, const u8* data, u32 data_size, vm::ptr<SceNpScoreRankNumber> tmp_rank, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id                              = get_req_id(0x3334);
		std::optional<std::string> str_comment  = comment ? std::optional(std::string(reinterpret_cast<const char*>(comment->data))) : std::nullopt;
		std::optional<std::vector<u8>> vec_data = data ? std::optional(std::vector(data, data + data_size)) : std::nullopt;
		trans_ctx->data.clear();
		if (tmp_rank)
		{
			trans_ctx->data.push_back(tmp_rank.addr());
		}

		rpcn->record_score(req_id, trans_ctx->communicationId, board_id, trans_ctx->pcId, score, str_comment, vec_data);

		return score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	bool np_handler::reply_record_score(u32 req_id, std::vector<u8>& reply_data)
	{
		std::lock_guard lock_trans(mutex_score_transactions);
		if (!score_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return false;
		}

		auto trans = ::at32(score_transactions, req_id);
		std::lock_guard lock(trans->mutex);

		if (rpcn::is_error(static_cast<rpcn::ErrorType>(reply_data[0])))
		{
			switch (reply_data[0])
			{
			case rpcn::ErrorType::ScoreNotBest:
			{
				trans->result = SCE_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE;
				trans->wake_cond.notify_one();
				return true;
			}
			default: return false;
			}
		}

		vec_stream reply(reply_data, 1);
		auto tmp_rank = reply.get<u32>();

		if (reply.is_error())
		{
			rpcn_log.error("Error parsing response in reply_record_score");
			return false;
		}

		ensure(trans->data.size() == 1 || trans->data.size() == 0);
		if (!trans->data.empty())
		{
			vm::ptr<u32> tmprank_ptr = vm::cast(trans->data[0]);
			*tmprank_ptr             = tmp_rank;
		}

		trans->result = CELL_OK;
		trans->wake_cond.notify_one();
		return true;
	}

	bool np_handler::reply_store_score_data(u32 /* req_id */, std::vector<u8>& /* reply_data */)
	{
		return true;
	}
	bool np_handler::reply_get_score_data(u32 /* req_id */, std::vector<u8>& /* reply_data */)
	{
		return true;
	}

	void np_handler::get_score_range(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, [[maybe_unused]] u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		trans_ctx->data.clear();
		trans_ctx->data.push_back(static_cast<u32>(arrayNum));
		trans_ctx->data.push_back(rankArray.addr());
		trans_ctx->data.push_back(commentArray.addr());
		trans_ctx->data.push_back(infoArray.addr());
		trans_ctx->data.push_back(static_cast<u32>((arrayNum * sizeof(SceNpScoreGameInfo)) == infoArraySize));
		trans_ctx->data.push_back(lastSortDate.addr());
		trans_ctx->data.push_back(totalRecord.addr());

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		rpcn->get_score_range(req_id, trans_ctx->communicationId, boardId, startSerialRank, arrayNum, with_comments, with_gameinfo);

		return score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	bool np_handler::handle_GetScoreResponse(u32 req_id, std::vector<u8>& reply_data)
	{
		std::lock_guard lock_trans(mutex_score_transactions);
		if (!score_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return false;
		}

		auto trans_ctx = ::at32(score_transactions, req_id);
		std::lock_guard lock(trans_ctx->mutex);

		if (rpcn::is_error(static_cast<rpcn::ErrorType>(reply_data[0])))
		{
			return false;
		}

		vec_stream reply(reply_data, 1);
		auto raw_getscore_response = reply.get_rawdata();
		auto* resp                 = flatbuffers::GetRoot<GetScoreResponse>(raw_getscore_response.data());
		flatbuffers::Verifier verifier(raw_getscore_response.data(), raw_getscore_response.size());

		if (reply.is_error() || !resp->Verify(verifier))
		{
			rpcn_log.error("Error parsing response in reply_record_score");
			return false;
		}

		ensure(trans_ctx->data.size() == 7);
		u32 arrayNum                              = trans_ctx->data[0];
		vm::ptr<SceNpScoreRankData> rankArray     = vm::cast(trans_ctx->data[1]);
		vm::ptr<SceNpScoreComment> commentArray   = vm::cast(trans_ctx->data[2]);
		vm::ptr<void> infoArray                   = vm::cast(trans_ctx->data[3]);
		bool info_array_is_SceNpScoreGameInfo     = trans_ctx->data[4];
		vm::ptr<CellRtcTick> lastSortDate         = vm::cast(trans_ctx->data[5]);
		vm::ptr<SceNpScoreRankNumber> totalRecord = vm::cast(trans_ctx->data[6]);

		ensure(resp->rankArray() && resp->rankArray()->size() <= arrayNum);

		memset(rankArray.get_ptr(), 0, sizeof(SceNpScoreRankData) * arrayNum);
		auto* fb_rankarray = resp->rankArray();
		for (flatbuffers::uoffset_t i = 0; i < fb_rankarray->size(); i++)
		{
			const auto* fb_rankdata = fb_rankarray->Get(i);
			ensure(fb_rankdata->npId() && fb_rankdata->onlineName());
			string_to_npid(fb_rankdata->npId()->string_view(), &rankArray[i].npId);
			string_to_online_name(fb_rankdata->onlineName()->string_view(), &rankArray[i].onlineName);
			rankArray[i].pcId            = fb_rankdata->pcId();
			rankArray[i].serialRank      = fb_rankdata->rank();
			rankArray[i].rank            = fb_rankdata->rank();
			rankArray[i].highestRank     = fb_rankdata->rank();
			rankArray[i].scoreValue      = fb_rankdata->score();
			rankArray[i].hasGameData     = fb_rankdata->hasGameData();
			rankArray[i].recordDate.tick = fb_rankdata->recordDate();
		}

		if (commentArray)
		{
			ensure(resp->commentArray() && resp->commentArray()->size() <= arrayNum);
			memset(commentArray.get_ptr(), 0, sizeof(SceNpScoreComment) * arrayNum);

			auto* fb_commentarray = resp->commentArray();
			for (flatbuffers::uoffset_t i = 0; i < fb_commentarray->size(); i++)
			{
				const auto* fb_comment = fb_commentarray->Get(i);
				strcpy_trunc(commentArray[i].data, fb_comment->string_view());
			}
		}

		if (infoArray)
		{
			ensure(resp->infoArray() && resp->infoArray()->size() <= arrayNum);
			auto* fb_infoarray = resp->infoArray();

			if (info_array_is_SceNpScoreGameInfo)
			{
				vm::ptr<SceNpScoreGameInfo> ptr_gameinfo = vm::static_ptr_cast<SceNpScoreGameInfo>(infoArray);
				memset(ptr_gameinfo.get_ptr(), 0, sizeof(SceNpScoreGameInfo) * arrayNum);
				for (flatbuffers::uoffset_t i = 0; i < fb_infoarray->size(); i++)
				{
					const auto* fb_info = fb_infoarray->Get(i);
					ensure(fb_info->data()->size() <= SCE_NP_SCORE_GAMEINFO_SIZE);
					memcpy(ptr_gameinfo[i].nativeData, fb_info->data()->data(), fb_info->data()->size());
				}
			}
			else
			{
				vm::ptr<SceNpScoreVariableSizeGameInfo> ptr_vargameinfo = vm::static_ptr_cast<SceNpScoreVariableSizeGameInfo>(infoArray);
				memset(ptr_vargameinfo.get_ptr(), 0, sizeof(SceNpScoreVariableSizeGameInfo) * arrayNum);
				for (flatbuffers::uoffset_t i = 0; i < fb_infoarray->size(); i++)
				{
					const auto* fb_info = fb_infoarray->Get(i);
					ensure(fb_info->data()->size() <= SCE_NP_SCORE_VARIABLE_SIZE_GAMEINFO_MAXSIZE);
					ptr_vargameinfo[i].infoSize = fb_info->data()->size();
					memcpy(ptr_vargameinfo[i].data, fb_info->data(), fb_info->data()->size());
				}
			}
		}

		lastSortDate->tick = resp->lastSortDate();
		*totalRecord       = resp->totalRecord();

		if (fb_rankarray->size())
			trans_ctx->result = not_an_error(fb_rankarray->size());
		else
			trans_ctx->result = SCE_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND;

		trans_ctx->wake_cond.notify_one();
		return true;
	}
	bool np_handler::reply_get_score_range(u32 req_id, std::vector<u8>& reply_data)
	{
		return handle_GetScoreResponse(req_id, reply_data);
	}

	void np_handler::get_score_friend(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, bool include_self, vm::ptr<SceNpScoreRankData> rankArray, [[maybe_unused]] u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		trans_ctx->data.clear();
		trans_ctx->data.push_back(static_cast<u32>(arrayNum));
		trans_ctx->data.push_back(rankArray.addr());
		trans_ctx->data.push_back(commentArray.addr());
		trans_ctx->data.push_back(infoArray.addr());
		trans_ctx->data.push_back(static_cast<u32>((arrayNum * sizeof(SceNpScoreGameInfo)) == infoArraySize));
		trans_ctx->data.push_back(lastSortDate.addr());
		trans_ctx->data.push_back(totalRecord.addr());

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		rpcn->get_score_friend(req_id, trans_ctx->communicationId, boardId, include_self, with_comments, with_gameinfo, arrayNum);

		return score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	bool np_handler::reply_get_score_friends(u32 req_id, std::vector<u8>& reply_data)
	{
		return handle_GetScoreResponse(req_id, reply_data);
	}

	void np_handler::get_score_npid(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const std::vector<std::pair<SceNpId, s32>>& npid_vec, vm::ptr<SceNpScorePlayerRankData> rankArray, [[maybe_unused]] u64 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u64 commentArraySize, vm::ptr<void> infoArray, u64 infoArraySize, u64 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		trans_ctx->data.clear();
		trans_ctx->data.push_back(static_cast<u32>(arrayNum));
		trans_ctx->data.push_back(rankArray.addr());
		trans_ctx->data.push_back(commentArray.addr());
		trans_ctx->data.push_back(infoArray.addr());
		trans_ctx->data.push_back(static_cast<u32>((arrayNum * sizeof(SceNpScoreGameInfo)) == infoArraySize));
		trans_ctx->data.push_back(lastSortDate.addr());
		trans_ctx->data.push_back(totalRecord.addr());

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		rpcn->get_score_npid(req_id, trans_ctx->communicationId, boardId, npid_vec, with_comments, with_gameinfo);

		return score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	bool np_handler::reply_get_score_npid(u32 req_id, std::vector<u8>& reply_data)
	{
		return handle_GetScoreResponse(req_id, reply_data);
	}

} // namespace np
