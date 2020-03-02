#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

#include <vector>

struct lv2_memory_container;

struct lv2_memory : lv2_obj
{
	static const u32 id_base = 0x08000000;

	const u32 size; // Memory size
	const u32 align; // Alignment required
	const u64 flags;
	lv2_memory_container* const ct; // Associated memory container
	const std::shared_ptr<utils::shm> shm;

	atomic_t<u32> counter{0};

	lv2_memory(u32 size, u32 align, u64 flags, lv2_memory_container* ct);
};

enum : u64
{
	SYS_MEMORY_PAGE_FAULT_EVENT_KEY	       = 0xfffe000000000000ULL,
};

enum : u64
{
	SYS_MMAPPER_NO_SHM_KEY = 0xffff000000000000ull, // Unofficial name
};

enum : u64
{
	SYS_MEMORY_PAGE_FAULT_CAUSE_NON_MAPPED = 0x2ULL,
	SYS_MEMORY_PAGE_FAULT_CAUSE_READ_ONLY  = 0x1ULL,
	SYS_MEMORY_PAGE_FAULT_TYPE_PPU_THREAD  = 0x0ULL,
	SYS_MEMORY_PAGE_FAULT_TYPE_SPU_THREAD  = 0x1ULL,
	SYS_MEMORY_PAGE_FAULT_TYPE_RAW_SPU     = 0x2ULL,
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
	shared_mutex mutex;
};

struct page_fault_event_entries
{
	// First = thread id, second = addr
	std::unordered_map<u32, u32> events;
	shared_mutex pf_mutex;
	cond_variable cond;
};

struct mmapper_unk_entry_struct0
{
	be_t<u32> a;    // 0x0
	be_t<u32> b;    // 0x4
	be_t<u32> c;    // 0x8
	be_t<u32> d;    // 0xc
	be_t<u64> type; // 0x10
};

// Aux
class ppu_thread;

error_code mmapper_thread_recover_page_fault(u32 id);

// SysCalls
error_code sys_mmapper_allocate_address(ppu_thread&, u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr);
error_code sys_mmapper_allocate_fixed_address(ppu_thread&);
error_code sys_mmapper_allocate_shared_memory(ppu_thread&, u64 ipc_key, u32 size, u64 flags, vm::ptr<u32> mem_id);
error_code sys_mmapper_allocate_shared_memory_from_container(ppu_thread&, u64 ipc_key, u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id);
error_code sys_mmapper_allocate_shared_memory_ext(ppu_thread&, u64 ipc_key, u32 size, u32 flags, vm::ptr<mmapper_unk_entry_struct0> src, s32 count, vm::ptr<u32> mem_id);
error_code sys_mmapper_allocate_shared_memory_from_container_ext(ppu_thread&, u64 ipc_key, u32 size, u64 flags, u32 mc_id, vm::ptr<mmapper_unk_entry_struct0> entries, s32 entry_count, vm::ptr<u32> mem_id);
error_code sys_mmapper_change_address_access_right(ppu_thread&, u32 addr, u64 flags);
error_code sys_mmapper_free_address(ppu_thread&, u32 addr);
error_code sys_mmapper_free_shared_memory(ppu_thread&, u32 mem_id);
error_code sys_mmapper_map_shared_memory(ppu_thread&, u32 addr, u32 mem_id, u64 flags);
error_code sys_mmapper_search_and_map(ppu_thread&, u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr);
error_code sys_mmapper_unmap_shared_memory(ppu_thread&, u32 addr, vm::ptr<u32> mem_id);
error_code sys_mmapper_enable_page_fault_notification(ppu_thread&, u32 start_addr, u32 event_queue_id);
