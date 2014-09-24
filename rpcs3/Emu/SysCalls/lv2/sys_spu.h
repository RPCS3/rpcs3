#pragma once

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

enum
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
	be_t<u32> nsize;
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

struct SpuGroupInfo
{
	std::vector<u32> list;
	std::atomic<u32> lock;
	std::string m_name;
	u32 m_id;
	s32 m_prio;
	s32 m_type;
	u32 m_ct;
	u32 m_count;
	s32 m_state;	//SPU Thread Group State.
	u32 m_exit_status;
	bool m_group_exit;

	SpuGroupInfo(const std::string& name, u32 num, s32 prio, s32 type, u32 ct)
		: m_name(name)
		, m_prio(prio)
		, m_type(type)
		, m_ct(ct)
		, lock(0)
		, m_count(num)
		, m_state(0)
		, m_exit_status(0)
		, m_group_exit(false)
	{
		m_state = SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED;	//Before all the nums done, it is not initialized.
		list.resize(256);
		for (auto& v : list) v = 0;
		m_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;	//Then Ready to Start. Cause Reference use New i can only place this here.
	}
};

class SPUThread;

// Aux
s32 spu_image_import(sys_spu_image& img, u32 src, u32 type);
SpuGroupInfo* spu_thread_group_create(const std::string& name, u32 num, s32 prio, s32 type, u32 container);
SPUThread* spu_thread_initialize(SpuGroupInfo* group, u32 spu_num, sys_spu_image& img, const std::string& name, u32 option, u64 a1, u64 a2, u64 a3, u64 a4, std::function<void(SPUThread&)> task = nullptr);

// SysCalls
s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
s32 sys_spu_image_open(vm::ptr<sys_spu_image> img, vm::ptr<const char> path);
s32 sys_spu_thread_initialize(vm::ptr<be_t<u32>> thread, u32 group, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg);
s32 sys_spu_thread_set_argument(u32 id, vm::ptr<sys_spu_thread_argument> arg);
s32 sys_spu_thread_group_destroy(u32 id);
s32 sys_spu_thread_group_start(u32 id);
s32 sys_spu_thread_group_suspend(u32 id);
s32 sys_spu_thread_group_resume(u32 id);
s32 sys_spu_thread_group_yield(u32 id);	
s32 sys_spu_thread_group_terminate(u32 id, int value);
s32 sys_spu_thread_group_create(vm::ptr<be_t<u32>> id, u32 num, int prio, vm::ptr<sys_spu_thread_group_attribute> attr);
s32 sys_spu_thread_create(vm::ptr<be_t<u32>> thread_id, vm::ptr<be_t<u32>> entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
s32 sys_spu_thread_group_join(u32 id, vm::ptr<be_t<u32>> cause, vm::ptr<be_t<u32>> status);
s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et);
s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et);
s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, vm::ptr<u8> spup);
s32 sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup);
s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
s32 sys_spu_thread_read_ls(u32 id, u32 address, vm::ptr<be_t<u64>> value, u32 type);
s32 sys_spu_thread_write_spu_mb(u32 id, u32 value);
s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value);
s32 sys_spu_thread_get_spu_cfg(u32 id, vm::ptr<be_t<u64>> value);
s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value);
s32 sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
s32 sys_spu_thread_disconnect_event(u32 id, u32 event_type, u8 spup);
s32 sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num);
s32 sys_spu_thread_unbind_queue(u32 id, u32 spuq_num);
s32 sys_spu_thread_get_exit_status(u32 id, vm::ptr<be_t<u32>> status);

s32 sys_raw_spu_create(vm::ptr<be_t<u32>> id, u32 attr_addr);
s32 sys_raw_spu_destroy(u32 id);
s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, vm::ptr<be_t<u32>> intrtag);
s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask);
s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, vm::ptr<be_t<u64>> mask);
s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat);
s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, vm::ptr<be_t<u64>> stat);
s32 sys_raw_spu_read_puint_mb(u32 id, vm::ptr<be_t<u32>> value);
s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value);
s32 sys_raw_spu_get_spu_cfg(u32 id, vm::ptr<be_t<u32>> value);
