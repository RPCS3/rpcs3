#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <semaphore>
#include "Utilities/mutex.h"
#include "Emu/localized_string_id.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/sceNpTus.h"
#include <flatbuffers/flatbuffers.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wextern-c-compat"
#endif
#include <wolfssl/ssl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "rpcn_types.h"

// COMID is sent as 9 chars - + '_' + 2 digits
constexpr usz COMMUNICATION_ID_COMID_COMPONENT_SIZE = 9;
constexpr usz COMMUNICATION_ID_SUBID_COMPONENT_SIZE = 2;
constexpr usz COMMUNICATION_ID_SIZE = COMMUNICATION_ID_COMID_COMPONENT_SIZE + COMMUNICATION_ID_SUBID_COMPONENT_SIZE + 1;

class vec_stream
{
public:
	vec_stream() = delete;
	vec_stream(std::vector<u8>& _vec, usz initial_index = 0)
		: vec(_vec), i(initial_index) {}
	bool is_error() const
	{
		return error;
	}

	void dump() const;

	// Getters

	template <typename T>
	T get()
	{
		if (sizeof(T) + i > vec.size() || error)
		{
			error = true;
			return static_cast<T>(0);
		}
		T res = read_from_ptr<le_t<T>>(&vec[i]);
		i += sizeof(T);
		return res;
	}
	std::string get_string(bool empty)
	{
		std::string res{};
		while (i < vec.size() && vec[i] != 0)
		{
			res.push_back(vec[i]);
			i++;
		}
		i++;

		if (!empty && res.empty())
		{
			error = true;
		}

		return res;
	}
	std::vector<u8> get_rawdata()
	{
		u32 size = get<u32>();

		if (i + size > vec.size())
		{
			error = true;
			return {};
		}

		std::vector<u8> ret;
		std::copy(vec.begin() + i, vec.begin() + i + size, std::back_inserter(ret));
		i += size;
		return ret;
	}

	SceNpCommunicationId get_com_id()
	{
		if (i + COMMUNICATION_ID_SIZE > vec.size() || error)
		{
			error = true;
			return {};
		}

		SceNpCommunicationId com_id{};
		std::memcpy(&com_id.data[0], &vec[i], COMMUNICATION_ID_COMID_COMPONENT_SIZE);
		const std::string sub_id(reinterpret_cast<const char*>(&vec[i + COMMUNICATION_ID_COMID_COMPONENT_SIZE + 1]), COMMUNICATION_ID_SUBID_COMPONENT_SIZE);
		const unsigned long result_num = std::strtoul(sub_id.c_str(), nullptr, 10);

		if (result_num > 99)
		{
			error = true;
			return {};
		}

		com_id.num = static_cast<u8>(result_num);
		i += COMMUNICATION_ID_SIZE;
		return com_id;
	}

	template <typename T>
	const T* get_flatbuffer()
	{
		auto rawdata_vec = get_rawdata();

		if (error)
			return nullptr;

		if (vec.empty())
		{
			error = true;
			return nullptr;
		}

		const T* ret = flatbuffers::GetRoot<T>(rawdata_vec.data());
		flatbuffers::Verifier verifier(rawdata_vec.data(), rawdata_vec.size());

		if (!ret->Verify(verifier))
		{
			error = true;
			return nullptr;
		}

		aligned_bufs.push_back(std::move(rawdata_vec));

		return ret;
	}

	// Setters

	template <typename T>
	void insert(T value)
	{
		value = std::bit_cast<le_t<T>, T>(value);
		// resize + memcpy instead?
		for (usz index = 0; index < sizeof(T); index++)
		{
			vec.push_back(*(reinterpret_cast<u8*>(&value) + index));
		}
	}
	void insert_string(const std::string& str) const
	{
		std::copy(str.begin(), str.end(), std::back_inserter(vec));
		vec.push_back(0);
	}

protected:
	std::vector<u8>& vec;
	std::vector<std::vector<u8>> aligned_bufs;
	usz i      = 0;
	bool error = false;
};

namespace rpcn
{
	using friend_cb_func  = void (*)(void* param, NotificationType ntype, const std::string& username, bool status);
	using message_cb_func = void (*)(void* param, const shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id);

	struct friend_online_data
	{
		friend_online_data(bool online, SceNpCommunicationId&& pr_com_id, std::string&& pr_title, std::string&& pr_status, std::string&& pr_comment, std::vector<u8>&& pr_data)
			: online(online), timestamp(0), pr_com_id(pr_com_id), pr_title(pr_title), pr_status(pr_status), pr_comment(pr_comment), pr_data(pr_data) {}

		friend_online_data(bool online, u64 timestamp)
			: online(online), timestamp(timestamp) {}

		void dump() const;

		bool online = false;
		u64 timestamp = 0;
		SceNpCommunicationId pr_com_id{};
		std::string pr_title;
		std::string pr_status;
		std::string pr_comment;
		std::vector<u8> pr_data;
	};

	struct friend_data
	{
		std::map<std::string, friend_online_data> friends;
		std::set<std::string> requests_sent;
		std::set<std::string> requests_received;
		std::set<std::string> blocked;
	};

	localized_string_id rpcn_state_to_localized_string_id(rpcn::rpcn_state state);
	std::string rpcn_state_to_string(rpcn::rpcn_state state);
	void print_error(rpcn::CommandType command, rpcn::ErrorType error);

	class rpcn_client
	{
	private:
		static inline std::weak_ptr<rpcn_client> instance;
		static inline shared_mutex inst_mutex;

		atomic_t<bool> connected    = false;
		atomic_t<bool> authentified = false;
		atomic_t<bool> want_conn    = false;
		atomic_t<bool> want_auth    = false;
		u32 binding_address = 0;
		std::binary_semaphore sem_connected, sem_authentified;
		std::mutex mutex_connected, mutex_authentified;

		std::binary_semaphore sem_reader, sem_writer, sem_rpcn;
		std::mutex mutex_read, mutex_write;

		std::thread thread_rpcn, thread_rpcn_reader, thread_rpcn_writer;

		atomic_t<bool> terminate = false;

		atomic_t<u32> num_failures = 0;
		atomic_t<rpcn_state> state = rpcn_state::failure_no_failure;

		std::vector<std::vector<u8>> packets_to_send;
		std::mutex mutex_packets_to_send;

		// Friends related
		shared_mutex mutex_friends;
		std::set<std::pair<friend_cb_func, void*>> friend_cbs;
		friend_data friend_infos;

		void handle_friend_notification(rpcn::NotificationType ntype, std::vector<u8> data);

		void handle_message(std::vector<u8> data);

	private:
		rpcn_client(u32 binding_address);

		void rpcn_reader_thread();
		void rpcn_writer_thread();
		void rpcn_thread();

		bool handle_input();
		bool handle_output();

		void add_packet(std::vector<u8> packet);

	private:
		enum class recvn_result
		{
			recvn_success,
			recvn_nodata,
			recvn_timeout,
			recvn_noconn,
			recvn_terminate,
			recvn_fatal,
		};

		recvn_result recvn(u8* buf, usz n);
		bool send_packet(const std::vector<u8>& packet);

	private:
		bool connect(const std::string& host);
		bool login(const std::string& npid, const std::string& password, const std::string& token);
		void disconnect();

	public:
		~rpcn_client();
		rpcn_client(rpcn_client& other)    = delete;
		void operator=(const rpcn_client&) = delete;
		static std::shared_ptr<rpcn_client> get_instance(u32 binding_address, bool check_config = false);
		rpcn_state wait_for_connection();
		rpcn_state wait_for_authentified();
		bool terminate_connection();
		void reset_state();

		void get_friends(friend_data& friend_infos);
		void get_friends_and_register_cb(friend_data& friend_infos, friend_cb_func cb_func, void* cb_param);
		void register_friend_cb(friend_cb_func, void* cb_param);
		void remove_friend_cb(friend_cb_func, void* cb_param);

		ErrorType create_user(std::string_view npid, std::string_view password, std::string_view online_name, std::string_view avatar_url, std::string_view email);
		ErrorType resend_token(const std::string& npid, const std::string& password);
		ErrorType send_reset_token(std::string_view npid, std::string_view email);
		ErrorType reset_password(std::string_view npid, std::string_view token, std::string_view password);
		bool add_friend(const std::string& friend_username);
		bool remove_friend(const std::string& friend_username);

		u32 get_num_friends();
		u32 get_num_blocks();
		std::optional<std::string> get_friend_by_index(u32 index);
		std::optional<std::pair<std::string, friend_online_data>> get_friend_presence_by_index(u32 index);
		std::optional<std::pair<std::string, friend_online_data>> get_friend_presence_by_npid(const std::string& npid);

		std::vector<std::pair<rpcn::NotificationType, std::vector<u8>>> get_notifications();
		std::unordered_map<u32, std::pair<rpcn::CommandType, std::vector<u8>>> get_replies();
		std::unordered_map<std::string, friend_online_data> get_presence_updates();
		std::map<std::string, friend_online_data> get_presence_states();

		std::vector<u64> get_new_messages();
		std::optional<shared_ptr<std::pair<std::string, message_data>>> get_message(u64 id);
		std::vector<std::pair<u64, shared_ptr<std::pair<std::string, message_data>>>> get_messages_and_register_cb(SceNpBasicMessageMainType type, bool include_bootable, message_cb_func cb_func, void* cb_param);
		void remove_message_cb(message_cb_func cb_func, void* cb_param);
		void mark_message_used(u64 id);

		bool is_connected() const;
		bool is_authentified() const;
		rpcn_state get_rpcn_state() const;

		void server_infos_updated();

		// Synchronous requests
		bool get_server_list(u32 req_id, const SceNpCommunicationId& communication_id, std::vector<u16>& server_list);
		u64 get_network_time(u32 req_id);
		// Asynchronous requests
		bool get_world_list(u32 req_id, const SceNpCommunicationId& communication_id, u16 server_id);
		bool createjoin_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2CreateJoinRoomRequest* req);
		bool join_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2JoinRoomRequest* req);
		bool leave_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2LeaveRoomRequest* req);
		bool search_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SearchRoomRequest* req);
		bool get_roomdata_external_list(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataExternalListRequest* req);
		bool set_roomdata_external(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataExternalRequest* req);
		bool get_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataInternalRequest* req);
		bool set_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataInternalRequest* req);
		bool get_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomMemberDataInternalRequest* req);
		bool set_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomMemberDataInternalRequest* req);
		bool set_userinfo(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetUserInfoRequest* req);
		bool ping_room_owner(u32 req_id, const SceNpCommunicationId& communication_id, u64 room_id);
		bool send_room_message(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SendRoomMessageRequest* req);
		bool req_sign_infos(u32 req_id, const std::string& npid);
		bool req_ticket(u32 req_id, const std::string& service_id, const std::vector<u8>& cookie);
		bool send_message(const message_data& msg_data, const std::set<std::string>& npids);
		bool get_board_infos(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id);
		bool record_score(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, SceNpScorePcId char_id, SceNpScoreValue score, const std::optional<std::string> comment, const std::optional<std::vector<u8>> score_data);
		bool get_score_range(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, u32 start_rank, u32 num_rank, bool with_comment, bool with_gameinfo);
		bool get_score_npid(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, const std::vector<std::pair<SceNpId, s32>>& npids, bool with_comment, bool with_gameinfo);
		bool get_score_friend(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, bool include_self, bool with_comment, bool with_gameinfo, u32 max_entries);
		bool record_score_data(u32 req_id, const SceNpCommunicationId& communication_id,  SceNpScorePcId pc_id, SceNpScoreBoardId board_id, s64 score, const std::vector<u8>& score_data);
		bool get_score_data(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScorePcId pc_id, SceNpScoreBoardId board_id, const SceNpId& npid);
		bool tus_set_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, bool vuser);
		bool tus_get_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser);
		bool tus_get_multiuser_variable(u32 req_id, const SceNpCommunicationId& communication_id, const std::vector<SceNpOnlineId>& targetNpIdArray, SceNpTusSlotId slotId, s32 arrayNum, bool vuser);
		bool tus_get_friends_variable(u32 req_id, const SceNpCommunicationId& communication_id, SceNpTusSlotId slotId, bool includeSelf, s32 sortType, s32 arrayNum);
		bool tus_add_and_get_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusAddAndGetVariableOptParam> option, bool vuser);
		bool tus_try_and_set_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusTryAndSetVariableOptParam> option, bool vuser);
		bool tus_delete_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser);
		bool tus_set_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, const std::vector<u8>& tus_data, vm::cptr<SceNpTusDataInfo> info, vm::ptr<SceNpTusSetDataOptParam> option, bool vuser);
		bool tus_get_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, bool vuser);
		bool tus_get_multislot_data_status(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser);
		bool tus_get_multiuser_data_status(u32 req_id, SceNpCommunicationId& communication_id, const std::vector<SceNpOnlineId>& targetNpIdArray, SceNpTusSlotId slotId, s32 arrayNum, bool vuser);
		bool tus_get_friends_data_status(u32 req_id, SceNpCommunicationId& communication_id, SceNpTusSlotId slotId, bool includeSelf, s32 sortType, s32 arrayNum);
		bool tus_delete_multislot_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser);
		bool send_presence(const SceNpCommunicationId& pr_com_id, const std::string& pr_title, const std::string& pr_status, const std::string& pr_comment, const std::vector<u8>& pr_data);
		bool createjoin_room_gui(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatchingAttr* attr_list);
		bool join_room_gui(u32 req_id, const SceNpRoomId& room_id);
		bool leave_room_gui(u32 req_id, const SceNpRoomId& room_id);
		bool get_room_list_gui(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatchingReqRange* range, vm::ptr<SceNpMatchingSearchCondition> cond, vm::ptr<SceNpMatchingAttr> attr);
		bool set_room_search_flag_gui(u32 req_id, const SceNpRoomId& room_id, bool stealth);
		bool get_room_search_flag_gui(u32 req_id, const SceNpRoomId& room_id);
		bool set_room_info_gui(u32 req_id, const SceNpRoomId& room_id, vm::ptr<SceNpMatchingAttr> attrs);
		bool get_room_info_gui(u32 req_id, const SceNpRoomId& room_id, vm::ptr<SceNpMatchingAttr> attrs);
		bool quickmatch_gui(u32 req_id, const SceNpCommunicationId& com_id, vm::cptr<SceNpMatchingSearchCondition> cond, s32 available_num);
		bool searchjoin_gui(u32 req_id, const SceNpCommunicationId& com_id, vm::cptr<SceNpMatchingSearchCondition> cond, vm::cptr<SceNpMatchingAttr> attr);

		const std::string& get_online_name() const;
		const std::string& get_avatar_url() const;

		u32 get_addr_sig() const;
		u16 get_port_sig() const;
		u32 get_addr_local() const;
		void update_local_addr(u32 addr);

	private:
		bool get_reply(u64 expected_id, std::vector<u8>& data);

		static void write_communication_id(const SceNpCommunicationId& com_id, std::vector<u8>& data);

		std::vector<u8> forge_request(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data) const;
		bool forge_send(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data);
		bool forge_request_with_com_id(const flatbuffers::FlatBufferBuilder& builder, const SceNpCommunicationId& com_id, CommandType command, u64 packet_id);
		bool forge_request_with_data(const flatbuffers::FlatBufferBuilder& builder, CommandType command, u64 packet_id);
		bool forge_send_reply(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data, std::vector<u8>& reply_data);

		bool error_and_disconnect(const std::string& error_mgs);
		bool error_and_disconnect_notice(const std::string& error_msg);

		std::string get_wolfssl_error(WOLFSSL* wssl, int error) const;

	private:
		WOLFSSL_CTX* wssl_ctx = nullptr;
		WOLFSSL* read_wssl    = nullptr;
		WOLFSSL* write_wssl   = nullptr;

		atomic_t<bool> server_info_received = false;
		u32 received_version                = 0;

		sockaddr_in addr_rpcn{};
		sockaddr_in addr_rpcn_udp_ipv4{};
		sockaddr_in6 addr_rpcn_udp_ipv6{};
#ifdef _WIN32
		SOCKET sockfd = 0;
#else
		int sockfd = 0;
#endif

		atomic_t<u64> rpcn_request_counter = 0x100000001; // Counter used for commands whose result is not forwarded to NP handler(login, create, sendmessage, etc)

		shared_mutex mutex_notifs, mutex_replies, mutex_replies_sync, mutex_presence_updates;
		std::vector<std::pair<rpcn::NotificationType, std::vector<u8>>> notifications;       // notif type / data
		std::unordered_map<u32, std::pair<rpcn::CommandType, std::vector<u8>>> replies;      // req id / (command / data)
		std::unordered_map<u64, std::pair<rpcn::CommandType, std::vector<u8>>> replies_sync; // same but for sync replies(see handle_input())
		std::unordered_map<std::string, friend_online_data> presence_updates;                // npid / presence data

		// Messages
		struct message_cb_t
		{
			message_cb_func cb_func;
			void* cb_param;
			SceNpBasicMessageMainType type_filter;
			bool inc_bootable;

			bool operator<(const message_cb_t& other) const
			{
				const void* void_cb_func       = reinterpret_cast<const void*>(cb_func);
				const void* void_other_cb_func = reinterpret_cast<const void*>(other.cb_func);
				return (void_cb_func < void_other_cb_func) || ((!(void_other_cb_func < void_cb_func)) && (cb_param < other.cb_param));
			}
		};
		shared_mutex mutex_messages;
		std::set<message_cb_t> message_cbs;
		std::unordered_map<u64, shared_ptr<std::pair<std::string, message_data>>> messages; // msg id / (sender / message)
		std::set<u64> active_messages;                                                           // msg id of messages that have not been discarded
		std::vector<u64> new_messages;                                                           // list of msg_id used to inform np_handler of new messages
		u64 message_counter = 3;                                                                 // id counter

		std::string online_name{};
		std::string avatar_url{};

		s64 user_id = 0;

		atomic_t<u32> addr_sig = 0;
		atomic_t<u32> port_sig = 0; // Stores an u16, u32 is used for atomic_t::wait convenience
		atomic_t<u32> local_addr_sig = 0;

		static constexpr int RPCN_TIMEOUT_INTERVAL = 50; // 50ms
		static constexpr int RPCN_TIMEOUT = 10'000;      // 10s
	};

} // namespace rpcn
