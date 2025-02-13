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
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/Cell/timers.hpp"
#include <shared_mutex>

#include "sys_net/network_context.h"
#include "sys_net/lv2_socket.h"
#include "sys_net/lv2_socket_native.h"
#include "sys_net/lv2_socket_raw.h"
#include "sys_net/lv2_socket_p2p.h"
#include "sys_net/lv2_socket_p2ps.h"
#include "sys_net/sys_net_helpers.h"

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sys_net_dump);

template <>
void fmt_class_string<sys_net_error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
		{
			switch (static_cast<s32>(error))
			{
#define SYS_NET_ERROR_CASE(x) \
	case -x: return "-" #x;   \
	case x:                   \
		return #x
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
void fmt_class_string<lv2_ip_protocol>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case SYS_NET_IPPROTO_IP: return "IPPROTO_IP";
			case SYS_NET_IPPROTO_ICMP: return "IPPROTO_ICMP";
			case SYS_NET_IPPROTO_IGMP: return "IPPROTO_IGMP";
			case SYS_NET_IPPROTO_TCP: return "IPPROTO_TCP";
			case SYS_NET_IPPROTO_UDP: return "IPPROTO_UDP";
			case SYS_NET_IPPROTO_ICMPV6: return "IPPROTO_ICMPV6";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<lv2_tcp_option>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case SYS_NET_TCP_NODELAY: return "TCP_NODELAY";
			case SYS_NET_TCP_MAXSEG: return "TCP_MAXSEG";
			case SYS_NET_TCP_MSS_TO_ADVERTISE: return "TCP_MSS_TO_ADVERTISE";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<lv2_socket_option>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case SYS_NET_SO_SNDBUF: return "SO_SNDBUF";
			case SYS_NET_SO_RCVBUF: return "SO_RCVBUF";
			case SYS_NET_SO_SNDLOWAT: return "SO_SNDLOWAT";
			case SYS_NET_SO_RCVLOWAT: return "SO_RCVLOWAT";
			case SYS_NET_SO_SNDTIMEO: return "SO_SNDTIMEO";
			case SYS_NET_SO_RCVTIMEO: return "SO_RCVTIMEO";
			case SYS_NET_SO_ERROR: return "SO_ERROR";
			case SYS_NET_SO_TYPE: return "SO_TYPE";
			case SYS_NET_SO_NBIO: return "SO_NBIO";
			case SYS_NET_SO_TPPOLICY: return "SO_TPPOLICY";
			case SYS_NET_SO_REUSEADDR: return "SO_REUSEADDR";
			case SYS_NET_SO_KEEPALIVE: return "SO_KEEPALIVE";
			case SYS_NET_SO_BROADCAST: return "SO_BROADCAST";
			case SYS_NET_SO_LINGER: return "SO_LINGER";
			case SYS_NET_SO_OOBINLINE: return "SO_OOBINLINE";
			case SYS_NET_SO_REUSEPORT: return "SO_REUSEPORT";
			case SYS_NET_SO_ONESBCAST: return "SO_ONESBCAST";
			case SYS_NET_SO_USECRYPTO: return "SO_USECRYPTO";
			case SYS_NET_SO_USESIGNATURE: return "SO_USESIGNATURE";
			case SYS_NET_SOL_SOCKET: return "SOL_SOCKET";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<lv2_ip_option>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
		{
			switch (value)
			{
			case SYS_NET_IP_HDRINCL: return "IP_HDRINCL";
			case SYS_NET_IP_TOS: return "IP_TOS";
			case SYS_NET_IP_TTL: return "IP_TTL";
			case SYS_NET_IP_MULTICAST_IF: return "IP_MULTICAST_IF";
			case SYS_NET_IP_MULTICAST_TTL: return "IP_MULTICAST_TTL";
			case SYS_NET_IP_MULTICAST_LOOP: return "IP_MULTICAST_LOOP";
			case SYS_NET_IP_ADD_MEMBERSHIP: return "IP_ADD_MEMBERSHIP";
			case SYS_NET_IP_DROP_MEMBERSHIP: return "IP_DROP_MEMBERSHIP";
			case SYS_NET_IP_TTLCHK: return "IP_TTLCHK";
			case SYS_NET_IP_MAXTTL: return "IP_MAXTTL";
			case SYS_NET_IP_DONTFRAG: return "IP_DONTFRAG";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<struct in_addr>::format(std::string& out, u64 arg)
{
	const u8* data = reinterpret_cast<const u8*>(&get_object(arg));

	fmt::append(out, "%u.%u.%u.%u", data[0], data[1], data[2], data[3]);
}

lv2_socket::lv2_socket(utils::serial& ar, lv2_socket_type _type)
	: family(ar)
	, type(_type)
	, protocol(ar)
	, so_nbio(ar)
	, so_error(ar)
	, so_tcp_maxseg(ar)
#ifdef _WIN32
	, so_reuseaddr(ar)
	, so_reuseport(ar)
{
#else
{
	// Try to match structure between different platforms
	ar.pos += 8;
#endif

	[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(lv2_net);

	ar(so_rcvtimeo, so_sendtimeo);

	lv2_id = idm::last_id();

	ar(last_bound_addr);
}

std::function<void(void*)> lv2_socket::load(utils::serial& ar)
{
	const lv2_socket_type type{ar};

	shared_ptr<lv2_socket> sock_lv2;

	switch (type)
	{
	case SYS_NET_SOCK_STREAM:
	case SYS_NET_SOCK_DGRAM:
	{
		auto lv2_native = make_shared<lv2_socket_native>(ar, type);
		ensure(lv2_native->create_socket() >= 0);
		sock_lv2 = std::move(lv2_native);
		break;
	}
	case SYS_NET_SOCK_RAW: sock_lv2 = make_shared<lv2_socket_raw>(ar, type); break;
	case SYS_NET_SOCK_DGRAM_P2P: sock_lv2 = make_shared<lv2_socket_p2p>(ar, type); break;
	case SYS_NET_SOCK_STREAM_P2P: sock_lv2 = make_shared<lv2_socket_p2ps>(ar, type); break;
	}

	if (std::memcmp(&sock_lv2->last_bound_addr, std::array<u8, 16>{}.data(), 16))
	{
		// NOTE: It is allowed fail
		sock_lv2->bind(sock_lv2->last_bound_addr);
	}

	return [ptr = sock_lv2](void* storage) { *static_cast<atomic_ptr<lv2_socket>*>(storage) = ptr; };;
}

void lv2_socket::save(utils::serial& ar, bool save_only_this_class)
{
	USING_SERIALIZATION_VERSION(lv2_net);

	if (save_only_this_class)
	{
		ar(family, protocol, so_nbio, so_error, so_tcp_maxseg);
#ifdef _WIN32
		ar(so_reuseaddr, so_reuseport);
#else
		ar(std::array<char, 8>{});
#endif
		ar(so_rcvtimeo, so_sendtimeo);
		ar(last_bound_addr);
		return;
	}

	ar(type);

	switch (type)
	{
	case SYS_NET_SOCK_STREAM:
	case SYS_NET_SOCK_DGRAM:
	{
		static_cast<lv2_socket_native*>(this)->save(ar);
		break;
	}
	case SYS_NET_SOCK_RAW: static_cast<lv2_socket_raw*>(this)->save(ar); break;
	case SYS_NET_SOCK_DGRAM_P2P: static_cast<lv2_socket_p2p*>(this)->save(ar); break;
	case SYS_NET_SOCK_STREAM_P2P: static_cast<lv2_socket_p2ps*>(this)->save(ar); break;
	}
}

void sys_net_dump_data(std::string_view desc, const u8* data, s32 len, const void* addr)
{
	const sys_net_sockaddr_in_p2p* p2p_addr = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(addr);

	if (p2p_addr)
		sys_net_dump.trace("%s(%s:%d:%d): %s", desc, np::ip_to_string(std::bit_cast<u32>(p2p_addr->sin_addr)), p2p_addr->sin_port, p2p_addr->sin_vport, fmt::buf_to_hexstring(data, len));
	else
		sys_net_dump.trace("%s: %s", desc, fmt::buf_to_hexstring(data, len));
}

error_code sys_net_bnet_accept(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_accept(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	if (addr.operator bool() != paddrlen.operator bool() || (paddrlen && *paddrlen < addr.size()))
	{
		return -SYS_NET_EINVAL;
	}

	s32 result = 0;
	sys_net_sockaddr sn_addr{};
	shared_ptr<lv2_socket> new_socket{};

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock)
		{
			auto [success, res, res_socket, res_addr] = sock.accept();

			if (success)
			{
				result  = res;
				sn_addr = res_addr;
				new_socket = std::move(res_socket);
				return true;
			}

			auto lock = sock.lock();

			sock.poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), lv2_socket::poll_t::read, [&](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::read)
					{
						auto [success, res, res_socket, res_addr] = sock.accept(false);
						if (success)
						{
							result  = res;
							sn_addr = res_addr;
							new_socket = std::move(res_socket);
							lv2_obj::awake(&ppu);
							return success;
						}
					}

					sock.set_poll_event(lv2_socket::poll_t::read);
					return false;
				});

			lv2_obj::prepare_for_sleep(ppu);
			lv2_obj::sleep(ppu);
			return false;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret)
	{
		while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
		{
			if (is_stopped(state))
			{
				return {};
			}

			if (state & cpu_flag::signal)
			{
				break;
			}

			ppu.state.wait(state);
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}

		if (result < 0)
		{
			return sys_net_error{result};
		}
	}

	if (result < 0)
	{
		return sys_net_error{result};
	}

	s32 id_ps3 = result;

	if (!id_ps3)
	{
		ensure(new_socket);
		id_ps3 = idm::import_existing<lv2_socket>(new_socket);
		if (id_ps3 == id_manager::id_traits<lv2_socket>::invalid)
		{
			return -SYS_NET_EMFILE;
		}
	}

	static_cast<void>(ppu.test_stopped());

	if (addr)
	{
		*paddrlen = sizeof(sys_net_sockaddr_in);
		*addr     = sn_addr;
	}

	// Socket ID
	return not_an_error(id_ps3);
}

error_code sys_net_bnet_bind(ppu_thread& ppu, s32 s, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_bind(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	if (!addr || addrlen < addr.size())
	{
		return -SYS_NET_EINVAL;
	}

	if (!idm::check_unlocked<lv2_socket>(s))
	{
		return -SYS_NET_EBADF;
	}

	const sys_net_sockaddr sn_addr = *addr;

	// 0 presumably defaults to AF_INET(to check?)
	if (sn_addr.sa_family != SYS_NET_AF_INET && sn_addr.sa_family != SYS_NET_AF_UNSPEC)
	{
		sys_net.error("sys_net_bnet_bind: unsupported sa_family (%d)", sn_addr.sa_family);
		return -SYS_NET_EAFNOSUPPORT;
	}

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			return sock.bind(sn_addr);
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		return sys_net_error{sock.ret};
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

	if (addr->sa_family != SYS_NET_AF_INET)
	{
		sys_net.error("sys_net_bnet_connect(s=%d): unsupported sa_family (%d)", s, addr->sa_family);
		return -SYS_NET_EAFNOSUPPORT;
	}

	if (!idm::check_unlocked<lv2_socket>(s))
	{
		return -SYS_NET_EBADF;
	}

	s32 result               = 0;
	sys_net_sockaddr sn_addr = *addr;

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock)
		{
			const auto success = sock.connect(sn_addr);

			if (success)
			{
				result = *success;
				return true;
			}

			auto lock = sock.lock();

			sock.poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), lv2_socket::poll_t::write, [&](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::write)
					{
						result = sock.connect_followup();

						lv2_obj::awake(&ppu);
						return true;
					}
					sock.set_poll_event(lv2_socket::poll_t::write);
					return false;
				});

			lv2_obj::prepare_for_sleep(ppu);
			lv2_obj::sleep(ppu);
			return false;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret)
	{
		if (result < 0)
		{
			return sys_net_error{result};
		}

		return not_an_error(result);
	}

	if (!sock.ret)
	{
		while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
		{
			if (is_stopped(state))
			{
				return {};
			}

			if (state & cpu_flag::signal)
			{
				break;
			}

			ppu.state.wait(state);
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}

		if (result)
		{
			if (result < 0)
			{
				return sys_net_error{result};
			}

			return not_an_error(result);
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

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			auto [res, sn_addr] = sock.getpeername();

			if (res == CELL_OK)
			{
				*paddrlen = sizeof(sys_net_sockaddr);
				*addr     = sn_addr;
			}

			return res;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

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

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			auto [res, sn_addr] = sock.getsockname();

			if (res == CELL_OK)
			{
				*paddrlen = sizeof(sys_net_sockaddr);
				*addr     = sn_addr;
			}

			return res;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

	return CELL_OK;
}

error_code sys_net_bnet_getsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::ptr<void> optval, vm::ptr<u32> optlen)
{
	ppu.state += cpu_flag::wait;

	switch (level)
	{
	case SYS_NET_SOL_SOCKET:
		sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=SYS_NET_SOL_SOCKET, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_socket_option>(optname), optval, optlen);
		break;
	case SYS_NET_IPPROTO_TCP:
		sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=SYS_NET_IPPROTO_TCP, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_tcp_option>(optname), optval, optlen);
		break;
	case SYS_NET_IPPROTO_IP:
		sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=SYS_NET_IPPROTO_IP, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_ip_option>(optname), optval, optlen);
		break;
	default:
		sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=%u)", s, level, optname, optval, optlen);
		break;
	}

	if (!optval || !optlen)
	{
		return -SYS_NET_EINVAL;
	}

	const u32 len = *optlen;

	if (!len)
	{
		return -SYS_NET_EINVAL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			if (len < sizeof(s32))
			{
				return -SYS_NET_EINVAL;
			}

			const auto& [res, out_val, out_len] = sock.getsockopt(level, optname, *optlen);

			if (res == CELL_OK)
			{
				std::memcpy(optval.get_ptr(), out_val.ch, out_len);
				*optlen = out_len;
			}

			return res;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

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

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			return sock.listen(backlog);
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

	return CELL_OK;
}

error_code sys_net_bnet_recvfrom(ppu_thread& ppu, s32 s, vm::ptr<void> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.trace("sys_net_bnet_recvfrom(s=%d, buf=*0x%x, len=%u, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);

	// If addr is null, paddrlen must be null as well
	if (!buf || !len || addr.operator bool() != paddrlen.operator bool())
	{
		return -SYS_NET_EINVAL;
	}

	if (flags & ~(SYS_NET_MSG_PEEK | SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL | SYS_NET_MSG_USECRYPTO | SYS_NET_MSG_USESIGNATURE))
	{
		fmt::throw_exception("sys_net_bnet_recvfrom(s=%d): unknown flags (0x%x)", flags);
	}

	s32 result = 0;
	sys_net_sockaddr sn_addr{};

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock)
		{
			const auto success = sock.recvfrom(flags, len);

			if (success)
			{
				const auto& [res, vec, res_addr] = *success;
				if (res > 0)
				{
					sn_addr = res_addr;
					std::memcpy(buf.get_ptr(), vec.data(), res);
					sys_net_dump_data("recvfrom", vec.data(), res, &res_addr);
				}

				result = res;
				return true;
			}

			auto lock = sock.lock();

			sock.poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), lv2_socket::poll_t::read, [&](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::read)
					{
						const auto success = sock.recvfrom(flags, len, false);

						if (success)
						{
							const auto& [res, vec, res_addr] = *success;
							if (res > 0)
							{
								sn_addr = res_addr;
								std::memcpy(buf.get_ptr(), vec.data(), res);
								sys_net_dump_data("recvfrom", vec.data(), res, &res_addr);
							}
							result = res;
							lv2_obj::awake(&ppu);
							return true;
						}
					}

					if (sock.so_rcvtimeo && get_guest_system_time() - ppu.start_time > sock.so_rcvtimeo)
					{
						result = -SYS_NET_EWOULDBLOCK;
						lv2_obj::awake(&ppu);
						return true;
					}

					sock.set_poll_event(lv2_socket::poll_t::read);
					return false;
				});

			lv2_obj::prepare_for_sleep(ppu);
			lv2_obj::sleep(ppu);
			return false;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret)
	{

		while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
		{
			if (is_stopped(state))
			{
				return {};
			}

			if (state & cpu_flag::signal)
			{
				break;
			}

			ppu.state.wait(state);
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	static_cast<void>(ppu.test_stopped());

	if (result == -SYS_NET_EWOULDBLOCK)
	{
		return not_an_error(result);
	}

	if (result >= 0)
	{
		if (addr)
		{
			*paddrlen = sizeof(sys_net_sockaddr_in);
			*addr     = sn_addr;
		}

		return not_an_error(result);
	}

	return sys_net_error{result};
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

	sys_net.warning("sys_net_bnet_sendmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);

	if (flags & ~(SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL | SYS_NET_MSG_USECRYPTO | SYS_NET_MSG_USESIGNATURE))
	{
		fmt::throw_exception("sys_net_bnet_sendmsg(s=%d): unknown flags (0x%x)", flags);
	}

	s32 result{};

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
		{
			auto netmsg = msg.get_ptr();
			const auto success = sock.sendmsg(flags, *netmsg);

			if (success)
			{
				result = *success;

				return true;
			}

			sock.poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), lv2_socket::poll_t::write, [&](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::write)
					{
						const auto success = sock.sendmsg(flags, *netmsg, false);

						if (success)
						{
							result = *success;
							lv2_obj::awake(&ppu);
							return true;
						}
					}

					sock.set_poll_event(lv2_socket::poll_t::write);
					return false;
				});

			lv2_obj::sleep(ppu);
			return false;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret)
	{
		while (true)
		{
			const auto state = ppu.state.fetch_sub(cpu_flag::signal);
			if (is_stopped(state) || state & cpu_flag::signal)
			{
				break;
			}
			thread_ctrl::wait_on(ppu.state, state);
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	if (result >= 0 || result == -SYS_NET_EWOULDBLOCK)
	{
		return not_an_error(result);
	}


	return sys_net_error{result};
}

error_code sys_net_bnet_sendto(ppu_thread& ppu, s32 s, vm::cptr<void> buf, u32 len, s32 flags, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.trace("sys_net_bnet_sendto(s=%d, buf=*0x%x, len=%u, flags=0x%x, addr=*0x%x, addrlen=%u)", s, buf, len, flags, addr, addrlen);

	if (flags & ~(SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL | SYS_NET_MSG_USECRYPTO | SYS_NET_MSG_USESIGNATURE))
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

	sys_net_dump_data("sendto", static_cast<const u8*>(buf.get_ptr()), len, addr ? addr.get_ptr() : nullptr);

	const std::optional<sys_net_sockaddr> sn_addr = addr ? std::optional<sys_net_sockaddr>(*addr) : std::nullopt;
	const std::vector<u8> buf_copy(vm::_ptr<const char>(buf.addr()), vm::_ptr<const char>(buf.addr()) + len);
	s32 result{};

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock)
		{
			auto success = sock.sendto(flags, buf_copy, sn_addr);

			if (success)
			{
				result = *success;
				return true;
			}

			auto lock = sock.lock();

			// Enable write event
			sock.poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), lv2_socket::poll_t::write, [&](bs_t<lv2_socket::poll_t> events) -> bool
				{
					if (events & lv2_socket::poll_t::write)
					{
						auto success = sock.sendto(flags, buf_copy, sn_addr, false);
						if (success)
						{
							result = *success;
							lv2_obj::awake(&ppu);
							return true;
						}
					}

					if (sock.so_sendtimeo && get_guest_system_time() - ppu.start_time > sock.so_sendtimeo)
					{
						result = -SYS_NET_EWOULDBLOCK;
						lv2_obj::awake(&ppu);
						return true;
					}

					sock.set_poll_event(lv2_socket::poll_t::write);
					return false;
				});

			lv2_obj::prepare_for_sleep(ppu);
			lv2_obj::sleep(ppu);
			return false;
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock.ret)
	{
		while (true)
		{
			const auto state = ppu.state.fetch_sub(cpu_flag::signal);
			if (is_stopped(state) || state & cpu_flag::signal)
			{
				break;
			}
			ppu.state.wait(state);
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			return -SYS_NET_EINTR;
		}
	}

	if (result >= 0 || result == -SYS_NET_EWOULDBLOCK)
	{
		return not_an_error(result);
	}

	return sys_net_error{result};
}

error_code sys_net_bnet_setsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
{
	ppu.state += cpu_flag::wait;

	switch (level)
	{
	case SYS_NET_SOL_SOCKET:
		sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=SYS_NET_SOL_SOCKET, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_socket_option>(optname), optval, optlen);
		break;
	case SYS_NET_IPPROTO_TCP:
		sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=SYS_NET_IPPROTO_TCP, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_tcp_option>(optname), optval, optlen);
		break;
	case SYS_NET_IPPROTO_IP:
		sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=SYS_NET_IPPROTO_IP, optname=%s, optval=*0x%x, optlen=%u)", s, static_cast<lv2_ip_option>(optname), optval, optlen);
		break;
	default:
		sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=%u)", s, level, optname, optval, optlen);
		break;
	}

	switch (optlen)
	{
	case 1:
		sys_net.warning("optval: 0x%02X", *static_cast<const u8*>(optval.get_ptr()));
		break;
	case 2:
		sys_net.warning("optval: 0x%04X", *static_cast<const be_t<u16>*>(optval.get_ptr()));
		break;
	case 4:
		sys_net.warning("optval: 0x%08X", *static_cast<const be_t<u32>*>(optval.get_ptr()));
		break;
	case 8:
		sys_net.warning("optval: 0x%016X", *static_cast<const be_t<u64>*>(optval.get_ptr()));
		break;
	}

	if (optlen < sizeof(s32))
	{
		return -SYS_NET_EINVAL;
	}

	std::vector<u8> optval_copy(vm::_ptr<u8>(optval.addr()), vm::_ptr<u8>(optval.addr() + optlen));

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			return sock.setsockopt(level, optname, optval_copy);
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

	return not_an_error(sock.ret);
}

error_code sys_net_bnet_shutdown(ppu_thread& ppu, s32 s, s32 how)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_shutdown(s=%d, how=%d)", s, how);

	if (how < 0 || how > 2)
	{
		return -SYS_NET_EINVAL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&, notify = lv2_obj::notify_all_t()](lv2_socket& sock) -> s32
		{
			return sock.shutdown(how);
		});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock.ret < 0)
	{
		return sys_net_error{sock.ret};
	}

	return CELL_OK;
}

error_code sys_net_bnet_socket(ppu_thread& ppu, lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_socket(family=%s, type=%s, protocol=%s)", family, type, protocol);

	if (family != SYS_NET_AF_INET)
	{
		sys_net.error("sys_net_bnet_socket(): unknown family (%d)", family);
	}

	if (type != SYS_NET_SOCK_STREAM && type != SYS_NET_SOCK_DGRAM && type != SYS_NET_SOCK_RAW && type != SYS_NET_SOCK_DGRAM_P2P && type != SYS_NET_SOCK_STREAM_P2P)
	{
		sys_net.error("sys_net_bnet_socket(): unsupported type (%d)", type);
		return -SYS_NET_EPROTONOSUPPORT;
	}

	shared_ptr<lv2_socket> sock_lv2;

	switch (type)
	{
	case SYS_NET_SOCK_STREAM:
	case SYS_NET_SOCK_DGRAM:
	{
		auto lv2_native = make_shared<lv2_socket_native>(family, type, protocol);
		if (s32 result = lv2_native->create_socket(); result < 0)
		{
			return sys_net_error{result};
		}

		sock_lv2 = std::move(lv2_native);
		break;
	}
	case SYS_NET_SOCK_RAW: sock_lv2 = make_shared<lv2_socket_raw>(family, type, protocol); break;
	case SYS_NET_SOCK_DGRAM_P2P: sock_lv2 = make_shared<lv2_socket_p2p>(family, type, protocol); break;
	case SYS_NET_SOCK_STREAM_P2P: sock_lv2 = make_shared<lv2_socket_p2ps>(family, type, protocol); break;
	}

	const s32 s = idm::import_existing<lv2_socket>(sock_lv2);

	// Can't allocate more than 1000 sockets
	if (s == id_manager::id_traits<lv2_socket>::invalid)
	{
		return -SYS_NET_EMFILE;
	}

	sock_lv2->set_lv2_id(s);

	return not_an_error(s);
}

error_code sys_net_bnet_close(ppu_thread& ppu, s32 s)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_close(s=%d)", s);

	auto sock = idm::withdraw<lv2_socket>(s);

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (sock->get_queue_size())
	{
		sock->abort_socket(0);
	}

	sock->close();

	{
		// Ensures the socket has no lingering copy from the network thread
		std::lock_guard nw_lock(g_fxo->get<network_context>().mutex_thread_loop);
		sock.reset();
	}

	return CELL_OK;
}

error_code sys_net_bnet_poll(ppu_thread& ppu, vm::ptr<sys_net_pollfd> fds, s32 nfds, s32 ms)
{
	ppu.state += cpu_flag::wait;

	sys_net.trace("sys_net_bnet_poll(fds=*0x%x, nfds=%d, ms=%d)", fds, nfds, ms);

	if (nfds <= 0)
	{
		return not_an_error(0);
	}

	atomic_t<s32> signaled{0};

	u64 timeout = ms < 0 ? 0 : ms * 1000ull;

	std::vector<sys_net_pollfd> fds_buf;

	{
		fds_buf.assign(fds.get_ptr(), fds.get_ptr() + nfds);

		lv2_obj::prepare_for_sleep(ppu);

		std::unique_lock nw_lock(g_fxo->get<network_context>().mutex_thread_loop);
		std::shared_lock lock(id_manager::g_mutex);

		std::vector<::pollfd> _fds(nfds);
#ifdef _WIN32
		std::vector<bool> connecting(nfds);
#endif

		for (s32 i = 0; i < nfds; i++)
		{
			_fds[i].fd         = -1;
			fds_buf[i].revents = 0;

			if (fds_buf[i].fd < 0)
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>(fds_buf[i].fd))
			{
				signaled += sock->poll(fds_buf[i], _fds[i]);
#ifdef _WIN32
				connecting[i] = sock->is_connecting();
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
		::poll(_fds.data(), nfds, 0);
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
			std::memcpy(fds.get_ptr(), fds_buf.data(), nfds * sizeof(sys_net_pollfd));
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
				auto lock = sock->lock();

#ifdef _WIN32
				sock->set_connecting(connecting[i]);
#endif

				bs_t<lv2_socket::poll_t> selected = +lv2_socket::poll_t::error;

				if (fds_buf[i].events & SYS_NET_POLLIN)
					selected += lv2_socket::poll_t::read;
				if (fds_buf[i].events & SYS_NET_POLLOUT)
					selected += lv2_socket::poll_t::write;
				// if (fds_buf[i].events & SYS_NET_POLLPRI) // Unimplemented
				//	selected += lv2_socket::poll::error;

				sock->poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), selected, [sock, selected, &fds_buf, i, &signaled, &ppu](bs_t<lv2_socket::poll_t> events)
					{
						if (events & selected)
						{
							if (events & selected & lv2_socket::poll_t::read)
								fds_buf[i].revents |= SYS_NET_POLLIN;
							if (events & selected & lv2_socket::poll_t::write)
								fds_buf[i].revents |= SYS_NET_POLLOUT;
							if (events & selected & lv2_socket::poll_t::error)
								fds_buf[i].revents |= SYS_NET_POLLERR;

							signaled++;
							sock->queue_wake(&ppu);
							return true;
						}

						sock->set_poll_event(selected);
						return false;
					});
			}
		}

		lv2_obj::sleep(ppu, timeout);
	}

	bool has_timedout = false;

	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (is_stopped(state))
		{
			return {};
		}

		if (state & cpu_flag::signal)
		{
			break;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return {};
				}

				has_timedout = network_clear_queue(ppu);
				clear_ppu_to_awake(ppu);
				ppu.state -= cpu_flag::signal;
				break;
			}
		}
		else
		{
			ppu.state.wait(state);
		}
	}

	if (!has_timedout && !signaled)
	{
		return -SYS_NET_EINTR;
	}

	std::memcpy(fds.get_ptr(), fds_buf.data(), nfds * sizeof(fds[0]));

	return not_an_error(signaled);
}

error_code sys_net_bnet_select(ppu_thread& ppu, s32 nfds, vm::ptr<sys_net_fd_set> readfds, vm::ptr<sys_net_fd_set> writefds, vm::ptr<sys_net_fd_set> exceptfds, vm::ptr<sys_net_timeval> _timeout)
{
	ppu.state += cpu_flag::wait;

	sys_net.trace("sys_net_bnet_select(nfds=%d, readfds=*0x%x, writefds=*0x%x, exceptfds=*0x%x, timeout=*0x%x(%d:%d))", nfds, readfds, writefds, exceptfds, _timeout, _timeout ? _timeout->tv_sec.value() : 0, _timeout ? _timeout->tv_usec.value() : 0);

	atomic_t<s32> signaled{0};

	if (exceptfds)
	{
		struct log_t
		{
			atomic_t<bool> logged = false;
		};

		if (!g_fxo->get<log_t>().logged.exchange(true))
		{
			sys_net.error("sys_net_bnet_select(): exceptfds not implemented");
		}
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

		std::lock_guard nw_lock(g_fxo->get<network_context>().mutex_thread_loop);
		reader_lock lock(id_manager::g_mutex);

		std::vector<::pollfd> _fds(nfds);
#ifdef _WIN32
		std::vector<bool> connecting(nfds);
#endif

		for (s32 i = 0; i < nfds; i++)
		{
			_fds[i].fd = -1;
			bs_t<lv2_socket::poll_t> selected{};

			if (readfds && _readfds.bit(i))
				selected += lv2_socket::poll_t::read;
			if (writefds && _writefds.bit(i))
				selected += lv2_socket::poll_t::write;
			// if (exceptfds && _exceptfds.bit(i))
			//	selected += lv2_socket::poll::error;

			if (selected)
			{
				selected += lv2_socket::poll_t::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{
				auto [read_set, write_set, except_set] = sock->select(selected, _fds[i]);

				if (read_set || write_set || except_set)
				{
					signaled++;
				}

				if (read_set)
				{
					rread.set(i);
				}

				if (write_set)
				{
					rwrite.set(i);
				}

				if (except_set)
				{
					rexcept.set(i);
				}

#ifdef _WIN32
				connecting[i] = sock->is_connecting();
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
		::poll(_fds.data(), nfds, 0);
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
			bs_t<lv2_socket::poll_t> selected{};

			if (readfds && _readfds.bit(i))
				selected += lv2_socket::poll_t::read;
			if (writefds && _writefds.bit(i))
				selected += lv2_socket::poll_t::write;
			// if (exceptfds && _exceptfds.bit(i))
			//	selected += lv2_socket::poll_t::error;

			if (selected)
			{
				selected += lv2_socket::poll_t::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{
				auto lock = sock->lock();
#ifdef _WIN32
				sock->set_connecting(connecting[i]);
#endif

				sock->poll_queue(idm::get_unlocked<named_thread<ppu_thread>>(ppu.id), selected, [sock, selected, i, &rread, &rwrite, &rexcept, &signaled, &ppu](bs_t<lv2_socket::poll_t> events)
					{
						if (events & selected)
						{
							if (selected & lv2_socket::poll_t::read && events & (lv2_socket::poll_t::read + lv2_socket::poll_t::error))
								rread.set(i);
							if (selected & lv2_socket::poll_t::write && events & (lv2_socket::poll_t::write + lv2_socket::poll_t::error))
								rwrite.set(i);
							// if (events & (selected & lv2_socket::poll::error))
						    //	rexcept.set(i);

							signaled++;
							sock->queue_wake(&ppu);
							return true;
						}

						sock->set_poll_event(selected);
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

	bool has_timedout = false;

	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (is_stopped(state))
		{
			return {};
		}

		if (state & cpu_flag::signal)
		{
			break;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return {};
				}

				has_timedout = network_clear_queue(ppu);
				clear_ppu_to_awake(ppu);
				ppu.state -= cpu_flag::signal;
				break;
			}
		}
		else
		{
			ppu.state.wait(state);
		}
	}

	if (!has_timedout && !signaled)
	{
		return -SYS_NET_EINTR;
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

	sys_net.todo("_sys_net_write_dump(id=0x%x, buf=*0x%x, len=%d, unk=0x%x)", id, buf, len, unknown);
	return CELL_OK;
}

error_code lv2_socket::abort_socket(s32 flags)
{
	decltype(queue) qcopy;
	{
		std::lock_guard lock(mutex);

		if (queue.empty())
		{
			if (flags & SYS_NET_ABORT_STRICT_CHECK)
			{
				// Strict error checking: ENOENT if nothing happened
				return -SYS_NET_ENOENT;
			}

			// TODO: Abort the subsequent function called on this socket (need to investigate correct behaviour)
			return CELL_OK;
		}

		qcopy = std::move(queue);
		queue = {};
		events.store({});
	}

	for (auto& [ppu, _] : qcopy)
	{
		if (!ppu)
			continue;

		// Avoid possible double signaling
		network_clear_queue(*ppu);
		clear_ppu_to_awake(*ppu);

		sys_net.warning("lv2_socket::abort_socket(): waking up \"%s\": (func: %s, r3=0x%x, r4=0x%x, r5=0x%x, r6=0x%x)", ppu->get_name(), ppu->current_function, ppu->gpr[3], ppu->gpr[4], ppu->gpr[5], ppu->gpr[6]);
		ppu->gpr[3] = static_cast<u64>(-SYS_NET_EINTR);
		lv2_obj::append(ppu.get());
	}

	const u32 num_waiters = ::size32(qcopy);
	if (num_waiters && (type == SYS_NET_SOCK_STREAM || type == SYS_NET_SOCK_DGRAM))
	{
		auto& nc = g_fxo->get<network_context>();
		const u32 prev_value = nc.num_polls.fetch_sub(num_waiters);
		ensure(prev_value >= num_waiters);
	}

	lv2_obj::awake_all();
	return CELL_OK;
}

error_code sys_net_abort(ppu_thread& ppu, s32 type, u64 arg, s32 flags)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_abort(type=%d, arg=0x%x, flags=0x%x)", type, arg, flags);

	enum abort_type : s32
	{
		_socket,
		resolver,
		type_2, // ??
		type_3, // ??
		all,
	};

	switch (type)
	{
	case _socket:
	{
		std::lock_guard nw_lock(g_fxo->get<network_context>().mutex_thread_loop);

		const auto sock = idm::get_unlocked<lv2_socket>(static_cast<u32>(arg));

		if (!sock)
		{
			return -SYS_NET_EBADF;
		}

		return sock->abort_socket(flags);
	}
	case all:
	{
		std::vector<u32> sockets;

		idm::select<lv2_socket>([&](u32 id, lv2_socket&)
		{
			sockets.emplace_back(id);
		});

		s32 failed = 0;

		for (u32 id : sockets)
		{
			const auto sock = idm::withdraw<lv2_socket>(id);

			if (!sock)
			{
				failed++;
				continue;
			}

			if (sock->get_queue_size())
				sys_net.error("ABORT 4");

			sock->close();

			sys_net.success("lv2_socket::handle_abort(): Closed socket %d", id);
		}

		// Ensures the socket has no lingering copy from the network thread
		g_fxo->get<network_context>().mutex_thread_loop.lock_unlock();

		return not_an_error(::narrow<s32>(sockets.size()) - failed);
	}
	case resolver:
	case type_2:
	case type_3:
	{
		break;
	}
	default: return -SYS_NET_EINVAL;
	}

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

		char buffer[nameserver.size() + 80]{};
		std::memcpy(buffer, nameserver.data(), nameserver.size());

		auto& nph          = g_fxo->get<named_thread<np::np_handler>>();
		const auto dns_str = np::ip_to_string(nph.get_dns_ip());
		std::memcpy(buffer + nameserver.size() - 1, dns_str.data(), dns_str.size());

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
