#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "sys_net_.h"

LOG_CHANNEL(libnet);

s32 sys_net_accept(s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	libnet.todo("accept(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	return 0;
}

s32 sys_net_bind(s32 s, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	libnet.todo("bind(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	return 0;
}

s32 sys_net_connect(s32 s, vm::ptr<sys_net_sockaddr> addr, u32 addrlen)
{
	libnet.todo("connect(s=%d, addr=*0x%x, addrlen=%u)", s, addr, addrlen);

	return 0;
}

s32 sys_net_gethostbyaddr()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_gethostbyname()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_getpeername(s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	libnet.todo("getpeername(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	return CELL_OK;
}

s32 sys_net_getsockname(s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	libnet.todo("getsockname(s=%d, addr=*0x%x, paddrlen=*0x%x)", s, addr, paddrlen);

	return CELL_OK;
}

s32 sys_net_getsockopt(s32 s, s32 level, s32 optname, vm::ptr<void> optval, vm::ptr<u32> optlen)
{
	libnet.todo("getsockopt(s=%d, level=%d, optname=%d, optval=*0x%x, optlen=*0x%x)", s, level, optname, optval, optlen);

	return CELL_OK;
}

u32 sys_net_inet_addr(vm::cptr<char> cp)
{
	libnet.todo("inet_addr(cp=%s)", cp);

	return 0xffffffff;
}

s32 sys_net_inet_aton()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_inet_lnaof()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_inet_makeaddr()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_inet_netof()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_inet_network()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

vm::ptr<char> sys_net_inet_ntoa(u32 in)
{
	libnet.todo("inet_ntoa(in=0x%x)", in);

	return vm::null;
}

vm::cptr<char> sys_net_inet_ntop(s32 af, vm::ptr<void> src, vm::ptr<char> dst, u32 size)
{
	libnet.todo("inet_ntop(af=%d, src=%s, dst=*0x%x, size=%d)", af, src, dst, size);

	return vm::null;
}

s32 sys_net_inet_pton(s32 af, vm::cptr<char> src, vm::ptr<char> dst)
{
	libnet.todo("inet_pton(af=%d, src=%s, dst=*0x%x)", af, src, dst);

	return 0;
}

s32 sys_net_listen(s32 s, s32 backlog)
{
	libnet.todo("listen(s=%d, backlog=%d)", s, backlog);

	return 0;
}

s32 sys_net_recv(s32 s, vm::ptr<void> buf, u32 len, s32 flags)
{
	libnet.todo("recv(s=%d, buf=*0x%x, len=%d, flags=0x%x)", s, buf, len, flags);

	return 0;
}

s32 sys_net_recvfrom(s32 s, vm::ptr<void> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen)
{
	libnet.todo("recvfrom(s=%d, buf=*0x%x, len=%d, flags=0x%x, addr=*0x%x, paddrlen=*0x%x)", s, buf, len, flags, addr, paddrlen);

	return 0;
}

s32 sys_net_recvmsg(s32 s, vm::ptr<sys_net_msghdr> msg, s32 flags)
{
	libnet.todo("recvmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);

	return CELL_OK;
}

s32 sys_net_send(s32 s, vm::cptr<void> buf, u32 len, s32 flags)
{
	libnet.todo("send(s=%d, buf=*0x%x, len=%d, flags=0x%x)", s, buf, len, flags);

	return 0;
}

s32 sys_net_sendmsg(s32 s, vm::cptr<sys_net_msghdr> msg, s32 flags)
{
	libnet.todo("sendmsg(s=%d, msg=*0x%x, flags=0x%x)", s, msg, flags);

	return CELL_OK;
}

s32 sys_net_sendto(s32 s, vm::cptr<void> buf, u32 len, s32 flags, vm::cptr<sys_net_sockaddr> addr, u32 addrlen)
{
	libnet.todo("sendto(s=%d, buf=*0x%x, len=%d, flags=0x%x, addr=*0x%x, addrlen=%d)", s, buf, len, flags, addr, addrlen);

	return 0;
}

s32 sys_net_setsockopt(s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen)
{
	libnet.todo("setsockopt(s=%d, level=%d, optname=%d, optval=*0x%x, optlen=%d)", s, level, optname, optval, optlen);

	return 0;
}

s32 sys_net_shutdown(s32 s, s32 how)
{
	libnet.todo("shutdown(s=%d, how=%d)", s, how);

	return 0;
}

s32 sys_net_socket(s32 family, s32 type, s32 protocol)
{
	libnet.todo("socket(family=%d, type=%d, protocol=%d)", family, type, protocol);

	return 0;
}

s32 sys_net_socketclose(s32 s)
{
	libnet.warning("socketclose(s=%d)", s);

	return 0;
}

s32 sys_net_socketpoll(vm::ptr<sys_net_pollfd> fds, s32 nfds, s32 ms)
{
	libnet.todo("socketpoll(fds=*0x%x, nfds=%d, ms=%d)", fds, nfds, ms);

	return CELL_OK;
}

s32 sys_net_socketselect(s32 nfds, vm::ptr<sys_net_fd_set> readfds, vm::ptr<sys_net_fd_set> writefds, vm::ptr<sys_net_fd_set> exceptfds, vm::ptr<sys_net_timeval> timeout)
{
	libnet.todo("socketselect(nfds=%d, readfds=*0x%x, writefds=*0x%x, exceptfds=*0x%x, timeout=*0x%x)", nfds, readfds, writefds, exceptfds, timeout);

	return 0;
}

s32 sys_net_initialize_network_ex(vm::ptr<sys_net_initialize_parameter_t> param)
{
	libnet.todo("sys_net_initialize_network_ex(param=*0x%x)", param);

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

s32 sys_net_get_if_list()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_get_name_server()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_get_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_get_routing_table_af()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_get_sockinfo(s32 s, vm::ptr<sys_net_sockinfo_t> p, s32 n)
{
	libnet.todo("sys_net_get_sockinfo(s=%d, p=*0x%x, n=%d)", s, p, n);

	return CELL_OK;
}

s32 sys_net_close_dump(s32 id, vm::ptr<s32> pflags)
{
	libnet.todo("sys_net_close_dump(id=%d, pflags=*0x%x)", id, pflags);

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

vm::ptr<s32> _sys_net_errno_loc(ppu_thread& ppu)
{
	libnet.warning("_sys_net_errno_loc()");

	// Return fake location from system TLS area
	return vm::cast(ppu.gpr[13] - 0x7030 + 0x2c);
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

s32 sys_net_read_dump(s32 id, vm::ptr<void> buf, s32 len, vm::ptr<s32> pflags)
{
	libnet.todo("sys_net_read_dump(id=%d, buf=*0x%x, len=%d, pflags=*0x%x)", id, buf, len, pflags);

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

s32 sys_net_open_dump(s32 len, s32 flags)
{
	libnet.todo("sys_net_open_dump(len=%d, flags=0x%x)", len, flags);

	return CELL_OK;
}

s32 sys_net_show_ifconfig()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_finalize_network()
{
	libnet.todo("sys_net_finalize_network()");

	return CELL_OK;
}

vm::ptr<s32> _sys_net_h_errno_loc(ppu_thread& ppu)
{
	libnet.warning("_sys_net_h_errno_loc()");

	// Return fake location from system TLS area
	return vm::cast(ppu.gpr[13] - 0x7030 + 0x28);
}

s32 sys_net_set_netemu_test_param()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_net_free_thread_context(u64 tid, s32 flags)
{
	libnet.todo("sys_net_free_thread_context(tid=0x%x, flags=%d)", tid, flags);

	return CELL_OK;
}

s32 _sys_net_lib_abort()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_bnet_control()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 __sys_net_lib_calloc()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_free()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_get_system_time()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_if_nametoindex()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_ioctl()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 __sys_net_lib_malloc()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_rand()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 __sys_net_lib_realloc()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_reset_libnetctl_queue()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_set_libnetctl_queue()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_thread_create()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_thread_exit()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_thread_join()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sync_clear()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sync_create()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sync_destroy()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sync_signal()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sync_wait()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_sysctl()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sys_net_lib_usleep()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_abort()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_close()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_get_if_id()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_get_status()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_if_down()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_get_key_value()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}
s32 sys_netset_if_up()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 sys_netset_open()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_get_name_server()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_add_name_server()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_add_name_server_with_char()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_flush_route()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_set_default_gateway()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_set_ip_and_mask()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

s32 _sce_net_set_name_server()
{
	UNIMPLEMENTED_FUNC(libnet);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sys_net)("sys_net", []()
{
	REG_FNID(sys_net, "accept", sys_net_accept);
	REG_FNID(sys_net, "bind", sys_net_bind);
	REG_FNID(sys_net, "connect", sys_net_connect);
	REG_FNID(sys_net, "gethostbyaddr", sys_net_gethostbyaddr);
	REG_FNID(sys_net, "gethostbyname", sys_net_gethostbyname);
	REG_FNID(sys_net, "getpeername", sys_net_getpeername);
	REG_FNID(sys_net, "getsockname", sys_net_getsockname);
	REG_FNID(sys_net, "getsockopt", sys_net_getsockopt);
	REG_FNID(sys_net, "inet_addr", sys_net_inet_addr);
	REG_FNID(sys_net, "inet_aton", sys_net_inet_aton);
	REG_FNID(sys_net, "inet_lnaof", sys_net_inet_lnaof);
	REG_FNID(sys_net, "inet_makeaddr", sys_net_inet_makeaddr);
	REG_FNID(sys_net, "inet_netof", sys_net_inet_netof);
	REG_FNID(sys_net, "inet_network", sys_net_inet_network);
	REG_FNID(sys_net, "inet_ntoa", sys_net_inet_ntoa);
	REG_FNID(sys_net, "inet_ntop", sys_net_inet_ntop);
	REG_FNID(sys_net, "inet_pton", sys_net_inet_pton);
	REG_FNID(sys_net, "listen", sys_net_listen);
	REG_FNID(sys_net, "recv", sys_net_recv);
	REG_FNID(sys_net, "recvfrom", sys_net_recvfrom);
	REG_FNID(sys_net, "recvmsg", sys_net_recvmsg);
	REG_FNID(sys_net, "send", sys_net_send);
	REG_FNID(sys_net, "sendmsg", sys_net_sendmsg);
	REG_FNID(sys_net, "sendto", sys_net_sendto);
	REG_FNID(sys_net, "setsockopt", sys_net_setsockopt);
	REG_FNID(sys_net, "shutdown", sys_net_shutdown);
	REG_FNID(sys_net, "socket", sys_net_socket);
	REG_FNID(sys_net, "socketclose", sys_net_socketclose);
	REG_FNID(sys_net, "socketpoll", sys_net_socketpoll);
	REG_FNID(sys_net, "socketselect", sys_net_socketselect);

	REG_FUNC(sys_net, sys_net_initialize_network_ex);
	REG_FUNC(sys_net, sys_net_get_udpp2p_test_param);
	REG_FUNC(sys_net, sys_net_set_udpp2p_test_param);
	REG_FUNC(sys_net, sys_net_get_lib_name_server);
	REG_FUNC(sys_net, sys_net_if_ctl);
	REG_FUNC(sys_net, sys_net_get_if_list);
	REG_FUNC(sys_net, sys_net_get_name_server);
	REG_FUNC(sys_net, sys_net_get_netemu_test_param);
	REG_FUNC(sys_net, sys_net_get_routing_table_af);
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

	REG_FUNC(sys_net, _sys_net_lib_abort);
	REG_FUNC(sys_net, _sys_net_lib_bnet_control);
	REG_FUNC(sys_net, __sys_net_lib_calloc);
	REG_FUNC(sys_net, _sys_net_lib_free);
	REG_FUNC(sys_net, _sys_net_lib_get_system_time);
	REG_FUNC(sys_net, _sys_net_lib_if_nametoindex);
	REG_FUNC(sys_net, _sys_net_lib_ioctl);
	REG_FUNC(sys_net, __sys_net_lib_malloc);
	REG_FUNC(sys_net, _sys_net_lib_rand);
	REG_FUNC(sys_net, __sys_net_lib_realloc);
	REG_FUNC(sys_net, _sys_net_lib_reset_libnetctl_queue);
	REG_FUNC(sys_net, _sys_net_lib_set_libnetctl_queue);

	REG_FUNC(sys_net, _sys_net_lib_thread_create);
	REG_FUNC(sys_net, _sys_net_lib_thread_exit);
	REG_FUNC(sys_net, _sys_net_lib_thread_join);

	REG_FUNC(sys_net, _sys_net_lib_sync_clear);
	REG_FUNC(sys_net, _sys_net_lib_sync_create);
	REG_FUNC(sys_net, _sys_net_lib_sync_destroy);
	REG_FUNC(sys_net, _sys_net_lib_sync_signal);
	REG_FUNC(sys_net, _sys_net_lib_sync_wait);

	REG_FUNC(sys_net, _sys_net_lib_sysctl);
	REG_FUNC(sys_net, _sys_net_lib_usleep);

	REG_FUNC(sys_net, sys_netset_abort);
	REG_FUNC(sys_net, sys_netset_close);
	REG_FUNC(sys_net, sys_netset_get_if_id);
	REG_FUNC(sys_net, sys_netset_get_key_value);
	REG_FUNC(sys_net, sys_netset_get_status);
	REG_FUNC(sys_net, sys_netset_if_down);
	REG_FUNC(sys_net, sys_netset_if_up);
	REG_FUNC(sys_net, sys_netset_open);

	REG_FUNC(sys_net, _sce_net_add_name_server);
	REG_FUNC(sys_net, _sce_net_add_name_server_with_char);
	REG_FUNC(sys_net, _sce_net_flush_route);

	REG_FUNC(sys_net, _sce_net_get_name_server);
	REG_FUNC(sys_net, _sce_net_set_default_gateway);
	REG_FUNC(sys_net, _sce_net_set_ip_and_mask);
	REG_FUNC(sys_net, _sce_net_set_name_server);
});
