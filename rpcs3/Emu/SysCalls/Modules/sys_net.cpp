#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_net_init();
Module sys_net((u16)0x0000, sys_net_init);

int accept()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int bind()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int connect()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
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

int inet_pton()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int listen()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int recv()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int recvfrom()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int recvmsg()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int send()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sendmsg()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int sendto()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int setsockopt()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int shutdown()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int socket()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
}

int socketclose()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
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

int sys_net_initialize_network_ex()
{
	UNIMPLEMENTED_FUNC(sys_net);
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

int _sys_net_errno_loc()
{
	UNIMPLEMENTED_FUNC(sys_net);
	return CELL_OK;
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
	UNIMPLEMENTED_FUNC(sys_net);
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
	// (TODO: Fix function overloading problem due to winsock.h and find addresses for ntohl and ntohs)

	//sys_net.AddFunc(0xc94f6939, accept);
	//sys_net.AddFunc(0xb0a59804, bind);
	//sys_net.AddFunc(0x64f66d35, connect);
	//sys_net.AddFunc(0xf7ac8941, gethostbyaddr);
	//sys_net.AddFunc(0x71f4c717, gethostbyname);
	//sys_net.AddFunc(0xf9ec2db6, getpeername);
	//sys_net.AddFunc(0x13efe7f5, getsockname);
	//sys_net.AddFunc(0x5a045bd1, getsockopt);
	//sys_net.AddFunc(0xdabbc2c0, inet_addr);
	sys_net.AddFunc(0xa9a079e0, inet_aton);
	sys_net.AddFunc(0x566893ce, inet_lnaof);
	sys_net.AddFunc(0xb4152c74, inet_makeaddr);
	sys_net.AddFunc(0xe39a62a7, inet_netof);
	sys_net.AddFunc(0x506ad863, inet_network);
	//sys_net.AddFunc(0x858a930b, inet_ntoa);
	sys_net.AddFunc(0xc98a3146, inet_ntop);
	sys_net.AddFunc(0x8af3825e, inet_pton);
	//sys_net.AddFunc(0x28e208bb, listen);
	//sys_net.AddFunc(, ntohl);
	//sys_net.AddFunc(, ntohs);
	//sys_net.AddFunc(0xfba04f37, recv);
	//sys_net.AddFunc(0x1f953b9f, recvfrom);
	sys_net.AddFunc(0xc9d09c34, recvmsg);
	//sys_net.AddFunc(0xdc751b40, send);
	sys_net.AddFunc(0xad09481b, sendmsg);
	//sys_net.AddFunc(0x9647570b, sendto);
	//sys_net.AddFunc(0x88f03575, setsockopt);
	//sys_net.AddFunc(0xa50777c6, shutdown);
	//sys_net.AddFunc(0x9c056962, socket);
	sys_net.AddFunc(0x6db6e8cd, socketclose);
	sys_net.AddFunc(0x051ee3ee, socketpoll);
	sys_net.AddFunc(0x3f09e20a, socketselect);

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