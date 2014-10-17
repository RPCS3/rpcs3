#pragma once

#define SYS_MMAPPER_FIXED_ADDR              0xB0000000
#define SYS_MMAPPER_FIXED_SIZE              0x10000000

struct mmapper_info
{
	u32 size;
	u64 flags;

	mmapper_info(u32 _size, u64 _flags)
		: size(_size)
		, flags(_flags)
	{
	}

	mmapper_info()
	{
	}
};

// SysCalls
s32 sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr);
s32 sys_mmapper_allocate_fixed_address();
s32 sys_mmapper_allocate_memory(u32 size, u64 flags, vm::ptr<u32> mem_id);
s32 sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id);
s32 sys_mmapper_change_address_access_right(u32 start_addr, u64 flags);
s32 sys_mmapper_free_address(u32 start_addr);
s32 sys_mmapper_free_memory(u32 mem_id);
s32 sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags);
s32 sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, u32 alloc_addr);
s32 sys_mmapper_unmap_memory(u32 start_addr, u32 mem_id_addr);
s32 sys_mmapper_enable_page_fault_notification(u32 start_addr, u32 q_id);
