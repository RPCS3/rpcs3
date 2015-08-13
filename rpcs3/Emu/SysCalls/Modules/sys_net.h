#pragma once

namespace vm { using namespace ps3; }

// must die
#undef s_addr

namespace sys_net
{
	// only for reference, no need to use it
	using in_addr_t = u32;
	using in_port_t = u16;
	using sa_family_t = u8;
	using socklen_t = u32;

	struct fd_set
	{
		be_t<u32> fds_bits[32];
	};

	struct hostent
	{
		vm::bptr<char> h_name;
		vm::bpptr<char> h_aliases;
		be_t<s32> h_addrtype;
		be_t<s32> h_length;
		vm::bpptr<char> h_addr_list;
	};

	struct in_addr
	{
		be_t<u32> s_addr;
	};

	struct iovec
	{
		be_t<s32> zero1;
		vm::bptr<void> iov_base;
		be_t<s32> zero2;
		be_t<u32> iov_len;
	};

	struct ip_mreq
	{
		be_t<u32> imr_multiaddr;
		be_t<u32> imr_interface;
	};

	struct msghdr
	{
		be_t<s32> zero1;
		vm::bptr<void> msg_name;
		be_t<u32> msg_namelen;
		be_t<s32> pad1;
		be_t<s32> zero2;
		vm::bptr<iovec> msg_iov;
		be_t<s32> msg_iovlen;
		be_t<s32> pad2;
		be_t<s32> zero3;
		vm::bptr<void> msg_control;
		be_t<u32> msg_controllen;
		be_t<s32> msg_flags;
	};

	struct pollfd
	{
		be_t<s32> fd;
		be_t<s16> events;
		be_t<s16> revents;
	};

	struct sockaddr
	{
		u8 sa_len;
		u8 sa_family;
		char sa_data[14];
	};

	struct sockaddr_dl
	{
		u8 sdl_len;
		u8 sdl_family;
		be_t<u16> sdl_index;
		u8 sdl_type;
		u8 sdl_nlen;
		u8 sdl_alen;
		u8 sdl_slen;
		char sdl_data[12];
	};

	struct sockaddr_in
	{
		u8 sin_len;
		u8 sin_family;
		be_t<u16> sin_port;
		be_t<u32> sin_addr;
		char sin_zero[8];
	};

	struct sockaddr_in_p2p
	{
		u8 sin_len;
		u8 sin_family;
		be_t<u16> sin_port;
		be_t<u32> sin_addr;
		be_t<u16> sin_vport;
		char sin_zero[6];
	};

	struct timeval
	{
		be_t<s64> tv_sec;
		be_t<s64> tv_usec;
	};
}

struct sys_net_initialize_parameter_t
{
	vm::bptr<void> memory;
	be_t<s32> memory_size;
	be_t<s32> flags;
};

// PS3 libnet socket struct
struct net_socket_t
{
	std::intptr_t native_handle;
};
