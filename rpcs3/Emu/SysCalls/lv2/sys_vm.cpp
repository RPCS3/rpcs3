#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_memory.h"
#include "sys_vm.h"

SysCallBase sys_vm("sys_vm");
std::shared_ptr<MemoryContainerInfo> current_ct;

s32 sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, u32 addr)
{
	sys_vm.Error("sys_vm_memory_map(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr_addr=0x%x)", 
		vsize, psize, cid, flag, policy, addr);

	// Check virtual size.
	if((vsize < (0x100000 * 32)) || (vsize > (0x100000 * 256)))
	{
		return CELL_EINVAL;
	}

	// Check physical size.
	if(psize > (0x100000 * 256))
	{
		return CELL_ENOMEM;
	}

	// Use fixed address (TODO: search and use some free address instead)
	u32 new_addr = vm::check_addr(0x60000000) ? 0x70000000 : 0x60000000;

	// If container ID is SYS_MEMORY_CONTAINER_ID_INVALID, allocate directly.
	if(cid == SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		// Create a new MemoryContainerInfo to act as default container with vsize.
		current_ct.reset(new MemoryContainerInfo(new_addr, vsize));
	}
	else
	{
		// Check memory container.
		const auto ct = Emu.GetIdManager().get<MemoryContainerInfo>(cid);

		if (!ct)
		{
			return CELL_ESRCH;
		}

		current_ct = ct;
	}

	// Allocate actual memory using virtual size (physical size is ignored)
	if (!Memory.Map(new_addr, vsize))
	{
		return CELL_ENOMEM;
	}

	// Write a pointer for the allocated memory.
	vm::write32(addr, new_addr);

	return CELL_OK;
}

s32 sys_vm_unmap(u32 addr)
{
	sys_vm.Error("sys_vm_unmap(addr=0x%x)", addr);

	// Unmap memory.
	assert(addr == 0x60000000 || addr == 0x70000000);
	if(!Memory.Unmap(addr)) return CELL_EINVAL;

	return CELL_OK;
}

s32 sys_vm_append_memory(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_append_memory(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// Total memory size must not be superior to 256MB.
	if((current_ct->size + size) > (0x100000 * 256))
	{
		return CELL_ENOMEM;
	}

	// The size is added to the virtual size, which should be inferior to the physical size allocated.
	current_ct->size += size;

	return CELL_OK;
}

s32 sys_vm_return_memory(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_return_memory(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// The memory size to return should not be superior to the virtual size in use minus 1MB.
	if(current_ct->size < (size + 0x100000))
	{
		return CELL_EBUSY;
	}

	// The size is returned to physical memory and is subtracted to the virtual size.
	current_ct->size -= size;

	return CELL_OK;
}

s32 sys_vm_lock(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_lock(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// The memory size to return should not be superior to the virtual size to lock minus 1MB.
	if(current_ct->size < (size + 0x100000))
	{
		return CELL_EBUSY;
	}

	// TODO: The locked memory area keeps allocated and unchanged until sys_vm_unlocked is called.
	return CELL_OK;
}

s32 sys_vm_unlock(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_unlock(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO: Unlock
	return CELL_OK;
}

s32 sys_vm_touch(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_touch(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_touch allocates physical memory for a virtual memory address.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

s32 sys_vm_flush(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_flush(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_flush frees physical memory for a virtual memory address and creates a backup if the memory area is dirty.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

s32 sys_vm_invalidate(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_invalidate(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_invalidate frees physical memory for a virtual memory address.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

s32 sys_vm_store(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_store(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_store creates a backup for a dirty virtual memory area and marks it as clean.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

s32 sys_vm_sync(u32 addr, u32 size)
{
	sys_vm.Todo("sys_vm_sync(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_sync stalls execution until all asynchronous vm calls finish.

	return CELL_OK;
}

s32 sys_vm_test(u32 addr, u32 size, vm::ptr<u64> result)
{
	sys_vm.Todo("sys_vm_test(addr=0x%x, size=0x%x, result_addr=0x%x)", addr, size, result.addr());

	// Check address and size.
	if((current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_test checks the state of a portion of the virtual memory area.

	// Faking.
	*result = SYS_VM_TEST_ALLOCATED;

	return CELL_OK;
}

s32 sys_vm_get_statistics(u32 addr, vm::ptr<sys_vm_statistics> stat)
{
	sys_vm.Todo("sys_vm_get_statistics(addr=0x%x, stat_addr=0x%x)", addr, stat.addr());

	// Check address.
	if(current_ct->addr != addr)
	{
		return CELL_EINVAL;
	}

	// TODO
	stat->page_fault_ppu = 0;
	stat->page_fault_spu = 0;
	stat->page_in = 0;
	stat->page_out = 0;
	stat->pmem_total = current_ct->size;
	stat->pmem_used = 0;
	stat->timestamp = 0;
	return CELL_OK;
}