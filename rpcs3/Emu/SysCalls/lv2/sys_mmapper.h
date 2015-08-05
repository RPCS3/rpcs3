#pragma once

#include "sys_memory.h"

namespace vm { using namespace ps3; }

struct lv2_memory_t
{
	const u32 size; // memory size
	const u32 align; // required alignment
	const u32 id;
	const u64 flags;
	const std::shared_ptr<lv2_memory_container_t> ct; // memory container the physical memory is taken from

	std::atomic<u32> addr{ 0 }; // actual mapping address

	lv2_memory_t(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container_t> ct);
};

// SysCalls
s32 sys_mmapper_allocate_address(u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr);
s32 sys_mmapper_allocate_fixed_address();
s32 sys_mmapper_allocate_memory(u64 size, u64 flags, vm::ptr<u32> mem_id);
s32 sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id);
s32 sys_mmapper_change_address_access_right(u32 addr, u64 flags);
s32 sys_mmapper_free_address(u32 addr);
s32 sys_mmapper_free_memory(u32 mem_id);
s32 sys_mmapper_map_memory(u32 addr, u32 mem_id, u64 flags);
s32 sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr);
s32 sys_mmapper_unmap_memory(u32 addr, vm::ptr<u32> mem_id);
s32 sys_mmapper_enable_page_fault_notification(u32 addr, u32 eq);
