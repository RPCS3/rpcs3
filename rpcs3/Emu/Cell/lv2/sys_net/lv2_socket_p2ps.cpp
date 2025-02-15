#include "stdafx.h"

#include "Utilities/Thread.h"
#include "util/atomic.hpp"
#include "lv2_socket_p2ps.h"
#include "Emu/NP/np_helpers.h"
#include "nt_p2p_port.h"
#include "network_context.h"
#include "sys_net_helpers.h"

LOG_CHANNEL(sys_net);

// Object in charge of retransmiting packets for STREAM_P2P sockets
class tcp_timeout_monitor
{
public:
	void add_message(s32 sock_id, const sockaddr_in* dst, std::vector<u8> data, u64 seq)
	{
		{
			std::lock_guard lock(data_mutex);

			const auto now = steady_clock::now();

			message msg;
			msg.dst_addr         = *dst;
			msg.sock_id          = sock_id;
			msg.data             = std::move(data);
			msg.seq              = seq;
			msg.initial_sendtime = now;

			rtt_info rtt = rtts[sock_id];

			const auto expected_time = now + rtt.rtt_time;

			msgs.insert(std::make_pair(expected_time, std::move(msg)));
		}
		wakey.release(1);
		wakey.notify_one(); // TODO: Should be improved to only wake if new timeout < old timeout
	}

	void confirm_data_received(s32 sock_id, u64 ack)
	{
		std::lock_guard lock(data_mutex);
		rtts[sock_id].num_retries = 0;

		const auto now = steady_clock::now();

		for (auto it = msgs.begin(); it != msgs.end();)
		{
			auto& msg = it->second;
			if (msg.sock_id == sock_id && msg.seq < ack)
			{
				// Decreases RTT if msg is early
				if (now < it->first)
				{
					const auto actual_rtt = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.initial_sendtime);
					const auto cur_rtt    = rtts[sock_id].rtt_time;
					if (cur_rtt > actual_rtt)
					{
						rtts[sock_id].rtt_time = (actual_rtt + cur_rtt) / 2;
					}
				}
				it = msgs.erase(it);
				continue;
			}
			it++;
		}
	}

	void clear_all_messages(s32 sock_id)
	{
		std::lock_guard lock(data_mutex);

		for (auto it = msgs.begin(); it != msgs.end();)
		{
			auto& msg = it->second;

			if (msg.sock_id == sock_id)
			{
				it = msgs.erase(it);
				continue;
			}
			it++;
		}
	}

	void operator()()
	{
		atomic_wait_timeout timeout = atomic_wait_timeout::inf;

		while (thread_ctrl::state() != thread_state::aborting)
		{
			if (!wakey)
			{
				wakey.wait(0, timeout);
			}

			wakey = 0;

			if (thread_ctrl::state() == thread_state::aborting)
				return;

			std::lock_guard lock(data_mutex);

			const auto now = steady_clock::now();
			// Check for messages that haven't been acked
			std::set<s32> rtt_increased;
			for (auto it = msgs.begin(); it != msgs.end();)
			{
				if (it->first > now)
					break;

				// reply is late, increases rtt
				auto& msg       = it->second;
				const auto addr = msg.dst_addr.sin_addr.s_addr;
				rtt_info rtt    = rtts[msg.sock_id];
				// Only increases rtt once per loop(in case a big number of packets are sent at once)
				if (!rtt_increased.count(msg.sock_id))
				{
					rtt.num_retries += 1;
					// Increases current rtt by 10%
					rtt.rtt_time += (rtt.rtt_time / 10);
					rtts[addr] = rtt;

					rtt_increased.emplace(msg.sock_id);
				}

				if (rtt.num_retries >= 10)
				{
					// Too many retries, need to notify the socket that the connection is dead
					idm::check<lv2_socket>(msg.sock_id, [&](lv2_socket& sock)
						{
							sys_net.error("[P2PS] Too many retries, closing the stream");
							ensure(sock.get_type() == SYS_NET_SOCK_STREAM_P2P);
							auto& sock_p2ps = reinterpret_cast<lv2_socket_p2ps&>(sock);
							sock_p2ps.close_stream();
						});
					it = msgs.erase(it);
					continue;
				}

				// resend the message
				const auto res = idm::check<lv2_socket>(msg.sock_id, [&](lv2_socket& sock) -> bool
					{
						ensure(sock.get_type() == SYS_NET_SOCK_STREAM_P2P);
						auto& sock_p2ps = reinterpret_cast<lv2_socket_p2ps&>(sock);

						while (np::sendto_possibly_ipv6(sock_p2ps.get_socket(), reinterpret_cast<const char*>(msg.data.data()), ::size32(msg.data), &msg.dst_addr, 0) == -1)
						{
							const sys_net_error err = get_last_error(false);
							// concurrency on the socket(from a sendto for example) can result in EAGAIN error in which case we try again
							if (err == SYS_NET_EAGAIN)
							{
								continue;
							}

							sys_net.error("[P2PS] Resending the packet failed(%s), closing the stream", err);
							sock_p2ps.close_stream();
							return false;
						}
						return true;
					});

				if (!res || !res.ret)
				{
					it = msgs.erase(it);
					continue;
				}

				// Update key timeout
				msgs.insert(std::make_pair(now + rtt.rtt_time, std::move(msg)));
				it = msgs.erase(it);
			}

			if (!msgs.empty())
			{
				const auto current_timepoint = steady_clock::now();
				const auto expected_timepoint = msgs.begin()->first;
				if (current_timepoint > expected_timepoint)
				{
					wakey = 1;
				}
				else
				{
					timeout = static_cast<atomic_wait_timeout>(std::chrono::duration_cast<std::chrono::nanoseconds>(expected_timepoint - current_timepoint).count());
				}
			}
			else
			{
				timeout = atomic_wait_timeout::inf;
			}
		}
	}

	tcp_timeout_monitor& operator=(thread_state)
	{
		wakey.release(1);
		wakey.notify_one();
		return *this;
	}

public:
	static constexpr auto thread_name = "Tcp Over Udp Timeout Manager Thread"sv;

private:
	atomic_t<u32> wakey = 0;
	shared_mutex data_mutex;
	// List of outgoing messages
	struct message
	{
		s32 sock_id = 0;
		::sockaddr_in dst_addr{};
		std::vector<u8> data;
		u64 seq = 0;
		steady_clock::time_point initial_sendtime{};
	};
	std::map<steady_clock::time_point, message> msgs; // (wakeup time, msg)
	// List of rtts
	struct rtt_info
	{
		unsigned long num_retries          = 0;
		std::chrono::milliseconds rtt_time = 50ms;
	};
	std::unordered_map<s32, rtt_info> rtts; // (sock_id, rtt)
};

u16 u2s_tcp_checksum(const le_t<u16>* buffer, usz size)
{
	u32 cksum = 0;
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(u16);
	}
	if (size)
		cksum += *reinterpret_cast<const u8*>(buffer);

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return static_cast<u16>(~cksum);
}

std::vector<u8> generate_u2s_packet(const p2ps_encapsulated_tcp& header, const u8* data, const u32 datasize)
{
	const u32 packet_size = (VPORT_P2P_HEADER_SIZE + sizeof(p2ps_encapsulated_tcp) + datasize);
	ensure(packet_size < 65535); // packet size shouldn't be bigger than possible UDP payload
	std::vector<u8> packet(packet_size);
	u8* packet_data        = packet.data();
	le_t<u16> dst_port_le  = +header.dst_port;
	le_t<u16> src_port_le  = +header.src_port;
	le_t<u16> p2p_flags_le = P2P_FLAG_P2PS;

	memcpy(packet_data, &dst_port_le, sizeof(u16));
	memcpy(packet_data + sizeof(u16), &src_port_le, sizeof(u16));
	memcpy(packet_data + sizeof(u16) + sizeof(u16), &p2p_flags_le, sizeof(u16));
	memcpy(packet_data + VPORT_P2P_HEADER_SIZE, &header, sizeof(p2ps_encapsulated_tcp));
	if (datasize)
		memcpy(packet_data + VPORT_P2P_HEADER_SIZE + sizeof(p2ps_encapsulated_tcp), data, datasize);

	auto* hdr_ptr     = reinterpret_cast<p2ps_encapsulated_tcp*>(packet_data + VPORT_P2P_HEADER_SIZE);
	hdr_ptr->checksum = 0;
	hdr_ptr->checksum = u2s_tcp_checksum(utils::bless<le_t<u16>>(hdr_ptr), sizeof(p2ps_encapsulated_tcp) + datasize);

	return packet;
}

lv2_socket_p2ps::lv2_socket_p2ps(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
	: lv2_socket_p2p(family, type, protocol)
{
	sockopt_cache cache_type;
	cache_type.data._int = SYS_NET_SOCK_STREAM_P2P;
	cache_type.len = 4;

	sockopts[(static_cast<u64>(SYS_NET_SOL_SOCKET) << 32ull) | SYS_NET_SO_TYPE] = cache_type;
}

lv2_socket_p2ps::lv2_socket_p2ps(socket_type native_socket, u16 port, u16 vport, u32 op_addr, u16 op_port, u16 op_vport, u64 cur_seq, u64 data_beg_seq, s32 so_nbio)
	: lv2_socket_p2p(SYS_NET_AF_INET, SYS_NET_SOCK_STREAM_P2P, SYS_NET_IPPROTO_IP)
{
	this->native_socket = native_socket;
	this->port         = port;
	this->vport        = vport;
	this->op_addr      = op_addr;
	this->op_port      = op_port;
	this->op_vport     = op_vport;
	this->cur_seq      = cur_seq;
	this->data_beg_seq = data_beg_seq;
	this->so_nbio      = so_nbio;
	status             = p2ps_stream_status::stream_connected;
}

lv2_socket_p2ps::lv2_socket_p2ps(utils::serial& ar, lv2_socket_type type)
	: lv2_socket_p2p(ar, type)
{
	ar(status, max_backlog, backlog, op_port, op_vport, op_addr, data_beg_seq, received_data, cur_seq);
}

void lv2_socket_p2ps::save(utils::serial& ar)
{
	static_cast<lv2_socket_p2p*>(this)->save(ar);
	ar(status, max_backlog, backlog, op_port, op_vport, op_addr, data_beg_seq, received_data, cur_seq);
}

bool lv2_socket_p2ps::handle_connected(p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr, nt_p2p_port* p2p_port)
{
	std::lock_guard lock(mutex);

	if (status != p2ps_stream_status::stream_connected && status != p2ps_stream_status::stream_handshaking)
	{
		sys_net.error("[P2PS] lv2_socket_p2ps::handle_connected() called on a non connected/handshaking socket(%d)!", static_cast<u8>(status));
		return false;
	}

	if (tcp_header->flags & static_cast<u8>(p2ps_tcp_flags::ACK))
	{
		auto& tcpm = g_fxo->get<named_thread<tcp_timeout_monitor>>();
		tcpm.confirm_data_received(lv2_id, tcp_header->ack);
	}

	auto send_ack = [&]()
	{
		auto final_ack = data_beg_seq;
		while (received_data.contains(final_ack))
		{
			final_ack += ::at32(received_data, final_ack).size();
		}
		data_available = final_ack - data_beg_seq;

		p2ps_encapsulated_tcp send_hdr;
		send_hdr.src_port = tcp_header->dst_port;
		send_hdr.dst_port = tcp_header->src_port;
		send_hdr.flags    = p2ps_tcp_flags::ACK;
		send_hdr.ack      = final_ack;
		auto packet       = generate_u2s_packet(send_hdr, nullptr, 0);
		sys_net.trace("[P2PS] Sent ack %d", final_ack);
		send_u2s_packet(std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), 0, false);

		// check if polling is happening
		if (data_available && events.test_and_reset(lv2_socket::poll_t::read))
		{
			bs_t<lv2_socket::poll_t> read_event = lv2_socket::poll_t::read;
			for (auto it = queue.begin(); it != queue.end();)
			{
				if (it->second(read_event))
				{
					it = queue.erase(it);
					continue;
				}
				it++;
			}

			if (queue.empty())
			{
				events.store({});
			}
		}
	};

	if (status == p2ps_stream_status::stream_handshaking)
	{
		// Only expect SYN|ACK
		if (tcp_header->flags == (p2ps_tcp_flags::SYN | p2ps_tcp_flags::ACK))
		{
			sys_net.trace("[P2PS] Received SYN|ACK, status is now connected");
			data_beg_seq = tcp_header->seq + 1;
			status       = p2ps_stream_status::stream_connected;
			send_ack();
		}
		else
		{
			sys_net.error("[P2PS] Unexpected U2S TCP flag received with handshaking state: 0x%02X", tcp_header->flags);
		}

		return true;
	}
	else if (status == p2ps_stream_status::stream_connected)
	{
		switch (tcp_header->flags)
		{
		case 0:
		case p2ps_tcp_flags::PSH:
		case p2ps_tcp_flags::ACK:
		case p2ps_tcp_flags::SYN:
		case p2ps_tcp_flags::SYN | p2ps_tcp_flags::ACK:
		{
			if (tcp_header->seq < data_beg_seq)
			{
				// Data has already been processed
				sys_net.trace("[P2PS] Data has already been processed");
				if (tcp_header->flags != p2ps_tcp_flags::ACK)
					send_ack();
				return true;
			}

			if (!received_data.count(tcp_header->seq))
			{
				// New data
				received_data.emplace(tcp_header->seq, std::vector<u8>(data, data + tcp_header->length));
			}
			else
			{
				sys_net.trace("[P2PS] Data was not new!");
			}

			send_ack();
			return true;
		}
		case p2ps_tcp_flags::RST:
		case p2ps_tcp_flags::FIN:
		{
			sys_net.error("[P2PS] Received RST/FIN packet(%d), closing the stream", tcp_header->flags);
			close_stream_nl(p2p_port);
			return false;
		}
		default:
		{
			sys_net.error("[P2PS] Unexpected U2S TCP flag received with connected state: 0x%02X", tcp_header->flags);
			return true;
		}
		}
	}

	return true;
}

bool lv2_socket_p2ps::handle_listening(p2ps_encapsulated_tcp* tcp_header, [[maybe_unused]] u8* data, ::sockaddr_storage* op_addr)
{
	std::lock_guard lock(mutex);

	if (status != p2ps_stream_status::stream_listening)
	{
		sys_net.error("[P2PS] lv2_socket_p2ps::handle_listening() called on a non listening socket(%d)!", static_cast<u8>(status));
		return false;
	}

	// Only valid packet
	if (tcp_header->flags == static_cast<u8>(p2ps_tcp_flags::SYN))
	{
		if (backlog.size() >= max_backlog)
		{
			// Send a RST packet on backlog full
			sys_net.trace("[P2PS] Backlog was full, sent a RST packet");
			p2ps_encapsulated_tcp send_hdr;
			send_hdr.src_port = tcp_header->dst_port;
			send_hdr.dst_port = tcp_header->src_port;
			send_hdr.flags    = p2ps_tcp_flags::RST;
			auto packet       = generate_u2s_packet(send_hdr, nullptr, 0);
			send_u2s_packet(std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), 0, false);
			return true;
		}

		// Yes, new connection and a backlog is available, create a new lv2_socket for it and send SYN|ACK
		// Prepare reply packet
		sys_net.notice("[P2PS] Received connection on listening STREAM-P2P socket!");
		p2ps_encapsulated_tcp send_hdr;
		send_hdr.src_port = tcp_header->dst_port;
		send_hdr.dst_port = tcp_header->src_port;
		send_hdr.flags    = p2ps_tcp_flags::SYN | p2ps_tcp_flags::ACK;
		send_hdr.ack      = tcp_header->seq + 1;
		// Generates random starting SEQ
		send_hdr.seq = rand();

		// Create new socket
		const u32 new_op_addr      = reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_addr.s_addr;
		const u16 new_op_port      = std::bit_cast<u16, be_t<u16>>((reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_port));
		const u16 new_op_vport     = tcp_header->src_port;
		const u64 new_cur_seq      = send_hdr.seq + 1;
		const u64 new_data_beg_seq = send_hdr.ack;
		auto sock_lv2 = make_shared<lv2_socket_p2ps>(native_socket, port, vport, new_op_addr, new_op_port, new_op_vport, new_cur_seq, new_data_beg_seq, so_nbio);
		const s32 new_sock_id      = idm::import_existing<lv2_socket>(sock_lv2);
		sock_lv2->set_lv2_id(new_sock_id);
		const u64 key_connected = (reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_addr.s_addr) | (static_cast<u64>(tcp_header->src_port) << 48) | (static_cast<u64>(tcp_header->dst_port) << 32);

		{
			auto& nc = g_fxo->get<p2p_context>();
			auto& pport = ::at32(nc.list_p2p_ports, port);
			pport.bound_p2p_streams.emplace(key_connected, new_sock_id);
		}

		auto packet = generate_u2s_packet(send_hdr, nullptr, 0);
		{
			std::lock_guard lock(sock_lv2->mutex);
			sock_lv2->send_u2s_packet(std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), send_hdr.seq, true);
		}

		backlog.push_back(new_sock_id);
		if (events.test_and_reset(lv2_socket::poll_t::read))
		{
			bs_t<lv2_socket::poll_t> read_event = lv2_socket::poll_t::read;
			for (auto it = queue.begin(); it != queue.end();)
			{
				if (it->second(read_event))
				{
					it = queue.erase(it);
					continue;
				}
				it++;
			}

			if (queue.empty())
			{
				events.store({});
			}
		}
	}
	else
	{
		sys_net.error("[P2PS] Unexpected U2S TCP flag received on listening socket: 0x%02X", tcp_header->flags);
	}

	// Ignore other packets?

	return true;
}

void lv2_socket_p2ps::send_u2s_packet(std::vector<u8> data, const ::sockaddr_in* dst, u64 seq, bool require_ack)
{
	char ip_str[16];
	inet_ntop(AF_INET, &dst->sin_addr, ip_str, sizeof(ip_str));
	sys_net.trace("[P2PS] Sending U2S packet on socket %d(id:%d): data(%d, seq %d, require_ack %d) to %s:%d", native_socket, lv2_id, data.size(), seq, require_ack, ip_str, std::bit_cast<u16, be_t<u16>>(dst->sin_port));

	while (np::sendto_possibly_ipv6(native_socket, reinterpret_cast<char*>(data.data()), ::size32(data), dst, 0) == -1)
	{
		const sys_net_error err = get_last_error(false);
		// concurrency on the socket can result in EAGAIN error in which case we try again
		if (err == SYS_NET_EAGAIN)
		{
			continue;
		}

		sys_net.error("[P2PS] Attempting to send a u2s packet failed(%s)!", err);
		return;
	}

	// Adds to tcp timeout monitor to resend the message until an ack is received
	if (require_ack)
	{
		auto& tcpm = g_fxo->get<named_thread<tcp_timeout_monitor>>();
		tcpm.add_message(lv2_id, dst, std::move(data), seq);
	}
}

void lv2_socket_p2ps::close_stream_nl(nt_p2p_port* p2p_port)
{
	status = p2ps_stream_status::stream_closed;

	for (auto it = p2p_port->bound_p2p_streams.begin(); it != p2p_port->bound_p2p_streams.end();)
	{
		if (it->second == lv2_id)
		{
			it = p2p_port->bound_p2p_streams.erase(it);
			continue;
		}
		it++;
	}

	auto& tcpm = g_fxo->get<named_thread<tcp_timeout_monitor>>();
	tcpm.clear_all_messages(lv2_id);
}

void lv2_socket_p2ps::close_stream()
{
	auto& nc = g_fxo->get<p2p_context>();

	std::lock_guard lock(nc.list_p2p_ports_mutex);
	auto& p2p_port = ::at32(nc.list_p2p_ports, port);

	std::scoped_lock more_lock(p2p_port.bound_p2p_vports_mutex, mutex);
	close_stream_nl(&p2p_port);
}

p2ps_stream_status lv2_socket_p2ps::get_status() const
{
	return status;
}

void lv2_socket_p2ps::set_status(p2ps_stream_status new_status)
{
	status = new_status;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_p2ps::getpeername()
{
	std::lock_guard lock(mutex);

	if (!op_addr || !op_port || !op_vport)
	{
		return {-SYS_NET_ENOTCONN, {}};
	}

	sys_net_sockaddr res{};
	sys_net_sockaddr_in_p2p* p2p_addr = reinterpret_cast<sys_net_sockaddr_in_p2p*>(&res);

	p2p_addr->sin_len = sizeof(sys_net_sockaddr_in_p2p);
	p2p_addr->sin_family = SYS_NET_AF_INET;
	p2p_addr->sin_addr = std::bit_cast<be_t<u32>, u32>(op_addr);
	p2p_addr->sin_port = op_vport;
	p2p_addr->sin_vport = op_port;

	return {CELL_OK, res};
}

std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> lv2_socket_p2ps::accept(bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	if (backlog.size() == 0)
	{
		if (so_nbio)
		{
			return {true, -SYS_NET_EWOULDBLOCK, {}, {}};
		}

		return {false, {}, {}, {}};
	}

	auto p2ps_client = backlog.front();
	backlog.pop_front();

	sys_net_sockaddr ps3_addr{};
	auto* paddr = reinterpret_cast<sys_net_sockaddr_in_p2p*>(&ps3_addr);

	lv2_socket_p2ps* sock_client = reinterpret_cast<lv2_socket_p2ps*>(idm::check_unlocked<lv2_socket>(p2ps_client));
	{
		std::lock_guard lock(sock_client->mutex);
		paddr->sin_family = SYS_NET_AF_INET;
		paddr->sin_addr   = std::bit_cast<be_t<u32>, u32>(sock_client->op_addr);
		paddr->sin_port   = sock_client->op_vport;
		paddr->sin_vport  = sock_client->op_port;
		paddr->sin_len    = sizeof(sys_net_sockaddr_in_p2p);
	}

	return {true, p2ps_client, {}, ps3_addr};
}

s32 lv2_socket_p2ps::bind(const sys_net_sockaddr& addr)
{
	const auto* psa_in_p2p = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(&addr);

	// For SYS_NET_SOCK_STREAM_P2P sockets, the port is the "fake" tcp port and the vport is the udp port it's bound to
	u16 p2p_port  = psa_in_p2p->sin_vport;
	u16 p2p_vport = psa_in_p2p->sin_port;

	sys_net.notice("[P2PS] Trying to bind %s:%d:%d", np::ip_to_string(std::bit_cast<u32>(psa_in_p2p->sin_addr)), p2p_port, p2p_vport);

	if (p2p_port == 0)
	{
		p2p_port = SCE_NP_PORT;
	}

	if (p2p_port != SCE_NP_PORT)
	{
		sys_net.warning("[P2PS] Attempting to bind a socket to a port != %d", +SCE_NP_PORT);
	}

	socket_type real_socket{};

	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);

		nc.create_p2p_port(p2p_port);
		auto& pport = ::at32(nc.list_p2p_ports, p2p_port);
		real_socket = pport.p2p_socket;

		{
			// Ensures the socket & the bound list are updated at the same time to avoid races
			std::lock_guard vport_lock(pport.bound_p2p_vports_mutex);
			std::lock_guard sock_lock(mutex);

			if (p2p_vport == 0)
			{
				sys_net.warning("[P2PS] vport was unassigned in bind!");
				p2p_vport = pport.get_port();

				while (pport.bound_p2ps_vports.contains(p2p_vport))
				{
					p2p_vport = pport.get_port();
				}

				std::set<s32> bound_ports{lv2_id};
				pport.bound_p2ps_vports.insert(std::make_pair(p2p_vport, std::move(bound_ports)));
			}
			else
			{
				if (pport.bound_p2ps_vports.contains(p2p_vport))
				{
					auto& bound_sockets = ::at32(pport.bound_p2ps_vports, p2p_vport);
					if (!sys_net_helpers::all_reusable(bound_sockets))
					{
						return -SYS_NET_EADDRINUSE;
					}

					bound_sockets.insert(lv2_id);
				}
				else
				{
					std::set<s32> bound_ports{lv2_id};
					pport.bound_p2ps_vports.insert(std::make_pair(p2p_vport, std::move(bound_ports)));
				}
			}

			port       = p2p_port;
			vport      = p2p_vport;
			native_socket = real_socket;
			bound_addr = psa_in_p2p->sin_addr;
		}
	}

	return CELL_OK;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_p2ps::getsockname()
{
	std::lock_guard lock(mutex);

	// Unbound socket
	if (!native_socket)
	{
		return {CELL_OK, {}};
	}

	sys_net_sockaddr sn_addr{};
	sys_net_sockaddr_in_p2p* paddr = reinterpret_cast<sys_net_sockaddr_in_p2p*>(&sn_addr);

	paddr->sin_len    = sizeof(sys_net_sockaddr_in);
	paddr->sin_family = SYS_NET_AF_INET;
	paddr->sin_port   = vport;
	paddr->sin_vport  = port;
	paddr->sin_addr   = bound_addr;

	return {CELL_OK, sn_addr};
}

std::optional<s32> lv2_socket_p2ps::connect(const sys_net_sockaddr& addr)
{
	std::lock_guard lock(mutex);

	if (status != p2ps_stream_status::stream_closed)
	{
		sys_net.error("[P2PS] Called connect on a socket that is not closed!");
		return -SYS_NET_EALREADY;
	}

	p2ps_encapsulated_tcp send_hdr;
	const auto psa_in_p2p = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(&addr);
	auto name             = sys_net_addr_to_native_addr(addr);

	// This is purposefully inverted, not a bug
	const u16 dst_vport = psa_in_p2p->sin_port;
	const u16 dst_port  = psa_in_p2p->sin_vport;

	socket_type real_socket{};

	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);

		nc.create_p2p_port(port);
		auto& pport = ::at32(nc.list_p2p_ports, port);
		real_socket = pport.p2p_socket;

		{
			std::lock_guard lock(pport.bound_p2p_vports_mutex);
			if (vport == 0)
			{
				// Unassigned vport, assigns one
				sys_net.warning("[P2PS] vport was unassigned before connect!");
				vport = pport.get_port();

				while (pport.bound_p2p_vports.count(vport) || pport.bound_p2p_streams.count(static_cast<u64>(vport) << 32))
				{
					vport = pport.get_port();
				}
			}
			const u64 key = name.sin_addr.s_addr | (static_cast<u64>(vport) << 32) | (static_cast<u64>(dst_vport) << 48);
			pport.bound_p2p_streams.emplace(key, lv2_id);
		}
	}

	native_socket = real_socket;

	send_hdr.src_port = vport;
	send_hdr.dst_port = dst_vport;
	send_hdr.flags    = p2ps_tcp_flags::SYN;
	send_hdr.seq      = rand();

	op_addr        = name.sin_addr.s_addr;
	op_port        = dst_port;
	op_vport       = dst_vport;
	cur_seq        = send_hdr.seq + 1;
	data_beg_seq   = 0;
	data_available = 0u;
	received_data.clear();
	status = p2ps_stream_status::stream_handshaking;

	std::vector<u8> packet = generate_u2s_packet(send_hdr, nullptr, 0);
	name.sin_port          = std::bit_cast<u16, be_t<u16>>(dst_port); // not a bug
	send_u2s_packet(std::move(packet), reinterpret_cast<::sockaddr_in*>(&name), send_hdr.seq, true);

	return CELL_OK;
}

s32 lv2_socket_p2ps::listen(s32 backlog)
{
	std::lock_guard lock(mutex);

	status      = p2ps_stream_status::stream_listening;
	max_backlog = backlog;

	return CELL_OK;
}

std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> lv2_socket_p2ps::recvfrom([[maybe_unused]] s32 flags, u32 len, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	if (!data_available)
	{
		if (status == p2ps_stream_status::stream_closed)
		{
			sys_net.error("[P2PS] Called recvfrom on closed socket!");
			return {{0, {}, {}}};
		}

		if (so_nbio || (flags & SYS_NET_MSG_DONTWAIT))
		{
			return {{-SYS_NET_EWOULDBLOCK, {}, {}}};
		}

		return std::nullopt;
	}

	const u32 to_give = static_cast<u32>(std::min<u64>(data_available, len));
	sys_net_sockaddr addr{};
	std::vector<u8> dest_buf(to_give);

	sys_net.trace("[P2PS] STREAM-P2P socket had %u available, given %u", data_available, to_give);

	u32 left_to_give = to_give;
	while (left_to_give)
	{
		auto& cur_data               = received_data.begin()->second;
		auto to_give_for_this_packet = std::min(static_cast<u32>(cur_data.size()), left_to_give);
		memcpy(dest_buf.data() + (to_give - left_to_give), cur_data.data(), to_give_for_this_packet);
		if (cur_data.size() != to_give_for_this_packet)
		{
			auto amount_left = cur_data.size() - to_give_for_this_packet;
			std::vector<u8> new_vec(amount_left);
			memcpy(new_vec.data(), cur_data.data() + to_give_for_this_packet, amount_left);
			auto new_key = (received_data.begin()->first) + to_give_for_this_packet;
			received_data.emplace(new_key, std::move(new_vec));
		}

		received_data.erase(received_data.begin());

		left_to_give -= to_give_for_this_packet;
	}

	data_available -= to_give;
	data_beg_seq += to_give;

	sys_net_sockaddr_in_p2p* addr_p2p = reinterpret_cast<sys_net_sockaddr_in_p2p*>(&addr);
	addr_p2p->sin_family              = AF_INET;
	addr_p2p->sin_addr                = std::bit_cast<be_t<u32>, u32>(op_addr);
	addr_p2p->sin_port                = op_vport;
	addr_p2p->sin_vport               = op_port;
	addr_p2p->sin_len                 = sizeof(sys_net_sockaddr_in_p2p);

	return {{to_give, dest_buf, addr}};
}

std::optional<s32> lv2_socket_p2ps::sendto([[maybe_unused]] s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	if (status == p2ps_stream_status::stream_closed)
	{
		sys_net.error("[P2PS] Called sendto on a closed socket!");
		return -SYS_NET_ECONNRESET;
	}

	constexpr u32 max_data_len = (65535 - (VPORT_P2P_HEADER_SIZE + sizeof(p2ps_encapsulated_tcp)));

	::sockaddr_in name{};
	if (opt_sn_addr)
	{
		name = sys_net_addr_to_native_addr(*opt_sn_addr);
	}

	// Prepare address
	name.sin_family      = AF_INET;
	name.sin_port        = std::bit_cast<u16, be_t<u16>>(op_port);
	name.sin_addr.s_addr = op_addr;
	// Prepares encapsulated tcp
	p2ps_encapsulated_tcp tcp_header;
	tcp_header.src_port = vport;
	tcp_header.dst_port = op_vport;
	// chop it up
	std::vector<std::vector<u8>> stream_packets;
	u32 cur_total_len = ::size32(buf);
	while (cur_total_len > 0)
	{
		u32 cur_data_len = std::min(cur_total_len, max_data_len);

		tcp_header.length = cur_data_len;
		tcp_header.seq    = cur_seq;

		auto packet = generate_u2s_packet(tcp_header, &buf[buf.size() - cur_total_len], cur_data_len);
		send_u2s_packet(std::move(packet), &name, tcp_header.seq, true);

		cur_total_len -= cur_data_len;
		cur_seq += cur_data_len;
	}

	return {::size32(buf)};
}

std::optional<s32> lv2_socket_p2ps::sendmsg([[maybe_unused]] s32 flags, [[maybe_unused]] const sys_net_msghdr& msg, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_p2ps::sendmsg");
	return {};
}

void lv2_socket_p2ps::close()
{
	if (!port || !vport)
	{
		return;
	}

	if (g_fxo->is_init<p2p_context>())
	{
		auto& nc = g_fxo->get<p2p_context>();
		std::lock_guard lock(nc.list_p2p_ports_mutex);
		auto& p2p_port = ::at32(nc.list_p2p_ports, port);
		{
			std::lock_guard lock(p2p_port.bound_p2p_vports_mutex);
			for (auto it = p2p_port.bound_p2p_streams.begin(); it != p2p_port.bound_p2p_streams.end();)
			{
				if (it->second == lv2_id)
				{
					it = p2p_port.bound_p2p_streams.erase(it);
					continue;
				}
				it++;
			}

			if (p2p_port.bound_p2ps_vports.contains(vport))
			{
				auto& bound_ports = ::at32(p2p_port.bound_p2ps_vports, vport);
				bound_ports.erase(lv2_id);

				if (bound_ports.empty())
				{
					p2p_port.bound_p2ps_vports.erase(vport);
				}
			}
		}
	}

	if (const auto tcpm = g_fxo->try_get<named_thread<tcp_timeout_monitor>>())
	{
		tcpm->clear_all_messages(lv2_id);
	}
}

s32 lv2_socket_p2ps::shutdown([[maybe_unused]] s32 how)
{
	sys_net.todo("[P2PS] shutdown");
	return CELL_OK;
}

s32 lv2_socket_p2ps::poll(sys_net_pollfd& sn_pfd, [[maybe_unused]] pollfd& native_pfd)
{
	std::lock_guard lock(mutex);
	sys_net.trace("[P2PS] poll checking for 0x%X", sn_pfd.events);
	if (status == p2ps_stream_status::stream_connected)
	{
		if ((sn_pfd.events & SYS_NET_POLLIN) && data_available)
		{
			sys_net.trace("[P2PS] p2ps has %u bytes available", data_available);
			sn_pfd.revents |= SYS_NET_POLLIN;
		}

		// Data can only be written if the socket is connected
		if (sn_pfd.events & SYS_NET_POLLOUT && status == p2ps_stream_status::stream_connected)
		{
			sn_pfd.revents |= SYS_NET_POLLOUT;
		}

		if (sn_pfd.revents)
		{
			return 1;
		}
	}

	return 0;
}

std::tuple<bool, bool, bool> lv2_socket_p2ps::select(bs_t<lv2_socket::poll_t> selected, [[maybe_unused]] pollfd& native_pfd)
{
	std::lock_guard lock(mutex);

	bool read_set  = false;
	bool write_set = false;

	if (status == p2ps_stream_status::stream_connected)
	{
		if ((selected & lv2_socket::poll_t::read) && data_available)
		{
			sys_net.trace("[P2PS] socket has %d bytes available", data_available);
			read_set = true;
		}

		if (selected & lv2_socket::poll_t::write)
		{
			sys_net.trace("[P2PS] socket is writeable");
			write_set = true;
		}
	}
	else if (status == p2ps_stream_status::stream_listening)
	{
		const auto bsize = backlog.size();
		if ((selected & lv2_socket::poll_t::read) && bsize)
		{
			sys_net.trace("[P2PS] socket has %d clients available", bsize);
			read_set = true;
		}
	}

	return {read_set, write_set, false};
}
