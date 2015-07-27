#pragma once

namespace vm { using namespace ps3; }

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

struct sys_net_in_addr
{
	be_t<u32> sa_addr;
};

struct sys_net_fd_set
{
	be_t<u32> fds_bits[32];
};

struct sys_net_timeval
{
	be_t<s64> tv_sec;
	be_t<s64> tv_usec;
};
