#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "sys_memory.h"

#include <array>

enum : u64
{
	SYS_VM_STATE_INVALID   = 0ull,
	SYS_VM_STATE_UNUSED    = 1ull,
	SYS_VM_STATE_ON_MEMORY = 2ull,
	SYS_VM_STATE_STORED    = 4ull,

	SYS_VM_POLICY_AUTO_RECOMMENDED = 1ull,
};

struct sys_vm_statistics_t
{
	be_t<u64> page_fault_ppu; // Number of bad virtual memory accesses from a PPU thread.
	be_t<u64> page_fault_spu; // Number of bad virtual memory accesses from a SPU thread.
	be_t<u64> page_in;        // Number of virtual memory backup reading operations.
	be_t<u64> page_out;       // Number of virtual memory backup writing operations.
	be_t<u32> pmem_total;     // Total physical memory allocated for the virtual memory area.
	be_t<u32> pmem_used;      // Physical memory in use by the virtual memory area.
	be_t<u64> timestamp;
};

// Block info
struct sys_vm_t
{
	static const u32 id_base = 0x1;
	static const u32 id_step = 0x1;
	static const u32 id_count = 16;

	lv2_memory_container* const ct;
	const u32 addr;
	const u32 size;
	atomic_t<u32> psize;

	sys_vm_t(u32 addr, u32 vsize, lv2_memory_container* ct, u32 psize);
	~sys_vm_t();

	SAVESTATE_INIT_POS(10);

	sys_vm_t(utils::serial& ar);
	void save(utils::serial& ar);

	static std::array<atomic_t<u32>, id_count> g_ids;

	static u32 find_id(u32 addr)
	{
		return g_ids[addr >> 28].load();
	}
};

// Aux
class ppu_thread;

// SysCalls
error_code sys_vm_memory_map(ppu_thread& ppu, u64 vsize, u64 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr);
error_code sys_vm_memory_map_different(ppu_thread& ppu, u64 vsize, u64 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr);
error_code sys_vm_unmap(ppu_thread& ppu, u32 addr);
error_code sys_vm_append_memory(ppu_thread& ppu, u32 addr, u64 size);
error_code sys_vm_return_memory(ppu_thread& ppu, u32 addr, u64 size);
error_code sys_vm_lock(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_unlock(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_touch(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_flush(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_invalidate(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_store(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_sync(ppu_thread& ppu, u32 addr, u32 size);
error_code sys_vm_test(ppu_thread& ppu, u32 addr, u32 size, vm::ptr<u64> result);
error_code sys_vm_get_statistics(ppu_thread& ppu, u32 addr, vm::ptr<sys_vm_statistics_t> stat);
