#pragma once

#include <queue>
#include <map>
#include <unordered_map>

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "Emu/NP/rpcn_client.h"
#include "Emu/NP/generated/np2_structs_generated.h"
#include "Emu/NP/signaling_handler.h"
#include "Emu/NP/np_allocator.h"
#include "Emu/NP/np_cache.h"
#include "Emu/NP/np_event_data.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/upnp_handler.h"

namespace np
{
	struct ticket_data
	{
		u16 id{}, len{};

		struct
		{
			u32 data_u32{};
			u64 data_u64{};
			std::vector<u8> data_vec;
			std::vector<ticket_data> data_nodes;
		} data;
	};

	class ticket
	{
	public:
		ticket() = default;
		ticket(std::vector<u8>&& raw_data);

		std::size_t size() const;
		const u8* data() const;
		bool empty() const;

		bool get_value(s32 param_id, vm::ptr<SceNpTicketParam> param) const;

	private:
		std::optional<ticket_data> parse_node(std::size_t index) const;
		void parse();

	private:
		static constexpr std::size_t MIN_TICKET_DATA_SIZE = 4;

		std::vector<u8> raw_data;

		bool parse_success = false;
		u32 version{};
		std::vector<ticket_data> nodes;
	};

	struct basic_event
	{
		s32 event = 0;
		SceNpUserInfo from{};
		std::vector<u8> data;
	};

	class np_handler
	{
	public:
		SAVESTATE_INIT_POS(5);

		np_handler();
		~np_handler();
		np_handler(utils::serial& ar);
		void save(utils::serial& ar);

		void init_np_handler_dependencies();
		const std::array<u8, 6>& get_ether_addr() const;
		const std::string& get_hostname() const;
		u32 get_local_ip_addr() const;
		u32 get_public_ip_addr() const;
		u32 get_dns_ip() const;
		u32 get_bind_ip() const;

		s32 get_psn_status() const;
		s32 get_net_status() const;
		s32 get_upnp_status() const;

		const SceNpId& get_npid() const;
		const SceNpOnlineId& get_online_id() const;
		const SceNpOnlineName& get_online_name() const;
		const SceNpAvatarUrl& get_avatar_url() const;

		// handles async messages from server(only needed for RPCN)
		void operator()();

		void init_NP(u32 poolsize, vm::ptr<void> poolptr);
		void terminate_NP();

		atomic_t<bool> is_netctl_init     = false;
		atomic_t<bool> is_NP_init         = false;
		atomic_t<bool> is_NP_Lookup_init  = false;
		atomic_t<bool> is_NP_Score_init   = false;
		atomic_t<bool> is_NP2_init        = false;
		atomic_t<bool> is_NP2_Match2_init = false;
		atomic_t<bool> is_NP_Auth_init    = false;

		// NP Handlers/Callbacks
		// Seems to be global
		vm::ptr<SceNpManagerCallback> manager_cb{}; // Connection status and tickets
		vm::ptr<void> manager_cb_arg{};

		// Basic event handler;
		struct
		{
			SceNpCommunicationId context{};
			vm::ptr<SceNpBasicEventHandler> handler_func;
			vm::ptr<void> handler_arg;
			bool registered        = false;
			bool context_sensitive = false;
		} basic_handler;

		void queue_basic_event(basic_event to_queue);
		bool send_basic_event(s32 event, s32 retCode, u32 reqId);
		error_code get_basic_event(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<u8> data, vm::ptr<u32> size);

		// Messages-related functions
		std::optional<std::shared_ptr<std::pair<std::string, message_data>>> get_message(u64 id);
		void set_message_selected(SceNpBasicAttachmentDataId id, u64 msg_id);
		std::optional<std::shared_ptr<std::pair<std::string, message_data>>> get_message_selected(SceNpBasicAttachmentDataId id);
		void clear_message_selected(SceNpBasicAttachmentDataId id);

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
		u32 delete_server_context(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id);
		u32 get_world_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, u16 server_id);
		u32 create_join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2CreateJoinRoomRequest* req);
		u32 join_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2JoinRoomRequest* req);
		u32 leave_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2LeaveRoomRequest* req);
		u32 search_room(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SearchRoomRequest* req);
		u32 get_roomdata_external_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataExternalListRequest* req);
		u32 set_roomdata_external(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataExternalRequest* req);
		u32 get_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomDataInternalRequest* req);
		u32 set_roomdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomDataInternalRequest* req);
		u32 set_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomMemberDataInternalRequest* req);
		u32 get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req);
		u32 send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req);

		u32 get_match2_event(SceNpMatching2EventKey event_key, u32 dest_addr, u32 size);

		// Score requests
		void score_async_handler(std::unique_lock<shared_mutex> lock, const std::shared_ptr<score_transaction_ctx>& trans_ctx, u32 req_id, bool async);
		void get_board_infos(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, bool async);
		void record_score(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, const u8* data, u32 data_size, vm::ptr<SceNpScoreRankNumber> tmpRank, bool async);
		void record_score_data(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, const u8* score_data, bool async);
		void get_score_data(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const SceNpId& npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> score_data, bool async);
		void get_score_range(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async);
		void get_score_npid(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const std::vector<std::pair<SceNpId, s32>>& npid_vec, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async);
		void get_score_friend(std::shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, bool include_self, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async);

		// Local functions
		std::pair<error_code, std::optional<SceNpId>> local_get_npid(u64 room_id, u16 member_id);
		std::pair<error_code, std::optional<SceNpMatching2RoomSlotInfo>> local_get_room_slots(SceNpMatching2RoomId room_id);
		std::pair<error_code, std::optional<SceNpMatching2SessionPassword>> local_get_room_password(SceNpMatching2RoomId room_id);
		std::pair<error_code, std::vector<SceNpMatching2RoomMemberId>> local_get_room_memberids(SceNpMatching2RoomId room_id, s32 sort_method);
		error_code local_get_room_member_data(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data);

		// Friend stuff
		u32 get_num_friends();
		u32 get_num_blocks();
		std::pair<error_code, std::optional<SceNpId>> get_friend_by_index(u32 index);

		// Misc stuff
		void req_ticket(u32 version, const SceNpId* npid, const char* service_id, const u8* cookie, u32 cookie_size, const char* entitlement_id, u32 consumed_count);
		const ticket& get_ticket() const;
		u32 add_players_to_history(vm::cptr<SceNpId> npids, u32 count);
		bool abort_request(u32 req_id);

		// For signaling
		void req_sign_infos(const std::string& npid, u32 conn_id);

		// For UPNP
		void upnp_add_port_mapping(u16 internal_port, std::string_view protocol);
		void upnp_remove_port_mapping(u16 internal_port, std::string_view protocol);

		// For custom menu
		struct custom_menu_action
		{
			s32 id   = 0;
			u32 mask = SCE_NP_CUSTOM_MENU_ACTION_MASK_ME;
			std::string name;
		};
		shared_mutex mutex_custom_menu;
		bool custom_menu_registered = false;
		vm::ptr<SceNpCustomMenuEventHandler> custom_menu_handler{};
		vm::ptr<void> custom_menu_user_arg{};
		std::vector<custom_menu_action> custom_menu_actions;
		SceNpCustomMenuIndexArray custom_menu_activation{};
		std::vector<SceNpCustomMenuActionExceptions> custom_menu_exception_list{};

		// Mutex for NP status change
		shared_mutex mutex_status;

		static constexpr std::string_view thread_name = "NP Handler Thread";

	private:
		// Various generic helpers
		bool discover_ip_address();
		bool discover_ether_address();
		bool error_and_disconnect(const std::string& error_msg);

		// Notification handlers
		void notif_user_joined_room(std::vector<u8>& data);
		void notif_user_left_room(std::vector<u8>& data);
		void notif_room_destroyed(std::vector<u8>& data);
		void notif_updated_room_data_internal(std::vector<u8>& data);
		void notif_updated_room_member_data_internal(std::vector<u8>& data);
		void notif_p2p_connect(std::vector<u8>& data);
		void notif_room_message_received(std::vector<u8>& data);

		// Reply handlers
		bool reply_get_world_list(u32 req_id, std::vector<u8>& reply_data);
		bool reply_create_join_room(u32 req_id, std::vector<u8>& reply_data);
		bool reply_join_room(u32 req_id, std::vector<u8>& reply_data);
		bool reply_leave_room(u32 req_id, std::vector<u8>& reply_data);
		bool reply_search_room(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_roomdata_external_list(u32 req_id, std::vector<u8>& reply_data);
		bool reply_set_roomdata_external(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_roomdata_internal(u32 req_id, std::vector<u8>& reply_data);
		bool reply_set_roomdata_internal(u32 req_id, std::vector<u8>& reply_data);
		bool reply_set_roommemberdata_internal(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_ping_info(u32 req_id, std::vector<u8>& reply_data);
		bool reply_send_room_message(u32 req_id, std::vector<u8>& reply_data);
		bool reply_req_sign_infos(u32 req_id, std::vector<u8>& reply_data);
		bool reply_req_ticket(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_board_infos(u32 req_id, std::vector<u8>& reply_data);
		bool reply_record_score(u32 req_id, std::vector<u8>& reply_data);
		bool reply_record_score_data(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_score_data(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_score_range(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_score_friends(u32 req_id, std::vector<u8>& reply_data);
		bool reply_get_score_npid(u32 req_id, std::vector<u8>& reply_data);

		// Helper functions(fb=>np2)
		void BinAttr_to_SceNpMatching2BinAttr(event_data& edata, const BinAttr* bin_attr, SceNpMatching2BinAttr* binattr_info);
		void BinAttrs_to_SceNpMatching2BinAttrs(event_data& edata, const flatbuffers::Vector<flatbuffers::Offset<BinAttr>>* fb_attr, SceNpMatching2BinAttr* binattr_info);
		void RoomMemberBinAttrInternal_to_SceNpMatching2RoomMemberBinAttrInternal(event_data& edata, const RoomMemberBinAttrInternal* fb_attr, SceNpMatching2RoomMemberBinAttrInternal* binattr_info);
		void RoomBinAttrInternal_to_SceNpMatching2RoomBinAttrInternal(event_data& edata, const BinAttrInternal* fb_attr, SceNpMatching2RoomBinAttrInternal* binattr_info);
		void RoomGroup_to_SceNpMatching2RoomGroup(const RoomGroup* fb_group, SceNpMatching2RoomGroup* sce_group);
		void RoomGroups_to_SceNpMatching2RoomGroups(const flatbuffers::Vector<flatbuffers::Offset<RoomGroup>>* fb_groups, SceNpMatching2RoomGroup* sce_groups);
		void UserInfo2_to_SceNpUserInfo2(event_data& edata, const UserInfo2* user, SceNpUserInfo2* user_info);
		void RoomDataExternal_to_SceNpMatching2RoomDataExternal(event_data& edata, const RoomDataExternal* room, SceNpMatching2RoomDataExternal* room_info);
		void SearchRoomResponse_to_SceNpMatching2SearchRoomResponse(event_data& edata, const SearchRoomResponse* resp, SceNpMatching2SearchRoomResponse* search_resp);
		void GetRoomDataExternalListResponse_to_SceNpMatching2GetRoomDataExternalListResponse(event_data& edata, const GetRoomDataExternalListResponse* resp, SceNpMatching2GetRoomDataExternalListResponse* get_resp);
		u16 RoomDataInternal_to_SceNpMatching2RoomDataInternal(event_data& edata, const RoomDataInternal* resp, SceNpMatching2RoomDataInternal* room_resp, const SceNpId& npid);
		void RoomMemberDataInternal_to_SceNpMatching2RoomMemberDataInternal(event_data& edata, const RoomMemberDataInternal* member_data, const SceNpMatching2RoomDataInternal* room_info, SceNpMatching2RoomMemberDataInternal* sce_member_data);
		void RoomMemberUpdateInfo_to_SceNpMatching2RoomMemberUpdateInfo(event_data& edata, const RoomMemberUpdateInfo* resp, SceNpMatching2RoomMemberUpdateInfo* room_info);
		void RoomUpdateInfo_to_SceNpMatching2RoomUpdateInfo(const RoomUpdateInfo* update_info, SceNpMatching2RoomUpdateInfo* sce_update_info);
		void GetPingInfoResponse_to_SceNpMatching2SignalingGetPingInfoResponse(const GetPingInfoResponse* resp, SceNpMatching2SignalingGetPingInfoResponse* sce_resp);
		void RoomMessageInfo_to_SceNpMatching2RoomMessageInfo(event_data& edata, const RoomMessageInfo* mi, SceNpMatching2RoomMessageInfo* sce_mi);
		void RoomDataInternalUpdateInfo_to_SceNpMatching2RoomDataInternalUpdateInfo(event_data& edata, const RoomDataInternalUpdateInfo* update_info, SceNpMatching2RoomDataInternalUpdateInfo* sce_update_info, const SceNpId& npid);
		void RoomMemberDataInternalUpdateInfo_to_SceNpMatching2RoomMemberDataInternalUpdateInfo(event_data& edata, const RoomMemberDataInternalUpdateInfo* update_info, SceNpMatching2RoomMemberDataInternalUpdateInfo* sce_update_info);
		bool handle_GetScoreResponse(u32 req_id, std::vector<u8>& reply_data);

		struct callback_info
		{
			SceNpMatching2ContextId ctx_id;
			vm::ptr<SceNpMatching2RequestCallback> cb;
			vm::ptr<void> cb_arg;
			SceNpMatching2Event event_type;

			void queue_callback(u32 req_id, u32 event_key, s32 error_code, u32 data_size) const
			{
				if (cb)
				{
					sysutil_register_cb([=, *this](ppu_thread& cb_ppu) -> s32
					{
						cb(cb_ppu, ctx_id, req_id, event_type, event_key, error_code, data_size, cb_arg);
						return 0;
					});
				}
			}
		};

		u32 generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, SceNpMatching2Event event_type);
		std::optional<callback_info> take_pending_request(u32 req_id);

		shared_mutex mutex_pending_requests;
		std::unordered_map<u32, callback_info> pending_requests;
		shared_mutex mutex_pending_sign_infos_requests;
		std::unordered_map<u32, u32> pending_sign_infos_requests;

		shared_mutex mutex_queue_basic_events;
		std::queue<basic_event> queue_basic_events;

		bool m_inited_np_handler_dependencies = false;

	private:
		bool is_connected  = false;
		bool is_psn_active = false;

		ticket current_ticket;

		// IP & DNS info
		std::string hostname = "localhost";
		std::array<u8, 6> ether_address{};
		be_t<u32> local_ip_addr{};
		be_t<u32> public_ip_addr{};
		be_t<u32> dns_ip = 0x08080808;
		be_t<u32> bind_ip = 0x00000000;

		// User infos
		SceNpId npid{};
		SceNpOnlineName online_name{};
		SceNpAvatarUrl avatar_url{};

		// Memory pool for sceNp/sceNp2
		memory_allocator np_memory;

		// Cache related
		std::optional<SceNpMatching2SessionPassword> cached_cj_password;
		cache_manager np_cache;

		// Messages related
		std::optional<u64> selected_invite_id{};
		std::optional<u64> selected_message_id{};

		// Requests(reqEventKey : data)
		shared_mutex mutex_match2_req_results;
		std::unordered_map<u32, event_data> match2_req_results;
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
		event_data& allocate_req_result(u32 event_key, u32 max_size, u32 initial_size);

		// Async score threads
		shared_mutex mutex_score_transactions;
		std::unordered_map<u32, std::shared_ptr<score_transaction_ctx>> score_transactions; // (req_id, transaction_ctx)

		// RPCN
		shared_mutex mutex_rpcn;
		std::shared_ptr<rpcn::rpcn_client> rpcn;
		std::shared_ptr<rpcn::rpcn_client> get_rpcn();

		// UPNP
		upnp_handler upnp;
	};
} // namespace np
