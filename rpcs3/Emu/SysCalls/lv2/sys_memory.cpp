#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_memory.h"
#include <map>

SysCallBase sys_memory("sys_memory");

s32 sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr)
{
	sys_memory.Log("sys_memory_allocate(size=0x%x, flags=0x%x)", size, flags);
	
	// Check page size.
	u32 addr;
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		addr = (u32)Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		addr = (u32)Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	if(!addr)
		return CELL_ENOMEM;
	
	// Write back the start address of the allocated area.
	sys_memory.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", addr, size);
	vm::write32(alloc_addr_addr, addr);

	return CELL_OK;
}

s32 sys_memory_allocate_from_container(u32 size, u32 cid, u32 flags, u32 alloc_addr_addr)
{
	sys_memory.Log("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%x)", size, cid, flags);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if (!sys_memory.CheckId(cid, ct))
		return CELL_ESRCH;
	
	// Check page size.
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		ct->addr = (u32)Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		ct->addr = (u32)Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	// Store the address and size in the container.
	if(!ct->addr)
		return CELL_ENOMEM;
	ct->size = size;

	// Write back the start address of the allocated area.
	sys_memory.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", ct->addr, ct->size);
	vm::write32(alloc_addr_addr, ct->addr);

	return CELL_OK;
}

s32 sys_memory_free(u32 start_addr)
{
	sys_memory.Log("sys_memory_free(start_addr=0x%x)", start_addr);

	// Release the allocated memory.
	if(!Memory.Free(start_addr))
		return CELL_EINVAL;

	return CELL_OK;
}

s32 sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	sys_memory.Warning("sys_memory_get_page_attribute(addr=0x%x, attr_addr=0x%x)", addr, attr.addr());

	// TODO: Implement per thread page attribute setting.
	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE
	attr->access_right = 0xFull; // SYS_MEMORY_ACCESS_RIGHT_ANY
	attr->page_size = 4096;

	return CELL_OK;
}

s32 sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info)
{
	sys_memory.Warning("sys_memory_get_user_memory_size(mem_info_addr=0x%x)", mem_info.addr());
	
	// Fetch the user memory available.
	mem_info->total_user_memory = Memory.GetUserMemTotalSize();
	mem_info->available_user_memory = Memory.GetUserMemAvailSize();
	return CELL_OK;
}

s32 sys_memory_container_create(vm::ptr<u32> cid, u32 yield_size)
{
	sys_memory.Warning("sys_memory_container_create(cid_addr=0x%x, yield_size=0x%x)", cid.addr(), yield_size);

	yield_size &= ~0xfffff; //round down to 1 MB granularity
	u32 addr = (u32)Memory.Alloc(yield_size, 0x100000); //1 MB alignment

	if(!addr)
		return CELL_ENOMEM;

	// Wrap the allocated memory in a memory container.
	MemoryContainerInfo *ct = new MemoryContainerInfo(addr, yield_size);
	u32 id = sys_memory.GetNewId(ct, TYPE_MEM);
	*cid = id;

	sys_memory.Warning("*** memory_container created(addr=0x%llx): id = %d", addr, id);

	return CELL_OK;
}

s32 sys_memory_container_destroy(u32 cid)
{
	sys_memory.Warning("sys_memory_container_destroy(cid=%d)", cid);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if (!sys_memory.CheckId(cid, ct))
		return CELL_ESRCH;

	// Release the allocated memory and remove the ID.
	Memory.Free(ct->addr);
	sys_memory.RemoveId(cid);

	return CELL_OK;
}

s32 sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid)
{
	sys_memory.Warning("sys_memory_container_get_size(mem_info_addr=0x%x, cid=%d)", mem_info.addr(), cid);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if (!sys_memory.CheckId(cid, ct))
		return CELL_ESRCH;

	// HACK: Return all memory.
	sys_memory_info_t info;
	mem_info->total_user_memory = ct->size;
	mem_info->available_user_memory = ct->size;
	return CELL_OK;
}
