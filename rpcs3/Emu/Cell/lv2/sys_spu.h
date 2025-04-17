#pragma once

#include "sys_sync.h"
#include "sys_event.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/ErrorCodes.h"

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/File.h"

#include <span>

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

enum spu_group_status : u32
{
	SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED,
	SPU_THREAD_GROUP_STATUS_INITIALIZED,
	SPU_THREAD_GROUP_STATUS_READY,
	SPU_THREAD_GROUP_STATUS_WAITING,
	SPU_THREAD_GROUP_STATUS_SUSPENDED,
	SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED,
	SPU_THREAD_GROUP_STATUS_RUNNING,
	SPU_THREAD_GROUP_STATUS_STOPPED,
	SPU_THREAD_GROUP_STATUS_DESTROYED, // Internal state
	SPU_THREAD_GROUP_STATUS_UNKNOWN,
};

enum : s32
{
	SYS_SPU_SEGMENT_TYPE_COPY = 1,
	SYS_SPU_SEGMENT_TYPE_FILL = 2,
	SYS_SPU_SEGMENT_TYPE_INFO = 4,
};

enum spu_stop_syscall : u32
{
	SYS_SPU_THREAD_STOP_YIELD                = 0x0100,
	SYS_SPU_THREAD_STOP_GROUP_EXIT           = 0x0101,
	SYS_SPU_THREAD_STOP_THREAD_EXIT          = 0x0102,
	SYS_SPU_THREAD_STOP_RECEIVE_EVENT        = 0x0110,
	SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT    = 0x0111,
	SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE = 0x0120,
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
	ENABLE_BITWISE_SERIALIZATION;

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
			if (phdr.p_type != 1u && phdr.p_type != 4u)
			{
				return -1;
			}

			if (phdr.p_type == 1u && phdr.p_filesz != phdr.p_memsz && phdr.p_filesz)
			{
				num_segs += 2;
			}
			else if (phdr.p_type == 1u || CountInfo)
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
			if (phdr.p_type == 1u)
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
			else if (WriteInfo && phdr.p_type == 4u)
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
			else if (phdr.p_type != 4u)
			{
				return -1;
			}
		}

		return num_segs;
	}

	bool load(const fs::file& stream);
	void free() const;
	static void deploy(u8* loc, std::span<const sys_spu_segment> segs, bool is_verbose = true);
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
	const vm::ptr<sys_spu_segment> segs;
	const s32 nsegs;

	lv2_spu_image(u32 entry, vm::ptr<sys_spu_segment> segs, s32 nsegs)
		: e_entry(entry)
		, segs(segs)
		, nsegs(nsegs)
	{
	}

	lv2_spu_image(utils::serial& ar);
	void save(utils::serial& ar);
};

struct sys_spu_thread_group_syscall_253_info
{
	be_t<u32> deadlineMeetCounter; // From cellSpursGetInfo
	be_t<u32> deadlineMissCounter; // Same
	be_t<u64> timestamp;
	be_t<u64> _x10[6];
};

struct lv2_spu_group
{
	static const u32 id_base = 0x04000100;
	static const u32 id_step = 0x100;
	static const u32 id_count = 255;
	static constexpr std::pair<u32, u32> id_invl_range = {0, 8};

	static_assert(spu_thread::id_count == id_count * 6 + 5);

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
	atomic_t<typename spu_thread::spu_prio_t> prio{}; // SPU Thread Group Priority
	atomic_t<spu_group_status> run_state; // SPU Thread Group State
	atomic_t<s32> exit_status; // SPU Thread Group Exit Status
	atomic_t<u32> join_state; // flags used to detect exit cause and signal
	atomic_t<u32> running = 0; // Number of running threads
	atomic_t<u32> spurs_running = 0;
	atomic_t<u32> stop_count = 0;
	atomic_t<u32> wait_term_count = 0; 
	u32 waiter_spu_index = -1; // Index of SPU executing a waiting syscall
	class ppu_thread* waiter = nullptr;
	bool set_terminate = false;

	std::array<shared_ptr<named_thread<spu_thread>>, 8> threads; // SPU Threads
	std::array<s8, 256> threads_map; // SPU Threads map based number
	std::array<std::pair<u32, std::vector<sys_spu_segment>>, 8> imgs; // Entry points, SPU image segments
	std::array<std::array<u64, 4>, 8> args; // SPU Thread Arguments

	shared_ptr<lv2_event_queue> ep_run; // port for SYS_SPU_THREAD_GROUP_EVENT_RUN events
	shared_ptr<lv2_event_queue> ep_exception; // TODO: SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION
	shared_ptr<lv2_event_queue> ep_sysmodule; // TODO: SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE

	lv2_spu_group(std::string name, u32 num, s32 _prio, s32 type, lv2_memory_container* ct, bool uses_scheduler, u32 mem_size) noexcept
		: name(std::move(name))
		, id(idm::last_id())
		, max_num(num)
		, mem_size(mem_size)
		, type(type)
		, ct(ct)
		, has_scheduler_context(uses_scheduler)
		, max_run(num)
		, init(0)
		, run_state(SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
		, exit_status(0)
		, join_state(0)
		, args({})
	{
		threads_map.fill(-1);
		prio.raw().prio = _prio;
	}

	SAVESTATE_INIT_POS(8); // Dependency on SPUs

	lv2_spu_group(utils::serial& ar) noexcept;
	void save(utils::serial& ar);

	CellError send_run_event(u64 data1, u64 data2, u64 data3) const
	{
		return ep_run ? ep_run->send(SYS_SPU_THREAD_GROUP_EVENT_RUN_KEY, data1, data2, data3) : CELL_ENOTCONN;
	}

	CellError send_exception_event(u64 data1, u64 data2, u64 data3) const
	{
		return ep_exception ? ep_exception->send(SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION_KEY, data1, data2, data3) : CELL_ENOTCONN;
	}

	CellError send_sysmodule_event(u64 data1, u64 data2, u64 data3) const
	{
		return ep_sysmodule ? ep_sysmodule->send(SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE_KEY, data1, data2, data3) : CELL_ENOTCONN;
	}

	static std::pair<named_thread<spu_thread>*, shared_ptr<lv2_spu_group>> get_thread(u32 id);
};

class ppu_thread;

// Syscalls

error_code sys_spu_initialize(ppu_thread&, u32 max_usable_spu, u32 max_raw_spu);
error_code _sys_spu_image_get_information(ppu_thread&, vm::ptr<sys_spu_image> img, vm::ptr<u32> entry_point, vm::ptr<s32> nsegs);
error_code sys_spu_image_open(ppu_thread&, vm::ptr<sys_spu_image> img, vm::cptr<char> path);
error_code sys_spu_image_open_by_fd(ppu_thread&, vm::ptr<sys_spu_image> img, s32 fd, s64 offset);
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
error_code sys_spu_thread_group_disconnect_event_all_threads(ppu_thread&, u32 id, u32 spup);
error_code sys_spu_thread_group_set_cooperative_victims(ppu_thread&, u32 id, u32 threads_mask);
error_code sys_spu_thread_group_syscall_253(ppu_thread& ppu, u32 id, vm::ptr<sys_spu_thread_group_syscall_253_info> info);
error_code sys_spu_thread_group_log(ppu_thread&, s32 command, vm::ptr<s32> stat);
error_code sys_spu_thread_write_ls(ppu_thread&, u32 id, u32 lsa, u64 value, u32 type);
error_code sys_spu_thread_read_ls(ppu_thread&, u32 id, u32 lsa, vm::ptr<u64> value, u32 type);
error_code sys_spu_thread_write_spu_mb(ppu_thread&, u32 id, u32 value);
error_code sys_spu_thread_set_spu_cfg(ppu_thread&, u32 id, u64 value);
error_code sys_spu_thread_get_spu_cfg(ppu_thread&, u32 id, vm::ptr<u64> value);
error_code sys_spu_thread_write_snr(ppu_thread&, u32 id, u32 number, u32 value);
error_code sys_spu_thread_connect_event(ppu_thread&, u32 id, u32 eq, u32 et, u32 spup);
error_code sys_spu_thread_disconnect_event(ppu_thread&, u32 id, u32 et, u32 spup);
error_code sys_spu_thread_bind_queue(ppu_thread&, u32 id, u32 spuq, u32 spuq_num);
error_code sys_spu_thread_unbind_queue(ppu_thread&, u32 id, u32 spuq_num);
error_code sys_spu_thread_get_exit_status(ppu_thread&, u32 id, vm::ptr<s32> status);
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

error_code sys_isolated_spu_create(ppu_thread&, vm::ptr<u32> id, vm::ptr<void> image, u64 arg1, u64 arg2, u64 arg3, u64 arg4);
error_code sys_isolated_spu_start(ppu_thread&, u32 id);
error_code sys_isolated_spu_destroy(ppu_thread& ppu, u32 id);
error_code sys_isolated_spu_create_interrupt_tag(ppu_thread&, u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag);
error_code sys_isolated_spu_set_int_mask(ppu_thread&, u32 id, u32 class_id, u64 mask);
error_code sys_isolated_spu_get_int_mask(ppu_thread&, u32 id, u32 class_id, vm::ptr<u64> mask);
error_code sys_isolated_spu_set_int_stat(ppu_thread&, u32 id, u32 class_id, u64 stat);
error_code sys_isolated_spu_get_int_stat(ppu_thread&, u32 id, u32 class_id, vm::ptr<u64> stat);
error_code sys_isolated_spu_read_puint_mb(ppu_thread&, u32 id, vm::ptr<u32> value);
error_code sys_isolated_spu_set_spu_cfg(ppu_thread&, u32 id, u32 value);
error_code sys_isolated_spu_get_spu_cfg(ppu_thread&, u32 id, vm::ptr<u32> value);
