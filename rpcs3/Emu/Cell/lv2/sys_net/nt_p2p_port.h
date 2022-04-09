#pragma once

#include "lv2_socket_p2ps.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
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

struct nt_p2p_port
{
	// Real socket where P2P packets are received/sent
	socket_type p2p_socket = 0;
	u16 port               = 0;

	shared_mutex bound_p2p_vports_mutex;
	// For DGRAM_P2P sockets(vport, sock_id)
	std::map<u16, s32> bound_p2p_vports{};
	// For STREAM_P2P sockets(key, sock_id)
	// key is ( (src_vport) << 48 | (dst_vport) << 32 | addr ) with src_vport and addr being 0 for listening sockets
	std::map<u64, s32> bound_p2p_streams{};

	// Queued messages from RPCN
	shared_mutex s_rpcn_mutex;
	std::vector<std::vector<u8>> rpcn_msgs{};
	// Queued signaling messages
	shared_mutex s_sign_mutex;
	std::vector<std::pair<std::pair<u32, u16>, std::vector<u8>>> sign_msgs{};

	std::array<u8, 65535> p2p_recv_data{};

	nt_p2p_port(u16 port);
	~nt_p2p_port();

	static void dump_packet(p2ps_encapsulated_tcp* tcph);

	bool handle_connected(s32 sock_id, p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr);
	bool handle_listening(s32 sock_id, p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr);
	bool recv_data();
};
