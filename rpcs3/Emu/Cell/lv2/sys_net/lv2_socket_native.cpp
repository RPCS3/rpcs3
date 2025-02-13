#include "stdafx.h"

#include "Emu/Cell/lv2/sys_net.h"
#include "Emu/NP/np_dnshook.h"
#include "Emu/NP/np_handler.h"
#include "lv2_socket_native.h"
#include "sys_net_helpers.h"

#ifdef _WIN32
constexpr SOCKET invalid_socket = INVALID_SOCKET;
#else
constexpr int invalid_socket = -1;
#endif

LOG_CHANNEL(sys_net);

lv2_socket_native::lv2_socket_native(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
	: lv2_socket(family, type, protocol)
{
}

lv2_socket_native::lv2_socket_native(utils::serial& ar, lv2_socket_type type)
	: lv2_socket(stx::make_exact(ar), type)
{
	[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(lv2_net);

#ifdef _WIN32
	ar(so_reuseaddr, so_reuseport);
#else
	std::array<char, 8> dummy{};
	ar(dummy);

	if (dummy != std::array<char, 8>{})
	{
		sys_net.error("[Native] Savestate tried to load Win32 specific data, compatibility may be affected");
	}
#endif

	if (version >= 2)
	{
		// Flag to signal failure of TCP connection on socket start
		ar(feign_tcp_conn_failure);
	}
}

void lv2_socket_native::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_net);

	lv2_socket::save(ar, true);
#ifdef _WIN32
	ar(so_reuseaddr, so_reuseport);
#else
	ar(std::array<char, 8>{});
#endif

	ar(is_socket_connected());
}

lv2_socket_native::~lv2_socket_native() noexcept
{
	lv2_socket_native::close();
}

s32 lv2_socket_native::create_socket()
{
	ensure(family == SYS_NET_AF_INET);
	ensure(type == SYS_NET_SOCK_STREAM || type == SYS_NET_SOCK_DGRAM);
	ensure(protocol == SYS_NET_IPPROTO_IP || protocol == SYS_NET_IPPROTO_TCP || protocol == SYS_NET_IPPROTO_UDP);

	const int native_domain = AF_INET;

	const int native_type = type == SYS_NET_SOCK_STREAM ? SOCK_STREAM : SOCK_DGRAM;

	int native_proto = protocol == SYS_NET_IPPROTO_TCP ? IPPROTO_TCP :
	                   protocol == SYS_NET_IPPROTO_UDP ? IPPROTO_UDP :
                                                         IPPROTO_IP;

	auto socket_res = ::socket(native_domain, native_type, native_proto);

	if (socket_res == invalid_socket)
	{
		return -get_last_error(false);
	}

	set_socket(socket_res, family, type, protocol);
	return CELL_OK;
}

void lv2_socket_native::set_socket(socket_type native_socket, lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
{
	this->native_socket = native_socket;
	this->family   = family;
	this->type     = type;
	this->protocol = protocol;

	set_default_buffers();
	set_non_blocking();
}

std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> lv2_socket_native::accept(bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	if (feign_tcp_conn_failure)
	{
		sys_net.error("Calling socket::accept() from a previously connected socket!");
	}

	socket_type client_socket = ::accept(native_socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

	if (client_socket != invalid_socket)
	{
		auto newsock = make_single<lv2_socket_native>(family, type, protocol);
		newsock->set_socket(client_socket, family, type, protocol);

		// Sockets inherit non blocking behaviour from their parent
		newsock->so_nbio = so_nbio;

		sys_net_sockaddr ps3_addr = native_addr_to_sys_net_addr(native_addr);

		return {true, 0, std::move(newsock), ps3_addr};
	}

	if (auto result = get_last_error(!so_nbio); result)
	{
		return {true, -result, {}, {}};
	}

	return {false, {}, {}, {}};
}

s32 lv2_socket_native::bind(const sys_net_sockaddr& addr)
{
	std::lock_guard lock(mutex);

	const auto* psa_in = reinterpret_cast<const sys_net_sockaddr_in*>(&addr);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	u32 saddr = nph.get_bind_ip();
	if (saddr == 0)
	{
		// If zero use the supplied address
		saddr = std::bit_cast<u32>(psa_in->sin_addr);
	}

	if (feign_tcp_conn_failure)
	{
		sys_net.error("Calling socket::bind() from a previously connected socket!");
	}

	::sockaddr_in native_addr{};
	native_addr.sin_family      = AF_INET;
	native_addr.sin_port        = std::bit_cast<u16>(psa_in->sin_port);
	native_addr.sin_addr.s_addr = saddr;
	::socklen_t native_addr_len = sizeof(native_addr);

	// Note that this is a hack(TODO)
	// ATM we don't support binding 3658 udp because we use it for the p2ps main socket
	// Only Fat Princess is known to do this to my knowledge
	if (psa_in->sin_port == 3658 && type == SYS_NET_SOCK_DGRAM)
	{
		native_addr.sin_port = std::bit_cast<u16, be_t<u16>>(3659);
	}

	sys_net.warning("[Native] Trying to bind %s:%d", native_addr.sin_addr, std::bit_cast<be_t<u16>, u16>(native_addr.sin_port));

	if (::bind(native_socket, reinterpret_cast<struct sockaddr*>(&native_addr), native_addr_len) == 0)
	{
		// Only UPNP port forward binds to 0.0.0.0
		if (saddr == 0)
		{
			if (native_addr.sin_port == 0)
			{
				sockaddr_in client_addr;
				socklen_t client_addr_size = sizeof(client_addr);
				ensure(::getsockname(native_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_size) == 0);
				bound_port = std::bit_cast<u16, be_t<u16>>(client_addr.sin_port);
			}
			else
			{
				bound_port = std::bit_cast<u16, be_t<u16>>(native_addr.sin_port);
			}

			nph.upnp_add_port_mapping(bound_port, type == SYS_NET_SOCK_STREAM ? "TCP" : "UDP");
		}

		last_bound_addr = addr;
		return CELL_OK;
	}

	auto error = get_last_error(false);

#ifdef __linux__
	if (error == SYS_NET_EACCES && std::bit_cast<be_t<u16>, u16>(native_addr.sin_port) < 1024)
	{
		sys_net.error("The game tried to bind a port < 1024 which is privileged on Linux\n"
					  "Consider setting rpcs3 privileges for it with: setcap 'cap_net_bind_service=+ep' /path/to/rpcs3");
	}
#endif

	return -error;
}

std::optional<s32> lv2_socket_native::connect(const sys_net_sockaddr& addr)
{
	std::lock_guard lock(mutex);

	const auto* psa_in = reinterpret_cast<const sys_net_sockaddr_in*>(&addr);

	::sockaddr_in native_addr   = sys_net_addr_to_native_addr(addr);
	::socklen_t native_addr_len = sizeof(native_addr);

	sys_net.notice("[Native] Attempting to connect on %s:%d", native_addr.sin_addr, std::bit_cast<be_t<u16>, u16>(native_addr.sin_port));

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	if (!nph.get_net_status() && is_ip_public_address(native_addr))
	{
		return -SYS_NET_EADDRNOTAVAIL;
	}

	if (psa_in->sin_port == 53)
	{
		// Add socket to the dns hook list
		sys_net.notice("[Native] sys_net_bnet_connect: using DNS...");
		auto& dnshook = g_fxo->get<np::dnshook>();
		dnshook.add_dns_spy(lv2_id);
	}

#ifdef _WIN32
	bool was_connecting = connecting;
#endif

	if (feign_tcp_conn_failure)
	{
		// As if still connected
		return -SYS_NET_EALREADY;
	}

	if (::connect(native_socket, reinterpret_cast<struct sockaddr*>(&native_addr), native_addr_len) == 0)
	{
		return CELL_OK;
	}

	sys_net_error result = get_last_error(!so_nbio);

#ifdef _WIN32
	// See https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
	if (was_connecting && (result == SYS_NET_EINVAL || result == SYS_NET_EWOULDBLOCK))
		return -SYS_NET_EALREADY;
#endif

	if (result)
	{
		if (result == SYS_NET_EWOULDBLOCK || result == SYS_NET_EINPROGRESS)
		{
			result = SYS_NET_EINPROGRESS;
#ifdef _WIN32
			connecting = true;
#endif
			this->poll_queue(null_ptr, lv2_socket::poll_t::write, [this](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::write)
					{
						int native_error;
						::socklen_t size = sizeof(native_error);
						if (::getsockopt(native_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&native_error), &size) != 0 || size != sizeof(int))
						{
							so_error = 1;
						}
						else
						{
							// TODO: check error formats (both native and translated)
							so_error = native_error ? convert_error(false, native_error) : 0;
						}

						return true;
					}

					events += lv2_socket::poll_t::write;
					return false;
				});
		}

		return -result;
	}

#ifdef _WIN32
	connecting = true;
#endif

	return std::nullopt;
}

s32 lv2_socket_native::connect_followup()
{
	int native_error;
	::socklen_t size = sizeof(native_error);
	if (::getsockopt(native_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&native_error), &size) != 0 || size != sizeof(int))
	{
		return -1;
	}

	// TODO: check error formats (both native and translated)
	return native_error ? -convert_error(false, native_error) : 0;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_native::getpeername()
{
	std::lock_guard lock(mutex);

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	if (::getpeername(native_socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen) == 0)
	{
		ensure(native_addr.ss_family == AF_INET);

		sys_net_sockaddr sn_addr = native_addr_to_sys_net_addr(native_addr);

		return {CELL_OK, sn_addr};
	}

	return {-get_last_error(false), {}};
}

std::pair<s32, sys_net_sockaddr> lv2_socket_native::getsockname()
{
	std::lock_guard lock(mutex);

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	if (::getsockname(native_socket, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen) == 0)
	{
		ensure(native_addr.ss_family == AF_INET);

		sys_net_sockaddr sn_addr = native_addr_to_sys_net_addr(native_addr);

		return {CELL_OK, sn_addr};
	}
#ifdef _WIN32
	else
	{
		// windows doesn't support getsockname for sockets that are not bound
		if (get_native_error() == WSAEINVAL)
		{
			return {CELL_OK, {}};
		}
	}
#endif

	return {-get_last_error(false), {}};
}

std::tuple<s32, lv2_socket::sockopt_data, u32> lv2_socket_native::getsockopt(s32 level, s32 optname, u32 len)
{
	std::lock_guard lock(mutex);

	sockopt_data out_val;
	u32 out_len = sizeof(out_val);

	int native_level = -1;
	int native_opt   = -1;

	union
	{
		char ch[128];
		int _int = 0;
		::timeval timeo;
		::linger linger;
	} native_val;
	::socklen_t native_len = sizeof(native_val);

	if (level == SYS_NET_SOL_SOCKET)
	{
		native_level = SOL_SOCKET;

		switch (optname)
		{
		case SYS_NET_SO_NBIO:
		{
			// Special
			out_val._int = so_nbio;
			out_len      = sizeof(s32);
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_SO_ERROR:
		{
			// Special
			out_val._int = std::exchange(so_error, 0);
			out_len      = sizeof(s32);
			return {CELL_OK, out_val, out_len};
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
			out_val._int = so_reuseaddr;
			out_len      = sizeof(s32);
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_SO_REUSEPORT:
		{
			out_val._int = so_reuseport;
			out_len      = sizeof(s32);
			return {CELL_OK, out_val, out_len};
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
				return {-SYS_NET_EINVAL, {}, {}};

			native_opt = optname == SYS_NET_SO_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
			break;
		}
		case SYS_NET_SO_LINGER:
		{
			if (len < sizeof(sys_net_linger))
				return {-SYS_NET_EINVAL, {}, {}};

			native_opt = SO_LINGER;
			break;
		}
		default:
		{
			sys_net.error("sys_net_bnet_getsockopt(s=%d, SOL_SOCKET): unknown option (0x%x)", lv2_id, optname);
			return {-SYS_NET_EINVAL, {}, {}};
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
			out_val._int = so_tcp_maxseg;
			out_len      = sizeof(s32);
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_TCP_NODELAY:
		{
			native_opt = TCP_NODELAY;
			break;
		}
		default:
		{
			sys_net.error("sys_net_bnet_getsockopt(s=%d, IPPROTO_TCP): unknown option (0x%x)", lv2_id, optname);
			return {-SYS_NET_EINVAL, {}, {}};
		}
		}
	}
	else if (level == SYS_NET_IPPROTO_IP)
	{
		native_level = IPPROTO_IP;
		switch (optname)
		{
		case SYS_NET_IP_HDRINCL:
		{
			native_opt = IP_HDRINCL;
			break;
		}
		case SYS_NET_IP_TOS:
		{
			native_opt = IP_TOS;
			break;
		}
		case SYS_NET_IP_TTL:
		{
			native_opt = IP_TTL;
			break;
		}
		case SYS_NET_IP_MULTICAST_IF:
		{
			native_opt = IP_MULTICAST_IF;
			break;
		}
		case SYS_NET_IP_MULTICAST_TTL:
		{
			native_opt = IP_MULTICAST_TTL;
			break;
		}
		case SYS_NET_IP_MULTICAST_LOOP:
		{
			native_opt = IP_MULTICAST_LOOP;
			break;
		}
		case SYS_NET_IP_ADD_MEMBERSHIP:
		{
			native_opt = IP_ADD_MEMBERSHIP;
			break;
		}
		case SYS_NET_IP_DROP_MEMBERSHIP:
		{
			native_opt = IP_DROP_MEMBERSHIP;
			break;
		}
		case SYS_NET_IP_TTLCHK:
		{
			sys_net.error("sys_net_bnet_getsockopt(IPPROTO_IP, SYS_NET_IP_TTLCHK): stubbed option");
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_IP_MAXTTL:
		{
			sys_net.error("sys_net_bnet_getsockopt(IPPROTO_IP, SYS_NET_IP_MAXTTL): stubbed option");
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_IP_DONTFRAG:
		{
#ifdef _WIN32
			native_opt = IP_DONTFRAGMENT;
#else
			native_opt = IP_DF;
#endif
			break;
		}
		default:
		{
			sys_net.error("sys_net_bnet_getsockopt(s=%d, IPPROTO_IP): unknown option (0x%x)", lv2_id, optname);
			return {-SYS_NET_EINVAL, {}, {}};
		}
		}
	}
	else
	{
		sys_net.error("sys_net_bnet_getsockopt(s=%d): unknown level (0x%x)", lv2_id, level);
		return {-SYS_NET_EINVAL, {}, {}};
	}

	if (::getsockopt(native_socket, native_level, native_opt, native_val.ch, &native_len) != 0)
	{
		return {-get_last_error(false), {}, {}};
	}

	if (level == SYS_NET_SOL_SOCKET)
	{
		switch (optname)
		{
		case SYS_NET_SO_SNDTIMEO:
		case SYS_NET_SO_RCVTIMEO:
		{
			// TODO
			out_val.timeo = {::narrow<s64>(native_val.timeo.tv_sec), ::narrow<s64>(native_val.timeo.tv_usec)};
			out_len       = sizeof(sys_net_timeval);
			return {CELL_OK, out_val, out_len};
		}
		case SYS_NET_SO_LINGER:
		{
			// TODO
			out_val.linger = {::narrow<s32>(native_val.linger.l_onoff), ::narrow<s32>(native_val.linger.l_linger)};
			out_len        = sizeof(sys_net_linger);
			return {CELL_OK, out_val, out_len};
		}
		default: break;
		}
	}

	// Fallback to int
	out_val._int = native_val._int;
	out_len      = sizeof(s32);
	return {CELL_OK, out_val, out_len};
}

s32 lv2_socket_native::setsockopt(s32 level, s32 optname, const std::vector<u8>& optval)
{
	std::lock_guard lock(mutex);

	int native_int         = 0;
	int native_level       = -1;
	int native_opt         = -1;
	const void* native_val = &native_int;
	::socklen_t native_len = sizeof(int);
	::linger native_linger;
	::ip_mreq native_mreq;

#ifdef _WIN32
	u32 native_timeo;
#else
	::timeval native_timeo;
#endif

	native_int = *reinterpret_cast<const be_t<s32>*>(optval.data());

	if (level == SYS_NET_SOL_SOCKET)
	{
		native_level = SOL_SOCKET;

		switch (optname)
		{
		case SYS_NET_SO_NBIO:
		{
			// Special
			so_nbio = native_int;
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
			native_opt   = SO_REUSEADDR;
			so_reuseaddr = native_int;
			native_int   = so_reuseaddr || so_reuseport ? 1 : 0;
			break;
		}
		case SYS_NET_SO_REUSEPORT:
		{
			native_opt   = SO_REUSEADDR;
			so_reuseport = native_int;
			native_int   = so_reuseaddr || so_reuseport ? 1 : 0;
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
			if (optval.size() < sizeof(sys_net_timeval))
				return -SYS_NET_EINVAL;

			native_opt = optname == SYS_NET_SO_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
			native_val = &native_timeo;
			native_len = sizeof(native_timeo);

			const int tv_sec = ::narrow<int>(reinterpret_cast<const sys_net_timeval*>(optval.data())->tv_sec);
			const int tv_usec = ::narrow<int>(reinterpret_cast<const sys_net_timeval*>(optval.data())->tv_usec);
#ifdef _WIN32
			native_timeo = tv_sec * 1000;
			native_timeo += tv_usec / 1000;
#else
			native_timeo.tv_sec = tv_sec;
			native_timeo.tv_usec = tv_usec;
#endif
			// TODO: Overflow detection?
			(optname == SYS_NET_SO_SNDTIMEO ? so_sendtimeo : so_rcvtimeo) = tv_usec + tv_sec * 1000000;
			break;
		}
		case SYS_NET_SO_LINGER:
		{
			if (optval.size() < sizeof(sys_net_linger))
				return -SYS_NET_EINVAL;

			// TODO
			native_opt             = SO_LINGER;
			native_val             = &native_linger;
			native_len             = sizeof(native_linger);
			native_linger.l_onoff  = reinterpret_cast<const sys_net_linger*>(optval.data())->l_onoff;
			native_linger.l_linger = reinterpret_cast<const sys_net_linger*>(optval.data())->l_linger;
			break;
		}
		case SYS_NET_SO_USECRYPTO:
		{
			// TODO
			sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USECRYPTO)", lv2_id, optname);
			return {};
		}
		case SYS_NET_SO_USESIGNATURE:
		{
			// TODO
			sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USESIGNATURE)", lv2_id, optname);
			return {};
		}
		default:
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): unknown option (0x%x)", lv2_id, optname);
			return -SYS_NET_EINVAL;
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
			so_tcp_maxseg = native_int;
			return {};
		}
		case SYS_NET_TCP_NODELAY:
		{
			native_opt = TCP_NODELAY;
			break;
		}
		default:
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d, IPPROTO_TCP): unknown option (0x%x)", lv2_id, optname);
			return -SYS_NET_EINVAL;
		}
		}
	}
	else if (level == SYS_NET_IPPROTO_IP)
	{
		native_level = IPPROTO_IP;

		switch (optname)
		{
		case SYS_NET_IP_HDRINCL:
		{
			native_opt = IP_HDRINCL;
			break;
		}
		case SYS_NET_IP_TOS:
		{
			native_opt = IP_TOS;
			break;
		}
		case SYS_NET_IP_TTL:
		{
			native_opt = IP_TTL;
			break;
		}
		case SYS_NET_IP_MULTICAST_IF:
		{
			native_opt = IP_MULTICAST_IF;
			break;
		}
		case SYS_NET_IP_MULTICAST_TTL:
		{
			native_opt = IP_MULTICAST_TTL;
			break;
		}
		case SYS_NET_IP_MULTICAST_LOOP:
		{
			native_opt = IP_MULTICAST_LOOP;
			break;
		}
		case SYS_NET_IP_ADD_MEMBERSHIP:
		case SYS_NET_IP_DROP_MEMBERSHIP:
		{
			if (optval.size() < sizeof(sys_net_ip_mreq))
				return -SYS_NET_EINVAL;

			native_opt                       = optname == SYS_NET_IP_ADD_MEMBERSHIP ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
			native_val                       = &native_mreq;
			native_len                       = sizeof(::ip_mreq);
			native_mreq.imr_interface.s_addr = std::bit_cast<u32>(reinterpret_cast<const sys_net_ip_mreq*>(optval.data())->imr_interface);
			native_mreq.imr_multiaddr.s_addr = std::bit_cast<u32>(reinterpret_cast<const sys_net_ip_mreq*>(optval.data())->imr_multiaddr);
			break;
		}
		case SYS_NET_IP_TTLCHK:
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d, IPPROTO_IP): Stubbed option (0x%x) (SYS_NET_IP_TTLCHK)", lv2_id, optname);
			break;
		}
		case SYS_NET_IP_MAXTTL:
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d, IPPROTO_IP): Stubbed option (0x%x) (SYS_NET_IP_MAXTTL)", lv2_id, optname);
			break;
		}
		case SYS_NET_IP_DONTFRAG:
		{
#ifdef _WIN32
			native_opt = IP_DONTFRAGMENT;
#else
			native_opt = IP_DF;
#endif
			break;
		}
		default:
		{
			sys_net.error("sys_net_bnet_setsockopt(s=%d, IPPROTO_IP): unknown option (0x%x)", lv2_id, optname);
			return -SYS_NET_EINVAL;
		}
		}
	}
	else
	{
		sys_net.error("sys_net_bnet_setsockopt(s=%d): unknown level (0x%x)", lv2_id, level);
		return -SYS_NET_EINVAL;
	}

	if (::setsockopt(native_socket, native_level, native_opt, static_cast<const char*>(native_val), native_len) == 0)
	{
		return {};
	}

	return -get_last_error(false);
}

s32 lv2_socket_native::listen(s32 backlog)
{
	std::lock_guard lock(mutex);

	if (::listen(native_socket, backlog) == 0)
	{
		return CELL_OK;
	}

	return -get_last_error(false);
}

std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> lv2_socket_native::recvfrom(s32 flags, u32 len, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	if (feign_tcp_conn_failure)
	{
		// As if just lost the connection
		feign_tcp_conn_failure = false;
		return {{-SYS_NET_ECONNRESET, {},{}}};
	}

	int native_flags = 0;
	::sockaddr_storage native_addr{};
	::socklen_t native_addrlen = sizeof(native_addr);
	std::vector<u8> res_buf(len);

	auto& dnshook = g_fxo->get<np::dnshook>();
	if (dnshook.is_dns(lv2_id) && dnshook.is_dns_queue(lv2_id))
	{
		auto& nph         = g_fxo->get<named_thread<np::np_handler>>();
		const auto packet = dnshook.get_dns_packet(lv2_id);
		ensure(packet.size() < len);
		memcpy(res_buf.data(), packet.data(), packet.size());
		native_addr.ss_family                                             = AF_INET;
		(reinterpret_cast<::sockaddr_in*>(&native_addr))->sin_port        = std::bit_cast<u16, be_t<u16>>(53); // htons(53)
		(reinterpret_cast<::sockaddr_in*>(&native_addr))->sin_addr.s_addr = nph.get_dns_ip();
		const auto sn_addr                                                = native_addr_to_sys_net_addr(native_addr);

		return {{::narrow<s32>(packet.size()), res_buf, sn_addr}};
	}

	if (flags & SYS_NET_MSG_PEEK)
	{
		native_flags |= MSG_PEEK;
	}

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	auto native_result = ::recvfrom(native_socket, reinterpret_cast<char*>(res_buf.data()), len, native_flags, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

	if (native_result >= 0)
	{
		const auto sn_addr = native_addr_to_sys_net_addr(native_addr);
		return {{::narrow<s32>(native_result), res_buf, sn_addr}};
	}
#ifdef _WIN32
	else
	{
		// Windows returns an error when trying to peek at a message and buffer not long enough to contain the whole message, should be ignored
		if ((native_flags & MSG_PEEK) && get_native_error() == WSAEMSGSIZE)
		{
			const auto sn_addr = native_addr_to_sys_net_addr(native_addr);
			return {{len, res_buf, sn_addr}};
		}
		// Windows will return WSASHUTDOWN when the connection is shutdown, POSIX just returns EOF (0) in this situation.
		if (get_native_error() == WSAESHUTDOWN)
		{
			const auto sn_addr = native_addr_to_sys_net_addr(native_addr);
			return {{0, {}, sn_addr}};
		}
	}
	const auto result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0, connecting);
#else
	const auto result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);
#endif

	if (result)
	{
		return {{-result, {}, {}}};
	}

	return std::nullopt;
}

std::optional<s32> lv2_socket_native::sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	int native_flags                       = 0;
	int native_result                      = -1;
	std::optional<sockaddr_in> native_addr = std::nullopt;

	if (opt_sn_addr)
	{
		native_addr = sys_net_addr_to_native_addr(*opt_sn_addr);
		sys_net.trace("[Native] Attempting to send to %s:%d", (*native_addr).sin_addr, std::bit_cast<be_t<u16>, u16>((*native_addr).sin_port));

		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		if (!nph.get_net_status() && is_ip_public_address(*native_addr))
		{
			return -SYS_NET_EADDRNOTAVAIL;
		}
	}
	else if (feign_tcp_conn_failure)
	{
		// As if just lost the connection
		feign_tcp_conn_failure = false;
		return -SYS_NET_ECONNRESET;
	}

	sys_net_error result{};

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	auto& dnshook = g_fxo->get<np::dnshook>();
	if (opt_sn_addr && type == SYS_NET_SOCK_DGRAM && reinterpret_cast<const sys_net_sockaddr_in*>(&*opt_sn_addr)->sin_port == 53)
	{
		dnshook.add_dns_spy(lv2_id);
	}

	if (dnshook.is_dns(lv2_id))
	{
		const s32 ret_analyzer = dnshook.analyze_dns_packet(lv2_id, reinterpret_cast<const u8*>(buf.data()), ::size32(buf));

		// Check if the packet is intercepted
		if (ret_analyzer >= 0)
		{
			return {ret_analyzer};
		}
	}

	native_result = ::sendto(native_socket, reinterpret_cast<const char*>(buf.data()), ::narrow<int>(buf.size()), native_flags, native_addr ? reinterpret_cast<struct sockaddr*>(&native_addr.value()) : nullptr, native_addr ? sizeof(sockaddr_in) : 0);

	if (native_result >= 0)
	{
		return {native_result};
	}

#ifdef _WIN32
	result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0, connecting);
#else
	result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);
#endif

	if (result)
	{
		return {-result};
	}

	// Note that this can only happen if the send buffer is full
	return std::nullopt;
}

std::optional<s32> lv2_socket_native::sendmsg(s32 flags, const sys_net_msghdr& msg, bool is_lock)
{
	std::unique_lock<shared_mutex> lock(mutex, std::defer_lock);

	if (is_lock)
	{
		lock.lock();
	}

	int native_flags                       = 0;
	int native_result                      = -1;

	sys_net_error result{};

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	if (feign_tcp_conn_failure)
	{
		// As if just lost the connection
		feign_tcp_conn_failure = false;
		return {-SYS_NET_ECONNRESET};
	}

	for (int i = 0; i < msg.msg_iovlen; i++)
	{
		auto iov_base = msg.msg_iov[i].iov_base;
		const u32 len = msg.msg_iov[i].iov_len;
		const std::vector<u8> buf_copy(vm::_ptr<const char>(iov_base.addr()), vm::_ptr<const char>(iov_base.addr()) + len);

		native_result = ::send(native_socket, reinterpret_cast<const char*>(buf_copy.data()), ::narrow<int>(buf_copy.size()), native_flags);

		if (native_result >= 0)
		{
			return {native_result};
		}
	}

	result = get_last_error(!so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0);

	if (result)
	{
		return {-result};
	}

	return std::nullopt;
}

void lv2_socket_native::close()
{
	std::lock_guard lock(mutex);

	np::close_socket(native_socket);
	native_socket = {};

	if (auto dnshook = g_fxo->try_get<np::dnshook>())
	{
		dnshook->remove_dns_spy(lv2_id);
	}

	if (bound_port && g_fxo->is_init<named_thread<np::np_handler>>())
	{
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		nph.upnp_remove_port_mapping(bound_port, type == SYS_NET_SOCK_STREAM ? "TCP" : "UDP");
		bound_port = 0;
	}
}

s32 lv2_socket_native::shutdown(s32 how)
{
	std::lock_guard lock(mutex);

	if (feign_tcp_conn_failure)
	{
		// As if still connected
		return CELL_OK;
	}

#ifdef _WIN32
	const int native_how =
		how == SYS_NET_SHUT_RD ? SD_RECEIVE :
		how == SYS_NET_SHUT_WR ? SD_SEND :
                                 SD_BOTH;
#else
	const int native_how =
		how == SYS_NET_SHUT_RD ? SHUT_RD :
		how == SYS_NET_SHUT_WR ? SHUT_WR :
                                 SHUT_RDWR;
#endif

	if (::shutdown(native_socket, native_how) == 0)
	{
		return CELL_OK;
	}

	return -get_last_error(false);
}

s32 lv2_socket_native::poll(sys_net_pollfd& sn_pfd, pollfd& native_pfd)
{
	// Check for fake packet for dns interceptions
	auto& dnshook = g_fxo->get<np::dnshook>();
	if (sn_pfd.events & SYS_NET_POLLIN && dnshook.is_dns(sn_pfd.fd) && dnshook.is_dns_queue(sn_pfd.fd))
	{
		sn_pfd.revents |= SYS_NET_POLLIN;
		return 1;
	}
	if (sn_pfd.events & ~(SYS_NET_POLLIN | SYS_NET_POLLOUT | SYS_NET_POLLERR))
	{
		sys_net.warning("sys_net_bnet_poll(fd=%d): events=0x%x", sn_pfd.fd, sn_pfd.events);
	}

	native_pfd.fd = native_socket;

	if (sn_pfd.events & SYS_NET_POLLIN)
	{
		native_pfd.events |= POLLIN;
	}
	if (sn_pfd.events & SYS_NET_POLLOUT)
	{
		native_pfd.events |= POLLOUT;
	}

	return 0;
}

std::tuple<bool, bool, bool> lv2_socket_native::select(bs_t<lv2_socket::poll_t> selected, pollfd& native_pfd)
{
	native_pfd.fd = native_socket;
	if (selected & lv2_socket::poll_t::read)
	{
		native_pfd.events |= POLLIN;
	}
	if (selected & lv2_socket::poll_t::write)
	{
		native_pfd.events |= POLLOUT;
	}

	return {};
}

void lv2_socket_native::set_default_buffers()
{
	// Those are the default PS3 values
	u32 default_RCVBUF = (type == SYS_NET_SOCK_STREAM) ? 65535 : 9216;
	if (::setsockopt(native_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&default_RCVBUF), sizeof(default_RCVBUF)) != 0)
	{
		sys_net.error("Error setting default SO_RCVBUF on sys_net_bnet_socket socket");
	}
	u32 default_SNDBUF = 131072;
	if (::setsockopt(native_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&default_SNDBUF), sizeof(default_SNDBUF)) != 0)
	{
		sys_net.error("Error setting default SO_SNDBUF on sys_net_bnet_socket socket");
	}
}

void lv2_socket_native::set_non_blocking()
{
	// Set non-blocking
	// This is done to avoid having threads stuck on blocking socket functions
	// Blocking functions just put the thread to sleep and delegate the waking up to network_thread which polls the sockets
	np::set_socket_non_blocking(native_socket);
}

bool lv2_socket_native::is_socket_connected()
{
	if (type != SYS_NET_SOCK_STREAM)
	{
		return false;
	}

	std::lock_guard lock(mutex);

	int listening = 0;
	socklen_t len = sizeof(listening);

	if (::getsockopt(native_socket, SOL_SOCKET, SO_ACCEPTCONN, reinterpret_cast<char*>(&listening), &len) == -1)
	{
		return false;
	}

	if (listening)
	{
		// Would be handled in other ways
		return false;
	}

	fd_set readfds, writefds;
	struct timeval timeout{0, 0}; // Zero timeout

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(native_socket, &readfds);
	FD_SET(native_socket, &writefds);

	// Use select to check for readability and writability
	const int result = ::select(1, &readfds, &writefds, NULL, &timeout);

	if (result < 0)
	{
		// Error occurred
		return false;
	}

	// Socket is connected if it's readable or writable
	return FD_ISSET(native_socket, &readfds) || FD_ISSET(native_socket, &writefds);
}
