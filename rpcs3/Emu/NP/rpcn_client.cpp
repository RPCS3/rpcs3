#include "stdafx.h"
#include "Emu/Cell/lv2/sys_net/sys_net_helpers.h"
#include "Emu/NP/ip_address.h"
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include "rpcn_client.h"
#include "Utilities/StrUtil.h"
#include "Utilities/StrFmt.h"
#include "Utilities/Thread.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/vport0.h"
#include "Emu/system_config.h"
#include "Emu/RSX/Overlays/overlay_message.h"

#include "generated/np2_structs_generated.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <netdb.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

LOG_CHANNEL(rpcn_log, "rpcn");

template <>
void fmt_class_string<rpcn::ErrorType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case rpcn::ErrorType::NoError: return "NoError";
			case rpcn::ErrorType::Malformed: return "Malformed";
			case rpcn::ErrorType::Invalid: return "Invalid";
			case rpcn::ErrorType::InvalidInput: return "InvalidInput";
			case rpcn::ErrorType::TooSoon: return "TooSoon";
			case rpcn::ErrorType::LoginError: return "LoginError";
			case rpcn::ErrorType::LoginAlreadyLoggedIn: return "LoginAlreadyLoggedIn";
			case rpcn::ErrorType::LoginInvalidUsername: return "LoginInvalidUsername";
			case rpcn::ErrorType::LoginInvalidPassword: return "LoginInvalidPassword";
			case rpcn::ErrorType::LoginInvalidToken: return "LoginInvalidToken";
			case rpcn::ErrorType::CreationError: return "CreationError";
			case rpcn::ErrorType::CreationExistingUsername: return "CreationExistingUsername";
			case rpcn::ErrorType::CreationBannedEmailProvider: return "CreationBannedEmailProvider";
			case rpcn::ErrorType::CreationExistingEmail: return "CreationExistingEmail";
			case rpcn::ErrorType::RoomMissing: return "RoomMissing";
			case rpcn::ErrorType::RoomAlreadyJoined: return "RoomAlreadyJoined";
			case rpcn::ErrorType::RoomFull: return "RoomFull";
			case rpcn::ErrorType::RoomPasswordMismatch: return "RoomPasswordMismatch";
			case rpcn::ErrorType::RoomPasswordMissing: return "RoomPasswordMissing";
			case rpcn::ErrorType::RoomGroupNoJoinLabel: return "RoomGroupNoJoinLabel";
			case rpcn::ErrorType::RoomGroupFull: return "RoomGroupFull";
			case rpcn::ErrorType::RoomGroupJoinLabelNotFound: return "RoomGroupJoinLabelNotFound";
			case rpcn::ErrorType::RoomGroupMaxSlotMismatch: return "RoomGroupMaxSlotMismatch";
			case rpcn::ErrorType::Unauthorized: return "Unauthorized";
			case rpcn::ErrorType::DbFail: return "DbFail";
			case rpcn::ErrorType::EmailFail: return "EmailFail";
			case rpcn::ErrorType::NotFound: return "NotFound";
			case rpcn::ErrorType::Blocked: return "Blocked";
			case rpcn::ErrorType::AlreadyFriend: return "AlreadyFriend";
			case rpcn::ErrorType::ScoreNotBest: return "ScoreNotBest";
			case rpcn::ErrorType::ScoreInvalid: return "ScoreInvalid";
			case rpcn::ErrorType::ScoreHasData: return "ScoreHasData";
			case rpcn::ErrorType::CondFail: return "CondFail";
			case rpcn::ErrorType::Unsupported: return "Unsupported";
			default: break;
			}

			return unknown;
		});
}

template <>
void fmt_class_string<rpcn::CommandType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case rpcn::CommandType::Login: return "Login";
			case rpcn::CommandType::Terminate: return "Terminate";
			case rpcn::CommandType::Create: return "Create";
			case rpcn::CommandType::SendToken: return "SendToken";
			case rpcn::CommandType::SendResetToken: return "SendResetToken";
			case rpcn::CommandType::ResetPassword: return "ResetPassword";
			case rpcn::CommandType::ResetState: return "ResetState";
			case rpcn::CommandType::AddFriend: return "AddFriend";
			case rpcn::CommandType::RemoveFriend: return "RemoveFriend";
			case rpcn::CommandType::AddBlock: return "AddBlock";
			case rpcn::CommandType::RemoveBlock: return "RemoveBlock";
			case rpcn::CommandType::GetServerList: return "GetServerList";
			case rpcn::CommandType::GetWorldList: return "GetWorldList";
			case rpcn::CommandType::CreateRoom: return "CreateRoom";
			case rpcn::CommandType::JoinRoom: return "JoinRoom";
			case rpcn::CommandType::LeaveRoom: return "LeaveRoom";
			case rpcn::CommandType::SearchRoom: return "SearchRoom";
			case rpcn::CommandType::GetRoomDataExternalList: return "GetRoomDataExternalList";
			case rpcn::CommandType::SetRoomDataExternal: return "SetRoomDataExternal";
			case rpcn::CommandType::GetRoomDataInternal: return "GetRoomDataInternal";
			case rpcn::CommandType::SetRoomDataInternal: return "SetRoomDataInternal";
			case rpcn::CommandType::GetRoomMemberDataInternal: return "GetRoomMemberDataInternal";
			case rpcn::CommandType::SetRoomMemberDataInternal: return "SetRoomMemberDataInternal";
			case rpcn::CommandType::SetUserInfo: return "SetUserInfo";
			case rpcn::CommandType::PingRoomOwner: return "PingRoomOwner";
			case rpcn::CommandType::SendRoomMessage: return "SendRoomMessage";
			case rpcn::CommandType::RequestSignalingInfos: return "RequestSignalingInfos";
			case rpcn::CommandType::RequestTicket: return "RequestTicket";
			case rpcn::CommandType::SendMessage: return "SendMessage";
			case rpcn::CommandType::GetBoardInfos: return "GetBoardInfos";
			case rpcn::CommandType::RecordScore: return "RecordScore";
			case rpcn::CommandType::RecordScoreData: return "RecordScoreData";
			case rpcn::CommandType::GetScoreData: return "GetScoreData";
			case rpcn::CommandType::GetScoreRange: return "GetScoreRange";
			case rpcn::CommandType::GetScoreFriends: return "GetScoreFriends";
			case rpcn::CommandType::GetScoreNpid: return "GetScoreNpid";
			case rpcn::CommandType::GetNetworkTime: return "GetNetworkTime";
			case rpcn::CommandType::TusSetMultiSlotVariable: return "TusSetMultiSlotVariable";
			case rpcn::CommandType::TusGetMultiSlotVariable: return "TusGetMultiSlotVariable";
			case rpcn::CommandType::TusGetMultiUserVariable: return "TusGetMultiUserVariable";
			case rpcn::CommandType::TusGetFriendsVariable: return "TusGetFriendsVariable";
			case rpcn::CommandType::TusAddAndGetVariable: return "TusAddAndGetVariable";
			case rpcn::CommandType::TusTryAndSetVariable: return "TusTryAndSetVariable";
			case rpcn::CommandType::TusDeleteMultiSlotVariable: return "TusDeleteMultiSlotVariable";
			case rpcn::CommandType::TusSetData: return "TusSetData";
			case rpcn::CommandType::TusGetData: return "TusGetData";
			case rpcn::CommandType::TusGetMultiSlotDataStatus: return "TusGetMultiSlotDataStatus";
			case rpcn::CommandType::TusGetMultiUserDataStatus: return "TusGetMultiUserDataStatus";
			case rpcn::CommandType::TusGetFriendsDataStatus: return "TusGetFriendsDataStatus";
			case rpcn::CommandType::TusDeleteMultiSlotData: return "TusDeleteMultiSlotData";
			case rpcn::CommandType::SetPresence: return "SetPresence";
			case rpcn::CommandType::CreateRoomGUI: return "CreateRoomGUI";
			case rpcn::CommandType::JoinRoomGUI: return "JoinRoomGUI";
			case rpcn::CommandType::LeaveRoomGUI: return "LeaveRoomGUI";
			case rpcn::CommandType::GetRoomListGUI: return "GetRoomListGUI";
			case rpcn::CommandType::SetRoomSearchFlagGUI: return "SetRoomSearchFlagGUI";
			case rpcn::CommandType::GetRoomSearchFlagGUI: return "GetRoomSearchFlagGUI";
			case rpcn::CommandType::SetRoomInfoGUI: return "SetRoomInfoGUI";
			case rpcn::CommandType::GetRoomInfoGUI: return "GetRoomInfoGUI";
			case rpcn::CommandType::QuickMatchGUI: return "QuickMatchGUI";
			case rpcn::CommandType::SearchJoinRoomGUI: return "SearchJoinRoomGUI";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<rpcn::NotificationType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case rpcn::NotificationType::UserJoinedRoom: return "UserJoinedRoom";
			case rpcn::NotificationType::UserLeftRoom: return "UserLeftRoom";
			case rpcn::NotificationType::RoomDestroyed: return "RoomDestroyed";
			case rpcn::NotificationType::UpdatedRoomDataInternal: return "UpdatedRoomDataInternal";
			case rpcn::NotificationType::UpdatedRoomMemberDataInternal: return "UpdatedRoomMemberDataInternal";
			case rpcn::NotificationType::FriendQuery: return "FriendQuery";
			case rpcn::NotificationType::FriendNew: return "FriendNew";
			case rpcn::NotificationType::FriendLost: return "FriendLost";
			case rpcn::NotificationType::FriendStatus: return "FriendStatus";
			case rpcn::NotificationType::RoomMessageReceived: return "RoomMessageReceived";
			case rpcn::NotificationType::MessageReceived: return "MessageReceived";
			case rpcn::NotificationType::FriendPresenceChanged: return "FriendPresenceChanged";
			case rpcn::NotificationType::SignalingHelper: return "SignalingHelper";
			case rpcn::NotificationType::MemberJoinedRoomGUI: return "MemberJoinedRoomGUI";
			case rpcn::NotificationType::MemberLeftRoomGUI: return "MemberLeftRoomGUI";
			case rpcn::NotificationType::RoomDisappearedGUI: return "RoomDisappearedGUI";
			case rpcn::NotificationType::RoomOwnerChangedGUI: return "RoomOwnerChangedGUI";
			case rpcn::NotificationType::UserKickedGUI: return "UserKickedGUI";
			case rpcn::NotificationType::QuickMatchCompleteGUI: return "QuickMatchCompleteGUI";
			}

			return unknown;
		});
}

void vec_stream::dump() const
{
	rpcn_log.error("vec_stream dump:\n%s", fmt::buf_to_hexstring(vec.data(), vec.size()));
}

namespace rpcn
{
	localized_string_id rpcn_state_to_localized_string_id(rpcn::rpcn_state state)
	{
		switch (state)
		{
		case rpcn::rpcn_state::failure_no_failure: return localized_string_id::RPCN_NO_ERROR;
		case rpcn::rpcn_state::failure_input: return localized_string_id::RPCN_ERROR_INVALID_INPUT;
		case rpcn::rpcn_state::failure_wolfssl: return localized_string_id::RPCN_ERROR_WOLFSSL;
		case rpcn::rpcn_state::failure_resolve: return localized_string_id::RPCN_ERROR_RESOLVE;
		case rpcn::rpcn_state::failure_binding: return localized_string_id::RPCN_ERROR_BINDING;
		case rpcn::rpcn_state::failure_connect: return localized_string_id::RPCN_ERROR_CONNECT;
		case rpcn::rpcn_state::failure_id: return localized_string_id::RPCN_ERROR_LOGIN_ERROR;
		case rpcn::rpcn_state::failure_id_already_logged_in: return localized_string_id::RPCN_ERROR_ALREADY_LOGGED;
		case rpcn::rpcn_state::failure_id_username: return localized_string_id::RPCN_ERROR_INVALID_LOGIN;
		case rpcn::rpcn_state::failure_id_password: return localized_string_id::RPCN_ERROR_INVALID_PASSWORD;
		case rpcn::rpcn_state::failure_id_token: return localized_string_id::RPCN_ERROR_INVALID_TOKEN;
		case rpcn::rpcn_state::failure_protocol: return localized_string_id::RPCN_ERROR_INVALID_PROTOCOL_VERSION;
		case rpcn::rpcn_state::failure_other: return localized_string_id::RPCN_ERROR_UNKNOWN;
		default: return localized_string_id::INVALID;
		}
	}

	void overlay_friend_callback(void* /*param*/, rpcn::NotificationType ntype, const std::string& username, bool status)
	{
		if (!g_cfg.misc.show_rpcn_popups)
			return;

		localized_string_id loc_id = localized_string_id::INVALID;

		switch (ntype)
		{
		case rpcn::NotificationType::FriendQuery: loc_id = localized_string_id::RPCN_FRIEND_REQUEST_RECEIVED; break;
		case rpcn::NotificationType::FriendNew: loc_id = localized_string_id::RPCN_FRIEND_ADDED; break;
		case rpcn::NotificationType::FriendLost: loc_id = localized_string_id::RPCN_FRIEND_LOST; break;
		case rpcn::NotificationType::FriendStatus: loc_id = status ? localized_string_id::RPCN_FRIEND_LOGGED_IN : localized_string_id::RPCN_FRIEND_LOGGED_OUT; break;
		case rpcn::NotificationType::FriendPresenceChanged: return;
		default: rpcn_log.fatal("An unhandled notification type was received by the overlay friend callback!"); break;
		}

		rsx::overlays::queue_message(get_localized_string(loc_id, username.c_str()), 3'000'000);
	}

	std::string rpcn_state_to_string(rpcn::rpcn_state state)
	{
		return get_localized_string(rpcn_state_to_localized_string_id(state));
	}

	void friend_online_data::dump() const
	{
		rpcn_log.notice("online: %s, pr_com_id: %s, pr_title: %s, pr_status: %s, pr_comment: %s, pr_data: %s", online ? "true" : "false", pr_com_id.data, pr_title, pr_status, pr_comment, fmt::buf_to_hexstring(pr_data.data(), pr_data.size()));
	}

	constexpr u32 RPCN_PROTOCOL_VERSION = 26;
	constexpr usz RPCN_HEADER_SIZE = 15;

	const char* error_to_explanation(rpcn::ErrorType error)
	{
		switch (error)
		{
		case rpcn::ErrorType::NoError: return "No error";
		case rpcn::ErrorType::Malformed: return "Sent packet was malformed!";
		case rpcn::ErrorType::Invalid: return "Sent command was invalid!";
		case rpcn::ErrorType::InvalidInput: return "Sent data was invalid!";
		case rpcn::ErrorType::TooSoon: return "Request happened too soon!";
		case rpcn::ErrorType::LoginError: return "Unknown login error!";
		case rpcn::ErrorType::LoginAlreadyLoggedIn: return "User is already logged in!";
		case rpcn::ErrorType::LoginInvalidUsername: return "Login error: invalid username!";
		case rpcn::ErrorType::LoginInvalidPassword: return "Login error: invalid password!";
		case rpcn::ErrorType::LoginInvalidToken: return "Login error: invalid token!";
		case rpcn::ErrorType::CreationError: return "Error creating an account!";
		case rpcn::ErrorType::CreationExistingUsername: return "Error creating an account: existing username!";
		case rpcn::ErrorType::CreationBannedEmailProvider: return "Error creating an account: banned email provider!";
		case rpcn::ErrorType::CreationExistingEmail: return "Error creating an account: an account with that email already exist!";
		case rpcn::ErrorType::RoomMissing: return "User tried to join a non-existent room!";
		case rpcn::ErrorType::RoomAlreadyJoined: return "User has already joined!";
		case rpcn::ErrorType::RoomFull: return "User tried to join a full room!";
		case rpcn::ErrorType::RoomPasswordMismatch: return "Room password used was invalid";
		case rpcn::ErrorType::RoomPasswordMissing: return "Room password was missing during room creation";
		case rpcn::ErrorType::RoomGroupNoJoinLabel: return "Tried to join a group room without a label";
		case rpcn::ErrorType::RoomGroupFull: return "Room group is full";
		case rpcn::ErrorType::RoomGroupJoinLabelNotFound: return "Join label was invalid";
		case rpcn::ErrorType::RoomGroupMaxSlotMismatch: return "Mismatch between max_slot and the listed slots in groups";
		case rpcn::ErrorType::Unauthorized: return "User attempted an unauthorized operation!";
		case rpcn::ErrorType::DbFail: return "A db query failed on the server!";
		case rpcn::ErrorType::EmailFail: return "An email action failed on the server!";
		case rpcn::ErrorType::NotFound: return "A request replied not found!";
		case rpcn::ErrorType::Blocked: return "You're blocked!";
		case rpcn::ErrorType::AlreadyFriend: return "You're already friends!";
		case rpcn::ErrorType::ScoreNotBest: return "Attempted to register a score that is not better!";
		case rpcn::ErrorType::ScoreInvalid: return "Score for player was found but wasn't what was expected!";
		case rpcn::ErrorType::ScoreHasData: return "Score already has game data associated with it!";
		case rpcn::ErrorType::CondFail: return "Condition related to the query failed!";
		case rpcn::ErrorType::Unsupported: return "An unsupported operation was attempted!";
		}

		fmt::throw_exception("Unknown error returned: %d", static_cast<u8>(error));
	}

	void print_error(rpcn::CommandType command, rpcn::ErrorType error)
	{
		const std::string error_message = fmt::format("command: %s result: %s, explanation: %s", command, error, error_to_explanation(error));

		if (error == rpcn::ErrorType::NoError)
		{
			rpcn_log.trace("%s", error_message);
		}
		else
		{
			rpcn_log.warning("%s", error_message);
		}
	}

	// Constructor, destructor & singleton manager

	rpcn_client::rpcn_client(u32 binding_address)
		: binding_address(binding_address), sem_connected(0), sem_authentified(0), sem_reader(0), sem_writer(0), sem_rpcn(0),
		  thread_rpcn(std::thread(&rpcn_client::rpcn_thread, this)),
		  thread_rpcn_reader(std::thread(&rpcn_client::rpcn_reader_thread, this)),
		  thread_rpcn_writer(std::thread(&rpcn_client::rpcn_writer_thread, this))
	{
		g_cfg_rpcn.load();

		sem_rpcn.release();
	}

	rpcn_client::~rpcn_client()
	{
		terminate_connection();
		std::lock_guard lock(inst_mutex);
		terminate = true;
		sem_rpcn.release();
		sem_reader.release();
		sem_writer.release();

		if (thread_rpcn.joinable())
			thread_rpcn.join();
		if (thread_rpcn_reader.joinable())
			thread_rpcn_reader.join();
		if (thread_rpcn_writer.joinable())
			thread_rpcn_writer.join();

		disconnect();

		sem_connected.release();
		sem_authentified.release();
	}

	std::shared_ptr<rpcn_client> rpcn_client::get_instance(u32 binding_address, bool check_config)
	{
		if (check_config && g_cfg.net.psn_status != np_psn_status::psn_rpcn)
		{
			fmt::throw_exception("RPCN is required to use PSN online features.");
		}

		std::shared_ptr<rpcn_client> sptr;

		std::lock_guard lock(inst_mutex);
		sptr = instance.lock();
		if (!sptr)
		{
			sptr = std::shared_ptr<rpcn_client>(new rpcn_client(binding_address));
			sptr->register_friend_cb(overlay_friend_callback, nullptr);
			instance = sptr;
		}

		return sptr;
	}

	// inform rpcn that the server infos have been updated and signal rpcn_thread to try again
	void rpcn_client::server_infos_updated()
	{
		if (connected)
		{
			disconnect();
		}

		sem_rpcn.release();
	}

	// RPCN thread
	void rpcn_client::rpcn_reader_thread()
	{
		thread_base::set_name("RPCN Reader");

		while (true)
		{
			sem_reader.acquire();
			{
				std::lock_guard lock(mutex_read);
				while (true)
				{
					if (terminate)
					{
						return;
					}

					if (!handle_input())
					{
						break;
					}
				}
			}
		}
	}

	void rpcn_client::rpcn_writer_thread()
	{
		thread_base::set_name("RPCN Writer");

		while (true)
		{
			sem_writer.acquire();

			{
				std::lock_guard lock(mutex_write);

				if (terminate)
				{
					return;
				}

				if (!handle_output())
				{
					break;
				}
			}
		}
	}

	void rpcn_client::rpcn_thread()
	{
		thread_base::set_name("RPCN Client");

		// UDP Signaling related
		steady_clock::time_point last_ping_time_ipv4{}, last_pong_time_ipv4{};
		steady_clock::time_point last_ping_time_ipv6{}, last_pong_time_ipv6{};

		while (true)
		{
			sem_rpcn.acquire();

			while (true)
			{
				if (terminate)
				{
					return;
				}

				// By default is the object is alive we should be connected
				if (!connected)
				{
					if (want_conn)
					{
						want_conn = false;
						{
							std::lock_guard lock(mutex_connected);
							connect(g_cfg_rpcn.get_host());
						}
						sem_connected.release();
					}

					break;
				}

				if (!authentified)
				{
					if (want_auth)
					{
						bool result_login;
						{
							std::lock_guard lock(mutex_authentified);
							result_login = login(g_cfg_rpcn.get_npid(), g_cfg_rpcn.get_password(), g_cfg_rpcn.get_token());
						}
						sem_authentified.release();

						if (!result_login)
						{
							break;
						}
						continue;
					}
					else
					{
						// Nothing to do while waiting for auth request so wait for main sem
						break;
					}
				}

				if (authentified && !Emu.IsStopped())
				{
					// Ping the UDP Signaling Server if we're authentified & ingame
					const auto now = steady_clock::now();
					const auto rpcn_msgs = get_rpcn_msgs();

					for (const auto& msg : rpcn_msgs)
					{
						if (msg.size() == 6)
						{
							const u32 new_addr_sig = read_from_ptr<le_t<u32>>(&msg[0]);
							const u16 new_port_sig = read_from_ptr<be_t<u16>>(&msg[4]);
							const u32 old_addr_sig = addr_sig;
							const u32 old_port_sig = port_sig;

							if (new_addr_sig != old_addr_sig)
							{
								addr_sig = new_addr_sig;
								if (old_addr_sig == 0)
								{
									addr_sig.notify_one();
								}
							}

							if (new_port_sig != old_port_sig)
							{
								port_sig = new_port_sig;
								if (old_port_sig == 0)
								{
									port_sig.notify_one();
								}
							}

							last_pong_time_ipv4 = now;
						}
						else if (msg.size() == 18)
						{
							// We don't really need ipv6 info stored so we just update the pong data
							// std::array<u8, 16> new_ipv6_addr;
							// std::memcpy(new_ipv6_addr.data(), &msg[3], 16);
							// const u32 new_ipv6_port = read_from_ptr<be_t<u16>>(&msg[16]);

							last_pong_time_ipv6 = now;
						}
						else
						{
							rpcn_log.error("Received faulty RPCN UDP message!");
						}
					}

					const std::chrono::nanoseconds time_since_last_ipv4_ping = now - last_ping_time_ipv4;
					const std::chrono::nanoseconds time_since_last_ipv4_pong = now - last_pong_time_ipv4;
					const std::chrono::nanoseconds time_since_last_ipv6_ping = now - last_ping_time_ipv6;
					const std::chrono::nanoseconds time_since_last_ipv6_pong = now - last_pong_time_ipv6;

					auto forge_ping_packet = [&]() -> std::vector<u8>
					{
						std::vector<u8> ping(13);
						ping[0] = 1;
						write_to_ptr<le_t<s64>>(ping, 1, user_id);
						write_to_ptr<be_t<u32>>(ping, 9, +local_addr_sig);
						return ping;
					};

					// Send a packet every 5 seconds and then every 500 ms until reply is received
					if (time_since_last_ipv4_pong >= 5s && time_since_last_ipv4_ping > 500ms)
					{
						const auto ping = forge_ping_packet();

						if (!send_packet_from_p2p_port_ipv4(ping, addr_rpcn_udp_ipv4))
							rpcn_log.error("Failed to send IPv4 ping to RPCN!");

						last_ping_time_ipv4 = now;
						continue;
					}

					if (np::is_ipv6_supported() && time_since_last_ipv6_pong >= 5s && time_since_last_ipv6_ping > 500ms)
					{
						const auto ping = forge_ping_packet();

						if (!send_packet_from_p2p_port_ipv6(ping, addr_rpcn_udp_ipv6))
							rpcn_log.error("Failed to send IPv6 ping to RPCN!");

						last_ping_time_ipv6 = now;
						continue;
					}

					auto min_duration_for = [&](const auto last_ping_time, const auto last_pong_time) -> std::chrono::nanoseconds
					{
						if ((now - last_pong_time) < 5s)
						{
							return (5s - (now - last_pong_time));
						}
						else
						{
							return (500ms - (now - last_ping_time));
						}
					};

					auto duration = min_duration_for(last_ping_time_ipv4, last_pong_time_ipv4);

					if (np::is_ipv6_supported())
					{
						const auto duration_ipv6 = min_duration_for(last_ping_time_ipv6, last_pong_time_ipv6);
						duration = std::min(duration, duration_ipv6);
					}

					// Expected to fail unless rpcn is terminated
					// The check is there to nuke a msvc warning
					if (!sem_rpcn.try_acquire_for(duration))
					{
					}
				}
			}
		}
	}

	bool rpcn_client::handle_input()
	{
		u8 header[RPCN_HEADER_SIZE];
		auto res_recvn = recvn(header, RPCN_HEADER_SIZE);

		switch (res_recvn)
		{
		case recvn_result::recvn_noconn: return error_and_disconnect("Disconnected");
		case recvn_result::recvn_fatal:
		case recvn_result::recvn_timeout: return error_and_disconnect("Failed to read a packet header on socket");
		case recvn_result::recvn_nodata: return true;
		case recvn_result::recvn_success: break;
		case recvn_result::recvn_terminate: return error_and_disconnect_notice("Recvn was forcefully aborted");
		}

		const u8 packet_type = header[0];
		const auto command = static_cast<rpcn::CommandType>(static_cast<u16>(read_from_ptr<le_t<u16>>(&header[1])));
		const u32 packet_size = read_from_ptr<le_t<u32>>(&header[3]);
		const u64 packet_id = read_from_ptr<le_t<u64>>(&header[7]);

		if (packet_size < RPCN_HEADER_SIZE)
			return error_and_disconnect("Invalid packet size");

		std::vector<u8> data;
		if (packet_size > RPCN_HEADER_SIZE)
		{
			const u32 data_size = packet_size - RPCN_HEADER_SIZE;
			data.resize(data_size);
			if (recvn(data.data(), data_size) != recvn_result::recvn_success)
				return error_and_disconnect("Failed to receive a whole packet");
		}

		switch (static_cast<PacketType>(packet_type))
		{
		case PacketType::Request: return error_and_disconnect("Client shouldn't receive request packets!");
		case PacketType::Reply:
		{
			if (data.empty())
				return error_and_disconnect("Reply packet without result");

			// Internal commands without feedback
			if (command == CommandType::ResetState)
			{
				ensure(data[0] == static_cast<u8>(ErrorType::NoError));
				break;
			}

			// Those commands are handled synchronously and won't be forwarded to NP Handler
			if (command == CommandType::Login || command == CommandType::GetServerList || command == CommandType::Create ||
				command == CommandType::AddFriend || command == CommandType::RemoveFriend ||
				command == CommandType::AddBlock || command == CommandType::RemoveBlock ||
				command == CommandType::SendMessage || command == CommandType::SendToken ||
				command == CommandType::SendResetToken || command == CommandType::ResetPassword ||
				command == CommandType::GetNetworkTime || command == CommandType::SetPresence || command == CommandType::Terminate)
			{
				std::lock_guard lock(mutex_replies_sync);
				replies_sync.insert(std::make_pair(packet_id, std::make_pair(command, std::move(data))));
			}
			else
			{
				if (packet_id < 0x100000000)
				{
					std::lock_guard lock(mutex_replies);
					replies.insert(std::make_pair(static_cast<u32>(packet_id), std::make_pair(command, std::move(data))));
				}
				else
				{
					rpcn_log.error("Tried to forward a reply whose packet_id marks it as internal to RPCN");
				}
			}

			break;
		}
		case PacketType::Notification:
		{
			auto notif_type = static_cast<rpcn::NotificationType>(command);

			switch (notif_type)
			{
			case NotificationType::FriendNew:
			case NotificationType::FriendLost:
			case NotificationType::FriendQuery:
			case NotificationType::FriendStatus:
			case NotificationType::FriendPresenceChanged:
			{
				handle_friend_notification(notif_type, std::move(data));
				break;
			}
			case NotificationType::MessageReceived:
			{
				handle_message(std::move(data));
				break;
			}
			default:
			{
				std::lock_guard lock(mutex_notifs);
				notifications.emplace_back(std::make_pair(notif_type, std::move(data)));
				break;
			}
			}
			break;
		}
		case PacketType::ServerInfo:
		{
			if (data.size() != 4)
				return error_and_disconnect("Invalid size of ServerInfo packet");

			received_version = reinterpret_cast<le_t<u32>&>(data[0]);
			server_info_received = true;
			break;
		}

		default: return error_and_disconnect("Unknown packet received!");
		}

		return true;
	}

	void rpcn_client::add_packet(std::vector<u8> packet)
	{
		std::lock_guard lock(mutex_packets_to_send);
		packets_to_send.push_back(std::move(packet));
		sem_writer.release();
	}

	bool rpcn_client::handle_output()
	{
		std::vector<std::vector<u8>> packets;
		{
			std::lock_guard lock(mutex_packets_to_send);
			packets = std::move(packets_to_send);
			packets_to_send.clear();
		}

		for (const auto& packet : packets)
		{
			if (!send_packet(packet))
			{
				return false;
			}
		}

		return true;
	}

	// WolfSSL operations

	std::string rpcn_client::get_wolfssl_error(WOLFSSL* wssl, int error) const
	{
		char error_string[WOLFSSL_MAX_ERROR_SZ]{};
		const auto wssl_err = wolfSSL_get_error(wssl, error);
		wolfSSL_ERR_error_string(wssl_err, &error_string[0]);
		return std::string(error_string);
	}

	rpcn_client::recvn_result rpcn_client::recvn(u8* buf, usz n)
	{
		u32 num_timeouts = 0;

		usz n_recv = 0;
		while (n_recv != n)
		{
			if (!connected)
				return recvn_result::recvn_noconn;

			if (terminate)
				return recvn_result::recvn_terminate;

			int res = wolfSSL_read(read_wssl, reinterpret_cast<char*>(buf) + n_recv, ::narrow<s32>(n - n_recv));
			if (res <= 0)
			{
				if (wolfSSL_want_read(read_wssl))
				{
					pollfd poll_fd{};

					while ((poll_fd.revents & POLLIN) != POLLIN && (poll_fd.revents & POLLRDNORM) != POLLRDNORM)
					{
						if (!connected)
							return recvn_result::recvn_noconn;

						if (terminate)
							return recvn_result::recvn_terminate;

						poll_fd.fd = sockfd;
						poll_fd.events = POLLIN;
						poll_fd.revents = 0;
#ifdef _WIN32
						int res_poll = WSAPoll(&poll_fd, 1, RPCN_TIMEOUT_INTERVAL);
#else
						int res_poll = poll(&poll_fd, 1, RPCN_TIMEOUT_INTERVAL);
#endif

						if (res_poll < 0)
						{
							rpcn_log.error("recvn poll failed with native error: %d)", get_native_error());
							return recvn_result::recvn_fatal;
						}

						num_timeouts++;

						if (num_timeouts > (RPCN_TIMEOUT / RPCN_TIMEOUT_INTERVAL))
						{
							if (n_recv == 0)
								return recvn_result::recvn_nodata;

							rpcn_log.error("recvn timeout with %d bytes received", n_recv);
							return recvn_result::recvn_timeout;
						}
					}
				}
				else
				{
					if (res == 0)
					{
						// Remote closed connection
						rpcn_log.notice("recv failed: connection reset by server");
						return recvn_result::recvn_noconn;
					}

					rpcn_log.error("recvn failed with error: %d:%s(native: %d)", res, get_wolfssl_error(read_wssl, res), get_native_error());
					return recvn_result::recvn_fatal;
				}
			}
			else
			{
				// Reset timeout each time something is received
				num_timeouts = 0;
				n_recv += res;
			}
		}

		return recvn_result::recvn_success;
	}

	bool rpcn_client::send_packet(const std::vector<u8>& packet)
	{
		u32 num_timeouts = 0;
		usz n_sent = 0;
		while (n_sent != packet.size())
		{
			if (terminate)
				return error_and_disconnect("send_packet was forcefully aborted");

			if (!connected)
				return false;

			int res = wolfSSL_write(write_wssl, reinterpret_cast<const char*>(packet.data() + n_sent), ::narrow<s32>(packet.size() - n_sent));
			if (res <= 0)
			{
				if (wolfSSL_want_write(write_wssl))
				{
					pollfd poll_fd{};

					while ((poll_fd.revents & POLLOUT) != POLLOUT)
					{
						if (terminate)
							return error_and_disconnect("send_packet was forcefully aborted");

						poll_fd.fd = sockfd;
						poll_fd.events = POLLOUT;
						poll_fd.revents = 0;
#ifdef _WIN32
						int res_poll = WSAPoll(&poll_fd, 1, RPCN_TIMEOUT_INTERVAL);
#else
						int res_poll = poll(&poll_fd, 1, RPCN_TIMEOUT_INTERVAL);
#endif
						if (res_poll < 0)
						{
							rpcn_log.error("send_packet failed with native error: %d)", get_native_error());
							return error_and_disconnect("send_packet failed on poll");
						}

						num_timeouts++;
						if (num_timeouts > (RPCN_TIMEOUT / RPCN_TIMEOUT_INTERVAL))
						{
							rpcn_log.error("send_packet timeout with %d bytes sent", n_sent);
							return error_and_disconnect("Failed to send all the bytes");
						}
					}
				}
				else
				{
					rpcn_log.error("send_packet failed with error: %s", get_wolfssl_error(write_wssl, res));
					return error_and_disconnect("Failed to send all the bytes");
				}

				res = 0;
			}
			n_sent += res;
		}

		return true;
	}

	// Helper functions

	bool rpcn_client::forge_send(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data)
	{
		// TODO: add a check for status?

		std::vector<u8> sent_packet = forge_request(command, packet_id, data);
		add_packet(std::move(sent_packet));
		return true;
	}

	bool rpcn_client::forge_send_reply(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data, std::vector<u8>& reply_data)
	{
		if (!forge_send(command, packet_id, data))
			return false;

		if (!get_reply(packet_id, reply_data))
			return false;

		return true;
	}

	// Connect & disconnect functions

	void rpcn_client::disconnect()
	{
		if (read_wssl)
		{
			wolfSSL_free(read_wssl);
			read_wssl = nullptr;
		}

		if (write_wssl)
		{
			wolfSSL_free(write_wssl);
			write_wssl = nullptr;
		}

		if (wssl_ctx)
		{
			wolfSSL_CTX_free(wssl_ctx);
			wssl_ctx = nullptr;
		}

		wolfSSL_Cleanup();

		if (sockfd)
		{
#ifdef _WIN32
			::shutdown(sockfd, SD_BOTH);
			::closesocket(sockfd);
#else
			::shutdown(sockfd, SHUT_RDWR);
			::close(sockfd);
#endif
			sockfd = 0;
		}

		connected = false;
		authentified = false;
		server_info_received = false;
	}

	bool rpcn_client::connect(const std::string& host)
	{
		rpcn_log.warning("connect: Attempting to connect");

		state = rpcn_state::failure_no_failure;

		const auto hostname_and_port = parse_rpcn_host(host);

		if (!hostname_and_port)
		{
			state = rpcn_state::failure_input;
			return false;
		}

		const auto [hostname, port] = *hostname_and_port;

		{
			// Ensures both read & write threads are in waiting state
			std::lock_guard lock_read(mutex_read), lock_write(mutex_write);

			// Cleans previous data if any
			disconnect();

			if (wolfSSL_Init() != WOLFSSL_SUCCESS)
			{
				rpcn_log.error("connect: Failed to initialize wolfssl");
				state = rpcn_state::failure_wolfssl;
				return false;
			}

			if ((wssl_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == nullptr)
			{
				rpcn_log.error("connect: Failed to create wolfssl context");
				state = rpcn_state::failure_wolfssl;
				return false;
			}

			wolfSSL_CTX_set_verify(wssl_ctx, SSL_VERIFY_NONE, nullptr);

			if ((read_wssl = wolfSSL_new(wssl_ctx)) == nullptr)
			{
				rpcn_log.error("connect: Failed to create wolfssl object");
				state = rpcn_state::failure_wolfssl;
				return false;
			}

			wolfSSL_set_using_nonblock(read_wssl, 1);

			memset(&addr_rpcn, 0, sizeof(addr_rpcn));

			addr_rpcn.sin_port = std::bit_cast<u16, be_t<u16>>(port); // htons
			addr_rpcn.sin_family = AF_INET;

			addrinfo* addr_info{};

			if (getaddrinfo(hostname.c_str(), nullptr, nullptr, &addr_info))
			{
				rpcn_log.error("connect: Failed to getaddrinfo %s", host);
				state = rpcn_state::failure_resolve;
				return false;
			}

			bool found_ipv4 = false;
			addrinfo* found = addr_info;

			while (found != nullptr)
			{
				switch (found->ai_family)
				{
				case AF_INET:
				{
					addr_rpcn.sin_addr = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr;
					found_ipv4 = true;
					break;
				}
				case AF_INET6:
				{
					addr_rpcn_udp_ipv6.sin6_family = AF_INET6;
					addr_rpcn_udp_ipv6.sin6_port = std::bit_cast<u16, be_t<u16>>(3657);
					addr_rpcn_udp_ipv6.sin6_addr = reinterpret_cast<sockaddr_in6*>(found->ai_addr)->sin6_addr;
					break;
				}
				default: break;
				}

				found = found->ai_next;
			}

			if (!found_ipv4)
			{
				rpcn_log.error("connect: Failed to find IPv4 for %s", host);
				state = rpcn_state::failure_resolve;
				return false;
			}

			memcpy(&addr_rpcn_udp_ipv4, &addr_rpcn, sizeof(addr_rpcn_udp_ipv4));
			addr_rpcn_udp_ipv4.sin_port = std::bit_cast<u16, be_t<u16>>(3657); // htons

			sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
			if (sockfd == INVALID_SOCKET)
#else
			if (sockfd == -1)
#endif
			{
				rpcn_log.error("connect: Failed to connect to RPCN server!");
				state = rpcn_state::failure_connect;
				return false;
			}

			sockaddr_in sock_addr = {.sin_family = AF_INET};
			sock_addr.sin_addr.s_addr = binding_address;

			if (binding_address != 0 && ::bind(sockfd, reinterpret_cast<const sockaddr*>(&sock_addr), sizeof(sock_addr)) == -1)
			{
				rpcn_log.error("bind: Failed to bind RPCN client socket to binding address(%s): 0x%x!", np::ip_to_string(binding_address), get_native_error());
				state = rpcn_state::failure_binding;
				return false;
			}

			if (::connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr_rpcn), sizeof(addr_rpcn)) != 0)
			{
				rpcn_log.error("connect: Failed to connect to RPCN server!");
				state = rpcn_state::failure_connect;
				return false;
			}

			rpcn_log.notice("connect: Connection successful");

#ifdef _WIN32
			u_long _true = 1;
			ensure(::ioctlsocket(sockfd, FIONBIO, &_true) == 0);
#else
			ensure(::fcntl(sockfd, F_SETFL, ::fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == 0);
#endif

			sockaddr_in client_addr;
			socklen_t client_addr_size = sizeof(client_addr);
			if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_size) != 0)
			{
				rpcn_log.error("Failed to get the client address from the socket!");
			}

			update_local_addr(client_addr.sin_addr.s_addr);
			// rpcn_log.notice("Updated local address to %s", np::ip_to_string(std::bit_cast<u32, be_t<u32>>(local_addr_sig.load())));

			if (wolfSSL_set_fd(read_wssl, ::narrow<int>(sockfd)) != WOLFSSL_SUCCESS)
			{
				rpcn_log.error("connect: Failed to associate wolfssl to the socket");
				state = rpcn_state::failure_wolfssl;
				return false;
			}

			int ret_connect;
			while ((ret_connect = wolfSSL_connect(read_wssl)) != SSL_SUCCESS)
			{
				if (wolfSSL_want_read(read_wssl) || wolfSSL_want_write(read_wssl))
					continue;

				state = rpcn_state::failure_wolfssl;
				rpcn_log.error("connect: Handshake failed with RPCN Server: %s", get_wolfssl_error(read_wssl, ret_connect));
				return false;
			}

			rpcn_log.notice("connect: Handshake successful");

			if ((write_wssl = wolfSSL_write_dup(read_wssl)) == NULL)
			{
				rpcn_log.error("connect: Failed to create write dup for SSL");
				state = rpcn_state::failure_wolfssl;
				return false;
			}
		}

		connected = true;
		// Wake up both read & write threads
		sem_reader.release();
		sem_writer.release();

		rpcn_log.notice("connect: Waiting for protocol version");

		while (!server_info_received && connected && !terminate)
		{
			std::this_thread::sleep_for(5ms);
		}

		if (!connected || terminate)
		{
			state = rpcn_state::failure_other;
			return true;
		}

		if (received_version != RPCN_PROTOCOL_VERSION)
		{
			state = rpcn_state::failure_protocol;
			rpcn_log.error("Server returned protocol version: %d, expected: %d", received_version, RPCN_PROTOCOL_VERSION);
			return false;
		}

		rpcn_log.notice("connect: Protocol version matches");
		return true;
	}

	bool rpcn_client::login(const std::string& npid, const std::string& password, const std::string& token)
	{
		if (npid.empty())
		{
			state = rpcn_state::failure_id_username;
			return false;
		}

		if (password.empty())
		{
			state = rpcn_state::failure_id_password;
			return false;
		}

		rpcn_log.notice("Attempting to login!");

		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(password.begin(), password.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(token.begin(), token.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;

		if (!forge_send_reply(CommandType::Login, req_id, data, packet_data))
		{
			state = rpcn_state::failure_other;
			return false;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());
		online_name = reply.get_string(false);
		avatar_url = reply.get_string(false);
		user_id = reply.get<s64>();

		auto get_usernames_and_status = [](vec_stream& stream, std::map<std::string, friend_online_data>& friends)
		{
			u32 num_friends = stream.get<u32>();
			for (u32 i = 0; i < num_friends; i++)
			{
				std::string friend_name = stream.get_string(false);
				bool online = !!(stream.get<u8>());

				SceNpCommunicationId pr_com_id = stream.get_com_id();
				std::string pr_title = fmt::truncate(stream.get_string(true), SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX - 1);
				std::string pr_status = fmt::truncate(stream.get_string(true), SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_SIZE_MAX - 1);
				std::string pr_comment = fmt::truncate(stream.get_string(true), SCE_NP_BASIC_PRESENCE_COMMENT_SIZE_MAX - 1);
				std::vector<u8> pr_data = stream.get_rawdata();

				if (pr_data.size() > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
				{
					pr_data.resize(SCE_NP_BASIC_MAX_PRESENCE_SIZE);
				}

				friend_online_data infos(online, std::move(pr_com_id), std::move(pr_title), std::move(pr_status), std::move(pr_comment), std::move(pr_data));
				friends.insert_or_assign(std::move(friend_name), std::move(infos));
			}
		};

		auto get_usernames = [](vec_stream& stream, std::set<std::string>& usernames)
		{
			u32 num_usernames = stream.get<u32>();
			for (u32 i = 0; i < num_usernames; i++)
			{
				std::string username = stream.get_string(false);
				usernames.insert(std::move(username));
			}
		};

		{
			std::lock_guard lock(mutex_friends);

			get_usernames_and_status(reply, friend_infos.friends);
			get_usernames(reply, friend_infos.requests_sent);
			get_usernames(reply, friend_infos.requests_received);
			get_usernames(reply, friend_infos.blocked);
		}

		if (error != rpcn::ErrorType::NoError)
		{
			switch (error)
			{
			case rpcn::ErrorType::LoginError: state = rpcn_state::failure_id; break;
			case rpcn::ErrorType::LoginAlreadyLoggedIn: state = rpcn_state::failure_id_already_logged_in; break;
			case rpcn::ErrorType::LoginInvalidUsername: state = rpcn_state::failure_id_username; break;
			case rpcn::ErrorType::LoginInvalidPassword: state = rpcn_state::failure_id_password; break;
			case rpcn::ErrorType::LoginInvalidToken: state = rpcn_state::failure_id_token; break;
			default: state = rpcn_state::failure_id; break;
			}

			disconnect();
			return false;
		}

		if (reply.is_error())
			return error_and_disconnect("Malformed reply to Login command");

		rpcn_log.success("You are now logged in RPCN(%s | %s)!", npid, online_name);
		authentified = true;

		return true;
	}

	bool rpcn_client::terminate_connection()
	{
		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		std::vector<u8> data;

		if (!forge_send_reply(CommandType::Terminate, req_id, data, packet_data))
		{
			return false;
		}

		return true;
	}

	void rpcn_client::reset_state()
	{
		if (!connected || !authentified)
			return;

		std::vector<u8> data;
		forge_send(CommandType::ResetState, rpcn_request_counter.fetch_add(1), data);
	}

	ErrorType rpcn_client::create_user(std::string_view npid, std::string_view password, std::string_view online_name, std::string_view avatar_url, std::string_view email)
	{
		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(password.begin(), password.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(online_name.begin(), online_name.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(avatar_url.begin(), avatar_url.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(email.begin(), email.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::Create, req_id, data, packet_data))
		{
			state = rpcn_state::failure_other;
			return ErrorType::CreationError;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error == rpcn::ErrorType::NoError)
		{
			rpcn_log.success("You have successfully created a RPCN account(%s | %s)!", npid, online_name);
		}

		return error;
	}

	ErrorType rpcn_client::resend_token(const std::string& npid, const std::string& password)
	{
		if (authentified)
		{
			// If you're already logged in why do you need a token?
			return ErrorType::LoginAlreadyLoggedIn;
		}

		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(password.begin(), password.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::SendToken, req_id, data, packet_data))
		{
			return ErrorType::Malformed;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error == rpcn::ErrorType::NoError)
		{
			rpcn_log.success("Token has successfully been resent!");
		}

		return error;
	}

	ErrorType rpcn_client::send_reset_token(std::string_view npid, std::string_view email)
	{
		if (authentified)
		{
			// If you're already logged in why do you need a password reset token?
			return ErrorType::LoginAlreadyLoggedIn;
		}

		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(email.begin(), email.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::SendResetToken, req_id, data, packet_data))
		{
			return ErrorType::Malformed;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error == rpcn::ErrorType::NoError)
		{
			rpcn_log.success("Password reset token has successfully been sent!");
		}

		return error;
	}

	ErrorType rpcn_client::reset_password(std::string_view npid, std::string_view token, std::string_view password)
	{
		if (authentified)
		{
			// If you're already logged in why do you need to reset the password?
			return ErrorType::LoginAlreadyLoggedIn;
		}

		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(token.begin(), token.end(), std::back_inserter(data));
		data.push_back(0);
		std::copy(password.begin(), password.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::ResetPassword, req_id, data, packet_data))
		{
			return ErrorType::Malformed;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error == rpcn::ErrorType::NoError)
		{
			rpcn_log.success("Password has successfully been reset!");
		}

		return error;
	}

	bool rpcn_client::add_friend(const std::string& friend_username)
	{
		std::vector<u8> data;
		std::copy(friend_username.begin(), friend_username.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::AddFriend, req_id, data, packet_data))
		{
			return false;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error != rpcn::ErrorType::NoError)
		{
			return false;
		}

		rpcn_log.success("You have successfully added \"%s\" as a friend", friend_username);
		return true;
	}

	bool rpcn_client::remove_friend(const std::string& friend_username)
	{
		std::vector<u8> data;
		std::copy(friend_username.begin(), friend_username.end(), std::back_inserter(data));
		data.push_back(0);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		std::vector<u8> packet_data;
		if (!forge_send_reply(CommandType::RemoveFriend, req_id, data, packet_data))
		{
			return false;
		}

		vec_stream reply(packet_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error != rpcn::ErrorType::NoError)
		{
			return false;
		}

		rpcn_log.success("You have successfully removed \"%s\" from your friendlist", friend_username);
		return true;
	}

	std::vector<std::pair<rpcn::NotificationType, std::vector<u8>>> rpcn_client::get_notifications()
	{
		std::lock_guard lock(mutex_notifs);
		auto notifs = std::move(notifications);
		notifications.clear();
		return notifs;
	}

	std::unordered_map<u32, std::pair<rpcn::CommandType, std::vector<u8>>> rpcn_client::get_replies()
	{
		std::lock_guard lock(mutex_replies);
		auto ret_replies = std::move(replies);
		replies.clear();
		return ret_replies;
	}

	std::unordered_map<std::string, friend_online_data> rpcn_client::get_presence_updates()
	{
		std::lock_guard lock(mutex_presence_updates);
		std::unordered_map<std::string, friend_online_data> ret_updates = std::move(presence_updates);
		presence_updates.clear();
		return ret_updates;
	}

	std::map<std::string, friend_online_data> rpcn_client::get_presence_states()
	{
		std::scoped_lock lock(mutex_friends, mutex_presence_updates);
		presence_updates.clear();
		return friend_infos.friends;
	}

	std::vector<u64> rpcn_client::get_new_messages()
	{
		std::lock_guard lock(mutex_messages);
		std::vector<u64> ret_new_messages = std::move(new_messages);
		new_messages.clear();
		return ret_new_messages;
	}

	bool rpcn_client::get_reply(const u64 expected_id, std::vector<u8>& data)
	{
		auto check_for_reply = [this, expected_id, &data]() -> bool
		{
			std::lock_guard lock(mutex_replies_sync);
			if (auto r = replies_sync.find(expected_id); r != replies_sync.end())
			{
				data = std::move(r->second.second);
				replies_sync.erase(r);
				return true;
			}
			return false;
		};

		while (connected && !terminate)
		{
			if (check_for_reply())
				return true;

			std::this_thread::sleep_for(5ms);
		}

		if (check_for_reply())
			return true;

		return false;
	}

	bool rpcn_client::get_server_list(u32 req_id, const SceNpCommunicationId& communication_id, std::vector<u16>& server_list)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE), reply_data;
		rpcn_client::write_communication_id(communication_id, data);

		if (!forge_send_reply(CommandType::GetServerList, req_id, data, reply_data))
		{
			return false;
		}

		vec_stream reply(reply_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error != rpcn::ErrorType::NoError)
		{
			return false;
		}

		u16 num_servs = reply.get<u16>();

		server_list.clear();
		for (u16 i = 0; i < num_servs; i++)
		{
			server_list.push_back(reply.get<u16>());
		}

		if (reply.is_error())
		{
			server_list.clear();
			return error_and_disconnect("Malformed reply to GetServerList command");
		}

		return true;
	}

	u64 rpcn_client::get_network_time(u32 req_id)
	{
		std::vector<u8> data, reply_data;
		if (!forge_send_reply(CommandType::GetNetworkTime, req_id, data, reply_data))
		{
			return 0;
		}

		vec_stream reply(reply_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (error != rpcn::ErrorType::NoError)
		{
			return 0;
		}

		u64 network_time = reply.get<u64>();

		if (reply.is_error())
		{
			error_and_disconnect("Malformed reply to GetNetworkTime command");
			return 0;
		}

		return network_time;
	}

	bool rpcn_client::get_world_list(u32 req_id, const SceNpCommunicationId& communication_id, u16 server_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u16));
		rpcn_client::write_communication_id(communication_id, data);
		reinterpret_cast<le_t<u16>&>(data[COMMUNICATION_ID_SIZE]) = server_id;

		return forge_send(CommandType::GetWorldList, req_id, data);
	}

	bool rpcn_client::createjoin_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2CreateJoinRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntAttr>>> final_searchintattrexternal_vec;
		if (req->roomSearchableIntAttrExternalNum && req->roomSearchableIntAttrExternal)
		{
			std::vector<flatbuffers::Offset<IntAttr>> davec;
			for (u32 i = 0; i < req->roomSearchableIntAttrExternalNum; i++)
			{
				auto bin = CreateIntAttr(builder, req->roomSearchableIntAttrExternal[i].id, req->roomSearchableIntAttrExternal[i].num);
				davec.push_back(bin);
			}
			final_searchintattrexternal_vec = builder.CreateVector(davec);
		}

		// WWE SmackDown vs. RAW 2009 passes roomBinAttrExternal in roomSearchableBinAttrExternal so we parse based on attribute ids

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_searchbinattrexternal_vec;
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrexternal_vec;

		std::vector<flatbuffers::Offset<BinAttr>> davec_binattrinternal;
		std::vector<flatbuffers::Offset<BinAttr>> davec_searchable_binattrexternal;
		std::vector<flatbuffers::Offset<BinAttr>> davec_binattrexternal;

		auto put_binattr = [&](SceNpMatching2AttributeId id, flatbuffers::Offset<BinAttr> bin)
		{
			switch (id)
			{
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_1_ID:
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_2_ID:
				davec_binattrinternal.push_back(bin);
				break;
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID:
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID:
				davec_binattrexternal.push_back(bin);
				break;
			case SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID:
				davec_searchable_binattrexternal.push_back(bin);
				break;
			default:
				rpcn_log.error("Unexpected bin attribute id in createjoin_room request: 0x%x", id);
				break;
			}
		};

		if (req->roomBinAttrInternalNum && req->roomBinAttrInternal)
		{
			for (u32 i = 0; i < req->roomBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrInternal[i].id, builder.CreateVector(req->roomBinAttrInternal[i].ptr.get_ptr(), req->roomBinAttrInternal[i].size));
				put_binattr(req->roomBinAttrInternal[i].id, bin);
			}
		}

		if (req->roomSearchableBinAttrExternalNum && req->roomSearchableBinAttrExternal)
		{
			for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomSearchableBinAttrExternal[i].id, builder.CreateVector(req->roomSearchableBinAttrExternal[i].ptr.get_ptr(), req->roomSearchableBinAttrExternal[i].size));
				put_binattr(req->roomSearchableBinAttrExternal[i].id, bin);
			}
		}

		if (req->roomBinAttrExternalNum && req->roomBinAttrExternal)
		{
			for (u32 i = 0; i < req->roomBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrExternal[i].id, builder.CreateVector(req->roomBinAttrExternal[i].ptr.get_ptr(), req->roomBinAttrExternal[i].size));
				put_binattr(req->roomBinAttrExternal[i].id, bin);
			}
		}

		if (!davec_binattrinternal.empty())
			final_binattrinternal_vec = builder.CreateVector(davec_binattrinternal);

		if (!davec_searchable_binattrexternal.empty())
			final_searchbinattrexternal_vec = builder.CreateVector(davec_searchable_binattrexternal);

		if (!davec_binattrexternal.empty())
			final_binattrexternal_vec = builder.CreateVector(davec_binattrexternal);

		flatbuffers::Offset<flatbuffers::Vector<u8>> final_roompassword;
		if (req->roomPassword)
			final_roompassword = builder.CreateVector(req->roomPassword->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<GroupConfig>>> final_groupconfigs_vec;
		if (req->groupConfigNum && req->groupConfig)
		{
			std::vector<flatbuffers::Offset<GroupConfig>> davec;
			for (u32 i = 0; i < req->groupConfigNum; i++)
			{
				auto bin = CreateGroupConfig(builder, req->groupConfig[i].slotNum, req->groupConfig[i].withLabel ? builder.CreateVector(req->groupConfig[i].label.data, 8) : 0, req->groupConfig[i].withPassword);
				davec.push_back(bin);
			}
			final_groupconfigs_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> final_allowedusers_vec;
		if (req->allowedUserNum && req->allowedUser)
		{
			std::vector<flatbuffers::Offset<flatbuffers::String>> davec;
			for (u32 i = 0; i < req->allowedUserNum; i++)
			{
				// Some games just give us garbage, make sure npid is valid before passing
				// Ex: Aquapazza (gives uninitialized buffer on the stack and allowedUserNum is hardcoded to 100)
				if (!np::is_valid_npid(req->allowedUser[i]))
				{
					continue;
				}

				auto bin = builder.CreateString(req->allowedUser[i].handle.data);
				davec.push_back(bin);
			}
			final_allowedusers_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> final_blockedusers_vec;
		if (req->blockedUserNum && req->blockedUser)
		{
			std::vector<flatbuffers::Offset<flatbuffers::String>> davec;
			for (u32 i = 0; i < req->blockedUserNum; i++)
			{
				if (!np::is_valid_npid(req->blockedUser[i]))
				{
					continue;
				}

				auto bin = builder.CreateString(req->blockedUser[i].handle.data);
				davec.push_back(bin);
			}
			final_blockedusers_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<u8>> final_grouplabel;
		if (req->joinRoomGroupLabel)
			final_grouplabel = builder.CreateVector(req->joinRoomGroupLabel->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_memberbinattrinternal_vec;
		if (req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomMemberBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(
					builder, req->roomMemberBinAttrInternal[i].id, builder.CreateVector(reinterpret_cast<const u8*>(req->roomMemberBinAttrInternal[i].ptr.get_ptr()), req->roomMemberBinAttrInternal[i].size));
				davec.push_back(bin);
			}
			final_memberbinattrinternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<OptParam> final_optparam;
		if (req->sigOptParam)
			final_optparam = CreateOptParam(builder, req->sigOptParam->type, req->sigOptParam->flag, req->sigOptParam->hubMemberId);
		u64 final_passwordSlotMask = 0;
		if (req->passwordSlotMask)
			final_passwordSlotMask = *req->passwordSlotMask;

		auto req_finished = CreateCreateJoinRoomRequest(builder, req->worldId, req->lobbyId, req->maxSlot, req->flagAttr, final_binattrinternal_vec, final_searchintattrexternal_vec,
			final_searchbinattrexternal_vec, final_binattrexternal_vec, final_roompassword, final_groupconfigs_vec, final_passwordSlotMask, final_allowedusers_vec, final_blockedusers_vec, final_grouplabel,
			final_memberbinattrinternal_vec, req->teamId, final_optparam);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::CreateRoom, req_id);
	}

	bool rpcn_client::join_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2JoinRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<u8>> final_roompassword;
		if (req->roomPassword)
			final_roompassword = builder.CreateVector(req->roomPassword->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<u8>> final_grouplabel;
		if (req->joinRoomGroupLabel)
			final_grouplabel = builder.CreateVector(req->joinRoomGroupLabel->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_memberbinattrinternal_vec;
		if (req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomMemberBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomMemberBinAttrInternal[i].id, builder.CreateVector(req->roomMemberBinAttrInternal[i].ptr.get_ptr(), req->roomMemberBinAttrInternal[i].size));
				davec.push_back(bin);
			}
			final_memberbinattrinternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<PresenceOptionData> final_optdata = CreatePresenceOptionData(builder, builder.CreateVector(req->optData.data, 16), req->optData.length);

		auto req_finished = CreateJoinRoomRequest(builder, req->roomId, final_roompassword, final_grouplabel, final_memberbinattrinternal_vec, final_optdata, req->teamId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::JoinRoom, req_id);
	}

	bool rpcn_client::leave_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2LeaveRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<PresenceOptionData> final_optdata = CreatePresenceOptionData(builder, builder.CreateVector(req->optData.data, 16), req->optData.length);
		auto req_finished = CreateLeaveRoomRequest(builder, req->roomId, final_optdata);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::LeaveRoom, req_id);
	}

	bool rpcn_client::search_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SearchRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntSearchFilter>>> final_intfilter_vec;
		if (req->intFilterNum && req->intFilter)
		{
			std::vector<flatbuffers::Offset<IntSearchFilter>> davec{};
			for (u32 i = 0; i < req->intFilterNum; i++)
			{
				auto int_attr = CreateIntAttr(builder, req->intFilter[i].attr.id, req->intFilter[i].attr.num);
				auto bin = CreateIntSearchFilter(builder, req->intFilter[i].searchOperator, int_attr);
				davec.push_back(bin);
			}
			final_intfilter_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinSearchFilter>>> final_binfilter_vec;
		if (req->binFilterNum && req->binFilter)
		{
			std::vector<flatbuffers::Offset<BinSearchFilter>> davec;
			for (u32 i = 0; i < req->binFilterNum; i++)
			{
				auto bin_attr = CreateBinAttr(builder, req->binFilter[i].attr.id, builder.CreateVector(req->binFilter[i].attr.ptr.get_ptr(), req->binFilter[i].attr.size));
				auto bin = CreateBinSearchFilter(builder, req->binFilter[i].searchOperator, bin_attr);
				davec.push_back(bin);
			}
			final_binfilter_vec = builder.CreateVector(davec);
		}

		flatbuffers::Offset<flatbuffers::Vector<u16>> attrid_vec;
		if (req->attrIdNum && req->attrId)
		{
			std::vector<u16> attr_ids;
			for (u32 i = 0; i < req->attrIdNum; i++)
			{
				attr_ids.push_back(req->attrId[i]);
			}
			attrid_vec = builder.CreateVector(attr_ids);
		}

		SearchRoomRequestBuilder s_req(builder);
		s_req.add_option(req->option);
		s_req.add_worldId(req->worldId);
		s_req.add_lobbyId(req->lobbyId);
		s_req.add_rangeFilter_startIndex(req->rangeFilter.startIndex);
		s_req.add_rangeFilter_max(req->rangeFilter.max);
		s_req.add_flagFilter(req->flagFilter);
		s_req.add_flagAttr(req->flagAttr);
		if (req->intFilterNum)
			s_req.add_intFilter(final_intfilter_vec);
		if (req->binFilterNum)
			s_req.add_binFilter(final_binfilter_vec);
		if (req->attrIdNum)
			s_req.add_attrId(attrid_vec);

		auto req_finished = s_req.Finish();
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SearchRoom, req_id);
	}

	bool rpcn_client::get_roomdata_external_list(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		std::vector<u64> roomIds;
		for (u32 i = 0; i < req->roomIdNum && req->roomId; i++)
		{
			roomIds.push_back(req->roomId[i]);
		}
		std::vector<u16> attrIds;
		for (u32 i = 0; i < req->attrIdNum && req->attrId; i++)
		{
			attrIds.push_back(req->attrId[i]);
		}

		auto req_finished = CreateGetRoomDataExternalListRequestDirect(builder, &roomIds, &attrIds);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetRoomDataExternalList, req_id);
	}

	bool rpcn_client::set_roomdata_external(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntAttr>>> final_searchintattrexternal_vec;
		if (req->roomSearchableIntAttrExternalNum && req->roomSearchableIntAttrExternal)
		{
			std::vector<flatbuffers::Offset<IntAttr>> davec;
			for (u32 i = 0; i < req->roomSearchableIntAttrExternalNum; i++)
			{
				auto bin = CreateIntAttr(builder, req->roomSearchableIntAttrExternal[i].id, req->roomSearchableIntAttrExternal[i].num);
				davec.push_back(bin);
			}
			final_searchintattrexternal_vec = builder.CreateVector(davec);
		}

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_searchbinattrexternal_vec;
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrexternal_vec;

		std::vector<flatbuffers::Offset<BinAttr>> davec_searchable_binattrexternal;
		std::vector<flatbuffers::Offset<BinAttr>> davec_binattrexternal;

		auto put_binattr = [&](SceNpMatching2AttributeId id, flatbuffers::Offset<BinAttr> bin)
		{
			switch (id)
			{
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID:
			case SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID:
				davec_binattrexternal.push_back(bin);
				break;
			case SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID:
				davec_searchable_binattrexternal.push_back(bin);
				break;
			default:
				rpcn_log.error("Unexpected bin attribute id in set_roomdata_external request: 0x%x", id);
				break;
			}
		};

		if (req->roomSearchableBinAttrExternalNum && req->roomSearchableBinAttrExternal)
		{
			for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomSearchableBinAttrExternal[i].id, builder.CreateVector(req->roomSearchableBinAttrExternal[i].ptr.get_ptr(), req->roomSearchableBinAttrExternal[i].size));
				put_binattr(req->roomSearchableBinAttrExternal[i].id, bin);
			}
		}

		if (req->roomBinAttrExternalNum && req->roomBinAttrExternal)
		{
			for (u32 i = 0; i < req->roomBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrExternal[i].id, builder.CreateVector(req->roomBinAttrExternal[i].ptr.get_ptr(), req->roomBinAttrExternal[i].size));
				put_binattr(req->roomBinAttrExternal[i].id, bin);
			}
		}

		if (!davec_searchable_binattrexternal.empty())
			final_searchbinattrexternal_vec = builder.CreateVector(davec_searchable_binattrexternal);

		if (!davec_binattrexternal.empty())
			final_binattrexternal_vec = builder.CreateVector(davec_binattrexternal);

		auto req_finished = CreateSetRoomDataExternalRequest(builder, req->roomId, final_searchintattrexternal_vec, final_searchbinattrexternal_vec, final_binattrexternal_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SetRoomDataExternal, req_id);
	}

	bool rpcn_client::get_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<u16>> final_attr_ids_vec;
		if (req->attrIdNum && req->attrId)
		{
			std::vector<u16> attr_ids;
			for (u32 i = 0; i < req->attrIdNum; i++)
			{
				attr_ids.push_back(req->attrId[i]);
			}
			final_attr_ids_vec = builder.CreateVector(attr_ids);
		}

		auto req_finished = CreateGetRoomDataInternalRequest(builder, req->roomId, final_attr_ids_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetRoomDataInternal, req_id);
	}

	bool rpcn_client::set_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		if (req->roomBinAttrInternalNum && req->roomBinAttrInternal)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrInternal[i].id, builder.CreateVector(req->roomBinAttrInternal[i].ptr.get_ptr(), req->roomBinAttrInternal[i].size));
				davec.push_back(bin);
			}
			final_binattrinternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<RoomGroupPasswordConfig>>> final_grouppasswordconfig_vec;
		if (req->passwordConfigNum && req->passwordConfig)
		{
			std::vector<flatbuffers::Offset<RoomGroupPasswordConfig>> davec;
			for (u32 i = 0; i < req->passwordConfigNum; i++)
			{
				auto rg = CreateRoomGroupPasswordConfig(builder, req->passwordConfig[i].groupId, req->passwordConfig[i].withPassword);
				davec.push_back(rg);
			}
			final_grouppasswordconfig_vec = builder.CreateVector(davec);
		}
		u64 final_passwordSlotMask = 0;
		if (req->passwordSlotMask)
			final_passwordSlotMask = *req->passwordSlotMask;

		flatbuffers::Offset<flatbuffers::Vector<u16>> final_ownerprivilege_vec;
		if (req->ownerPrivilegeRankNum && req->ownerPrivilegeRank)
		{
			std::vector<u16> priv_ranks;
			for (u32 i = 0; i < req->ownerPrivilegeRankNum; i++)
			{
				priv_ranks.push_back(req->ownerPrivilegeRank[i]);
			}
			final_ownerprivilege_vec = builder.CreateVector(priv_ranks);
		}

		auto req_finished =
			CreateSetRoomDataInternalRequest(builder, req->roomId, req->flagFilter, req->flagAttr, final_binattrinternal_vec, final_grouppasswordconfig_vec, final_passwordSlotMask, final_ownerprivilege_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SetRoomDataInternal, req_id);
	}

	bool rpcn_client::get_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomMemberDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<u16>> final_attrid_vec;
		if (req->attrIdNum && req->attrId)
		{
			std::vector<u16> attrid_vec;
			for (u32 i = 0; i < req->attrIdNum; i++)
			{
				attrid_vec.push_back(req->attrId[i]);
			}
			final_attrid_vec = builder.CreateVector(attrid_vec);
		}

		auto req_finished = CreateGetRoomMemberDataInternalRequest(builder, req->roomId, req->memberId, final_attrid_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetRoomMemberDataInternal, req_id);
	}

	bool rpcn_client::set_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		if (req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomMemberBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomMemberBinAttrInternal[i].id, builder.CreateVector(req->roomMemberBinAttrInternal[i].ptr.get_ptr(), req->roomMemberBinAttrInternal[i].size));
				davec.push_back(bin);
			}
			final_binattrinternal_vec = builder.CreateVector(davec);
		}

		auto req_finished = CreateSetRoomMemberDataInternalRequest(builder, req->roomId, req->memberId, req->teamId, final_binattrinternal_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SetRoomMemberDataInternal, req_id);
	}

	bool rpcn_client::set_userinfo(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetUserInfoRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_memberbinattr_vec;
		if (req->userBinAttrNum && req->userBinAttr)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->userBinAttrNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->userBinAttr[i].id, builder.CreateVector(req->userBinAttr[i].ptr.get_ptr(), req->userBinAttr[i].size));
				davec.push_back(bin);
			}
			final_memberbinattr_vec = builder.CreateVector(davec);
		}

		auto req_finished = CreateSetUserInfo(builder, req->serverId, final_memberbinattr_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SetUserInfo, req_id);
	}

	bool rpcn_client::ping_room_owner(u32 req_id, const SceNpCommunicationId& communication_id, u64 room_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u64));

		rpcn_client::write_communication_id(communication_id, data);
		write_to_ptr<le_t<u64>>(data, COMMUNICATION_ID_SIZE, room_id);

		return forge_send(CommandType::PingRoomOwner, req_id, data);
	}

	bool rpcn_client::send_room_message(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SendRoomMessageRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<u16> dst;
		switch (req->castType)
		{
		case SCE_NP_MATCHING2_CASTTYPE_BROADCAST:
			break;
		case SCE_NP_MATCHING2_CASTTYPE_UNICAST:
			dst.push_back(req->dst.unicastTarget);
			break;
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST:
			for (u32 i = 0; i < req->dst.multicastTarget.memberIdNum && req->dst.multicastTarget.memberId; i++)
			{
				dst.push_back(req->dst.multicastTarget.memberId[i]);
			}
			break;
		case SCE_NP_MATCHING2_CASTTYPE_MULTICAST_TEAM:
			dst.push_back(req->dst.multicastTargetTeamId);
			break;
		default:
			ensure(false);
			break;
		}

		auto req_finished = CreateSendRoomMessageRequest(builder, req->roomId, req->castType, builder.CreateVector(dst.data(), dst.size()), builder.CreateVector(reinterpret_cast<const u8*>(req->msg.get_ptr()), req->msgLen), req->option);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::SendRoomMessage, req_id);
	}

	bool rpcn_client::req_sign_infos(u32 req_id, const std::string& npid)
	{
		std::vector<u8> data;
		std::copy(npid.begin(), npid.end(), std::back_inserter(data));
		data.push_back(0);

		return forge_send(CommandType::RequestSignalingInfos, req_id, data);
	}

	bool rpcn_client::req_ticket(u32 req_id, const std::string& service_id, const std::vector<u8>& cookie)
	{
		std::vector<u8> data;
		std::copy(service_id.begin(), service_id.end(), std::back_inserter(data));
		data.push_back(0);
		const le_t<u32> size = ::size32(cookie);
		std::copy(reinterpret_cast<const u8*>(&size), reinterpret_cast<const u8*>(&size) + sizeof(le_t<u32>), std::back_inserter(data));
		std::copy(cookie.begin(), cookie.end(), std::back_inserter(data));

		return forge_send(CommandType::RequestTicket, req_id, data);
	}

	bool rpcn_client::send_message(const message_data& msg_data, const std::set<std::string>& npids)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::FlatBufferBuilder nested_builder(1024);
		auto fb_message = CreateMessageDetailsDirect(nested_builder, static_cast<const char*>(msg_data.commId.data), msg_data.msgId, msg_data.mainType, msg_data.subType, msg_data.msgFeatures, msg_data.subject.c_str(), msg_data.body.c_str(), &msg_data.data);
		nested_builder.Finish(fb_message);
		builder.ForceVectorAlignment(nested_builder.GetSize(), sizeof(uint8_t), nested_builder.GetBufferMinAlignment());
		auto nested_flatbuffer_vector = builder.CreateVector(nested_builder.GetBufferPointer(), nested_builder.GetSize());

		std::vector<flatbuffers::Offset<flatbuffers::String>> davec;
		for (const auto& npid : npids)
		{
			auto s_npid = builder.CreateString(npid);
			davec.push_back(s_npid);
		}
		auto npids_vector = builder.CreateVector(davec);

		// auto npids = builder.Create
		auto fb_sendmessage = CreateSendMessageRequest(builder, nested_flatbuffer_vector, npids_vector);

		builder.Finish(fb_sendmessage);
		const u8* buf = builder.GetBufferPointer();
		const usz bufsize = builder.GetSize();
		std::vector<u8> data(bufsize + sizeof(u32));

		reinterpret_cast<le_t<u32>&>(data[0]) = static_cast<u32>(bufsize);
		memcpy(data.data() + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SendMessage, rpcn_request_counter.fetch_add(1), data);
	}

	bool rpcn_client::get_board_infos(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32));
		rpcn_client::write_communication_id(communication_id, data);
		write_to_ptr<le_t<u32>>(data, COMMUNICATION_ID_SIZE, board_id);

		return forge_send(CommandType::GetBoardInfos, req_id, data);
	}

	bool rpcn_client::record_score(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, SceNpScorePcId char_id, SceNpScoreValue score, const std::optional<std::string> comment, const std::optional<std::vector<u8>> score_data)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		auto req_finished = CreateRecordScoreRequestDirect(builder, board_id, char_id, score, comment ? (*comment).c_str() : nullptr, score_data ? &*score_data : nullptr);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::RecordScore, req_id);
	}

	bool rpcn_client::get_score_range(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, u32 start_rank, u32 num_rank, bool with_comment, bool with_gameinfo)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateGetScoreRangeRequest(builder, board_id, start_rank, num_rank, with_comment, with_gameinfo);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetScoreRange, req_id);
	}

	bool rpcn_client::get_score_npid(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, const std::vector<std::pair<SceNpId, s32>>& npids, bool with_comment, bool with_gameinfo)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<flatbuffers::Offset<ScoreNpIdPcId>> davec;
		for (usz i = 0; i < npids.size(); i++)
		{
			auto npid = CreateScoreNpIdPcId(builder, builder.CreateString(static_cast<const char*>(npids[i].first.handle.data)), npids[i].second);
			davec.push_back(npid);
		}

		auto req_finished = CreateGetScoreNpIdRequest(builder, board_id, builder.CreateVector(davec), with_comment, with_gameinfo);

		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetScoreNpid, req_id);
	}

	bool rpcn_client::get_score_friend(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, bool include_self, bool with_comment, bool with_gameinfo, u32 max_entries)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateGetScoreFriendsRequest(builder, board_id, include_self, max_entries, with_comment, with_gameinfo);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetScoreFriends, req_id);
	}

	bool rpcn_client::record_score_data(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScorePcId pc_id, SceNpScoreBoardId board_id, s64 score, const std::vector<u8>& score_data)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateRecordScoreGameDataRequest(builder, board_id, pc_id, score);
		builder.Finish(req_finished);

		const u8* buf = builder.GetBufferPointer();
		const usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize + sizeof(u32) + score_data.size());

		rpcn_client::write_communication_id(communication_id, data);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize]) = static_cast<u32>(score_data.size());
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize + sizeof(u32), score_data.data(), score_data.size());

		return forge_send(CommandType::RecordScoreData, req_id, data);
	}

	bool rpcn_client::get_score_data(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScorePcId pc_id, SceNpScoreBoardId board_id, const SceNpId& npid)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateGetScoreGameDataRequest(builder, board_id, builder.CreateString(reinterpret_cast<const char*>(npid.handle.data)), pc_id);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetScoreData, req_id);
	}

	bool rpcn_client::tus_set_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, vm::cptr<s64> variableArray, s32 arrayNum, bool vuser)
	{
		const std::vector<s32> slotid_array(slotIdArray.get_ptr(), slotIdArray.get_ptr() + arrayNum);
		const std::vector<s64> variable_array(variableArray.get_ptr(), variableArray.get_ptr() + arrayNum);

		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusSetMultiSlotVariableRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), builder.CreateVector(slotid_array), builder.CreateVector(variable_array));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusSetMultiSlotVariable, req_id);
	}

	bool rpcn_client::tus_get_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser)
	{
		const std::vector<s32> slotid_array(slotIdArray.get_ptr(), slotIdArray.get_ptr() + arrayNum);

		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusGetMultiSlotVariableRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), builder.CreateVector(slotid_array));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetMultiSlotVariable, req_id);
	}

	bool rpcn_client::tus_get_multiuser_variable(u32 req_id, const SceNpCommunicationId& communication_id, const std::vector<SceNpOnlineId>& targetNpIdArray, SceNpTusSlotId slotId, s32 arrayNum, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<flatbuffers::Offset<TusUser>> davec;
		for (s32 i = 0; i < std::min(arrayNum, ::narrow<s32>(targetNpIdArray.size())); i++)
		{
			davec.push_back(CreateTusUser(builder, vuser, builder.CreateString(targetNpIdArray[i].data)));
		}

		auto req_finished = CreateTusGetMultiUserVariableRequest(builder, builder.CreateVector(davec), slotId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetMultiUserVariable, req_id);
	}

	bool rpcn_client::tus_get_friends_variable(u32 req_id, const SceNpCommunicationId& communication_id, SceNpTusSlotId slotId, bool includeSelf, s32 sortType, s32 arrayNum)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusGetFriendsVariableRequest(builder, slotId, includeSelf, sortType, arrayNum);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetFriendsVariable, req_id);
	}

	bool rpcn_client::tus_add_and_get_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s64 inVariable, vm::ptr<SceNpTusAddAndGetVariableOptParam> option, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<uint64_t>> isLastChangedDate;
		flatbuffers::Offset<flatbuffers::String> isLastChangedAuthorId;

		if (option)
		{
			if (option->isLastChangedDate)
			{
				std::vector<u64> is_last_changed_date_vec;
				is_last_changed_date_vec.push_back(option->isLastChangedDate->tick);
				isLastChangedDate = builder.CreateVector(is_last_changed_date_vec);
			}

			if (option->isLastChangedAuthorId)
			{
				isLastChangedAuthorId = builder.CreateString(option->isLastChangedAuthorId->handle.data);
			}
		}

		auto req_finished = CreateTusAddAndGetVariableRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), slotId, inVariable, isLastChangedDate, isLastChangedAuthorId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusAddAndGetVariable, req_id);
	}

	bool rpcn_client::tus_try_and_set_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, s32 opeType, s64 variable, vm::ptr<SceNpTusTryAndSetVariableOptParam> option, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<uint64_t>> isLastChangedDate;
		flatbuffers::Offset<flatbuffers::String> isLastChangedAuthorId;
		flatbuffers::Offset<flatbuffers::Vector<int64_t>> compareValue;

		if (option)
		{
			if (option->isLastChangedDate)
			{
				std::vector<u64> is_last_changed_date_vec;
				is_last_changed_date_vec.push_back(option->isLastChangedDate->tick);
				isLastChangedDate = builder.CreateVector(is_last_changed_date_vec);
			}

			if (option->isLastChangedAuthorId)
			{
				isLastChangedAuthorId = builder.CreateString(option->isLastChangedAuthorId->handle.data);
			}

			if (option->compareValue)
			{
				std::vector<s64> compare_value_vec;
				compare_value_vec.push_back(*(option->compareValue));
				compareValue = builder.CreateVector(compare_value_vec);
			}
		}

		auto req_finished = CreateTusTryAndSetVariableRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), slotId, opeType, variable, isLastChangedDate, isLastChangedAuthorId, compareValue);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusTryAndSetVariable, req_id);
	}

	bool rpcn_client::tus_delete_multislot_variable(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser)
	{
		const std::vector<s32> slotid_array(slotIdArray.get_ptr(), slotIdArray.get_ptr() + arrayNum);

		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusDeleteMultiSlotVariableRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), builder.CreateVector(slotid_array));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusDeleteMultiSlotVariable, req_id);
	}

	bool rpcn_client::tus_set_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, const std::vector<u8>& tus_data, vm::cptr<SceNpTusDataInfo> info, vm::ptr<SceNpTusSetDataOptParam> option, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<uint64_t>> isLastChangedDate;
		flatbuffers::Offset<flatbuffers::String> isLastChangedAuthorId;

		if (option)
		{
			if (option->isLastChangedDate)
			{
				std::vector<u64> is_last_changed_date_vec;
				is_last_changed_date_vec.push_back(option->isLastChangedDate->tick);
				isLastChangedDate = builder.CreateVector(is_last_changed_date_vec);
			}

			if (option->isLastChangedAuthorId)
			{
				isLastChangedAuthorId = builder.CreateString(option->isLastChangedAuthorId->handle.data);
			}
		}

		flatbuffers::Offset<flatbuffers::Vector<uint8_t>> fb_info;

		if (info)
		{
			fb_info = builder.CreateVector(info->data, static_cast<size_t>(info->infoSize));
		}

		auto req_finished = CreateTusSetDataRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), slotId, builder.CreateVector(tus_data), fb_info, isLastChangedDate, isLastChangedAuthorId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusSetData, req_id);
	}

	bool rpcn_client::tus_get_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, SceNpTusSlotId slotId, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusGetDataRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), slotId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetData, req_id);
	}

	bool rpcn_client::tus_get_multislot_data_status(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser)
	{
		const std::vector<s32> slotid_array(slotIdArray.get_ptr(), slotIdArray.get_ptr() + arrayNum);

		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusGetMultiSlotDataStatusRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), builder.CreateVector(slotid_array));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetMultiSlotDataStatus, req_id);
	}

	bool rpcn_client::tus_get_multiuser_data_status(u32 req_id, SceNpCommunicationId& communication_id, const std::vector<SceNpOnlineId>& targetNpIdArray, SceNpTusSlotId slotId, s32 arrayNum, bool vuser)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<flatbuffers::Offset<TusUser>> davec;
		for (s32 i = 0; i < std::min(arrayNum, ::narrow<s32>(targetNpIdArray.size())); i++)
		{
			davec.push_back(CreateTusUser(builder, vuser, builder.CreateString(targetNpIdArray[i].data)));
		}

		auto req_finished = CreateTusGetMultiUserDataStatusRequest(builder, builder.CreateVector(davec), slotId);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetMultiUserDataStatus, req_id);
	}

	bool rpcn_client::tus_get_friends_data_status(u32 req_id, SceNpCommunicationId& communication_id, SceNpTusSlotId slotId, bool includeSelf, s32 sortType, s32 arrayNum)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusGetFriendsDataStatusRequest(builder, slotId, includeSelf, sortType, arrayNum);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusGetFriendsDataStatus, req_id);
	}

	bool rpcn_client::tus_delete_multislot_data(u32 req_id, SceNpCommunicationId& communication_id, const SceNpOnlineId& targetNpId, vm::cptr<SceNpTusSlotId> slotIdArray, s32 arrayNum, bool vuser)
	{
		const std::vector<s32> slotid_array(slotIdArray.get_ptr(), slotIdArray.get_ptr() + arrayNum);

		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateTusDeleteMultiSlotDataRequest(builder, CreateTusUser(builder, vuser, builder.CreateString(targetNpId.data)), builder.CreateVector(slotid_array));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::TusDeleteMultiSlotData, req_id);
	}

	bool rpcn_client::send_presence(const SceNpCommunicationId& pr_com_id, const std::string& pr_title, const std::string& pr_status, const std::string& pr_comment, const std::vector<u8>& pr_data)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateSetPresenceRequest(builder, builder.CreateString(pr_title), builder.CreateString(pr_status), builder.CreateString(pr_comment), builder.CreateVector(pr_data));
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, pr_com_id, CommandType::SetPresence, rpcn_request_counter.fetch_add(1));
	}

	bool rpcn_client::createjoin_room_gui(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatchingAttr* attr_list)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		u32 total_slots = 0;
		u32 private_slots = 0;
		bool privilege_grant = false;
		bool stealth = false;

		std::vector<flatbuffers::Offset<MatchingAttr>> vec_attrs;

		for (const SceNpMatchingAttr* cur_attr = attr_list; cur_attr != nullptr; cur_attr = cur_attr->next ? cur_attr->next.get_ptr() : nullptr)
		{
			switch (cur_attr->type)
			{
			case SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM:
			{
				switch (cur_attr->id)
				{
				case SCE_NP_MATCHING_ROOM_ATTR_ID_TOTAL_SLOT:
					total_slots = cur_attr->value.num;
					break;
				case SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVATE_SLOT:
					private_slots = cur_attr->value.num;
					break;
				case SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVILEGE_TYPE:
					ensure(cur_attr->value.num == SCE_NP_MATCHING_ROOM_PRIVILEGE_TYPE_NO_AUTO_GRANT || cur_attr->value.num == SCE_NP_MATCHING_ROOM_PRIVILEGE_TYPE_AUTO_GRANT, "Invalid SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVILEGE_TYPE value");
					privilege_grant = (cur_attr->value.num == SCE_NP_MATCHING_ROOM_PRIVILEGE_TYPE_AUTO_GRANT);
					break;
				case SCE_NP_MATCHING_ROOM_ATTR_ID_ROOM_SEARCH_FLAG:
					ensure(cur_attr->value.num == SCE_NP_MATCHING_ROOM_SEARCH_FLAG_OPEN || cur_attr->value.num == SCE_NP_MATCHING_ROOM_SEARCH_FLAG_STEALTH, "Invalid SCE_NP_MATCHING_ROOM_ATTR_ID_ROOM_SEARCH_FLAG value");
					stealth = (cur_attr->value.num == SCE_NP_MATCHING_ROOM_SEARCH_FLAG_STEALTH);
					break;
				default:
					fmt::throw_exception("Invalid basic num attribute id");
					break;
				}

				break;
			}
			case SCE_NP_MATCHING_ATTR_TYPE_GAME_BIN:
			{
				ensure(cur_attr->id >= 1u && cur_attr->id <= 16u, "Invalid game bin attribute id");
				ensure(cur_attr->value.data.size <= 64u || ((cur_attr->id == 1u || cur_attr->id == 2u) && cur_attr->value.data.size <= 256u), "Invalid game bin size");

				const std::vector<u8> vec_data(static_cast<const u8*>(cur_attr->value.data.ptr.get_ptr()), static_cast<const u8*>(cur_attr->value.data.ptr.get_ptr()) + cur_attr->value.data.size);
				auto attr = CreateMatchingAttrDirect(builder, cur_attr->type, cur_attr->id, 0, &vec_data);
				vec_attrs.push_back(attr);
				break;
			}
			case SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM:
			{
				ensure(cur_attr->id >= 1u && cur_attr->id <= 16u, "Invalid game num attribute id");

				auto attr = CreateMatchingAttrDirect(builder, cur_attr->type, cur_attr->id, cur_attr->value.num, nullptr);
				vec_attrs.push_back(attr);
				break;
			}
			default:
				fmt::throw_exception("Invalid attribute type");
			}
		}

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<MatchingAttr>>> final_attrs_vec;

		if (!vec_attrs.empty())
		{
			final_attrs_vec = builder.CreateVector(vec_attrs);
		}

		auto req_finished = CreateCreateRoomGUIRequest(builder, total_slots, private_slots, privilege_grant, stealth, final_attrs_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::CreateRoomGUI, req_id);
	}

	bool rpcn_client::join_room_gui(u32 req_id, const SceNpRoomId& room_id)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		auto req_finished = CreateMatchingGuiRoomIdDirect(builder, &vec_room_id);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::JoinRoomGUI, req_id);
	}

	bool rpcn_client::leave_room_gui(u32 req_id, const SceNpRoomId& room_id)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		auto req_finished = CreateMatchingGuiRoomIdDirect(builder, &vec_room_id);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::LeaveRoomGUI, req_id);
	}

	bool rpcn_client::get_room_list_gui(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatchingReqRange* range, vm::ptr<SceNpMatchingSearchCondition> cond, vm::ptr<SceNpMatchingAttr> attr)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		const s32 range_start = range->start;
		const u32 range_max = range->max;

		std::vector<flatbuffers::Offset<MatchingSearchCondition>> vec_conds;
		std::vector<flatbuffers::Offset<MatchingAttr>> vec_attrs;

		for (auto cur_cond = cond; cur_cond; cur_cond = cur_cond->next)
		{
			auto fb_cond = CreateMatchingSearchCondition(builder, cur_cond->target_attr_type, cur_cond->target_attr_id, cur_cond->comp_op, cur_cond->compared.value.num);
			vec_conds.push_back(fb_cond);
		}

		for (auto cur_attr = attr; cur_attr; cur_attr = cur_attr->next)
		{
			auto fb_attr = CreateMatchingAttr(builder, cur_attr->type, cur_attr->id);
			vec_attrs.push_back(fb_attr);
		}

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<MatchingSearchCondition>>> final_conds_vec;
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<MatchingAttr>>> final_attrs_vec;

		if (!vec_conds.empty())
		{
			final_conds_vec = builder.CreateVector(vec_conds);
		}

		if (!vec_attrs.empty())
		{
			final_attrs_vec = builder.CreateVector(vec_attrs);
		}

		auto req_finished = CreateGetRoomListGUIRequest(builder, range_start, range_max, final_conds_vec, final_attrs_vec);
		builder.Finish(req_finished);

		return forge_request_with_com_id(builder, communication_id, CommandType::GetRoomListGUI, req_id);
	}

	bool rpcn_client::set_room_search_flag_gui(u32 req_id, const SceNpRoomId& room_id, bool stealth)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		auto req_finished = CreateSetRoomSearchFlagGUIDirect(builder, &vec_room_id, stealth);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::SetRoomSearchFlagGUI, req_id);
	}

	bool rpcn_client::get_room_search_flag_gui(u32 req_id, const SceNpRoomId& room_id)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		auto req_finished = CreateMatchingGuiRoomIdDirect(builder, &vec_room_id);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::GetRoomSearchFlagGUI, req_id);
	}

	bool rpcn_client::set_room_info_gui(u32 req_id, const SceNpRoomId& room_id, vm::ptr<SceNpMatchingAttr> attrs)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		std::vector<flatbuffers::Offset<MatchingAttr>> vec_attrs;

		for (auto cur_attr = attrs; cur_attr; cur_attr = cur_attr->next)
		{
			u32 num = 0;
			flatbuffers::Offset<flatbuffers::Vector<uint8_t>> fb_vec_data = 0;

			switch (cur_attr->type)
			{
			case SCE_NP_MATCHING_ATTR_TYPE_GAME_BIN:
			{
				const std::vector<u8> vec_data(static_cast<const u8*>(cur_attr->value.data.ptr.get_ptr()), static_cast<const u8*>(cur_attr->value.data.ptr.get_ptr()) + cur_attr->value.data.size);
				fb_vec_data = builder.CreateVector(vec_data);
				break;
			}
			case SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM:
			{
				num = cur_attr->value.num;
				break;
			}
			default: fmt::throw_exception("Invalid attr type reached set_room_info_gui");
			}

			auto fb_attr = CreateMatchingAttr(builder, cur_attr->type, cur_attr->id, num, fb_vec_data);
			vec_attrs.push_back(fb_attr);
		}

		auto req_finished = CreateMatchingRoomDirect(builder, &vec_room_id, &vec_attrs);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::SetRoomInfoGUI, req_id);
	}

	bool rpcn_client::get_room_info_gui(u32 req_id, const SceNpRoomId& room_id, vm::ptr<SceNpMatchingAttr> attrs)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		const std::vector<u8> vec_room_id(room_id.opt, room_id.opt + sizeof(room_id.opt));
		std::vector<flatbuffers::Offset<MatchingAttr>> vec_attrs;

		for (auto cur_attr = attrs; cur_attr; cur_attr = cur_attr->next)
		{
			auto fb_attr = CreateMatchingAttr(builder, cur_attr->type, cur_attr->id);
			vec_attrs.push_back(fb_attr);
		}

		auto req_finished = CreateMatchingRoomDirect(builder, &vec_room_id, &vec_attrs);
		builder.Finish(req_finished);
		return forge_request_with_data(builder, CommandType::GetRoomInfoGUI, req_id);
	}

	bool rpcn_client::quickmatch_gui(u32 req_id, const SceNpCommunicationId& com_id, vm::cptr<SceNpMatchingSearchCondition> cond, s32 available_num)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		std::vector<flatbuffers::Offset<MatchingSearchCondition>> vec_conds;

		for (auto cur_cond = cond; cur_cond; cur_cond = cur_cond->next)
		{
			auto fb_cond = CreateMatchingSearchCondition(builder, cur_cond->target_attr_type, cur_cond->target_attr_id, cur_cond->comp_op, cur_cond->compared.value.num);
			vec_conds.push_back(fb_cond);
		}

		auto req_finished = CreateQuickMatchGUIRequestDirect(builder, &vec_conds, available_num);
		builder.Finish(req_finished);
		return forge_request_with_com_id(builder, com_id, CommandType::QuickMatchGUI, req_id);
	}

	bool rpcn_client::searchjoin_gui(u32 req_id, const SceNpCommunicationId& com_id, vm::cptr<SceNpMatchingSearchCondition> cond, vm::cptr<SceNpMatchingAttr> attr)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<flatbuffers::Offset<MatchingSearchCondition>> vec_conds;
		std::vector<flatbuffers::Offset<MatchingAttr>> vec_attrs;

		for (auto cur_cond = cond; cur_cond; cur_cond = cur_cond->next)
		{
			auto fb_cond = CreateMatchingSearchCondition(builder, cur_cond->target_attr_type, cur_cond->target_attr_id, cur_cond->comp_op, cur_cond->compared.value.num);
			vec_conds.push_back(fb_cond);
		}

		for (auto cur_attr = attr; cur_attr; cur_attr = cur_attr->next)
		{
			auto fb_attr = CreateMatchingAttr(builder, cur_attr->type, cur_attr->id);
			vec_attrs.push_back(fb_attr);
		}

		auto req_finished = CreateSearchJoinRoomGUIRequestDirect(builder, vec_conds.empty() ? nullptr : &vec_conds, vec_attrs.empty() ? nullptr : &vec_attrs);
		builder.Finish(req_finished);
		return forge_request_with_com_id(builder, com_id, CommandType::SearchJoinRoomGUI, req_id);
	}

	void rpcn_client::write_communication_id(const SceNpCommunicationId& com_id, std::vector<u8>& data)
	{
		ensure(com_id.data[9] == 0 && com_id.num <= 99, "rpcn_client::write_communication_id: Invalid SceNpCommunicationId");
		const std::string com_id_str = np::communication_id_to_string(com_id);
		ensure(com_id_str.size() == 12, "rpcn_client::write_communication_id: Error formatting SceNpCommunicationId");
		memcpy(data.data(), com_id_str.data(), COMMUNICATION_ID_SIZE);
	}

	bool rpcn_client::forge_request_with_com_id(const flatbuffers::FlatBufferBuilder& builder, const SceNpCommunicationId& com_id, CommandType command, u64 packet_id)
	{
		const u8* buf = builder.GetBufferPointer();
		const usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		rpcn_client::write_communication_id(com_id, data);

		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(command, packet_id, data);
	}

	bool rpcn_client::forge_request_with_data(const flatbuffers::FlatBufferBuilder& builder, CommandType command, u64 packet_id)
	{
		const u8* buf = builder.GetBufferPointer();
		const usz bufsize = builder.GetSize();
		std::vector<u8> data(sizeof(u32) + bufsize);

		reinterpret_cast<le_t<u32>&>(data[0]) = static_cast<u32>(bufsize);
		memcpy(data.data() + sizeof(u32), buf, bufsize);

		return forge_send(command, packet_id, data);
	}

	std::vector<u8> rpcn_client::forge_request(rpcn::CommandType command, u64 packet_id, const std::vector<u8>& data) const
	{
		const usz packet_size = data.size() + RPCN_HEADER_SIZE;

		std::vector<u8> packet(packet_size);
		packet[0] = static_cast<u8>(PacketType::Request);
		reinterpret_cast<le_t<u16>&>(packet[1]) = static_cast<u16>(command);
		reinterpret_cast<le_t<u32>&>(packet[3]) = ::narrow<u32>(packet_size);
		reinterpret_cast<le_t<u64>&>(packet[7]) = packet_id;

		memcpy(packet.data() + RPCN_HEADER_SIZE, data.data(), data.size());
		return packet;
	}

	bool rpcn_client::error_and_disconnect(const std::string& error_msg)
	{
		connected = false;
		rpcn_log.error("%s", error_msg);
		return false;
	}

	bool rpcn_client::error_and_disconnect_notice(const std::string& error_msg)
	{
		connected = false;
		rpcn_log.notice("%s", error_msg);
		return false;
	}

	rpcn_state rpcn_client::wait_for_connection()
	{
		{
			std::lock_guard lock(mutex_connected);
			if (connected)
			{
				return rpcn_state::failure_no_failure;
			}

			if (state != rpcn_state::failure_no_failure)
			{
				return state;
			}

			want_conn = true;
			sem_rpcn.release();
		}

		sem_connected.acquire();

		return state;
	}

	rpcn_state rpcn_client::wait_for_authentified()
	{
		{
			std::lock_guard lock(mutex_authentified);

			if (authentified)
			{
				return rpcn_state::failure_no_failure;
			}

			if (state != rpcn_state::failure_no_failure)
			{
				return state;
			}

			want_auth = true;
			sem_rpcn.release();
		}

		sem_authentified.acquire();

		return state;
	}

	void rpcn_client::get_friends(friend_data& friend_infos)
	{
		std::lock_guard lock(mutex_friends);
		friend_infos = this->friend_infos;
	}

	void rpcn_client::get_friends_and_register_cb(friend_data& friend_infos, friend_cb_func cb_func, void* cb_param)
	{
		std::lock_guard lock(mutex_friends);

		friend_infos = this->friend_infos;
		friend_cbs.insert(std::make_pair(cb_func, cb_param));
	}

	void rpcn_client::register_friend_cb(friend_cb_func cb_func, void* cb_param)
	{
		std::lock_guard lock(mutex_friends);
		friend_cbs.insert(std::make_pair(cb_func, cb_param));
	}

	void rpcn_client::remove_friend_cb(friend_cb_func cb_func, void* cb_param)
	{
		std::lock_guard lock(mutex_friends);

		for (const auto& friend_cb : friend_cbs)
		{
			if (friend_cb.first == cb_func && friend_cb.second == cb_param)
			{
				friend_cbs.erase(friend_cb);
				break;
			}
		}
	}

	void rpcn_client::handle_friend_notification(rpcn::NotificationType ntype, std::vector<u8> data)
	{
		std::lock_guard lock(mutex_friends);

		vec_stream vdata(data);

		const auto call_callbacks = [&](NotificationType ntype, const std::string& username, bool status)
		{
			for (auto& friend_cb : friend_cbs)
			{
				friend_cb.first(friend_cb.second, ntype, username, status);
			}
		};

		switch (ntype)
		{
		case NotificationType::FriendQuery: // Other user sent a friend request
		{
			const std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendQuery notification");
				break;
			}

			friend_infos.requests_received.insert(username);
			call_callbacks(ntype, username, false);
			break;
		}
		case NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
		{
			const bool online = !!vdata.get<u8>();
			const std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendNew notification");
				break;
			}

			friend_infos.requests_received.erase(username);
			friend_infos.requests_sent.erase(username);
			friend_infos.friends.insert_or_assign(username, friend_online_data(online, 0));
			call_callbacks(ntype, username, online);

			break;
		}
		case NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
		{
			const std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendLost notification");
				break;
			}

			friend_infos.requests_received.erase(username);
			friend_infos.requests_sent.erase(username);
			friend_infos.friends.erase(username);
			call_callbacks(ntype, username, false);
			break;
		}
		case NotificationType::FriendStatus: // Set status of friend to Offline or Online
		{
			const bool online = !!vdata.get<u8>();
			const u64 timestamp = vdata.get<u64>();
			const std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendStatus notification");
				break;
			}

			if (auto u = friend_infos.friends.find(username); u != friend_infos.friends.end())
			{
				if (timestamp < u->second.timestamp)
				{
					break;
				}

				// clear presence data on online/offline
				u->second = friend_online_data(online, timestamp);

				{
					std::lock_guard lock(mutex_presence_updates);
					presence_updates.insert_or_assign(username, u->second);
				}

				call_callbacks(ntype, username, online);
			}
			break;
		}
		case NotificationType::FriendPresenceChanged:
		{
			const std::string username = vdata.get_string(true);
			SceNpCommunicationId pr_com_id = vdata.get_com_id();
			std::string pr_title = fmt::truncate(vdata.get_string(true), SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX - 1);
			std::string pr_status = fmt::truncate(vdata.get_string(true), SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_SIZE_MAX - 1);
			std::string pr_comment = fmt::truncate(vdata.get_string(true), SCE_NP_BASIC_PRESENCE_COMMENT_SIZE_MAX - 1);
			std::vector<u8> pr_data = vdata.get_rawdata();
			if (pr_data.size() > SCE_NP_BASIC_MAX_PRESENCE_SIZE)
			{
				pr_data.resize(SCE_NP_BASIC_MAX_PRESENCE_SIZE);
			}

			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendPresenceChanged notification");
				break;
			}

			if (auto u = friend_infos.friends.find(username); u != friend_infos.friends.end())
			{
				u->second.pr_com_id = std::move(pr_com_id);
				u->second.pr_title = std::move(pr_title);
				u->second.pr_status = std::move(pr_status);
				u->second.pr_comment = std::move(pr_comment);
				u->second.pr_data = std::move(pr_data);

				std::lock_guard lock(mutex_presence_updates);
				presence_updates.insert_or_assign(username, u->second);
			}

			call_callbacks(ntype, username, false);
			break;
		}
		default:
		{
			rpcn_log.fatal("Unknown notification forwarded to handle_friend_notification");
			break;
		}
		}
	}

	void rpcn_client::handle_message(std::vector<u8> data)
	{
		// Unserialize the message
		vec_stream sdata(data);
		std::string sender = sdata.get_string(false);
		auto* fb_mdata = sdata.get_flatbuffer<MessageDetails>();

		if (sdata.is_error())
		{
			return;
		}

		if (!fb_mdata->communicationId() || fb_mdata->communicationId()->size() == 0 || fb_mdata->communicationId()->size() > 9 ||
			!fb_mdata->subject() || !fb_mdata->body() || !fb_mdata->data())
		{
			rpcn_log.warning("Discarded invalid message!");
			return;
		}

		message_data mdata = {
			.msgId = message_counter,
			.mainType = fb_mdata->mainType(),
			.subType = fb_mdata->subType(),
			.msgFeatures = fb_mdata->msgFeatures(),
			.subject = fb_mdata->subject()->str(),
			.body = fb_mdata->body()->str()};

		strcpy_trunc(mdata.commId.data, fb_mdata->communicationId()->str());
		mdata.data.assign(fb_mdata->data()->Data(), fb_mdata->data()->Data() + fb_mdata->data()->size());

		// Save the message and call callbacks
		{
			std::lock_guard lock(mutex_messages);
			const u64 msg_id = message_counter++;
			auto id_and_msg = stx::make_shared<std::pair<std::string, message_data>>(std::make_pair(std::move(sender), std::move(mdata)));
			messages.emplace(msg_id, id_and_msg);
			new_messages.push_back(msg_id);
			active_messages.insert(msg_id);

			const auto& msg = id_and_msg->second;
			for (const auto& message_cb : message_cbs)
			{
				if (msg.mainType == message_cb.type_filter && (message_cb.inc_bootable || !(msg.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE)))
				{
					message_cb.cb_func(message_cb.cb_param, id_and_msg, msg_id);
				}
			}
		}
	}

	std::optional<shared_ptr<std::pair<std::string, message_data>>> rpcn_client::get_message(u64 id)
	{
		{
			std::lock_guard lock(mutex_messages);

			if (!messages.contains(id))
				return std::nullopt;

			return ::at32(messages, id);
		}
	}

	std::vector<std::pair<u64, shared_ptr<std::pair<std::string, message_data>>>> rpcn_client::get_messages_and_register_cb(SceNpBasicMessageMainType type_filter, bool include_bootable, message_cb_func cb_func, void* cb_param)
	{
		std::vector<std::pair<u64, shared_ptr<std::pair<std::string, message_data>>>> vec_messages;
		{
			std::lock_guard lock(mutex_messages);
			for (auto id : active_messages)
			{
				const auto& entry = ::at32(messages, id);
				const auto& msg = entry->second;
				if (msg.mainType == type_filter && (include_bootable || !(msg.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE)))
				{
					vec_messages.push_back(std::make_pair(id, entry));
				}
			}

			message_cbs.insert(message_cb_t{
				.cb_func = cb_func,
				.cb_param = cb_param,
				.type_filter = type_filter,
				.inc_bootable = include_bootable,
			});
		}
		return vec_messages;
	}

	void rpcn_client::remove_message_cb(message_cb_func cb_func, void* cb_param)
	{
		std::lock_guard lock(mutex_messages);

		for (const auto& message_cb : message_cbs)
		{
			if (message_cb.cb_func == cb_func && message_cb.cb_param == cb_param)
			{
				message_cbs.erase(message_cb);
				break;
			}
		}
	}

	void rpcn_client::mark_message_used(u64 id)
	{
		std::lock_guard lock(mutex_messages);

		active_messages.erase(id);
	}

	u32 rpcn_client::get_num_friends()
	{
		std::lock_guard lock(mutex_friends);

		return ::size32(friend_infos.friends);
	}

	u32 rpcn_client::get_num_blocks()
	{
		std::lock_guard lock(mutex_friends);

		return ::size32(friend_infos.blocked);
	}

	std::optional<std::string> rpcn_client::get_friend_by_index(u32 index)
	{
		std::lock_guard lock(mutex_friends);

		if (index >= friend_infos.friends.size())
		{
			return {};
		}

		auto it = friend_infos.friends.begin();
		for (usz i = 0; i < index; i++)
		{
			it++;
		}

		return it->first;
	}

	std::optional<std::pair<std::string, friend_online_data>> rpcn_client::get_friend_presence_by_index(u32 index)
	{
		std::lock_guard lock(mutex_friends);

		if (index >= friend_infos.friends.size())
		{
			return std::nullopt;
		}

		auto it = friend_infos.friends.begin();
		std::advance(it, index);
		return std::optional(*it);
	}

	std::optional<std::pair<std::string, friend_online_data>> rpcn_client::get_friend_presence_by_npid(const std::string& npid)
	{
		std::lock_guard lock(mutex_friends);
		const auto it = friend_infos.friends.find(npid);
		return it == friend_infos.friends.end() ? std::nullopt : std::optional(*it);
	}

	bool rpcn_client::is_connected() const
	{
		return connected;
	}

	bool rpcn_client::is_authentified() const
	{
		return authentified;
	}
	rpcn_state rpcn_client::get_rpcn_state() const
	{
		return state;
	}

	const std::string& rpcn_client::get_online_name() const
	{
		return online_name;
	}

	const std::string& rpcn_client::get_avatar_url() const
	{
		return avatar_url;
	}

	u32 rpcn_client::get_addr_sig() const
	{
		if (!addr_sig)
		{
			addr_sig.wait(0, static_cast<atomic_wait_timeout>(10'000'000'000));
		}

		return addr_sig.load();
	}

	u16 rpcn_client::get_port_sig() const
	{
		if (!port_sig)
		{
			port_sig.wait(0, static_cast<atomic_wait_timeout>(10'000'000'000));
		}

		return port_sig.load();
	}

	u32 rpcn_client::get_addr_local() const
	{
		return local_addr_sig.load();
	}

	void rpcn_client::update_local_addr(u32 addr)
	{
		local_addr_sig = std::bit_cast<u32, be_t<u32>>(addr);
	}

} // namespace rpcn
