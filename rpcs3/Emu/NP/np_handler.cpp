#include "stdafx.h"
#include <thread>
#include "Emu/system_config.h"
#include "np_handler.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Utilities/StrUtil.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/IdManager.h"
#include "np_structs_extra.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNp);

LOG_CHANNEL(rpcn_log, "rpcn");
LOG_CHANNEL(nph_log, "NPHandler");

np_handler::np_handler()
{
	g_cfg_rpcn.load();

	is_connected  = (g_cfg.net.net_active == np_internet_status::enabled);
	is_psn_active = (g_cfg.net.psn_status >= np_psn_status::fake);

	if (get_net_status() == CELL_NET_CTL_STATE_IPObtained)
	{
		if (!discover_ip_address())
		{
			nph_log.error("Failed to discover local IP!");
			is_connected = false;
			is_psn_active = false;
		}

		// Convert dns address
		// TODO: recover actual user dns through OS specific API
		in_addr conv{};
		if (!inet_pton(AF_INET, g_cfg.net.dns.to_string().c_str(), &conv))
		{
			// Do not set to disconnected on invalid IP just error and continue using default(google's 8.8.8.8)
			nph_log.error("Provided IP(%s) address for DNS is invalid!", g_cfg.net.dns.to_string());
		}
		else
		{
			dns_ip = conv.s_addr;
		}

		// Init switch map for dns
		auto swaps = fmt::split(g_cfg.net.swap_list.to_string(), {"&&"});
		for (std::size_t i = 0; i < swaps.size(); i++)
		{
			auto host_and_ip = fmt::split(swaps[i], {"="});
			if (host_and_ip.size() != 2)
			{
				nph_log.error("Pattern <%s> contains more than one '='", swaps[i]);
				continue;
			}

			in_addr conv;
			if (!inet_pton(AF_INET, host_and_ip[1].c_str(), &conv))
			{
				nph_log.error("IP(%s) provided for %s in the switch list is invalid!", host_and_ip[1], host_and_ip[0]);
			}
			else
			{
				switch_map[host_and_ip[0]] = conv.s_addr;
			}
		}
	}
}

bool np_handler::discover_ip_address()
{
	std::array<char, 1024> hostname;
	
	if (gethostname(hostname.data(), hostname.size()) == -1)
	{
		nph_log.error("gethostname failed in IP discovery!");
		return false;
	}
	
	hostent *host = gethostbyname(hostname.data());
	if (!host)
	{
		nph_log.error("gethostbyname failed in IP discovery!");
		return false;
	}

	if (host->h_addrtype != AF_INET)
	{
		nph_log.error("Could only find IPv6 addresses for current host!");
		return false;
	}

	// First address is used for now, (TODO combobox with possible local addresses to use?)
	local_ip_addr = *reinterpret_cast<u32 *>(host->h_addr_list[0]);

	// Set public address to local discovered address for now, may be updated later;
	public_ip_addr = local_ip_addr;

	return true;
}

u32 np_handler::get_local_ip_addr() const
{
	return local_ip_addr;
}

u32 np_handler::get_public_ip_addr() const
{
	return public_ip_addr;
}

u32 np_handler::get_dns_ip() const
{
	return dns_ip;
}

s32 np_handler::get_net_status() const
{
	return is_connected ? CELL_NET_CTL_STATE_IPObtained : CELL_NET_CTL_STATE_Disconnected;
}

s32 np_handler::get_psn_status() const
{
	return is_psn_active ? SCE_NP_MANAGER_STATUS_ONLINE : SCE_NP_MANAGER_STATUS_OFFLINE;
}

const SceNpId& np_handler::get_npid() const
{
	return npid;
}

const SceNpOnlineId& np_handler::get_online_id() const
{
	return npid.handle;
}

const SceNpOnlineName& np_handler::get_online_name() const
{
	return online_name;
}

const SceNpAvatarUrl& np_handler::get_avatar_url() const
{
	return avatar_url;
}

std::string np_handler::ip_to_string(u32 ip_addr)
{
	std::string result;
	in_addr addr;
	addr.s_addr = ip_addr;

	result = inet_ntoa(addr);

	return result;
}

void np_handler::string_to_npid(const char* str, SceNpId* npid)
{
	strncpy(npid->handle.data, str, sizeof(npid->handle.data));
	npid->handle.term = 0;
	npid->reserved[0] = 1;
}

void np_handler::string_to_online_name(const char* str, SceNpOnlineName* online_name)
{
	strncpy(online_name->data, str, sizeof(online_name->data));
	online_name->term = 0;
}

void np_handler::string_to_avatar_url(const char* str, SceNpAvatarUrl* avatar_url)
{
	strncpy(avatar_url->data, str, sizeof(avatar_url->data));
	avatar_url->term = 0;
}

void np_handler::init_NP(u32 poolsize, vm::ptr<void> poolptr)
{
	// Init memory pool
	mpool       = poolptr;
	mpool_size  = poolsize;
	mpool_avail = poolsize;
	mpool_allocs.clear();

	memset(&npid, 0, sizeof(npid));
	memset(&online_name, 0, sizeof(online_name));
	memset(&avatar_url, 0, sizeof(avatar_url));

	if (g_cfg.net.psn_status >= np_psn_status::fake)
	{
		std::string s_npid = g_cfg_rpcn.get_npid();
		ASSERT(!s_npid.empty()); // It should have been generated before this

		np_handler::string_to_npid(s_npid.c_str(), &npid);
		const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
		sigh->set_self_sig_info(npid);
	}

	switch (g_cfg.net.psn_status)
	{
	case np_psn_status::disabled:
		break;
	case np_psn_status::fake:
	{
		np_handler::string_to_online_name("RPCS3's user", &online_name);
		np_handler::string_to_avatar_url("https://rpcs3.net/cdn/netplay/DefaultAvatar.png", &avatar_url);
		break;
	}
	case np_psn_status::rpcn:
	{
		if (!is_psn_active)
			break;
		
		// Connect RPCN client
		if (!rpcn.connect(g_cfg_rpcn.get_host()))
		{
			rpcn_log.error("Connection to RPCN Failed!");
			is_psn_active = false;
			return;
		}

		if (!rpcn.login(g_cfg_rpcn.get_npid(), g_cfg_rpcn.get_password(), g_cfg_rpcn.get_token()))
		{
			rpcn_log.error("RPCN login attempt failed!");
			is_psn_active = false;
			return;
		}

		np_handler::string_to_online_name(rpcn.get_online_name().c_str(), &online_name);
		np_handler::string_to_avatar_url(rpcn.get_avatar_url().c_str(), &avatar_url);
		public_ip_addr = rpcn.get_addr_sig();

		break;
	}
	default:
		break;
	}
}

void np_handler::terminate_NP()
{
	// is_psn_active = false;

	// Reset memory pool
	mpool.set(0);
	mpool_size  = 0;
	mpool_avail = 0;
	mpool_allocs.clear();

	if (g_cfg.net.psn_status == np_psn_status::rpcn)
	{
		rpcn_log.error("Disconnecting from RPCN!");
		rpcn.disconnect();
	}
}

vm::addr_t np_handler::allocate(u32 size)
{
	// Align allocs
	const u32 alloc_size = ::align(size, 4);
	if (alloc_size > mpool_avail)
	{
		sceNp.error("Not enough memory available in NP pool!");
		return vm::cast<u32>(0);
	}

	u32 last_free    = 0;
	bool found_space = false;

	for (auto& a : mpool_allocs)
	{
		if ((a.first - last_free) >= alloc_size)
		{
			found_space = true;
			break;
		}

		last_free = a.first + a.second;
	}

	if (!found_space)
	{
		if ((mpool_size - last_free) < alloc_size)
		{
			sceNp.error("Not enough memory available in NP pool(continuous block)!");
			return vm::cast<u32>(0);
		}
	}

	mpool_allocs.emplace(last_free, alloc_size);
	mpool_avail -= alloc_size;

	memset((static_cast<u8*>(mpool.get_ptr())) + last_free, 0, alloc_size);

	sceNp.trace("Allocation off:%d size:%d psize:%d, pavail:%d", last_free, alloc_size, mpool_size, mpool_avail);

	return vm::cast(mpool.addr() + last_free);
}

std::vector<SceNpMatching2ServerId> np_handler::get_match2_server_list(SceNpMatching2ContextId ctx_id)
{
	std::vector<SceNpMatching2ServerId> server_list{};

	if (g_cfg.net.psn_status == np_psn_status::rpcn)
	{
		if (!rpcn.get_server_list(get_req_id(0), idm::get<match2_ctx>(ctx_id)->communicationId.data, server_list))
		{
			rpcn_log.error("Disconnecting from RPCN!");
			is_psn_active = false;
		}
	}

	return server_list;
}

u32 np_handler::get_server_status(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
{
	// TODO: actually implement interaction with server for this?
	u32 req_id    = generate_callback_info(ctx_id, optParam);
	u32 event_key = get_event_key();

	SceNpMatching2GetServerInfoResponse* serv_info = reinterpret_cast<SceNpMatching2GetServerInfoResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2GetServerInfoResponse)));
	serv_info->server.serverId                     = server_id;
	serv_info->server.status                       = SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE;

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo, event_key, 0, sizeof(SceNpMatching2GetServerInfoResponse), cb_info.cb_arg);
		return 0;
	});

	return req_id;
}

u32 np_handler::create_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
{
	u32 req_id    = generate_callback_info(ctx_id, optParam);
	u32 event_key = get_event_key();

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext, event_key, 0, 0, cb_info.cb_arg);
		return 0;
	});

	return req_id;
}

u32 np_handler::get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.get_world_list(req_id, server_id))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.createjoin_room(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.join_room(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.leave_room(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.search_room(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	extra_nps::print_set_roomdata_ext_req(req);

	if (!rpcn.set_roomdata_external(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.get_roomdata_internal(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	extra_nps::print_set_roomdata_int_req(req);

	if (!rpcn.set_roomdata_internal(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.ping_room_owner(req_id, req->roomId))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

u32 np_handler::send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req)
{
	u32 req_id = generate_callback_info(ctx_id, optParam);

	if (!rpcn.send_room_message(req_id, req))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return req_id;
}

void np_handler::req_sign_infos(const std::string& npid, u32 conn_id)
{
	u32 req_id = get_req_id(0x3333);
	pending_sign_infos_requests[req_id] = conn_id;

	if (!rpcn.req_sign_infos(req_id, npid))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return;
}

void np_handler::req_ticket(u32 version, const SceNpId *npid, const char *service_id, const u8 *cookie, u32 cookie_size, const char *entitlement_id, u32 consumed_count)
{
	u32 req_id = get_req_id(0x3333);

	std::string service_id_str(service_id);

	if (!rpcn.req_ticket(req_id, service_id_str))
	{
		rpcn_log.error("Disconnecting from RPCN!");
		is_psn_active = false;
	}

	return;
}


u32 np_handler::get_match2_event(SceNpMatching2EventKey event_key, u8* dest, u32 size)
{
	std::lock_guard lock(mutex_req_results);

	if (!match2_req_results.count(event_key))
		return 0;

	u32 size_copied = std::min(size, static_cast<u32>(match2_req_results.at(event_key).size()));
	memcpy(dest, match2_req_results.at(event_key).data(), size_copied);

	return size_copied;
}

void np_handler::operator()()
{
	if (g_cfg.net.psn_status != np_psn_status::rpcn)
		return;

	while (thread_ctrl::state() != thread_state::aborting && !Emu.IsStopped())
	{
		if (!rpcn.manage_connection())
		{
			std::this_thread::sleep_for(200ms);
			continue;
		}

		auto replies = rpcn.get_replies();
		for (auto& reply : replies)
		{
			const u16 command     = reply.second.first;
			const u32 req_id      = reply.first;
			std::vector<u8>& data = reply.second.second;

			switch (command)
			{
			case CommandType::GetWorldList: reply_get_world_list(req_id, data); break;
			case CommandType::CreateRoom: reply_create_join_room(req_id, data); break;
			case CommandType::JoinRoom: reply_join_room(req_id, data); break;
			case CommandType::LeaveRoom: reply_leave_room(req_id, data); break;
			case CommandType::SearchRoom: reply_search_room(req_id, data); break;
			case CommandType::SetRoomDataExternal: reply_set_roomdata_external(req_id, data); break;
			case CommandType::GetRoomDataInternal: reply_get_roomdata_internal(req_id, data); break;
			case CommandType::SetRoomDataInternal: reply_set_roomdata_internal(req_id, data); break;
			case CommandType::PingRoomOwner: reply_get_ping_info(req_id, data); break;
			case CommandType::SendRoomMessage: reply_send_room_message(req_id, data); break;
			case CommandType::RequestSignalingInfos: reply_req_sign_infos(req_id, data); break;
			case CommandType::RequestTicket: reply_req_ticket(req_id, data); break;
			default: rpcn_log.error("Unknown reply(%d) received!", command); break;
			}
		}

		auto notifications = rpcn.get_notifications();
		for (auto& notif : notifications)
		{
			switch (notif.first)
			{
			case NotificationType::UserJoinedRoom: notif_user_joined_room(notif.second); break;
			case NotificationType::UserLeftRoom: notif_user_left_room(notif.second); break;
			case NotificationType::RoomDestroyed: notif_room_destroyed(notif.second); break;
			case NotificationType::SignalP2PConnect: notif_p2p_connect(notif.second); break;
			case NotificationType::RoomMessageReceived: notif_room_message_received(notif.second); break;
			default: rpcn_log.error("Unknown notification(%d) received!", notif.first); break;
			}
		}
	}
}

bool np_handler::reply_get_world_list(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to GetWorldList");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);

	std::vector<u32> world_list;
	u32 num_worlds = reply.get<u32>();
	for (u32 i = 0; i < num_worlds; i++)
	{
		world_list.push_back(reply.get<u32>());
	}

	if (reply.is_error())
	{
		world_list.clear();
		return error_and_disconnect("Malformed reply to GetWorldList command");
	}

	u32 event_key = get_event_key();

	SceNpMatching2GetWorldInfoListResponse* world_info = reinterpret_cast<SceNpMatching2GetWorldInfoListResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2GetWorldInfoListResponse)));
	world_info->worldNum                               = world_list.size();

	if (!world_list.empty())
	{
		world_info->world.set(allocate(sizeof(SceNpMatching2World) * world_list.size()));
		for (size_t i = 0; i < world_list.size(); i++)
		{
			world_info->world[i].worldId                  = world_list[i];
			world_info->world[i].numOfLobby               = 1; // TODO
			world_info->world[i].maxNumOfTotalLobbyMember = 10000;
			world_info->world[i].curNumOfTotalLobbyMember = 1;
			world_info->world[i].curNumOfRoom             = 1;
			world_info->world[i].curNumOfTotalRoomMember  = 1;
		}
	}

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList, event_key, 0, sizeof(SceNpMatching2GetWorldInfoListResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_create_join_room(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to CreateRoom");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);
	auto create_room_resp = reply.get_rawdata();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to CreateRoom command");

	u32 event_key = get_event_key();

	auto resp = flatbuffers::GetRoot<RoomDataInternal>(create_room_resp.data());

	SceNpMatching2CreateJoinRoomResponse* room_resp = reinterpret_cast<SceNpMatching2CreateJoinRoomResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2CreateJoinRoomResponse)));
	vm::ptr<SceNpMatching2RoomDataInternal> room_info(allocate(sizeof(SceNpMatching2RoomDataInternal)));
	room_resp->roomDataInternal = room_info;

	RoomDataInternal_to_SceNpMatching2RoomDataInternal(resp, room_info.get_ptr(), npid);

	// Establish Matching2 self signaling info
	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->set_self_sig2_info(room_info->roomId, 1);
	sigh->set_sig2_infos(room_info->roomId, 1, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE, rpcn.get_addr_sig(), rpcn.get_port_sig(), true);
	// TODO? Should this send a message to Signaling CB? Is this even necessary?

	extra_nps::print_create_room_resp(room_resp);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom, event_key, 0, sizeof(SceNpMatching2CreateJoinRoomResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_join_room(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to JoinRoom");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);

	auto join_room_resp = reply.get_rawdata();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to JoinRoom command");

	u32 event_key = get_event_key();

	auto resp = flatbuffers::GetRoot<RoomDataInternal>(join_room_resp.data());

	SceNpMatching2JoinRoomResponse* room_resp = reinterpret_cast<SceNpMatching2JoinRoomResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2JoinRoomResponse)));
	vm::ptr<SceNpMatching2RoomDataInternal> room_info(allocate(sizeof(SceNpMatching2RoomDataInternal)));
	room_resp->roomDataInternal = room_info;

	u16 member_id = RoomDataInternal_to_SceNpMatching2RoomDataInternal(resp, room_info.get_ptr(), npid);

	extra_nps::print_room_data_internal(room_resp->roomDataInternal.get_ptr());

	// Establish Matching2 self signaling info
	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->set_self_sig2_info(room_info->roomId, member_id);
	sigh->set_sig2_infos(room_info->roomId, member_id, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE, rpcn.get_addr_sig(), rpcn.get_port_sig(), true);
	// TODO? Should this send a message to Signaling CB? Is this even necessary?

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom, event_key, 0, sizeof(SceNpMatching2JoinRoomResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_leave_room(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to LeaveRoom");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);
	u64 room_id = reply.get<u64>();
	if (reply.is_error())
		return error_and_disconnect("Malformed reply to LeaveRoom command");

	u32 event_key = get_event_key(); // Unsure if necessary if there is no data

	// Disconnect all users from that room
	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->disconnect_sig2_users(room_id);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom, event_key, 0, 0, cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_search_room(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to SearchRoom");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);
	auto search_room_resp = reply.get_rawdata();
	if (reply.is_error())
		return error_and_disconnect("Malformed reply to SearchRoom command");

	u32 event_key = get_event_key();

	auto resp                                     = flatbuffers::GetRoot<SearchRoomResponse>(search_room_resp.data());
	SceNpMatching2SearchRoomResponse* search_resp = reinterpret_cast<SceNpMatching2SearchRoomResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2SearchRoomResponse)));

	SearchRoomReponse_to_SceNpMatching2SearchRoomResponse(resp, search_resp);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom, event_key, 0, sizeof(SceNpMatching2SearchRoomResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_set_roomdata_external(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to SetRoomDataExternal");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	u32 event_key = get_event_key(); // Unsure if necessary if there is no data

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataExternal, event_key, 0, 0, cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_get_roomdata_internal(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to GetRoomDataInternal");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);

	auto internal_data = reply.get_rawdata();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to GetRoomDataInternal command");

	u32 event_key = get_event_key();

	auto resp                                            = flatbuffers::GetRoot<RoomDataInternal>(internal_data.data());
	SceNpMatching2GetRoomDataInternalResponse* room_resp = reinterpret_cast<SceNpMatching2GetRoomDataInternalResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2GetRoomDataInternalResponse)));
	vm::ptr<SceNpMatching2RoomDataInternal> room_info(allocate(sizeof(SceNpMatching2RoomDataInternal)));
	room_resp->roomDataInternal = room_info;
	RoomDataInternal_to_SceNpMatching2RoomDataInternal(resp, room_info.get_ptr(), npid);

	extra_nps::print_room_data_internal(room_resp->roomDataInternal.get_ptr());

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataInternal, event_key, 0, sizeof(SceNpMatching2GetRoomDataInternalResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_set_roomdata_internal(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to SetRoomDataInternal");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	u32 event_key = get_event_key(); // Unsure if necessary if there is no data

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataInternal, event_key, 0, 0, cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_get_ping_info(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to PingRoomOwner");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	vec_stream reply(reply_data, 1);

	auto ping_resp = reply.get_rawdata();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to PingRoomOwner command");

	u32 event_key = get_event_key();

	auto resp                                                   = flatbuffers::GetRoot<GetPingInfoResponse>(ping_resp.data());
	SceNpMatching2SignalingGetPingInfoResponse* final_ping_resp = reinterpret_cast<SceNpMatching2SignalingGetPingInfoResponse*>(allocate_req_result(event_key, sizeof(SceNpMatching2SignalingGetPingInfoResponse)));
	GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(resp, final_ping_resp);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SignalingGetPingInfo, event_key, 0, sizeof(SceNpMatching2SignalingGetPingInfoResponse), cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_send_room_message(u32 req_id, std::vector<u8>& reply_data)
{
	if (pending_requests.count(req_id) == 0)
		return error_and_disconnect("Unexpected reply ID to PingRoomOwner");

	const auto cb_info = std::move(pending_requests.at(req_id));
	pending_requests.erase(req_id);

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32 {
		cb_info.cb(cb_ppu, cb_info.ctx_id, req_id, SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomMessage, 0, 0, 0, cb_info.cb_arg);
		return 0;
	});

	return true;
}

bool np_handler::reply_req_sign_infos(u32 req_id, std::vector<u8>& reply_data)
{
	if (!pending_sign_infos_requests.count(req_id))
		return error_and_disconnect("Unexpected reply ID to req RequestSignalingInfos");
	
	u32 conn_id = pending_sign_infos_requests.at(req_id);
	pending_sign_infos_requests.erase(req_id);

	vec_stream reply(reply_data, 1);
	u32 addr = reply.get<u32>();
	u16 port = reply.get<u16>();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to RequestSignalingInfos command");
	
	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->start_sig(conn_id, addr, port);

	return true;
}

bool np_handler::reply_req_ticket(u32 req_id, std::vector<u8>& reply_data)
{
	vec_stream reply(reply_data, 1);
	auto ticket_raw = reply.get_rawdata();

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to RequestTicket command");
	
	current_ticket = std::move(ticket_raw);
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

	auto update_info                               = flatbuffers::GetRoot<RoomMemberUpdateInfo>(update_info_raw.data());
	SceNpMatching2RoomMemberUpdateInfo* notif_data = reinterpret_cast<SceNpMatching2RoomMemberUpdateInfo*>(allocate_req_result(event_key, sizeof(SceNpMatching2RoomMemberUpdateInfo)));
	RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(update_info, notif_data);

	rpcn_log.notice("Received notification that user %s(%d) joined the room(%d)", notif_data->roomMemberDataInternal->userInfo.npId.handle.data, notif_data->roomMemberDataInternal->memberId, room_id);
	extra_nps::print_room_member_data_internal(notif_data->roomMemberDataInternal.get_ptr());

	sysutil_register_cb([room_event_cb = this->room_event_cb, room_id, event_key, room_event_cb_ctx = this->room_event_cb_ctx, room_event_cb_arg = this->room_event_cb_arg](ppu_thread& cb_ppu) -> s32 {
		room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined, event_key, 0, sizeof(SceNpMatching2RoomMemberUpdateInfo), room_event_cb_arg);
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

	auto update_info                               = flatbuffers::GetRoot<RoomMemberUpdateInfo>(update_info_raw.data());
	SceNpMatching2RoomMemberUpdateInfo* notif_data = reinterpret_cast<SceNpMatching2RoomMemberUpdateInfo*>(allocate_req_result(event_key, sizeof(SceNpMatching2RoomMemberUpdateInfo)));
	RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(update_info, notif_data);

	rpcn_log.notice("Received notification that user %s(%d) left the room(%d)", notif_data->roomMemberDataInternal->userInfo.npId.handle.data, notif_data->roomMemberDataInternal->memberId, room_id);
	extra_nps::print_room_member_data_internal(notif_data->roomMemberDataInternal.get_ptr());

	sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg](ppu_thread& cb_ppu) -> s32 {
		room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft, event_key, 0, sizeof(SceNpMatching2RoomMemberUpdateInfo), room_event_cb_arg);
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

	auto update_info                         = flatbuffers::GetRoot<RoomUpdateInfo>(update_info_raw.data());
	SceNpMatching2RoomUpdateInfo* notif_data = reinterpret_cast<SceNpMatching2RoomUpdateInfo*>(allocate_req_result(event_key, sizeof(SceNpMatching2RoomUpdateInfo)));
	RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(update_info, notif_data);

	rpcn_log.notice("Received notification that room(%d) was destroyed", room_id);

	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->disconnect_sig2_users(room_id);

	sysutil_register_cb([room_event_cb = this->room_event_cb, room_event_cb_ctx = this->room_event_cb_ctx, room_id, event_key, room_event_cb_arg = this->room_event_cb_arg](ppu_thread& cb_ppu) -> s32 {
		room_event_cb(cb_ppu, room_event_cb_ctx, room_id, SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed, event_key, 0, sizeof(SceNpMatching2RoomUpdateInfo), room_event_cb_arg);
		return 0;
	});
}

void np_handler::notif_p2p_connect(std::vector<u8>& data)
{
	if (data.size() != 16)
	{
		rpcn_log.error("Notification data for SignalP2PConnect != 14");
		return;
	}

	const u64 room_id    = reinterpret_cast<le_t<u64>&>(data[0]);
	const u16 member_id  = reinterpret_cast<le_t<u16>&>(data[8]);
	const u16 port_p2p   = reinterpret_cast<be_t<u16>&>(data[10]);
	const u32 addr_p2p   = reinterpret_cast<le_t<u32>&>(data[12]);

	rpcn_log.notice("Received notification to connect to member(%d) of room(%d): %s:%d", member_id, room_id, ip_to_string(addr_p2p), port_p2p);

	// Attempt Signaling
	const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
	sigh->set_sig2_infos(room_id, member_id, SCE_NP_SIGNALING_CONN_STATUS_PENDING, addr_p2p, port_p2p);
	sigh->start_sig2(room_id, member_id);
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

	auto message_info                        = flatbuffers::GetRoot<RoomMessageInfo>(message_info_raw.data());
	SceNpMatching2RoomMessageInfo* notif_data = reinterpret_cast<SceNpMatching2RoomMessageInfo*>(allocate_req_result(event_key, sizeof(SceNpMatching2RoomMessageInfo)));
	RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(message_info, notif_data);

	rpcn_log.notice("Received notification of a room message from member(%d) in room(%d)", member_id, room_id);

	if (room_msg_cb)
	{
		sysutil_register_cb([room_msg_cb = this->room_msg_cb, room_msg_cb_ctx = this->room_msg_cb_ctx, room_id, member_id, event_key, room_msg_cb_arg = this->room_msg_cb_arg](ppu_thread& cb_ppu) -> s32
		{
			room_msg_cb(cb_ppu, room_msg_cb_ctx, room_id, member_id, SCE_NP_MATCHING2_ROOM_MSG_EVENT_Message, event_key, 0, sizeof(SceNpMatching2RoomUpdateInfo), room_msg_cb_arg);
			return 0;
		});
	}
}

void np_handler::add_dns_spy(u32 sock)
{
	dns_spylist.emplace(std::make_pair(sock, std::queue<std::vector<u8>>()));
}

void np_handler::remove_dns_spy(u32 sock)
{
	dns_spylist.erase(sock);
}

bool np_handler::is_dns(u32 sock) const
{
	return dns_spylist.count(sock) != 0;
}

bool np_handler::is_dns_queue(u32 sock) const
{
	return !dns_spylist.at(sock).empty();
}

std::vector<u8> np_handler::get_dns_packet(u32 sock)
{
	auto ret_vec = std::move(dns_spylist.at(sock).front());
	dns_spylist.at(sock).pop();

	return ret_vec;
}

s32 np_handler::analyze_dns_packet(s32 s, const u8* buf, u32 len)
{
	if (sys_net.enabled == logs::level::trace)
	{
		std::string datrace;
		const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

		for (u32 index = 0; index < len; index++)
		{
			if ((index % 16) == 0)
				datrace += '\n';

			datrace += hex[(buf[index] >> 4) & 15];
			datrace += hex[(buf[index]) & 15];
			datrace += ' ';
		}
		sys_net.trace("DNS REQUEST: %s", datrace);
	}

	struct dns_header
	{
		u16 id; // identification number

		u8 rd : 1;     // recursion desired
		u8 tc : 1;     // truncated message
		u8 aa : 1;     // authoritive answer
		u8 opcode : 4; // purpose of message
		u8 qr : 1;     // query/response flag

		u8 rcode : 4; // response code
		u8 cd : 1;    // checking disabled
		u8 ad : 1;    // authenticated data
		u8 z : 1;     // its z! reserved
		u8 ra : 1;    // recursion available

		be_t<u16> q_count;    // number of question entries
		be_t<u16> ans_count;  // number of answer entries
		be_t<u16> auth_count; // number of authority entries
		be_t<u16> add_count;  // number of resource entries
	};

	if (len < sizeof(dns_header))
		return -1;

	const dns_header* dhead = reinterpret_cast<const dns_header*>(buf);
	// We are only looking for queries not truncated(todo?), only handle one dns query at a time(todo?)
	if (dhead->qr != 0 || dhead->tc != 0 || dhead->q_count != 1 || dhead->ans_count != 0 || dhead->auth_count != 0 || dhead->add_count != 0)
		return -1;

	// Get the actual address
	u8 count = 0;
	std::string host{};
	for (u32 i = sizeof(dns_header); (i < len) && buf[i] != 0; i++)
	{
		if (count == 0)
		{
			count = buf[i];
			if (i != sizeof(dns_header))
			{
				host += '.';
			}
		}
		else
		{
			host += static_cast<char>(buf[i]);
			count--;
		}
	}

	sys_net.warning("DNS query for %s", host);

	if (switch_map.count(host))
	{
		// design fake packet
		std::vector<u8> fake(len);
		memcpy(fake.data(), buf, len);
		dns_header* fake_header = reinterpret_cast<dns_header*>(fake.data());
		fake_header->qr         = 1;
		fake_header->ra         = 1;
		fake_header->ans_count  = 1;
		fake.insert(fake.end(), {0xC0, 0x0C});             // Ref to name in header
		fake.insert(fake.end(), {0x00, 0x01});             // IPv4
		fake.insert(fake.end(), {0x00, 0x01});             // Class?
		fake.insert(fake.end(), {0x00, 0x00, 0x00, 0x3B}); // TTL
		fake.insert(fake.end(), {0x00, 0x04});             // Size of data
		u32 ip     = switch_map[host];
		u8* ptr_ip = reinterpret_cast<u8*>(&ip);
		fake.insert(fake.end(), ptr_ip, ptr_ip + 4); // IP

		sys_net.warning("Solving %s to %d.%d.%d.%d", host, ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

		dns_spylist[s].push(std::move(fake));
		return len;
	}

	return -1;
}

s32 np_handler::create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	return static_cast<s32>(idm::make<score_ctx>(communicationId, passphrase));
}
bool np_handler::destroy_score_context(s32 ctx_id)
{
	return idm::remove<score_ctx>(static_cast<u32>(ctx_id));
}

s32 np_handler::create_score_transaction_context(s32 score_context_id)
{
	return static_cast<s32>(idm::make<score_transaction_ctx>(score_context_id));
}
bool np_handler::destroy_score_transaction_context(s32 ctx_id)
{
	return idm::remove<score_transaction_ctx>(static_cast<u32>(ctx_id));
}

u16 np_handler::create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	return static_cast<u16>(idm::make<match2_ctx>(communicationId, passphrase));
}
bool np_handler::destroy_match2_context(u16 ctx_id)
{
	return idm::remove<match2_ctx>(static_cast<u32>(ctx_id));
}
std::shared_ptr<np_handler::match2_ctx> np_handler::get_match2_context(u16 ctx_id)
{
	return idm::get_unlocked<match2_ctx>(ctx_id);
}

s32 np_handler::create_lookup_title_context(vm::cptr<SceNpCommunicationId> communicationId)
{
	return static_cast<s32>(idm::make<lookup_title_ctx>(communicationId));
}
bool np_handler::destroy_lookup_title_context(s32 ctx_id)
{
	return idm::remove<lookup_title_ctx>(static_cast<u32>(ctx_id));
}

s32 np_handler::create_lookup_transaction_context(s32 lt_ctx)
{
	return static_cast<s32>(idm::make<lookup_transaction_ctx>(lt_ctx));
}
bool np_handler::destroy_lookup_transaction_context(s32 ctx_id)
{
	return idm::remove<lookup_transaction_ctx>(static_cast<u32>(ctx_id));
}

s32 np_handler::create_commerce2_context(u32 version, vm::cptr<SceNpId> npid, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg)
{
	return static_cast<s32>(idm::make<commerce2_ctx>(version, npid, handler, arg));
}
bool np_handler::destroy_commerce2_context(s32 ctx_id)
{
	return idm::remove<commerce2_ctx>(static_cast<u32>(ctx_id));
}
std::shared_ptr<np_handler::commerce2_ctx> np_handler::get_commerce2_context(u16 ctx_id)
{
	return idm::get_unlocked<commerce2_ctx>(ctx_id);
}

s32 np_handler::create_signaling_context(vm::ptr<SceNpId> npid, vm::ptr<SceNpSignalingHandler> handler, vm::ptr<void> arg)
{
	return static_cast<s32>(idm::make<signaling_ctx>(npid, handler, arg));
}
bool np_handler::destroy_signaling_context(s32 ctx_id)
{
	return idm::remove<signaling_ctx>(static_cast<u32>(ctx_id));
}

bool np_handler::error_and_disconnect(const std::string& error_msg)
{
	rpcn_log.error("%s", error_msg);
	rpcn.disconnect();

	return false;
}

u32 np_handler::generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam)
{
	callback_info ret;

	const auto ctx = get_match2_context(ctx_id);
	ASSERT(ctx);

	const u32 req_id = get_req_id(optParam ? optParam->appReqId : ctx->default_match2_optparam.appReqId);

	ret.ctx_id = ctx_id;
	ret.cb     = optParam ? optParam->cbFunc : ctx->default_match2_optparam.cbFunc;
	ret.cb_arg = optParam ? optParam->cbFuncArg : ctx->default_match2_optparam.cbFuncArg;

	nph_log.warning("Callback used is 0x%x", ret.cb);

	pending_requests[req_id] = std::move(ret);

	return req_id;
}

u8* np_handler::allocate_req_result(u32 event_key, size_t size)
{
	std::lock_guard lock(mutex_req_results);
	match2_req_results[event_key] = std::vector<u8>(size, 0);
	return match2_req_results[event_key].data();
}

u32 np_handler::add_players_to_history(vm::cptr<SceNpId> npids, u32 count)
{
	const u32 req_id = get_req_id(0);

	// if (basic_handler)
	// {
	// 	sysutil_register_cb([basic_handler = this->basic_handler, req_id, basic_handler_arg = this->basic_handler_arg](ppu_thread& cb_ppu) -> s32 {
	// 		basic_handler(cb_ppu, SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT, 0, req_id, basic_handler_arg);
	// 		return 0;
	// 	});
	// }

	return req_id;
}
