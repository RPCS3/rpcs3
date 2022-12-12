#pragma once

#include "Utilities/bit_set.h"
#include "Utilities/mutex.h"

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

#include <vector>
#include <utility>
#include <functional>
#include <queue>

// Error codes
enum sys_net_error : s32
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

static constexpr sys_net_error operator-(sys_net_error v)
{
	return sys_net_error{-+v};
}

// Socket types (prefixed with SYS_NET_)
enum lv2_socket_type : s32
{
	SYS_NET_SOCK_STREAM     = 1,
	SYS_NET_SOCK_DGRAM      = 2,
	SYS_NET_SOCK_RAW        = 3,
	SYS_NET_SOCK_DGRAM_P2P  = 6,
	SYS_NET_SOCK_STREAM_P2P = 10,
};

// Socket options (prefixed with SYS_NET_)
enum lv2_socket_option : s32
{
	SYS_NET_SO_SNDBUF       = 0x1001,
	SYS_NET_SO_RCVBUF       = 0x1002,
	SYS_NET_SO_SNDLOWAT     = 0x1003,
	SYS_NET_SO_RCVLOWAT     = 0x1004,
	SYS_NET_SO_SNDTIMEO     = 0x1005,
	SYS_NET_SO_RCVTIMEO     = 0x1006,
	SYS_NET_SO_ERROR        = 0x1007,
	SYS_NET_SO_TYPE         = 0x1008,
	SYS_NET_SO_NBIO         = 0x1100, // Non-blocking IO
	SYS_NET_SO_TPPOLICY     = 0x1101,

	SYS_NET_SO_REUSEADDR    = 0x0004,
	SYS_NET_SO_KEEPALIVE    = 0x0008,
	SYS_NET_SO_BROADCAST    = 0x0020,
	SYS_NET_SO_LINGER       = 0x0080,
	SYS_NET_SO_OOBINLINE    = 0x0100,
	SYS_NET_SO_REUSEPORT    = 0x0200,
	SYS_NET_SO_ONESBCAST    = 0x0800,
	SYS_NET_SO_USECRYPTO    = 0x1000,
	SYS_NET_SO_USESIGNATURE = 0x2000,

	SYS_NET_SOL_SOCKET      = 0xffff,
};

// IP options (prefixed with SYS_NET_)
enum lv2_ip_option : s32
{
	SYS_NET_IP_HDRINCL         = 2,
	SYS_NET_IP_TOS             = 3,
	SYS_NET_IP_TTL             = 4,
	SYS_NET_IP_MULTICAST_IF    = 9,
	SYS_NET_IP_MULTICAST_TTL   = 10,
	SYS_NET_IP_MULTICAST_LOOP  = 11,
	SYS_NET_IP_ADD_MEMBERSHIP  = 12,
	SYS_NET_IP_DROP_MEMBERSHIP = 13,
	SYS_NET_IP_TTLCHK          = 23,
	SYS_NET_IP_MAXTTL          = 24,
	SYS_NET_IP_DONTFRAG        = 26
};

// Family (prefixed with SYS_NET_)
enum lv2_socket_family : s32
{
	SYS_NET_AF_UNSPEC       = 0,
	SYS_NET_AF_LOCAL        = 1,
	SYS_NET_AF_UNIX         = SYS_NET_AF_LOCAL,
	SYS_NET_AF_INET         = 2,
	SYS_NET_AF_INET6        = 24,
};

// Flags (prefixed with SYS_NET_)
enum
{
	SYS_NET_MSG_OOB         = 0x1,
	SYS_NET_MSG_PEEK        = 0x2,
	SYS_NET_MSG_DONTROUTE   = 0x4,
	SYS_NET_MSG_EOR         = 0x8,
	SYS_NET_MSG_TRUNC       = 0x10,
	SYS_NET_MSG_CTRUNC      = 0x20,
	SYS_NET_MSG_WAITALL     = 0x40,
	SYS_NET_MSG_DONTWAIT    = 0x80,
	SYS_NET_MSG_BCAST       = 0x100,
	SYS_NET_MSG_MCAST       = 0x200,
	SYS_NET_MSG_USECRYPTO   = 0x400,
	SYS_NET_MSG_USESIGNATURE= 0x800,
};

// Shutdown types (prefixed with SYS_NET_)
enum
{
	SYS_NET_SHUT_RD         = 0,
	SYS_NET_SHUT_WR         = 1,
	SYS_NET_SHUT_RDWR       = 2,
};

// TCP options (prefixed with SYS_NET_)
enum lv2_tcp_option : s32
{
	SYS_NET_TCP_NODELAY          = 1,
	SYS_NET_TCP_MAXSEG           = 2,
	SYS_NET_TCP_MSS_TO_ADVERTISE = 3,
};

// IP protocols (prefixed with SYS_NET_)
enum lv2_ip_protocol : s32
{
	SYS_NET_IPPROTO_IP     = 0,
	SYS_NET_IPPROTO_ICMP   = 1,
	SYS_NET_IPPROTO_IGMP   = 2,
	SYS_NET_IPPROTO_TCP    = 6,
	SYS_NET_IPPROTO_UDP    = 17,
	SYS_NET_IPPROTO_ICMPV6 = 58,
};

// Poll events (prefixed with SYS_NET_)
enum
{
	SYS_NET_POLLIN         = 0x0001,
	SYS_NET_POLLPRI        = 0x0002,
	SYS_NET_POLLOUT        = 0x0004,
	SYS_NET_POLLERR        = 0x0008, /* revent only */
	SYS_NET_POLLHUP        = 0x0010, /* revent only */
	SYS_NET_POLLNVAL       = 0x0020, /* revent only */
	SYS_NET_POLLRDNORM     = 0x0040,
	SYS_NET_POLLWRNORM     = SYS_NET_POLLOUT,
	SYS_NET_POLLRDBAND     = 0x0080,
	SYS_NET_POLLWRBAND     = 0x0100,
};

enum lv2_socket_abort_flags : s32
{
	SYS_NET_ABORT_STRICT_CHECK = 1,
};

// in_addr_t type prefixed with sys_net_
using sys_net_in_addr_t    = u32;

// in_port_t type prefixed with sys_net_
using sys_net_in_port_t    = u16;

// sa_family_t type prefixed with sys_net_
using sys_net_sa_family_t  = u8;

// socklen_t type prefixed with sys_net_
using sys_net_socklen_t    = u32;

// fd_set prefixed with sys_net_
struct sys_net_fd_set
{
	be_t<u32> fds_bits[32];

	u32 bit(s32 s) const
	{
		return (fds_bits[(s >> 5) & 31] >> (s & 31)) & 1u;
	}

	void set(s32 s)
	{
		fds_bits[(s >> 5) & 31] |= (1u << (s & 31));
	}
};

// hostent prefixed with sys_net_
struct sys_net_hostent
{
	vm::bptr<char> h_name;
	vm::bpptr<char> h_aliases;
	be_t<s32> h_addrtype;
	be_t<s32> h_length;
	vm::bpptr<char> h_addr_list;
};

// in_addr prefixed with sys_net_
struct sys_net_in_addr
{
	be_t<u32> _s_addr;
};

// iovec prefixed with sys_net_
struct sys_net_iovec
{
	be_t<s32> zero1;
	vm::bptr<void> iov_base;
	be_t<s32> zero2;
	be_t<u32> iov_len;
};

// ip_mreq prefixed with sys_net_
struct sys_net_ip_mreq
{
	be_t<u32> imr_multiaddr;
	be_t<u32> imr_interface;
};

// msghdr prefixed with sys_net_
struct sys_net_msghdr
{
	be_t<s32> zero1;
	vm::bptr<void> msg_name;
	be_t<u32> msg_namelen;
	be_t<s32> pad1;
	be_t<s32> zero2;
	vm::bptr<sys_net_iovec> msg_iov;
	be_t<s32> msg_iovlen;
	be_t<s32> pad2;
	be_t<s32> zero3;
	vm::bptr<void> msg_control;
	be_t<u32> msg_controllen;
	be_t<s32> msg_flags;
};

// pollfd prefixed with sys_net_
struct sys_net_pollfd
{
	be_t<s32> fd;
	be_t<s16> events;
	be_t<s16> revents;
};

// sockaddr prefixed with sys_net_
struct sys_net_sockaddr
{
	ENABLE_BITWISE_SERIALIZATION;

	u8 sa_len;
	u8 sa_family;
	char sa_data[14];
};

// sockaddr_dl prefixed with sys_net_
struct sys_net_sockaddr_dl
{
	ENABLE_BITWISE_SERIALIZATION;

	u8 sdl_len;
	u8 sdl_family;
	be_t<u16> sdl_index;
	u8 sdl_type;
	u8 sdl_nlen;
	u8 sdl_alen;
	u8 sdl_slen;
	char sdl_data[12];
};

// sockaddr_in prefixed with sys_net_
struct sys_net_sockaddr_in
{
	ENABLE_BITWISE_SERIALIZATION;

	u8 sin_len;
	u8 sin_family;
	be_t<u16> sin_port;
	be_t<u32> sin_addr;
	be_t<u64> sin_zero;
};

// sockaddr_in_p2p prefixed with sys_net_
struct sys_net_sockaddr_in_p2p
{
	ENABLE_BITWISE_SERIALIZATION;

	u8 sin_len;
	u8 sin_family;
	be_t<u16> sin_port;
	be_t<u32> sin_addr;
	be_t<u16> sin_vport;
	char sin_zero[6];
};

// timeval prefixed with sys_net_
struct sys_net_timeval
{
	be_t<s64> tv_sec;
	be_t<s64> tv_usec;
};

// linger prefixed with sys_net_
struct sys_net_linger
{
	be_t<s32> l_onoff;
	be_t<s32> l_linger;
};

class ppu_thread;

// Syscalls

error_code sys_net_bnet_accept(ppu_thread&, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen);
error_code sys_net_bnet_bind(ppu_thread&, s32 s, vm::cptr<sys_net_sockaddr> addr, u32 addrlen);
error_code sys_net_bnet_connect(ppu_thread&, s32 s, vm::ptr<sys_net_sockaddr> addr, u32 addrlen);
error_code sys_net_bnet_getpeername(ppu_thread&, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen);
error_code sys_net_bnet_getsockname(ppu_thread&, s32 s, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen);
error_code sys_net_bnet_getsockopt(ppu_thread&, s32 s, s32 level, s32 optname, vm::ptr<void> optval, vm::ptr<u32> optlen);
error_code sys_net_bnet_listen(ppu_thread&, s32 s, s32 backlog);
error_code sys_net_bnet_recvfrom(ppu_thread&, s32 s, vm::ptr<void> buf, u32 len, s32 flags, vm::ptr<sys_net_sockaddr> addr, vm::ptr<u32> paddrlen);
error_code sys_net_bnet_recvmsg(ppu_thread&, s32 s, vm::ptr<sys_net_msghdr> msg, s32 flags);
error_code sys_net_bnet_sendmsg(ppu_thread&, s32 s, vm::cptr<sys_net_msghdr> msg, s32 flags);
error_code sys_net_bnet_sendto(ppu_thread&, s32 s, vm::cptr<void> buf, u32 len, s32 flags, vm::cptr<sys_net_sockaddr> addr, u32 addrlen);
error_code sys_net_bnet_setsockopt(ppu_thread&, s32 s, s32 level, s32 optname, vm::cptr<void> optval, u32 optlen);
error_code sys_net_bnet_shutdown(ppu_thread&, s32 s, s32 how);
error_code sys_net_bnet_socket(ppu_thread&, lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
error_code sys_net_bnet_close(ppu_thread&, s32 s);
error_code sys_net_bnet_poll(ppu_thread&, vm::ptr<sys_net_pollfd> fds, s32 nfds, s32 ms);
error_code sys_net_bnet_select(ppu_thread&, s32 nfds, vm::ptr<sys_net_fd_set> readfds, vm::ptr<sys_net_fd_set> writefds, vm::ptr<sys_net_fd_set> exceptfds, vm::ptr<sys_net_timeval> timeout);
error_code _sys_net_open_dump(ppu_thread&, s32 len, s32 flags);
error_code _sys_net_read_dump(ppu_thread&, s32 id, vm::ptr<void> buf, s32 len, vm::ptr<s32> pflags);
error_code _sys_net_close_dump(ppu_thread&, s32 id, vm::ptr<s32> pflags);
error_code _sys_net_write_dump(ppu_thread&, s32 id, vm::cptr<void> buf, s32 len, u32 unknown);
error_code sys_net_abort(ppu_thread&, s32 type, u64 arg, s32 flags);
error_code sys_net_infoctl(ppu_thread&, s32 cmd, vm::ptr<void> arg);
error_code sys_net_control(ppu_thread&, u32 arg1, s32 arg2, vm::ptr<void> arg3, s32 arg4);
error_code sys_net_bnet_ioctl(ppu_thread&, s32 arg1, u32 arg2, u32 arg3);
error_code sys_net_bnet_sysctl(ppu_thread&, u32 arg1, u32 arg2, u32 arg3, vm::ptr<void> arg4, u32 arg5, u32 arg6);
error_code sys_net_eurus_post_command(ppu_thread&, s32 arg1, u32 arg2, u32 arg3);
