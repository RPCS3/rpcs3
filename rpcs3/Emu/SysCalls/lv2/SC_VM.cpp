#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Memory.h"

SysCallBase sc_vm("vm");
MemoryContainerInfo* current_ct;

int sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, u32 addr)
{
	sc_vm.Warning("sys_vm_memory_map(vsize=0x%x,psize=0x%x,cidr=0x%x,flags=0x%llx,policy=0x%llx,addr=0x%x)", 
		vsize, psize, cid, flag, policy, addr);

	// Check output address.
	if(!Memory.IsGoodAddr(addr, 4))
	{
		return CELL_EFAULT;
	}

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

	// If container ID is SYS_MEMORY_CONTAINER_ID_INVALID, allocate directly.
	if(cid == SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		u32 new_addr;
		switch(flag)
		{
		case SYS_MEMORY_PAGE_SIZE_1M:
			new_addr = Memory.Alloc(psize, 0x100000);
			break;

		case SYS_MEMORY_PAGE_SIZE_64K:
			new_addr = Memory.Alloc(psize, 0x10000);
			break;

		default: return CELL_EINVAL;
		}

		if(!new_addr) return CELL_ENOMEM;

		// Create a new MemoryContainerInfo to act as default container with vsize.
		current_ct = new MemoryContainerInfo(new_addr, vsize);
	}
	else
	{
		// Check memory container.
		MemoryContainerInfo* ct;
		if(!sc_vm.CheckId(cid, ct)) return CELL_ESRCH;

		current_ct = ct;
	}

	// Write a pointer for the allocated memory.
	Memory.Write32(addr, current_ct->addr);

	return CELL_OK;
}

int sys_vm_unmap(u32 addr)
{
	sc_vm.Warning("sys_vm_unmap(addr=0x%x)", addr);

	// Simply free the memory to unmap.
	if(!Memory.Free(addr)) return CELL_EINVAL;

	return CELL_OK;
}

int sys_vm_append_memory(u32 addr, u32 size)
{
	sc_vm.Warning("sys_vm_append_memory(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (size <= 0))
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

int sys_vm_return_memory(u32 addr, u32 size)
{
	sc_vm.Warning("sys_vm_return_memory(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (size <= 0))
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

int sys_vm_lock(u32 addr, u32 size)
{
	sc_vm.Warning("sys_vm_lock(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// The memory size to return should not be superior to the virtual size to lock minus 1MB.
	if(current_ct->size < (size + 0x100000))
	{
		return CELL_EBUSY;
	}

	// The locked memory area keeps allocated and unchanged until sys_vm_unlocked is called.
	Memory.Lock(addr, size);

	return CELL_OK;
}

int sys_vm_unlock(u32 addr, u32 size)
{
	sc_vm.Warning("sys_vm_unlock(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	Memory.Unlock(addr, size);

	return CELL_OK;
}

int sys_vm_touch(u32 addr, u32 size)
{
	sc_vm.Warning("Unimplemented function: sys_vm_touch(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_touch allocates physical memory for a virtual memory address.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

int sys_vm_flush(u32 addr, u32 size)
{
	sc_vm.Warning("Unimplemented function: sys_vm_flush(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_flush frees physical memory for a virtual memory address and creates a backup if the memory area is dirty.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

int sys_vm_invalidate(u32 addr, u32 size)
{
	sc_vm.Warning("Unimplemented function: sys_vm_invalidate(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_invalidate frees physical memory for a virtual memory address.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

int sys_vm_store(u32 addr, u32 size)
{
	sc_vm.Warning("Unimplemented function: sys_vm_store(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_store creates a backup for a dirty virtual memory area and marks it as clean.
	// This function is asynchronous, so it may not complete immediately.

	return CELL_OK;
}

int sys_vm_sync(u32 addr, u32 size)
{
	sc_vm.Warning("Unimplemented function: sys_vm_sync(addr=0x%x,size=0x%x)", addr, size);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_sync stalls execution until all asynchronous vm calls finish.

	return CELL_OK;
}

int sys_vm_test(u32 addr, u32 size, u32 result_addr)
{
	sc_vm.Warning("Unimplemented function: sys_vm_test(addr=0x%x,size=0x%x,result_addr=0x%x)", addr, size, result_addr);

	// Check address and size.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr) || (current_ct->size < size) || (size <= 0))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_test checks the state of a portion of the virtual memory area.

	// Faking.
	Memory.Write64(result_addr, SYS_VM_TEST_ALLOCATED);

	return CELL_OK;
}

int sys_vm_get_statistics(u32 addr, u32 stat_addr)
{
	sc_vm.Warning("Unimplemented function: sys_vm_get_statistics(addr=0x%x,stat_addr=0x%x)", addr, stat_addr);

	// Check address.
	if(!Memory.IsGoodAddr(addr, 4) ||  (current_ct->addr != addr))
	{
		return CELL_EINVAL;
	}

	// TODO
	// sys_vm_get_statistics collects virtual memory management stats.

	sys_vm_statistics stats;
	stats.physical_mem_size = current_ct->size;  // Total physical memory allocated for the virtual memory area.
	stats.physical_mem_used = 0;                 // Physical memory in use by the virtual memory area.
	stats.timestamp = 0;                         // Current time.
	stats.vm_crash_ppu = 0;                      // Number of bad virtual memory accesses from a PPU thread.
	stats.vm_crash_spu = 0;                      // Number of bad virtual memory accesses from a SPU thread.
	stats.vm_read = 0;                           // Number of virtual memory backup reading operations.
	stats.vm_write = 0;                          // Number of virtual memory backup writing operations.
	Memory.WriteData(stat_addr, stats);          // Faking.

	return CELL_OK;
}