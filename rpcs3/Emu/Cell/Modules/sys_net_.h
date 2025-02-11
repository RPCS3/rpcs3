#pragma once

#include "Emu/Cell/lv2/sys_net.h"

struct sys_net_sockinfo_t
{
	be_t<s32> s;
	be_t<s32> proto;
	be_t<s32> recv_queue_length;
	be_t<s32> send_queue_length;
	sys_net_in_addr local_adr;
	be_t<s32> local_port;
	sys_net_in_addr remote_adr;
	be_t<s32> remote_port;
	be_t<s32> state;
};

struct sys_net_sockinfo_ex_t
{
	be_t<s32> s;
	be_t<s32> proto;
	be_t<s32> recv_queue_length;
	be_t<s32> send_queue_length;
	sys_net_in_addr local_adr;
	be_t<s32> local_port;
	sys_net_in_addr remote_adr;
	be_t<s32> remote_port;
	be_t<s32> state;
	be_t<s32> socket_type;
	be_t<s32> local_vport;
	be_t<s32> remote_vport;
	be_t<s32> reserved[8];
};

struct sys_net_initialize_parameter_t
{
	vm::bptr<void> memory;
	be_t<s32> memory_size;
	be_t<s32> flags;
};
