#include "stdafx.h"
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include "rpcn_client.h"
#include "np_structs_extra.h"
#include "Utilities/StrUtil.h"
#include "Utilities/BEType.h"
#include "Utilities/Thread.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"

#include "generated/np2_structs_generated.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
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
#endif

LOG_CHANNEL(rpcn_log, "rpcn");

#define RPCN_PROTOCOL_VERSION 10
#define RPCN_HEADER_SIZE 9
#define COMMUNICATION_ID_SIZE 9

rpcn_client::rpcn_client(bool in_config)
    : in_config(in_config)
{
#ifdef _WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

rpcn_client::~rpcn_client()
{
	disconnect();
}

std::string rpcn_client::get_wolfssl_error(int error)
{
	char error_string[80]{};
	auto wssl_err = wolfSSL_get_error(wssl, error);
	wolfSSL_ERR_error_string(wssl_err, &error_string[0]);
	return std::string(error_string);
}

void rpcn_client::disconnect()
{
	std::lock_guard lock(mutex_socket);

	if (wssl)
	{
		wolfSSL_free(wssl);
		wssl = nullptr;
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

rpcn_client::recvn_result rpcn_client::recvn(u8* buf, std::size_t n)
{
	u32 num_timeouts = 0;

	size_t n_recv = 0;
	while (n_recv != n && !is_abort())
	{
		std::lock_guard lock(mutex_socket);

		if (!connected)
			return recvn_result::recvn_noconn;

		int res = wolfSSL_read(wssl, reinterpret_cast<char*>(buf) + n_recv, n - n_recv);
		if (res <= 0)
		{
			if (wolfSSL_want_read(wssl))
			{
				// If we received partially what we want try to wait longer
				if (n_recv == 0)
					return recvn_result::recvn_nodata;

				num_timeouts++;
				if (num_timeouts >= 1000)
				{
					rpcn_log.error("recvn timeout with %d bytes received", n_recv);
					return recvn_result::recvn_timeout;
				}
			}
			else
			{
				rpcn_log.error("recvn failed with error: %s", get_wolfssl_error(res));
				return recvn_result::recvn_fatal;
			}

			res = 0;
		}
		n_recv += res;
	}

	return recvn_result::recvn_success;
}

bool rpcn_client::send_packet(const std::vector<u8>& packet)
{
	u32 num_timeouts = 0;
	std::size_t n_sent = 0;
	while (n_sent != packet.size())
	{
		std::lock_guard lock(mutex_socket);

		if (!connected)
			return false;

		int res = wolfSSL_write(wssl, reinterpret_cast<const char*>(packet.data()), packet.size());
		if (res <= 0)
		{
			if (wolfSSL_want_write(wssl))
			{
				num_timeouts++;
				if (num_timeouts >= 1000)
				{
					rpcn_log.error("send_packet timeout with %d bytes sent", n_sent);
					return error_and_disconnect("Failed to send all the bytes");
				}
			}
			else
			{
				rpcn_log.error("send_packet failed with error: %s", get_wolfssl_error(res));
				return error_and_disconnect("Failed to send all the bytes");
			}

			res = 0;
		}
		n_sent += res;
	}

	return true;
}

bool rpcn_client::forge_send(u16 command, u32 packet_id, const std::vector<u8>& data)
{
	const auto sent_packet = forge_request(command, packet_id, data);
	if (!send_packet(sent_packet))
		return false;

	return true;
}

bool rpcn_client::forge_send_reply(u16 command, u32 packet_id, const std::vector<u8>& data, std::vector<u8>& reply_data)
{
	if (!forge_send(command, packet_id, data))
		return false;

	if (!get_reply(packet_id, reply_data))
		return false;

	if (is_error(static_cast<ErrorType>(reply_data[0])))
	{
		disconnect();
		return false;
	}

	return true;
}

bool rpcn_client::connect(const std::string& host)
{
	rpcn_log.warning("Attempting to connect to RPCN!");

	// Cleans previous data if any
	disconnect();

	{
		std::lock_guard lock(mutex_socket);

		if (wolfSSL_Init() != WOLFSSL_SUCCESS)
		{
			rpcn_log.fatal("Failed to initialize wolfssl");
			return false;
		}

		if ((wssl_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == nullptr)
		{
			rpcn_log.fatal("Failed to create wolfssl context");
			return false;
		}

		wolfSSL_CTX_set_verify(wssl_ctx, SSL_VERIFY_NONE, 0);

		if ((wssl = wolfSSL_new(wssl_ctx)) == nullptr)
		{
			rpcn_log.fatal("Failed to create wolfssl object");
			return false;
		}

		memset(&addr_rpcn, 0, sizeof(addr_rpcn));

		addr_rpcn.sin_port   = std::bit_cast<u16, be_t<u16>>(31313); // htons
		addr_rpcn.sin_family = AF_INET;
		auto splithost       = fmt::split(host, {":"});

		if (splithost.size() != 1 && splithost.size() != 2)
		{
			rpcn_log.fatal("RPCN host is invalid!");
			return false;
		}

		if (splithost.size() == 2)
			addr_rpcn.sin_port = std::bit_cast<u16, be_t<u16>>(std::stoul(splithost[1])); // htons

		hostent* host_addr = gethostbyname(splithost[0].c_str());
		if (!host_addr)
		{
			rpcn_log.fatal("Failed to resolve %s", splithost[0]);
			return false;
		}

		addr_rpcn.sin_addr.s_addr = *reinterpret_cast<u32*>(host_addr->h_addr_list[0]);

		memcpy(&addr_rpcn_udp, &addr_rpcn, sizeof(addr_rpcn_udp));
		addr_rpcn_udp.sin_port = std::bit_cast<u16, be_t<u16>>(3657); // htons

		sockfd = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
		u32 timeout = 5;
#else
		struct timeval timeout;
		timeout.tv_sec  = 0;
		timeout.tv_usec = 5000;
#endif

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0)
		{
			rpcn_log.fatal("Failed to setsockopt!");
			return false;
		}

		if (::connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr_rpcn), sizeof(addr_rpcn)) != 0)
		{
			rpcn_log.fatal("Failed to connect to RPCN server!");
			return false;
		}

		if (wolfSSL_set_fd(wssl, sockfd) != WOLFSSL_SUCCESS)
		{
			rpcn_log.fatal("Failed to associate wolfssl to the socket");
			return false;
		}

		int ret_connect;
		while((ret_connect = wolfSSL_connect(wssl)) != SSL_SUCCESS)
		{
			if(wolfSSL_want_read(wssl))
				continue;

			rpcn_log.fatal("Handshake failed with RPCN Server: %s", get_wolfssl_error(ret_connect));
			return false;
		}
	}

	connected = true;

	while (!server_info_received && connected && !is_abort())
		std::this_thread::sleep_for(5ms);

	if (received_version != RPCN_PROTOCOL_VERSION)
	{
		rpcn_log.fatal("Server returned protocol version: %d, expected: %d", received_version, RPCN_PROTOCOL_VERSION);
		disconnect();
		return false;
	}

	last_ping_time = std::chrono::system_clock::now() - std::chrono::seconds(5);
	last_pong_time = last_ping_time;

	return true;
}

bool rpcn_client::login(const std::string& npid, const std::string& password, const std::string& token)
{
	std::vector<u8> data{};
	std::copy(npid.begin(), npid.end(), std::back_inserter(data));
	data.push_back(0);
	std::copy(password.begin(), password.end(), std::back_inserter(data));
	data.push_back(0);
	std::copy(token.begin(), token.end(), std::back_inserter(data));
	data.push_back(0);
	const auto sent_packet = forge_request(CommandType::Login, 1, data);
	if (!send_packet(sent_packet))
		return false;

	std::vector<u8> packet_data{};
	if (!get_reply(1, packet_data))
		return false;

	vec_stream reply(packet_data);
	u8 error    = reply.get<u8>();
	online_name = reply.get_string(false);
	avatar_url  = reply.get_string(false);
	user_id     = reply.get<s64>();

	if (is_error(static_cast<ErrorType>(error)))
	{
		disconnect();
		return false;
	}

	if (reply.is_error())
		return error_and_disconnect("Malformed reply to Login command");

	rpcn_log.success("You are now logged in RPCN(%s | %s)!", npid, online_name);
	authentified = true;

	// Make sure signaling works
	if (!in_config)
	{
		auto start = std::chrono::system_clock::now();

		while (!get_addr_sig() && (std::chrono::system_clock::now() - start) < std::chrono::seconds(5))
		{
			std::this_thread::sleep_for(5ms);
		}

		if (!get_addr_sig())
			return error_and_disconnect("Failed to get Signaling going with the server!");
	}

	return true;
}

bool rpcn_client::create_user(const std::string& npid, const std::string& password, const std::string& online_name, const std::string& avatar_url, const std::string& email)
{
	std::vector<u8> data{};
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
	const auto sent_packet = forge_request(CommandType::Create, 1, data);
	if (!send_packet(sent_packet))
		return false;

	std::vector<u8> packet_data{};
	if (!get_reply(1, packet_data))
		return false;

	vec_stream reply(packet_data);
	u8 error = reply.get<u8>();

	if (is_error(static_cast<ErrorType>(error)))
	{
		disconnect();
		return false;
	}

	rpcn_log.success("You have successfully created a RPCN account(%s | %s)!", npid, online_name);

	return true;
}

s32 send_packet_from_p2p_port(const std::vector<u8>& data, const sockaddr_in& addr);
std::vector<std::vector<u8>> get_rpcn_msgs();

bool rpcn_client::manage_connection()
{
	if (!connected)
		return false;

	if (authentified && !in_config)
	{
		// Ping the UDP Signaling Server
		auto now = std::chrono::system_clock::now();

		auto rpcn_msgs = get_rpcn_msgs();

		for (const auto& msg : rpcn_msgs)
		{
			if (msg.size() == 6)
			{
				addr_sig = reinterpret_cast<const le_t<u32>&>(msg[0]);
				port_sig = reinterpret_cast<const be_t<u16>&>(msg[4]);

				in_addr orig{};
				orig.s_addr = addr_sig;

				last_pong_time = now;
			}
			else
			{
				rpcn_log.error("Received faulty RPCN UDP message!");
			}
		}

		// Send a packet every 5 seconds and then every 500 ms until reply is received
		if ((now - last_pong_time) > std::chrono::seconds(5) && (now - last_ping_time) > std::chrono::milliseconds(500))
		{
			std::vector<u8> ping(9);
			ping[0]                                 = 1;
			*reinterpret_cast<le_t<s64>*>(&ping[1]) = user_id;
			if (send_packet_from_p2p_port(ping, addr_rpcn_udp) == -1)
			{
				rpcn_log.error("Failed to send ping to rpcn!");
			}
			last_ping_time = now;
		}
	}

	u8 header[RPCN_HEADER_SIZE];
	auto res_recvn = recvn(header, RPCN_HEADER_SIZE);

	switch (res_recvn)
	{
	case recvn_result::recvn_noconn: return error_and_disconnect("Disconnected");
	case recvn_result::recvn_fatal:
	case recvn_result::recvn_timeout: return error_and_disconnect("Failed to read a packet header on socket");
	case recvn_result::recvn_nodata: return false;
	case recvn_result::recvn_success: break;
	}

	const u8 packet_type  = header[0];
	const u16 command     = reinterpret_cast<le_t<u16>&>(header[1]);
	const u16 packet_size = reinterpret_cast<le_t<u16>&>(header[3]);
	const u32 packet_id   = reinterpret_cast<le_t<u32>&>(header[5]);

	if (packet_size < RPCN_HEADER_SIZE)
		return error_and_disconnect("Invalid packet size");

	std::vector<u8> data{};
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
		if (command == CommandType::Login || command == CommandType::GetServerList || command == CommandType::Create)
		{
			std::lock_guard lock(mutex_replies_sync);
			replies_sync.insert(std::make_pair(packet_id, std::make_pair(command, std::move(data))));
		}
		else
		{
			std::lock_guard lock(mutex_replies);
			replies.insert(std::make_pair(packet_id, std::make_pair(command, std::move(data))));
		}

		break;
	}
	case PacketType::Notification:
	{
		std::lock_guard lock(mutex_notifs);
		notifications.push_back(std::make_pair(command, std::move(data)));
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

bool rpcn_client::get_reply(const u32 expected_id, std::vector<u8>& data)
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

	while (connected && !is_abort())
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
	std::vector<u8> data(COMMUNICATION_ID_SIZE), reply_data{};
	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);

	if (!forge_send_reply(CommandType::GetServerList, req_id, data, reply_data))
		return false;

	vec_stream reply(reply_data, 1);
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

	if (!forge_send(CommandType::GetWorldList, req_id, data))
		return false;

	return true;
}

bool rpcn_client::createjoin_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2CreateJoinRoomRequest* req)
{
	std::vector<u8> data{};

	extra_nps::print_createjoinroom(req);

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
	flatbuffers::Offset<flatbuffers::Vector<uint8_t>> final_roompassword;
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
	flatbuffers::Offset<flatbuffers::Vector<uint8_t>> final_grouplabel;
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
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::CreateRoom, req_id, data))
		return false;

	return true;
}

bool rpcn_client::join_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2JoinRoomRequest* req)
{
	std::vector<u8> data{};

	extra_nps::print_joinroom(req);

	flatbuffers::FlatBufferBuilder builder(1024);

	flatbuffers::Offset<flatbuffers::Vector<uint8_t>> final_roompassword;
	if (req->roomPassword)
		final_roompassword = builder.CreateVector(req->roomPassword->data, 8);
	flatbuffers::Offset<flatbuffers::Vector<uint8_t>> final_grouplabel;
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
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::JoinRoom, req_id, data))
		return false;

	return true;
}

bool rpcn_client::leave_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2LeaveRoomRequest* req)
{
	std::vector<u8> data{};

	flatbuffers::FlatBufferBuilder builder(1024);
	flatbuffers::Offset<PresenceOptionData> final_optdata = CreatePresenceOptionData(builder, builder.CreateVector(req->optData.data, 16), req->optData.length);
	auto req_finished                                     = CreateLeaveRoomRequest(builder, req->roomId, final_optdata);
	builder.Finish(req_finished);
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + sizeof(u32) + bufsize);

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::LeaveRoom, req_id, data))
		return false;

	return true;
}

bool rpcn_client::search_room(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SearchRoomRequest* req)
{
	std::vector<u8> data{};

	extra_nps::print_search_room(req);

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
	flatbuffers::Offset<flatbuffers::Vector<uint16_t>> attrid_vec;
	if (req->attrIdNum)
		attrid_vec = builder.CreateVector(reinterpret_cast<const u16*>(req->attrId.get_ptr()), req->attrIdNum);

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
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::SearchRoom, req_id, data))
		return false;

	return true;
}

bool rpcn_client::set_roomdata_external(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataExternalRequest* req)
{
	std::vector<u8> data{};

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
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::SetRoomDataExternal, req_id, data))
		return false;

	return true;
}

bool rpcn_client::get_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2GetRoomDataInternalRequest* req)
{
	std::vector<u8> data{}, reply_data{};

	flatbuffers::FlatBufferBuilder builder(1024);

	flatbuffers::Offset<flatbuffers::Vector<uint16_t>> final_attr_ids_vec;
	if (req->attrIdNum)
		final_attr_ids_vec = builder.CreateVector(reinterpret_cast<const u16*>(req->attrId.get_ptr()), req->attrIdNum);

	auto req_finished = CreateGetRoomDataInternalRequest(builder, req->roomId, final_attr_ids_vec);

	builder.Finish(req_finished);
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::GetRoomDataInternal, req_id, data))
		return false;

	return true;
}

bool rpcn_client::set_roomdata_internal(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SetRoomDataInternalRequest* req)
{
	std::vector<u8> data{};

	//	extra_nps::print_set_roomdata_req(req);

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

	flatbuffers::Offset<flatbuffers::Vector<uint16_t>> final_ownerprivilege_vec;
	if (req->ownerPrivilegeRankNum)
		final_ownerprivilege_vec = builder.CreateVector(reinterpret_cast<const u16*>(req->ownerPrivilegeRank.get_ptr()), req->ownerPrivilegeRankNum);

	auto req_finished =
	    CreateSetRoomDataInternalRequest(builder, req->roomId, req->flagFilter, req->flagAttr, final_binattrinternal_vec, final_grouppasswordconfig_vec, final_passwordSlotMask, final_ownerprivilege_vec);

	builder.Finish(req_finished);
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::SetRoomDataInternal, req_id, data))
		return false;

	return true;
}

bool rpcn_client::ping_room_owner(u32 req_id, const SceNpCommunicationId& communication_id, u64 room_id)
{
	std::vector<u8> data{};

	data.resize(COMMUNICATION_ID_SIZE + sizeof(u64));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u64>&>(data[COMMUNICATION_ID_SIZE]) = room_id;

	if (!forge_send(CommandType::PingRoomOwner, req_id, data))
		return false;

	return true;
}

bool rpcn_client::send_room_message(u32 req_id, const SceNpCommunicationId& communication_id, const SceNpMatching2SendRoomMessageRequest* req)
{
	std::vector<u8> data{};
	flatbuffers::FlatBufferBuilder builder(1024);

	std::vector<u16> dst;
	switch(req->castType)
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

	auto req_finished = CreateSendRoomMessageRequest(builder, req->roomId, req->castType, builder.CreateVector(dst.data(), dst.size()), builder.CreateVector(reinterpret_cast<const u8 *>(req->msg.get_ptr()), req->msgLen), req->option);

	builder.Finish(req_finished);
	u8* buf        = builder.GetBufferPointer();
	size_t bufsize = builder.GetSize();
	data.resize(COMMUNICATION_ID_SIZE + bufsize + sizeof(u32));

	memcpy(data.data(), communication_id.data, COMMUNICATION_ID_SIZE);
	reinterpret_cast<le_t<u32>&>(data[COMMUNICATION_ID_SIZE]) = static_cast<u32>(bufsize);
	memcpy(data.data() + COMMUNICATION_ID_SIZE + sizeof(u32), buf, bufsize);

	if (!forge_send(CommandType::SendRoomMessage, req_id, data))
		return false;

	return true;
}

bool rpcn_client::req_sign_infos(u32 req_id, const std::string& npid)
{
	std::vector<u8> data{};
	std::copy(npid.begin(), npid.end(), std::back_inserter(data));
	data.push_back(0);

	if (!forge_send(CommandType::RequestSignalingInfos, req_id, data))
		return false;

	return true;
}

bool rpcn_client::req_ticket(u32 req_id, const std::string& service_id)
{
	std::vector<u8> data{};
	std::copy(service_id.begin(), service_id.end(), std::back_inserter(data));
	data.push_back(0);

	if (!forge_send(CommandType::RequestTicket, req_id, data))
		return false;

	return true;
}

std::vector<u8> rpcn_client::forge_request(u16 command, u32 packet_id, const std::vector<u8>& data) const
{
	u16 packet_size = data.size() + RPCN_HEADER_SIZE;

	std::vector<u8> packet(packet_size);
	packet[0]                               = PacketType::Request;
	reinterpret_cast<le_t<u16>&>(packet[1]) = command;
	reinterpret_cast<le_t<u16>&>(packet[3]) = packet_size;
	reinterpret_cast<le_t<u32>&>(packet[5]) = packet_id;

	memcpy(packet.data() + RPCN_HEADER_SIZE, data.data(), data.size());
	return packet;
}

bool rpcn_client::is_error(ErrorType err) const
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
	case ErrorLogin: rpcn_log.error("Sent password/token was incorrect!"); break;
	case ErrorCreate: rpcn_log.error("Error creating an account!"); break;
	case AlreadyLoggedIn: rpcn_log.error("User is already logged in!"); break;
	case DbFail: rpcn_log.error("A db query failed on the server!"); break;
	case NotFound: rpcn_log.error("A request replied not found!"); return false;
	case Unsupported: rpcn_log.error("An unsupported operation was attempted!"); return false;
	default: rpcn_log.fatal("Unhandled ErrorType reached the switch?"); break;
	}

	return true;
}

bool rpcn_client::error_and_disconnect(const std::string& error_msg)
{
	rpcn_log.error("%s", error_msg);
	disconnect();
	return false;
}

bool rpcn_client::is_abort()
{
	if (in_config)
	{
		if (abort_config)
			return true;
	}
	else
	{
		if (Emu.IsStopped() || thread_ctrl::state() == thread_state::aborting)
			return true;
	}

	return false;
}

void rpcn_client::abort()
{
	ensure(in_config);
	abort_config = true;
}
