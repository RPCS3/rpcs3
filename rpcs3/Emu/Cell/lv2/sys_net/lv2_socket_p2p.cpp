#include "stdafx.h"
#include "lv2_socket_p2p.h"
#include "Emu/NP/np_helpers.h"
#include "network_context.h"
#include "sys_net_helpers.h"

LOG_CHANNEL(sys_net);

lv2_socket_p2p::lv2_socket_p2p(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
	: lv2_socket(family, type, protocol)
{
	sockopt_cache cache_type;
	cache_type.data._int = SYS_NET_SOCK_DGRAM_P2P;
	cache_type.len = 4;

	sockopts[(static_cast<u64>(SYS_NET_SOL_SOCKET) << 32ull) | SYS_NET_SO_TYPE] = cache_type;
}

lv2_socket_p2p::lv2_socket_p2p(utils::serial& ar, lv2_socket_type type)
	: lv2_socket(stx::make_exact(ar), type)
{
	ar(port, vport, bound_addr);

	auto data_dequeue = ar.pop<std::deque<std::pair<sys_net_sockaddr_in_p2p, std::vector<u8>>>>();

	for (; !data_dequeue.empty(); data_dequeue.pop_front())
	{
		data.push(std::move(data_dequeue.front()));
	}
}

void lv2_socket_p2p::save(utils::serial& ar)
{
	lv2_socket::save(ar, true);
	ar(port, vport, bound_addr);

	std::deque<std::pair<sys_net_sockaddr_in_p2p, std::vector<u8>>> data_dequeue;

	for (auto save_data = ::as_rvalue(data); !save_data.empty(); save_data.pop())
	{
		data_dequeue.push_back(std::move(save_data.front()));
	}

	ar(data_dequeue);
}

void lv2_socket_p2p::handle_new_data(sys_net_sockaddr_in_p2p p2p_addr, std::vector<u8> p2p_data)
{
	std::lock_guard lock(mutex);

	sys_net.trace("Received a P2P packet for vport %d and saved it", p2p_addr.sin_vport);
	data.push(std::make_pair(std::move(p2p_addr), std::move(p2p_data)));

	// Check if poll is happening
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

std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> lv2_socket_p2p::accept([[maybe_unused]] bool is_lock)
{
	sys_net.fatal("[P2P] accept() called on a P2P socket");
	return {};
}

std::optional<s32> lv2_socket_p2p::connect([[maybe_unused]] const sys_net_sockaddr& addr)
{
	sys_net.fatal("[P2P] connect() called on a P2P socket");
	return {};
}

s32 lv2_socket_p2p::connect_followup()
{
	sys_net.fatal("[P2P] connect_followup() called on a P2P socket");
	return {};
}

std::pair<s32, sys_net_sockaddr> lv2_socket_p2p::getpeername()
{
	sys_net.fatal("[P2P] getpeername() called on a P2P socket");
	return {};
}

s32 lv2_socket_p2p::listen([[maybe_unused]] s32 backlog)
{
	sys_net.fatal("[P2P] listen() called on a P2P socket");
	return {};
}

s32 lv2_socket_p2p::bind(const sys_net_sockaddr& addr)
{
	const auto* psa_in_p2p = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(&addr);
	u16 p2p_port           = psa_in_p2p->sin_port;
	u16 p2p_vport          = psa_in_p2p->sin_vport;

	sys_net.notice("[P2P] Trying to bind %s:%d:%d", np::ip_to_string(std::bit_cast<u32>(psa_in_p2p->sin_addr)), p2p_port, p2p_vport);

	if (p2p_port != SCE_NP_PORT)
	{
		if (p2p_port == 0)
		{
			return -SYS_NET_EINVAL;
		}
		sys_net.warning("[P2P] Attempting to bind a socket to a port != %d", +SCE_NP_PORT);
	}

	socket_type real_socket{};

	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);

		nc.create_p2p_port(p2p_port);
		auto& pport = ::at32(nc.list_p2p_ports, p2p_port);
		real_socket = pport.p2p_socket;

		{
			std::lock_guard lock(pport.bound_p2p_vports_mutex);

			if (p2p_vport == 0)
			{
				// Find a free vport starting at 30000
				p2p_vport = 30000;
				while (pport.bound_p2p_vports.contains(p2p_vport))
				{
					p2p_vport++;
				}
			}
			
			if (pport.bound_p2p_vports.contains(p2p_vport))
			{
				// Check that all other sockets are SO_REUSEADDR or SO_REUSEPORT
				auto& bound_sockets = ::at32(pport.bound_p2p_vports, p2p_vport);
				if (!sys_net_helpers::all_reusable(bound_sockets))
				{
					return -SYS_NET_EADDRINUSE;
				}

				bound_sockets.insert(lv2_id);
			}
			else
			{
				std::set<s32> bound_ports{lv2_id};
				pport.bound_p2p_vports.insert(std::make_pair(p2p_vport, std::move(bound_ports)));
			}
		}
	}

	{
		std::lock_guard lock(mutex);
		port       = p2p_port;
		vport      = p2p_vport;
		native_socket = real_socket;
		bound_addr = psa_in_p2p->sin_addr;
	}

	return CELL_OK;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_p2p::getsockname()
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
	paddr->sin_port   = port;
	paddr->sin_vport  = vport;
	paddr->sin_addr   = bound_addr;

	return {CELL_OK, sn_addr};
}

std::tuple<s32, lv2_socket::sockopt_data, u32> lv2_socket_p2p::getsockopt(s32 level, s32 optname, u32 len)
{
	std::lock_guard lock(mutex);

	const u64 key = (static_cast<u64>(level) << 32) | static_cast<u64>(optname);

	if (!sockopts.contains(key))
	{
		sys_net.error("Unhandled getsockopt(level=%d, optname=%d, len=%d)", level, optname, len);
		return {};
	}

	const auto& cache = ::at32(sockopts, key);
	return {CELL_OK, cache.data, cache.len};
}

s32 lv2_socket_p2p::setsockopt(s32 level, s32 optname, const std::vector<u8>& optval)
{
	std::lock_guard lock(mutex);

	int native_int = *reinterpret_cast<const be_t<s32>*>(optval.data());

	if (level == SYS_NET_SOL_SOCKET && optname == SYS_NET_SO_NBIO)
	{
		so_nbio = native_int;
	}

	const u64 key = (static_cast<u64>(level) << 32) | static_cast<u64>(optname);
	sockopt_cache cache{};
	memcpy(&cache.data._int, optval.data(), optval.size());
	cache.len = ::size32(optval);

	sockopts[key] = std::move(cache);

	return CELL_OK;
}

std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> lv2_socket_p2p::recvfrom(s32 flags, u32 len, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	if (data.empty())
	{
		if (so_nbio || (flags & SYS_NET_MSG_DONTWAIT))
			return {{-SYS_NET_EWOULDBLOCK, {}, {}}};

		return std::nullopt;
	}

	sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", vport, data.size());

	std::vector<u8> res_buf(len);

	const auto& p2p_data = data.front();
	s32 native_result    = std::min(len, static_cast<u32>(p2p_data.second.size()));
	memcpy(res_buf.data(), p2p_data.second.data(), native_result);

	sys_net_sockaddr sn_addr;
	memcpy(&sn_addr, &p2p_data.first, sizeof(sn_addr));

	data.pop();

	return {{native_result, res_buf, sn_addr}};
}

std::optional<s32> lv2_socket_p2p::sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	ensure(opt_sn_addr);
	ensure(native_socket); // ensures it has been bound
	ensure(buf.size() <= static_cast<usz>(65535 - VPORT_P2P_HEADER_SIZE)); // catch games using full payload for future fragmentation implementation if necessary
	const u16 p2p_port  = reinterpret_cast<const sys_net_sockaddr_in*>(&*opt_sn_addr)->sin_port;
	const u16 p2p_vport = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(&*opt_sn_addr)->sin_vport;

	auto native_addr = sys_net_addr_to_native_addr(*opt_sn_addr);

	char ip_str[16];
	inet_ntop(AF_INET, &native_addr.sin_addr, ip_str, sizeof(ip_str));
	sys_net.trace("[P2P] Sending a packet to %s:%d:%d", ip_str, p2p_port, p2p_vport);

	std::vector<u8> p2p_data(buf.size() + VPORT_P2P_HEADER_SIZE);
	const le_t<u16> p2p_vport_le = p2p_vport;
	const le_t<u16> src_vport_le = vport;
	const le_t<u16> p2p_flags_le = P2P_FLAG_P2P;
	memcpy(p2p_data.data(), &p2p_vport_le, sizeof(u16));
	memcpy(p2p_data.data() + sizeof(u16), &src_vport_le, sizeof(u16));
	memcpy(p2p_data.data() + sizeof(u16) + sizeof(u16), &p2p_flags_le, sizeof(u16));
	memcpy(p2p_data.data() + VPORT_P2P_HEADER_SIZE, buf.data(), buf.size());

	int native_flags = 0;
	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	auto native_result = np::sendto_possibly_ipv6(native_socket, reinterpret_cast<const char*>(p2p_data.data()), ::size32(p2p_data), &native_addr, native_flags);

	if (native_result >= 0)
	{
		return {std::max<s32>(native_result - VPORT_P2P_HEADER_SIZE, 0l)};
	}

	s32 result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);

	if (result)
	{
		return {-result};
	}

	// Note that this can only happen if the send buffer is full
	return std::nullopt;
}

std::optional<s32> lv2_socket_p2p::sendmsg([[maybe_unused]] s32 flags, [[maybe_unused]] const sys_net_msghdr& msg, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_p2p::sendmsg");
	return {};
}

void lv2_socket_p2p::close()
{
	if (!port || !vport)
	{
		return;
	}

	if (g_fxo->is_init<p2p_context>())
	{
		auto& nc = g_fxo->get<p2p_context>();
		std::lock_guard lock(nc.list_p2p_ports_mutex);

		if (!nc.list_p2p_ports.contains(port))
			return;

		auto& p2p_port = ::at32(nc.list_p2p_ports, port);
		{
			std::lock_guard lock(p2p_port.bound_p2p_vports_mutex);
			if (!p2p_port.bound_p2p_vports.contains(vport))
			{
				return;
			}

			auto& bound_sockets = ::at32(p2p_port.bound_p2p_vports, vport);
			bound_sockets.erase(lv2_id);

			if (bound_sockets.empty())
			{
				p2p_port.bound_p2p_vports.erase(vport);
			}
		}
	}
}

s32 lv2_socket_p2p::shutdown([[maybe_unused]] s32 how)
{
	sys_net.todo("[P2P] shutdown");
	return CELL_OK;
}

s32 lv2_socket_p2p::poll(sys_net_pollfd& sn_pfd, [[maybe_unused]] pollfd& native_pfd)
{
	std::lock_guard lock(mutex);
	ensure(vport);

	// Check if it's a bound P2P socket
	if ((sn_pfd.events & SYS_NET_POLLIN) && !data.empty())
	{
		sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", vport, data.size());
		sn_pfd.revents |= SYS_NET_POLLIN;
	}

	// Data can always be written on a dgram socket
	if (sn_pfd.events & SYS_NET_POLLOUT)
	{
		sn_pfd.revents |= SYS_NET_POLLOUT;
	}

	return sn_pfd.revents ? 1 : 0;
}

std::tuple<bool, bool, bool> lv2_socket_p2p::select(bs_t<lv2_socket::poll_t> selected, [[maybe_unused]] pollfd& native_pfd)
{
	std::lock_guard lock(mutex);

	bool read_set  = false;
	bool write_set = false;

	// Check if it's a bound P2P socket
	if ((selected & lv2_socket::poll_t::read) && vport && !data.empty())
	{
		sys_net.trace("[P2P] p2p_data for vport %d contains %d elements", vport, data.size());
		read_set = true;
	}

	if (selected & lv2_socket::poll_t::write)
	{
		write_set = true;
	}

	return {read_set, write_set, false};
}
