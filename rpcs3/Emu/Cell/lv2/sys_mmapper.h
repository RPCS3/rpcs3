#pragma once

#include "sys_sync.h"
#include "sys_memory.h"

struct lv2_memory : lv2_obj
{
	static const u32 id_base = 0x08000000;

	const u32 size; // Memory size
	const u32 align; // Alignment required
	const u64 flags;
	const std::shared_ptr<lv2_memory_container> ct; // Associated memory container

	atomic_t<u32> addr{}; // Actual mapping address

	std::vector<uchar> data;

	lv2_memory(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container>& ct)
		: size(size)
		, align(align)
		, flags(flags)
		, ct(ct)
	{
		data.resize(size);
	}
};

// SysCalls
error_code sys_mmapper_allocate_address(u64 size, u64 flags, u64 alignment, vm::ps3::ptr<u32> alloc_addr);
error_code sys_mmapper_allocate_fixed_address();
error_code sys_mmapper_allocate_shared_memory(u64 unk, u32 size, u64 flags, vm::ps3::ptr<u32> mem_id);
error_code sys_mmapper_allocate_shared_memory_from_container(u64 unk, u32 size, u32 cid, u64 flags, vm::ps3::ptr<u32> mem_id);
error_code sys_mmapper_change_address_access_right(u32 addr, u64 flags);
error_code sys_mmapper_free_address(u32 addr);
error_code sys_mmapper_free_shared_memory(u32 mem_id);
error_code sys_mmapper_map_shared_memory(u32 addr, u32 mem_id, u64 flags);
error_code sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, vm::ps3::ptr<u32> alloc_addr);
error_code sys_mmapper_unmap_shared_memory(u32 addr, vm::ps3::ptr<u32> mem_id);
error_code sys_mmapper_enable_page_fault_notification(u32 addr, u32 eq);
