#pragma once

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep);

enum
{
	SYS_SPU_THREAD_GROUP_TYPE_NORMAL = 0x00,
	SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL = 0x01,
	SYS_SPU_THREAD_GROUP_TYPE_SYSTEM = 0x02,
	SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER = 0x04,
	SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT = 0x08,
	SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT = 0x18,
	SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM = 0x20
};

enum
{
	SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT = 0x0001,
	SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT = 0x0002,
	SYS_SPU_THREAD_GROUP_JOIN_TERMINATED = 0x0004
};

enum {
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

enum
{
	SYS_SPU_SEGMENT_TYPE_COPY = 0x0001,
	SYS_SPU_SEGMENT_TYPE_FILL = 0x0002,
	SYS_SPU_SEGMENT_TYPE_INFO = 0x0004,
};

struct sys_spu_thread_group_attribute
{
	be_t<u32> name_len;
	be_t<u32> name_addr;
	be_t<int> type;
	/* struct {} option; */
	be_t<u32> ct; // memory container id
};

struct sys_spu_thread_attribute
{
	be_t<u32> name_addr;
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

struct sys_spu_image
{
	be_t<u32> type;
	be_t<u32> entry_point; 
	be_t<u32> segs_addr; //temporarily used as offset of LS image after elf loading
	be_t<int> nsegs;
};

struct sys_spu_segment
{
	be_t<int> type;
	be_t<u32> ls_start;
	be_t<int> size;
	be_t<u64> src;
};

struct SpuGroupInfo
{
	std::vector<u32> list;
	std::atomic<u32> lock;
	std::string m_name;
	int m_prio;
	int m_type;
	int m_ct;
	u32 m_count;
	int m_state;	//SPU Thread Group State.
	int m_exit_status;

	SpuGroupInfo(const std::string& name, u32 num, int prio, int type, u32 ct) 
		: m_name(name)
		, m_prio(prio)
		, m_type(type)
		, m_ct(ct)
		, lock(0)
		, m_count(num)
	{
		m_state = SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED;	//Before all the nums done, it is not initialized.
		list.resize(256);
		for (auto& v : list) v = 0;
		m_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;	//Then Ready to Start. Cause Reference use New i can only place this here.
		m_exit_status = 0;
	}
};

// SysCalls
s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
s32 sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr);
s32 sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg);
s32 sys_spu_thread_set_argument(u32 id, mem_ptr_t<sys_spu_thread_argument> arg);
s32 sys_spu_thread_group_destroy(u32 id);
s32 sys_spu_thread_group_start(u32 id);
s32 sys_spu_thread_group_suspend(u32 id);
s32 sys_spu_thread_group_resume(u32 id);
s32 sys_spu_thread_group_yield(u32 id);	
s32 sys_spu_thread_group_terminate(u32 id, int value);
s32 sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr);
s32 sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
s32 sys_spu_thread_group_join(u32 id, mem32_t cause, mem32_t status);
s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et);
s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et);
s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, mem8_t spup);
s32 sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup);
s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
s32 sys_spu_thread_read_ls(u32 id, u32 address, mem64_t value, u32 type);
s32 sys_spu_thread_write_spu_mb(u32 id, u32 value);
s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value);
s32 sys_spu_thread_get_spu_cfg(u32 id, mem64_t value);
s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value);
s32 sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
s32 sys_spu_thread_disconnect_event(u32 id, u32 event_type, u8 spup);
s32 sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num);
s32 sys_spu_thread_unbind_queue(u32 id, u32 spuq_num);
s32 sys_spu_thread_get_exit_status(u32 id, mem32_t status);

s32 sys_raw_spu_create(mem32_t id, u32 attr_addr);
s32 sys_raw_spu_destroy(u32 id);
s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, mem32_t intrtag);
s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask);
s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, mem64_t mask);
s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat);
s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, mem64_t stat);
s32 sys_raw_spu_read_puint_mb(u32 id, mem32_t value);
s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value);
s32 sys_raw_spu_get_spu_cfg(u32 id, mem32_t value);
