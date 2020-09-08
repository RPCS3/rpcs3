#include "stdafx.h"
#include "sys_net.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Utilities/Thread.h"

#include "sys_sync.h"

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
#endif

#include "Emu/NP/np_handler.h"

#include <limits>
#include <chrono>

LOG_CHANNEL(sys_net);

template<>
void fmt_class_string<sys_net_error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (s32 _error = error)
		{
#define SYS_NET_ERROR_CASE(x) case -x: return "-" #x; case x: return #x
		SYS_NET_ERROR_CASE(SYS_NET_ENOENT);
		SYS_NET_ERROR_CASE(SYS_NET_EINTR);
		SYS_NET_ERROR_CASE(SYS_NET_EBADF);
		SYS_NET_ERROR_CASE(SYS_NET_ENOMEM);
		SYS_NET_ERROR_CASE(SYS_NET_EACCES);
		SYS_NET_ERROR_CASE(SYS_NET_EFAULT);
		SYS_NET_ERROR_CASE(SYS_NET_EBUSY);
		SYS_NET_ERROR_CASE(SYS_NET_EINVAL);
		SYS_NET_ERROR_CASE(SYS_NET_EMFILE);
		SYS_NET_ERROR_CASE(SYS_NET_ENOSPC);
		SYS_NET_ERROR_CASE(SYS_NET_EPIPE);
		SYS_NET_ERROR_CASE(SYS_NET_EAGAIN);
			static_assert(SYS_NET_EWOULDBLOCK == SYS_NET_EAGAIN);
		SYS_NET_ERROR_CASE(SYS_NET_EINPROGRESS);
		SYS_NET_ERROR_CASE(SYS_NET_EALREADY);
		SYS_NET_ERROR_CASE(SYS_NET_EDESTADDRREQ);
		SYS_NET_ERROR_CASE(SYS_NET_EMSGSIZE);
		SYS_NET_ERROR_CASE(SYS_NET_EPROTOTYPE);
		SYS_NET_ERROR_CASE(SYS_NET_ENOPROTOOPT);
		SYS_NET_ERROR_CASE(SYS_NET_EPROTONOSUPPORT);
		SYS_NET_ERROR_CASE(SYS_NET_EOPNOTSUPP);
		SYS_NET_ERROR_CASE(SYS_NET_EPFNOSUPPORT);
		SYS_NET_ERROR_CASE(SYS_NET_EAFNOSUPPORT);
		SYS_NET_ERROR_CASE(SYS_NET_EADDRINUSE);
		SYS_NET_ERROR_CASE(SYS_NET_EADDRNOTAVAIL);
		SYS_NET_ERROR_CASE(SYS_NET_ENETDOWN);
		SYS_NET_ERROR_CASE(SYS_NET_ENETUNREACH);
		SYS_NET_ERROR_CASE(SYS_NET_ECONNABORTED);
		SYS_NET_ERROR_CASE(SYS_NET_ECONNRESET);
		SYS_NET_ERROR_CASE(SYS_NET_ENOBUFS);
		SYS_NET_ERROR_CASE(SYS_NET_EISCONN);
		SYS_NET_ERROR_CASE(SYS_NET_ENOTCONN);
		SYS_NET_ERROR_CASE(SYS_NET_ESHUTDOWN);
		SYS_NET_ERROR_CASE(SYS_NET_ETOOMANYREFS);
		SYS_NET_ERROR_CASE(SYS_NET_ETIMEDOUT);
		SYS_NET_ERROR_CASE(SYS_NET_ECONNREFUSED);
		SYS_NET_ERROR_CASE(SYS_NET_EHOSTDOWN);
		SYS_NET_ERROR_CASE(SYS_NET_EHOSTUNREACH);
#undef SYS_NET_ERROR_CASE
		}

		return unknown;
	});
}

template <>
void fmt_class_string<lv2_socket_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case SYS_NET_SOCK_STREAM: return "STREAM";
		case SYS_NET_SOCK_DGRAM: return "DGRAM";
		case SYS_NET_SOCK_RAW: return "RAW";
		case SYS_NET_SOCK_DGRAM_P2P: return "DGRAM-P2P";
		case SYS_NET_SOCK_STREAM_P2P: return "STREAM-P2P";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<lv2_socket_family>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case SYS_NET_AF_UNSPEC: return "UNSPEC";
		case SYS_NET_AF_LOCAL: return "LOCAL";
		case SYS_NET_AF_INET: return "INET";
		case SYS_NET_AF_INET6: return "INET6";
		}

		return unknown;
	});
}


template <>
void fmt_class_string<struct in_addr>::format(std::string& out, u64 arg)
{
	const uchar* data = reinterpret_cast<const uchar*>(&get_object(arg));

	fmt::append(out, "%u.%u.%u.%u", data[0], data[1], data[2], data[3]);
}

#ifdef _WIN32
// Workaround function for WSAPoll not reporting failed connections
void windows_poll(pollfd* fds, unsigned long nfds, int timeout, bool* connecting)
{
	// Don't call WSAPoll with zero nfds (errors 10022 or 10038)
	if (std::none_of(fds, fds + nfds, [](pollfd& pfd) { return pfd.fd != INVALID_SOCKET; }))
	{
		if (timeout > 0)
		{
			Sleep(timeout);
		}

		return;
	}

	int r = ::WSAPoll(fds, nfds, timeout);

	if (r == SOCKET_ERROR)
	{
		sys_net.error("WSAPoll failed: %u", WSAGetLastError());
		return;
	}

	for (unsigned long i = 0; i < nfds; i++)
	{
		if (connecting[i])
		{
			if (!fds[i].revents)
			{
				int error = 0;
				socklen_t intlen = sizeof(error);
				if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &intlen) == -1 || error != 0)
				{
					// Connection silently failed
					connecting[i] = false;
					fds[i].revents = POLLERR | POLLHUP | (fds[i].events & (POLLIN | POLLOUT));
				}
			}
			else
			{
				connecting[i] = false;
			}
		}
	}
}
#endif

// Error helper functions
static sys_net_error get_last_error(bool is_blocking, int native_error = 0)
{
	// Convert the error code for socket functions to a one for sys_net
	sys_net_error result;
	const char* name{};

#ifdef _WIN32
	if (!native_error)
	{
		native_error = WSAGetLastError();
	}

	switch (native_error)
#define ERROR_CASE(error) case WSA ## error: result = SYS_NET_ ## error; name = #error; break;
#else
	if (!native_error)
	{
		native_error = errno;
	}

	switch (native_error)
#define ERROR_CASE(error) case error: result = SYS_NET_ ## error; name = #error; break;
#endif
	{
		ERROR_CASE(EWOULDBLOCK);
		ERROR_CASE(EINPROGRESS);
		ERROR_CASE(EALREADY);
		ERROR_CASE(ENOTCONN);
		ERROR_CASE(ECONNRESET);
		ERROR_CASE(EADDRINUSE);
	default: sys_net.error("Unknown/illegal socket error: %d", native_error);
	}

	if (name && result != SYS_NET_EWOULDBLOCK && result != SYS_NET_EINPROGRESS)
	{
		sys_net.error("Socket error %s", name);
	}

	if (is_blocking && result == SYS_NET_EWOULDBLOCK)
	{
		return {};
	}

	if (is_blocking && result == SYS_NET_EINPROGRESS)
	{
		return {};
	}

	return result;
#undef ERROR_CASE
}

static void network_clear_queue(ppu_thread& ppu)
{
	idm::select<lv2_socket>([&](u32, lv2_socket& sock)
	{
		std::lock_guard lock(sock.mutex);

		for (auto it = sock.queue.begin(); it != sock.queue.end();)
		{
			if (it->first == ppu.id)
			{
				it = sock.queue.erase(it);
				continue;
			}

			it++;
		}

		if (sock.queue.empty())
		{
			sock.events.store({});
		}
	});
}

// Object in charge of retransmiting packets for STREAM_P2P sockets
class tcp_timeout_monitor : public need_wakeup
{
public:
	void add_message(s32 sock_id, const sockaddr_in *dst, std::vector<u8> data, u64 seq)
	{
		{
			std::lock_guard lock(data_mutex);

			const auto now = std::chrono::system_clock::now();

			message msg;
			msg.dst_addr = *dst;
			msg.sock_id = sock_id;
			msg.data = std::move(data);
			msg.seq = seq;
			msg.initial_sendtime = now;

			rtt_info rtt = rtts[sock_id];

			const auto expected_time = now + rtt.rtt_time;

			msgs.insert(std::make_pair(expected_time, std::move(msg)));
		}
		wakey.notify_one(); // TODO: Should be improved to only wake if new timeout < old timeout
	}

	void confirm_data_received(s32 sock_id, u64 ack)
	{
		std::lock_guard lock(data_mutex);
		rtts[sock_id].num_retries = 0;
		// TODO: reduce RTT?
		for (auto it = msgs.begin(); it != msgs.end();)
		{
			auto& msg = it->second;
			if (msg.sock_id == sock_id && msg.seq < ack)
			{
				it = msgs.erase(it);
				continue;
			}
			it++;
		}
	}

	void operator()()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			std::unique_lock<std::mutex> lock(data_mutex);
			if (msgs.size())
				wakey.wait_until(lock, msgs.begin()->first);
			else
				wakey.wait(lock);

			if (thread_ctrl::state() == thread_state::aborting)
				return;

			const auto now = std::chrono::system_clock::now();
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
						sock.p2ps.status = lv2_socket::p2ps_i::stream_status::stream_closed;
					});
					it = msgs.erase(it);
					continue;
				}

				// resend the message
				const auto res = idm::check<lv2_socket>(msg.sock_id, [&](lv2_socket& sock) -> bool
				{
					if (sendto(sock.socket, reinterpret_cast<char*>(msg.data.data()), msg.data.size(), 0, reinterpret_cast<const sockaddr*>(&msg.dst_addr), sizeof(msg.dst_addr)) == -1)
					{
						sock.p2ps.status = lv2_socket::p2ps_i::stream_status::stream_closed;
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
		}
	}

	void wake_up()
	{
		wakey.notify_one();
	}

	public:
		static constexpr auto thread_name = "Tcp Over Udp Timeout Manager Thread"sv;

	private:
		std::condition_variable wakey;
		std::mutex data_mutex;
		// List of outgoing messages
		struct message
		{
			s32 sock_id;
			::sockaddr_in dst_addr;
			std::vector<u8> data;
			u64 seq;
			std::chrono::time_point<std::chrono::system_clock> initial_sendtime;
		};
		std::map<std::chrono::time_point<std::chrono::system_clock>, message> msgs; // (wakeup time, msg)
		// List of rtts
		struct rtt_info
		{
			unsigned long num_retries          = 0;
			std::chrono::milliseconds rtt_time = 50ms;
		};
		std::unordered_map<s32, rtt_info> rtts; // (sock_id, rtt)
	};

struct nt_p2p_port
{
	// Real socket where P2P packets are received/sent
	lv2_socket::socket_type p2p_socket = 0;
	u16 port;

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

	nt_p2p_port(u16 port) : port(port)
	{
		// Creates and bind P2P Socket
		p2p_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

		if (p2p_socket == -1)
			sys_net.fatal("Failed to create DGRAM socket for P2P socket!");

#ifdef _WIN32
		u_long _true = 1;
		::ioctlsocket(p2p_socket, FIONBIO, &_true);
#else
		::fcntl(p2p_socket, F_SETFL, ::fcntl(p2p_socket, F_GETFL, 0) | O_NONBLOCK);
#endif

		u32 optval = 131072;
		if (setsockopt(p2p_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&optval), sizeof(optval)) != 0)
			sys_net.fatal("Error setsockopt SO_RCVBUF on P2P socket");

		::sockaddr_in p2p_saddr{};
		p2p_saddr.sin_family      = AF_INET;
		p2p_saddr.sin_port        = std::bit_cast<u16, be_t<u16>>(port); // htons(port);
		p2p_saddr.sin_addr.s_addr = 0; // binds to 0.0.0.0
		const auto ret_bind       = ::bind(p2p_socket, reinterpret_cast<sockaddr*>(&p2p_saddr), sizeof(p2p_saddr));

		if (ret_bind == -1)
			sys_net.fatal("Failed to bind DGRAM socket to %d for P2P!", port);

		sys_net.notice("P2P port %d was bound!", port);
	}

	~nt_p2p_port()
	{
		if (p2p_socket)
		{
#ifdef _WIN32
			::closesocket(p2p_socket);
#else
			::close(p2p_socket);
#endif
		}
	}

	static u16 tcp_checksum(const u16* buffer, std::size_t size)
	{
		u32 cksum = 0;
		while (size > 1)
		{
			cksum += *buffer++;
			size -= sizeof(u16);
		}
		if (size)
			cksum += *reinterpret_cast<const u8 *>(buffer);

		cksum = (cksum >> 16) + (cksum & 0xffff);
		cksum += (cksum >> 16);
		return static_cast<u16>(~cksum);
	}

	static std::vector<u8> generate_u2s_packet(const lv2_socket::p2ps_i::encapsulated_tcp& header, const u8 *data, const u32 datasize)
	{
		const u32 packet_size = (sizeof(u16) + sizeof(lv2_socket::p2ps_i::encapsulated_tcp) + datasize);
		ASSERT(packet_size < 65535); // packet size shouldn't be bigger than possible UDP payload
		std::vector<u8> packet(packet_size);
		u8 *packet_data = packet.data();

		*reinterpret_cast<le_t<u16> *>(packet_data) = header.dst_port;
		memcpy(packet_data+sizeof(u16), &header, sizeof(lv2_socket::p2ps_i::encapsulated_tcp));
		if(datasize)
			memcpy(packet_data+sizeof(u16)+sizeof(lv2_socket::p2ps_i::encapsulated_tcp), data, datasize);
		
		auto* hdr_ptr = reinterpret_cast<lv2_socket::p2ps_i::encapsulated_tcp *>(packet_data+sizeof(u16));
		hdr_ptr->checksum = 0;
		hdr_ptr->checksum = tcp_checksum(reinterpret_cast<u16 *>(hdr_ptr), sizeof(lv2_socket::p2ps_i::encapsulated_tcp) + datasize);

		return packet;
	}

	static void send_u2s_packet(lv2_socket &sock, s32 sock_id, std::vector<u8> data, const ::sockaddr_in* dst, u32 seq = 0, bool require_ack = true)
	{
		sys_net.trace("Sending U2S packet on socket %d(id:%d): data(%d, seq %d, require_ack %d) to %s:%d", sock.socket, sock_id, data.size(), seq, require_ack, inet_ntoa(dst->sin_addr), std::bit_cast<u16, be_t<u16>>(dst->sin_port));
		if (sendto(sock.socket, reinterpret_cast<char *>(data.data()), data.size(), 0, reinterpret_cast<const sockaddr*>(dst), sizeof(sockaddr_in)) == -1)
		{
			sys_net.error("Attempting to send a u2s packet failed(%s), closing socket!", get_last_error(false));
			sock.p2ps.status = lv2_socket::p2ps_i::stream_status::stream_closed;
			return;
		}

		// Adds to tcp timeout monitor to resend the message until an ack is received
		if (require_ack)
		{
			auto tcpm = g_fxo->get<named_thread<tcp_timeout_monitor>>();
			tcpm->add_message(sock_id, dst, std::move(data), seq);
		}
	}

	void dump_packet(lv2_socket::p2ps_i::encapsulated_tcp* tcph)
	{
		const std::string result = fmt::format("src_port: %d\ndst_port: %d\nflags: %d\nseq: %d\nack: %d\nlen: %d", tcph->src_port, tcph->dst_port, tcph->flags, tcph->seq, tcph->ack, tcph->length);
		sys_net.trace("PACKET DUMP:\n%s", result);
	}

	bool handle_connected(s32 sock_id, lv2_socket::p2ps_i::encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr)
	{
		const auto sock = idm::check<lv2_socket>(sock_id, [&](lv2_socket& sock) -> bool
		{
			std::lock_guard lock(sock.mutex);

			if (sock.p2ps.status != lv2_socket::p2ps_i::stream_status::stream_connected && sock.p2ps.status != lv2_socket::p2ps_i::stream_status::stream_handshaking)
				return false;
			
			dump_packet(tcp_header);

			if (tcp_header->flags == lv2_socket::p2ps_i::ACK)
			{
				auto tcpm = g_fxo->get<named_thread<tcp_timeout_monitor>>();
				tcpm->confirm_data_received(sock_id, tcp_header->ack);
			}

			auto send_ack = [&]()
			{
				auto final_ack = sock.p2ps.data_beg_seq;
				while (sock.p2ps.received_data.count(final_ack))
				{
					final_ack += sock.p2ps.received_data.at(final_ack).size();
				}
				sock.p2ps.data_available = final_ack - sock.p2ps.data_beg_seq;

				lv2_socket::p2ps_i::encapsulated_tcp send_hdr;
				send_hdr.src_port = tcp_header->dst_port;
				send_hdr.dst_port = tcp_header->src_port;
				send_hdr.flags    = lv2_socket::p2ps_i::ACK;
				send_hdr.ack      = final_ack;
				auto packet       = generate_u2s_packet(send_hdr, nullptr, 0);
				sys_net.trace("Sent ack %d", final_ack);
				send_u2s_packet(sock, sock_id, std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), 0, false);
			};

			if (sock.p2ps.status == lv2_socket::p2ps_i::stream_status::stream_handshaking)
			{
				// Only expect SYN|ACK
				if (tcp_header->flags == (lv2_socket::p2ps_i::SYN | lv2_socket::p2ps_i::ACK))
				{
					sys_net.trace("Received SYN|ACK, status is now connected");
					sock.p2ps.data_beg_seq = tcp_header->seq + 1;
					sock.p2ps.status       = lv2_socket::p2ps_i::stream_status::stream_connected;
					send_ack();
				}

				return true;
			}
			else if (sock.p2ps.status == lv2_socket::p2ps_i::stream_status::stream_connected)
			{
				if (tcp_header->seq < sock.p2ps.data_beg_seq)
				{
					// Data has already been processed
					sys_net.trace("Data has already been processed");
					if (tcp_header->flags != lv2_socket::p2ps_i::ACK && tcp_header->flags != lv2_socket::p2ps_i::RST)
						send_ack();
					return true;
				}

				switch (tcp_header->flags)
				{
				case lv2_socket::p2ps_i::PSH:
				case 0:
				{
					if (!sock.p2ps.received_data.count(tcp_header->seq))
					{
						// New data
						sock.p2ps.received_data.emplace(tcp_header->seq, std::vector<u8>(data, data + tcp_header->length));
					}
					else
					{
						sys_net.trace("Data was not new!");
					}

					send_ack();
					return true;
				}
				case lv2_socket::p2ps_i::RST:
				case lv2_socket::p2ps_i::FIN:
				{
					sock.p2ps.status = lv2_socket::p2ps_i::stream_status::stream_closed;
					return false;
				}
				default:
				{
					sys_net.error("Unknown U2S TCP flag received");
					return true;
				}
				}
			}

			return true;
		});

		if (!sock || !sock.ret)
			return false;
		
		return true;
	}

	bool handle_listening(s32 sock_id, lv2_socket::p2ps_i::encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr)
	{
		auto sock = idm::get<lv2_socket>(sock_id);
		if (!sock)
			return false;

		std::lock_guard lock(sock->mutex);

		if (sock->p2ps.status != lv2_socket::p2ps_i::stream_status::stream_listening)
			return false;

		// Only valid packet
		if (tcp_header->flags == lv2_socket::p2ps_i::SYN && sock->p2ps.backlog.size() < sock->p2ps.max_backlog)
		{
			// Yes, new connection and a backlog is available, create a new lv2_socket for it and send SYN|ACK
			// Prepare reply packet
			sys_net.notice("Received connection on listening STREAM-P2P socket!");
			lv2_socket::p2ps_i::encapsulated_tcp send_hdr;
			send_hdr.src_port = tcp_header->dst_port;
			send_hdr.dst_port = tcp_header->src_port;
			send_hdr.flags    = lv2_socket::p2ps_i::SYN | lv2_socket::p2ps_i::ACK;
			send_hdr.ack      = tcp_header->seq + 1;
			// Generates random starting SEQ
			send_hdr.seq      = rand();

			// Create new socket
			auto sock_lv2               = std::make_shared<lv2_socket>(0, SYS_NET_SOCK_STREAM_P2P, SYS_NET_AF_INET);
			sock_lv2->socket            = sock->socket;
			sock_lv2->p2p.port          = sock->p2p.port;
			sock_lv2->p2p.vport         = sock->p2p.vport;
			sock_lv2->p2ps.op_addr      = reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_addr.s_addr;
			sock_lv2->p2ps.op_port      = std::bit_cast<u16, be_t<u16>>((reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_port));
			sock_lv2->p2ps.op_vport     = tcp_header->src_port;
			sock_lv2->p2ps.cur_seq      = send_hdr.seq + 1;
			sock_lv2->p2ps.data_beg_seq = send_hdr.ack;
			sock_lv2->p2ps.status       = lv2_socket::p2ps_i::stream_status::stream_connected;
			const s32 new_sock_id       = idm::import_existing<lv2_socket>(sock_lv2);
			const u64 key_connected     = (reinterpret_cast<struct sockaddr_in*>(op_addr)->sin_addr.s_addr) | (static_cast<u64>(tcp_header->src_port) << 48) | (static_cast<u64>(tcp_header->dst_port) << 32);
			bound_p2p_streams.emplace(key_connected, new_sock_id);

			auto packet = generate_u2s_packet(send_hdr, nullptr, 0);
			{
				std::lock_guard lock(sock_lv2->mutex);
				send_u2s_packet(*sock_lv2, new_sock_id, std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), send_hdr.seq);
			}

			sock->p2ps.backlog.push(new_sock_id);
		}
		else if (tcp_header->flags == lv2_socket::p2ps_i::SYN)
		{
			// Send a RST packet on backlog full
			sys_net.trace("Backlog was full, sent a RST packet");
			lv2_socket::p2ps_i::encapsulated_tcp send_hdr;
			send_hdr.src_port = tcp_header->dst_port;
			send_hdr.dst_port = tcp_header->src_port;
			send_hdr.flags    = lv2_socket::p2ps_i::RST;
			auto packet       = generate_u2s_packet(send_hdr, nullptr, 0);
			send_u2s_packet(*sock, sock_id, std::move(packet), reinterpret_cast<::sockaddr_in*>(op_addr), 0, false);
		}

		// Ignore other packets?

		return true;
	}

	bool recv_data()
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

		if (dst_vport == 0) // Reserved for messages from RPCN server
		{
			std::vector<u8> rpcn_msg(recv_res - sizeof(u16));
			memcpy(rpcn_msg.data(), p2p_recv_data.data() + sizeof(u16), recv_res - sizeof(u16));

			std::lock_guard lock(s_rpcn_mutex);
			rpcn_msgs.push_back(std::move(rpcn_msg));
			return true;
		}

		if (dst_vport == 65535) // Reserved for signaling
		{
			std::vector<u8> sign_msg(recv_res - sizeof(u16));
			memcpy(sign_msg.data(), p2p_recv_data.data() + sizeof(u16), recv_res - sizeof(u16));

			std::pair<std::pair<u32, u16>, std::vector<u8>> msg;
			msg.first.first = reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr;
			msg.first.second = std::bit_cast<u16, be_t<u16>>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
			msg.second = std::move(sign_msg);

			{
				std::lock_guard lock(s_sign_mutex);
				sign_msgs.push_back(std::move(msg));
			}

			const auto sigh = g_fxo->get<named_thread<signaling_handler>>();
			sigh->wake_up();

			return true;
		}

		{
			std::lock_guard lock(bound_p2p_vports_mutex);
			if (bound_p2p_vports.count(dst_vport))
			{
				sys_net_sockaddr_in_p2p p2p_addr{};

				p2p_addr.sin_len    = sizeof(sys_net_sockaddr_in);
				p2p_addr.sin_family = SYS_NET_AF_INET;
				p2p_addr.sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
				p2p_addr.sin_vport  = dst_vport; // That is weird stuff
				p2p_addr.sin_port = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);

				std::vector<u8> p2p_data(recv_res - sizeof(u16));
				memcpy(p2p_data.data(), p2p_recv_data.data() + sizeof(u16), recv_res - sizeof(u16));

				const auto sock = idm::check<lv2_socket>(bound_p2p_vports.at(dst_vport), [&](lv2_socket& sock)
				{
					std::lock_guard lock(sock.mutex);
					ASSERT(sock.type == SYS_NET_SOCK_DGRAM_P2P);

					sock.p2p.data.push(std::make_pair(std::move(p2p_addr), std::move(p2p_data)));
					sys_net.trace("Received a P2P packet for vport %d and saved it", dst_vport);
				});

				// Should not happen in theory
				if (!sock)
					bound_p2p_vports.erase(dst_vport);

				return true;
			}
		}

		// Not directed at a bound DGRAM_P2P vport so check if the packet is a STREAM-P2P packet

		const auto sp_size = recv_res - sizeof(u16);
		u8 *sp_data = p2p_recv_data.data() + sizeof(u16);

		if (sp_size < sizeof(lv2_socket::p2ps_i::encapsulated_tcp))
		{
			sys_net.trace("Received P2P packet targeted at unbound vport(likely) or invalid");
			return true;
		}

		auto* tcp_header = reinterpret_cast<lv2_socket::p2ps_i::encapsulated_tcp*>(sp_data);
		
		// Validate signature & length
		if (tcp_header->signature != lv2_socket::p2ps_i::U2S_sig)
		{
			sys_net.trace("Received P2P packet targeted at unbound vport");
			return true;
		}

		if (tcp_header->length != (sp_size - sizeof(lv2_socket::p2ps_i::encapsulated_tcp)))
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
		if (given_checksum != nt_p2p_port::tcp_checksum(reinterpret_cast<const u16*>(sp_data), sp_size))
		{
			sys_net.error("Checksum is invalid, dropping packet!");
			return true;
		}

		// The packet is valid, check if it's bound
		const u64 key_connected = (reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr) | (static_cast<u64>(tcp_header->src_port) << 48) | (static_cast<u64>(tcp_header->dst_port) << 32);
		const u64 key_listening = (static_cast<u64>(tcp_header->dst_port) << 32);

		{
			std::lock_guard lock(bound_p2p_vports_mutex);
			if (bound_p2p_streams.count(key_connected))
			{
				const auto sock_id = bound_p2p_streams.at(key_connected);
				sys_net.trace("Received packet for connected STREAM-P2P socket(s=%d)", sock_id);
				handle_connected(sock_id, tcp_header, sp_data + sizeof(lv2_socket::p2ps_i::encapsulated_tcp), &native_addr);
				return true;
			}

			if(bound_p2p_streams.count(key_listening))
			{
				const auto sock_id = bound_p2p_streams.at(key_listening);
				sys_net.trace("Received packet for listening STREAM-P2P socket(s=%d)", sock_id);
				handle_listening(sock_id, tcp_header, sp_data + sizeof(lv2_socket::p2ps_i::encapsulated_tcp), &native_addr);
				return true;
			}
		}

		sys_net.trace("Received a STREAM-P2P packet with no bound target");
		return true;
	}
};

struct network_thread
{
	std::vector<ppu_thread*> s_to_awake;
	shared_mutex s_nw_mutex;

	shared_mutex list_p2p_ports_mutex;
	std::map<u16, nt_p2p_port> list_p2p_ports{};	

	static constexpr auto thread_name = "Network Thread";

	network_thread() noexcept
	{
#ifdef _WIN32
		WSADATA wsa_data;
		WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
		list_p2p_ports.emplace(std::piecewise_construct, std::forward_as_tuple(3658), std::forward_as_tuple(3658));
	}

	~network_thread()
	{
#ifdef _WIN32
		WSACleanup();
#endif
	}

	void operator()()
	{
		std::vector<std::shared_ptr<lv2_socket>> socklist;
		socklist.reserve(lv2_socket::id_count);

		s_to_awake.clear();

		::pollfd fds[lv2_socket::id_count]{};
#ifdef _WIN32
		bool connecting[lv2_socket::id_count]{};
		bool was_connecting[lv2_socket::id_count]{};
#endif

		::pollfd p2p_fd[lv2_socket::id_count]{};

		while (thread_ctrl::state() != thread_state::aborting)
		{
			// Wait with 1ms timeout
#ifdef _WIN32
			windows_poll(fds, ::size32(socklist), 1, connecting);
#else
			::poll(fds, socklist.size(), 1);
#endif

			// Check P2P sockets for incoming packets(timeout could probably be set at 0)
			{
				std::lock_guard lock(list_p2p_ports_mutex);
				std::memset(p2p_fd, 0, sizeof(p2p_fd));
				auto num_p2p_sockets = 0;
				for (const auto& p2p_port : list_p2p_ports)
				{
					p2p_fd[num_p2p_sockets].events  = POLLIN;
					p2p_fd[num_p2p_sockets].revents = 0;
					p2p_fd[num_p2p_sockets].fd      = p2p_port.second.p2p_socket;
					num_p2p_sockets++;
				}

				if (num_p2p_sockets != 0)
				{
#ifdef _WIN32
					const auto ret_p2p = WSAPoll(p2p_fd, num_p2p_sockets, 1);
#else
					const auto ret_p2p = ::poll(p2p_fd, num_p2p_sockets, 1);
#endif
					if (ret_p2p > 0)
					{
						auto fd_index = 0;
						for (auto& p2p_port : list_p2p_ports)
						{
							if ((p2p_fd[fd_index].revents & POLLIN) == POLLIN || (p2p_fd[fd_index].revents & POLLRDNORM) == POLLRDNORM)
							{
								while(p2p_port.second.recv_data());
							}
							fd_index++;
						}
					}
					else if (ret_p2p < 0)
					{
						sys_net.error("[P2P] Error poll on master P2P socket: %d", get_last_error(false));
					}
				}
			}

			std::lock_guard lock(s_nw_mutex);

			for (std::size_t i = 0; i < socklist.size(); i++)
			{
				bs_t<lv2_socket::poll> events{};

				lv2_socket& sock = *socklist[i];

				if (fds[i].revents & (POLLIN | POLLHUP) && socklist[i]->events.test_and_reset(lv2_socket::poll::read))
					events += lv2_socket::poll::read;
				if (fds[i].revents & POLLOUT && socklist[i]->events.test_and_reset(lv2_socket::poll::write))
					events += lv2_socket::poll::write;
				if (fds[i].revents & POLLERR && socklist[i]->events.test_and_reset(lv2_socket::poll::error))
					events += lv2_socket::poll::error;

				if (events)
				{
					std::lock_guard lock(socklist[i]->mutex);

#ifdef _WIN32
					if (was_connecting[i] && !connecting[i])
						sock.is_connecting = false;
#endif

					for (auto it = socklist[i]->queue.begin(); events && it != socklist[i]->queue.end();)
					{
						if (it->second(events))
						{
							it = socklist[i]->queue.erase(it);
							continue;
						}

						it++;
					}

					if (socklist[i]->queue.empty())
					{
						socklist[i]->events.store({});
					}
				}
			}

			s_to_awake.erase(std::unique(s_to_awake.begin(), s_to_awake.end()), s_to_awake.end());

			for (ppu_thread* ppu : s_to_awake)
			{
				network_clear_queue(*ppu);
				lv2_obj::append(ppu);
			}

			if (!s_to_awake.empty())
			{
				lv2_obj::awake_all();
			}

			s_to_awake.clear();
			socklist.clear();

			// Obtain all active sockets
			idm::select<lv2_socket>([&](u32 id, lv2_socket&)
			{
				socklist.emplace_back(idm::get_unlocked<lv2_socket>(id));
			});

			for (std::size_t i = 0; i < socklist.size(); i++)
			{
#ifdef _WIN32
				std::lock_guard lock(socklist[i]->mutex);
#endif

				auto events = socklist[i]->events.load();

				fds[i].fd = events ? socklist[i]->socket : -1;
				fds[i].events =
					(events & lv2_socket::poll::read ? POLLIN : 0) |
					(events & lv2_socket::poll::write ? POLLOUT : 0) |
					0;
				fds[i].revents = 0;
#ifdef _WIN32
				was_connecting[i] = socklist[i]->is_connecting;
				connecting[i] = socklist[i]->is_connecting;
#endif
			}
		}
	}
};

using network_context = named_thread<network_thread>;

// Used by RPCN to send signaling packets to RPCN server(for UDP hole punching)
s32 send_packet_from_p2p_port(const std::vector<u8>& data, const sockaddr_in& addr)
{
	s32 res       = 0;
	const auto nc = g_fxo->get<network_context>();
	{
		std::lock_guard list_lock(nc->list_p2p_ports_mutex);
		auto& def_port = nc->list_p2p_ports.at(3658);

		res = ::sendto(def_port.p2p_socket, reinterpret_cast<const char*>(data.data()), data.size(), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_in));
	}

	return res;
}

std::vector<std::vector<u8>> get_rpcn_msgs()
{
	auto msgs     = std::vector<std::vector<u8>>();
	const auto nc = g_fxo->get<network_context>();
	{
		std::lock_guard list_lock(nc->list_p2p_ports_mutex);
		auto& def_port = nc->list_p2p_ports.at(3658);
		{
			std::lock_guard lock(def_port.s_rpcn_mutex);
			msgs               = std::move(def_port.rpcn_msgs);
			def_port.rpcn_msgs.clear();
		}
	}

	return msgs;
}

std::vector<std::pair<std::pair<u32, u16>, std::vector<u8>>> get_sign_msgs()
{
	auto msgs     = std::vector<std::pair<std::pair<u32, u16>, std::vector<u8>>>();
	const auto nc = g_fxo->get<network_context>();
	{
		std::lock_guard list_lock(nc->list_p2p_ports_mutex);
		auto& def_port = nc->list_p2p_ports.at(3658);
		{
			std::lock_guard lock(def_port.s_sign_mutex);
			msgs = std::move(def_port.sign_msgs);
			def_port.sign_msgs.clear();
		}
	}

	return msgs;
}


lv2_socket::lv2_socket(lv2_socket::socket_type s, s32 s_type, s32 family)
	: socket(s), type{s_type}, family{family}
{
	if (socket)
	{
		// Set non-blocking
#ifdef _WIN32
		u_long _true = 1;
		::ioctlsocket(socket, FIONBIO, &_true);
#else
		::fcntl(socket, F_SETFL, ::fcntl(socket, F_GETFL, 0) | O_NONBLOCK);
#endif
	}
}

lv2_socket::~lv2_socket()
{
	if (type != SYS_NET_SOCK_DGRAM_P2P && type != SYS_NET_SOCK_STREAM_P2P)
	{
#ifdef _WIN32
		::closesocket(socket);
#else
		::close(socket);
#endif
	}
}

error_code sys_net_bnet_accept(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_accept(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	if (addr.operator bool() != paddrlen.operator bool() || (paddrlen && *paddrlen < addr.size()))
	{
		return -SYS_NET_EINVAL;
	}

	lv2_socket::socket_type native_socket = -1;
	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);
	s32 result = 0;

	bool p2ps = false;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		std::lock_guard lock(sock.mutex);

		if (sock.type == SYS_NET_SOCK_STREAM_P2P)
		{
			if (sock.p2ps.backlog.size() == 0)
			{
				result = SYS_NET_EWOULDBLOCK;
				return false;
			}

			sys_net.trace("Found a socket in backlog!");

			p2ps = true;
			result = sock.p2ps.backlog.front();
			sock.p2ps.backlog.pop();

			if (addr)
			{
				sys_net_sockaddr_in_p2p *addr_p2p = reinterpret_cast<sys_net_sockaddr_in_p2p *>(addr.get_ptr());
				addr_p2p->sin_family = AF_INET;
				addr_p2p->sin_addr = std::bit_cast<be_t<u32>, u32>(sock.p2ps.op_addr);
				addr_p2p->sin_port = std::bit_cast<be_t<u16>, u16>(sock.p2ps.op_vport);
				addr_p2p->sin_vport = std::bit_cast<be_t<u16>, u16>(sock.p2ps.op_port);
				addr_p2p->sin_len = sizeof(sys_net_sockaddr_in_p2p);
			}

			return true;
		}

		//if (!(sock.events & lv2_socket::poll::read))
		{
			native_socket = ::accept(sock.socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

			if (native_socket != -1)
			{
				return true;
			}

			result = get_last_error(!sock.so_nbio);

			if (result)
			{
				return false;
			}
		}

		// Enable read event
		sock.events += lv2_socket::poll::read;
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (events & lv2_socket::poll::read)
			{
				native_socket = ::accept(sock.socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

				if (native_socket != -1 || (result = get_last_error(!sock.so_nbio)))
				{
					lv2_obj::awake(&ppu);
					return true;
				}
			}

			sock.events += lv2_socket::poll::read;
			return false;
		});

		lv2_obj::sleep(ppu);
		return false;
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret && result)
	{
		if (result == SYS_NET_EWOULDBLOCK)
		{
			return not_an_error(-result);
		}

		return -sys_net_error{result};
	}

	if (p2ps)
	{
		return not_an_error(result);
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			if (ppu.is_stopped())
			{
				return 0;
			}

			thread_ctrl::wait();
		}

		if (result)
		{
			return -sys_net_error{result};
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	if (ppu.is_stopped())
	{
		return 0;
	}

	auto newsock = std::make_shared<lv2_socket>(native_socket, 0, 0);

	result = idm::import_existing<lv2_socket>(newsock);

	if (result == id_manager::id_traits<lv2_socket>::invalid)
	{
		return -SYS_NET_EMFILE;
	}

	if (addr)
	{
		verify(HERE), native_addr.ss_family == AF_INET;

		vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

		*paddrlen         = sizeof(sys_net_sockaddr_in);

		paddr->sin_len    = sizeof(sys_net_sockaddr_in);
		paddr->sin_family = SYS_NET_AF_INET;
		paddr->sin_port   = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
		paddr->sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
		paddr->sin_zero   = 0;
	}

	// Socket id
	return not_an_error(result);
}

error_code sys_net_bnet_bind(ppu_thread& ppu, s32 s, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_bind(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	if (!addr || addrlen < addr.size())
	{
		return -SYS_NET_EINVAL;
	}

	alignas(16) char addr_buf[sizeof(sys_net_sockaddr)];

	if (idm::check<lv2_socket>(s))
	{
		std::memcpy(addr_buf, addr.get_ptr(), addr.size());
	}
	else
	{
		return -SYS_NET_EBADF;
	}

	const auto psa_in = reinterpret_cast<sys_net_sockaddr_in*>(addr_buf);
	const auto _addr = reinterpret_cast<sys_net_sockaddr*>(addr_buf);

	::sockaddr_in name{};
	name.sin_family      = AF_INET;
	name.sin_port        = std::bit_cast<u16>(psa_in->sin_port);
	name.sin_addr.s_addr = std::bit_cast<u32>(psa_in->sin_addr);
	::socklen_t namelen  = sizeof(name);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		// 0 presumably defaults to AF_INET(to check?)
		if (_addr->sa_family != SYS_NET_AF_INET && _addr->sa_family != 0)
		{
			sys_net.error("sys_net_bnet_bind(s=%d): unsupported sa_family (%d)", s, addr->sa_family);
			return SYS_NET_EAFNOSUPPORT;
		}

		if (sock.type == SYS_NET_SOCK_DGRAM_P2P || sock.type == SYS_NET_SOCK_STREAM_P2P)
		{
			auto psa_in_p2p = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(psa_in);
			u16 p2p_port, p2p_vport;
			if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
			{
				p2p_port  = psa_in_p2p->sin_port;
				p2p_vport = psa_in_p2p->sin_vport;
			}
			else
			{
				// For SYS_NET_SOCK_STREAM_P2P sockets, the port is the "fake" tcp port and the vport is the udp port it's bound to
				p2p_port  = psa_in_p2p->sin_vport;
				p2p_vport = psa_in_p2p->sin_port;
			}

			sys_net.notice("[P2P] %s, Socket bind to %s:%d:%d", sock.type, inet_ntoa(name.sin_addr), p2p_port, p2p_vport);

			if (p2p_port != 3658)
			{
				sys_net.warning("[P2P] Attempting to bind a socket to a port != 3658");
			}
			ASSERT(p2p_vport != 0);

			lv2_socket::socket_type real_socket{};

			const auto nc = g_fxo->get<network_context>();
			{
				std::lock_guard list_lock(nc->list_p2p_ports_mutex);
				if (nc->list_p2p_ports.count(p2p_port) == 0)
				{
					nc->list_p2p_ports.emplace(std::piecewise_construct, std::forward_as_tuple(p2p_port), std::forward_as_tuple(p2p_port));
				}

				auto& pport = nc->list_p2p_ports.at(p2p_port);
				real_socket = pport.p2p_socket;
				{
					std::lock_guard lock(pport.bound_p2p_vports_mutex);
					if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
					{
						if (pport.bound_p2p_vports.count(p2p_vport) != 0)
						{
							return sys_net_error::SYS_NET_EADDRINUSE;
						}
						pport.bound_p2p_vports.insert(std::make_pair(p2p_vport, s));
					}
					else
					{
						const u64 key = (static_cast<u64>(p2p_vport) << 32);
						pport.bound_p2p_streams.emplace(key, s);
					}
				}
			}

			{
				std::lock_guard lock(sock.mutex);
				sock.p2p.port  = p2p_port;
				sock.p2p.vport = p2p_vport;
				sock.socket    = real_socket;
			}

			return {};
		}
		else
		{
			sys_net.warning("Trying to bind %s:%d", name.sin_addr, std::bit_cast<be_t<u16>, u16>(name.sin_port)); // ntohs(name.sin_port)
		}

		std::lock_guard lock(sock.mutex);

		if (::bind(sock.socket, reinterpret_cast<struct sockaddr*>(&name), namelen) == 0)
		{
			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	return CELL_OK;
}

error_code sys_net_bnet_connect(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, u32 addrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_connect(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	if (!addr || addrlen < addr.size())
	{
		return -SYS_NET_EINVAL;
	}

	struct
	{
		alignas(16) char buf[sizeof(sys_net_sockaddr)];
		bool changed = false;
	} addr_buf;

	const auto psa_in = reinterpret_cast<sys_net_sockaddr_in*>(addr_buf.buf);
	const auto _addr = reinterpret_cast<sys_net_sockaddr*>(addr_buf.buf);

	s32 result = 0;
	::sockaddr_in name{};
	name.sin_family      = AF_INET;

	if (idm::check<lv2_socket>(s))
	{
		std::memcpy(addr_buf.buf, addr.get_ptr(), 16);
		name.sin_port        = std::bit_cast<u16>(psa_in->sin_port);
		name.sin_addr.s_addr = std::bit_cast<u32>(psa_in->sin_addr);
	}
	else
	{
		return -SYS_NET_EBADF;
	}

	::socklen_t namelen  = sizeof(name);

	sys_net.notice("Attempting to connect on %s:%d", name.sin_addr, std::bit_cast<be_t<u16>, u16>(name.sin_port)); // ntohs(name.sin_port)

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		if (sock.type == SYS_NET_SOCK_STREAM_P2P)
		{
			lv2_socket::p2ps_i::encapsulated_tcp send_hdr;
			std::vector<u8> packet;
			const auto psa_in_p2p = reinterpret_cast<sys_net_sockaddr_in_p2p*>(addr_buf.buf);
			{
				std::lock_guard lock(sock.mutex);
				// This is purposefully inverted, not a bug
				const u16 dst_vport = psa_in_p2p->sin_port;
				const u16 dst_port  = psa_in_p2p->sin_vport;

				lv2_socket::socket_type real_socket{};

				const auto nc = g_fxo->get<network_context>();
				{
					std::lock_guard list_lock(nc->list_p2p_ports_mutex);
					if (!nc->list_p2p_ports.count(sock.p2p.port))
						nc->list_p2p_ports.emplace(std::piecewise_construct, std::forward_as_tuple(sock.p2p.port), std::forward_as_tuple(sock.p2p.port));

					auto& pport = nc->list_p2p_ports.at(sock.p2p.port);
					real_socket = pport.p2p_socket;
					{
						std::lock_guard lock(pport.bound_p2p_vports_mutex);
						if (sock.p2p.vport == 0)
						{
							// Unassigned vport, assigns one
							sys_net.warning("vport was unassigned before connect!");
							u16 found_vport = 30000;
							while (true)
							{
								found_vport++;
								if (pport.bound_p2p_vports.count(found_vport))
									continue;
								if (pport.bound_p2p_streams.count(static_cast<u64>(found_vport) << 32))
									continue;

								break;
							}
							sock.p2p.vport = found_vport;
						}
						const u64 key = name.sin_addr.s_addr | (static_cast<u64>(sock.p2p.vport) << 32) | (static_cast<u64>(dst_vport) << 48);
						pport.bound_p2p_streams.emplace(key, s);
					}
				}

				sock.socket = real_socket;

				send_hdr.src_port = sock.p2p.vport;
				send_hdr.dst_port = dst_vport;
				send_hdr.flags    = lv2_socket::p2ps_i::SYN;
				send_hdr.seq      = rand();

				// sock.socket            = p2p_socket;
				sock.p2ps.op_addr      = name.sin_addr.s_addr;
				sock.p2ps.op_port      = dst_port;
				sock.p2ps.op_vport     = dst_vport;
				sock.p2ps.cur_seq      = send_hdr.seq + 1;
				sock.p2ps.data_beg_seq = 0;
				sock.p2ps.data_available = 0;
				sock.p2ps.received_data.clear();
				sock.p2ps.status       = lv2_socket::p2ps_i::stream_status::stream_handshaking;

				packet = nt_p2p_port::generate_u2s_packet(send_hdr, nullptr, 0);
				name.sin_port = std::bit_cast<u16>(psa_in_p2p->sin_vport); // not a bug
				nt_p2p_port::send_u2s_packet(sock, s, std::move(packet), reinterpret_cast<::sockaddr_in*>(&name), send_hdr.seq);
			}

			return true;
		}

		std::lock_guard lock(sock.mutex);

		if (psa_in->sin_port == 53)
		{
			const auto nph = g_fxo->get<named_thread<np_handler>>();

			// Hack for DNS (8.8.8.8:53)
			name.sin_port        = std::bit_cast<u16, be_t<u16>>(53);
			name.sin_addr.s_addr = nph->get_dns_ip();

			// Overwrite arg (probably used to validate recvfrom addr)
			psa_in->sin_family = SYS_NET_AF_INET;
			psa_in->sin_port   = 53;
			psa_in->sin_addr   = nph->get_dns_ip();
			addr_buf.changed = true;
			sys_net.notice("sys_net_bnet_connect: using DNS...");

			nph->add_dns_spy(s);
		}
		else if (_addr->sa_family != SYS_NET_AF_INET)
		{
			sys_net.error("sys_net_bnet_connect(s=%d): unsupported sa_family (%d)", s, _addr->sa_family);
		}

		if (::connect(sock.socket, reinterpret_cast<struct sockaddr*>(&name), namelen) == 0)
		{
			return true;
		}

		const bool is_blocking = !sock.so_nbio;

		result = get_last_error(is_blocking);

		if (result)
		{
			if (result == SYS_NET_EWOULDBLOCK)
			{
				result = SYS_NET_EINPROGRESS;
			}

			if (result == SYS_NET_EINPROGRESS)
			{
#ifdef _WIN32
				sock.is_connecting = true;
#endif
				sock.events += lv2_socket::poll::write;
				sock.queue.emplace_back(u32{0}, [&sock](bs_t<lv2_socket::poll> events) -> bool
				{
					if (events & lv2_socket::poll::write)
					{
						int native_error;
						::socklen_t size = sizeof(native_error);
						if (::getsockopt(sock.socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&native_error), &size) != 0 || size != sizeof(int))
						{
							sock.so_error = 1;
						}
						else
						{
							// TODO: check error formats (both native and translated)
							sock.so_error = native_error ? get_last_error(false, native_error) : 0;
						}

						return true;
					}

					sock.events += lv2_socket::poll::write;
					return false;
				});
			}

			return false;
		}

#ifdef _WIN32
		sock.is_connecting = true;
#endif
		sock.events += lv2_socket::poll::write;
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (events & lv2_socket::poll::write)
			{
				int native_error;
				::socklen_t size = sizeof(native_error);
				if (::getsockopt(sock.socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&native_error), &size) != 0 || size != sizeof(int))
				{
					result = 1;
				}
				else
				{
					// TODO: check error formats (both native and translated)
					result = native_error ? get_last_error(false, native_error) : 0;
				}

				lv2_obj::awake(&ppu);
				return true;
			}

			sock.events += lv2_socket::poll::write;
			return false;
		});

		lv2_obj::sleep(ppu);
		return false;
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (addr_buf.changed)
	{
		std::memcpy(addr.get_ptr(), addr_buf.buf, sizeof(addr_buf.buf));
	}

	if (!sock.ret && result)
	{
		if (result == SYS_NET_EWOULDBLOCK || result == SYS_NET_EINPROGRESS)
		{
			return not_an_error(-result);
		}

		return -sys_net_error{result};
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			if (ppu.is_stopped())
			{
				return 0;
			}

			thread_ctrl::wait();
		}

		if (result)
		{
			return -sys_net_error{result};
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	return CELL_OK;
}

error_code sys_net_bnet_getpeername(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_getpeername(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	// Note: paddrlen is both an input and output argument
	if (!addr || !paddrlen || *paddrlen < addr.size())
	{
		return -SYS_NET_EINVAL;
	}

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		ASSERT(sock.type != SYS_NET_SOCK_DGRAM_P2P);

		if (::getpeername(sock.socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen) == 0)
		{
			verify(HERE), native_addr.ss_family == AF_INET;

			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	*paddrlen = sizeof(sys_net_sockaddr_in);

	vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

	paddr->sin_len    = sizeof(sys_net_sockaddr_in);
	paddr->sin_family = SYS_NET_AF_INET;
	paddr->sin_port   = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
	paddr->sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
	paddr->sin_zero   = 0;

	return CELL_OK;
}

error_code sys_net_bnet_getsockname(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_getsockname(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	// Note: paddrlen is both an input and output argument
	if (!addr || !paddrlen || *paddrlen < addr.size())
	{
		return -SYS_NET_EINVAL;
	}

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	lv2_socket_type type;
	u16 p2p_vport;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		type = sock.type;
		p2p_vport = sock.p2p.vport;

		if (::getsockname(sock.socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen) == 0)
		{
			verify(HERE), native_addr.ss_family == AF_INET;

			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	*paddrlen         = sizeof(sys_net_sockaddr_in);

	vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());
	paddr->sin_len    = sizeof(sys_net_sockaddr_in);
	paddr->sin_family = SYS_NET_AF_INET;
	paddr->sin_port   = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
	paddr->sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
	paddr->sin_zero   = 0;

	if (type == SYS_NET_SOCK_DGRAM_P2P)
	{
		vm::ptr<sys_net_sockaddr_in_p2p> paddr_p2p = vm::cast(addr.addr());
		paddr_p2p->sin_vport                       = p2p_vport;
		struct in_addr rep;
		rep.s_addr = htonl(paddr->sin_addr);
		sys_net.trace("[P2P] Reporting socket address as %s:%d:%d", rep, paddr_p2p->sin_port, paddr_p2p->sin_vport);
	}

	return CELL_OK;
}

error_code sys_net_bnet_getsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::ptr<void> optval, vm::ptr<u32> optlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=*0x%x)", s, level, optname, optval, optlen);

	if (!optval || !optlen)
	{
		return -SYS_NET_EINVAL;
	}

	const u32 len = *optlen;

	if (!len)
	{
		return -SYS_NET_EINVAL;
	}

	int native_level = -1;
	int native_opt = -1;

	union
	{
		char ch[128];
		int _int = 0;
		::timeval timeo;
		::linger linger;
	} native_val;
	::socklen_t native_len = sizeof(native_val);

	union
	{
		char ch[128];
		be_t<s32> _int = 0;
		sys_net_timeval timeo;
		sys_net_linger linger;
	} out_val;
	u32 out_len = sizeof(out_val);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		if (len < sizeof(s32))
		{
			return SYS_NET_EINVAL;
		}

		if (level == SYS_NET_SOL_SOCKET)
		{
			native_level = SOL_SOCKET;

			switch (optname)
			{
			case SYS_NET_SO_NBIO:
			{
				// Special
				out_val._int = sock.so_nbio;
				out_len = sizeof(s32);
				return {};
			}
			case SYS_NET_SO_ERROR:
			{
				// Special
				out_val._int = std::exchange(sock.so_error, 0);
				out_len = sizeof(s32);
				return {};
			}
			case SYS_NET_SO_KEEPALIVE:
			{
				native_opt = SO_KEEPALIVE;
				break;
			}
			case SYS_NET_SO_SNDBUF:
			{
				native_opt = SO_SNDBUF;
				break;
			}
			case SYS_NET_SO_RCVBUF:
			{
				native_opt = SO_RCVBUF;
				break;
			}
			case SYS_NET_SO_SNDLOWAT:
			{
				native_opt = SO_SNDLOWAT;
				break;
			}
			case SYS_NET_SO_RCVLOWAT:
			{
				native_opt = SO_RCVLOWAT;
				break;
			}
			case SYS_NET_SO_BROADCAST:
			{
				native_opt = SO_BROADCAST;
				break;
			}
#ifdef _WIN32
			case SYS_NET_SO_REUSEADDR:
			{
				out_val._int = sock.so_reuseaddr;
				out_len = sizeof(s32);
				return {};
			}
			case SYS_NET_SO_REUSEPORT:
			{
				out_val._int = sock.so_reuseport;
				out_len = sizeof(s32);
				return {};
			}
#else
			case SYS_NET_SO_REUSEADDR:
			{
				native_opt = SO_REUSEADDR;
				break;
			}
			case SYS_NET_SO_REUSEPORT:
			{
				native_opt = SO_REUSEPORT;
				break;
			}
#endif
			case SYS_NET_SO_SNDTIMEO:
			case SYS_NET_SO_RCVTIMEO:
			{
				if (len < sizeof(sys_net_timeval))
					return SYS_NET_EINVAL;

				native_opt = optname == SYS_NET_SO_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
				break;
			}
			case SYS_NET_SO_LINGER:
			{
				if (len < sizeof(sys_net_linger))
					return SYS_NET_EINVAL;

				native_opt = SO_LINGER;
				break;
			}
			default:
			{
				sys_net.error("sys_net_bnet_getsockopt(s=%d, SOL_SOCKET): unknown option (0x%x)", s, optname);
				return SYS_NET_EINVAL;
			}
			}
		}
		else if (level == SYS_NET_IPPROTO_TCP)
		{
			native_level = IPPROTO_TCP;

			switch (optname)
			{
			case SYS_NET_TCP_MAXSEG:
			{
				// Special (no effect)
				out_val._int = sock.so_tcp_maxseg;
				out_len = sizeof(s32);
				return {};
			}
			case SYS_NET_TCP_NODELAY:
			{
				native_opt = TCP_NODELAY;
				break;
			}
			default:
			{
				sys_net.error("sys_net_bnet_getsockopt(s=%d, IPPROTO_TCP): unknown option (0x%x)", s, optname);
				return SYS_NET_EINVAL;
			}
			}
		}
		else
		{
			sys_net.error("sys_net_bnet_getsockopt(s=%d): unknown level (0x%x)", s, level);
			return SYS_NET_EINVAL;
		}

		if (::getsockopt(sock.socket, native_level, native_opt, native_val.ch, &native_len) != 0)
		{
			return get_last_error(false);
		}

		if (level == SYS_NET_SOL_SOCKET)
		{
			switch (optname)
			{
			case SYS_NET_SO_SNDTIMEO:
			case SYS_NET_SO_RCVTIMEO:
			{
				// TODO
				out_val.timeo = { ::narrow<s64>(native_val.timeo.tv_sec), ::narrow<s64>(native_val.timeo.tv_usec) };
				out_len = sizeof(sys_net_timeval);
				return {};
			}
			case SYS_NET_SO_LINGER:
			{
				// TODO
				out_val.linger = { ::narrow<s32>(native_val.linger.l_onoff), ::narrow<s32>(native_val.linger.l_linger) };
				out_len = sizeof(sys_net_linger);
				return {};
			}
			}
		}

		// Fallback to int
		out_val._int = native_val._int;
		out_len = sizeof(s32);
		return {};
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	std::memcpy(optval.get_ptr(), out_val.ch, out_len);
	*optlen = out_len;
	return CELL_OK;
}

error_code sys_net_bnet_listen(ppu_thread& ppu, s32 s, s32 backlog)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_listen(s=%d, backlog=%d)", s, backlog);

	if (backlog <= 0)
	{
		return -SYS_NET_EINVAL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		ASSERT(sock.type == SYS_NET_SOCK_STREAM_P2P || sock.type == SYS_NET_SOCK_STREAM);

		if (sock.type == SYS_NET_SOCK_STREAM_P2P)
		{
			sock.p2ps.status = lv2_socket::p2ps_i::stream_status::stream_listening;
			sock.p2ps.max_backlog = backlog;
			return {};
		}
		else if (::listen(sock.socket, backlog) == 0)
		{
			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	return CELL_OK;
}

error_code sys_net_bnet_recvfrom(ppu_thread& ppu, s32 s, vm::ptr<void> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_recvfrom(s=%d, buf=*0x%x, len=%u, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);

	// If addr is null, paddrlen must be null as well
	if (!buf || !len || addr.operator bool() != paddrlen.operator bool())
	{
		return -SYS_NET_EINVAL;
	}

	if (flags & ~(SYS_NET_MSG_PEEK | SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL))
	{
		fmt::throw_exception("sys_net_bnet_recvfrom(s=%d): unknown flags (0x%x)", flags);
	}

	int native_flags = 0;
	int native_result = -1;
	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);
	sys_net_error result{};
	std::vector<u8> _buf;

	if (flags & SYS_NET_MSG_PEEK)
	{
		native_flags |= MSG_PEEK;
	}

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	s32 type = 0;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		type = sock.type;
		std::lock_guard lock(sock.mutex);

		//if (!(sock.events & lv2_socket::poll::read))
		{
			const auto nph = g_fxo->get<named_thread<np_handler>>();
			if (nph->is_dns(s) && nph->is_dns_queue(s))
			{
				const auto packet = nph->get_dns_packet(s);
				ASSERT(packet.size() < len);

				memcpy(buf.get_ptr(), packet.data(), packet.size());
				native_result = ::narrow<int>(packet.size());

				native_addr.ss_family = AF_INET;
				(reinterpret_cast<::sockaddr_in*>(&native_addr))->sin_port = std::bit_cast<u16, be_t<u16>>(53); // htons(53)
				(reinterpret_cast<::sockaddr_in*>(&native_addr))->sin_addr.s_addr = nph->get_dns_ip();

				return true;
			}

			if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
			{
				sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", sock.p2p.vport, sock.p2p.data.size());

				if (sock.p2p.data.empty())
				{
					result = SYS_NET_EWOULDBLOCK;
					return false;
				}

				const auto& p2p_data = sock.p2p.data.front();
				native_result = std::min(len, static_cast<u32>(p2p_data.second.size()));
				memcpy(buf.get_ptr(), p2p_data.second.data(), native_result);

				if (addr)
				{
					*paddrlen = sizeof(sys_net_sockaddr_in);
					memcpy(addr.get_ptr(), &p2p_data.first, addr.size());
				}

				sock.p2p.data.pop();

				return true;
			}

			if (sock.type == SYS_NET_SOCK_STREAM_P2P)
			{
				if (!sock.p2ps.data_available)
				{
					result = SYS_NET_EWOULDBLOCK;
					return false;
				}

				const u32 to_give = std::min(sock.p2ps.data_available, len);
				sys_net.trace("STREAM-P2P socket had %d available, given %d", sock.p2ps.data_available, to_give);

				u32 left_to_give = to_give;
				while (left_to_give)
				{
					auto& cur_data = sock.p2ps.received_data.begin()->second;
					auto to_give_for_this_packet = std::min(static_cast<u32>(cur_data.size()), left_to_give);
					memcpy(reinterpret_cast<u8 *>(buf.get_ptr()) + (to_give - left_to_give), cur_data.data(), to_give_for_this_packet);
					if (cur_data.size() != to_give_for_this_packet)
					{
						auto amount_left = cur_data.size() - to_give_for_this_packet;
						std::vector<u8> new_vec(amount_left);
						memcpy(new_vec.data(), cur_data.data() + to_give_for_this_packet, amount_left);
						auto new_key = (sock.p2ps.received_data.begin()->first) + to_give_for_this_packet;
						sock.p2ps.received_data.emplace(new_key, std::move(new_vec));
					}

					sock.p2ps.received_data.erase(sock.p2ps.received_data.begin());

					left_to_give -= to_give_for_this_packet;
				}

				sock.p2ps.data_available -= to_give;
				sock.p2ps.data_beg_seq += to_give;
				native_result = to_give;

				return true;
			}

			native_result = ::recvfrom(sock.socket, reinterpret_cast<char*>(buf.get_ptr()), len, native_flags, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

			if (native_result >= 0)
			{
				if (sys_net.enabled == logs::level::trace)
				{
					std::string datrace;
					const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

					u8* dabuf = static_cast<u8 *>(buf.get_ptr());

					for (s32 index = 0; index < native_result; index++)
					{
						if ((index % 16) == 0)
							datrace += '\n';

						datrace += hex[(dabuf[index] >> 4) & 15];
						datrace += hex[(dabuf[index]) & 15];
						datrace += ' ';
					}
					sys_net.trace("DNS RESULT: %s", datrace);
				}

				return true;
			}

			result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);

			if (result)
			{
				return false;
			}
		}

		// Enable read event
		sock.events += lv2_socket::poll::read;
		_buf.resize(len);
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (events & lv2_socket::poll::read)
			{
				native_result = ::recvfrom(sock.socket, reinterpret_cast<char *>(_buf.data()), len, native_flags, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

				if (native_result >= 0 || (result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0)))
				{
					lv2_obj::awake(&ppu);
					return true;
				}
			}

			sock.events += lv2_socket::poll::read;
			return false;
		});

		lv2_obj::sleep(ppu);
		return false;
	});

	if (!sock)
	{
		if (type == SYS_NET_SOCK_DGRAM_P2P)
			sys_net.error("[P2P] Error recvfrom(bad socket)");
		return -SYS_NET_EBADF;
	}

	if (!sock.ret && result)
	{
		if (result == SYS_NET_EWOULDBLOCK)
		{
			return not_an_error(-result);
		}

		if (type == SYS_NET_SOCK_DGRAM_P2P)
			sys_net.error("[P2P] Error recvfrom(result early): %d", result);

		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			if (ppu.is_stopped())
			{
				return 0;
			}

			thread_ctrl::wait();
		}

		if (result)
		{
			if (type == SYS_NET_SOCK_DGRAM_P2P)
				sys_net.error("[P2P] Error recvfrom(result): %d", result);
			return -result;
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			if (type == SYS_NET_SOCK_DGRAM_P2P)
				sys_net.error("[P2P] Error recvfrom(interrupted)");
			return -SYS_NET_EINTR;
		}

		std::memcpy(buf.get_ptr(), _buf.data(), len);
	}

	if (ppu.is_stopped())
	{
		return 0;
	}

	// addr is set earlier for P2P socket
	if (addr && type != SYS_NET_SOCK_DGRAM_P2P && type != SYS_NET_SOCK_STREAM_P2P)
	{
		verify(HERE), native_addr.ss_family == AF_INET;

		vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

		*paddrlen         = sizeof(sys_net_sockaddr_in);

		paddr->sin_len    = sizeof(sys_net_sockaddr_in);
		paddr->sin_family = SYS_NET_AF_INET;
		paddr->sin_port   = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_port);
		paddr->sin_addr   = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<struct sockaddr_in*>(&native_addr)->sin_addr.s_addr);
		paddr->sin_zero   = 0;
	}

	// Length
	if (type == SYS_NET_SOCK_DGRAM_P2P || type == SYS_NET_SOCK_STREAM_P2P)
		sys_net.trace("[P2P] %s Ok recvfrom: %d", type, native_result);

	return not_an_error(native_result);
}

error_code sys_net_bnet_recvmsg(ppu_thread& ppu, s32 s, vm::ptr<sys_net_msghdr> msg, s32 flags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_bnet_recvmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);
	return CELL_OK;
}

error_code sys_net_bnet_sendmsg(ppu_thread& ppu, s32 s, vm::cptr<sys_net_msghdr> msg, s32 flags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_bnet_sendmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);
	return CELL_OK;
}

error_code sys_net_bnet_sendto(ppu_thread& ppu, s32 s, vm::cptr<void> buf, u32 len, s32 flags, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_sendto(s=%d, buf=*0x%x, len=%u, flags=0x%x, addr=*0x%x, addrlen=%u)", s, buf, len, flags, addr, addrlen);

	if (flags & ~(SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL))
	{
		fmt::throw_exception("sys_net_bnet_sendto(s=%d): unknown flags (0x%x)", flags);
	}

	if (addr && addrlen < 8)
	{
		sys_net.error("sys_net_bnet_sendto(s=%d): bad addrlen (%u)", s, addrlen);
		return -SYS_NET_EINVAL;
	}

	if (addr && addr->sa_family != SYS_NET_AF_INET)
	{
		sys_net.error("sys_net_bnet_sendto(s=%d): unsupported sa_family (%d)", s, addr->sa_family);
		return -SYS_NET_EAFNOSUPPORT;
	}

	const auto psa_in = vm::_ptr<const sys_net_sockaddr_in>(addr.addr());

	int native_flags = 0;
	int native_result = -1;
	::sockaddr_in name{};
	std::vector<u8> _buf;

	if (idm::check<lv2_socket>(s))
	{
		_buf.assign(vm::_ptr<const char>(buf.addr()), vm::_ptr<const char>(buf.addr()) + len);
	}
	else
	{
		return -SYS_NET_EBADF;
	}

	if (addr)
	{
		name.sin_family      = AF_INET;
		name.sin_port        = std::bit_cast<u16>(psa_in->sin_port);
		name.sin_addr.s_addr = std::bit_cast<u32>(psa_in->sin_addr);

		sys_net.trace("Sending to %s:%d", inet_ntoa(name.sin_addr), psa_in->sin_port);
	}

	::socklen_t namelen = sizeof(name);
	sys_net_error result{};

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	std::vector<u8> p2p_data;
	char *data = reinterpret_cast<char *>(_buf.data());
	u32 data_len = len;

	s32 type        = 0;
	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		std::lock_guard lock(sock.mutex);
		type = sock.type;

		if (type == SYS_NET_SOCK_DGRAM_P2P)
		{
			ASSERT(addr);
			ASSERT(sock.socket); // ensures it has been bound
			ASSERT(len <= (65535 - sizeof(u16))); // catch games using full payload for future fragmentation implementation if necessary
			const u16 p2p_port  = reinterpret_cast<const sys_net_sockaddr_in*>(addr.get_ptr())->sin_port;
			const u16 p2p_vport = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(addr.get_ptr())->sin_vport;

			sys_net.trace("[P2P] Sending a packet to %s:%d:%d", inet_ntoa(name.sin_addr), p2p_port, p2p_vport);

			p2p_data.resize(len + sizeof(u16));
			reinterpret_cast<le_t<u16>&>(p2p_data[0]) = p2p_vport;
			memcpy(p2p_data.data()+sizeof(u16), _buf.data(), len);

			data = reinterpret_cast<char *>(p2p_data.data());
			data_len = len + sizeof(u16);
		}
		else if (type == SYS_NET_SOCK_STREAM_P2P)
		{
			constexpr s64 max_data_len = (65535 - (sizeof(u16) + sizeof(lv2_socket::p2ps_i::encapsulated_tcp)));

			// Prepare address
			name.sin_family = AF_INET;
			name.sin_port        = std::bit_cast<u16, be_t<u16>>(sock.p2ps.op_port);
			name.sin_addr.s_addr = sock.p2ps.op_addr;
			// Prepares encapsulated tcp
			lv2_socket::p2ps_i::encapsulated_tcp tcp_header;
			tcp_header.src_port = sock.p2p.vport;
			tcp_header.dst_port = sock.p2ps.op_vport;
			// chop it up
			std::vector<std::vector<u8>> stream_packets;
			s64 cur_total_len = len;
			while(cur_total_len > 0)
			{
				s64 cur_data_len;
				if (cur_total_len >= max_data_len)
					cur_data_len = max_data_len;
				else
					cur_data_len = cur_total_len;

				tcp_header.length = cur_data_len;
				tcp_header.seq = sock.p2ps.cur_seq;

				auto packet       = nt_p2p_port::generate_u2s_packet(tcp_header, &_buf[len - cur_total_len], cur_data_len);
				nt_p2p_port::send_u2s_packet(sock, s, std::move(packet), &name, tcp_header.seq);

				cur_total_len -= cur_data_len;
				sock.p2ps.cur_seq += cur_data_len;
			}
	
			native_result = len;
			return true;
		}

		//if (!(sock.events & lv2_socket::poll::write))
		{
			const auto nph = g_fxo->get<named_thread<np_handler>>();
			if (addr && type == SYS_NET_SOCK_DGRAM && psa_in->sin_port == 53)
			{
				nph->add_dns_spy(s);
			}

			if (nph->is_dns(s))
			{
				const s32 ret_analyzer = nph->analyze_dns_packet(s, reinterpret_cast<const u8*>(_buf.data()), len);
				// Check if the packet is intercepted
				if (ret_analyzer >= 0)
				{
					native_result = ret_analyzer;
					return true;
				}
			}

			native_result = ::sendto(sock.socket, data, data_len, native_flags, addr ? reinterpret_cast<struct sockaddr*>(&name) : nullptr, addr ? namelen : 0);

			if (native_result >= 0)
			{
				if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
				{
					native_result -= sizeof(u16);
				}
				return true;
			}

			result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);

			if (result)
			{
				return false;
			}
		}

		// Enable write event
		sock.events += lv2_socket::poll::write;
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (events & lv2_socket::poll::write)
			{
				native_result = ::sendto(sock.socket, data, data_len, native_flags, addr ? reinterpret_cast<struct sockaddr*>(&name) : nullptr, addr ? namelen : 0);

				if (native_result >= 0 || (result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0)))
				{
					lv2_obj::awake(&ppu);
					return true;
				}
			}

			sock.events += lv2_socket::poll::write;
			return false;
		});

		lv2_obj::sleep(ppu);
		return false;
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret && result)
	{
		if (result == SYS_NET_EWOULDBLOCK)
		{
			return not_an_error(-result);
		}
		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			if (ppu.is_stopped())
			{
				return 0;
			}

			thread_ctrl::wait();
		}

		if (result)
		{
			return -result;
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	return not_an_error(native_result);
}

error_code sys_net_bnet_setsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=%u)", s, level, optname, optval, optlen);
	switch(optlen)
	{
		case 1:
			sys_net.warning("optval: 0x%02X", *reinterpret_cast<const u8 *>(optval.get_ptr()));
			break;
		case 2:
			sys_net.warning("optval: 0x%04X", *reinterpret_cast<const be_t<u16> *>(optval.get_ptr()));
			break;
		case 4:
			sys_net.warning("optval: 0x%08X", *reinterpret_cast<const be_t<u32> *>(optval.get_ptr()));
			break;
		case 8:
			sys_net.warning("optval: 0x%016X", *reinterpret_cast<const be_t<u64> *>(optval.get_ptr()));
			break;
	}

	int native_int = 0;
	int native_level = -1;
	int native_opt = -1;
	const void* native_val = &native_int;
	::socklen_t native_len = sizeof(int);
	::linger native_linger;

#ifdef _WIN32
	u32 native_timeo;
#else
	::timeval native_timeo;
#endif

	std::vector<char> optval_buf(vm::_ptr<char>(optval.addr()), vm::_ptr<char>(optval.addr() + optlen));

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		if (sock.type == SYS_NET_SOCK_DGRAM_P2P || sock.type == SYS_NET_SOCK_STREAM_P2P)
		{
			return {};
		}		

		if (optlen >= sizeof(s32))
		{
			std::memcpy(&native_int, optval_buf.data(), sizeof(s32));
		}
		else
		{
			return SYS_NET_EINVAL;
		}

		if (level == SYS_NET_SOL_SOCKET)
		{
			native_level = SOL_SOCKET;

			switch (optname)
			{
			case SYS_NET_SO_NBIO:
			{
				// Special
				sock.so_nbio = native_int;
				return {};
			}
			case SYS_NET_SO_KEEPALIVE:
			{
				native_opt = SO_KEEPALIVE;
				break;
			}
			case SYS_NET_SO_SNDBUF:
			{
				native_opt = SO_SNDBUF;
				break;
			}
			case SYS_NET_SO_RCVBUF:
			{
				native_opt = SO_RCVBUF;
				break;
			}
			case SYS_NET_SO_SNDLOWAT:
			{
				native_opt = SO_SNDLOWAT;
				break;
			}
			case SYS_NET_SO_RCVLOWAT:
			{
				native_opt = SO_RCVLOWAT;
				break;
			}
			case SYS_NET_SO_BROADCAST:
			{
				native_opt = SO_BROADCAST;
				break;
			}
#ifdef _WIN32
			case SYS_NET_SO_REUSEADDR:
			{
				native_opt = SO_REUSEADDR;
				sock.so_reuseaddr = native_int;
				native_int = sock.so_reuseaddr || sock.so_reuseport ? 1 : 0;
				break;
			}
			case SYS_NET_SO_REUSEPORT:
			{
				native_opt = SO_REUSEADDR;
				sock.so_reuseport = native_int;
				native_int = sock.so_reuseaddr || sock.so_reuseport ? 1 : 0;
				break;
			}
#else
			case SYS_NET_SO_REUSEADDR:
			{
				native_opt = SO_REUSEADDR;
				break;
			}
			case SYS_NET_SO_REUSEPORT:
			{
				native_opt = SO_REUSEPORT;
				break;
			}
#endif
			case SYS_NET_SO_SNDTIMEO:
			case SYS_NET_SO_RCVTIMEO:
			{
				if (optlen < sizeof(sys_net_timeval))
					return SYS_NET_EINVAL;

				native_opt = optname == SYS_NET_SO_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
				native_val = &native_timeo;
				native_len = sizeof(native_timeo);
#ifdef _WIN32
				native_timeo = ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_sec) * 1000;
				native_timeo += ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_usec) / 1000;
#else
				native_timeo.tv_sec = ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_sec);
				native_timeo.tv_usec = ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_usec);
#endif 
				break;
			}
			case SYS_NET_SO_LINGER:
			{
				if (optlen < sizeof(sys_net_linger))
					return SYS_NET_EINVAL;

				// TODO
				native_opt = SO_LINGER;
				native_val = &native_linger;
				native_len = sizeof(native_linger);
				native_linger.l_onoff = reinterpret_cast<sys_net_linger*>(optval_buf.data())->l_onoff;
				native_linger.l_linger = reinterpret_cast<sys_net_linger*>(optval_buf.data())->l_linger;
				break;
			}
			case SYS_NET_SO_USECRYPTO:
			{
				//TODO
				sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USECRYPTO)", s, optname);
				return {};
			}
			case SYS_NET_SO_USESIGNATURE:
			{
				//TODO
				sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USESIGNATURE)", s, optname);
				return {};
			}
			default:
			{
				sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): unknown option (0x%x)", s, optname);
				return SYS_NET_EINVAL;
			}
			}
		}
		else if (level == SYS_NET_IPPROTO_TCP)
		{
			native_level = IPPROTO_TCP;

			switch (optname)
			{
			case SYS_NET_TCP_MAXSEG:
			{
				// Special (no effect)
				sock.so_tcp_maxseg = native_int;
				return {};
			}
			case SYS_NET_TCP_NODELAY:
			{
				native_opt = TCP_NODELAY;
				break;
			}
			default:
			{
				sys_net.error("sys_net_bnet_setsockopt(s=%d, IPPROTO_TCP): unknown option (0x%x)", s, optname);
				return SYS_NET_EINVAL;
			}
			}
		}
		else
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d): unknown level (0x%x)", s, level);
			return SYS_NET_EINVAL;
		}

		if (::setsockopt(sock.socket, native_level, native_opt, reinterpret_cast<const char*>(native_val), native_len) == 0)
		{
			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	return CELL_OK;
}

error_code sys_net_bnet_shutdown(ppu_thread& ppu, s32 s, s32 how)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_shutdown(s=%d, how=%d)", s, how);

	if (how < 0 || how > 2)
	{
		return -SYS_NET_EINVAL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		// Shutdown of P2P socket is always successful
		if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
		{
			return {};
		}

#ifdef _WIN32
		const int native_how =
			how == SYS_NET_SHUT_RD ? SD_RECEIVE :
			how == SYS_NET_SHUT_WR ? SD_SEND : SD_BOTH;
#else
		const int native_how =
			how == SYS_NET_SHUT_RD ? SHUT_RD :
			how == SYS_NET_SHUT_WR ? SHUT_WR : SHUT_RDWR;
#endif

		if (::shutdown(sock.socket, native_how) == 0)
		{
			return {};
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return -sock.ret;
	}

	return CELL_OK;
}

error_code sys_net_bnet_socket(ppu_thread& ppu, s32 family, s32 type, s32 protocol)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_socket(family=%d, type=%d, protocol=%d)", family, type, protocol);

	if (family != SYS_NET_AF_INET && family != SYS_NET_AF_UNSPEC)
	{
		sys_net.error("sys_net_bnet_socket(): unknown family (%d)", family);
	}

	if (type != SYS_NET_SOCK_STREAM && type != SYS_NET_SOCK_DGRAM && type != SYS_NET_SOCK_DGRAM_P2P && type != SYS_NET_SOCK_STREAM_P2P)
	{
		sys_net.error("sys_net_bnet_socket(): unsupported type (%d)", type);
		return -SYS_NET_EPROTONOSUPPORT;
	}

	lv2_socket::socket_type native_socket = 0;

	if (type != SYS_NET_SOCK_DGRAM_P2P && type != SYS_NET_SOCK_STREAM_P2P)
	{
		const int native_domain = AF_INET;

		const int native_type =
			type == SYS_NET_SOCK_STREAM ? SOCK_STREAM :
			type == SYS_NET_SOCK_DGRAM ? SOCK_DGRAM : SOCK_RAW;

		int native_proto =
			protocol == SYS_NET_IPPROTO_IP ? IPPROTO_IP :
			protocol == SYS_NET_IPPROTO_ICMP ? IPPROTO_ICMP :
			protocol == SYS_NET_IPPROTO_IGMP ? IPPROTO_IGMP :
			protocol == SYS_NET_IPPROTO_TCP ? IPPROTO_TCP :
			protocol == SYS_NET_IPPROTO_UDP ? IPPROTO_UDP :
			protocol == SYS_NET_IPPROTO_ICMPV6 ? IPPROTO_ICMPV6 : 0;

		if (native_domain == AF_UNSPEC && type == SYS_NET_SOCK_DGRAM)
		{
			// Windows gets all errory if you try a unspec socket with protocol 0
			native_proto = IPPROTO_UDP;
		}

		native_socket = ::socket(native_domain, native_type, native_proto);

		if (native_socket == -1)
		{
			return -get_last_error(false);
		}
	}

	auto sock_lv2 = std::make_shared<lv2_socket>(native_socket, type, family);
	if (type == SYS_NET_SOCK_STREAM_P2P)
	{
		sock_lv2->p2p.port = 3658; // Default value if unspecified later
	}

	const s32 s = idm::import_existing<lv2_socket>(sock_lv2);

	if (s == id_manager::id_traits<lv2_socket>::invalid)
	{
		return -SYS_NET_EMFILE;
	}

	return not_an_error(s);
}

error_code sys_net_bnet_close(ppu_thread& ppu, s32 s)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_close(s=%d)", s);

	const auto sock = idm::withdraw<lv2_socket>(s);

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock->queue.empty())
		sys_net.error("CLOSE");

	// If it's a bound socket we "close" the vport
	if (sock->type == SYS_NET_SOCK_DGRAM_P2P && sock->p2p.port && sock->p2p.vport)
	{
		const auto nc = g_fxo->get<network_context>();
		{
			std::lock_guard lock(nc->list_p2p_ports_mutex);
			auto& p2p_port = nc->list_p2p_ports.at(sock->p2p.port);
			{
				std::lock_guard lock(p2p_port.bound_p2p_vports_mutex);
				p2p_port.bound_p2p_vports.erase(sock->p2p.vport);
			}
		}
	}

	const auto nph = g_fxo->get<named_thread<np_handler>>();
	nph->remove_dns_spy(s);

	return CELL_OK;
}

error_code sys_net_bnet_poll(ppu_thread& ppu, vm::ptr<sys_net_pollfd> fds, s32 nfds, s32 ms)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_poll(fds=*0x%x, nfds=%d, ms=%d)", fds, nfds, ms);

	if (nfds <= 0)
	{
		return not_an_error(0);
	}

	atomic_t<s32> signaled{0};

	u64 timeout = ms < 0 ? 0 : ms * 1000ull;

	std::vector<sys_net_pollfd> fds_buf;

	if (true)
	{
		fds_buf.assign(fds.get_ptr(), fds.get_ptr() + nfds);

		std::unique_lock nw_lock(g_fxo->get<network_context>()->s_nw_mutex);

		std::shared_lock lock(id_manager::g_mutex);

		::pollfd _fds[1024]{};
#ifdef _WIN32
		bool connecting[1024]{};
#endif

		for (s32 i = 0; i < nfds; i++)
		{
			_fds[i].fd = -1;
			fds_buf[i].revents = 0;

			if (fds_buf[i].fd < 0)
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>(fds_buf[i].fd))
			{
				if (sock->type == SYS_NET_SOCK_DGRAM_P2P)
				{
					std::lock_guard lock(sock->mutex);
					ASSERT(sock->p2p.vport);
					sys_net.trace("[P2P] poll checking for 0x%X", fds[i].events);
					// Check if it's a bound P2P socket
					if ((fds[i].events & SYS_NET_POLLIN) && !sock->p2p.data.empty())
					{
						sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", sock->p2p.vport, sock->p2p.data.size());
						fds_buf[i].revents |= SYS_NET_POLLIN;
					}

					if (fds[i].events & SYS_NET_POLLOUT)
						fds_buf[i].revents |= SYS_NET_POLLOUT;

					if (fds_buf[i].revents)
						signaled++;
				}
				else if (sock->type == SYS_NET_SOCK_STREAM_P2P)
				{
					std::lock_guard lock(sock->mutex);
					sys_net.trace("[P2PS] poll checking for 0x%X", fds[i].events);
					if (sock->p2ps.status == lv2_socket::p2ps_i::stream_status::stream_connected)
					{
						if ((fds[i].events & SYS_NET_POLLIN) && sock->p2ps.data_available)
						{
							sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", sock->p2p.vport, sock->p2p.data.size());
							fds_buf[i].revents |= SYS_NET_POLLIN;
						}

						if (fds[i].events & SYS_NET_POLLOUT)
							fds_buf[i].revents |= SYS_NET_POLLOUT;

						if (fds_buf[i].revents)
							signaled++;
					}
				}
				else 
				{
					// Check for fake packet for dns interceptions
					const auto nph = g_fxo->get<named_thread<np_handler>>();
					if (fds_buf[i].events & SYS_NET_POLLIN && nph->is_dns(fds_buf[i].fd) && nph->is_dns_queue(fds_buf[i].fd))
						fds_buf[i].revents |= SYS_NET_POLLIN;

					if (fds_buf[i].events & ~(SYS_NET_POLLIN | SYS_NET_POLLOUT | SYS_NET_POLLERR))
						sys_net.warning("sys_net_bnet_poll(fd=%d): events=0x%x", fds[i].fd, fds[i].events);
					_fds[i].fd = sock->socket;
					if (fds_buf[i].events & SYS_NET_POLLIN)
						_fds[i].events |= POLLIN;
					if (fds_buf[i].events & SYS_NET_POLLOUT)
						_fds[i].events |= POLLOUT;
				}
#ifdef _WIN32
				connecting[i] = sock->is_connecting;
#endif
			}
			else
			{
				fds_buf[i].revents |= SYS_NET_POLLNVAL;
				signaled++;
			}
		}

#ifdef _WIN32
		windows_poll(_fds, nfds, 0, connecting);
#else
		::poll(_fds, nfds, 0);
#endif
		for (s32 i = 0; i < nfds; i++)
		{
			if (_fds[i].revents & (POLLIN | POLLHUP))
				fds_buf[i].revents |= SYS_NET_POLLIN;
			if (_fds[i].revents & POLLOUT)
				fds_buf[i].revents |= SYS_NET_POLLOUT;
			if (_fds[i].revents & POLLERR)
				fds_buf[i].revents |= SYS_NET_POLLERR;

			if (fds_buf[i].revents)
			{
				signaled++;
			}
		}

		if (ms == 0 || signaled)
		{
			lock.unlock();
			nw_lock.unlock();
			std::memcpy(fds.get_ptr(), fds_buf.data(), nfds * sizeof(fds[0]));
			return not_an_error(signaled);
		}

		for (s32 i = 0; i < nfds; i++)
		{
			if (fds_buf[i].fd < 0)
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>(fds_buf[i].fd))
			{
				std::lock_guard lock(sock->mutex);

#ifdef _WIN32
				sock->is_connecting = connecting[i];
#endif

				bs_t<lv2_socket::poll> selected = +lv2_socket::poll::error;

				if (fds_buf[i].events & SYS_NET_POLLIN)
					selected += lv2_socket::poll::read;
				if (fds_buf[i].events & SYS_NET_POLLOUT)
					selected += lv2_socket::poll::write;
				//if (fds_buf[i].events & SYS_NET_POLLPRI) // Unimplemented
				//	selected += lv2_socket::poll::error;

				sock->events += selected;
				sock->queue.emplace_back(ppu.id, [sock, selected, &fds_buf, i, &signaled, &ppu](bs_t<lv2_socket::poll> events)
				{
					if (events & selected)
					{
						if (events & selected & lv2_socket::poll::read)
							fds_buf[i].revents |= SYS_NET_POLLIN;
						if (events & selected & lv2_socket::poll::write)
							fds_buf[i].revents |= SYS_NET_POLLOUT;
						if (events & selected & lv2_socket::poll::error)
							fds_buf[i].revents |= SYS_NET_POLLERR;

						signaled++;
						g_fxo->get<network_context>()->s_to_awake.emplace_back(&ppu);
						return true;
					}

					sock->events += selected;
					return false;
				});
			}
		}

		lv2_obj::sleep(ppu, timeout);
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return 0;
				}

				std::lock_guard nw_lock(g_fxo->get<network_context>()->s_nw_mutex);

				if (signaled)
				{
					break;
				}

				network_clear_queue(ppu);
				break;
			}
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	std::memcpy(fds.get_ptr(), fds_buf.data(), nfds * sizeof(fds[0]));
	return not_an_error(signaled);
}

error_code sys_net_bnet_select(ppu_thread& ppu, s32 nfds, vm::ptr<sys_net_fd_set> readfds, vm::ptr<sys_net_fd_set> writefds, vm::ptr<sys_net_fd_set> exceptfds, vm::ptr<sys_net_timeval> _timeout)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_select(nfds=%d, readfds=*0x%x, writefds=*0x%x, exceptfds=*0x%x, timeout=*0x%x)", nfds, readfds, writefds, exceptfds, _timeout);

	atomic_t<s32> signaled{0};

	if (exceptfds)
	{
		sys_net.error("sys_net_bnet_select(): exceptfds not implemented");
	}

	sys_net_fd_set rread{}, _readfds{};
	sys_net_fd_set rwrite{}, _writefds{};
	sys_net_fd_set rexcept{}, _exceptfds{};
	u64 timeout = !_timeout ? 0 : _timeout->tv_sec * 1000000ull + _timeout->tv_usec;

	if (nfds > 0 && nfds <= 1024)
	{
		if (readfds)
			_readfds = *readfds;
		if (writefds)
			_writefds = *writefds;
		if (exceptfds)
			_exceptfds = *exceptfds;

		std::lock_guard nw_lock(g_fxo->get<network_context>()->s_nw_mutex);

		reader_lock lock(id_manager::g_mutex);

		::pollfd _fds[1024]{};
#ifdef _WIN32
		bool connecting[1024]{};
#endif

		for (s32 i = 0; i < nfds; i++)
		{
			_fds[i].fd = -1;
			bs_t<lv2_socket::poll> selected{};

			if (readfds && _readfds.bit(i))
				selected += lv2_socket::poll::read;
			if (writefds && _writefds.bit(i))
				selected += lv2_socket::poll::write;
			//if (exceptfds && _exceptfds.bit(i))
			//	selected += lv2_socket::poll::error;

			if (selected)
			{
				selected += lv2_socket::poll::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{

				if (sock->type != SYS_NET_SOCK_DGRAM_P2P)
				{
					_fds[i].fd = sock->socket;
					if (selected & lv2_socket::poll::read)
						_fds[i].events |= POLLIN;
					if (selected & lv2_socket::poll::write)
						_fds[i].events |= POLLOUT;
				}
				else
				{
					std::lock_guard lock(sock->mutex);
					// Check if it's a bound P2P socket
					if ((selected & lv2_socket::poll::read) && sock->p2p.vport && !sock->p2p.data.empty())
					{
						sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", sock->p2p.vport, sock->p2p.data.size());
						rread.set(i);
						signaled++;
					}
				}
#ifdef _WIN32
				connecting[i] = sock->is_connecting;
#endif
			}
			else
			{
				return -SYS_NET_EBADF;
			}
		}

#ifdef _WIN32
		windows_poll(_fds, nfds, 0, connecting);
#else
		::poll(_fds, nfds, 0);
#endif
		for (s32 i = 0; i < nfds; i++)
		{
			bool sig = false;
			if (_fds[i].revents & (POLLIN | POLLHUP | POLLERR))
				sig = true, rread.set(i);
			if (_fds[i].revents & (POLLOUT | POLLERR))
				sig = true, rwrite.set(i);

			if (sig)
			{
				signaled++;
			}
		}

		if ((_timeout && !timeout) || signaled)
		{
			if (readfds)
				*readfds = rread;
			if (writefds)
				*writefds = rwrite;
			if (exceptfds)
				*exceptfds = rexcept;
			return not_an_error(signaled);
		}

		for (s32 i = 0; i < nfds; i++)
		{
			bs_t<lv2_socket::poll> selected{};

			if (readfds && _readfds.bit(i))
				selected += lv2_socket::poll::read;
			if (writefds && _writefds.bit(i))
				selected += lv2_socket::poll::write;
			//if (exceptfds && _exceptfds.bit(i))
			//	selected += lv2_socket::poll::error;

			if (selected)
			{
				selected += lv2_socket::poll::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{
				std::lock_guard lock(sock->mutex);

#ifdef _WIN32
				sock->is_connecting = connecting[i];
#endif

				sock->events += selected;
				sock->queue.emplace_back(ppu.id, [sock, selected, i, &rread, &rwrite, &rexcept, &signaled, &ppu](bs_t<lv2_socket::poll> events)
				{
					if (events & selected)
					{
						if (selected & lv2_socket::poll::read && events & (lv2_socket::poll::read + lv2_socket::poll::error))
							rread.set(i);
						if (selected & lv2_socket::poll::write && events & (lv2_socket::poll::write + lv2_socket::poll::error))
							rwrite.set(i);
						//if (events & (selected & lv2_socket::poll::error))
						//	rexcept.set(i);

						signaled++;
						g_fxo->get<network_context>()->s_to_awake.emplace_back(&ppu);
						return true;
					}

					sock->events += selected;
					return false;
				});
			}
			else
			{
				return -SYS_NET_EBADF;
			}
		}

		lv2_obj::sleep(ppu, timeout);
	}
	else
	{
		return -SYS_NET_EINVAL;
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return 0;
				}

				std::lock_guard nw_lock(g_fxo->get<network_context>()->s_nw_mutex);

				if (signaled)
				{
					break;
				}

				network_clear_queue(ppu);
				break;
			}
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	if (ppu.is_stopped())
	{
		return 0;
	}

	if (readfds)
		*readfds = rread;
	if (writefds)
		*writefds = rwrite;
	if (exceptfds)
		*exceptfds = rexcept;

	return not_an_error(signaled);
}

error_code _sys_net_open_dump(ppu_thread& ppu, s32 len, s32 flags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("_sys_net_open_dump(len=%d, flags=0x%x)", len, flags);
	return CELL_OK;
}

error_code _sys_net_read_dump(ppu_thread& ppu, s32 id, vm::ptr<void> buf, s32 len, vm::ptr<s32> pflags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("_sys_net_read_dump(id=0x%x, buf=*0x%x, len=%d, pflags=*0x%x)", id, buf, len, pflags);
	return CELL_OK;
}

error_code _sys_net_close_dump(ppu_thread& ppu, s32 id, vm::ptr<s32> pflags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("_sys_net_close_dump(id=0x%x, pflags=*0x%x)", id, pflags);
	return CELL_OK;
}

error_code _sys_net_write_dump(ppu_thread& ppu, s32 id, vm::cptr<void> buf, s32 len, u32 unknown)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo(__func__);
	return CELL_OK;
}

error_code sys_net_abort(ppu_thread& ppu, s32 type, u64 arg, s32 flags)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_abort(type=%d, arg=0x%x, flags=0x%x)", type, arg, flags);
	return CELL_OK;
}

struct net_infoctl_cmd_9_t
{
	be_t<u32> zero;
	vm::bptr<char> server_name;
	// More (TODO)
};

error_code sys_net_infoctl(ppu_thread& ppu, s32 cmd, vm::ptr<void> arg)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_infoctl(cmd=%d, arg=*0x%x)", cmd, arg);

	// TODO
	switch (cmd)
	{
	case 9:
	{
		constexpr auto nameserver = "nameserver \0"sv;
		constexpr auto default_ip = "192.168.1.1\0"sv;

		char buffer[nameserver.size() + 80];
		std::memcpy(buffer, nameserver.data(), nameserver.size());

		const auto nph = g_fxo->get<named_thread<np_handler>>();
		if (nph->get_dns_ip())
		{
			struct sockaddr_in serv;
			std::memset(&serv, 0, sizeof(serv));
			serv.sin_addr.s_addr = nph->get_dns_ip();
			inet_ntop(AF_INET, &serv.sin_addr, buffer + nameserver.size() - 1, sizeof(buffer) - nameserver.size());
		}
		else
		{
			std::memcpy(buffer + nameserver.size() - 1, default_ip.data(), default_ip.size());
		}

		std::string_view name{buffer};
		vm::static_ptr_cast<net_infoctl_cmd_9_t>(arg)->zero = 0;
		std::memcpy(vm::static_ptr_cast<net_infoctl_cmd_9_t>(arg)->server_name.get_ptr(), name.data(), name.size());
		break;
	}
	default: break;
	}

	return CELL_OK;
}

error_code sys_net_control(ppu_thread& ppu, u32 arg1, s32 arg2, vm::ptr<void> arg3, s32 arg4)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_control(0x%x, %d, *0x%x, %d)", arg1, arg2, arg3, arg4);
	return CELL_OK;
}

error_code sys_net_bnet_ioctl(ppu_thread& ppu, s32 arg1, u32 arg2, u32 arg3)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_bnet_ioctl(%d, 0x%x, 0x%x)", arg1, arg2, arg3);
	return CELL_OK;
}

error_code sys_net_bnet_sysctl(ppu_thread& ppu, u32 arg1, u32 arg2, u32 arg3, vm::ptr<void> arg4, u32 arg5, u32 arg6)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_bnet_sysctl(0x%x, 0x%x, 0x%x, *0x%x, 0x%x, 0x%x)", arg1, arg2, arg3, arg4, arg5, arg6);
	return CELL_OK;
}

error_code sys_net_eurus_post_command(ppu_thread& ppu, s32 arg1, u32 arg2, u32 arg3)
{
	ppu.state += cpu_flag::wait;

	sys_net.todo("sys_net_eurus_post_command(%d, 0x%x, 0x%x)", arg1, arg2, arg3);
	return CELL_OK;
}
