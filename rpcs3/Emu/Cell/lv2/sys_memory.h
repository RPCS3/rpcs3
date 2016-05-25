#pragma once

#include "Emu/Memory/Memory.h"
#include "Emu/Cell/ErrorCodes.h"
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
	const u32 size = 0x10000000; // Amount of "physical" memory in this container

	atomic_t<u32> used{}; // Amount of "physical" memory currently used
	
	lv2_memory_container() = default;

	lv2_memory_container(u32 size)
		: size(size)
	{
	}

	// Try to get specified amount of "physical" memory
	u32 take(u32 amount)
	{
		const u32 old_value = used.fetch_op([&](u32& value)
		{
			if (size - value >= amount)
			{
				value += amount;
			}
		});

		if (size - old_value >= amount)
		{
			return amount;
		}

		return 0;
	}
};

// SysCalls
ppu_error_code sys_memory_allocate(u32 size, u64 flags, vm::ps3::ptr<u32> alloc_addr);
ppu_error_code sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ps3::ptr<u32> alloc_addr);
ppu_error_code sys_memory_free(u32 start_addr);
ppu_error_code sys_memory_get_page_attribute(u32 addr, vm::ps3::ptr<sys_page_attr_t> attr);
ppu_error_code sys_memory_get_user_memory_size(vm::ps3::ptr<sys_memory_info_t> mem_info);
ppu_error_code sys_memory_container_create(vm::ps3::ptr<u32> cid, u32 size);
ppu_error_code sys_memory_container_destroy(u32 cid);
ppu_error_code sys_memory_container_get_size(vm::ps3::ptr<sys_memory_info_t> mem_info, u32 cid);
