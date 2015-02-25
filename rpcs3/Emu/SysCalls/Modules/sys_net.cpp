#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#ifdef _WIN32
#include <winsock.h>
#else
extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
}
#endif

#include "sys_net.h"

extern Module sys_net;

vm::ptr<s32> g_lastError = vm::ptr<s32>::make(0);


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

#ifdef _WIN32
using pck_len_t = s32;
#else
using pck_len_t = u32;
#endif

namespace sys_net_func
{
	// Functions
	s32 accept(s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<pck_len_t> paddrlen)
	{
		sys_net.Warning("accept(s=%d, family_addr=0x%x, paddrlen=0x%x)", s, addr.addr(), paddrlen.addr());
		if (!addr) {
			int ret = ::accept(s, nullptr, nullptr);
			*g_lastError = getLastError();
			return ret;
		}
		else {
			sockaddr _addr;
			memcpy(&_addr, addr.get_ptr(), sizeof(sockaddr));
			_addr.sa_family = addr->sa_family;
			pck_len_t _paddrlen;
			int ret = ::accept(s, &_addr, &_paddrlen);
			*paddrlen = _paddrlen;
			*g_lastError = getLastError();
			return ret;
		}
	}

	s32 bind(s32 s, vm::ptr<sys_net_sockaddr_in> addr, u32 addrlen)
	{
		sys_net.Warning("bind(s=%d, family_addr=0x%x, addrlen=%d)", s, addr.addr(), addrlen);
		sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(sockaddr_in));
		saddr.sin_family = addr->sin_family;
		const char *ipaddr = inet_ntoa(saddr.sin_addr);
		sys_net.Warning("binding on %s to port %d", ipaddr, ntohs(saddr.sin_port));
		int ret = ::bind(s, (const sockaddr *)&saddr, addrlen);
		*g_lastError = getLastError();
		return ret;
	}

	s32 connect(s32 s, vm::ptr<sys_net_sockaddr_in> addr, u32 addrlen)
	{
		sys_net.Warning("connect(s=%d, family_addr=0x%x, addrlen=%d)", s, addr.addr(), addrlen);
		sockaddr_in saddr;
		memcpy(&saddr, addr.get_ptr(), sizeof(sockaddr_in));
		saddr.sin_family = addr->sin_family;
		const char *ipaddr = inet_ntoa(saddr.sin_addr);
		sys_net.Warning("connecting on %s to port %d", ipaddr, ntohs(saddr.sin_port));
		int ret = ::connect(s, (const sockaddr *)&saddr, addrlen);
		*g_lastError = getLastError();
		return ret;
	}

	s32 gethostbyaddr()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 gethostbyname()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 getpeername()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 getsockname()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 getsockopt()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_addr(vm::ptr<const char> cp)
	{
		sys_net.Warning("inet_addr(cp_addr=0x%x['%s'])", cp.addr(), cp.get_ptr());
		return htonl(::inet_addr(cp.get_ptr())); // return a big-endian IP address (WTF? function should return LITTLE-ENDIAN value)
	}

	s32 inet_aton()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_lnaof()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_makeaddr()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_netof()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_network()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_ntoa()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_ntop()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 inet_pton(s32 af, vm::ptr<const char> src, vm::ptr<char> dst)
	{
		sys_net.Warning("inet_pton(af=%d, src_addr=0x%x, dst_addr=0x%x)", af, src.addr(), dst.addr());

		return ::inet_pton(af, src.get_ptr(), dst.get_ptr());
	}

	s32 listen(s32 s, s32 backlog)
	{
		sys_net.Warning("listen(s=%d, backlog=%d)", s, backlog);
		int ret = ::listen(s, backlog);
		*g_lastError = getLastError();
		return ret;
	}

	s32 recv(s32 s, vm::ptr<char> buf, u32 len, s32 flags)
	{
		sys_net.Warning("recv(s=%d, buf_addr=0x%x, len=%d, flags=0x%x)", s, buf.addr(), len, flags);

		int ret = ::recv(s, buf.get_ptr(), len, flags);
		*g_lastError = getLastError();
		return ret;
	}

	s32 recvfrom(s32 s, vm::ptr<char> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<pck_len_t> paddrlen)
	{
		sys_net.Warning("recvfrom(s=%d, buf_addr=0x%x, len=%d, flags=0x%x, addr_addr=0x%x, paddrlen=0x%x)",
			s, buf.addr(), len, flags, addr.addr(), paddrlen.addr());

		sockaddr _addr;
		memcpy(&_addr, addr.get_ptr(), sizeof(sockaddr));
		_addr.sa_family = addr->sa_family;
		pck_len_t _paddrlen;
		int ret = ::recvfrom(s, buf.get_ptr(), len, flags, &_addr, &_paddrlen);
		*paddrlen = _paddrlen;
		*g_lastError = getLastError();
		return ret;
	}

	s32 recvmsg()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 send(s32 s, vm::ptr<const char> buf, u32 len, s32 flags)
	{
		sys_net.Warning("send(s=%d, buf_addr=0x%x, len=%d, flags=0x%x)", s, buf.addr(), len, flags);

		int ret = ::send(s, buf.get_ptr(), len, flags);
		*g_lastError = getLastError();
		return ret;
	}

	s32 sendmsg()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 sendto(s32 s, vm::ptr<const char> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, u32 addrlen)
	{
		sys_net.Warning("sendto(s=%d, buf_addr=0x%x, len=%d, flags=0x%x, addr=0x%x, addrlen=%d)",
			s, buf.addr(), len, flags, addr.addr(), addrlen);

		sockaddr _addr;
		memcpy(&_addr, addr.get_ptr(), sizeof(sockaddr));
		_addr.sa_family = addr->sa_family;
		int ret = ::sendto(s, buf.get_ptr(), len, flags, &_addr, addrlen);
		*g_lastError = getLastError();
		return ret;
	}

	s32 setsockopt(s32 s, s32 level, s32 optname, vm::ptr<const char> optval, u32 optlen)
	{
		sys_net.Warning("socket(s=%d, level=%d, optname=%d, optval_addr=0x%x, optlen=%d)", s, level, optname, optval.addr(), optlen);

		int ret = ::setsockopt(s, level, optname, optval.get_ptr(), optlen);
		*g_lastError = getLastError();
		return ret;
	}

	s32 shutdown(s32 s, s32 how)
	{
		sys_net.Warning("shutdown(s=%d, how=%d)", s, how);
		int ret = ::shutdown(s, how);
		*g_lastError = getLastError();
		return ret;
	}

	s32 socket(s32 family, s32 type, s32 protocol)
	{
		sys_net.Warning("socket(family=%d, type=%d, protocol=%d)", family, type, protocol);
		int ret = ::socket(family, type, protocol);
		*g_lastError = getLastError();
		return ret;
	}

	s32 socketclose(s32 s)
	{
		sys_net.Warning("socket(s=%d)", s);
#ifdef _WIN32
		int ret = ::closesocket(s);
#else
		int ret = ::close(s);
#endif
		*g_lastError = getLastError();
		return ret;
	}

	s32 socketpoll()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}

	s32 socketselect()
	{
		UNIMPLEMENTED_FUNC(sys_net);
		return CELL_OK;
	}
}

s32 sys_net_initialize_network_ex(vm::ptr<sys_net_initialize_parameter> param)
{
	sys_net.Warning("sys_net_initialize_network_ex(param_addr=0x%x)", param.addr());
	g_lastError = vm::ptr<s32>::make((u32)Memory.Alloc(4, 1));
#ifdef _WIN32
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &wsaData);
#endif
	return CELL_OK;
}

s32 sys_net_get_udpp2p_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_set_udpp2p_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_get_lib_name_server()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_if_ctl()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_get_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_get_sockinfo()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_close_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_set_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_show_nameserver()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

u32 _sys_net_errno_loc()
{
	sys_net.Warning("_sys_net_errno_loc()");
	return g_lastError.addr();
}

s32 sys_net_set_resolver_configurations()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_show_route()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_read_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_abort_resolver()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_abort_socket()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_set_lib_name_server()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_get_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_get_sockinfo_ex()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_open_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_show_ifconfig()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_finalize_network()
{
	sys_net.Warning("sys_net_initialize_network_ex()");
	Memory.Free(g_lastError.addr());
	g_lastError = vm::ptr<s32>::make(0);
#ifdef _WIN32
	WSACleanup();
#endif
	return CELL_OK;
}

s32 _sys_net_h_errno_loc()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_set_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 sys_net_free_thread_context()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

// define additional macro for specific namespace
#define REG_FUNC_(name) add_ppu_func(ModuleFunc(get_function_id(#name), 0, &sys_net, bind_func(sys_net_func::name)))

Module sys_net("sys_net", []()
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

	REG_FUNC(sys_net, sys_net_initialize_network_ex);
	REG_FUNC(sys_net, sys_net_get_udpp2p_test_param);
	REG_FUNC(sys_net, sys_net_set_udpp2p_test_param);
	REG_FUNC(sys_net, sys_net_get_lib_name_server);
	REG_FUNC(sys_net, sys_net_if_ctl);
	REG_FUNC(sys_net, sys_net_get_netemu_test_param);
	REG_FUNC(sys_net, sys_net_get_sockinfo);
	REG_FUNC(sys_net, sys_net_close_dump);
	REG_FUNC(sys_net, sys_net_set_test_param);
	REG_FUNC(sys_net, sys_net_show_nameserver);
	REG_FUNC(sys_net, _sys_net_errno_loc);
	REG_FUNC(sys_net, sys_net_set_resolver_configurations);
	REG_FUNC(sys_net, sys_net_show_route);
	REG_FUNC(sys_net, sys_net_read_dump);
	REG_FUNC(sys_net, sys_net_abort_resolver);
	REG_FUNC(sys_net, sys_net_abort_socket);
	REG_FUNC(sys_net, sys_net_set_lib_name_server);
	REG_FUNC(sys_net, sys_net_get_test_param);
	REG_FUNC(sys_net, sys_net_get_sockinfo_ex);
	REG_FUNC(sys_net, sys_net_open_dump);
	REG_FUNC(sys_net, sys_net_show_ifconfig);
	REG_FUNC(sys_net, sys_net_finalize_network);
	REG_FUNC(sys_net, _sys_net_h_errno_loc);
	REG_FUNC(sys_net, sys_net_set_netemu_test_param);
	REG_FUNC(sys_net, sys_net_free_thread_context);
});
