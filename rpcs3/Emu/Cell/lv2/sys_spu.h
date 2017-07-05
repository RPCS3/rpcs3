#pragma once

#include "sys_event.h"

enum : s32
{
	SYS_SPU_THREAD_GROUP_TYPE_NORMAL                = 0x00,
	SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL            = 0x01,
	SYS_SPU_THREAD_GROUP_TYPE_SYSTEM                = 0x02,
	SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER = 0x04,
	SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT           = 0x08,
	SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT = 0x18,
	SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM = 0x20,
};

enum
{
	SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT       = 0x0001,
	SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT = 0x0002,
	SYS_SPU_THREAD_GROUP_JOIN_TERMINATED       = 0x0004
};

enum
{
	SYS_SPU_THREAD_GROUP_EVENT_RUN           = 1,
	SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION     = 2,
	SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE = 4,
};

enum : u64
{
	SYS_SPU_THREAD_GROUP_EVENT_RUN_KEY           = 0xFFFFFFFF53505500ull,
	SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION_KEY     = 0xFFFFFFFF53505503ull,
	SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE_KEY = 0xFFFFFFFF53505504ull,
};

enum : u32
{
	SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED,
	SPU_THREAD_GROUP_STATUS_INITIALIZED,
	SPU_THREAD_GROUP_STATUS_READY,
	SPU_THREAD_GROUP_STATUS_WAITING,
	SPU_THREAD_GROUP_STATUS_SUSPENDED,
	SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED,
	SPU_THREAD_GROUP_STATUS_RUNNING,
	SPU_THREAD_GROUP_STATUS_STOPPED,
	SPU_THREAD_GROUP_STATUS_UNKNOWN,
};

enum : s32
{
	SYS_SPU_SEGMENT_TYPE_COPY = 1,
	SYS_SPU_SEGMENT_TYPE_FILL = 2,
	SYS_SPU_SEGMENT_TYPE_INFO = 4,
};

struct sys_spu_thread_group_attribute
{
	be_t<u32> nsize; // name length including NULL terminator
	vm::ps3::bcptr<char> name;
	be_t<s32> type;
	be_t<u32> ct; // memory container id
};

enum : u32
{
	SYS_SPU_THREAD_OPTION_NONE               = 0,
	SYS_SPU_THREAD_OPTION_ASYNC_INTR_ENABLE  = 1,
	SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE = 2,
};

struct sys_spu_thread_attribute
{
	vm::ps3::bcptr<char> name;
	be_t<u32> name_len;
	be_t<u32> option;
};

struct sys_spu_thread_argument
{
	be_t<u64> arg1;
	be_t<u64> arg2;
	be_t<u64> arg3;
	be_t<u64> arg4;
};

struct sys_spu_segment
{
	be_t<s32> type; // copy, fill, info
	be_t<u32> ls; // local storage address
	be_t<u32> size;

	union
	{
		be_t<u32> addr; // address or fill value
		u64 pad;
	};
};

CHECK_SIZE(sys_spu_segment, 0x18);

enum : u32
{
	SYS_SPU_IMAGE_TYPE_USER   = 0,
	SYS_SPU_IMAGE_TYPE_KERNEL = 1,
};

struct sys_spu_image
{
	be_t<u32> type; // user, kernel
	be_t<u32> entry_point;
	vm::ps3::bptr<sys_spu_segment> segs;
	be_t<s32> nsegs;

	void load(const fs::file& stream);
	void free();
	void deploy(u32 loc);
};

enum : u32
{
	SYS_SPU_IMAGE_PROTECT = 0,
	SYS_SPU_IMAGE_DIRECT  = 1,
};

// SPU Thread Group Join State Flag
enum : u32
{
	SPU_TGJSF_IS_JOINING = (1 << 0),
	SPU_TGJSF_TERMINATED = (1 << 1), // set if SPU Thread Group is terminated by sys_spu_thread_group_terminate
	SPU_TGJSF_GROUP_EXIT = (1 << 2), // set if SPU Thread Group is terminated by sys_spu_thread_group_exit
};

class SPUThread;

struct lv2_spu_group
{
	static const u32 id_base = 1; // Wrong?
	static const u32 id_step = 1;
	static const u32 id_count = 255;

	const std::string name;
	const u32 num; // SPU Number
	const s32 type; // SPU Thread Group Type
	const u32 ct; // Memory Container Id

	semaphore<> mutex;
	atomic_t<u32> init; // Initialization Counter
	atomic_t<s32> prio; // SPU Thread Group Priority
	atomic_t<u32> run_state; // SPU Thread Group State
	atomic_t<s32> exit_status; // SPU Thread Group Exit Status
	atomic_t<u32> join_state; // flags used to detect exit cause
	cond_variable cv; // used to signal waiting PPU thread

	std::array<std::shared_ptr<SPUThread>, 256> threads; // SPU Threads
	std::array<sys_spu_image, 256> imgs; // SPU Images
	std::array<std::array<u64, 4>, 256> args; // SPU Thread Arguments

	std::weak_ptr<lv2_event_queue> ep_run; // port for SYS_SPU_THREAD_GROUP_EVENT_RUN events
	std::weak_ptr<lv2_event_queue> ep_exception; // TODO: SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION
	std::weak_ptr<lv2_event_queue> ep_sysmodule; // TODO: SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE

	lv2_spu_group(std::string name, u32 num, s32 prio, s32 type, u32 ct)
		: name(name)
		, num(num)
		, init(0)
		, prio(prio)
		, type(type)
		, ct(ct)
		, run_state(SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
		, exit_status(0)
		, join_state(0)
	{
	}

	void send_run_event(u64 data1, u64 data2, u64 data3)
	{
		if (const auto queue = ep_run.lock())
		{
			queue->send(SYS_SPU_THREAD_GROUP_EVENT_RUN_KEY, data1, data2, data3);
		}
	}

	void send_exception_event(u64 data1, u64 data2, u64 data3)
	{
		if (const auto queue = ep_exception.lock())
		{
			queue->send(SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION_KEY, data1, data2, data3);
		}
	}

	void send_sysmodule_event(u64 data1, u64 data2, u64 data3)
	{
		if (const auto queue = ep_sysmodule.lock())
		{
			queue->send(SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE_KEY, data1, data2, data3);
		}
	}
};

class ppu_thread;

// Syscalls

error_code sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
error_code sys_spu_image_open(vm::ps3::ptr<sys_spu_image> img, vm::ps3::cptr<char> path);
error_code sys_spu_thread_initialize(vm::ps3::ptr<u32> thread, u32 group, u32 spu_num, vm::ps3::ptr<sys_spu_image>, vm::ps3::ptr<sys_spu_thread_attribute>, vm::ps3::ptr<sys_spu_thread_argument>);
error_code sys_spu_thread_set_argument(u32 id, vm::ps3::ptr<sys_spu_thread_argument> arg);
error_code sys_spu_thread_group_create(vm::ps3::ptr<u32> id, u32 num, s32 prio, vm::ps3::ptr<sys_spu_thread_group_attribute> attr);
error_code sys_spu_thread_group_destroy(u32 id);
error_code sys_spu_thread_group_start(ppu_thread&, u32 id);
error_code sys_spu_thread_group_suspend(u32 id);
error_code sys_spu_thread_group_resume(u32 id);
error_code sys_spu_thread_group_yield(u32 id);	
error_code sys_spu_thread_group_terminate(u32 id, s32 value);
error_code sys_spu_thread_group_join(ppu_thread&, u32 id, vm::ps3::ptr<u32> cause, vm::ps3::ptr<u32> status);
error_code sys_spu_thread_group_set_priority(u32 id, s32 priority);
error_code sys_spu_thread_group_get_priority(u32 id, vm::ps3::ptr<s32> priority);
error_code sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et);
error_code sys_spu_thread_group_disconnect_event(u32 id, u32 et);
error_code sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, vm::ps3::ptr<u8> spup);
error_code sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup);
error_code sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
error_code sys_spu_thread_read_ls(u32 id, u32 address, vm::ps3::ptr<u64> value, u32 type);
error_code sys_spu_thread_write_spu_mb(u32 id, u32 value);
error_code sys_spu_thread_set_spu_cfg(u32 id, u64 value);
error_code sys_spu_thread_get_spu_cfg(u32 id, vm::ps3::ptr<u64> value);
error_code sys_spu_thread_write_snr(u32 id, u32 number, u32 value);
error_code sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
error_code sys_spu_thread_disconnect_event(u32 id, u32 event_type, u8 spup);
error_code sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num);
error_code sys_spu_thread_unbind_queue(u32 id, u32 spuq_num);
error_code sys_spu_thread_get_exit_status(u32 id, vm::ps3::ptr<u32> status);

error_code sys_raw_spu_create(vm::ps3::ptr<u32> id, vm::ps3::ptr<void> attr);
error_code sys_raw_spu_destroy(ppu_thread& ppu, u32 id);
error_code sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, vm::ps3::ptr<u32> intrtag);
error_code sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask);
error_code sys_raw_spu_get_int_mask(u32 id, u32 class_id, vm::ps3::ptr<u64> mask);
error_code sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat);
error_code sys_raw_spu_get_int_stat(u32 id, u32 class_id, vm::ps3::ptr<u64> stat);
error_code sys_raw_spu_read_puint_mb(u32 id, vm::ps3::ptr<u32> value);
error_code sys_raw_spu_set_spu_cfg(u32 id, u32 value);
error_code sys_raw_spu_get_spu_cfg(u32 id, vm::ps3::ptr<u32> value);
