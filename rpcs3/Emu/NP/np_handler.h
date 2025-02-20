#pragma once

#include <queue>
#include <map>
#include <unordered_map>

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "Emu/NP/rpcn_client.h"
#include "Emu/NP/np_allocator.h"
#include "Emu/NP/np_cache.h"
#include "Emu/NP/np_gui_cache.h"
#include "Emu/NP/np_event_data.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/upnp_handler.h"

namespace np
{
	constexpr usz MAX_SceNpMatchingAttr_list_SIZE = ((SCE_NP_MATCHING_ATTR_ID_MAX * 2) * sizeof(SceNpMatchingAttr))
		+ (SCE_NP_MATCHING_ATTR_BIN_BIG_SIZE_ID_MAX * SCE_NP_MATCHING_ATTR_BIN_MAX_SIZE_BIG) +
		+ ((SCE_NP_MATCHING_ATTR_ID_MAX - SCE_NP_MATCHING_ATTR_BIN_BIG_SIZE_ID_MAX) * SCE_NP_MATCHING_ATTR_BIN_MAX_SIZE_SMALL);
	constexpr usz MAX_MEMBERS_PER_ROOM = 64;
	constexpr usz MAX_ROOMS_PER_GET_ROOM_LIST = 20;
	constexpr usz MAX_SceNpMatchingRoomStatus_SIZE = sizeof(SceNpMatchingRoomStatus) + (MAX_MEMBERS_PER_ROOM * sizeof(SceNpMatchingRoomMember)) + sizeof(SceNpId);
	constexpr usz MAX_SceNpMatchingJoinedRoomInfo_SIZE = sizeof(SceNpMatchingJoinedRoomInfo) + (MAX_MEMBERS_PER_ROOM * sizeof(SceNpMatchingRoomMember)) + sizeof(SceNpId);
	constexpr usz MAX_SceNpMatchingRoomList_SIZE = sizeof(SceNpMatchingRoomList) + MAX_ROOMS_PER_GET_ROOM_LIST * (sizeof(SceNpMatchingRoom) + MAX_SceNpMatchingAttr_list_SIZE);
	constexpr usz MAX_SceNpMatchingRoom_SIZE = sizeof(SceNpMatchingRoom) + MAX_SceNpMatchingAttr_list_SIZE;
	constexpr usz MAX_SceNpMatchingSearchJoinRoomInfo_SIZE = sizeof(SceNpMatchingSearchJoinRoomInfo) + MAX_SceNpMatchingAttr_list_SIZE;

	enum class REQUEST_ID_HIGH : u16
	{
		MISC = 0x3333,
		SCORE = 0x3334,
		TUS = 0x3335,
		GUI = 0x3336,
	};

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

	struct player_history
	{
		u64 timestamp{};
		std::set<std::string> communication_ids;
		std::string description;
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
		atomic_t<bool> is_NP_TUS_init     = false; // TODO: savestate
		atomic_t<bool> is_NP_Com2_init    = false; // TODO: savestate

		// NP Handlers/Callbacks
		// Seems to be global
		vm::ptr<SceNpManagerCallback> manager_cb{}; // Connection status and tickets
		vm::ptr<void> manager_cb_arg{};

		atomic_t<bool> basic_handler_registered = false;

		void register_basic_handler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg, bool context_sensitive);
		SceNpCommunicationId get_basic_handler_context();
		void queue_basic_event(basic_event to_queue);
		bool send_basic_event(s32 event, s32 retCode, u32 reqId);
		error_code get_basic_event(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<u8> data, vm::ptr<u32> size);

		// Messages-related functions
		std::optional<shared_ptr<std::pair<std::string, message_data>>> get_message(u64 id);
		void set_message_selected(SceNpBasicAttachmentDataId id, u64 msg_id);
		std::optional<shared_ptr<std::pair<std::string, message_data>>> get_message_selected(SceNpBasicAttachmentDataId id);
		void clear_message_selected(SceNpBasicAttachmentDataId id);
		void send_message(const message_data& msg_data, const std::set<std::string>& npids);

		// Those should probably be under match2 ctx
		vm::ptr<SceNpMatching2RoomEventCallback> room_event_cb{}; // Room events
		u16 room_event_cb_ctx = 0;
		vm::ptr<void> room_event_cb_arg{};
		vm::ptr<SceNpMatching2RoomMessageCallback> room_msg_cb{};
		u16 room_msg_cb_ctx = 0;
		vm::ptr<void> room_msg_cb_arg{};

		// Synchronous requests
		std::vector<SceNpMatching2ServerId> get_match2_server_list(SceNpMatching2ContextId);
		u64 get_network_time();
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
		u32 get_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetRoomMemberDataInternalRequest* req);
		u32 set_roommemberdata_internal(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetRoomMemberDataInternalRequest* req);
		u32 set_userinfo(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SetUserInfoRequest* req);
		u32 get_ping_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SignalingGetPingInfoRequest* req);
		u32 send_room_message(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2SendRoomMessageRequest* req);
		u32 get_lobby_info_list(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, const SceNpMatching2GetLobbyInfoListRequest* req);

		u32 get_match2_event(SceNpMatching2EventKey event_key, u32 dest_addr, u32 size);

		// Old GUI Matching requests
		error_code get_matching_result(u32 ctx_id, u32 req_id, vm::ptr<void> buf, vm::ptr<u32> size, vm::ptr<s32> event);
		error_code get_result_gui(vm::ptr<void> buf, vm::ptr<u32> size, vm::ptr<s32> event);
		error_code create_room_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg);
		error_code join_room_gui(u32 ctx_id, vm::ptr<SceNpRoomId> roomid, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg);
		error_code leave_room_gui(u32 ctx_id, vm::cptr<SceNpRoomId> roomid);
		error_code get_room_list_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::ptr<SceNpMatchingReqRange> range, vm::ptr<SceNpMatchingSearchCondition> cond, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg, bool limit);
		error_code set_room_search_flag_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, s32 flag);
		error_code get_room_search_flag_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id);
		error_code set_room_info_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr);
		error_code get_room_info_gui(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr);
		error_code quickmatch_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, s32 available_num, s32 timeout, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg);
		error_code searchjoin_gui(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingSearchCondition> cond, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg);

		void set_current_gui_ctx_id(u32 id);

		// Score requests
		void transaction_async_handler(std::unique_lock<shared_mutex> lock, const shared_ptr<generic_async_transaction_context>& trans_ctx, u32 req_id, bool async);
		void get_board_infos(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, bool async);
		void record_score(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, vm::cptr<SceNpScoreComment> scoreComment, const u8* data, u32 data_size, vm::ptr<SceNpScoreRankNumber> tmpRank, bool async);
		void record_score_data(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreValue score, u32 totalSize, u32 sendSize, const u8* score_data, bool async);
		void get_score_data(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const SceNpId& npId, vm::ptr<u32> totalSize, u32 recvSize, vm::ptr<void> score_data, bool async);
		void get_score_range(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, SceNpScoreRankNumber startSerialRank, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated);
		void get_score_npid(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, const std::vector<std::pair<SceNpId, s32>>& npid_vec, vm::ptr<SceNpScorePlayerRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated);
		void get_score_friend(shared_ptr<score_transaction_ctx>& trans_ctx, SceNpScoreBoardId boardId, bool include_self, vm::ptr<SceNpScoreRankData> rankArray, u32 rankArraySize, vm::ptr<SceNpScoreComment> commentArray, u32 commentArraySize, vm::ptr<void> infoArray, u32 infoArraySize, u32 arrayNum, vm::ptr<CellRtcTick> lastSortDate, vm::ptr<SceNpScoreRankNumber> totalRecord, bool async, bool deprecated);

		// TUS requests
		void tus_set_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, bool vuser, bool async);
		void tus_get_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusVariable> variableArray, s32 arrayNum, bool vuser, bool async);
		void tus_get_multiuser_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, std::vector<SceNpOnlineId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusVariable> variableArray, s32 arrayNum, bool vuser, bool async);
		void tus_get_friends_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusVariable> variableArray,s32 arrayNum, bool async);
		void tus_add_and_get_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusVariable> outVariable, vm::ptr<SceNpTusAddAndGetVariableOptParam> option, bool vuser, bool async);
		void tus_try_and_set_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusVariable> resultVariable, vm::ptr<SceNpTusTryAndSetVariableOptParam> option, bool vuser, bool async);
		void tus_delete_multislot_variable(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser, bool async);
		void tus_set_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, u32 totalSize, u32 sendSize, vm::cptr<void> data, vm::cptr<SceNpTusDataInfo> info, vm::ptr<SceNpTusSetDataOptParam> option, bool vuser, bool async);
		void tus_get_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> dataStatus, vm::ptr<void> data, u32 recvSize, bool vuser, bool async);
		void tus_get_multislot_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool vuser, bool async);
		void tus_get_multiuser_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, std::vector<SceNpOnlineId> targetNpIdArray, SceNpTusSlotId slotId, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool vuser, bool async);
		void tus_get_friends_data_status(shared_ptr<tus_transaction_ctx>& trans_ctx, SceNpTusSlotId slotId, s32 includeSelf, s32 sortType, vm::ptr<SceNpTusDataStatus> statusArray, s32 arrayNum, bool async);
		void tus_delete_multislot_data(shared_ptr<tus_transaction_ctx>& trans_ctx, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser, bool async);

		// Local functions
		std::pair<error_code, std::optional<SceNpId>> local_get_npid(u64 room_id, u16 member_id);
		std::pair<error_code, std::optional<SceNpMatching2RoomSlotInfo>> local_get_room_slots(SceNpMatching2RoomId room_id);
		std::pair<error_code, std::optional<SceNpMatching2SessionPassword>> local_get_room_password(SceNpMatching2RoomId room_id);
		std::pair<error_code, std::vector<SceNpMatching2RoomMemberId>> local_get_room_memberids(SceNpMatching2RoomId room_id, s32 sort_method);
		error_code local_get_room_member_data(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data, u32 ctx_id);

		// Local GUI functions
		error_code get_room_member_list_local_gui(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<u32> buflen, vm::ptr<void> buf);

		// Friend stuff
		u32 get_num_friends();
		u32 get_num_blocks();
		std::pair<error_code, std::optional<SceNpId>> get_friend_by_index(u32 index);
		void set_presence(std::optional<std::string> status, std::optional<std::vector<u8>> data);

		template <typename T>
		error_code get_friend_presence_by_index(u32 index, SceNpUserInfo* user, T* pres);

		template <typename T>
		error_code get_friend_presence_by_npid(const SceNpId& npid, T* pres);

		// Misc stuff
		void req_ticket(u32 version, const SceNpId* npid, const char* service_id, const u8* cookie, u32 cookie_size, const char* entitlement_id, u32 consumed_count);
		const ticket& get_ticket() const;
		void add_player_to_history(const SceNpId* npid, const char* description);
		u32 add_players_to_history(const SceNpId* npids, const char* description, u32 count);
		u32 get_players_history_count(u32 options);
		bool get_player_history_entry(u32 options, u32 index, SceNpId* npid);
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
		void notif_user_joined_room(vec_stream& noti);
		void notif_user_left_room(vec_stream& noti);
		void notif_room_destroyed(vec_stream& noti);
		void notif_updated_room_data_internal(vec_stream& noti);
		void notif_updated_room_member_data_internal(vec_stream& noti);
		void notif_signaling_helper(vec_stream& noti);
		void notif_room_message_received(vec_stream& noti);

		void generic_gui_notification_handler(vec_stream& noti, std::string_view name, s32 notification_type);

		void notif_member_joined_room_gui(vec_stream& noti);
		void notif_member_left_room_gui(vec_stream& noti);
		void notif_room_disappeared_gui(vec_stream& noti);
		void notif_room_owner_changed_gui(vec_stream& noti);
		void notif_user_kicked_gui(vec_stream& noti);
		void notif_quickmatch_complete_gui(vec_stream& noti);

		// Reply handlers
		void reply_get_world_list(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_create_join_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_join_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_leave_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_search_room(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_roomdata_external_list(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_set_roomdata_external(u32 req_id, rpcn::ErrorType error);
		void reply_get_roomdata_internal(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_set_roomdata_internal(u32 req_id, rpcn::ErrorType error);
		void reply_set_roommemberdata_internal(u32 req_id, rpcn::ErrorType error);
		void reply_get_roommemberdata_internal(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_set_userinfo(u32 req_id, rpcn::ErrorType error);
		void reply_get_ping_info(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_send_room_message(u32 req_id, rpcn::ErrorType error);
		void reply_req_sign_infos(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_req_ticket(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_board_infos(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_record_score(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_record_score_data(u32 req_id, rpcn::ErrorType error);
		void reply_get_score_data(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_score_range(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_score_friends(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_score_npid(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_set_multislot_variable(u32 req_id, rpcn::ErrorType error);
		void reply_tus_get_multislot_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_get_multiuser_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_get_friends_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_add_and_get_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_try_and_set_variable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_delete_multislot_variable(u32 req_id, rpcn::ErrorType error);
		void reply_tus_set_data(u32 req_id, rpcn::ErrorType error);
		void reply_tus_get_data(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_get_multislot_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_get_multiuser_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_get_friends_data_status(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_tus_delete_multislot_data(u32 req_id, rpcn::ErrorType error);
		void reply_create_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_join_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_leave_room_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_get_room_list_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_set_room_search_flag_gui(u32 req_id, rpcn::ErrorType error);
		void reply_get_room_search_flag_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_set_room_info_gui(u32 req_id, rpcn::ErrorType error);
		void reply_get_room_info_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_quickmatch_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void reply_searchjoin_gui(u32 req_id, rpcn::ErrorType error, vec_stream& reply);

		// Helper functions
		std::pair<bool, bool> get_match2_context_options(u32 ctx_id);
		void handle_GetScoreResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply, bool simple_result = false);
		void handle_tus_no_data(u32 req_id, rpcn::ErrorType error);
		void handle_TusVarResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void handle_TusVariable(u32 req_id, rpcn::ErrorType error, vec_stream& reply);
		void handle_TusDataStatusResponse(u32 req_id, rpcn::ErrorType error, vec_stream& reply);

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
					sysutil_register_cb([=, ctx_id = this->ctx_id, event_type = this->event_type, cb = this->cb, cb_arg = this->cb_arg](ppu_thread& cb_ppu) -> s32
					{
						cb(cb_ppu, ctx_id, req_id, event_type, event_key, error_code, data_size, cb_arg);
						return 0;
					});
				}
			}
		};

		u32 generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, SceNpMatching2Event event_type);
		std::optional<callback_info> take_pending_request(u32 req_id);

	private:
		shared_mutex mutex_pending_requests;
		std::unordered_map<u32, callback_info> pending_requests;
		shared_mutex mutex_pending_sign_infos_requests;
		std::unordered_map<u32, u32> pending_sign_infos_requests;

		shared_mutex mutex_queue_basic_events;
		std::queue<basic_event> queue_basic_events;

		bool m_inited_np_handler_dependencies = false;

		// Basic event handler;
		struct
		{
			shared_mutex mutex;
			SceNpCommunicationId context{};
			vm::ptr<SceNpBasicEventHandler> handler_func;
			vm::ptr<void> handler_arg;
			bool context_sensitive = false;
		} basic_handler;

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
		gui_cache_manager gui_cache;

		// Messages related
		std::optional<u64> selected_invite_id{};
		std::optional<u64> selected_message_id{};

		// Misc
		s64 network_time_offset = 0;

		// Requests(reqEventKey : data)
		shared_mutex mutex_match2_req_results;
		std::unordered_map<u32, event_data> match2_req_results;
		atomic_t<u16> match2_low_reqid_cnt = 1;
		atomic_t<u32> match2_event_cnt     = 1;
		u32 get_req_id(u16 app_req)
		{
			return ((app_req << 16) | match2_low_reqid_cnt.fetch_add(1));
		}

		u32 get_req_id(REQUEST_ID_HIGH int_req)
		{
			return ((static_cast<u16>(int_req) << 16) | match2_low_reqid_cnt.fetch_add(1));
		}

		u32 get_event_key()
		{
			return match2_event_cnt.fetch_add(1);
		}
		event_data& allocate_req_result(u32 event_key, u32 max_size, u32 initial_size);

		// GUI result
		struct
		{
			shared_mutex mutex;
			s32 event = 0;
			event_data data;
		} gui_result;

		void set_gui_result(s32 event, event_data data);

		// GUI notifications
		struct gui_notification
		{
			s32 event = 0;
			event_data edata;
		};

		struct
		{
			shared_mutex mutex;
			std::map<std::pair<u32, u32>, gui_notification> list; // (ctx_id, req_id), notif
			u32 counter_req_id = 1;
			u32 current_gui_ctx_id = 0;
		} gui_notifications;

		// Async transaction threads
		shared_mutex mutex_async_transactions;
		std::unordered_map<u32, shared_ptr<generic_async_transaction_context>> async_transactions; // (req_id, transaction_ctx)

		// RPCN
		shared_mutex mutex_rpcn;
		std::shared_ptr<rpcn::rpcn_client> rpcn;
		std::shared_ptr<rpcn::rpcn_client> get_rpcn();

		// UPNP
		upnp_handler upnp;

		// Presence
		struct
		{
			SceNpCommunicationId pr_com_id;
			std::string pr_title;
			std::string pr_status;
			std::string pr_comment;
			std::vector<u8> pr_data;
			atomic_t<bool> advertised = false;
		} presence_self;

		player_history& get_player_and_set_timestamp(const SceNpId& npid, u64 timestamp);
		void save_players_history();

		shared_mutex mutex_history;
		std::map<std::string, player_history> players_history; // npid / history

		struct
		{
			shared_mutex mutex;
			std::map<u32, u32> list; // req_id / ctx_id
		} gui_requests;

		void add_gui_request(u32 req_id, u32 ctx_id);
		void remove_gui_request(u32 req_id);
		u32 take_gui_request(u32 req_id);
		shared_ptr<matching_ctx> take_pending_gui_request(u32 req_id);

		shared_mutex mutex_quickmatching;
		std::map<SceNpRoomId, u32> pending_quickmatching;
	};
} // namespace np
