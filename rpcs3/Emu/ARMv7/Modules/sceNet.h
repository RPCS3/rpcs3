#pragma once

typedef u32 SceNetInAddr_t;
typedef u16 SceNetInPort_t;
typedef u8 SceNetSaFamily_t;
typedef u32 SceNetSocklen_t;

struct SceNetInAddr
{
	SceNetInAddr_t s_addr;
};

struct SceNetSockaddrIn
{
	u8 sin_len;
	SceNetSaFamily_t sin_family;
	SceNetInPort_t sin_port;
	SceNetInAddr sin_addr;
	SceNetInPort_t sin_vport;
	char sin_zero[6];
};

struct SceNetDnsInfo
{
	SceNetInAddr dns_addr[2];
};

struct SceNetSockaddr
{
	u8 sa_len;
	SceNetSaFamily_t sa_family;
	char sa_data[14];
};

struct SceNetEpollDataExt
{
	s32 id;
	u32 u32;
};

union SceNetEpollData
{
	vm::psv::ptr<void> ptr;
	s32 fd;
	u32 u32;
	u64 u64;
	SceNetEpollDataExt ext;
};

struct SceNetEpollSystemData
{
	u32 system[4];
};

struct SceNetEpollEvent
{
	u32 events;
	u32 reserved;
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
	vm::psv::ptr<void> memory;
	s32 size;
	s32 flags;
};

struct SceNetEmulationData
{
	u16 drop_rate;
	u16 drop_duration;
	u16 pass_duration;
	u16 delay_time;
	u16 delay_jitter;
	u16 order_rate;
	u16 order_delay_time;
	u16 duplication_rate;
	u32 bps_limit;
	u16 lower_size_limit;
	u16 upper_size_limit;
	u32 system_policy_pattern;
	u32 game_policy_pattern;
	u16 policy_flags[64];
	u8 reserved[64];
};

struct SceNetEmulationParam
{
	u16 version;
	u16 option_number;
	u16 current_version;
	u16 result;
	u32 flags;
	u32 reserved1;
	SceNetEmulationData send;
	SceNetEmulationData recv;
	u32 seed;
	u8 reserved[44];
};

typedef vm::psv::ptr<vm::psv::ptr<void>(u32 size, s32 rid, vm::psv::ptr<const char> name, vm::psv::ptr<void> user)> SceNetResolverFunctionAllocate;

typedef vm::psv::ptr<void(vm::psv::ptr<void> ptr, s32 rid, vm::psv::ptr<const char> name, vm::psv::ptr<void> user)> SceNetResolverFunctionFree;

struct SceNetResolverParam
{
	SceNetResolverFunctionAllocate allocate;
	SceNetResolverFunctionFree free;
	vm::psv::ptr<void> user;
};

struct SceNetLinger
{
	s32 l_onoff;
	s32 l_linger;
};

struct SceNetIovec
{
	vm::psv::ptr<void> iov_base;
	u32 iov_len;
};

struct SceNetMsghdr
{
	vm::psv::ptr<void> msg_name;
	SceNetSocklen_t msg_namelen;
	vm::psv::ptr<SceNetIovec> msg_iov;
	s32 msg_iovlen;
	vm::psv::ptr<void> msg_control;
	SceNetSocklen_t msg_controllen;
	s32 msg_flags;
};

struct SceNetSockInfo
{
	char name[32];
	s32 pid;
	s32 s;
	s8 socket_type;
	s8 policy;
	s16 reserved16;
	s32 recv_queue_length;
	s32 send_queue_length;
	SceNetInAddr local_adr;
	SceNetInAddr remote_adr;
	SceNetInPort_t local_port;
	SceNetInPort_t remote_port;
	SceNetInPort_t local_vport;
	SceNetInPort_t remote_vport;
	s32 state;
	s32 flags;
	s32 reserved[8];
};

struct SceNetStatisticsInfo
{
	s32 kernel_mem_free_size;
	s32 kernel_mem_free_min;
	s32 packet_count;
	s32 packet_qos_count;
	s32 libnet_mem_free_size;
	s32 libnet_mem_free_min;
};


extern psv_log_base sceNet;
