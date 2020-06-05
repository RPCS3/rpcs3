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

struct network_thread
{
	std::vector<ppu_thread*> s_to_awake;

	shared_mutex s_nw_mutex;

	static constexpr auto thread_name = "Network Thread";

	network_thread() noexcept
	{
#ifdef _WIN32
		WSADATA wsa_data;
		WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
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

		while (thread_ctrl::state() != thread_state::aborting)
		{
			// Wait with 1ms timeout
#ifdef _WIN32
			windows_poll(fds, ::size32(socklist), 1, connecting);
#else
			::poll(fds, socklist.size(), 1);
#endif

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

lv2_socket::lv2_socket(lv2_socket::socket_type s, s32 s_type, s32 family)
	: socket(s), type{s_type}, family{family}
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

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		std::lock_guard lock(sock.mutex);

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

		if (sock.type == SYS_NET_SOCK_DGRAM_P2P)
		{
			const u16 daport = reinterpret_cast<const sys_net_sockaddr_in*>(addr_buf)->sin_port;
			const u16 davport = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(addr_buf)->sin_vport;
			sys_net.warning("Trying to bind %s:%d:%d", name.sin_addr, daport, davport);
			name.sin_port = std::bit_cast<u16, be_t<u16>>(daport + davport); // htons(daport + davport)
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

	sys_net.warning("Attempting to connect on %s:%d", name.sin_addr, std::bit_cast<be_t<u16>, u16>(name.sin_port)); // ntohs(name.sin_port)

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		std::lock_guard lock(sock.mutex);

		if (_addr->sa_family == 0 && !psa_in->sin_port && !psa_in->sin_addr)
		{
			const auto nph = g_fxo->get<named_thread<np_handler>>();

			// Hack for DNS (8.8.8.8:53)
			name.sin_port        = std::bit_cast<u16, be_t<u16>>(53);
			name.sin_addr.s_addr = nph->get_dns();

			// Overwrite arg (probably used to validate recvfrom addr)
			psa_in->sin_family = SYS_NET_AF_INET;
			psa_in->sin_port   = 53;
			psa_in->sin_addr   = nph->get_dns();
			addr_buf.changed = true;
			sys_net.warning("sys_net_bnet_connect: using DNS...");

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
	s32 type = 0;

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

		type = sock.type;

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

	if (type == SYS_NET_SOCK_DGRAM_P2P)
	{
		vm::ptr<sys_net_sockaddr_in_p2p> paddr_p2p = vm::cast(addr.addr());
		paddr_p2p->sin_vport                       = paddr_p2p->sin_port - 3658;
		paddr_p2p->sin_port                        = 3658;
		struct in_addr rep;
		rep.s_addr = htonl(paddr->sin_addr);
		sys_net.error("Reporting P2P socket address as %s:%d:%d", rep, paddr_p2p->sin_port, paddr_p2p->sin_vport);
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

	::sockaddr_storage native_addr;
	::socklen_t native_addrlen = sizeof(native_addr);

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

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

		if (::listen(sock.socket, backlog) == 0)
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
	std::string _buf;

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
				(reinterpret_cast<::sockaddr_in*>(&native_addr))->sin_addr.s_addr = nph->get_dns();

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
				native_result = ::recvfrom(sock.socket, _buf.data(), len, native_flags, reinterpret_cast<struct sockaddr*>(&native_addr), &native_addrlen);

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
			sys_net.error("Error recvfrom(bad socket)");
		return -SYS_NET_EBADF;
	}

	if (!sock.ret && result)
	{
		if (result == SYS_NET_EWOULDBLOCK)
		{
			return not_an_error(-result);
		}

		if (type == SYS_NET_SOCK_DGRAM_P2P)
			sys_net.error("Error recvfrom(result early): %d", result);

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
				sys_net.error("Error recvfrom(result): %d", result);
			return -result;
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			if (type == SYS_NET_SOCK_DGRAM_P2P)
				sys_net.error("Error recvfrom(interrupted)");
			return -SYS_NET_EINTR;
		}

		std::memcpy(buf.get_ptr(), _buf.c_str(), len);
	}

	if (ppu.is_stopped())
	{
		return 0;
	}

	// TODO
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

		if (type == SYS_NET_SOCK_DGRAM_P2P)
		{
			vm::ptr<sys_net_sockaddr_in_p2p> paddr_p2p = vm::cast(addr.addr());
			paddr_p2p->sin_vport                       = paddr_p2p->sin_port - 3658;
			paddr_p2p->sin_port                        = 3658;

			const u16 daport  = reinterpret_cast<sys_net_sockaddr_in*>(addr.get_ptr())->sin_port;
			const u16 davport = reinterpret_cast<sys_net_sockaddr_in_p2p*>(addr.get_ptr())->sin_vport;
			sys_net.error("Received a P2P packet from %s:%d:%d", reinterpret_cast<::sockaddr_in*>(&native_addr)->sin_addr, daport, davport);
		}
	}

	// Length
	if (type == SYS_NET_SOCK_DGRAM_P2P)
		sys_net.error("Ok recvfrom: %d", native_result);

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
	std::string _buf;

	if (idm::check<lv2_socket>(s))
	{
		_buf.assign(vm::_ptr<const char>(buf.addr()), vm::_ptr<const char>(buf.addr()) + len);
	}
	else
	{
		return -SYS_NET_EBADF;
	}

	// Used by DGRAM_P2P socket
	u16 davport = 0;

	if (addr)
	{
		davport = reinterpret_cast<const sys_net_sockaddr_in_p2p*>(addr.get_ptr())->sin_vport;
		name.sin_family      = AF_INET;
		name.sin_port        = std::bit_cast<u16>(psa_in->sin_port);
		name.sin_addr.s_addr = std::bit_cast<u32>(psa_in->sin_addr);
	}

	::socklen_t namelen = sizeof(name);
	sys_net_error result{};

	if (flags & SYS_NET_MSG_WAITALL)
	{
		native_flags |= MSG_WAITALL;
	}

	s32 type        = 0;
	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock)
	{
		type = sock.type;
		if (sock.type == SYS_NET_SOCK_DGRAM_P2P && addr)
		{
			const u16 daport = std::bit_cast<be_t<u16>, u16>(name.sin_port);
			sys_net.error("Sending a P2P packet to %s:%d:%d", name.sin_addr, daport, davport);
			name.sin_port = std::bit_cast<u16, be_t<u16>>(daport + davport); // htons(daport + davport)
		}

		std::lock_guard lock(sock.mutex);

		//if (!(sock.events & lv2_socket::poll::write))
		{
			const auto nph = g_fxo->get<named_thread<np_handler>>();
			if (nph->is_dns(s))
			{
				const s32 ret_analyzer = nph->analyze_dns_packet(s, reinterpret_cast<const u8*>(_buf.c_str()), len);
				// Check if the packet is intercepted
				if (ret_analyzer >= 0)
				{
					native_result = ret_analyzer;
					return true;
				}
			}

			native_result = ::sendto(sock.socket, _buf.c_str(), len, native_flags, addr ? reinterpret_cast<struct sockaddr*>(&name) : nullptr, addr ? namelen : 0);

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
			if (events & lv2_socket::poll::write)
			{
				native_result = ::sendto(sock.socket, _buf.c_str(), len, native_flags, addr ? reinterpret_cast<struct sockaddr*>(&name) : nullptr, addr ? namelen : 0);

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
		if (type == SYS_NET_SOCK_DGRAM_P2P)
			sys_net.error("Error sendto(bad socket)");
		return -SYS_NET_EBADF;
	}

	if (!sock.ret && result)
	{
		if (type == SYS_NET_SOCK_DGRAM_P2P)
			sys_net.error("Error sendto(early result): %d", result);

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
			if (type == SYS_NET_SOCK_DGRAM_P2P)
				sys_net.error("Error sendto(result): %d", result);
			return -result;
		}

		if (ppu.gpr[3] == static_cast<u64>(-SYS_NET_EINTR))
		{
			if (type == SYS_NET_SOCK_DGRAM_P2P)
				sys_net.error("Error sendto(interrupted)");
			return -SYS_NET_EINTR;
		}
	}

	// Length
	if (type == SYS_NET_SOCK_DGRAM_P2P)
		sys_net.error("Ok sendto: %d", native_result);

	return not_an_error(native_result);
}

error_code sys_net_bnet_setsockopt(ppu_thread& ppu, s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
{
	ppu.state += cpu_flag::wait;

	sys_net.warning("sys_net_bnet_setsockopt(s=%d, level=0x%x, optname=0x%x, optval=*0x%x, optlen=%u)", s, level, optname, optval, optlen);

	int native_int = 0;
	int native_level = -1;
	int native_opt = -1;
	const void* native_val = &native_int;
	::socklen_t native_len = sizeof(int);
	::timeval native_timeo;
	::linger native_linger;

	std::vector<char> optval_buf(vm::_ptr<char>(optval.addr()), vm::_ptr<char>(optval.addr() + optlen));

	const auto sock = idm::check<lv2_socket>(s, [&](lv2_socket& sock) -> sys_net_error
	{
		std::lock_guard lock(sock.mutex);

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
				native_timeo.tv_sec = ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_sec);
				native_timeo.tv_usec = ::narrow<int>(reinterpret_cast<sys_net_timeval*>(optval_buf.data())->tv_usec);
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

	const s32 s = idm::import_existing<lv2_socket>(std::make_shared<lv2_socket>(native_socket, type, family));

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
				// Check for fake packet for dns interceptions
				const auto nph = g_fxo->get<named_thread<np_handler>>();
				if (fds_buf[i].events & SYS_NET_POLLIN && nph->is_dns(fds_buf[i].fd) && nph->is_dns_queue(fds_buf[i].fd))
					fds_buf[i].revents |= SYS_NET_POLLIN;

				if (fds_buf[i].events & ~(SYS_NET_POLLIN | SYS_NET_POLLOUT | SYS_NET_POLLERR))
					sys_net.error("sys_net_bnet_poll(fd=%d): events=0x%x", fds[i].fd, fds[i].events);
				_fds[i].fd = sock->socket;
				if (fds_buf[i].events & SYS_NET_POLLIN)
					_fds[i].events |= POLLIN;
				if (fds_buf[i].events & SYS_NET_POLLOUT)
					_fds[i].events |= POLLOUT;
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
				_fds[i].fd = sock->socket;
				if (selected & lv2_socket::poll::read)
					_fds[i].events |= POLLIN;
				if (selected & lv2_socket::poll::write)
					_fds[i].events |= POLLOUT;
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
		// TODO: Find out if this string can change
		constexpr auto name = "nameserver 192.168.1.1\0"sv; 
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
