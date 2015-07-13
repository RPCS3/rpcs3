#pragma once

struct sys_net_initialize_parameter
{
	be_t<u32> memory_addr;
	be_t<s32> memory_size;
	be_t<s32> flags;
};

// The names of the following structs are modified to avoid overloading problems
struct sys_net_sockaddr
{
	u8 sa_len;
	u8 sa_family; // sa_family_t
	u8 sa_data[14];
};

struct sys_net_sockaddr_in
{
	u8 sin_len;
	u8 sin_family;
	be_t<u16> sin_port;
	be_t<u32> sa_addr; // struct in_addr
	u8 unused[8];
};
