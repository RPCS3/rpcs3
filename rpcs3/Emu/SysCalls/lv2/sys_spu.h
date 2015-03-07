#pragma once

struct event_queue_t;

enum : s32
{
	SYS_SPU_THREAD_GROUP_TYPE_NORMAL = 0x00,
	SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL = 0x01,
	SYS_SPU_THREAD_GROUP_TYPE_SYSTEM = 0x02,
	SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER = 0x04,
	SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT = 0x08,
	SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT = 0x18,
	SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM = 0x20,
};

enum
{
	SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT = 0x0001,
	SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT = 0x0002,
	SYS_SPU_THREAD_GROUP_JOIN_TERMINATED = 0x0004
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
	SPU_THREAD_GROUP_STATUS_UNKNOWN
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
	vm::bptr<const char> name;
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
	vm::bptr<const char> name;
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
	be_t<s32> size;

	union
	{
		be_t<u32> addr; // address or fill value
		u64 pad;
	};
};

static_assert(sizeof(sys_spu_segment) == 0x18, "Wrong sys_spu_segment size");

enum : u32
{
	SYS_SPU_IMAGE_TYPE_USER   = 0,
	SYS_SPU_IMAGE_TYPE_KERNEL = 1,
};

struct sys_spu_image
{
	be_t<u32> type; // user, kernel
	be_t<u32> entry_point;
	union
	{
		be_t<u32> addr; // temporarily used as offset of the whole LS image (should be removed)
		vm::bptr<sys_spu_segment> segs;
	};
	be_t<s32> nsegs;
};

enum : u32
{
	SYS_SPU_IMAGE_PROTECT = 0,
	SYS_SPU_IMAGE_DIRECT  = 1,
};

struct spu_arg_t
{
	u64 arg1;
	u64 arg2;
	u64 arg3;
	u64 arg4;
};

// SPU Thread Group Join State Flag
enum : u32
{
	SPU_TGJSF_IS_JOINING = (1 << 0),
	SPU_TGJSF_TERMINATED = (1 << 1), // set if SPU Thread Group is terminated by sys_spu_thread_group_terminate
	SPU_TGJSF_GROUP_EXIT = (1 << 2), // set if SPU Thread Group is terminated by sys_spu_thread_group_exit
};

struct spu_group_t
{
	const std::string name;
	const u32 num; // SPU Number
	const s32 type; // SPU Thread Group Type
	const u32 ct; // Memory Container Id

	std::array<std::shared_ptr<CPUThread>, 256> threads; // SPU Threads
	std::array<vm::ptr<sys_spu_image>, 256> images; // SPU Images
	std::array<spu_arg_t, 256> args; // SPU Thread Arguments

	s32 prio; // SPU Thread Group Priority
	u32 state; // SPU Thread Group State
	s32 exit_status; // SPU Thread Group Exit Status

	std::atomic<u32> join_state; // flags used to detect exit cause
	std::condition_variable join_cv; // used to signal waiting PPU thread

	std::weak_ptr<event_queue_t> ep_run; // port for SYS_SPU_THREAD_GROUP_EVENT_RUN events
	std::weak_ptr<event_queue_t> ep_exception; // TODO: SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION
	std::weak_ptr<event_queue_t> ep_sysmodule; // TODO: SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE

	spu_group_t(std::string name, u32 num, s32 prio, s32 type, u32 ct)
		: name(name)
		, num(num)
		, prio(prio)
		, type(type)
		, ct(ct)
		, state(SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
		, exit_status(0)
		, join_state(0)
	{
	}
};

class SPUThread;
struct vfsStream;

void LoadSpuImage(vfsStream& stream, u32& spu_ep, u32 addr);
u32 LoadSpuImage(vfsStream& stream, u32& spu_ep);

// Aux
s32 spu_image_import(sys_spu_image& img, u32 src, u32 type);
u32 spu_thread_group_create(const std::string& name, u32 num, s32 prio, s32 type, u32 container);
u32 spu_thread_initialize(u32 group, u32 spu_num, vm::ptr<sys_spu_image> img, const std::string& name, u32 option, u64 a1, u64 a2, u64 a3, u64 a4, std::function<void(SPUThread&)> task = nullptr);

// SysCalls
s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
s32 sys_spu_image_open(vm::ptr<sys_spu_image> img, vm::ptr<const char> path);
s32 sys_spu_thread_initialize(vm::ptr<u32> thread, u32 group, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg);
s32 sys_spu_thread_set_argument(u32 id, vm::ptr<sys_spu_thread_argument> arg);
s32 sys_spu_thread_group_create(vm::ptr<u32> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr);
s32 sys_spu_thread_group_destroy(u32 id);
s32 sys_spu_thread_group_start(u32 id);
s32 sys_spu_thread_group_suspend(u32 id);
s32 sys_spu_thread_group_resume(u32 id);
s32 sys_spu_thread_group_yield(u32 id);	
s32 sys_spu_thread_group_terminate(u32 id, s32 value);
s32 sys_spu_thread_group_join(u32 id, vm::ptr<u32> cause, vm::ptr<u32> status);
s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et);
s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et);
s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, vm::ptr<u8> spup);
s32 sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup);
s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
s32 sys_spu_thread_read_ls(u32 id, u32 address, vm::ptr<u64> value, u32 type);
s32 sys_spu_thread_write_spu_mb(u32 id, u32 value);
s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value);
s32 sys_spu_thread_get_spu_cfg(u32 id, vm::ptr<u64> value);
s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value);
s32 sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
s32 sys_spu_thread_disconnect_event(u32 id, u32 event_type, u8 spup);
s32 sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num);
s32 sys_spu_thread_unbind_queue(u32 id, u32 spuq_num);
s32 sys_spu_thread_get_exit_status(u32 id, vm::ptr<u32> status);

s32 sys_raw_spu_create(vm::ptr<u32> id, vm::ptr<void> attr);
s32 sys_raw_spu_destroy(u32 id);
s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag);
s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask);
s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, vm::ptr<u64> mask);
s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat);
s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, vm::ptr<u64> stat);
s32 sys_raw_spu_read_puint_mb(u32 id, vm::ptr<u32> value);
s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value);
s32 sys_raw_spu_get_spu_cfg(u32 id, vm::ptr<u32> value);

// SPU Calls
void sys_spu_thread_exit(SPUThread & spu, s32 status);
void sys_spu_thread_group_exit(SPUThread & spu, s32 status);
s32 sys_spu_thread_send_event(SPUThread & spu, u8 spup, u32 data0, u32 data1);
s32 sys_spu_thread_switch_system_module(SPUThread & spu, u32 status);
