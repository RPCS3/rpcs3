#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
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

		if (!get_rpcn()->get_server_list(get_req_id(0), get_match2_context(ctx_id)->communicationId, server_list))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return server_list;
	}

	u32 np_handler::get_server_status(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		// TODO: actually implement interaction with server for this?
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo);
		u32 event_key = get_event_key();

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
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext);

		const auto cb_info_opt = take_pending_request(req_id);
		ensure(cb_info_opt);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return req_id;
	}

	u32 np_handler::delete_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 /*server_id*/)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_DeleteServerContext);

		const auto cb_info_opt = take_pending_request(req_id);
		ensure(cb_info_opt);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return req_id;
	}

	u32 np_handler::get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList);

		if (!get_rpcn()->get_world_list(req_id, get_match2_context(ctx_id)->communicationId, server_id))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_world_list(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

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

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetWorldInfoList, sizeof(SceNpMatching2GetWorldInfoListResponse));
		auto* world_info = reinterpret_cast<SceNpMatching2GetWorldInfoListResponse*>(edata.data());
		world_info->worldNum = world_list.size();

		if (!world_list.empty())
		{
			auto* worlds = edata.allocate<SceNpMatching2World>(sizeof(SceNpMatching2World) * world_list.size(), world_info->world);
			for (usz i = 0; i < world_list.size(); i++)
			{
				worlds[i].worldId = world_list[i];
			}
		}

		np_memory.shrink_allocation(edata.addr(), edata.size());
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom);

		extra_nps::print_createjoinroom(req);

		if (!get_rpcn()->createjoin_room(req_id, get_match2_context(ctx_id)->communicationId, req))
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
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);
		auto* resp = reply.get_flatbuffer<RoomDataInternal>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to CreateRoom command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_CreateJoinRoom, sizeof(SceNpMatching2CreateJoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2CreateJoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);
		np_cache.update_password(room_resp->roomDataInternal->roomId, cached_cj_password);

		extra_nps::print_create_room_resp(room_resp);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom);

		extra_nps::print_joinroom(req);

		if (!get_rpcn()->join_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_join_room(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		s32 error_code = 0;

		if (rpcn::is_error(static_cast<rpcn::ErrorType>(reply_data[0])))
		{
			switch (reply_data[0])
			{
			case rpcn::ErrorType::RoomMissing: error_code = SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM; break;
			case rpcn::ErrorType::RoomAlreadyJoined: error_code = SCE_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED; break;
			case rpcn::ErrorType::RoomFull: error_code = SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL; break;
			default: return false;
			}
		}

		if (error_code != 0)
		{
			cb_info_opt->queue_callback(req_id, 0, error_code, 0);
			return true;
		}

		vec_stream reply(reply_data, 1);

		auto* resp = reply.get_flatbuffer<RoomDataInternal>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to JoinRoom command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_JoinRoom, sizeof(SceNpMatching2JoinRoomResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2JoinRoomResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_room_data_internal(room_info);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom);

		if (!get_rpcn()->leave_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_leave_room(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);
		u64 room_id = reply.get<u64>();
		if (reply.is_error())
			return error_and_disconnect("Malformed reply to LeaveRoom command");

		// Disconnect all users from that room
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh.disconnect_sig2_users(room_id);
		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return true;
	}

	u32 np_handler::search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom);

		extra_nps::print_search_room(req);

		if (!get_rpcn()->search_room(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_search_room(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);
		auto* resp = reply.get_flatbuffer<SearchRoomResponse>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to SearchRoom command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SearchRoom, sizeof(SceNpMatching2SearchRoomResponse));
		auto* search_resp = reinterpret_cast<SceNpMatching2SearchRoomResponse*>(edata.data());
		SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(edata, resp, search_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_search_room_resp(search_resp);
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::get_roomdata_external_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataExternalList);

		extra_nps::print_get_roomdata_external_list_req(req);

		if (!get_rpcn()->get_roomdata_external_list(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_roomdata_external_list(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);
		auto* resp = reply.get_flatbuffer<GetRoomDataExternalListResponse>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to GetRoomDataExternalList command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataExternalList, sizeof(SceNpMatching2GetRoomDataExternalListResponse));
		auto* sce_get_room_ext_resp = reinterpret_cast<SceNpMatching2GetRoomDataExternalListResponse*>(edata.data());
		GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(edata, resp, sce_get_room_ext_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		extra_nps::print_get_roomdata_external_list_resp(sce_get_room_ext_resp);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataExternal);

		extra_nps::print_set_roomdata_ext_req(req);

		if (!get_rpcn()->set_roomdata_external(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roomdata_external(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return true;
	}

	u32 np_handler::get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataInternal);

		if (!get_rpcn()->get_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_roomdata_internal(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);

		auto* resp = reply.get_flatbuffer<RoomDataInternal>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to GetRoomDataInternal command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataInternal, sizeof(SceNpMatching2GetRoomDataInternalResponse));
		auto* room_resp = reinterpret_cast<SceNpMatching2GetRoomDataInternalResponse*>(edata.data());
		auto* room_info = edata.allocate<SceNpMatching2RoomDataInternal>(sizeof(SceNpMatching2RoomDataInternal), room_resp->roomDataInternal);
		RoomDataInternal_to_SceNpMatching2RoomDataInternal(edata, resp, room_info, npid);
		np_memory.shrink_allocation(edata.addr(), edata.size());

		np_cache.insert_room(room_info);

		extra_nps::print_room_data_internal(room_info);

		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataInternal);

		// TODO: extra_nps::print_set_roomdata_req(req);

		extra_nps::print_set_roomdata_int_req(req);

		if (!get_rpcn()->set_roomdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roomdata_internal(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return true;
	}

	u32 np_handler::set_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomMemberDataInternal);

		extra_nps::print_set_roommemberdata_int_req(req);

		if (!get_rpcn()->set_roommemberdata_internal(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_set_roommemberdata_internal(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return true;
	}

	u32 np_handler::get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SignalingGetPingInfo);

		if (!get_rpcn()->ping_room_owner(req_id, get_match2_context(ctx_id)->communicationId, req->roomId))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_get_ping_info(u32 req_id, std::vector<u8>& reply_data)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		vec_stream reply(reply_data, 1);

		auto* resp = reply.get_flatbuffer<GetPingInfoResponse>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to PingRoomOwner command");

		u32 event_key = get_event_key();

		auto& edata = allocate_req_result(event_key, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SignalingGetPingInfo, sizeof(SceNpMatching2SignalingGetPingInfoResponse));
		auto* final_ping_resp = reinterpret_cast<SceNpMatching2SignalingGetPingInfoResponse*>(edata.data());
		GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(resp, final_ping_resp);
		np_memory.shrink_allocation(edata.addr(), edata.size());
		cb_info_opt->queue_callback(req_id, event_key, 0, edata.size());

		return true;
	}

	u32 np_handler::send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req)
	{
		u32 req_id = generate_callback_info(ctx_id, optParam, SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomMessage);

		if (!get_rpcn()->send_room_message(req_id, get_match2_context(ctx_id)->communicationId, req))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}

		return req_id;
	}

	bool np_handler::reply_send_room_message(u32 req_id, std::vector<u8>& /*reply_data*/)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return true;

		cb_info_opt->queue_callback(req_id, 0, 0, 0);

		return true;
	}

	void np_handler::req_sign_infos(const std::string& npid, u32 conn_id)
	{
		u32 req_id = get_req_id(0x3333);
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

	bool np_handler::reply_req_sign_infos(u32 req_id, std::vector<u8>& reply_data)
	{
		u32 conn_id;
		{
			std::lock_guard lock(mutex_pending_sign_infos_requests);
			conn_id = ::at32(pending_sign_infos_requests, req_id);
			pending_sign_infos_requests.erase(req_id);
		}

		if (rpcn::is_error(static_cast<rpcn::ErrorType>(reply_data[0])))
		{
			switch (reply_data[0])
			{
			case rpcn::ErrorType::NotFound:
			{
				rpcn_log.error("Signaling information was requested for a user that doesn't exist or is not online");
				return true;
			}
			case rpcn::ErrorType::Malformed:
				return error_and_disconnect("RequestSignalingInfos request was malformed!");
			default:
				return error_and_disconnect(fmt::format("RequestSignalingInfos failed with unknown error(%d)!", reply_data[0]));
			}
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

	bool np_handler::reply_req_ticket([[maybe_unused]] u32 req_id, std::vector<u8>& reply_data)
	{
		vec_stream reply(reply_data, 1);
		auto ticket_raw = reply.get_rawdata();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to RequestTicket command");

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

		auto& cpu_thread = *get_current_cpu_thread();
		lv2_obj::sleep(cpu_thread);
		worker_function(std::move(lock));
		cpu_thread.check_state();
	}

	void np_handler::get_board_infos(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		u32 req_id = get_req_id(0x3334);
		trans_ctx->tdata = tdata_get_board_infos{.boardInfo = boardInfo};
		get_rpcn()->get_board_infos(req_id, trans_ctx->communicationId, boardId);

		score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}

	bool np_handler::reply_get_board_infos(u32 req_id, std::vector<u8>& reply_data)
	{
		vec_stream reply(reply_data, 1);

		auto* resp = reply.get_flatbuffer<BoardInfo>();

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to GetBoardInfos command");

		SceNpScoreBoardInfo board_info;

		board_info.rankLimit = resp->rankLimit();
		board_info.updateMode = resp->updateMode();
		board_info.sortMode = resp->sortMode();
		board_info.uploadNumLimit = resp->uploadNumLimit();
		board_info.uploadSizeLimit = resp->uploadSizeLimit();

		std::lock_guard lock_trans(mutex_score_transactions);
		if (!score_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return false;
		}

		auto trans = ::at32(score_transactions, req_id);
		std::lock_guard lock(trans->mutex);

		const auto* tdata = std::get_if<tdata_get_board_infos>(&trans->tdata);
		ensure(tdata);

		memcpy(reinterpret_cast<u8*>(tdata->boardInfo.get_ptr()), &board_info, sizeof(SceNpScoreBoardInfo));
		trans->result = CELL_OK;
		trans->wake_cond.notify_one();

		return true;
	}

	void np_handler::record_score(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, const u8* data, u32 data_size, vm::ptr<SceNpScoreRankNumber> tmpRank, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		std::optional<std::string> str_comment = scoreComment ? std::optional(std::string(reinterpret_cast<const char*>(scoreComment->data))) : std::nullopt;
		std::optional<std::vector<u8>> vec_data;

		if (data)
		{
			vec_data = std::vector<u8>(data, data + data_size);
		}

		trans_ctx->tdata = tdata_record_score{.tmpRank = tmpRank};

		get_rpcn()->record_score(req_id, trans_ctx->communicationId, boardId, trans_ctx->pcId, score, str_comment, vec_data);

		score_async_handler(std::move(lock), trans_ctx, req_id, async);
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

		const auto* tdata = std::get_if<tdata_record_score>(&trans->tdata);
		ensure(tdata);

		if (tdata->tmpRank)
		{
			*tdata->tmpRank = tmp_rank;
		}

		trans->result = CELL_OK;
		trans->wake_cond.notify_one();
		return true;
	}

	void np_handler::record_score_data(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, const u8* score_data, bool async)
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
			u32 req_id = get_req_id(0x3334);
			get_rpcn()->record_score_data(req_id, trans_ctx->communicationId, trans_ctx->pcId, boardId, score, tdata->game_data);
			score_async_handler(std::move(lock), trans_ctx, req_id, async);
		}
		else
		{
			trans_ctx->result = CELL_OK;
		}
	}

	bool np_handler::reply_record_score_data(u32 req_id, std::vector<u8>& reply_data)
	{
		std::lock_guard lock_trans(mutex_score_transactions);
		if (!score_transactions.count(req_id))
		{
			rpcn_log.error("Couldn't find transaction(%d) in trans_id!", req_id);
			return false;
		}

		auto trans = ::at32(score_transactions, req_id);
		std::lock_guard lock(trans->mutex);

		auto set_result_and_wake = [&](error_code err) -> bool
		{
			trans->result = err;
			trans->wake_cond.notify_one();
			return true;
		};

		if (rpcn::is_error(static_cast<rpcn::ErrorType>(reply_data[0])))
		{
			switch (reply_data[0])
			{
			case rpcn::ErrorType::NotFound: return trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_STORE_NOT_FOUND);
			case rpcn::ErrorType::ScoreInvalid: return trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE);
			case rpcn::ErrorType::ScoreHasData: return trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_DATA_ALREADY_EXISTS);
			default: return false;
			}
		}

		return set_result_and_wake(CELL_OK);
	}

	void np_handler::get_score_data(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const SceNpId& npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> score_data, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);

		auto* tdata = std::get_if<tdata_get_score_data>(&trans_ctx->tdata);
		if (!tdata)
		{
			trans_ctx->tdata = tdata_get_score_data{.totalSize = totalSize, .recvSize = recvSize, .score_data = score_data};

			u32 req_id = get_req_id(0x3334);
			get_rpcn()->get_score_data(req_id, trans_ctx->communicationId, trans_ctx->pcId, boardId, npId);
			score_async_handler(std::move(lock), trans_ctx, req_id, async);
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

	bool np_handler::reply_get_score_data(u32 req_id, std::vector<u8>& reply_data)
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
			case rpcn::ErrorType::NotFound: return trans->set_result_and_wake(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_GAME_DATA_MASTER_NOT_FOUND);
			default: return false;
			}
		}

		vec_stream reply(reply_data, 1);

		auto* tdata = std::get_if<tdata_get_score_data>(&trans->tdata);
		ensure(tdata);

		tdata->game_data = reply.get_rawdata();

		if (reply.is_error())
		{
			rpcn_log.error("Error parsing response in reply_get_score_data");
			return false;
		}

		tdata->game_data_size = tdata->game_data.size();

		usz to_copy = std::min(tdata->game_data.size(), static_cast<usz>(tdata->recvSize));
		std::memcpy(tdata->score_data.get_ptr(), tdata->game_data.data(), to_copy);
		tdata->game_data.erase(tdata->game_data.begin(), tdata->game_data.begin() + to_copy);
		*tdata->totalSize = tdata->game_data_size;

		return trans->set_result_and_wake(not_an_error(to_copy));
	}

	void np_handler::get_score_range(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);

		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_range(req_id, trans_ctx->communicationId, boardId, startSerialRank, arrayNum, with_comments, with_gameinfo);

		score_async_handler(std::move(lock), trans_ctx, req_id, async);
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
		auto* resp = reply.get_flatbuffer<GetScoreResponse>();

		if (reply.is_error())
		{
			rpcn_log.error("Error parsing response in handle_GetScoreResponse");
			return false;
		}

		const auto* tdata = std::get_if<tdata_get_score_generic>(&trans_ctx->tdata);
		ensure(tdata);
		ensure(resp->rankArray() && resp->rankArray()->size() <= tdata->arrayNum);

		memset(tdata->rankArray.get_ptr(), 0, tdata->rankArraySize);
		auto* fb_rankarray = resp->rankArray();

		vm::ptr<SceNpScoreRankData> rankArray = vm::static_ptr_cast<SceNpScoreRankData>(tdata->rankArray);
		vm::ptr<SceNpScorePlayerRankData> rankPlayerArray = vm::static_ptr_cast<SceNpScorePlayerRankData>(tdata->rankArray);

		for (flatbuffers::uoffset_t i = 0; i < fb_rankarray->size(); i++)
		{
			const auto* fb_rankdata = fb_rankarray->Get(i);
			ensure(fb_rankdata->npId() && fb_rankdata->onlineName());

			if (fb_rankdata->recordDate() == 0)
				continue;

			SceNpScoreRankData* cur_rank;
			if (tdata->rankArraySize == (tdata->arrayNum * sizeof(SceNpScoreRankData)))
			{
				cur_rank = &rankArray[i];
			}
			else
			{
				rankPlayerArray[i].hasData = 1;
				cur_rank = &rankPlayerArray[i].rankData;
			}

			string_to_npid(fb_rankdata->npId()->string_view(), cur_rank->npId);
			string_to_online_name(fb_rankdata->onlineName()->string_view(), cur_rank->onlineName);

			cur_rank->pcId = fb_rankdata->pcId();
			cur_rank->serialRank = fb_rankdata->rank();
			cur_rank->rank = fb_rankdata->rank();
			cur_rank->highestRank = fb_rankdata->rank();
			cur_rank->scoreValue = fb_rankdata->score();
			cur_rank->hasGameData = fb_rankdata->hasGameData();
			cur_rank->recordDate.tick = fb_rankdata->recordDate();
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

	void np_handler::get_score_friend(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, bool include_self, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_friend(req_id, trans_ctx->communicationId, boardId, include_self, with_comments, with_gameinfo, arrayNum);

		score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	bool np_handler::reply_get_score_friends(u32 req_id, std::vector<u8>& reply_data)
	{
		return handle_GetScoreResponse(req_id, reply_data);
	}

	void np_handler::get_score_npid(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const std::vector<std::pair<SceNpId, s32>>& npid_vec, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, [[maybe_unused]] u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async)
	{
		std::unique_lock lock(trans_ctx->mutex);
		u32 req_id = get_req_id(0x3334);
		trans_ctx->tdata = tdata_get_score_generic{
			.rankArray = rankArray,
			.rankArraySize = rankArraySize,
			.commentArray = commentArray,
			.infoArray = infoArray,
			.infoArraySize = infoArraySize,
			.arrayNum = arrayNum,
			.lastSortDate = lastSortDate,
			.totalRecord = totalRecord,
		};

		bool with_comments = !!commentArray;
		bool with_gameinfo = !!infoArray;

		get_rpcn()->get_score_npid(req_id, trans_ctx->communicationId, boardId, npid_vec, with_comments, with_gameinfo);

		score_async_handler(std::move(lock), trans_ctx, req_id, async);
	}
	bool np_handler::reply_get_score_npid(u32 req_id, std::vector<u8>& reply_data)
	{
		return handle_GetScoreResponse(req_id, reply_data);
	}

} // namespace np
