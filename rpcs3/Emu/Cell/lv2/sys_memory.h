#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

class cpu_thread;
class ppu_thread;

enum lv2_mem_container_id : u32
{
	SYS_MEMORY_CONTAINER_ID_INVALID = 0xFFFFFFFF,
};

enum : u64
{
	SYS_MEMORY_ACCESS_RIGHT_NONE    = 0x00000000000000F0ULL,
	SYS_MEMORY_ACCESS_RIGHT_ANY     = 0x000000000000000FULL,
	SYS_MEMORY_ACCESS_RIGHT_PPU_THR = 0x0000000000000008ULL,
	SYS_MEMORY_ACCESS_RIGHT_HANDLER = 0x0000000000000004ULL,
	SYS_MEMORY_ACCESS_RIGHT_SPU_THR = 0x0000000000000002ULL,
	SYS_MEMORY_ACCESS_RIGHT_RAW_SPU = 0x0000000000000001ULL,

	SYS_MEMORY_ATTR_READ_ONLY       = 0x0000000000080000ULL,
	SYS_MEMORY_ATTR_READ_WRITE      = 0x0000000000040000ULL,
};

enum : u64
{
	SYS_MEMORY_PAGE_SIZE_4K   = 0x100ull,
	SYS_MEMORY_PAGE_SIZE_64K  = 0x200ull,
	SYS_MEMORY_PAGE_SIZE_1M   = 0x400ull,
	SYS_MEMORY_PAGE_SIZE_MASK = 0xf00ull,
};

enum : u64
{
	SYS_MEMORY_GRANULARITY_64K  = 0x0000000000000200,
	SYS_MEMORY_GRANULARITY_1M   = 0x0000000000000400,
	SYS_MEMORY_GRANULARITY_MASK = 0x0000000000000f00,
};

enum : u64
{
	SYS_MEMORY_PROT_READ_WRITE = 0x0000000000040000,
	SYS_MEMORY_PROT_READ_ONLY  = 0x0000000000080000,
	SYS_MEMORY_PROT_MASK       = 0x00000000000f0000,
};

struct sys_memory_info_t
{
	be_t<u32> total_user_memory;
	be_t<u32> available_user_memory;
};


struct sys_page_attr_t
{
	be_t<u64> attribute;
	be_t<u64> access_right;
	be_t<u32> page_size;
	be_t<u32> pad;
};

struct lv2_memory_container
{
	static const u32 id_base = 0x3F000000;
	static const u32 id_step = 0x1;
	static const u32 id_count = 16;

	const u32 size; // Amount of "physical" memory in this container
	const lv2_mem_container_id id; // ID of the container in if placed at IDM, otherwise SYS_MEMORY_CONTAINER_ID_INVALID
	atomic_t<u32> used{}; // Amount of "physical" memory currently used

	SAVESTATE_INIT_POS(1);

	lv2_memory_container(u32 size, bool from_idm = false) noexcept;
	lv2_memory_container(utils::serial& ar, bool from_idm = false) noexcept;
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);
	static lv2_memory_container* search(u32 id);

	// Try to get specified amount of "physical" memory
	// Values greater than UINT32_MAX will fail
	u32 take(u64 amount)
	{
		auto [_, result] = used.fetch_op([&](u32& value) -> u32
		{
			if (size - value >= amount)
			{
				value += static_cast<u32>(amount);
				return static_cast<u32>(amount);
			}

			return 0;
		});

		return result;
	}

	u32 free(u64 amount)
	{
		auto [_, result] = used.fetch_op([&](u32& value) -> u32
		{
			if (value >= amount)
			{
				value -= static_cast<u32>(amount);
				return static_cast<u32>(amount);
			}

			return 0;
		});

		// Sanity check
		ensure(result == amount);

		return result;
	}
};

struct sys_memory_user_memory_stat_t
{
	be_t<u32> a; // 0x0
	be_t<u32> b; // 0x4
	be_t<u32> c; // 0x8
	be_t<u32> d; // 0xc
	be_t<u32> e; // 0x10
	be_t<u32> f; // 0x14
	be_t<u32> g; // 0x18
};

// SysCalls
error_code sys_memory_allocate(cpu_thread& cpu, u64 size, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_memory_allocate_from_container(cpu_thread& cpu, u64 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_memory_free(cpu_thread& cpu, u32 start_addr);
error_code sys_memory_get_page_attribute(ppu_thread& cpu, u32 addr, vm::ptr<sys_page_attr_t> attr);
error_code sys_memory_get_user_memory_size(cpu_thread& cpu, vm::ptr<sys_memory_info_t> mem_info);
error_code sys_memory_get_user_memory_stat(cpu_thread& cpu, vm::ptr<sys_memory_user_memory_stat_t> mem_stat);
error_code sys_memory_container_create(cpu_thread& cpu, vm::ptr<u32> cid, u64 size);
error_code sys_memory_container_destroy(cpu_thread& cpu, u32 cid);
error_code sys_memory_container_get_size(cpu_thread& cpu, vm::ptr<sys_memory_info_t> mem_info, u32 cid);
error_code sys_memory_container_destroy_parent_with_childs(cpu_thread& cpu, u32 cid, u32 must_0, vm::ptr<u32> mc_child);
