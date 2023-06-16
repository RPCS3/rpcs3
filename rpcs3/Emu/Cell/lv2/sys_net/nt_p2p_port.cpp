#include "stdafx.h"

#include <fcntl.h>

#include "nt_p2p_port.h"
#include "lv2_socket_native.h"
#include "lv2_socket_p2ps.h"
#include "util/asm.hpp"
#include "sys_net_helpers.h"
#include "Emu/NP/signaling_handler.h"
#include "sys_net_helpers.h"
#include "Emu/NP/vport0.h"
#include "Emu/NP/np_handler.h"

LOG_CHANNEL(sys_net);

namespace sys_net_helpers
{
	bool all_reusable(const std::set<s32>& sock_ids)
	{
		for (const s32 sock_id : sock_ids)
		{
			const auto [_, reusable] = idm::check<lv2_socket>(sock_id, [&](lv2_socket& sock) -> bool
				{
					auto [res_reuseaddr, optval_reuseaddr, optlen_reuseaddr] = sock.getsockopt(SYS_NET_SOL_SOCKET, SYS_NET_SO_REUSEADDR, sizeof(s32));
					auto [res_reuseport, optval_reuseport, optlen_reuseport] = sock.getsockopt(SYS_NET_SOL_SOCKET, SYS_NET_SO_REUSEPORT, sizeof(s32));

					const bool reuse_addr = optlen_reuseaddr == 4 && !!optval_reuseaddr._int;
					const bool reuse_port = optlen_reuseport == 4 && !!optval_reuseport._int;

					return (reuse_addr || reuse_port);
				});

			if (!reusable)
			{
				return false;
			}
		}

		return true;
	}
} // namespace sys_net_helpers

nt_p2p_port::nt_p2p_port(u16 port)
	: port(port)
{
	// Creates and bind P2P Socket
	p2p_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

#ifdef _WIN32
	if (p2p_socket == INVALID_SOCKET)
#else
	if (p2p_socket == -1)
#endif
		fmt::throw_exception("Failed to create DGRAM socket for P2P socket: %s!", get_last_error(true));

#ifdef _WIN32
	u_long _true = 1;
	::ioctlsocket(p2p_socket, FIONBIO, &_true);
#else
	::fcntl(p2p_socket, F_SETFL, ::fcntl(p2p_socket, F_GETFL, 0) | O_NONBLOCK);
#endif

	u32 optval = 131072; // value obtained from DECR for a SOCK_DGRAM_P2P socket(should maybe be bigger for actual socket?)
	if (setsockopt(p2p_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&optval), sizeof(optval)) != 0)
		fmt::throw_exception("Error setsockopt SO_RCVBUF on P2P socket: %s", get_last_error(true));

	::sockaddr_in p2p_saddr{};
	p2p_saddr.sin_family      = AF_INET;
	p2p_saddr.sin_port        = std::bit_cast<u16, be_t<u16>>(port); // htons(port);
	p2p_saddr.sin_addr.s_addr = 0;                                   // binds to 0.0.0.0
	const auto ret_bind       = ::bind(p2p_socket, reinterpret_cast<sockaddr*>(&p2p_saddr), sizeof(p2p_saddr));

	if (ret_bind == -1)
		fmt::throw_exception("Failed to bind DGRAM socket to %d for P2P: %s!", port, get_last_error(true));

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	nph.upnp_add_port_mapping(port, "UDP");

	sys_net.notice("P2P port %d was bound!", port);
}

nt_p2p_port::~nt_p2p_port()
{
	if (p2p_socket)
	{
#ifdef _WIN32
		::closesocket(p2p_socket);
#else
		::close(p2p_socket);
#endif
	}

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	nph.upnp_remove_port_mapping(port, "UDP");
}

void nt_p2p_port::dump_packet(p2ps_encapsulated_tcp* tcph)
{
	sys_net.trace("PACKET DUMP:\nsrc_port: %d\ndst_port: %d\nflags: %d\nseq: %d\nack: %d\nlen: %d", tcph->src_port, tcph->dst_port, tcph->flags, tcph->seq, tcph->ack, tcph->length);
}

bool nt_p2p_port::handle_connected(s32 sock_id, p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr)
{
	const auto sock = idm::check<lv2_socket>(sock_id, [&](lv2_socket& sock) -> bool
		{
			ensure(sock.get_type() == SYS_NET_SOCK_STREAM_P2P);
			auto& sock_p2ps = reinterpret_cast<lv2_socket_p2ps&>(sock);

			return sock_p2ps.handle_connected(tcp_header, data, op_addr);
		});

	if (!sock)
	{
		sys_net.error("[P2PS] Couldn't find the socket!");
		return false;
	}

	if (!sock.ret)
	{
		sys_net.error("[P2PS] handle_connected() failed!");
		return false;
	}

	return true;
}

bool nt_p2p_port::handle_listening(s32 sock_id, p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr)
{
	auto sock = idm::get<lv2_socket>(sock_id);
	if (!sock)
		return false;

	auto& sock_p2ps = reinterpret_cast<lv2_socket_p2ps&>(*sock.get());
	return sock_p2ps.handle_listening(tcp_header, data, op_addr);
}

bool nt_p2p_port::recv_data()
{
	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);
	const auto recv_res        = ::recvfrom(p2p_socket, reinterpret_cast<char*>(p2p_recv_data.data()), p2p_recv_data.size(), 0, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

	if (recv_res == -1)
	{
		auto lerr = get_last_error(false);
		if (lerr != SYS_NET_EINPROGRESS && lerr != SYS_NET_EWOULDBLOCK)
			sys_net.error("Error recvfrom on P2P socket: %d", lerr);

		return false;
	}

	if (recv_res < static_cast<s32>(sizeof(u16)))
	{
		sys_net.error("Received badly formed packet on P2P port(no vport)!");
		return true;
	}

	u16 dst_vport = reinterpret_cast<le_t<u16>&>(p2p_recv_data[0]);

	if (dst_vport == 0)
	{
		if (recv_res < VPORT_0_HEADER_SIZE)
		{
			sys_net.error("Bad vport 0 packet(no subset)!");
			return true;
		}

		const u8 subset      = p2p_recv_data[2];
		const auto data_size = recv_res - VPORT_0_HEADER_SIZE;
		std::vector<u8> vport_0_data(p2p_recv_data.data() + VPORT_0_HEADER_SIZE, p2p_recv_data.data() + VPORT_0_HEADER_SIZE + data_size);

		switch (subset)
		{
		case SUBSET_RPCN:
		{
			std::lock_guard lock(s_rpcn_mutex);
			rpcn_msgs.push_back(std::move(vport_0_data));
			return true;
		}
		case SUBSET_SIGNALING:
		{
			signaling_message msg;
			msg.src_addr = reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr;
			msg.src_port = std::bit_cast<u16, be_t<u16>>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
			msg.data     = std::move(vport_0_data);

			{
				std::lock_guard lock(s_sign_mutex);
				sign_msgs.push_back(std::move(msg));
			}

			auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
			sigh.wake_up();
			return true;
		}
		default:
		{
			sys_net.error("Invalid vport 0 subset!");
			return true;
		}
		}
	}

	if (recv_res < VPORT_P2P_HEADER_SIZE)
	{
		return true;
	}

	const u16 vport_flags = *reinterpret_cast<le_t<u16>*>(p2p_recv_data.data() + sizeof(u16));
	std::vector<u8> p2p_data(recv_res - VPORT_P2P_HEADER_SIZE);
	memcpy(p2p_data.data(), p2p_recv_data.data() + VPORT_P2P_HEADER_SIZE, p2p_data.size());

	if (vport_flags & P2P_FLAG_P2P)
	{
		std::lock_guard lock(bound_p2p_vports_mutex);
		if (bound_p2p_vports.contains(dst_vport))
		{
			sys_net_sockaddr_in_p2p p2p_addr{};

			p2p_addr.sin_len    = sizeof(sys_net_sockaddr_in);
			p2p_addr.sin_family = SYS_NET_AF_INET;
			p2p_addr.sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
			p2p_addr.sin_vport  = dst_vport;
			p2p_addr.sin_port   = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);

			auto& bound_sockets = ::at32(bound_p2p_vports, dst_vport);

			for (const auto sock_id : bound_sockets)
			{
				const auto sock = idm::check<lv2_socket>(sock_id, [&](lv2_socket& sock)
					{
						ensure(sock.get_type() == SYS_NET_SOCK_DGRAM_P2P);
						auto& sock_p2p = reinterpret_cast<lv2_socket_p2p&>(sock);

						sock_p2p.handle_new_data(p2p_addr, p2p_data);
					});

				if (!sock)
				{
					sys_net.error("Socket %d found in bound_p2p_vports didn't exist!", sock_id);
					bound_sockets.erase(sock_id);
					if (bound_sockets.empty())
					{
						bound_p2p_vports.erase(dst_vport);
					}
				}
			}

			return true;
		}
	}
	else if (vport_flags & P2P_FLAG_P2PS)
	{
		if (p2p_data.size() < sizeof(p2ps_encapsulated_tcp))
		{
			sys_net.notice("Received P2P packet targeted at unbound vport(likely) or invalid(vport=%d)", dst_vport);
			return true;
		}

		auto* tcp_header = reinterpret_cast<p2ps_encapsulated_tcp*>(p2p_data.data());

		// Validate signature & length
		if (tcp_header->signature != P2PS_U2S_SIG)
		{
			sys_net.notice("Received P2P packet targeted at unbound vport(vport=%d)", dst_vport);
			return true;
		}

		if (tcp_header->length != (p2p_data.size() - sizeof(p2ps_encapsulated_tcp)))
		{
			sys_net.error("Received STREAM-P2P packet tcp length didn't match packet length");
			return true;
		}

		// Sanity check
		if (tcp_header->dst_port != dst_vport)
		{
			sys_net.error("Received STREAM-P2P packet with dst_port != vport");
			return true;
		}

		// Validate checksum
		u16 given_checksum   = tcp_header->checksum;
		tcp_header->checksum = 0;
		if (given_checksum != u2s_tcp_checksum(reinterpret_cast<const le_t<u16>*>(p2p_data.data()), p2p_data.size()))
		{
			sys_net.error("Checksum is invalid, dropping packet!");
			return true;
		}

		// The packet is valid, check if it's bound
		const u64 key_connected = (reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr) | (static_cast<u64>(tcp_header->src_port) << 48) | (static_cast<u64>(tcp_header->dst_port) << 32);

		{
			std::lock_guard lock(bound_p2p_vports_mutex);
			if (bound_p2p_streams.contains(key_connected))
			{
				const auto sock_id = ::at32(bound_p2p_streams, key_connected);
				sys_net.trace("Received packet for connected STREAM-P2P socket(s=%d)", sock_id);
				handle_connected(sock_id, tcp_header, p2p_data.data() + sizeof(p2ps_encapsulated_tcp), &native_addr);
				return true;
			}

			if (bound_p2ps_vports.contains(tcp_header->dst_port))
			{
				const auto& bound_sockets = ::at32(bound_p2ps_vports, tcp_header->dst_port);

				for (const auto sock_id : bound_sockets)
				{
					sys_net.trace("Received packet for listening STREAM-P2P socket(s=%d)", sock_id);
					handle_listening(sock_id, tcp_header, p2p_data.data() + sizeof(p2ps_encapsulated_tcp), &native_addr);
				}
				return true;
			}
		}
	}

	sys_net.notice("Received a STREAM-P2P packet with no bound target");
	return true;
}
