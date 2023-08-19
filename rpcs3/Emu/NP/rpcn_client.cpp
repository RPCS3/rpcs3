#include "stdafx.h"
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include "rpcn_client.h"
#include "np_structs_extra.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/vport0.h"
#include "Emu/system_config.h"

#include "util/asm.hpp"

#include "generated/np2_structs_generated.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <errno.h>
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
#pragma GCC diagnostic pop
#endif
#endif

LOG_CHANNEL(rpcn_log, "rpcn");

int get_native_error();

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

	std::string rpcn_state_to_string(rpcn::rpcn_state state)
	{
		return get_localized_string(rpcn_state_to_localized_string_id(state));
	}

	constexpr u32 RPCN_PROTOCOL_VERSION = 19;
	constexpr usz RPCN_HEADER_SIZE      = 15;
	constexpr usz COMMUNICATION_ID_SIZE = 9;

	bool is_error(ErrorType err)
	{
		if (err >= ErrorType::__error_last)
		{
			rpcn_log.error("Invalid error returned!");
			return true;
		}

		switch (err)
		{
		case NoError: return false;
		case Malformed: rpcn_log.error("Sent packet was malformed!"); break;
		case Invalid: rpcn_log.error("Sent command was invalid!"); break;
		case InvalidInput: rpcn_log.error("Sent data was invalid!"); break;
		case TooSoon: rpcn_log.error("Request happened too soon!"); break;
		case LoginError: rpcn_log.error("Unknown login error!"); break;
		case LoginAlreadyLoggedIn: rpcn_log.error("User is already logged in!"); break;
		case LoginInvalidUsername: rpcn_log.error("Login error: invalid username!"); break;
		case LoginInvalidPassword: rpcn_log.error("Login error: invalid password!"); break;
		case LoginInvalidToken: rpcn_log.error("Login error: invalid token!"); break;
		case CreationError: rpcn_log.error("Error creating an account!"); break;
		case CreationExistingUsername: rpcn_log.error("Error creating an account: existing username!"); break;
		case CreationBannedEmailProvider: rpcn_log.error("Error creating an account: banned email provider!"); break;
		case CreationExistingEmail: rpcn_log.error("Error creating an account: an account with that email already exist!"); break;
		case RoomMissing: rpcn_log.error("User tried to join a non-existent room!"); break;
		case RoomAlreadyJoined: rpcn_log.error("User has already joined!"); break;
		case RoomFull: rpcn_log.error("User tried to join a full room!"); break;
		case Unauthorized: rpcn_log.error("User attempted an unauthorized operation!"); break;
		case DbFail: rpcn_log.error("A db query failed on the server!"); break;
		case EmailFail: rpcn_log.error("An email action failed on the server!"); break;
		case NotFound: rpcn_log.error("A request replied not found!"); break;
		case Blocked: rpcn_log.error("You're blocked!"); break;
		case AlreadyFriend: rpcn_log.error("You're already friends!"); break;
		case ScoreNotBest: rpcn_log.error("Attempted to register a score that is not better!"); break;
		case ScoreInvalid: rpcn_log.error("Score for player was found but wasn't what was expected!"); break;
		case ScoreHasData: rpcn_log.error("Score already has game data associated with it!"); break;
		case Unsupported: rpcn_log.error("An unsupported operation was attempted!"); break;
		default: rpcn_log.fatal("Unhandled ErrorType reached the switch?"); break;
		}

		return true;
	}

	// Constructor, destructor & singleton manager

	rpcn_client::rpcn_client()
		: sem_connected(0), sem_authentified(0), sem_reader(0), sem_writer(0), sem_rpcn(0),
		  thread_rpcn(std::thread(&rpcn_client::rpcn_thread, this)), thread_rpcn_reader(std::thread(&rpcn_client::rpcn_reader_thread, this)),
		  thread_rpcn_writer(std::thread(&rpcn_client::rpcn_writer_thread, this))
	{
		g_cfg_rpcn.load();

		sem_rpcn.release();
	}

	rpcn_client::~rpcn_client()
	{
		std::lock_guard lock(inst_mutex);
		terminate = true;
		sem_rpcn.release();
		sem_reader.release();
		sem_writer.release();

		thread_rpcn.join();
		thread_rpcn_reader.join();
		thread_rpcn_writer.join();

		disconnect();

		sem_connected.release();
		sem_authentified.release();
	}

	std::shared_ptr<rpcn_client> rpcn_client::get_instance(bool check_config)
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
			sptr     = std::shared_ptr<rpcn_client>(new rpcn_client());
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
					auto now = steady_clock::now();

					auto rpcn_msgs = get_rpcn_msgs();

					for (const auto& msg : rpcn_msgs)
					{
						if (msg.size() == 6)
						{
							addr_sig = read_from_ptr<le_t<u32>>(&msg[0]);
							port_sig = read_from_ptr<be_t<u16>>(&msg[4]);

							last_pong_time = now;
						}
						else
						{
							rpcn_log.error("Received faulty RPCN UDP message!");
						}
					}

					// Send a packet every 5 seconds and then every 500 ms until reply is received
					if (now - last_pong_time >= 5s && now - last_ping_time > 500ms)
					{
						std::vector<u8> ping(13);
						ping[0]                               = 1;
						write_to_ptr<le_t<s64>>(ping, 1, user_id);
						write_to_ptr<be_t<u32>>(ping, 9, +local_addr_sig);
						if (send_packet_from_p2p_port(ping, addr_rpcn_udp) == -1)
						{
							rpcn_log.error("Failed to send ping to RPCN!");
						}
						last_ping_time = now;
					}
					else
					{
						std::chrono::nanoseconds duration;
						if ((now - last_pong_time) < 5s)
						{
							duration = 5s - (now - last_pong_time);
						}
						else
						{
							duration = 500ms - (now - last_ping_time);
						}

						if (!sem_rpcn.try_acquire_for(duration))
						{
							// TODO
						}
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
		case recvn_result::recvn_terminate: return error_and_disconnect("Recvn was forcefully aborted");
		}

		const u8 packet_type  = header[0];
		const u16 command     = read_from_ptr<le_t<u16>>(&header[1]);
		const u32 packet_size = read_from_ptr<le_t<u32>>(&header[3]);
		const u64 packet_id   = read_from_ptr<le_t<u64>>(&header[7]);

		if (packet_size < RPCN_HEADER_SIZE)
			return error_and_disconnect("Invalid packet size");

		std::vector<u8> data;
		if (packet_size > RPCN_HEADER_SIZE)
		{
			const u16 data_size = packet_size - RPCN_HEADER_SIZE;
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

			// Those commands are handled synchronously and won't be forwarded to NP Handler
			if (command == CommandType::Login || command == CommandType::GetServerList || command == CommandType::Create ||
				command == CommandType::AddFriend || command == CommandType::RemoveFriend ||
				command == CommandType::AddBlock || command == CommandType::RemoveBlock ||
				command == CommandType::SendMessage || command == CommandType::SendToken ||
				command == CommandType::SendResetToken || command == CommandType::ResetPassword)
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
			switch (command)
			{
			case NotificationType::FriendNew:
			case NotificationType::FriendLost:
			case NotificationType::FriendQuery:
			case NotificationType::FriendStatus:
			{
				handle_friend_notification(command, std::move(data));
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
				notifications.emplace_back(std::make_pair(command, std::move(data)));
				break;
			}
			}
			break;
		}
		case PacketType::ServerInfo:
		{
			if (data.size() != 4)
				return error_and_disconnect("Invalid size of ServerInfo packet");

			received_version     = reinterpret_cast<le_t<u32>&>(data[0]);
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

			int res = wolfSSL_read(read_wssl, reinterpret_cast<char*>(buf) + n_recv, n - n_recv);
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
						rpcn_log.error("recv failed: connection reset by server");
						return recvn_result::recvn_noconn;
					}

					rpcn_log.error("recvn failed with error: %d:%s(native: %d)", res, get_wolfssl_error(read_wssl, res), get_native_error());
					return recvn_result::recvn_fatal;
				}

				res = 0;
			}
			else
			{
				// Reset timeout each time something is received
				num_timeouts = 0;
			}

			n_recv += res;
		}

		return recvn_result::recvn_success;
	}

	bool rpcn_client::send_packet(const std::vector<u8>& packet)
	{
		u32 num_timeouts = 0;
		usz n_sent       = 0;
		while (n_sent != packet.size())
		{
			if (terminate)
				return error_and_disconnect("send_packet was forcefully aborted");

			if (!connected)
				return false;

			int res = wolfSSL_write(write_wssl, reinterpret_cast<const char*>(packet.data() + n_sent), packet.size() - n_sent);
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

	bool rpcn_client::forge_send(u16 command, u64 packet_id, const std::vector<u8>& data)
	{
		// TODO: add a check for status?

		std::vector<u8> sent_packet = forge_request(command, packet_id, data);
		add_packet(std::move(sent_packet));
		return true;
	}

	bool rpcn_client::forge_send_reply(u16 command, u64 packet_id, const std::vector<u8>& data, std::vector<u8>& reply_data)
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
			::closesocket(sockfd);
#else
			::close(sockfd);
#endif
			sockfd = 0;
		}

		connected            = false;
		authentified         = false;
		server_info_received = false;
	}

	bool rpcn_client::connect(const std::string& host)
	{
		rpcn_log.warning("connect: Attempting to connect");

		state = rpcn_state::failure_no_failure;

		if (host.empty())
		{
			rpcn_log.error("connect: RPCN host is empty!");
			state = rpcn_state::failure_input;
			return false;
		}

		auto splithost = fmt::split(host, {":"});
		if (splithost.size() != 1 && splithost.size() != 2)
		{
			rpcn_log.error("connect: RPCN host is invalid!");
			state = rpcn_state::failure_input;
			return false;
		}

		u16 port = 31313;

		if (splithost.size() == 2)
		{
			port = std::stoul(splithost[1]);
			if (port == 0)
			{
				rpcn_log.error("connect: RPCN port is invalid!");
				state = rpcn_state::failure_input;
				return false;
			}
		}

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

			addr_rpcn.sin_port   = std::bit_cast<u16, be_t<u16>>(port); // htons
			addr_rpcn.sin_family = AF_INET;

			hostent* host_addr = gethostbyname(splithost[0].c_str());
			if (!host_addr)
			{
				rpcn_log.error("connect: Failed to resolve %s", host);
				state = rpcn_state::failure_resolve;
				return false;
			}

			addr_rpcn.sin_addr.s_addr = *reinterpret_cast<u32*>(host_addr->h_addr_list[0]);

			memcpy(&addr_rpcn_udp, &addr_rpcn, sizeof(addr_rpcn_udp));
			addr_rpcn_udp.sin_port = std::bit_cast<u16, be_t<u16>>(3657); // htons

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

			if (wolfSSL_set_fd(read_wssl, sockfd) != WOLFSSL_SUCCESS)
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

		last_ping_time = steady_clock::now() - 5s;
		last_pong_time = last_ping_time;

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
		auto error  = static_cast<ErrorType>(reply.get<u8>());
		online_name = reply.get_string(false);
		avatar_url  = reply.get_string(false);
		user_id     = reply.get<s64>();

		auto get_usernames_and_status = [](vec_stream& stream, std::map<std::string, std::pair<bool, u64>>& friends)
		{
			u32 num_friends = stream.get<u32>();
			for (u32 i = 0; i < num_friends; i++)
			{
				std::string friend_name = stream.get_string(false);
				bool online             = !!(stream.get<u8>());
				friends.insert(std::make_pair(std::move(friend_name), std::make_pair(online, 0)));
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

		if (is_error(error))
		{
			switch (error)
			{
			case LoginError: state = rpcn_state::failure_id; break;
			case LoginAlreadyLoggedIn: state = rpcn_state::failure_id_already_logged_in; break;
			case LoginInvalidUsername: state = rpcn_state::failure_id_username; break;
			case LoginInvalidPassword: state = rpcn_state::failure_id_password; break;
			case LoginInvalidToken: state = rpcn_state::failure_id_token; break;
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

		if (is_error(error))
		{
			return error;
		}

		rpcn_log.success("You have successfully created a RPCN account(%s | %s)!", npid, online_name);

		return ErrorType::NoError;
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

		if (is_error(error))
		{
			return error;
		}

		rpcn_log.success("Token has successfully been resent!");

		return ErrorType::NoError;
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

		if (is_error(error))
		{
			return error;
		}

		rpcn_log.success("Password reset token has successfully been sent!");

		return ErrorType::NoError;
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

		if (is_error(error))
		{
			return error;
		}

		rpcn_log.success("Password has successfully been reset!");

		return ErrorType::NoError;
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

		if (error == ErrorType::NotFound)
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

		if (is_error(error))
		{
			return false;
		}

		rpcn_log.success("You have successfully removed \"%s\" from your friendlist", friend_username);
		return true;
	}

	std::vector<std::pair<u16, std::vector<u8>>> rpcn_client::get_notifications()
	{
		std::vector<std::pair<u16, std::vector<u8>>> notifs;

		{
			std::lock_guard lock(mutex_notifs);
			notifs = std::move(notifications);
			notifications.clear();
		}

		return notifs;
	}

	std::unordered_map<u32, std::pair<u16, std::vector<u8>>> rpcn_client::get_replies()
	{
		std::unordered_map<u32, std::pair<u16, std::vector<u8>>> ret_replies;

		{
			std::lock_guard lock(mutex_replies);
			ret_replies = std::move(replies);
			replies.clear();
		}

		return ret_replies;
	}

	std::vector<u64> rpcn_client::get_new_messages()
	{
		std::vector<u64> ret_new_messages;
		{
			std::lock_guard lock(mutex_messages);
			ret_new_messages = std::move(new_messages);
			new_messages.clear();
		}

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
		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);

		if (!forge_send_reply(CommandType::GetServerList, req_id, data, reply_data))
		{
			return false;
		}

		vec_stream reply(reply_data);
		auto error = static_cast<ErrorType>(reply.get<u8>());

		if (is_error(error))
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

	bool rpcn_client::get_world_list(u32 req_id, const SceNpCommunicationId& communication_id, u16 server_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u16));
		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u16>&>(data[COMMUNICATION_ID_SIZE]) = server_id;

		return forge_send(CommandType::GetWorldList, req_id, data);
	}

	bool rpcn_client::createjoin_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2CreateJoinRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		if (req->roomBinAttrInternalNum)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomBinAttrInternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrInternal[i].id, builder.CreateVector(req->roomBinAttrInternal[i].ptr.get_ptr(), req->roomBinAttrInternal[i].size));
				davec.push_back(bin);
			}
			final_binattrinternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntAttr>>> final_searchintattrexternal_vec;
		if (req->roomSearchableIntAttrExternalNum)
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
		if (req->roomSearchableBinAttrExternalNum)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomSearchableBinAttrExternal[i].id, builder.CreateVector(req->roomSearchableBinAttrExternal[i].ptr.get_ptr(), req->roomSearchableBinAttrExternal[i].size));
				davec.push_back(bin);
			}
			final_searchbinattrexternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrexternal_vec;
		if (req->roomBinAttrExternalNum)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrExternal[i].id, builder.CreateVector(req->roomBinAttrExternal[i].ptr.get_ptr(), req->roomBinAttrExternal[i].size));
				davec.push_back(bin);
			}
			final_binattrexternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<u8>> final_roompassword;
		if (req->roomPassword)
			final_roompassword = builder.CreateVector(req->roomPassword->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<GroupConfig>>> final_groupconfigs_vec;
		if (req->groupConfigNum)
		{
			std::vector<flatbuffers::Offset<GroupConfig>> davec;
			for (u32 i = 0; i < req->groupConfigNum; i++)
			{
				auto bin = CreateGroupConfig(builder, req->groupConfig[i].slotNum, req->groupConfig[i].withLabel, builder.CreateVector(req->groupConfig[i].label.data, 8), req->groupConfig[i].withPassword);
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
				auto bin = builder.CreateString(req->allowedUser[i].handle.data);
				davec.push_back(bin);
			}
			final_allowedusers_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> final_blockedusers_vec;
		if (req->blockedUserNum)
		{
			std::vector<flatbuffers::Offset<flatbuffers::String>> davec;
			for (u32 i = 0; i < req->blockedUserNum; i++)
			{
				auto bin = builder.CreateString(req->blockedUser[i].handle.data);
				davec.push_back(bin);
			}
			final_blockedusers_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<u8>> final_grouplabel;
		if (req->joinRoomGroupLabel)
			final_grouplabel = builder.CreateVector(req->joinRoomGroupLabel->data, 8);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_memberbinattrinternal_vec;
		if (req->roomMemberBinAttrInternalNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::CreateRoom, req_id, data);
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
		if (req->roomMemberBinAttrInternalNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::JoinRoom, req_id, data);
	}

	bool rpcn_client::leave_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2LeaveRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<PresenceOptionData> final_optdata = CreatePresenceOptionData(builder, builder.CreateVector(req->optData.data, 16), req->optData.length);
		auto req_finished                                     = CreateLeaveRoomRequest(builder, req->roomId, final_optdata);
		builder.Finish(req_finished);
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::LeaveRoom, req_id, data);
	}

	bool rpcn_client::search_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SearchRoomRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntSearchFilter>>> final_intfilter_vec;
		if (req->intFilterNum)
		{
			std::vector<flatbuffers::Offset<IntSearchFilter>> davec{};
			for (u32 i = 0; i < req->intFilterNum; i++)
			{
				auto int_attr = CreateIntAttr(builder, req->intFilter[i].attr.id, req->intFilter[i].attr.num);
				auto bin      = CreateIntSearchFilter(builder, req->intFilter[i].searchOperator, int_attr);
				davec.push_back(bin);
			}
			final_intfilter_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinSearchFilter>>> final_binfilter_vec;
		if (req->binFilterNum)
		{
			std::vector<flatbuffers::Offset<BinSearchFilter>> davec;
			for (u32 i = 0; i < req->binFilterNum; i++)
			{
				auto bin_attr = CreateBinAttr(builder, req->binFilter[i].attr.id, builder.CreateVector(req->binFilter[i].attr.ptr.get_ptr(), req->binFilter[i].attr.size));
				auto bin      = CreateBinSearchFilter(builder, req->binFilter[i].searchOperator, bin_attr);
				davec.push_back(bin);
			}
			final_binfilter_vec = builder.CreateVector(davec);
		}

		flatbuffers::Offset<flatbuffers::Vector<u16>> attrid_vec;
		if (req->attrIdNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SearchRoom, req_id, data);
	}

	bool rpcn_client::get_roomdata_external_list(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		std::vector<u64> roomIds;
		for (u32 i = 0; i < req->roomIdNum; i++)
		{
			roomIds.push_back(req->roomId[i]);
		}
		std::vector<u16> attrIds;
		for (u32 i = 0; i < req->attrIdNum; i++)
		{
			attrIds.push_back(req->attrId[i]);
		}

		auto req_finished = CreateGetRoomDataExternalListRequestDirect(builder, &roomIds, &attrIds);

		builder.Finish(req_finished);
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetRoomDataExternalList, req_id, data);
	}

	bool rpcn_client::set_roomdata_external(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<IntAttr>>> final_searchintattrexternal_vec;
		if (req->roomSearchableIntAttrExternalNum)
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
		if (req->roomSearchableBinAttrExternalNum)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomSearchableBinAttrExternal[i].id, builder.CreateVector(req->roomSearchableBinAttrExternal[i].ptr.get_ptr(), req->roomSearchableBinAttrExternal[i].size));
				davec.push_back(bin);
			}
			final_searchbinattrexternal_vec = builder.CreateVector(davec);
		}
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrexternal_vec;
		if (req->roomBinAttrExternalNum)
		{
			std::vector<flatbuffers::Offset<BinAttr>> davec;
			for (u32 i = 0; i < req->roomBinAttrExternalNum; i++)
			{
				auto bin = CreateBinAttr(builder, req->roomBinAttrExternal[i].id, builder.CreateVector(req->roomBinAttrExternal[i].ptr.get_ptr(), req->roomBinAttrExternal[i].size));
				davec.push_back(bin);
			}
			final_binattrexternal_vec = builder.CreateVector(davec);
		}
		auto req_finished = CreateSetRoomDataExternalRequest(builder, req->roomId, final_searchintattrexternal_vec, final_searchbinattrexternal_vec, final_binattrexternal_vec);

		builder.Finish(req_finished);
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SetRoomDataExternal, req_id, data);
	}

	bool rpcn_client::get_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		flatbuffers::Offset<flatbuffers::Vector<u16>> final_attr_ids_vec;
		if (req->attrIdNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetRoomDataInternal, req_id, data);
	}

	bool rpcn_client::set_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		if (req->roomBinAttrInternalNum)
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
		if (req->passwordConfigNum)
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
		if (req->ownerPrivilegeRankNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SetRoomDataInternal, req_id, data);
	}

	bool rpcn_client::set_roommemberdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<BinAttr>>> final_binattrinternal_vec;
		if (req->roomMemberBinAttrInternalNum)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SetRoomMemberDataInternal, req_id, data);
	}

	bool rpcn_client::ping_room_owner(u32 req_id, const SceNpCommunicationId& communication_id, u64 room_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u64));

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
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
			for (u32 i = 0; i < req->dst.multicastTarget.memberIdNum; i++)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::SendRoomMessage, req_id, data);
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
		const le_t<u32> size = cookie.size();
		std::copy(reinterpret_cast<const u8*>(&size), reinterpret_cast<const u8*>(&size) + sizeof(le_t<u32>), std::back_inserter(data));
		std::copy(cookie.begin(), cookie.end(), std::back_inserter(data));

		return forge_send(CommandType::RequestTicket, req_id, data);
	}

	bool rpcn_client::sendmessage(const message_data& msg_data, const std::set<std::string>& npids)
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(bufsize + sizeof(u32));

		reinterpret_cast<le_t<u32>&>(data[0]) = static_cast<u32>(bufsize);
		memcpy(data.data() + sizeof(u32), buf, bufsize);

		u64 req_id = rpcn_request_counter.fetch_add(1);

		return forge_send(CommandType::SendMessage, req_id, data);
	}

	bool rpcn_client::get_board_infos(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id)
	{
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32));
		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		write_to_ptr<le_t<u32>>(data, COMMUNICATION_ID_SIZE, board_id);

		return forge_send(CommandType::GetBoardInfos, req_id, data);
	}

	bool rpcn_client::record_score(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, SceNpScorePcId char_id, SceNpScoreValue score, const std::optional<std::string> comment, const std::optional<std::vector<u8>> score_data)
	{
		flatbuffers::FlatBufferBuilder builder(1024);

		auto req_finished = CreateRecordScoreRequestDirect(builder, board_id, char_id, score, comment ? (*comment).c_str() : nullptr, score_data ? &*score_data : nullptr);

		builder.Finish(req_finished);
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::RecordScore, req_id, data);
	}

	bool rpcn_client::get_score_range(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, u32 start_rank, u32 num_rank, bool with_comment, bool with_gameinfo)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateGetScoreRangeRequest(builder, board_id, start_rank, num_rank, with_comment, with_gameinfo);

		builder.Finish(req_finished);
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetScoreRange, req_id, data);
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
		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetScoreNpid, req_id, data);
	}

	bool rpcn_client::get_score_friend(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScoreBoardId board_id, bool include_self, bool with_comment, bool with_gameinfo, u32 max_entries)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateGetScoreFriendsRequest(builder, board_id, include_self, max_entries, with_comment, with_gameinfo);
		builder.Finish(req_finished);

		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetScoreFriends, req_id, data);
	}

	bool rpcn_client::record_score_data(u32 req_id, const SceNpCommunicationId& communication_id, SceNpScorePcId pc_id, SceNpScoreBoardId board_id, s64 score, const std::vector<u8>& score_data)
	{
		flatbuffers::FlatBufferBuilder builder(1024);
		auto req_finished = CreateRecordScoreGameDataRequest(builder, board_id, pc_id, score);
		builder.Finish(req_finished);

		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize + sizeof(u32) + score_data.size());

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
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

		u8* buf     = builder.GetBufferPointer();
		usz bufsize = builder.GetSize();
		std::vector<u8> data(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

		memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
		reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
		memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

		return forge_send(CommandType::GetScoreData, req_id, data);
	}

	std::vector<u8> rpcn_client::forge_request(u16 command, u64 packet_id, const std::vector<u8>& data) const
	{
		u32 packet_size = data.size() + RPCN_HEADER_SIZE;

		std::vector<u8> packet(packet_size);
		packet[0]                               = PacketType::Request;
		reinterpret_cast<le_t<u16>&>(packet[1]) = command;
		reinterpret_cast<le_t<u32>&>(packet[3]) = packet_size;
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

	void rpcn_client::get_friends_and_register_cb(friend_data& friend_infos, friend_cb_func cb_func, void* cb_param)
	{
		std::lock_guard lock(mutex_friends);

		friend_infos = this->friend_infos;
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

	void rpcn_client::handle_friend_notification(u16 command, std::vector<u8> data)
	{
		std::lock_guard lock(mutex_friends);

		NotificationType ntype = static_cast<NotificationType>(command);
		vec_stream vdata(data);

		auto call_callbacks = [&](NotificationType ntype, std::string username, bool status)
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
			std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendQuery notification");
				break;
			}

			friend_infos.requests_received.insert(username);
			call_callbacks(ntype, username, 0);
			break;
		}
		case NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
		{
			bool status          = !!vdata.get<u8>();
			std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendNew notification");
				break;
			}

			friend_infos.requests_received.erase(username);
			friend_infos.requests_sent.erase(username);
			friend_infos.friends.insert(std::make_pair(username, std::make_pair(status, 0)));
			call_callbacks(ntype, username, status);

			break;
		}
		case NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
		{
			std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendLost notification");
				break;
			}

			friend_infos.friends.erase(username);
			call_callbacks(ntype, username, 0);
			break;
		}
		case NotificationType::FriendStatus: // Set status of friend to Offline or Online
		{
			bool status          = !!vdata.get<u8>();
			u64 timestamp        = vdata.get<u64>();
			std::string username = vdata.get_string(false);
			if (vdata.is_error())
			{
				rpcn_log.error("Error parsing FriendStatus notification");
				break;
			}

			if (auto u = friend_infos.friends.find(username); u != friend_infos.friends.end())
			{
				if (timestamp < u->second.second)
				{
					break;
				}

				u->second.second = timestamp;
				u->second.first  = status;
				call_callbacks(ntype, username, status);
			}
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
		auto* fb_mdata     = sdata.get_flatbuffer<MessageDetails>();

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
			.msgId       = message_counter,
			.mainType    = fb_mdata->mainType(),
			.subType     = fb_mdata->subType(),
			.msgFeatures = fb_mdata->msgFeatures(),
			.subject     = fb_mdata->subject()->str(),
			.body        = fb_mdata->body()->str()};

		strcpy_trunc(mdata.commId.data, fb_mdata->communicationId()->str());
		mdata.data.assign(fb_mdata->data()->Data(), fb_mdata->data()->Data() + fb_mdata->data()->size());

		// Save the message and call callbacks
		{
			std::lock_guard lock(mutex_messages);
			const u64 msg_id = message_counter++;
			auto id_and_msg  = std::make_shared<std::pair<std::string, message_data>>(std::make_pair(std::move(sender), std::move(mdata)));
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

	std::optional<std::shared_ptr<std::pair<std::string, message_data>>> rpcn_client::get_message(u64 id)
	{
		{
			std::lock_guard lock(mutex_messages);

			if (!messages.contains(id))
				return std::nullopt;

			return ::at32(messages, id);
		}
	}

	std::vector<std::pair<u64, std::shared_ptr<std::pair<std::string, message_data>>>> rpcn_client::get_messages_and_register_cb(SceNpBasicMessageMainType type_filter, bool include_bootable, message_cb_func cb_func, void* cb_param)
	{
		std::vector<std::pair<u64, std::shared_ptr<std::pair<std::string, message_data>>>> vec_messages;
		{
			std::lock_guard lock(mutex_messages);
			for (auto id : active_messages)
			{
				const auto& entry = ::at32(messages, id);
				const auto& msg   = entry->second;
				if (msg.mainType == type_filter && (include_bootable || !(msg.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE)))
				{
					vec_messages.push_back(std::make_pair(id, entry));
				}
			}

			message_cbs.insert(message_cb_t{
				.cb_func      = cb_func,
				.cb_param     = cb_param,
				.type_filter  = type_filter,
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

		return friend_infos.friends.size();
	}

	u32 rpcn_client::get_num_blocks()
	{
		std::lock_guard lock(mutex_friends);

		return friend_infos.blocked.size();
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

} // namespace rpcn
