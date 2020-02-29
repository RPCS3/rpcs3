#pragma once

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/IdManager.h"

enum : u32
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
	SYS_MEMORY_PAGE_SIZE_1M   = 0x400ull,
	SYS_MEMORY_PAGE_SIZE_64K  = 0x200ull,
	SYS_MEMORY_PAGE_SIZE_MASK = 0xf00ull,
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
	atomic_t<u32> used{}; // Amount of "physical" memory currently used

	lv2_memory_container(u32 size)
		: size(size)
	{
	}

	// Try to get specified amount of "physical" memory
	u32 take(u32 amount)
	{
		auto [_, result] = used.fetch_op([&](u32& value) -> u32
		{
			if (size - value >= amount)
			{
				value += amount;
				return amount;
			}

			return 0;
		});

		return result;
	}
};

struct lv2_memory_alloca
{
	static const u32 id_base = 0x1;
	static const u32 id_step = 0x1;
	static const u32 id_count = 0x2000;

	const u32 size; // Memory size
	const u32 align; // Alignment required
	const u64 flags;
	const std::shared_ptr<lv2_memory_container> ct;
	const std::shared_ptr<utils::shm> shm;

	lv2_memory_alloca(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container>& ct);
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
error_code sys_memory_allocate(u32 size, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_memory_free(u32 start_addr);
error_code sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr);
error_code sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info);
error_code sys_memory_get_user_memory_stat(vm::ptr<sys_memory_user_memory_stat_t> mem_stat);
error_code sys_memory_container_create(vm::ptr<u32> cid, u32 size);
error_code sys_memory_container_destroy(u32 cid);
error_code sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid);
