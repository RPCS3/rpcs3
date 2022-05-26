#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <semaphore>
#include "Utilities/mutex.h"
#include "Emu/localized_string.h"

#include "util/asm.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

#include "generated/np2_structs_generated.h"

#include <wolfssl/ssl.h>

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

	// Getters

	template <typename T>
	T get()
	{
		if (sizeof(T) + i > vec.size())
		{
			error = true;
			return 0;
		}
		T res = *utils::bless<le_t<T, 1>>(&vec[i]);
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
		std::vector<u8> ret;
		u32 size = get<u32>();

		if ((vec.begin() + i + size) <= vec.end())
			std::copy(vec.begin() + i, vec.begin() + i + size, std::back_inserter(ret));
		else
			error = true;

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
	usz i      = 0;
	bool error = false;
};

namespace rpcn
{
	enum CommandType : u16
	{
		Login,
		Terminate,
		Create,
		SendToken,
		AddFriend,
		RemoveFriend,
		AddBlock,
		RemoveBlock,
		GetServerList,
		GetWorldList,
		CreateRoom,
		JoinRoom,
		LeaveRoom,
		SearchRoom,
		GetRoomDataExternalList,
		SetRoomDataExternal,
		GetRoomDataInternal,
		SetRoomDataInternal,
		SetRoomMemberDataInternal,
		PingRoomOwner,
		SendRoomMessage,
		RequestSignalingInfos,
		RequestTicket,
		SendMessage,
	};

	enum NotificationType : u16
	{
		UserJoinedRoom,
		UserLeftRoom,
		RoomDestroyed,
		UpdatedRoomDataInternal,
		UpdatedRoomMemberDataInternal,
		SignalP2PConnect,
		_SignalP2PDisconnect,
		FriendQuery,  // Other user sent a friend request
		FriendNew,    // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
		FriendLost,   // Remove friend from the friendlist(user removed friend or friend removed friend)
		FriendStatus, // Set status of friend to Offline or Online
		RoomMessageReceived,
		MessageReceived,
	};

	enum class rpcn_state
	{
		failure_no_failure,
		failure_input,
		failure_wolfssl,
		failure_resolve,
		failure_connect,
		failure_id,
		failure_id_already_logged_in,
		failure_id_username,
		failure_id_password,
		failure_id_token,
		failure_protocol,
		failure_other,
	};

	enum PacketType : u8
	{
		Request,
		Reply,
		Notification,
		ServerInfo,
	};

	enum ErrorType : u8
	{
		NoError,                     // No error
		Malformed,                   // Query was malformed, critical error that should close the connection
		Invalid,                     // The request type is invalid(wrong stage?)
		InvalidInput,                // The Input doesn't fit the constraints of the request
		TooSoon,                     // Time limited operation attempted too soon
		LoginError,                  // An error happened related to login
		LoginAlreadyLoggedIn,        // Can't log in because you're already logged in
		LoginInvalidUsername,        // Invalid username
		LoginInvalidPassword,        // Invalid password
		LoginInvalidToken,           // Invalid token
		CreationError,               // An error happened related to account creation
		CreationExistingUsername,    // Specific
		CreationBannedEmailProvider, // Specific to Account Creation: the email provider is banned
		CreationExistingEmail,       // Specific to Account Creation: that email is already registered to an account
		AlreadyJoined,               // User tried to join a room he's already part of
		Unauthorized,                // User attempted an unauthorized operation
		DbFail,                      // Generic failure on db side
		EmailFail,                   // Generic failure related to email
		NotFound,                    // Object of the query was not found(room, user, etc)
		Blocked,                     // The operation can't complete because you've been blocked
		AlreadyFriend,               // Can't add friend because already friend
		Unsupported,
		__error_last
	};

	using friend_cb_func  = void (*)(void* param, NotificationType ntype, const std::string& username, bool status);
	using message_cb_func = void (*)(void* param, const std::shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id);

	struct friend_data
	{
		std::map<std::string, std::pair<bool, u64>> friends;
		std::set<std::string> requests_sent;
		std::set<std::string> requests_received;
		std::set<std::string> blocked;
	};

	localized_string_id rpcn_state_to_localized_string_id(rpcn::rpcn_state state);
	std::string rpcn_state_to_string(rpcn::rpcn_state state);

	class rpcn_client
	{
	private:
		static inline std::weak_ptr<rpcn_client> instance;
		static inline shared_mutex inst_mutex;

		atomic_t<bool> connected    = false;
		atomic_t<bool> authentified = false;
		atomic_t<bool> want_conn    = false;
		atomic_t<bool> want_auth    = false;
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

		void handle_friend_notification(u16 command, std::vector<u8> data);

		void handle_message(std::vector<u8> data);

	private:
		rpcn_client();

		void rpcn_reader_thread();
		void rpcn_writer_thread();
		void rpcn_thread();

		bool handle_input();
		bool handle_output();

		void add_packet(const std::vector<u8> packet);

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
		rpcn_client(rpcn_client& other) = delete;
		void operator=(const rpcn_client&) = delete;
		static std::shared_ptr<rpcn_client> get_instance();
		rpcn_state wait_for_connection();
		rpcn_state wait_for_authentified();

		void get_friends_and_register_cb(friend_data& friend_infos, friend_cb_func cb_func, void* cb_param);
		void remove_friend_cb(friend_cb_func, void* cb_param);

		ErrorType create_user(const std::string& npid, const std::string& password, const std::string& online_name, const std::string& avatar_url, const std::string& email);
		ErrorType resend_token(const std::string& npid, const std::string& password);
		bool add_friend(const std::string& friend_username);
		bool remove_friend(const std::string& friend_username);

		u32 get_num_friends();
		u32 get_num_blocks();
		std::optional<std::string> get_friend_by_index(u32 index);

		std::vector<std::pair<u16, std::vector<u8>>> get_notifications();
		std::unordered_map<u32, std::pair<u16, std::vector<u8>>> get_replies();

		std::vector<u64> get_new_messages();
		std::optional<std::shared_ptr<std::pair<std::string, message_data>>> get_message(u64 id);
		std::vector<std::pair<u64, std::shared_ptr<std::pair<std::string, message_data>>>> get_messages_and_register_cb(SceNpBasicMessageMainType type, bool include_bootable, message_cb_func cb_func, void* cb_param);
		void remove_message_cb(message_cb_func cb_func, void* cb_param);
		void discard_active_message(u64 id);

		bool is_connected() const
		{
			return connected;
		}
		bool is_authentified() const
		{
			return authentified;
		}
		rpcn_state get_rpcn_state() const
		{
			return state;
		}

		void server_infos_updated();

		// Synchronous requests
		bool get_server_list(u32 req_id, const SceNpCommunicationId& communication_id, std::vector<u16>& server_list);
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
		bool set_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomMemberDataInternalRequest* req);
		bool ping_room_owner(u32 req_id, const SceNpCommunicationId& communication_id, u64 room_id);
		bool send_room_message(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SendRoomMessageRequest* req);
		bool req_sign_infos(u32 req_id, const std::string& npid);
		bool req_ticket(u32 req_id, const std::string& service_id, const std::vector<u8>& cookie);
		bool sendmessage(const message_data& msg_data, const std::set<std::string>& npids);

		const std::string& get_online_name() const
		{
			return online_name;
		}
		const std::string& get_avatar_url() const
		{
			return avatar_url;
		}

		u32 get_addr_sig() const
		{
			return addr_sig.load();
		}
		u16 get_port_sig() const
		{
			return port_sig.load();
		}

	private:
		bool get_reply(u64 expected_id, std::vector<u8>& data);

		std::vector<u8> forge_request(u16 command, u64 packet_id, const std::vector<u8>& data) const;
		bool forge_send(u16 command, u64 packet_id, const std::vector<u8>& data);
		bool forge_send_reply(u16 command, u64 packet_id, const std::vector<u8>& data, std::vector<u8>& reply_data);

		bool is_error(ErrorType err) const;
		bool error_and_disconnect(const std::string& error_mgs);

		std::string get_wolfssl_error(WOLFSSL* wssl, int error) const;

	private:
		WOLFSSL_CTX* wssl_ctx = nullptr;
		WOLFSSL* read_wssl    = nullptr;
		WOLFSSL* write_wssl   = nullptr;

		atomic_t<bool> server_info_received = false;
		u32 received_version                = 0;

		// UDP Signaling related
		steady_clock::time_point last_ping_time{}, last_pong_time{};

		sockaddr_in addr_rpcn{};
		sockaddr_in addr_rpcn_udp{};
		int sockfd = 0;

		atomic_t<u64> rpcn_request_counter = 0x100000001; // Counter used for commands whose result is not forwarded to NP handler(login, create, sendmessage, etc)

		shared_mutex mutex_notifs, mutex_replies, mutex_replies_sync;
		std::vector<std::pair<u16, std::vector<u8>>> notifications;            // notif type / data
		std::unordered_map<u32, std::pair<u16, std::vector<u8>>> replies;      // req id / (command / data)
		std::unordered_map<u64, std::pair<u16, std::vector<u8>>> replies_sync; // same but for sync replies(Login, Create, GetServerList)

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
		std::unordered_map<u64, std::shared_ptr<std::pair<std::string, message_data>>> messages; // msg id / (sender / message)
		std::set<u64> active_messages;                                                           // msg id of messages that have not been discarded
		std::vector<u64> new_messages;                                                           // list of msg_id used to inform np_handler of new messages
		u64 message_counter = 3;                                                                 // id counter

		std::string online_name{};
		std::string avatar_url{};

		s64 user_id = 0;

		atomic_t<u32> addr_sig{};
		atomic_t<u16> port_sig{};
	};

} // namespace rpcn
