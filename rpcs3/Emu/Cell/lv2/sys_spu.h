#pragma once

#include "sys_sync.h"
#include "sys_event.h"
#include "Emu/Cell/SPUThread.h"

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/File.h"

struct lv2_memory_container;

enum : s32
{
	SYS_SPU_THREAD_GROUP_TYPE_NORMAL                = 0x00,
	//SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL            = 0x01, doesn't exist
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

enum
{
	SYS_SPU_THREAD_GROUP_LOG_ON         = 0x0,
	SYS_SPU_THREAD_GROUP_LOG_OFF        = 0x1,
	SYS_SPU_THREAD_GROUP_LOG_GET_STATUS = 0x2,
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
	vm::bcptr<char> name;
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
	vm::bcptr<char> name;
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
	be_t<u32> entry_point; // Note: in kernel mode it's used to store id
	vm::bptr<sys_spu_segment> segs;
	be_t<s32> nsegs;

	template <bool CountInfo = true, typename Phdrs>
	static s32 get_nsegs(const Phdrs& phdrs)
	{
		s32 num_segs = 0;

		for (const auto& phdr : phdrs)
		{
			if (phdr.p_type != 1 && phdr.p_type != 4)
			{
				return -1;
			}

			if (phdr.p_type == 1 && phdr.p_filesz != phdr.p_memsz && phdr.p_filesz)
			{
				num_segs += 2;
			}
			else if (phdr.p_type == 1 || CountInfo)
			{
				num_segs += 1;
			}
		}

		return num_segs;
	}

	template <bool WriteInfo = true, typename Phdrs>
	static s32 fill(vm::ptr<sys_spu_segment> segs, s32 nsegs, const Phdrs& phdrs, u32 src)
	{
		s32 num_segs = 0;

		for (const auto& phdr : phdrs)
		{
			if (phdr.p_type == 1)
			{
				if (phdr.p_filesz)
				{
					if (num_segs >= nsegs)
					{
						return -2;
					}

					auto* seg = &segs[num_segs++];
					seg->type = SYS_SPU_SEGMENT_TYPE_COPY;
					seg->ls   = static_cast<u32>(phdr.p_vaddr);
					seg->size = static_cast<u32>(phdr.p_filesz);
					seg->addr = static_cast<u32>(phdr.p_offset + src);
				}

				if (phdr.p_memsz > phdr.p_filesz)
				{
					if (num_segs >= nsegs)
					{
						return -2;
					}

					auto* seg = &segs[num_segs++];
					seg->type = SYS_SPU_SEGMENT_TYPE_FILL;
					seg->ls   = static_cast<u32>(phdr.p_vaddr + phdr.p_filesz);
					seg->size = static_cast<u32>(phdr.p_memsz - phdr.p_filesz);
					seg->addr = 0;
				}
			}
			else if (WriteInfo && phdr.p_type == 4)
			{
				if (num_segs >= nsegs)
				{
					return -2;
				}

				auto* seg = &segs[num_segs++];
				seg->type = SYS_SPU_SEGMENT_TYPE_INFO;
				seg->size = 0x20;
				seg->addr = static_cast<u32>(phdr.p_offset + 0x14 + src);
			}
			else if (phdr.p_type != 4)
			{
				return -1;
			}
		}

		return num_segs;
	}

	void load(const fs::file& stream);
	void free();
	static void deploy(u32 loc, sys_spu_segment* segs, u32 nsegs);
};

enum : u32
{
	SYS_SPU_IMAGE_PROTECT = 0,
	SYS_SPU_IMAGE_DIRECT  = 1,
};

struct lv2_spu_image : lv2_obj
{
	static const u32 id_base = 0x22000000;

	const u32 e_entry;

	lv2_spu_image(u32 entry)
		: e_entry(entry)
	{
	}
};

struct lv2_spu_group
{
	static const u32 id_base = 0x04000100;
	static const u32 id_step = 0x100;
	static const u32 id_count = 255;

	const std::string name;
	const u32 id;
	const u32 max_num;
	const u32 mem_size;
	const s32 type; // SPU Thread Group Type
	lv2_memory_container* const ct; // Memory Container
	const bool has_scheduler_context;
	u32 max_run;

	shared_mutex mutex;

	atomic_t<u32> init; // Initialization Counter
	atomic_t<s32> prio; // SPU Thread Group Priority
	atomic_t<u32> run_state; // SPU Thread Group State
	atomic_t<s32> exit_status; // SPU Thread Group Exit Status
	atomic_t<u32> join_state; // flags used to detect exit cause and signal
	atomic_t<u32> running; // Number of running threads
	cond_variable cond; // used to signal waiting PPU thread
	atomic_t<u64> stop_count;
	class ppu_thread* waiter = nullptr;

	std::array<std::shared_ptr<named_thread<spu_thread>>, 8> threads; // SPU Threads
	std::array<s8, 256> threads_map; // SPU Threads map based number
	std::array<std::pair<sys_spu_image, std::vector<sys_spu_segment>>, 8> imgs; // SPU Images
	std::array<std::array<u64, 4>, 8> args; // SPU Thread Arguments

	std::weak_ptr<lv2_event_queue> ep_run; // port for SYS_SPU_THREAD_GROUP_EVENT_RUN events
	std::weak_ptr<lv2_event_queue> ep_exception; // TODO: SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION
	std::weak_ptr<lv2_event_queue> ep_sysmodule; // TODO: SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE

	lv2_spu_group(std::string name, u32 num, s32 prio, s32 type, lv2_memory_container* ct, bool uses_scheduler, u32 mem_size)
		: id(idm::last_id())
		, name(name)
		, max_num(num)
		, max_run(num)
		, mem_size(mem_size)
		, init(0)
		, prio(prio)
		, type(type)
		, ct(ct)
		, has_scheduler_context(uses_scheduler)
		, run_state(SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
		, exit_status(0)
		, join_state(0)
		, running(0)
		, stop_count(0)
	{
		threads_map.fill(-1);
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

	static std::pair<named_thread<spu_thread>*, std::shared_ptr<lv2_spu_group>> get_thread(u32 id);
};

class ppu_thread;

// Syscalls

error_code sys_spu_initialize(ppu_thread&, u32 max_usable_spu, u32 max_raw_spu);
error_code _sys_spu_image_get_information(ppu_thread&, vm::ptr<sys_spu_image> img, vm::ptr<u32> entry_point, vm::ptr<s32> nsegs);
error_code sys_spu_image_open(ppu_thread&, vm::ptr<sys_spu_image> img, vm::cptr<char> path);
error_code _sys_spu_image_import(ppu_thread&, vm::ptr<sys_spu_image> img, u32 src, u32 size, u32 arg4);
error_code _sys_spu_image_close(ppu_thread&, vm::ptr<sys_spu_image> img);
error_code _sys_spu_image_get_segments(ppu_thread&, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_segment> segments, s32 nseg);
error_code sys_spu_thread_initialize(ppu_thread&, vm::ptr<u32> thread, u32 group, u32 spu_num, vm::ptr<sys_spu_image>, vm::ptr<sys_spu_thread_attribute>, vm::ptr<sys_spu_thread_argument>);
error_code sys_spu_thread_set_argument(ppu_thread&, u32 id, vm::ptr<sys_spu_thread_argument> arg);
error_code sys_spu_thread_group_create(ppu_thread&, vm::ptr<u32> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr);
error_code sys_spu_thread_group_destroy(ppu_thread&, u32 id);
error_code sys_spu_thread_group_start(ppu_thread&, u32 id);
error_code sys_spu_thread_group_suspend(ppu_thread&, u32 id);
error_code sys_spu_thread_group_resume(ppu_thread&, u32 id);
error_code sys_spu_thread_group_yield(ppu_thread&, u32 id);
error_code sys_spu_thread_group_terminate(ppu_thread&, u32 id, s32 value);
error_code sys_spu_thread_group_join(ppu_thread&, u32 id, vm::ptr<u32> cause, vm::ptr<u32> status);
error_code sys_spu_thread_group_set_priority(ppu_thread&, u32 id, s32 priority);
error_code sys_spu_thread_group_get_priority(ppu_thread&, u32 id, vm::ptr<s32> priority);
error_code sys_spu_thread_group_connect_event(ppu_thread&, u32 id, u32 eq, u32 et);
error_code sys_spu_thread_group_disconnect_event(ppu_thread&, u32 id, u32 et);
error_code sys_spu_thread_group_connect_event_all_threads(ppu_thread&, u32 id, u32 eq_id, u64 req, vm::ptr<u8> spup);
error_code sys_spu_thread_group_disconnect_event_all_threads(ppu_thread&, u32 id, u8 spup);
error_code sys_spu_thread_group_log(ppu_thread&, s32 command, vm::ptr<s32> stat);
error_code sys_spu_thread_write_ls(ppu_thread&, u32 id, u32 address, u64 value, u32 type);
error_code sys_spu_thread_read_ls(ppu_thread&, u32 id, u32 address, vm::ptr<u64> value, u32 type);
error_code sys_spu_thread_write_spu_mb(ppu_thread&, u32 id, u32 value);
error_code sys_spu_thread_set_spu_cfg(ppu_thread&, u32 id, u64 value);
error_code sys_spu_thread_get_spu_cfg(ppu_thread&, u32 id, vm::ptr<u64> value);
error_code sys_spu_thread_write_snr(ppu_thread&, u32 id, u32 number, u32 value);
error_code sys_spu_thread_connect_event(ppu_thread&, u32 id, u32 eq, u32 et, u8 spup);
error_code sys_spu_thread_disconnect_event(ppu_thread&, u32 id, u32 event_type, u8 spup);
error_code sys_spu_thread_bind_queue(ppu_thread&, u32 id, u32 spuq, u32 spuq_num);
error_code sys_spu_thread_unbind_queue(ppu_thread&, u32 id, u32 spuq_num);
error_code sys_spu_thread_get_exit_status(ppu_thread&, u32 id, vm::ptr<u32> status);
error_code sys_spu_thread_recover_page_fault(ppu_thread&, u32 id);

error_code sys_raw_spu_create(ppu_thread&, vm::ptr<u32> id, vm::ptr<void> attr);
error_code sys_raw_spu_destroy(ppu_thread& ppu, u32 id);
error_code sys_raw_spu_create_interrupt_tag(ppu_thread&, u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag);
error_code sys_raw_spu_set_int_mask(ppu_thread&, u32 id, u32 class_id, u64 mask);
error_code sys_raw_spu_get_int_mask(ppu_thread&, u32 id, u32 class_id, vm::ptr<u64> mask);
error_code sys_raw_spu_set_int_stat(ppu_thread&, u32 id, u32 class_id, u64 stat);
error_code sys_raw_spu_get_int_stat(ppu_thread&, u32 id, u32 class_id, vm::ptr<u64> stat);
error_code sys_raw_spu_read_puint_mb(ppu_thread&, u32 id, vm::ptr<u32> value);
error_code sys_raw_spu_set_spu_cfg(ppu_thread&, u32 id, u32 value);
error_code sys_raw_spu_get_spu_cfg(ppu_thread&, u32 id, vm::ptr<u32> value);
error_code sys_raw_spu_recover_page_fault(ppu_thread&, u32 id);
