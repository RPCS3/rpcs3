#pragma once

#include <queue>
#include <map>
#include <unordered_map>

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

#include "Emu/NP/rpcn_client.h"
#include "generated/np2_structs_generated.h"
#include "signaling_handler.h"

class np_handler
{
public:
	np_handler();

	const std::array<u8, 6>& get_ether_addr() const;
	const std::string& get_hostname() const;
	u32 get_local_ip_addr() const;
	u32 get_public_ip_addr() const;
	u32 get_dns_ip() const;

	s32 get_psn_status() const;
	s32 get_net_status() const;

	const SceNpId& get_npid() const;
	const SceNpOnlineId& get_online_id() const;
	const SceNpOnlineName& get_online_name() const;
	const SceNpAvatarUrl& get_avatar_url() const;

	// Public helpers
	static std::string ip_to_string(u32 addr);
	static std::string ether_to_string(std::array<u8, 6>& ether);
	// Helpers for setting various structures from string
	static void string_to_npid(const std::string&, SceNpId* npid);
	static void string_to_online_name(const std::string&, SceNpOnlineName* online_name);
	static void string_to_avatar_url(const std::string&, SceNpAvatarUrl* avatar_url);

	// DNS hooking functions
	void add_dns_spy(u32 sock);
	void remove_dns_spy(u32 sock);
	bool is_dns(u32 sock) const;
	bool is_dns_queue(u32 sock) const;
	std::vector<u8> get_dns_packet(u32 sock);
	s32 analyze_dns_packet(s32 s, const u8* buf, u32 len);

	enum NotificationType : u16
	{
		UserJoinedRoom,
		UserLeftRoom,
		RoomDestroyed,
		SignalP2PConnect,
		_SignalP2PDisconnect,
		RoomMessageReceived,
	};

	// handles async messages from server(only needed for RPCN)
	void operator()();

	void init_NP(u32 poolsize, vm::ptr<void> poolptr);
	void terminate_NP();

	bool is_netctl_init     = false;
	bool is_NP_init         = false;
	bool is_NP_Lookup_init  = false;
	bool is_NP_Score_init   = false;
	bool is_NP2_init        = false;
	bool is_NP2_Match2_init = false;
	bool is_NP_Auth_init    = false;

	// NP Handlers/Callbacks
	// Seems to be global
	vm::ptr<SceNpManagerCallback> manager_cb{};               // Connection status and tickets
	vm::ptr<void> manager_cb_arg{};

	// Registered by SceNpCommunicationId
	vm::ptr<SceNpBasicEventHandler> basic_handler;
	vm::ptr<void> basic_handler_arg;

	// Those should probably be under match2 ctx
	vm::ptr<SceNpMatching2RoomEventCallback> room_event_cb{}; // Room events
	u16 room_event_cb_ctx = 0;
	vm::ptr<void> room_event_cb_arg{};
	vm::ptr<SceNpMatching2RoomMessageCallback> room_msg_cb{};
	u16 room_msg_cb_ctx = 0;
	vm::ptr<void> room_msg_cb_arg{};

	// Synchronous requests
	std::vector<SceNpMatching2ServerId> get_match2_server_list(SceNpMatching2ContextId);
	// Asynchronous requests
	u32 get_server_status(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id);
	u32 create_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id);
	u32 get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id);
	u32 create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req);
	u32 join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req);
	u32 leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req);
	u32 search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req);
	u32 set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req);
	u32 get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req);
	u32 set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req);
	u32 get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req);
	u32 send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req);

	u32 get_match2_event(SceNpMatching2EventKey event_key, u8* dest, u32 size);

	// Misc stuff
	void req_ticket(u32 version, const SceNpId *npid, const char *service_id, const u8 *cookie, u32 cookie_size, const char *entitlement_id, u32 consumed_count);
	const std::vector<u8>& get_ticket() const { return current_ticket; }
	u32 add_players_to_history(vm::cptr<SceNpId> npids, u32 count);

	// For signaling
	void req_sign_infos(const std::string& npid, u32 conn_id);

	// Mutex for NP status change
	shared_mutex mutex_status;

	static constexpr std::string_view thread_name = "NP Handler Thread";

protected:
	// Various generic helpers
	bool discover_ip_address();
	bool discover_ether_address();
	bool error_and_disconnect(const std::string& error_msg);

	// Notification handlers
	void notif_user_joined_room(std::vector<u8>& data);
	void notif_user_left_room(std::vector<u8>& data);
	void notif_room_destroyed(std::vector<u8>& data);
	void notif_p2p_connect(std::vector<u8>& data);
	void notif_room_message_received(std::vector<u8>& data);

	// Reply handlers
	bool reply_get_world_list(u32 req_id, std::vector<u8>& reply_data);
	bool reply_create_join_room(u32 req_id, std::vector<u8>& reply_data);
	bool reply_join_room(u32 req_id, std::vector<u8>& reply_data);
	bool reply_leave_room(u32 req_id, std::vector<u8>& reply_data);
	bool reply_search_room(u32 req_id, std::vector<u8>& reply_data);
	bool reply_set_roomdata_external(u32 req_id, std::vector<u8>& reply_data);
	bool reply_get_roomdata_internal(u32 req_id, std::vector<u8>& reply_data);
	bool reply_set_roomdata_internal(u32 req_id, std::vector<u8>& reply_data);
	bool reply_get_ping_info(u32 req_id, std::vector<u8>& reply_data);
	bool reply_send_room_message(u32 req_id, std::vector<u8>& reply_data);
	bool reply_req_sign_infos(u32 req_id, std::vector<u8>& reply_data);
	bool reply_req_ticket(u32 req_id, std::vector<u8>& reply_data);

	// Helper functions(fb=>np2)
	void BinAttr_to_SceNpMatching2BinAttr(const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, vm::ptr<SceNpMatching2BinAttr> binattr_info);
	void RoomGroup_to_SceNpMatching2RoomGroup(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_group, vm::ptr<SceNpMatching2RoomGroup> group_info);
	void UserInfo2_to_SceNpUserInfo2(const UserInfo2* user, SceNpUserInfo2* user_info);
	void SearchRoomReponse_to_SceNpMatching2SearchRoomResponse(const SearchRoomResponse* resp, SceNpMatching2SearchRoomResponse* search_resp);
	u16 RoomDataInternal_to_SceNpMatching2RoomDataInternal(const RoomDataInternal* resp, SceNpMatching2RoomDataInternal* room_resp, const SceNpId& npid);
	void RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(const RoomMemberUpdateInfo* resp, SceNpMatching2RoomMemberUpdateInfo* room_info);
	void RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const RoomUpdateInfo* update_info, SceNpMatching2RoomUpdateInfo* sce_update_info);
	void GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const GetPingInfoResponse* resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp);
	void RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(const RoomMessageInfo* mi, SceNpMatching2RoomMessageInfo* sce_mi);

	struct callback_info
	{
		SceNpMatching2ContextId ctx_id;
		vm::ptr<SceNpMatching2RequestCallback> cb;
		vm::ptr<void> cb_arg;
	};
	u32 generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam);

	std::unordered_map<u32, callback_info> pending_requests;
	std::unordered_map<u32, u32> pending_sign_infos_requests;

protected:
	bool is_connected  = false;
	bool is_psn_active = false;

	std::vector<u8> current_ticket;

	// IP & DNS info
	std::string hostname = "localhost";
	std::array<u8, 6> ether_address{};
	be_t<u32> local_ip_addr{};
	be_t<u32> public_ip_addr{};
	be_t<u32> dns_ip = 0x08080808;

	// User infos
	SceNpId npid{};
	SceNpOnlineName online_name{};
	SceNpAvatarUrl avatar_url{};

	// DNS related
	std::map<s32, std::queue<std::vector<u8>>> dns_spylist{};
	std::map<std::string, u32> switch_map{};

	// Memory pool for sceNp/sceNp2
	vm::ptr<void> mpool{};
	u32 mpool_size  = 0;
	u32 mpool_avail = 0;
	std::map<u32, u32> mpool_allocs{}; // offset/size
	vm::addr_t allocate(u32 size);

	// Requests(reqEventKey : data)
	std::unordered_map<u32, std::vector<u8>> match2_req_results{};
	atomic_t<u16> match2_low_reqid_cnt = 1;
	atomic_t<u32> match2_event_cnt     = 1;
	u32 get_req_id(u16 app_req)
	{
		return ((app_req << 16) | match2_low_reqid_cnt.fetch_add(1));
	}
	u32 get_event_key()
	{
		return match2_event_cnt.fetch_add(1);
	}
	shared_mutex mutex_req_results;
	u8* allocate_req_result(u32 event_key, usz size);

	// RPCN
	rpcn_client rpcn;
};
