#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sys_net.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

LOG_CHANNEL(libnet);

// We map host sockets to sequential IDs to return as FDs because syscalls using
// socketselect(), etc. expect socket FDs to be under 1024.
// We start at 1 because 0 is an invalid socket.
std::vector<s64> g_socketMap{ 0 };

// Auxiliary Functions
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

s32 getLastError()
{
#ifdef _WIN32
	s32 ret = WSAGetLastError();
	if (ret > 10000 && ret < 11000)
		return ret % 10000;
	else
		return -1;
#else
	return errno;
#endif
}

void copy_fdset(fd_set* set, vm::ptr<sys_net::fd_set> src)
{
	FD_ZERO(set);

	if (src)
	{
		// Go through the bit set fds_bits and calculate the
		// socket FDs from it, setting it in the native fd-set.

		for (int i = 0; i < 32; i++)
		{
			for (int bit = 0; bit < 32; bit++)
			{
				if (src->fds_bits[i] & (1 << bit))
				{
					u32 sock = (i << 5) | bit;
					//libnet.error("setting: fd %d", sock);
					FD_SET(g_socketMap[sock], set);
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
	};

	// TODO
	thread_local vm::ptr<_tls_data_t> g_tls_net_data{};

	static never_inline void initialize_tls()
	{
		// allocate if not initialized
		if (!g_tls_net_data)
		{
			g_tls_net_data.set(vm::alloc(sizeof(decltype(g_tls_net_data)::type), vm::main));

			thread_ctrl::at_exit([addr = g_tls_net_data.addr()]
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

	// Functions
	s32 accept(s32 s, vm::ptr<sockaddr> addr, vm::ptr<u32> paddrlen)
	{
		libnet.warning("accept(s=%d, family=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);
		s = g_socketMap[s];

		if (!addr) {
			int ret = ::accept(s, nullptr, nullptr);
			get_errno() = getLastError();
			return ret;
		}
		else {
			::sockaddr _addr;
			memcpy(&_addr, addr.get_ptr(), sizeof(::sockaddr));
			_addr.sa_family = addr->sa_family;
			::socklen_t _paddrlen;
			s32 ret = ::accept(s, &_addr, &_paddrlen);
			*paddrlen = _paddrlen;
			get_errno() = getLastError();
			return ret;
		}
	}

	s32 bind(s32 s, vm::cptr<sockaddr> addr, u32 addrlen)
	{
		libnet.warning("bind(s=%d, family=*0x%x, addrlen=%d)", s, addr, addrlen);
		s = g_socketMap[s];

		::sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(::sockaddr_in));
		saddr.sin_family = addr->sa_family;
		const char *ipaddr = ::inet_ntoa(saddr.sin_addr);
		libnet.warning("binding on %s to port %d", ipaddr, ntohs(saddr.sin_port));
		s32 ret = ::bind(s, (const ::sockaddr*)&saddr, addrlen);
		get_errno() = getLastError();

		return ret;
	}

	s32 connect(s32 s, vm::ptr<sockaddr> addr, u32 addrlen)
	{
		libnet.warning("connect(s=%d, family=*0x%x, addrlen=%d)", s, addr, addrlen);
		s = g_socketMap[s];

		::sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(::sockaddr_in));
		saddr.sin_family = addr->sa_family;
		const char *ipaddr = ::inet_ntoa(saddr.sin_addr);
		libnet.warning("connecting on %s to port %d", ipaddr, ntohs(saddr.sin_port));
		s32 ret = ::connect(s, (const ::sockaddr*)&saddr, addrlen);
		get_errno() = getLastError();

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

	s32 inet_ntoa()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
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
		s = g_socketMap[s];
		s32 ret = ::listen(s, backlog);
		get_errno() = getLastError();

		return ret;
	}

	s32 recv(s32 s, vm::ptr<char> buf, u32 len, s32 flags)
	{
		libnet.warning("recv(s=%d, buf=*0x%x, len=%d, flags=0x%x)", s, buf, len, flags);
		s = g_socketMap[s];

		s32 ret = ::recv(s, buf.get_ptr(), len, flags);
		get_errno() = getLastError();

		return ret;
	}

	s32 recvfrom(s32 s, vm::ptr<char> buf, u32 len, s32 flags, vm::ptr<sockaddr> addr, vm::ptr<u32> paddrlen)
	{
		libnet.warning("recvfrom(s=%d, buf=*0x%x, len=%d, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);
		s = g_socketMap[s];

		::sockaddr _addr;
		memcpy(&_addr, addr.get_ptr(), sizeof(::sockaddr));
		_addr.sa_family = addr->sa_family;
		::socklen_t _paddrlen;
		s32 ret = ::recvfrom(s, buf.get_ptr(), len, flags, &_addr, &_paddrlen);
		*paddrlen = _paddrlen;
		get_errno() = getLastError();

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
		s = g_socketMap[s];

		s32 ret = ::send(s, buf.get_ptr(), len, flags);
		get_errno() = getLastError();

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
		s = g_socketMap[s];

		::sockaddr _addr;
		memcpy(&_addr, addr.get_ptr(), sizeof(::sockaddr));
		_addr.sa_family = addr->sa_family;
		s32 ret = ::sendto(s, buf.get_ptr(), len, flags, &_addr, addrlen);
		get_errno() = getLastError();

		return ret;
	}

	s32 setsockopt(s32 s, s32 level, s32 optname, vm::cptr<char> optval, u32 optlen)
	{
		libnet.warning("socket(s=%d, level=%d, optname=%d, optval=*0x%x, optlen=%d)", s, level, optname, optval, optlen);
		s = g_socketMap[s];

		s32 ret = ::setsockopt(s, level, optname, optval.get_ptr(), optlen);
		get_errno() = getLastError();

		return ret;
	}

	s32 shutdown(s32 s, s32 how)
	{
		libnet.warning("shutdown(s=%d, how=%d)", s, how);
		s = g_socketMap[s];

		s32 ret = ::shutdown(s, how);
		get_errno() = getLastError();

		return ret;
	}

	s32 socket(s32 family, s32 type, s32 protocol)
	{
		libnet.warning("socket(family=%d, type=%d, protocol=%d)", family, type, protocol);

		s32 sock = ::socket(family, type, protocol);
		get_errno() = getLastError();

		g_socketMap.push_back(sock);
		return g_socketMap.size() - 1;
	}

	s32 socketclose(s32 s)
	{
		libnet.warning("socket(s=%d)", s);
		s = g_socketMap[s];

#ifdef _WIN32
		int ret = ::closesocket(s);
#else
		int ret = ::close(s);
#endif
		get_errno() = getLastError();
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

		s32 ret = ::select(nfds, &_readfds, &_writefds, &_exceptfds, timeout ? &_timeout : NULL);
		get_errno() = getLastError();

		if (getLastError() >= 0)
		{
			libnet.error("socketselect(): error %d", getLastError());
		}

		//return ret;
		return CELL_OK;
	}

	s32 sys_net_initialize_network_ex(vm::ptr<sys_net_initialize_parameter_t> param)
	{
		libnet.warning("sys_net_initialize_network_ex(param=*0x%x)", param);

#ifdef _WIN32
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);
#endif
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

	s32 sys_net_get_sockinfo()
	{
		UNIMPLEMENTED_FUNC(libnet);
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
		libnet.warning("sys_net_initialize_network_ex()");

#ifdef _WIN32
		WSACleanup();
#endif
		return CELL_OK;
	}

	s32 _sys_net_h_errno_loc()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_set_netemu_test_param()
	{
		UNIMPLEMENTED_FUNC(libnet);
		return CELL_OK;
	}

	s32 sys_net_free_thread_context()
	{
		UNIMPLEMENTED_FUNC(libnet);
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
