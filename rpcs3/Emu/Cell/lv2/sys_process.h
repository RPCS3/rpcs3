#pragma once

#include "Crypto/unself.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUAnalyser.h"

#include "sys_sync.h"

// Process Local Object Type
enum : u32
{
	SYS_SPU_THREAD_GROUP_OBJECT      = 0x04,
	SYS_PROCESS_OBJECT               = 0x05,
	SYS_CONFIG_OBJECT                = 0x41,
	SYS_MEM_OBJECT                   = 0x08,
	SYS_MUTEX_OBJECT                 = 0x85,
	SYS_COND_OBJECT                  = 0x86,
	SYS_RWLOCK_OBJECT                = 0x88,
	SYS_INTR_TAG_OBJECT              = 0x0A,
	SYS_INTR_SERVICE_HANDLE_OBJECT   = 0x0B,
	SYS_EVENT_QUEUE_OBJECT           = 0x8D,
	SYS_EVENT_PORT_OBJECT            = 0x0E,
	SYS_TRACE_OBJECT                 = 0x21,
	SYS_SPUIMAGE_OBJECT              = 0x22,
	SYS_PRX_OBJECT                   = 0x23,
	SYS_SPUPORT_OBJECT               = 0x24,
	SYS_OVERLAY_OBJECT               = 0x25,
	SYS_LWMUTEX_OBJECT               = 0x95,
	SYS_TIMER_OBJECT                 = 0x11,
	SYS_SEMAPHORE_OBJECT             = 0x96,
	SYS_FS_FD_OBJECT                 = 0x73,
	SYS_LWCOND_OBJECT                = 0x97,
	SYS_EVENT_FLAG_OBJECT            = 0x98,
	SYS_RSXAUDIO_OBJECT              = 0x60,
};

enum : u64
{
	SYS_PROCESS_PRIMARY_STACK_SIZE_32K  = 0x0000000000000010,
	SYS_PROCESS_PRIMARY_STACK_SIZE_64K  = 0x0000000000000020,
	SYS_PROCESS_PRIMARY_STACK_SIZE_96K  = 0x0000000000000030,
	SYS_PROCESS_PRIMARY_STACK_SIZE_128K = 0x0000000000000040,
	SYS_PROCESS_PRIMARY_STACK_SIZE_256K = 0x0000000000000050,
	SYS_PROCESS_PRIMARY_STACK_SIZE_512K = 0x0000000000000060,
	SYS_PROCESS_PRIMARY_STACK_SIZE_1M   = 0x0000000000000070,
};

constexpr auto SYS_PROCESS_PARAM_SECTION_NAME = ".sys_proc_param";

enum
{
	SYS_PROCESS_PARAM_INVALID_PRIO = -32768,
};

enum : u32
{
	SYS_PROCESS_PARAM_INVALID_STACK_SIZE = 0xffffffff,

	SYS_PROCESS_PARAM_STACK_SIZE_MIN = 0x1000,   // 4KB
	SYS_PROCESS_PARAM_STACK_SIZE_MAX = 0x100000, // 1MB

	SYS_PROCESS_PARAM_VERSION_INVALID = 0xffffffff,
	SYS_PROCESS_PARAM_VERSION_1       = 0x00000001, // for SDK 08X
	SYS_PROCESS_PARAM_VERSION_084_0   = 0x00008400,
	SYS_PROCESS_PARAM_VERSION_090_0   = 0x00009000,
	SYS_PROCESS_PARAM_VERSION_330_0   = 0x00330000,

	SYS_PROCESS_PARAM_MAGIC = 0x13bcc5f6,

	SYS_PROCESS_PARAM_MALLOC_PAGE_SIZE_NONE = 0x00000000,
	SYS_PROCESS_PARAM_MALLOC_PAGE_SIZE_64K  = 0x00010000,
	SYS_PROCESS_PARAM_MALLOC_PAGE_SIZE_1M   = 0x00100000,

	SYS_PROCESS_PARAM_PPC_SEG_DEFAULT       = 0x00000000,
	SYS_PROCESS_PARAM_PPC_SEG_OVLM          = 0x00000001,
	SYS_PROCESS_PARAM_PPC_SEG_FIXEDADDR_PRX = 0x00000002,

	SYS_PROCESS_PARAM_SDK_VERSION_UNKNOWN = 0xffffffff,
};

struct sys_exit2_param
{
	be_t<u64> x0; // 0x85
	be_t<u64> this_size; // 0x30
	be_t<u64> next_size;
	be_t<s64> prio;
	be_t<u64> flags;
	vm::bpptr<char, u64, u64> args;
};

namespace vm
{
	struct ps3_virtual_memory_object;
};

class ppu_function_manager;
struct lv2_memory_container;
struct lv2_rsx_process_info;

struct lv2_process : public ppu_module<lv2_obj>
{
	static constexpr u32 id_base = 0x50000000;

	u32 seg0_code_end = 0;
	u32 elf_entry = 0;

	u32 sdk_ver = umax;
	u32 ppc_seg = 0;
	SelfAdditionalInfo self_info{};
	u32 ctrl_flags1 = 0;

	std::string ELF_file_path;
	std::array<u8, 256> ELF_file_hash{};

	// shared_ptr so it will be initialized externally (incomplete typename)
	std::shared_ptr<vm::ps3_virtual_memory_object> memory_4GB_model;

	// Parent memory container (default for sys_memory_allocate etc)
	// Current process is oblivious to its ID
	shared_ptr<lv2_memory_container> parent_memory_container;

	// HLE table manager
	std::shared_ptr<ppu_function_manager> func_manager;

	// RSX proces-local information
	std::shared_ptr<lv2_rsx_process_info> rsx_info;

	bool has_root_perm() const;
	static bool has_process_root_perm();
	bool has_debug_perm() const;
	bool debug_or_root() const;
	std::string_view get_cellos_appname() const;

	lv2_process() noexcept;
	lv2_process(shared_ptr<lv2_memory_container> pp_memory) noexcept;
	lv2_process(utils::serial&) noexcept;
	void save(utils::serial&) noexcept;
	
	static std::shared_ptr<void> acquire_globals(u32 proc_id);
	static void release_globals();

	struct cleanup_t
	{
		cleanup_t() = default;

		cleanup_t(const cleanup_t&) = delete;
		int operator=(const cleanup_t&) = delete;

		~cleanup_t() noexcept
		{
			lv2_process::release_globals();
		}
	};

	static shared_ptr<lv2_memory_container> get_default_memory_container();
};

// Auxiliary functions
s32 process_getpid();
s32 process_get_sdk_version(u32 pid, s32& ver);
void lv2_exitspawn(ppu_thread& ppu, bool exit_current, shared_ptr<lv2_memory_container> pp_mem, std::vector<std::string>& argv, std::vector<std::string>& envp, std::vector<u8>& data);

namespace vm
{
	std::shared_ptr<vm::ps3_virtual_memory_object> get_current_memory_object();
}

enum CellError : u32;
CellError process_is_spu_lock_line_reservation_address(u32 addr, u64 flags);

// SysCalls
s32 sys_process_getpid();
s32 sys_process_getppid();
error_code sys_process_get_number_of_object(u32 object, vm::ptr<u32> nump);
error_code sys_process_get_id(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size);
error_code sys_process_get_id2(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size);
error_code _sys_process_get_paramsfo(vm::ptr<char> buffer);
error_code sys_process_get_sdk_version(u32 pid, vm::ptr<s32> version);
error_code sys_process_get_status(u64 unk);
error_code sys_process_is_spu_lock_line_reservation_address(u32 addr, u64 flags);
error_code sys_process_kill(u32 pid);
error_code sys_process_wait_for_child(u32 pid, vm::ptr<u32> status, u64 unk);
error_code sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6);
error_code sys_process_detach_child(u64 unk);
void _sys_process_exit(ppu_thread& ppu, s32 status, u32 arg2, u32 arg3);
void _sys_process_exit2(ppu_thread& ppu, s32 status, vm::ptr<sys_exit2_param> arg, u32 arg_size, u32 arg4);
void sys_process_exit3(ppu_thread& ppu, s32 status);
error_code sys_process_spawns_a_self2(ppu_thread& ppu, vm::ptr<u32> pid, u32 primary_prio, u64 flags, vm::ptr<void> stack, u32 stack_size, u32 mem_id, vm::ptr<void> param_sfo, vm::ptr<void> dbg_data);
