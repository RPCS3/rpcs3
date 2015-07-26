#pragma once

typedef u32 SceNetSocklen_t;

struct SceNetInAddr
{
	le_t<u32> s_addr;
};

struct SceNetSockaddrIn
{
	u8 sin_len;
	u8 sin_family;
	le_t<u16> sin_port;
	SceNetInAddr sin_addr;
	le_t<u16> sin_vport;
	char sin_zero[6];
};

struct SceNetDnsInfo
{
	SceNetInAddr dns_addr[2];
};

struct SceNetSockaddr
{
	u8 sa_len;
	u8 sa_family;
	char sa_data[14];
};

struct SceNetEpollDataExt
{
	le_t<s32> id;
	le_t<u32> data;
};

union SceNetEpollData
{
	vm::lptr<void> ptr;
	le_t<s32> fd;
	le_t<u32> _u32;
	le_t<u64> _u64;
	SceNetEpollDataExt ext;
};

struct SceNetEpollSystemData
{
	le_t<u32> system[4];
};

struct SceNetEpollEvent
{
	le_t<u32> events;
	le_t<u32> reserved;
	SceNetEpollSystemData system;
	SceNetEpollData data;
};

struct SceNetEtherAddr
{
	u8 data[6];
};

typedef u32 SceNetIdMask;

struct SceNetFdSet
{
	SceNetIdMask bits[32];
};

struct SceNetIpMreq
{
	SceNetInAddr imr_multiaddr;
	SceNetInAddr imr_interface;
};

struct SceNetInitParam
{
	vm::lptr<void> memory;
	le_t<s32> size;
	le_t<s32> flags;
};

struct SceNetEmulationData
{
	le_t<u16> drop_rate;
	le_t<u16> drop_duration;
	le_t<u16> pass_duration;
	le_t<u16> delay_time;
	le_t<u16> delay_jitter;
	le_t<u16> order_rate;
	le_t<u16> order_delay_time;
	le_t<u16> duplication_rate;
	le_t<u32> bps_limit;
	le_t<u16> lower_size_limit;
	le_t<u16> upper_size_limit;
	le_t<u32> system_policy_pattern;
	le_t<u32> game_policy_pattern;
	le_t<u16> policy_flags[64];
	u8 reserved[64];
};

struct SceNetEmulationParam
{
	le_t<u16> version;
	le_t<u16> option_number;
	le_t<u16> current_version;
	le_t<u16> result;
	le_t<u32> flags;
	le_t<u32> reserved1;
	SceNetEmulationData send;
	SceNetEmulationData recv;
	le_t<u32> seed;
	u8 reserved[44];
};

using SceNetResolverFunctionAllocate = vm::ptr<void>(u32 size, s32 rid, vm::cptr<char> name, vm::ptr<void> user);
using SceNetResolverFunctionFree = void(vm::ptr<void> ptr, s32 rid, vm::cptr<char> name, vm::ptr<void> user);

struct SceNetResolverParam
{
	vm::lptr<SceNetResolverFunctionAllocate> allocate;
	vm::lptr<SceNetResolverFunctionFree> free;
	vm::lptr<void> user;
};

struct SceNetLinger
{
	le_t<s32> l_onoff;
	le_t<s32> l_linger;
};

struct SceNetIovec
{
	vm::lptr<void> iov_base;
	le_t<u32> iov_len;
};

struct SceNetMsghdr
{
	vm::lptr<void> msg_name;
	le_t<u32> msg_namelen;
	vm::lptr<SceNetIovec> msg_iov;
	le_t<s32> msg_iovlen;
	vm::lptr<void> msg_control;
	le_t<u32> msg_controllen;
	le_t<s32> msg_flags;
};

struct SceNetSockInfo
{
	char name[32];
	le_t<s32> pid;
	le_t<s32> s;
	s8 socket_type;
	s8 policy;
	le_t<s16> reserved16;
	le_t<s32> recv_queue_length;
	le_t<s32> send_queue_length;
	SceNetInAddr local_adr;
	SceNetInAddr remote_adr;
	le_t<u16> local_port;
	le_t<u16> remote_port;
	le_t<u16> local_vport;
	le_t<u16> remote_vport;
	le_t<s32> state;
	le_t<s32> flags;
	le_t<s32> reserved[8];
};

struct SceNetStatisticsInfo
{
	le_t<s32> kernel_mem_free_size;
	le_t<s32> kernel_mem_free_min;
	le_t<s32> packet_count;
	le_t<s32> packet_qos_count;
	le_t<s32> libnet_mem_free_size;
	le_t<s32> libnet_mem_free_min;
};

extern psv_log_base sceNet;
