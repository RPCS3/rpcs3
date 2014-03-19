#pragma once

struct sys_net_initialize_parameter
{
	u32 memory_addr;
	int memory_size;
	int flags;
};

// The names of the following structs are modified to avoid overloading problems
struct sys_net_sockaddr
{
	u8 sa_len;
	u8 sa_family; // sa_family_t
	u8 sa_data[14];
};
