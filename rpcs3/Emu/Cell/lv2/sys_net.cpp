#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Utilities/Thread.h"

#include "sys_sync.h"
#include "sys_net.h"

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



logs::channel sys_net("sys_net");

static std::vector<ppu_thread*> s_to_awake;

static semaphore<> s_nw_mutex;

extern u64 get_system_time();

// Error helper functions
static s32 get_last_error(bool is_blocking, int native_error = 0)
{
	// Convert the error code for socket functions to a one for sys_net
	s32 result;
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
	default: sys_net.error("Unknown/illegal socket error: %d", native_error);
	}

	if (name && result != SYS_NET_EWOULDBLOCK && result != SYS_NET_EINPROGRESS)
	{
		sys_net.error("Socket error %s", name);
	}

	if (is_blocking && result == SYS_NET_EWOULDBLOCK)
	{
		return 0;
	}

	if (is_blocking && result == SYS_NET_EINPROGRESS)
	{
		return 0;
	}

	return result;
#undef ERROR_CASE
}

static void network_clear_queue(ppu_thread& ppu)
{
	idm::select<lv2_socket>([&](u32, lv2_socket& sock)
	{
		semaphore_lock lock(sock.mutex);

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
			sock.events = {};
		}
	});
}

extern void network_thread_init()
{
	thread_ctrl::spawn("Network Thread", []()
	{
		std::vector<std::shared_ptr<lv2_socket>> socklist;
		socklist.reserve(lv2_socket::id_count);

		s_to_awake.clear();

#ifdef _WIN32
		HANDLE _eventh = CreateEventW(nullptr, false, false, nullptr);

		WSADATA wsa_data;
		WSAStartup(MAKEWORD(2, 2), &wsa_data);
#else
		::pollfd fds[lv2_socket::id_count]{};
#endif

		do
		{
			// Wait with 1ms timeout
#ifdef _WIN32
			WaitForSingleObjectEx(_eventh, 1, false);
#else
			::poll(fds, socklist.size(), 1);
#endif

			semaphore_lock lock(s_nw_mutex);

			for (std::size_t i = 0; i < socklist.size(); i++)
			{
				bs_t<lv2_socket::poll> events{};

				lv2_socket& sock = *socklist[i];

#ifdef _WIN32
				WSANETWORKEVENTS nwe;
				if (WSAEnumNetworkEvents(sock.socket, nullptr, &nwe) == 0)
				{
					sock.ev_set |= nwe.lNetworkEvents;

					if (sock.ev_set & (FD_READ | FD_ACCEPT | FD_CLOSE) && sock.events.test_and_reset(lv2_socket::poll::read))
						events += lv2_socket::poll::read;
					if (sock.ev_set & (FD_WRITE | FD_CONNECT) && sock.events.test_and_reset(lv2_socket::poll::write))
						events += lv2_socket::poll::write;

					if ((nwe.lNetworkEvents & FD_READ && nwe.iErrorCode[FD_READ_BIT]) ||
						(nwe.lNetworkEvents & FD_ACCEPT && nwe.iErrorCode[FD_ACCEPT_BIT]) ||
						(nwe.lNetworkEvents & FD_CLOSE && nwe.iErrorCode[FD_CLOSE_BIT]) ||
						(nwe.lNetworkEvents & FD_WRITE && nwe.iErrorCode[FD_WRITE_BIT]) ||
						(nwe.lNetworkEvents & FD_CONNECT && nwe.iErrorCode[FD_CONNECT_BIT]))
					{
						// TODO
						if (sock.events.test_and_reset(lv2_socket::poll::error))
							events += lv2_socket::poll::error;
					}
				}
				else
				{
					sys_net.error("WSAEnumNetworkEvents() failed (s=%d)", i);
				}
#else
				if (fds[i].revents & (POLLIN | POLLHUP) && socklist[i]->events.test_and_reset(lv2_socket::poll::read))
					events += lv2_socket::poll::read;
				if (fds[i].revents & POLLOUT && socklist[i]->events.test_and_reset(lv2_socket::poll::write))
					events += lv2_socket::poll::write;
				if (fds[i].revents & POLLERR && socklist[i]->events.test_and_reset(lv2_socket::poll::error))
					events += lv2_socket::poll::error;
#endif

				if (test(events))
				{
					semaphore_lock lock(socklist[i]->mutex);

					for (auto it = socklist[i]->queue.begin(); test(events) && it != socklist[i]->queue.end();)
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
						socklist[i]->events = {};
					}
				}
			}

			s_to_awake.erase(std::unique(s_to_awake.begin(), s_to_awake.end()), s_to_awake.end());

			for (ppu_thread* ppu : s_to_awake)
			{
				network_clear_queue(*ppu);
				lv2_obj::awake(*ppu);
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
				auto events = socklist[i]->events.load();

#ifdef _WIN32
				verify(HERE), 0 == WSAEventSelect(socklist[i]->socket, _eventh, FD_READ | FD_ACCEPT | FD_CLOSE | FD_WRITE | FD_CONNECT);
#else
				fds[i].fd = test(events) ? socklist[i]->socket : -1;
				fds[i].events =
					(test(events, lv2_socket::poll::read) ? POLLIN : 0) |
					(test(events, lv2_socket::poll::write) ? POLLOUT : 0) |
					0;
				fds[i].revents = 0;
#endif
			}
		}
		while (!Emu.IsStopped());

#ifdef _WIN32
		CloseHandle(_eventh);
		WSACleanup();
#endif
	});
}

lv2_socket::lv2_socket(lv2_socket::socket_type s)
	: socket(s)
{
	// Set non-blocking
#ifdef _WIN32
	u_long _true = 1;
	::ioctlsocket(socket, FIONBIO, &_true);
#else
	::fcntl(socket, F_SETFL, ::fcntl(socket, F_GETFL, 0) | O_NONBLOCK);
#endif
}

lv2_socket::~lv2_socket()
{
#ifdef _WIN32
	::closesocket(socket);
#else
	::close(socket);
#endif
}

s32 sys_net_bnet_accept(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	sys_net.warning("sys_net_bnet_accept(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	lv2_socket::socket_type native_socket = -1;
	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);
	s32 result = 0;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		semaphore_lock lock(sock.mutex);

		//if (!test(sock.events, lv2_socket::poll::read))
		{
#ifdef _WIN32
			sock.ev_set &= ~FD_ACCEPT;
#endif
			native_socket = ::accept(sock.socket, (::sockaddr*)&native_addr, &native_addrlen);

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
			if (test(events, lv2_socket::poll::read))
			{
#ifdef _WIN32
				sock.ev_set &= ~FD_ACCEPT;
#endif
				native_socket = ::accept(sock.socket, (::sockaddr*)&native_addr, &native_addrlen);

				if (native_socket != -1 || (result = get_last_error(!sock.so_nbio)))
				{
					lv2_obj::awake(ppu);
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
		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			thread_ctrl::wait();
		}

		if (result)
		{
			return -result;
		}

		if (ppu.gpr[3] == -SYS_NET_EINTR)
		{
			return -SYS_NET_EINTR;
		}
	}

	auto newsock = std::make_shared<lv2_socket>(native_socket);

	result = idm::import_existing<lv2_socket>(newsock);

	if (result == id_manager::id_traits<lv2_socket>::invalid)
	{
		return -SYS_NET_EMFILE;
	}

	if (addr)
	{
		verify(HERE), native_addr.ss_family == AF_INET;

		vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

		if (paddrlen)
		{
			*paddrlen     = sizeof(sys_net_sockaddr_in);
		}

		paddr->sin_len    = sizeof(sys_net_sockaddr_in);
		paddr->sin_family = SYS_NET_AF_INET;
		paddr->sin_port   = ntohs(((::sockaddr_in*)&native_addr)->sin_port);
		paddr->sin_addr   = ntohl(((::sockaddr_in*)&native_addr)->sin_addr.s_addr);
		paddr->sin_zero   = 0;
	}

	// Socket id
	return result;
}

s32 sys_net_bnet_bind(ppu_thread& ppu, s32 s, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	sys_net.warning("sys_net_bnet_bind(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	if (addr->sa_family != SYS_NET_AF_INET)
	{
		sys_net.error("sys_net_bnet_bind(s=%d): unsupported sa_family (%d)", s, addr->sa_family);
		return -SYS_NET_EAFNOSUPPORT;
	}

	::sockaddr_in name{};
	name.sin_family      = AF_INET;
	name.sin_port        = htons(((sys_net_sockaddr_in*)addr.get_ptr())->sin_port);
	name.sin_addr.s_addr = htonl(((sys_net_sockaddr_in*)addr.get_ptr())->sin_addr);
	::socklen_t namelen  = sizeof(name);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		if (::bind(sock.socket, (::sockaddr*)&name, namelen) == 0)
		{
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_connect(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, u32 addrlen)
{
	sys_net.warning("sys_net_bnet_connect(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	s32 result = 0;
	::sockaddr_in name{};
	name.sin_family      = AF_INET;
	name.sin_port        = htons(((sys_net_sockaddr_in*)addr.get_ptr())->sin_port);
	name.sin_addr.s_addr = htonl(((sys_net_sockaddr_in*)addr.get_ptr())->sin_addr);
	::socklen_t namelen  = sizeof(name);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		semaphore_lock lock(sock.mutex);

		if (addr->sa_family == 0 && !((sys_net_sockaddr_in*)addr.get_ptr())->sin_port && !((sys_net_sockaddr_in*)addr.get_ptr())->sin_addr)
		{
			// Hack for DNS (8.8.8.8:53)
			name.sin_port        = htons(53);
			name.sin_addr.s_addr = 0x08080808;

			// Overwrite arg (probably used to validate recvfrom addr)
			((sys_net_sockaddr_in*)addr.get_ptr())->sin_family = SYS_NET_AF_INET;
			((sys_net_sockaddr_in*)addr.get_ptr())->sin_port   = 53;
			((sys_net_sockaddr_in*)addr.get_ptr())->sin_addr   = 0x08080808;
			sys_net.warning("sys_net_bnet_connect(s=%d): using DNS 8.8.8.8:53...");
		}
		else if (addr->sa_family != SYS_NET_AF_INET)
		{
			sys_net.error("sys_net_bnet_connect(s=%d): unsupported sa_family (%d)", s, addr->sa_family);
		}

		if (::connect(sock.socket, (::sockaddr*)&name, namelen) == 0)
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
				sock.events += lv2_socket::poll::write;
				sock.queue.emplace_back(u32{0}, [&sock](bs_t<lv2_socket::poll> events) -> bool
				{
					if (test(events, lv2_socket::poll::write))
					{
#ifdef _WIN32
						sock.ev_set &= ~FD_CONNECT;
#endif
						int native_error;
						::socklen_t size = sizeof(native_error);
						if (::getsockopt(sock.socket, SOL_SOCKET, SO_ERROR, (char*)&native_error, &size) != 0 || size != sizeof(int))
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

		sock.events += lv2_socket::poll::write;
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (test(events, lv2_socket::poll::write))
			{
#ifdef _WIN32
				sock.ev_set &= ~FD_CONNECT;
#endif
				int native_error;
				::socklen_t size = sizeof(native_error);
				if (::getsockopt(sock.socket, SOL_SOCKET, SO_ERROR, (char*)&native_error, &size) != 0 || size != sizeof(int))
				{
					result = 1;
				}
				else
				{
					// TODO: check error formats (both native and translated)
					result = native_error ? get_last_error(false, native_error) : 0;
				}

				lv2_obj::awake(ppu);
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

	if (!sock.ret && result)
	{
		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			thread_ctrl::wait();
		}

		if (result)
		{
			return -result;
		}

		if (ppu.gpr[3] == -SYS_NET_EINTR)
		{
			return -SYS_NET_EINTR;
		}
	}

	return CELL_OK;
}

s32 sys_net_bnet_getpeername(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	sys_net.warning("sys_net_bnet_getpeername(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		::sockaddr_storage native_addr;
		::socklen_t native_addrlen = sizeof(native_addr);

		if (::getpeername(sock.socket, (::sockaddr*)&native_addr, &native_addrlen) == 0)
		{
			verify(HERE), native_addr.ss_family == AF_INET;

			vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

			if (paddrlen)
			{
				*paddrlen     = sizeof(sys_net_sockaddr_in);
			}

			paddr->sin_len    = sizeof(sys_net_sockaddr_in);
			paddr->sin_family = SYS_NET_AF_INET;
			paddr->sin_port   = ntohs(((::sockaddr_in*)&native_addr)->sin_port);
			paddr->sin_addr   = ntohl(((::sockaddr_in*)&native_addr)->sin_addr.s_addr);
			paddr->sin_zero   = 0;
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_getsockname(ppu_thread& ppu, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	sys_net.warning("sys_net_bnet_getsockname(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		::sockaddr_storage native_addr;
		::socklen_t native_addrlen = sizeof(native_addr);

		if (::getsockname(sock.socket, (::sockaddr*)&native_addr, &native_addrlen) == 0)
		{
			verify(HERE), native_addr.ss_family == AF_INET;

			vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

			if (paddrlen)
			{
				*paddrlen     = sizeof(sys_net_sockaddr_in);
			}

			paddr->sin_len    = sizeof(sys_net_sockaddr_in);
			paddr->sin_family = SYS_NET_AF_INET;
			paddr->sin_port   = ntohs(((::sockaddr_in*)&native_addr)->sin_port);
			paddr->sin_addr   = ntohl(((::sockaddr_in*)&native_addr)->sin_addr.s_addr);
			paddr->sin_zero   = 0;
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_getsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::ptr<void> optval, vm::ptr<u32> optlen)
{
	sys_net.warning("sys_net_bnet_getsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=*0x%x)", s, level, optname, optval, optlen);

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

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		if (*optlen < sizeof(int))
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
				*(be_t<s32>*)optval.get_ptr() = sock.so_nbio;
				return 0;
			}
			case SYS_NET_SO_ERROR:
			{
				// Special
				*(be_t<s32>*)optval.get_ptr() = std::exchange(sock.so_error, 0);
				return 0;
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
				*(be_t<s32>*)optval.get_ptr() = sock.so_reuseaddr;
				return 0;
			}
			case SYS_NET_SO_REUSEPORT:
			{
				*(be_t<s32>*)optval.get_ptr() = sock.so_reuseport;
				return 0;
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
				if (*optlen < sizeof(sys_net_timeval))
					return SYS_NET_EINVAL;

				native_opt = optname == SYS_NET_SO_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
				break;
			}
			case SYS_NET_SO_LINGER:
			{
				if (*optlen < sizeof(sys_net_linger))
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
				*(be_t<s32>*)optval.get_ptr() = sock.so_tcp_maxseg;
				return 0;
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
				*(sys_net_timeval*)optval.get_ptr() = { ::narrow<s64>(native_val.timeo.tv_sec), ::narrow<s64>(native_val.timeo.tv_usec) };
				return 0;
			}
			case SYS_NET_SO_LINGER:
			{
				// TODO
				*(sys_net_linger*)optval.get_ptr() = { ::narrow<s32>(native_val.linger.l_onoff), ::narrow<s32>(native_val.linger.l_linger) };
				return 0;
			}
			}
		}

		// Fallback to int
		*(be_t<s32>*)optval.get_ptr() = native_val._int;
		return 0;
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_listen(ppu_thread& ppu, s32 s, s32 backlog)
{
	sys_net.warning("sys_net_bnet_listen(s=%d, backlog=%d)", s, backlog);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		if (::listen(sock.socket, backlog) == 0)
		{
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_recvfrom(ppu_thread& ppu, s32 s, vm::ptr<void> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	sys_net.warning("sys_net_bnet_recvfrom(s=%d, buf=*0x%x, len=%u, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);

	if (flags & ~(SYS_NET_MSG_PEEK | SYS_NET_MSG_DONTWAIT | SYS_NET_MSG_WAITALL))
	{
		fmt::throw_exception("sys_net_bnet_recvfrom(s=%d): unknown flags (0x%x)", flags);
	}

	int native_flags = 0;
	int native_result = -1;
	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);
	s32 result = 0;

	if (flags & SYS_NET_MSG_PEEK)
	{
		native_flags |= MSG_PEEK;
	}

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		semaphore_lock lock(sock.mutex);

		//if (!test(sock.events, lv2_socket::poll::read))
		{
#ifdef _WIN32
			if (!(native_flags & MSG_PEEK)) sock.ev_set &= ~FD_READ;
#endif
			native_result = ::recvfrom(sock.socket, (char*)buf.get_ptr(), len, native_flags, (::sockaddr*)&native_addr, &native_addrlen);

			if (native_result >= 0)
			{
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
		sock.queue.emplace_back(ppu.id, [&](bs_t<lv2_socket::poll> events) -> bool
		{
			if (test(events, lv2_socket::poll::read))
			{
#ifdef _WIN32
				if (!(native_flags & MSG_PEEK)) sock.ev_set &= ~FD_READ;
#endif
				native_result = ::recvfrom(sock.socket, (char*)buf.get_ptr(), len, native_flags, (::sockaddr*)&native_addr, &native_addrlen);

				if (native_result >= 0 || (result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0)))
				{
					lv2_obj::awake(ppu);
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
		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			thread_ctrl::wait();
		}

		if (result)
		{
			return -result;
		}

		if (ppu.gpr[3] == -SYS_NET_EINTR)
		{
			return -SYS_NET_EINTR;
		}
	}

	// TODO
	if (addr)
	{
		verify(HERE), native_addr.ss_family == AF_INET;

		vm::ptr<sys_net_sockaddr_in> paddr = vm::cast(addr.addr());

		if (paddrlen)
		{
			*paddrlen     = sizeof(sys_net_sockaddr_in);
		}

		paddr->sin_len    = sizeof(sys_net_sockaddr_in);
		paddr->sin_family = SYS_NET_AF_INET;
		paddr->sin_port   = ntohs(((::sockaddr_in*)&native_addr)->sin_port);
		paddr->sin_addr   = ntohl(((::sockaddr_in*)&native_addr)->sin_addr.s_addr);
		paddr->sin_zero   = 0;
	}

	// Length
	return native_result;
}

s32 sys_net_bnet_recvmsg(ppu_thread& ppu, s32 s, vm::ptr<sys_net_msghdr> msg, s32 flags)
{
	sys_net.todo("sys_net_bnet_recvmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);
	return 0;
}

s32 sys_net_bnet_sendmsg(ppu_thread& ppu, s32 s, vm::cptr<sys_net_msghdr> msg, s32 flags)
{
	sys_net.todo("sys_net_bnet_sendmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);
	return 0;
}

s32 sys_net_bnet_sendto(ppu_thread& ppu, s32 s, vm::cptr<void> buf, u32 len, s32 flags, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
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

	int native_flags = 0;
	int native_result = -1;
	::sockaddr_in name{};

	if (addr)
	{
		name.sin_family      = AF_INET;
		name.sin_port        = htons(((sys_net_sockaddr_in*)addr.get_ptr())->sin_port);
		name.sin_addr.s_addr = htonl(((sys_net_sockaddr_in*)addr.get_ptr())->sin_addr);
	}

	::socklen_t namelen = sizeof(name);
	s32 result = 0;

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		semaphore_lock lock(sock.mutex);

		//if (!test(sock.events, lv2_socket::poll::write))
		{
#ifdef _WIN32
			sock.ev_set &= ~FD_WRITE;
#endif
			native_result = ::sendto(sock.socket, (const char*)buf.get_ptr(), len, native_flags, addr ? (::sockaddr*)&name : nullptr, addr ? namelen : 0);

			if (native_result >= 0)
			{
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
			if (test(events, lv2_socket::poll::write))
			{
#ifdef _WIN32
				sock.ev_set &= ~FD_WRITE;
#endif
				native_result = ::sendto(sock.socket, (const char*)buf.get_ptr(), len, native_flags, addr ? (::sockaddr*)&name : nullptr, addr ? namelen : 0);

				if (native_result >= 0 || (result = get_last_error(!sock.so_nbio && (flags & SYS_NET_MSG_DONTWAIT) == 0)))
				{
					lv2_obj::awake(ppu);
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
		return -result;
	}

	if (!sock.ret)
	{
		while (!ppu.state.test_and_reset(cpu_flag::signal))
		{
			thread_ctrl::wait();
		}

		if (result)
		{
			return -result;
		}

		if (ppu.gpr[3] == -SYS_NET_EINTR)
		{
			return -SYS_NET_EINTR;
		}
	}

	// Length
	return native_result;
}

s32 sys_net_bnet_setsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
{
	sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=%u)", s, level, optname, optval, optlen);

	int native_int = 0;
	int native_level = -1;
	int native_opt = -1;
	const void* native_val = &native_int;
	::socklen_t native_len = sizeof(int);
	::timeval native_timeo;
	::linger native_linger;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

		if (optlen >= sizeof(int))
		{
			native_int = *(const be_t<s32>*)optval.get_ptr();
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
				return 0;
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
				native_timeo.tv_sec = ::narrow<int>(((const sys_net_timeval*)optval.get_ptr())->tv_sec);
				native_timeo.tv_usec = ::narrow<int>(((const sys_net_timeval*)optval.get_ptr())->tv_usec);
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
				native_linger.l_onoff = ((const sys_net_linger*)optval.get_ptr())->l_onoff;
				native_linger.l_linger = ((const sys_net_linger*)optval.get_ptr())->l_linger;
				break;
			}
			case SYS_NET_SO_USECRYPTO:
			{
				//TODO
				sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USECRYPTO)", s, optname);
				return 0;
			}
			case SYS_NET_SO_USESIGNATURE:
			{
				//TODO
				sys_net.error("sys_net_bnet_setsockopt(s=%d, SOL_SOCKET): Stubbed option (0x%x) (SYS_NET_SO_USESIGNATURE)", s, optname);
				return 0;
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
				return 0;
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

		if (::setsockopt(sock.socket, native_level, native_opt, (const char*)native_val, native_len) == 0)
		{
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_shutdown(ppu_thread& ppu, s32 s, s32 how)
{
	sys_net.warning("sys_net_bnet_shutdown(s=%d, how=%d)", s, how);

	if (how < 0 || how > 2)
	{
		return -SYS_NET_EINVAL;
	}

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> s32
	{
		semaphore_lock lock(sock.mutex);

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
			return 0;
		}

		return get_last_error(false);
	});

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	return -sock.ret;
}

s32 sys_net_bnet_socket(ppu_thread& ppu, s32 family, s32 type, s32 protocol)
{
	sys_net.warning("sys_net_bnet_socket(family=%d, type=%d, protocol=%d)", family, type, protocol);

	if (family != SYS_NET_AF_INET && family != SYS_NET_AF_UNSPEC)
	{
		sys_net.error("sys_net_bnet_socket(): unknown family (%d)", family);
	}

	if (type != SYS_NET_SOCK_STREAM && type != SYS_NET_SOCK_DGRAM && type != SYS_NET_SOCK_DGRAM_P2P)
	{
		sys_net.error("sys_net_bnet_socket(): unsupported type (%d)", type);
		return -SYS_NET_EPROTONOSUPPORT;
	}

	const int native_domain = AF_INET;

	const int native_type =
		type == SYS_NET_SOCK_STREAM ? SOCK_STREAM :
		type == SYS_NET_SOCK_DGRAM ? SOCK_DGRAM :
		type == SYS_NET_SOCK_DGRAM_P2P ? SOCK_DGRAM : SOCK_RAW;

	int native_proto =
		protocol == SYS_NET_IPPROTO_IP ? IPPROTO_IP :
		protocol == SYS_NET_IPPROTO_ICMP ? IPPROTO_ICMP :
		protocol == SYS_NET_IPPROTO_IGMP ? IPPROTO_IGMP :
		protocol == SYS_NET_IPPROTO_TCP ? IPPROTO_TCP :
		protocol == SYS_NET_IPPROTO_UDP ? IPPROTO_UDP :
		protocol == SYS_NET_IPPROTO_ICMPV6 ? IPPROTO_ICMPV6 : 0;

	if (native_domain == AF_UNSPEC && type == SYS_NET_SOCK_DGRAM)
	{
		//Windows gets all errory if you try a unspec socket with protocol 0
		native_proto = IPPROTO_UDP;
	}

	const auto native_socket = ::socket(native_domain, native_type, native_proto);

	if (native_socket == -1)
	{
		return -get_last_error(false);
	}

	const s32 s = idm::import_existing<lv2_socket>(std::make_shared<lv2_socket>(native_socket));

	if (s == id_manager::id_traits<lv2_socket>::invalid)
	{
		return -SYS_NET_EMFILE;
	}

	return s;
}

s32 sys_net_bnet_close(ppu_thread& ppu, s32 s)
{
	sys_net.warning("sys_net_bnet_close(s=%d)", s);

	const auto sock = idm::withdraw<lv2_socket>(s);

	if (!sock)
	{
		return -SYS_NET_EBADF;
	}

	if (!sock->queue.empty())
		sys_net.fatal("CLOSE");

	return 0;
}

s32 sys_net_bnet_poll(ppu_thread& ppu, vm::ptr<sys_net_pollfd> fds, s32 nfds, s32 ms)
{
	sys_net.warning("sys_net_bnet_poll(fds=*0x%x, nfds=%d, ms=%d)", fds, nfds, ms);

	atomic_t<s32> signaled{0};

	u64 timeout = ms < 0 ? 0 : ms * 1000ull;

	if (nfds)
	{
		semaphore_lock nw_lock(s_nw_mutex);

		reader_lock lock(id_manager::g_mutex);

#ifndef _WIN32
		::pollfd _fds[1024]{};
#endif

		for (s32 i = 0; i < nfds; i++)
		{
#ifndef _WIN32
			_fds[i].fd = -1;
#endif
			fds[i].revents = 0;

			if (fds[i].fd < 0)
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>(fds[i].fd))
			{
				if (fds[i].events & ~(SYS_NET_POLLIN | SYS_NET_POLLOUT))
					sys_net.error("sys_net_bnet_poll(fd=%d): events=0x%x", fds[i].fd, fds[i].events);
#ifdef _WIN32
				if (fds[i].events & SYS_NET_POLLIN && sock->ev_set & (FD_READ | FD_ACCEPT | FD_CLOSE))
					fds[i].revents |= SYS_NET_POLLIN;
				if (fds[i].events & SYS_NET_POLLOUT && sock->ev_set & (FD_WRITE | FD_CONNECT))
					fds[i].revents |= SYS_NET_POLLOUT;

				if (fds[i].revents)
				{
					signaled++;
				}
#else
				_fds[i].fd = sock->socket;
				if (fds[i].events & SYS_NET_POLLIN)
					_fds[i].events |= POLLIN;
				if (fds[i].events & SYS_NET_POLLOUT)
					_fds[i].events |= POLLOUT;
#endif
			}
			else
			{
				fds[i].revents |= SYS_NET_POLLNVAL;
				signaled++;
			}
		}

#ifndef _WIN32
		::poll(_fds, nfds, 0);

		for (s32 i = 0; i < nfds; i++)
		{
			if (_fds[i].revents & (POLLIN | POLLHUP))
				fds[i].revents |= SYS_NET_POLLIN;
			if (_fds[i].revents & POLLOUT)
				fds[i].revents |= SYS_NET_POLLOUT;
			if (_fds[i].revents & POLLERR)
				fds[i].revents |= SYS_NET_POLLERR;

			if (fds[i].revents)
			{
				signaled++;
			}
		}
#endif

		if (ms == 0 || signaled)
		{
			return signaled;
		}

		for (s32 i = 0; i < nfds; i++)
		{
			if (fds[i].fd < 0)
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>(fds[i].fd))
			{
				semaphore_lock lock(sock->mutex);

				bs_t<lv2_socket::poll> selected = +lv2_socket::poll::error;

				if (fds[i].events & SYS_NET_POLLIN)
					selected += lv2_socket::poll::read;
				if (fds[i].events & SYS_NET_POLLOUT)
					selected += lv2_socket::poll::write;
				//if (fds[i].events & SYS_NET_POLLPRI) // Unimplemented
				//	selected += lv2_socket::poll::error;

				sock->events += selected;
				sock->queue.emplace_back(ppu.id, [sock, selected, fds, i, &signaled, &ppu](bs_t<lv2_socket::poll> events)
				{
					if (test(events, selected))
					{
						if (test(events, selected & lv2_socket::poll::read))
							fds[i].revents |= SYS_NET_POLLIN;
						if (test(events, selected & lv2_socket::poll::write))
							fds[i].revents |= SYS_NET_POLLOUT;
						if (test(events, selected & lv2_socket::poll::error))
							fds[i].revents |= SYS_NET_POLLERR;

						signaled++;
						s_to_awake.emplace_back(&ppu);
						return true;
					}

					sock->events += selected;
					return false;
				});
			}
		}

		lv2_obj::sleep(ppu, timeout);
	}
	else
	{
		return 0;
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock nw_lock(s_nw_mutex);

				if (signaled)
				{
					timeout = 0;
					continue;
				}

				network_clear_queue(ppu);
				break;
			}

			thread_ctrl::wait_for(timeout - passed);
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	return signaled;
}

s32 sys_net_bnet_select(ppu_thread& ppu, s32 nfds, vm::ptr<sys_net_fd_set> readfds, vm::ptr<sys_net_fd_set> writefds, vm::ptr<sys_net_fd_set> exceptfds, vm::ptr<sys_net_timeval> _timeout)
{
	sys_net.warning("sys_net_bnet_select(nfds=%d, readfds=*0x%x, writefds=*0x%x, exceptfds=*0x%x, timeout=*0x%x)", nfds, readfds, writefds, exceptfds, _timeout);

	atomic_t<s32> signaled{0};

	if (exceptfds)
	{
		sys_net.error("sys_net_bnet_select(): exceptfds not implemented");
	}

	sys_net_fd_set rread{};
	sys_net_fd_set rwrite{};
	sys_net_fd_set rexcept{};
	u64 timeout = !_timeout ? 0 : _timeout->tv_sec * 1000000ull + _timeout->tv_usec;

	if (nfds >= 0)
	{
		semaphore_lock nw_lock(s_nw_mutex);

		reader_lock lock(id_manager::g_mutex);

#ifndef _WIN32
		::pollfd _fds[1024]{};
#endif

		for (s32 i = 0; i < nfds; i++)
		{
#ifndef _WIN32
			_fds[i].fd = -1;
#endif
			bs_t<lv2_socket::poll> selected{};

			if (readfds && readfds->bit(i))
				selected += lv2_socket::poll::read;
			if (writefds && writefds->bit(i))
				selected += lv2_socket::poll::write;
			//if (exceptfds && exceptfds->bit(i))
			//	selected += lv2_socket::poll::error;

			if (test(selected))
			{
				selected += lv2_socket::poll::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{
#ifdef _WIN32
				bool sig = false;
				if (sock->ev_set & (FD_READ | FD_ACCEPT | FD_CLOSE) && test(selected, lv2_socket::poll::read))
					sig = true, rread.set(i);
				if (sock->ev_set & (FD_WRITE | FD_CONNECT) && test(selected, lv2_socket::poll::write))
					sig = true, rwrite.set(i);

				if (sig)
				{
					signaled++;
				}
#else
				_fds[i].fd = sock->socket;
				if (test(selected, lv2_socket::poll::read))
					_fds[i].events |= POLLIN;
				if (test(selected, lv2_socket::poll::write))
					_fds[i].events |= POLLOUT;
#endif
			}
			else
			{
				return -SYS_NET_EBADF;
			}
		}

#ifndef _WIN32
		::poll(_fds, nfds, 0);

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
#endif

		if ((_timeout && !timeout) || signaled)
		{
			if (readfds)
				*readfds = rread;
			if (writefds)
				*writefds = rwrite;
			if (exceptfds)
				*exceptfds = rexcept;
			return signaled;
		}

		for (s32 i = 0; i < nfds; i++)
		{
			bs_t<lv2_socket::poll> selected{};

			if (readfds && readfds->bit(i))
				selected += lv2_socket::poll::read;
			if (writefds && writefds->bit(i))
				selected += lv2_socket::poll::write;
			//if (exceptfds && exceptfds->bit(i))
			//	selected += lv2_socket::poll::error;

			if (test(selected))
			{
				selected += lv2_socket::poll::error;
			}
			else
			{
				continue;
			}

			if (auto sock = idm::check_unlocked<lv2_socket>((lv2_socket::id_base & -1024) + i))
			{
				semaphore_lock lock(sock->mutex);

				sock->events += selected;
				sock->queue.emplace_back(ppu.id, [sock, selected, i, &rread, &rwrite, &rexcept, &signaled, &ppu](bs_t<lv2_socket::poll> events)
				{
					if (test(events, selected))
					{
						if (test(selected, lv2_socket::poll::read) && test(events, lv2_socket::poll::read + lv2_socket::poll::error))
							rread.set(i);
						if (test(selected, lv2_socket::poll::write) && test(events, lv2_socket::poll::write + lv2_socket::poll::error))
							rwrite.set(i);
						//if (test(events, selected & lv2_socket::poll::error))
						//	rexcept.set(i);

						signaled++;
						s_to_awake.emplace_back(&ppu);
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
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock nw_lock(s_nw_mutex);

				if (signaled)
				{
					timeout = 0;
					continue;
				}

				network_clear_queue(ppu);
				break;
			}

			thread_ctrl::wait_for(timeout - passed);
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	if (readfds)
		*readfds = rread;
	if (writefds)
		*writefds = rwrite;
	if (exceptfds)
		*exceptfds = rexcept;

	return signaled;
}

s32 _sys_net_open_dump(ppu_thread& ppu, s32 len, s32 flags)
{
	sys_net.todo("_sys_net_open_dump(len=%d, flags=0x%x)", len, flags);
	return 0;
}

s32 _sys_net_read_dump(ppu_thread& ppu, s32 id, vm::ptr<void> buf, s32 len, vm::ptr<s32> pflags)
{
	sys_net.todo("_sys_net_read_dump(id=0x%x, buf=*0x%x, len=%d, pflags=*0x%x)", id, buf, len, pflags);
	return 0;
}

s32 _sys_net_close_dump(ppu_thread& ppu, s32 id, vm::ptr<s32> pflags)
{
	sys_net.todo("_sys_net_close_dump(id=0x%x, pflags=*0x%x)", id, pflags);
	return 0;
}

s32 _sys_net_write_dump(ppu_thread& ppu, s32 id, vm::cptr<void> buf, s32 len, u32 unknown)
{
	sys_net.todo(__func__);
	return 0;
}

s32 sys_net_abort(ppu_thread& ppu, s32 type, u64 arg, s32 flags)
{
	sys_net.todo("sys_net_abort(type=%d, arg=0x%x, flags=0x%x)", type, arg, flags);
	return 0;
}

s32 sys_net_infoctl(ppu_thread& ppu, s32 cmd, vm::ptr<void> arg)
{
	sys_net.todo("sys_net_infoctl(cmd=%d, arg=*0x%x)", cmd, arg);
	return 0;
}

s32 sys_net_control(ppu_thread& ppu, u32 arg1, s32 arg2, vm::ptr<void> arg3, s32 arg4)
{
	sys_net.todo("sys_net_control(0x%x, %d, *0x%x, %d)", arg1, arg2, arg3, arg4);
	return 0;
}

s32 sys_net_bnet_ioctl(ppu_thread& ppu, s32 arg1, u32 arg2, u32 arg3)
{
	sys_net.todo("sys_net_bnet_ioctl(%d, 0x%x, 0x%x)", arg1, arg2, arg3);
	return 0;
}

s32 sys_net_bnet_sysctl(ppu_thread& ppu, u32 arg1, u32 arg2, u32 arg3, vm::ptr<void> arg4, u32 arg5, u32 arg6)
{
	sys_net.todo("sys_net_bnet_sysctl(0x%x, 0x%x, 0x%x, *0x%x, 0x%x, 0x%x)", arg1, arg2, arg3, arg4, arg5, arg6);
	return 0;
}

s32 sys_net_eurus_post_command(ppu_thread& ppu, s32 arg1, u32 arg2, u32 arg3)
{
	sys_net.todo("sys_net_eurus_post_command(%d, 0x%x, 0x%x)", arg1, arg2, arg3);
	return 0;
}
