#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sys_net.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <fcntl.h>

logs::channel libnet("libnet", logs::level::notice);

namespace sys_net
{
#ifdef _WIN32
	using socket_t = SOCKET;
#else
	using socket_t = int;
#endif
}

// Auxiliary Functions
// FIXME: Use the variant from OS instead? Why do we even have such a custom function?
int inet_pton4(const char *src, char *dst)
{
	const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	unsigned char tmp[4], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr(digits, ch)) != NULL) {
			unsigned int n = *tp * 10 + (pch - digits);

			if (n > 255)
				return (0);
			*tp = n;
			if (! saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return 0;
			*++tp = 0;
			saw_digit = 0;
		} else
			return (0);
	}
	if (octets < 4)
		return 0;

	memcpy(dst, tmp, 4);
	return 1;
}

int inet_pton(int af, const char *src, char *dst)
{
	switch (af) {
	case AF_INET:
		return (inet_pton4(src, dst));

	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

// Custom structure for sockets
// We map host sockets to sequential IDs to return as descriptors because syscalls expect socket IDs to be under 1024.
struct sys_net_socket final
{
	using id_base = sys_net_socket;

	static constexpr u32 id_min = 0; // Minimal valid socket number is 0 (not 1).
	static constexpr u32 id_max = 1023;

	sys_net::socket_t s = -1;

	explicit sys_net_socket(s32 socket) : s(socket)
	{
	}

	~sys_net_socket()
	{
		if (s != -1)
#ifdef _WIN32
			::closesocket(s);
#else
			::close(s);
#endif
	}

	sys_net_socket(sys_net_socket const &) = delete;
	sys_net_socket& operator=(const sys_net_socket&) = delete;
};

void copy_fdset(fd_set* set, vm::ptr<sys_net::fd_set> src)
{
	FD_ZERO(set);

	if (src)
	{
		// Go through the bit set fds_bits and calculate the
		// socket FDs from it, setting it in the native fd-set.

		for (s32 i = 0; i < 32; i++)
		{
			for (s32 bit = 0; bit < 32; bit++)
			{
				if (src->fds_bits[i] & (1 << bit))
				{
					sys_net::socket_t sock = idm::get<sys_net_socket>((i << 5) | bit)->s;
					//libnet.error("setting: fd %d", sock);
					FD_SET(sock, set);
				}
			}
		}
	}
}

namespace sys_net
{
	struct _tls_data_t
	{
		be_t<s32> _errno;
		be_t<s32> _h_errno;
		char addr[16];
	};

	// TODO
	thread_local vm::ptr<_tls_data_t> g_tls_net_data{};

	static never_inline void initialize_tls()
	{
		// allocate if not initialized
		if (!g_tls_net_data)
		{
			g_tls_net_data.set(vm::alloc(sizeof(decltype(g_tls_net_data)::type), vm::main));
			
			// Initial values
			g_tls_net_data->_errno = SYS_NET_EBUSY;

			thread_ctrl::atexit([addr = g_tls_net_data.addr()]
			{
				vm::dealloc_verbose_nothrow(addr, vm::main);
			});
		}
	}

	vm::ref<s32> get_errno()
	{
		initialize_tls();

		return g_tls_net_data.ref(&_tls_data_t::_errno);
	}

	vm::ref<s32> get_h_errno()
	{
		initialize_tls();

		return g_tls_net_data.ref(&_tls_data_t::_h_errno);
	}

	// Error helper functions
	s32 get_last_error()
	{
		// Convert the error code for socket functions to a one for sys_net
		s32 result;
		const char* name{};

#ifdef _WIN32
		switch (s32 code = WSAGetLastError())
#define ERROR_CASE(error) case WSA ## error: result = SYS_NET_ ## error; name = #error; break;
#else
		switch (s32 code = errno)
#define ERROR_CASE(error) case error: result = SYS_NET_ ## error; name = #error; break;
#endif
		{
			ERROR_CASE(EWOULDBLOCK);
		default: throw fmt::exception("Unknown/illegal socket error: %d" HERE, code);
		}

		if (name && result != SYS_NET_EWOULDBLOCK)
		{
			ppu_error_code::report(result, name);
		}

		return result;
#undef ERROR_CASE
	}

	// Functions
	s32 accept(s32 s, vm::ptr<sockaddr> addr, vm::ptr<u32> paddrlen)
	{
		libnet.warning("accept(s=%d, family=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		s32 ret;

		if (!addr)
		{
			ret = ::accept(sock->s, nullptr, nullptr);

			if (ret < 0)
			{
				libnet.error("accept(): error %d", get_errno() = get_last_error());
				return -1;
			}
		}
		else
		{
			::sockaddr _addr;
			::socklen_t _paddrlen = 16;

			ret = ::accept(sock->s, &_addr, &_paddrlen);

			if (ret < 0)
			{
				libnet.error("accept(): error %d", get_errno() = get_last_error());
				return -1;
			}

			*paddrlen = _paddrlen;
			addr->sa_len = _paddrlen;
			addr->sa_family = _addr.sa_family;
			memcpy(addr->sa_data, _addr.sa_data, addr->sa_len - 2);
		}

		return idm::make<sys_net_socket>(ret);
	}

	s32 bind(s32 s, vm::cptr<sockaddr> addr, u32 addrlen)
	{
		libnet.warning("bind(s=%d, family=*0x%x, addrlen=%d)", s, addr, addrlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		::sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(::sockaddr_in));
		saddr.sin_family = addr->sa_family;
		const char *ipaddr = ::inet_ntoa(saddr.sin_addr);
		libnet.warning("binding to %s on port %d", ipaddr, ntohs(saddr.sin_port));
		s32 ret = ::bind(sock->s, (const ::sockaddr*)&saddr, addrlen);

		if (ret != 0)
		{
			libnet.error("bind(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 connect(s32 s, vm::ptr<sockaddr> addr, u32 addrlen)
	{
		libnet.warning("connect(s=%d, family=*0x%x, addrlen=%d)", s, addr, addrlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		::sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(::sockaddr_in));
		saddr.sin_family = addr->sa_family;

		libnet.warning("connecting to %s on port %d", ::inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
		s32 ret = ::connect(sock->s, (const ::sockaddr*)&saddr, addrlen);
		
		if (ret != 0)
		{
			if ((get_errno() = get_last_error()) != SYS_NET_EWOULDBLOCK)
			{
				libnet.error("connect(): error %d", get_errno().get_ref());
			}
			
			return -1;
		}

		return ret;
	}

	s32 gethostbyaddr()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 gethostbyname()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 getpeername()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 getsockname()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 getsockopt()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	u32 inet_addr(vm::cptr<char> cp)
	{
		libnet.warning("inet_addr(cp=*0x%x)", cp);
		return htonl(::inet_addr(cp.get_ptr())); // return a big-endian IP address (WTF? function should return LITTLE-ENDIAN value)
	}

	s32 inet_aton()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 inet_lnaof()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 inet_makeaddr()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 inet_netof()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 inet_network()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	vm::ptr<char> inet_ntoa(u32 in)
	{
		libnet.warning("inet_ntoa(in=0x%x)", in);
		initialize_tls();

		::in_addr addr;
		addr.s_addr = in;

		char* result = ::inet_ntoa(addr);
		strcpy(g_tls_net_data->addr, result);

		return vm::ptr<char>::make(vm::get_addr(g_tls_net_data->addr));
	}

	vm::cptr<char> inet_ntop(s32 af, vm::ptr<void> src, vm::ptr<char> dst, u32 size)
	{
		libnet.warning("inet_ntop(af=%d, src=*0x%x, dst=*0x%x, size=%d)", af, src, dst, size);
		const char* result = ::inet_ntop(af, src.get_ptr(), dst.get_ptr(), size);

		if (result == nullptr)
		{
			return vm::null;
		}

		return dst;
	}

	s32 inet_pton(s32 af, vm::cptr<char> src, vm::ptr<char> dst)
	{
		libnet.warning("inet_pton(af=%d, src=*0x%x, dst=*0x%x)", af, src, dst);
		return ::inet_pton(af, src.get_ptr(), dst.get_ptr());
	}

	s32 listen(s32 s, s32 backlog)
	{
		libnet.warning("listen(s=%d, backlog=%d)", s, backlog);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		s32 ret = ::listen(sock->s, backlog);

		if (ret != 0)
		{
			libnet.error("listen(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 recv(s32 s, vm::ptr<char> buf, u32 len, s32 flags)
	{
		libnet.warning("recv(s=%d, buf=*0x%x, len=%d, flags=0x%x)", s, buf, len, flags);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		s32 ret = ::recv(sock->s, buf.get_ptr(), len, flags);
		
		if (ret < 0)
		{
			libnet.error("recv(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 recvfrom(s32 s, vm::ptr<char> buf, u32 len, s32 flags, vm::ptr<sockaddr> addr, vm::ptr<u32> paddrlen)
	{
		libnet.warning("recvfrom(s=%d, buf=*0x%x, len=%d, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		::sockaddr _addr;
		::socklen_t _paddrlen;

		memcpy(&_addr, addr.get_ptr(), sizeof(::sockaddr));
		_addr.sa_family = addr->sa_family;
		s32 ret = ::recvfrom(sock->s, buf.get_ptr(), len, flags, &_addr, &_paddrlen);
		*paddrlen = _paddrlen;

		if (ret < 0)
		{
			libnet.error("recvfrom(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 recvmsg()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 send(s32 s, vm::cptr<char> buf, u32 len, s32 flags)
	{
		libnet.warning("send(s=%d, buf=*0x%x, len=%d, flags=0x%x)", s, buf, len, flags);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		s32 ret = ::send(sock->s, buf.get_ptr(), len, flags);
		
		if (ret < 0)
		{
			libnet.error("send(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 sendmsg()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sendto(s32 s, vm::cptr<char> buf, u32 len, s32 flags, vm::ptr<sockaddr> addr, u32 addrlen)
	{
		libnet.warning("sendto(s=%d, buf=*0x%x, len=%d, flags=0x%x, addr=*0x%x, addrlen=%d)", s, buf, len, flags, addr, addrlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		::sockaddr _addr;
		memcpy(&_addr, addr.get_ptr(), sizeof(::sockaddr));
		_addr.sa_family = addr->sa_family;
		s32 ret = ::sendto(sock->s, buf.get_ptr(), len, flags, &_addr, addrlen);

		if (ret < 0)
		{
			libnet.error("sendto(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 setsockopt(s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
	{
		libnet.warning("setsockopt(s=%d, level=%d, optname=%d, optval=*0x%x, optlen=%d)", s, level, optname, optval, optlen);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		if (level != SOL_SOCKET && level != IPPROTO_TCP)
		{
			throw EXCEPTION("Invalid socket option level!");
		}

		s32 ret;

#ifdef _WIN32
		if (level == SOL_SOCKET)
		{
			switch (optname)
			{
			case OP_SO_NBIO:
			{
				unsigned long mode = *(unsigned long*)optval.get_ptr();
				ret = ioctlsocket(sock->s, FIONBIO, &mode);
				break;
			}
			case OP_SO_SNDBUF:
			{
				u32 sendbuff = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuff, sizeof(sendbuff));
				break;
			}
			case OP_SO_RCVBUF:
			{
				u32 recvbuff = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_RCVBUF, (const char*)&recvbuff, sizeof(recvbuff));
				break;
			}
			case OP_SO_SNDTIMEO:
			{
				u32 sendtimeout = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_SNDTIMEO, (char*)&sendtimeout, sizeof(sendtimeout));
				break;
			}
			case OP_SO_RCVTIMEO:
			{
				u32 recvtimeout = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvtimeout, sizeof(recvtimeout));
				break;
			}
			case OP_SO_SNDLOWAT:
			{
				u32 sendlowmark = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_SNDLOWAT, (char*)&sendlowmark, sizeof(sendlowmark));
				break;
			}
			case OP_SO_RCVLOWAT:
			{
				u32 recvlowmark = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, SOL_SOCKET, SO_RCVLOWAT, (char*)&recvlowmark, sizeof(recvlowmark));
				break;
			}
			default:
				throw EXCEPTION("Unknown socket option for Win32: 0x%x", optname);
			}
		}
		else if (level == PROTO_IPPROTO_TCP)
		{
			switch (optname)
			{
			case OP_TCP_NODELAY:
			{
				const char delay = *(char*)optval.get_ptr();
				ret = ::setsockopt(sock->s, IPPROTO_TCP, TCP_NODELAY, &delay, sizeof(delay));
				break;
			}

			case OP_TCP_MAXSEG:
			{
				libnet.warning("TCP_MAXSEG can't be set on Windows.");
				break;
			}

			default:
				throw EXCEPTION("Unknown TCP option for Win32: 0x%x", optname);
			}
		}
#else
		if (level == SOL_SOCKET)
		{
			switch (optname)
			{
			case OP_SO_NBIO:
			{
				// Obtain the flags
				s32 flags = fcntl(s, F_GETFL, 0);

				if (flags < 0)
				{
					throw EXCEPTION("Failed to obtain socket flags.");
				}

				u32 mode = *(u32*)optval.get_ptr();
				flags = mode ? (flags &~O_NONBLOCK) : (flags | O_NONBLOCK);

				// Re-set the flags
				ret = fcntl(sock->s, F_SETFL, flags);
				break;
			}

			default:
				throw EXCEPTION("Unknown socket option for Unix: 0x%x", optname);
			}
		}
		else if (level == PROTO_IPPROTO_TCP)
		{
			switch (optname)
			{
			case OP_TCP_NODELAY:
			{
				u32 delay = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, IPPROTO_TCP, TCP_NODELAY, &delay, optlen);
				break;
			}

			case OP_TCP_MAXSEG:
			{
				u32 maxseg = *(u32*)optval.get_ptr();
				ret = ::setsockopt(sock->s, IPPROTO_TCP, TCP_MAXSEG, &maxseg, optlen);
				break;
			}

			default:
				throw EXCEPTION("Unknown TCP option for Unix: 0x%x", optname);
			}
		}
#endif

		if (ret != 0)
		{
			libnet.error("setsockopt(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 shutdown(s32 s, s32 how)
	{
		libnet.warning("shutdown(s=%d, how=%d)", s, how);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

		s32 ret = ::shutdown(sock->s, how);

		if (ret != 0)
		{
			libnet.error("shutdown(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 socket(s32 family, s32 type, s32 protocol)
	{
		libnet.warning("socket(family=%d, type=%d, protocol=%d)", family, type, protocol);

		if (type < 1 || type > 10 || (type > 4 && type < 6) || (type > 6 && type < 10))
		{
			get_errno() = SYS_NET_EPROTONOSUPPORT;
			return -1;
		}

		// HACKS: Neither Unix nor Windows support TCP/UDP over UDPP2P.
		// But what's the usage of it anyways?
		if (type == SOCK_STREAM_P2P)
		{
			libnet.warning("SOCK_STREAM_P2P is not properly implemented.");
			type = SOCK_STREAM;
		}
		else if (type == SOCK_DGRAM_P2P)
		{
			libnet.warning("SOCK_DGRAM_P2P is not properly implemented.");
			type = SOCK_DGRAM;
		}

		socket_t sock = ::socket(family, type, protocol);

		if (sock < 0)
		{
			libnet.error("socket(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return idm::make<sys_net_socket>(sock);
	}

	s32 socketclose(s32 s)
	{
		libnet.warning("socketclose(s=%d)", s);
		std::shared_ptr<sys_net_socket> sock = idm::get<sys_net_socket>(s);

#ifdef _WIN32
		s32 ret = ::closesocket(sock->s);
#else
		s32 ret = ::close(sock->s);
#endif

		if (ret != 0)
		{
			libnet.error("socketclose(): error %d", get_errno() = get_last_error());
			return -1;
		}

		idm::get<sys_net_socket>(s)->s = -1;
		idm::remove<sys_net_socket>(s);

		return ret;
	}

	s32 socketpoll()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 socketselect(s32 nfds, vm::ptr<fd_set> readfds, vm::ptr<fd_set> writefds, vm::ptr<fd_set> exceptfds, vm::ptr<timeval> timeout)
	{
		libnet.warning("socketselect(nfds=%d, readfds=*0x%x, writefds=*0x%x, exceptfds=*0x%x, timeout=*0x%x)", nfds, readfds, writefds, exceptfds, timeout);

		::timeval _timeout;

		if (timeout)
		{
			_timeout.tv_sec = timeout->tv_sec;
			_timeout.tv_usec = timeout->tv_usec;
		}

		//libnet.error("timeval: %d . %d", _timeout.tv_sec, _timeout.tv_usec);

		::fd_set _readfds;
		::fd_set _writefds;
		::fd_set _exceptfds;

		// Copy the fd_sets to native ones
		copy_fdset(&_readfds, readfds);
		copy_fdset(&_writefds, writefds);
		copy_fdset(&_exceptfds, exceptfds);

#ifdef _WIN32
		// On Unix, when the sets are empty (thus nothing to wait on), it waits until the timeout.
		// This behaviour is often used to "sleep" on Unix based systems.
		// WinSock in such case returns WSAEINVAL and doesn't allow such behaviour.
		// Since it's not possible on Windows, we just return and say that the timeout is over and hope that it's good enough.
		if (_readfds.fd_count == 0 && _writefds.fd_count == 0 && _exceptfds.fd_count == 0)
		{
			return 0; // Timeout!
		}
#endif

		// There's no good way to determine nfds and it shouldn't be too slow, so let's let it check the whole set. It also isn't used on Windows.
		s32 ret = ::select(FD_SETSIZE, &_readfds, &_writefds, &_exceptfds, timeout ? &_timeout : nullptr);

		if (ret < 0)
		{
			libnet.error("socketselect(): error %d", get_errno() = get_last_error());
			return -1;
		}

		return ret;
	}

	s32 sys_net_initialize_network_ex(vm::ptr<sys_net_initialize_parameter_t> param)
	{
		libnet.warning("sys_net_initialize_network_ex(param=*0x%x)", param);

		// Errno is set to 0 upon initialization
		get_errno() = 0;

		return CELL_OK;
	}

	s32 sys_net_get_udpp2p_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_set_udpp2p_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_get_lib_name_server()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_if_ctl()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_get_netemu_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_get_sockinfo(s32 s, vm::ptr<sys_net_sockinfo_t> p, s32 n)
	{
		libnet.todo("sys_net_get_sockinfo()");
		return CELL_OK;
	}

	s32 sys_net_close_dump()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_set_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_show_nameserver()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	vm::ptr<s32> _sys_net_errno_loc()
	{
		libnet.warning("_sys_net_errno_loc()");
		return get_errno().ptr();
	}

	s32 sys_net_set_resolver_configurations()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_show_route()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_read_dump()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_abort_resolver()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_abort_socket()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_set_lib_name_server()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_get_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_get_sockinfo_ex()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_open_dump()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_show_ifconfig()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_finalize_network()
	{
		libnet.warning("sys_net_finalize_network()");

		// Errno is set to SYS_NET_EBUSY after finalization
		get_errno() = SYS_NET_EBUSY;

		return CELL_OK;
	}

	vm::ptr<s32> _sys_net_h_errno_loc()
	{
		libnet.warning("_sys_net_h_errno_loc()");
		return get_h_errno().ptr();
	}

	s32 sys_net_set_netemu_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_free_thread_context(u64 tid, s32 flags)
	{
		libnet.todo("sys_net_free_thread_context(tid=%d, flags=%d)", tid, flags);
		return CELL_OK;
	}
}

// Define macro for namespace
#define REG_FUNC_(name) REG_FNID(sys_net, ppu_generate_id(#name), sys_net::name)

DECLARE(ppu_module_manager::libnet)("sys_net", []()
{
	REG_FUNC_(accept);
	REG_FUNC_(bind);
	REG_FUNC_(connect);
	REG_FUNC_(gethostbyaddr);
	REG_FUNC_(gethostbyname);
	REG_FUNC_(getpeername);
	REG_FUNC_(getsockname);
	REG_FUNC_(getsockopt);
	REG_FUNC_(inet_addr);
	REG_FUNC_(inet_aton);
	REG_FUNC_(inet_lnaof);
	REG_FUNC_(inet_makeaddr);
	REG_FUNC_(inet_netof);
	REG_FUNC_(inet_network);
	REG_FUNC_(inet_ntoa);
	REG_FUNC_(inet_ntop);
	REG_FUNC_(inet_pton);
	REG_FUNC_(listen);
	REG_FUNC_(recv);
	REG_FUNC_(recvfrom);
	REG_FUNC_(recvmsg);
	REG_FUNC_(send);
	REG_FUNC_(sendmsg);
	REG_FUNC_(sendto);
	REG_FUNC_(setsockopt);
	REG_FUNC_(shutdown);
	REG_FUNC_(socket);
	REG_FUNC_(socketclose);
	REG_FUNC_(socketpoll);
	REG_FUNC_(socketselect);

	REG_FUNC_(sys_net_initialize_network_ex);
	REG_FUNC_(sys_net_get_udpp2p_test_param);
	REG_FUNC_(sys_net_set_udpp2p_test_param);
	REG_FUNC_(sys_net_get_lib_name_server);
	REG_FUNC_(sys_net_if_ctl);
	REG_FUNC_(sys_net_get_netemu_test_param);
	REG_FUNC_(sys_net_get_sockinfo);
	REG_FUNC_(sys_net_close_dump);
	REG_FUNC_(sys_net_set_test_param);
	REG_FUNC_(sys_net_show_nameserver);
	REG_FUNC_(_sys_net_errno_loc);
	REG_FUNC_(sys_net_set_resolver_configurations);
	REG_FUNC_(sys_net_show_route);
	REG_FUNC_(sys_net_read_dump);
	REG_FUNC_(sys_net_abort_resolver);
	REG_FUNC_(sys_net_abort_socket);
	REG_FUNC_(sys_net_set_lib_name_server);
	REG_FUNC_(sys_net_get_test_param);
	REG_FUNC_(sys_net_get_sockinfo_ex);
	REG_FUNC_(sys_net_open_dump);
	REG_FUNC_(sys_net_show_ifconfig);
	REG_FUNC_(sys_net_finalize_network);
	REG_FUNC_(_sys_net_h_errno_loc);
	REG_FUNC_(sys_net_set_netemu_test_param);
	REG_FUNC_(sys_net_free_thread_context);
});
