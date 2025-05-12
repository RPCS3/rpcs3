#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUThread.h"
#include "lv2_socket.h"
#include "sys_net_helpers.h"
#include "network_context.h"

#ifdef _WIN32
#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_helpers.h"
#endif

LOG_CHANNEL(sys_net);

int get_native_error()
{
	int native_error;
#ifdef _WIN32
	native_error = WSAGetLastError();
#else
	native_error = errno;
#endif

	return native_error;
}

sys_net_error convert_error(bool is_blocking, int native_error, [[maybe_unused]] bool is_connecting)
{
	// Convert the error code for socket functions to a one for sys_net
	sys_net_error result{};
	const char* name{};

#ifdef _WIN32
#define ERROR_CASE(error)         \
	case WSA##error:              \
		result = SYS_NET_##error; \
		name = #error;            \
		break;
#else
#define ERROR_CASE(error)         \
	case error:                   \
		result = SYS_NET_##error; \
		name = #error;            \
		break;
#endif
	switch (native_error)
	{
#ifndef _WIN32
		ERROR_CASE(ENOENT);
		ERROR_CASE(ENOMEM);
		ERROR_CASE(EBUSY);
		ERROR_CASE(ENOSPC);
		ERROR_CASE(EPIPE);
#endif

		// TODO: We don't currently support EFAULT or EINTR
		// ERROR_CASE(EFAULT);
		// ERROR_CASE(EINTR);

		ERROR_CASE(EBADF);
		ERROR_CASE(EACCES);
		ERROR_CASE(EINVAL);
		ERROR_CASE(EMFILE);
		ERROR_CASE(EWOULDBLOCK);
		ERROR_CASE(EINPROGRESS);
		ERROR_CASE(EALREADY);
		ERROR_CASE(EDESTADDRREQ);
		ERROR_CASE(EMSGSIZE);
		ERROR_CASE(EPROTOTYPE);
		ERROR_CASE(ENOPROTOOPT);
		ERROR_CASE(EPROTONOSUPPORT);
		ERROR_CASE(EOPNOTSUPP);
		ERROR_CASE(EPFNOSUPPORT);
		ERROR_CASE(EAFNOSUPPORT);
		ERROR_CASE(EADDRINUSE);
		ERROR_CASE(EADDRNOTAVAIL);
		ERROR_CASE(ENETDOWN);
		ERROR_CASE(ENETUNREACH);
		ERROR_CASE(ECONNABORTED);
		ERROR_CASE(ECONNRESET);
		ERROR_CASE(ENOBUFS);
		ERROR_CASE(EISCONN);
		ERROR_CASE(ENOTCONN);
		ERROR_CASE(ESHUTDOWN);
		ERROR_CASE(ETOOMANYREFS);
		ERROR_CASE(ETIMEDOUT);
		ERROR_CASE(ECONNREFUSED);
		ERROR_CASE(EHOSTDOWN);
		ERROR_CASE(EHOSTUNREACH);
#ifdef _WIN32
	// Windows likes to be special with unique errors
	case WSAENETRESET:
		result = SYS_NET_ECONNRESET;
		name = "WSAENETRESET";
		break;
#endif
	default:
		fmt::throw_exception("sys_net get_last_error(is_blocking=%d, native_error=%d): Unknown/illegal socket error", is_blocking, native_error);
	}

#ifdef _WIN32
	if (is_connecting)
	{
		// Windows will return SYS_NET_ENOTCONN when recvfrom/sendto is called on a socket that is connecting but not yet connected
		if (result == SYS_NET_ENOTCONN)
			return SYS_NET_EAGAIN;
	}
#endif

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

sys_net_error get_last_error(bool is_blocking, bool is_connecting)
{
	return convert_error(is_blocking, get_native_error(), is_connecting);
}

sys_net_sockaddr native_addr_to_sys_net_addr(const ::sockaddr_storage& native_addr)
{
	ensure(native_addr.ss_family == AF_INET || native_addr.ss_family == AF_UNSPEC);

	sys_net_sockaddr sn_addr;

	sys_net_sockaddr_in* paddr = reinterpret_cast<sys_net_sockaddr_in*>(&sn_addr);
	*paddr = {};

	paddr->sin_len = sizeof(sys_net_sockaddr_in);
	paddr->sin_family = SYS_NET_AF_INET;
	paddr->sin_port = std::bit_cast<be_t<u16>, u16>(reinterpret_cast<const sockaddr_in*>(&native_addr)->sin_port);
	paddr->sin_addr = std::bit_cast<be_t<u32>, u32>(reinterpret_cast<const sockaddr_in*>(&native_addr)->sin_addr.s_addr);

	return sn_addr;
}

::sockaddr_in sys_net_addr_to_native_addr(const sys_net_sockaddr& sn_addr)
{
	ensure(sn_addr.sa_family == SYS_NET_AF_INET);

	const sys_net_sockaddr_in* psa_in = reinterpret_cast<const sys_net_sockaddr_in*>(&sn_addr);

	::sockaddr_in native_addr{};
	native_addr.sin_family = AF_INET;
	native_addr.sin_port = std::bit_cast<u16>(psa_in->sin_port);
	native_addr.sin_addr.s_addr = std::bit_cast<u32>(psa_in->sin_addr);

#ifdef _WIN32
	// Windows doesn't support sending packets to 0.0.0.0 but it works on unixes, send to 127.0.0.1 instead
	if (native_addr.sin_addr.s_addr == 0x00000000)
	{
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		if (const u32 bind_addr = nph.get_bind_ip(); bind_addr != 0)
		{
			// If bind IP is set 0.0.0.0 was bound to binding_ip so we need to connect to that ip
			sys_net.warning("[Native] Redirected 0.0.0.0 to %s", np::ip_to_string(bind_addr));
			native_addr.sin_addr.s_addr = bind_addr;
		}
		else
		{
			// Otherwise we connect to localhost which should be bound
			sys_net.warning("[Native] Redirected 0.0.0.0 to 127.0.0.1");
			native_addr.sin_addr.s_addr = std::bit_cast<u32, be_t<u32>>(0x7F000001);
		}
	}
#endif

	return native_addr;
}

bool is_ip_public_address(const ::sockaddr_in& addr)
{
	const u8* ip = reinterpret_cast<const u8*>(&addr.sin_addr.s_addr);

	if ((ip[0] == 10) ||
		(ip[0] == 127) ||
		(ip[0] == 172 && (ip[1] >= 16 && ip[1] <= 31)) ||
		(ip[0] == 192 && ip[1] == 168))
	{
		return false;
	}

	return true;
}

u32 network_clear_queue(ppu_thread& ppu)
{
	u32 cleared = 0;

	idm::select<lv2_socket>([&](u32, lv2_socket& sock)
		{
			cleared += sock.clear_queue(&ppu);
		});

	return cleared;
}

void clear_ppu_to_awake(ppu_thread& ppu)
{
	g_fxo->get<network_context>().del_ppu_to_awake(&ppu);
	g_fxo->get<p2p_context>().del_ppu_to_awake(&ppu);
}

be_t<u32> resolve_binding_ip()
{
	in_addr conv{};
	const std::string cfg_bind_addr = g_cfg.net.bind_address.to_string();

	if (cfg_bind_addr == "0.0.0.0" || cfg_bind_addr == "")
	{
		return 0;
	}

	if (!inet_pton(AF_INET, cfg_bind_addr.c_str(), &conv))
	{
		// Do not set to disconnected on invalid IP just error and continue using default (0.0.0.0)
		sys_net.error("Provided IP(%s) address for bind is invalid!", g_cfg.net.bind_address.to_string());
		return 0;
	}

	return conv.s_addr;
}

#ifdef _WIN32
// Workaround function for WSAPoll not reporting failed connections
// Note that this was fixed in Windows 10 version 2004 (after more than 10 years lol)
void windows_poll(std::vector<pollfd>& fds, unsigned long nfds, int timeout, std::vector<bool>& connecting)
{
	ensure(fds.size() >= nfds);
	ensure(connecting.size() >= nfds);

	// Don't call WSAPoll with zero nfds (errors 10022 or 10038)
	if (std::none_of(fds.begin(), fds.begin() + nfds, [](pollfd& pfd)
			{
				return pfd.fd != INVALID_SOCKET;
			}))
	{
		if (timeout > 0)
		{
			Sleep(timeout);
		}

		return;
	}

	int r = ::WSAPoll(fds.data(), nfds, timeout);

	if (r == SOCKET_ERROR)
	{
		sys_net.error("WSAPoll failed: %s", fmt::win_error{static_cast<unsigned long>(WSAGetLastError()), nullptr});
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
