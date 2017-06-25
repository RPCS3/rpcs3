#if defined(__FreeBSD__)
#include <sys/select.h>
#undef fds_bits
#endif

#pragma once

namespace vm { using namespace ps3; }

namespace sys_net
{
	// Error codes
	enum
	{
		SYS_NET_ENOENT          = 2,
		SYS_NET_EINTR           = 4,
		SYS_NET_EBADF           = 9,
		SYS_NET_ENOMEM          = 12,
		SYS_NET_EACCES          = 13,
		SYS_NET_EFAULT          = 14,
		SYS_NET_EBUSY           = 16,
		SYS_NET_EINVAL          = 22,
		SYS_NET_EMFILE          = 24,
		SYS_NET_ENOSPC          = 28,
		SYS_NET_EPIPE           = 32,
		SYS_NET_EAGAIN          = 35,
		SYS_NET_EWOULDBLOCK     = SYS_NET_EAGAIN,
		SYS_NET_EINPROGRESS     = 36,
		SYS_NET_EALREADY        = 37,
		SYS_NET_EDESTADDRREQ    = 39,
		SYS_NET_EMSGSIZE        = 40,
		SYS_NET_EPROTOTYPE      = 41,
		SYS_NET_ENOPROTOOPT     = 42,
		SYS_NET_EPROTONOSUPPORT = 43,
		SYS_NET_EOPNOTSUPP      = 45,
		SYS_NET_EPFNOSUPPORT    = 46,
		SYS_NET_EAFNOSUPPORT    = 47,
		SYS_NET_EADDRINUSE      = 48,
		SYS_NET_EADDRNOTAVAIL   = 49,
		SYS_NET_ENETDOWN        = 50,
		SYS_NET_ENETUNREACH     = 51,
		SYS_NET_ECONNABORTED    = 53,
		SYS_NET_ECONNRESET      = 54,
		SYS_NET_ENOBUFS         = 55,
		SYS_NET_EISCONN         = 56,
		SYS_NET_ENOTCONN        = 57,
		SYS_NET_ESHUTDOWN       = 58,
		SYS_NET_ETOOMANYREFS    = 59,
		SYS_NET_ETIMEDOUT       = 60,
		SYS_NET_ECONNREFUSED    = 61,
		SYS_NET_EHOSTDOWN       = 64,
		SYS_NET_EHOSTUNREACH    = 65,
	};

	// Socket types
	enum
	{
		SOCK_STREAM     = 1,
		SOCK_DGRAM      = 2,
		SOCK_RAW        = 3,
		SOCK_DGRAM_P2P  = 6,
		SOCK_STREAM_P2P = 10,
	};

	// Socket options
	// Note: All options are prefixed with "OP_" to prevent name conflicts.
	enum
	{
		OP_SO_SNDBUF   = 0x1001,
		OP_SO_RCVBUF   = 0x1002,
		OP_SO_SNDLOWAT = 0x1003,
		OP_SO_RCVLOWAT = 0x1004,
		OP_SO_SNDTIMEO = 0x1005,
		OP_SO_RCVTIMEO = 0x1006,
		OP_SO_ERROR    = 0x1007,
		OP_SO_TYPE     = 0x1008,
		OP_SO_NBIO     = 0x1100, // Non-blocking IO
		OP_SO_TPPOLICY = 0x1101,
		
		OP_SO_REUSEADDR	= 0x0004,
		OP_SO_KEEPALIVE	= 0x0008,
		OP_SO_BROADCAST	= 0x0020,
		OP_SO_LINGER	= 0x0080,
		OP_SO_OOBINLINE	= 0x0100,
		OP_SO_REUSEPORT	= 0x0200,
		OP_SO_ONESBCAST	= 0x0800,
		OP_SO_USECRYPTO	= 0x1000,
		OP_SO_USESIGNATURE = 0x2000,
	};

	// TCP options
	enum
	{
		OP_TCP_NODELAY          = 1,
		OP_TCP_MAXSEG           = 2,
		OP_TCP_MSS_TO_ADVERTISE = 3,
	};

	// IP protocols
	// Note: Proctols are prefixed with "PROTO_" to prevent name conflicts
	enum
	{
		PROTO_IPPROTO_IP     = 0,
		PROTO_IPPROTO_ICMP   = 1,
		PROTO_IPPROTO_IGMP   = 2,
		PROTO_IPPROTO_TCP    = 6,
		PROTO_IPPROTO_UDP    = 17,
		PROTO_IPPROTO_ICMPV6 = 58,
	};

	// only for reference, no need to use it
	using in_addr_t   = u32;
	using in_port_t   = u16;
	using sa_family_t = u8;
	using socklen_t   = u32;

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

	struct sys_net_sockinfo_t
	{
		be_t<s32> s;
		be_t<s32> proto;
		be_t<s32> recv_queue_length;
		be_t<s32> send_queue_length;
		in_addr local_adr;
		be_t<s32> local_port;
		in_addr remote_adr;
		be_t<s32> remote_port;
		be_t<s32> state;
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
