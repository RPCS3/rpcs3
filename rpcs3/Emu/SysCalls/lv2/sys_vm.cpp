#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_memory.h"
#include "sys_vm.h"

SysCallBase sys_vm("sys_vm");

s32 sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	sys_vm.Error("sys_vm_memory_map(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);

	LV2_LOCK;

	// Use fixed address (TODO: search and use some free address instead)
	const u32 new_addr = vm::check_addr(0x60000000) ? 0x70000000 : 0x60000000;

	// Map memory
	const auto area = vm::map(new_addr, vsize, flag);

	// Alloc memory
	if (!area || !area->alloc(vsize))
	{
		return CELL_ENOMEM;
	}

	// Write a pointer for the allocated memory.
	*addr = new_addr;

	return CELL_OK;
}

s32 sys_vm_unmap(u32 addr)
{
	sys_vm.Error("sys_vm_unmap(addr=0x%x)", addr);

	LV2_LOCK;

	if (!vm::unmap(addr))
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_vm_append_memory(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_append_memory(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_return_memory(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_return_memory(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_lock(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_lock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_unlock(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_unlock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_touch(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_touch(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_flush(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_flush(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_invalidate(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_invalidate(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_store(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_store(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_sync(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_sync(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

s32 sys_vm_test(u32 addr, u32 size, vm::ptr<u64> result)
{
	sys_vm.Todo("sys_vm_test(addr=0x%x, size=0x%x, result=*0x%x)", addr, size, result);

	*result = SYS_VM_STATE_ON_MEMORY;

	return CELL_OK;
}

s32 sys_vm_get_statistics(u32 addr, vm::ptr<sys_vm_statistics_t> stat)
{
	sys_vm.Todo("sys_vm_get_statistics(addr=0x%x, stat=*0x%x)", addr, stat);

	stat->page_fault_ppu = 0;
	stat->page_fault_spu = 0;
	stat->page_in = 0;
	stat->page_out = 0;
	stat->pmem_total = 0;
	stat->pmem_used = 0;
	stat->timestamp = 0;

	return CELL_OK;
}
