#pragma once

#include "sys_sync.h"
#include <vector>

struct lv2_memory_container;

struct lv2_memory : lv2_obj
{
	static const u32 id_base = 0x08000000;

	const u32 size; // Memory size
	const u32 align; // Alignment required
	const u64 flags;
	const std::shared_ptr<lv2_memory_container> ct; // Associated memory container
	const std::shared_ptr<utils::shm> shm;

	atomic_t<u32> counter{0};

	lv2_memory(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container>& ct);
};

enum : u64
{
	SYS_MEMORY_PAGE_FAULT_EVENT_KEY	       = 0xfffe000000000000ULL,
};

enum : u32
{
	SYS_MEMORY_PAGE_FAULT_CAUSE_NON_MAPPED = 0x00000002U,
	SYS_MEMORY_PAGE_FAULT_CAUSE_READ_ONLY  = 0x00000001U,
	SYS_MEMORY_PAGE_FAULT_TYPE_PPU_THREAD  = 0x00000000U,
	SYS_MEMORY_PAGE_FAULT_TYPE_SPU_THREAD  = 0x00000001U,
	SYS_MEMORY_PAGE_FAULT_TYPE_RAW_SPU     = 0x00000002U,
};

struct page_fault_notification_entry
{
	u32 start_addr; // Starting address of region to monitor.
	u32 event_queue_id; // Queue to be notified.
	u32 port_id; // Port used to notify the queue.
};

// Used to hold list of queues to be notified on page fault event.
struct page_fault_notification_entries
{
	std::vector<page_fault_notification_entry> entries;
};

struct page_fault_event
{
	u32 thread_id;
	u32 fault_addr;
};

struct page_fault_event_entries
{
	std::vector<page_fault_event> events;
	semaphore<> pf_mutex;
};

// SysCalls
error_code sys_mmapper_allocate_address(u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr);
error_code sys_mmapper_allocate_fixed_address();
error_code sys_mmapper_allocate_shared_memory(u64 unk, u32 size, u64 flags, vm::ptr<u32> mem_id);
error_code sys_mmapper_allocate_shared_memory_from_container(u64 unk, u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id);
error_code sys_mmapper_change_address_access_right(u32 addr, u64 flags);
error_code sys_mmapper_free_address(u32 addr);
error_code sys_mmapper_free_shared_memory(u32 mem_id);
error_code sys_mmapper_map_shared_memory(u32 addr, u32 mem_id, u64 flags);
error_code sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_mmapper_unmap_shared_memory(u32 addr, vm::ptr<u32> mem_id);
error_code sys_mmapper_enable_page_fault_notification(u32 start_addr, u32 event_queue_id);
