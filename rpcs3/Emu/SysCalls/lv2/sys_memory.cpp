#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_memory.h"
#include <map>

SysCallBase sc_mem("memory");

s32 sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr)
{
	sc_mem.Log("sys_memory_allocate(size=0x%x, flags=0x%x)", size, flags);
	
	// Check page size.
	u32 addr;
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	if(!addr)
		return CELL_ENOMEM;
	
	// Write back the start address of the allocated area.
	sc_mem.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", addr, size);
	Memory.Write32(alloc_addr_addr, addr);

	return CELL_OK;
}

s32 sys_memory_allocate_from_container(u32 size, u32 cid, u32 flags, u32 alloc_addr_addr)
{
	sc_mem.Log("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%x)", size, cid, flags);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if(!sc_mem.CheckId(cid, ct))
		return CELL_ESRCH;
	
	// Check page size.
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		ct->addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		ct->addr = Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	// Store the address and size in the container.
	if(!ct->addr)
		return CELL_ENOMEM;
	ct->size = size;

	// Write back the start address of the allocated area.
	sc_mem.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", ct->addr, ct->size);
	Memory.Write32(alloc_addr_addr, ct->addr);

	return CELL_OK;
}

s32 sys_memory_free(u32 start_addr)
{
	sc_mem.Log("sys_memory_free(start_addr=0x%x)", start_addr);

	// Release the allocated memory.
	if(!Memory.Free(start_addr))
		return CELL_EINVAL;

	return CELL_OK;
}

s32 sys_memory_get_page_attribute(u32 addr, mem_ptr_t<sys_page_attr_t> attr)
{
	sc_mem.Warning("sys_memory_get_page_attribute(addr=0x%x, attr_addr=0x%x)", addr, attr.GetAddr());

	// TODO: Implement per thread page attribute setting.
	attr->attribute = 0;
	attr->page_size = 0;
	attr->access_right = 0;
	attr->pad = 0;

	return CELL_OK;
}

s32 sys_memory_get_user_memory_size(mem_ptr_t<sys_memory_info_t> mem_info)
{
	sc_mem.Warning("sys_memory_get_user_memory_size(mem_info_addr=0x%x)", mem_info.GetAddr());
	
	// Fetch the user memory available.
	mem_info->total_user_memory = Memory.GetUserMemTotalSize();
	mem_info->available_user_memory = Memory.GetUserMemAvailSize();
	return CELL_OK;
}

s32 sys_memory_container_create(mem32_t cid, u32 yield_size)
{
	sc_mem.Warning("sys_memory_container_create(cid_addr=0x%x, yield_size=0x%x)", cid.GetAddr(), yield_size);

	yield_size &= ~0xfffff; //round down to 1 MB granularity
	u64 addr = Memory.Alloc(yield_size, 0x100000); //1 MB alignment

	if(!addr)
		return CELL_ENOMEM;

	// Wrap the allocated memory in a memory container.
	MemoryContainerInfo *ct = new MemoryContainerInfo(addr, yield_size);
	cid = sc_mem.GetNewId(ct, TYPE_MEM);

	sc_mem.Warning("*** memory_container created(addr=0x%llx): id = %d", addr, cid.GetValue());

	return CELL_OK;
}

s32 sys_memory_container_destroy(u32 cid)
{
	sc_mem.Warning("sys_memory_container_destroy(cid=%d)", cid);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if(!sc_mem.CheckId(cid, ct))
		return CELL_ESRCH;

	// Release the allocated memory and remove the ID.
	Memory.Free(ct->addr);
	Emu.GetIdManager().RemoveID(cid);

	return CELL_OK;
}

s32 sys_memory_container_get_size(mem_ptr_t<sys_memory_info_t> mem_info, u32 cid)
{
	sc_mem.Warning("sys_memory_container_get_size(mem_info_addr=0x%x, cid=%d)", mem_info.GetAddr(), cid);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if(!sc_mem.CheckId(cid, ct))
		return CELL_ESRCH;

	// HACK: Return all memory.
	sys_memory_info_t info;
	mem_info->total_user_memory = ct->size;
	mem_info->available_user_memory = ct->size;
	return CELL_OK;
}
