#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "sys_net.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

void sys_net_init();
Module sys_net((u16)0x0000, sys_net_init);

mem32_t g_lastError(NULL);


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

// Functions
int sys_net_accept(s32 s, mem_ptr_t<sys_net_sockaddr> addr, mem32_t paddrlen)
{
	sys_net.Warning("accept(s=%d, family_addr=0x%x, paddrlen=0x%x)", s, addr.GetAddr(), paddrlen.GetAddr());
	sockaddr _addr;
	memcpy(&_addr, Memory.VirtualToRealAddr(addr.GetAddr()), sizeof(sockaddr));
	_addr.sa_family = addr->sa_family;
	pck_len_t *_paddrlen = (pck_len_t *) Memory.VirtualToRealAddr(paddrlen.GetAddr());
	int ret = accept(s, &_addr, _paddrlen);
	g_lastError = getLastError();
	return ret;
}

int sys_net_bind(s32 s, mem_ptr_t<sys_net_sockaddr> family, u32 addrlen)
{
	sys_net.Warning("bind(s=%d, family_addr=0x%x, addrlen=%u)", s, family.GetAddr(), addrlen);
	sockaddr _family;
	memcpy(&_family, Memory.VirtualToRealAddr(family.GetAddr()), sizeof(sockaddr));
	_family.sa_family = family->sa_family;
	int ret = bind(s, &_family, addrlen);
	g_lastError = getLastError();
	return ret;
}

int sys_net_connect(s32 s, mem_ptr_t<sys_net_sockaddr> family, u32 addrlen)
{
	sys_net.Warning("connect(s=%d, family_addr=0x%x, addrlen=%u)", s, family.GetAddr(), addrlen);
	sockaddr _family;
	memcpy(&_family, Memory.VirtualToRealAddr(family.GetAddr()), sizeof(sockaddr));
	_family.sa_family = family->sa_family;
	int ret = connect(s, &_family, addrlen);
	g_lastError = getLastError();
	return ret;
}

int gethostbyaddr()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int gethostbyname()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int getpeername()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int getsockname()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int getsockopt()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_addr()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_aton()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_lnaof()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_makeaddr()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_netof()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_network()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_ntoa()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int inet_ntop()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_inet_pton(s32 af, u32 src_addr, u32 dst_addr)
{
	sys_net.Warning("inet_pton(af=%d, src_addr=0x%x, dst_addr=0x%x)", af, src_addr, dst_addr);
	char *src = (char *)Memory.VirtualToRealAddr(src_addr);
	char *dst = (char *)Memory.VirtualToRealAddr(dst_addr);
	return inet_pton(af, src, dst);
}

int listen()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_recv(s32 s, u32 buf_addr, u32 len, s32 flags)
{
	sys_net.Warning("recv(s=%d, buf_addr=0x%x, len=%d, flags=0x%x)", s, buf_addr, len, flags);
	char *buf = (char *)Memory.VirtualToRealAddr(buf_addr);
	int ret = recv(s, buf, len, flags);
	g_lastError = getLastError();
	return ret;
}

int sys_net_recvfrom(s32 s, u32 buf_addr, u32 len, s32 flags, mem_ptr_t<sys_net_sockaddr> addr, mem32_t paddrlen)
{
	sys_net.Warning("recvfrom(s=%d, buf_addr=0x%x, len=%u, flags=0x%x, addr_addr=0x%x, paddrlen=0x%x)",
		s, buf_addr, len, flags, addr.GetAddr(), paddrlen.GetAddr());

	char *_buf_addr = (char *)Memory.VirtualToRealAddr(buf_addr);
	sockaddr _addr;
	memcpy(&_addr, Memory.VirtualToRealAddr(addr.GetAddr()), sizeof(sockaddr));
	_addr.sa_family = addr->sa_family;
	pck_len_t *_paddrlen = (pck_len_t *) Memory.VirtualToRealAddr(paddrlen.GetAddr());
	int ret = recvfrom(s, _buf_addr, len, flags, &_addr, _paddrlen);
	g_lastError = getLastError();
	return ret;
}

int recvmsg()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_send(s32 s, u32 buf_addr, u32 len, s32 flags)
{
	sys_net.Warning("send(s=%d, buf_addr=0x%x, len=%d, flags=0x%x)", s, buf_addr, len, flags);
	char *buf = (char *)Memory.VirtualToRealAddr(buf_addr);
	int ret = send(s, buf, len, flags);
	g_lastError = getLastError();
	return ret;
}

int sendmsg()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_sendto(s32 s, u32 buf_addr, u32 len, s32 flags, mem_ptr_t<sys_net_sockaddr> addr, u32 addrlen)
{
	sys_net.Warning("sendto(s=%d, buf_addr=0x%x, len=%u, flags=0x%x, addr=0x%x, addrlen=%u)",
		s, buf_addr, len, flags, addr.GetAddr(), addrlen);

	char *_buf_addr = (char *)Memory.VirtualToRealAddr(buf_addr);
	sockaddr _addr;
	memcpy(&_addr, Memory.VirtualToRealAddr(addr.GetAddr()), sizeof(sockaddr));
	_addr.sa_family = addr->sa_family;
	int ret = sendto(s, _buf_addr, len, flags, &_addr, addrlen);
	g_lastError = getLastError();
	return ret;
}

int sys_net_setsockopt(s32 s, s32 level, s32 optname, u32 optval_addr, u32 optlen)
{
	sys_net.Warning("socket(s=%d, level=%d, optname=%d, optval_addr=0x%x, optlen=%u)", s, level, optname, optval_addr, optlen);
	char *_optval_addr = (char *)Memory.VirtualToRealAddr(optval_addr);
	int ret = setsockopt(s, level, optname, _optval_addr, optlen);
	g_lastError = getLastError();
	return ret;
}

int sys_net_shutdown(s32 s, s32 how)
{
	sys_net.Warning("shutdown(s=%d, how=%d)", s, how);
	int ret = shutdown(s, how);
	g_lastError = getLastError();
	return ret;
}

int sys_net_socket(s32 family, s32 type, s32 protocol)
{
	sys_net.Warning("socket(family=%d, type=%d, protocol=%d)", family, type, protocol);
	int ret = socket(family, type, protocol);
	g_lastError = getLastError();
	return ret;
}

int sys_net_socketclose(s32 s)
{
	sys_net.Warning("socket(s=%d)", s);
#ifdef _WIN32
	int ret = closesocket(s);
#else
	int ret = close(s);
#endif
	g_lastError = getLastError();
	return ret;
}

int socketpoll()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int socketselect()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_initialize_network_ex(mem_ptr_t<sys_net_initialize_parameter> param)
{
	sys_net.Warning("sys_net_initialize_network_ex(param_addr=0x%x)", param.GetAddr());
	g_lastError.SetAddr(Memory.Alloc(4, 1));
#ifdef _WIN32
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1,1);
	WSAStartup(wVersionRequested, &wsaData);
#endif
	return CELL_OK;
}

int sys_net_get_udpp2p_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_set_udpp2p_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_get_lib_name_server()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_if_ctl()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_get_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_get_sockinfo()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_close_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_set_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_show_nameserver()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

s32 _sys_net_errno_loc()
{
	sys_net.Warning("_sys_net_errno_loc()");
	return g_lastError.GetAddr();
}

int sys_net_set_resolver_configurations()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_show_route()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_read_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_abort_resolver()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_abort_socket()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_set_lib_name_server()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_get_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_get_sockinfo_ex()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_open_dump()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_show_ifconfig()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_finalize_network()
{
	sys_net.Warning("sys_net_initialize_network_ex()");
	Memory.Free(g_lastError.GetAddr());
	g_lastError.SetAddr(NULL);
#ifdef _WIN32
	WSACleanup();
#endif
	return CELL_OK;
}

int _sys_net_h_errno_loc()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_set_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sys_net_free_thread_context()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

void sys_net_init()
{
	// The names of the following functions are modified to avoid overloading problems
	sys_net.AddFunc(0xc94f6939, sys_net_accept);
	sys_net.AddFunc(0xb0a59804, sys_net_bind);
	sys_net.AddFunc(0x64f66d35, sys_net_connect);
	//sys_net.AddFunc(0xf7ac8941, sys_net_gethostbyaddr);
	//sys_net.AddFunc(0x71f4c717, sys_net_gethostbyname);
	//sys_net.AddFunc(0xf9ec2db6, sys_net_getpeername);
	//sys_net.AddFunc(0x13efe7f5, sys_net_getsockname);
	//sys_net.AddFunc(0x5a045bd1, sys_net_getsockopt);
	//sys_net.AddFunc(0xdabbc2c0, sys_net_inet_addr);
	//sys_net.AddFunc(0xa9a079e0, sys_net_inet_aton);
	//sys_net.AddFunc(0x566893ce, sys_net_inet_lnaof);
	//sys_net.AddFunc(0xb4152c74, sys_net_inet_makeaddr);
	//sys_net.AddFunc(0xe39a62a7, sys_net_inet_netof);
	//sys_net.AddFunc(0x506ad863, sys_net_inet_network);
	//sys_net.AddFunc(0x858a930b, sys_net_inet_ntoa);
	//sys_net.AddFunc(0xc98a3146, sys_net_inet_ntop);
	sys_net.AddFunc(0x8af3825e, sys_net_inet_pton);
	//sys_net.AddFunc(0x28e208bb, sys_net_listen);
	//sys_net.AddFunc(, sys_net_ntohl);
	//sys_net.AddFunc(, sys_net_ntohs);
	sys_net.AddFunc(0xfba04f37, sys_net_recv);
	sys_net.AddFunc(0x1f953b9f, sys_net_recvfrom);
	//sys_net.AddFunc(0xc9d09c34, sys_net_recvmsg);
	sys_net.AddFunc(0xdc751b40, sys_net_send);
	//sys_net.AddFunc(0xad09481b, sys_net_sendmsg);
	sys_net.AddFunc(0x9647570b, sys_net_sendto);
	sys_net.AddFunc(0x88f03575, sys_net_setsockopt);
	sys_net.AddFunc(0xa50777c6, sys_net_shutdown);
	sys_net.AddFunc(0x9c056962, sys_net_socket);
	sys_net.AddFunc(0x6db6e8cd, sys_net_socketclose);
	//sys_net.AddFunc(0x051ee3ee, sys_net_socketpoll);
	//sys_net.AddFunc(0x3f09e20a, sys_net_socketselect);

	sys_net.AddFunc(0x139a9e9b, sys_net_initialize_network_ex);
	sys_net.AddFunc(0x05bd4438, sys_net_get_udpp2p_test_param);
	sys_net.AddFunc(0x10b81ed6, sys_net_set_udpp2p_test_param);
	sys_net.AddFunc(0x1d14d6e4, sys_net_get_lib_name_server);
	sys_net.AddFunc(0x27fb339d, sys_net_if_ctl);
	sys_net.AddFunc(0x368823c0, sys_net_get_netemu_test_param);
	sys_net.AddFunc(0x3b27c780, sys_net_get_sockinfo);
	sys_net.AddFunc(0x44328aa2, sys_net_close_dump);
	sys_net.AddFunc(0x4ab0b9b9, sys_net_set_test_param);
	sys_net.AddFunc(0x5420e419, sys_net_show_nameserver);
	sys_net.AddFunc(0x6005cde1, _sys_net_errno_loc);
	sys_net.AddFunc(0x7687d48c, sys_net_set_resolver_configurations);
	sys_net.AddFunc(0x79b61646, sys_net_show_route);
	sys_net.AddFunc(0x89c9917c, sys_net_read_dump);
	sys_net.AddFunc(0x8ccf05ed, sys_net_abort_resolver);
	sys_net.AddFunc(0x8d1b77fb, sys_net_abort_socket);
	sys_net.AddFunc(0x9a318259, sys_net_set_lib_name_server);
	sys_net.AddFunc(0xa5a86557, sys_net_get_test_param);
	sys_net.AddFunc(0xa765d029, sys_net_get_sockinfo_ex);
	sys_net.AddFunc(0xab447704, sys_net_open_dump);
	sys_net.AddFunc(0xb48636c4, sys_net_show_ifconfig);
	sys_net.AddFunc(0xb68d5625, sys_net_finalize_network);
	sys_net.AddFunc(0xc9157d30, _sys_net_h_errno_loc);
	sys_net.AddFunc(0xe2434507, sys_net_set_netemu_test_param);
	sys_net.AddFunc(0xfdb8f926, sys_net_free_thread_context);
}
