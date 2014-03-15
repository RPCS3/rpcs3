#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Memory.h"

SysCallBase sc_mem("memory");
std::map<u32, u32> mmapper_info_map;

int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr)
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

int sys_memory_allocate_from_container(u32 size, u32 cid, u32 flags, u32 alloc_addr_addr)
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

int sys_memory_free(u32 start_addr)
{
	sc_mem.Log("sys_memory_free(start_addr=0x%x)", start_addr);

	// Release the allocated memory.
	if(!Memory.Free(start_addr))
		return CELL_EFAULT;

	return CELL_OK;
}

int sys_memory_get_page_attribute(u32 addr, mem_ptr_t<sys_page_attr_t> attr)
{
	sc_mem.Warning("sys_memory_get_page_attribute(addr=0x%x, attr_addr=0x%x)", addr, attr.GetAddr());

	if (!attr.IsGood())
		return CELL_EFAULT;

	// TODO: Implement per thread page attribute setting.
	attr->attribute = 0;
	attr->page_size = 0;
	attr->access_right = 0;
	attr->pad = 0;

	return CELL_OK;
}

int sys_memory_get_user_memory_size(u32 mem_info_addr)
{
	sc_mem.Warning("sys_memory_get_user_memory_size(mem_info_addr=0x%x)", mem_info_addr);
	
	// Fetch the user memory available.
	sys_memory_info info;
	info.total_user_memory = re(Memory.GetUserMemTotalSize());
	info.available_user_memory = re(Memory.GetUserMemAvailSize());
	
	Memory.WriteData(mem_info_addr, info);
	
	return CELL_OK;
}

int sys_memory_container_create(mem32_t cid, u32 yield_size)
{
	sc_mem.Warning("sys_memory_container_create(cid_addr=0x%x, yield_size=0x%x)", cid.GetAddr(), yield_size);

	if (!cid.IsGood())
		return CELL_EFAULT;

	yield_size &= ~0xfffff; //round down to 1 MB granularity
	u64 addr = Memory.Alloc(yield_size, 0x100000); //1 MB alignment

	if(!addr)
		return CELL_ENOMEM;

	// Wrap the allocated memory in a memory container.
	MemoryContainerInfo *ct = new MemoryContainerInfo(addr, yield_size);
	cid = sc_mem.GetNewId(ct);

	sc_mem.Warning("*** memory_container created(addr=0x%llx): id = %d", addr, cid.GetValue());

	return CELL_OK;
}

int sys_memory_container_destroy(u32 cid)
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

int sys_memory_container_get_size(u32 mem_info_addr, u32 cid)
{
	sc_mem.Warning("sys_memory_container_get_size(mem_info_addr=0x%x, cid=%d)", mem_info_addr, cid);

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if(!sc_mem.CheckId(cid, ct))
		return CELL_ESRCH;

	// HACK: Return all memory.
	sys_memory_info info;
	info.total_user_memory = re(ct->size);
	info.available_user_memory = re(ct->size);
	
	Memory.WriteData(mem_info_addr, info);

	return CELL_OK;
}

int sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr)
{
	sc_mem.Warning("sys_mmapper_allocate_address(size=0x%x, flags=0x%llx, alignment=0x%x, alloc_addr=0x%x)", 
		size, flags, alignment, alloc_addr);

	if(!Memory.IsGoodAddr(alloc_addr))
		return CELL_EFAULT;

	// Check for valid alignment.
	if(alignment > 0x80000000)
		return CELL_EALIGN;

	// Check page size.
	u32 addr;
	switch(flags & (SYS_MEMORY_PAGE_SIZE_1M | SYS_MEMORY_PAGE_SIZE_64K))
	{
	default:
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(Memory.AlignAddr(size, alignment) & 0xfffff)
			return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(Memory.AlignAddr(size, alignment) & 0xffff)
			return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;
	}

	// Write back the start address of the allocated area.
	Memory.Write32(alloc_addr, addr);

	return CELL_OK;
}

int sys_mmapper_allocate_fixed_address()
{
	sc_mem.Warning("sys_mmapper_allocate_fixed_address");

	// Allocate a fixed size from user memory.
	if (!Memory.Alloc(SYS_MMAPPER_FIXED_SIZE, 0x100000))
		return CELL_EEXIST;
	
	return CELL_OK;
}

int sys_mmapper_allocate_memory(u32 size, u64 flags, mem32_t mem_id)
{
	sc_mem.Warning("sys_mmapper_allocate_memory(size=0x%x, flags=0x%llx, mem_id_addr=0x%x)", size, flags, mem_id.GetAddr());

	if(!mem_id.IsGood())
		return CELL_EFAULT;

	// Check page granularity.
	u32 addr;
	switch(flags & (SYS_MEMORY_PAGE_SIZE_1M | SYS_MEMORY_PAGE_SIZE_64K))
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff)
			return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff)
			return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;

	default:
		return CELL_EINVAL;
	}

	if(!addr)
		return CELL_ENOMEM;

	// Generate a new mem ID.
	mem_id = sc_mem.GetNewId(new mmapper_info(addr, size, flags));

	return CELL_OK;
}

int sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, mem32_t mem_id)
{
	sc_mem.Warning("sys_mmapper_allocate_memory_from_container(size=0x%x, cid=%d, flags=0x%llx, mem_id_addr=0x%x)", 
		size, cid, flags, mem_id.GetAddr());

	if(!mem_id.IsGood())
		return CELL_EFAULT;

	// Check if this container ID is valid.
	MemoryContainerInfo* ct;
	if(!sc_mem.CheckId(cid, ct))
		return CELL_ESRCH;

	// Check page granularity.
	switch(flags & (SYS_MEMORY_PAGE_SIZE_1M | SYS_MEMORY_PAGE_SIZE_64K))
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff)
			return CELL_EALIGN;
		ct->addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff)
			return CELL_EALIGN;
		ct->addr = Memory.Alloc(size, 0x10000);
	break;

	default:
		return CELL_EINVAL;
	}

	if(!ct->addr)
		return CELL_ENOMEM;
	ct->size = size;

	// Generate a new mem ID.
	mem_id = sc_mem.GetNewId(new mmapper_info(ct->addr, ct->size, flags));

	return CELL_OK;
}

int sys_mmapper_change_address_access_right(u32 start_addr, u64 flags)
{
	sc_mem.Warning("sys_mmapper_change_address_access_right(start_addr=0x%x, flags=0x%llx)", start_addr, flags);

	if (!Memory.IsGoodAddr(start_addr))
		return CELL_EINVAL;

	// TODO

	return CELL_OK;
}

int sys_mmapper_free_address(u32 start_addr)
{
	sc_mem.Warning("sys_mmapper_free_address(start_addr=0x%x)", start_addr);

	if(!Memory.IsGoodAddr(start_addr))
		return CELL_EINVAL;
	
	// Free the address.
	Memory.Free(start_addr);
	return CELL_OK;
}

int sys_mmapper_free_memory(u32 mem_id)
{
	sc_mem.Warning("sys_mmapper_free_memory(mem_id=0x%x)", mem_id);

	// Check if this mem ID is valid.
	mmapper_info* info;
	if(!sc_mem.CheckId(mem_id, info))
		return CELL_ESRCH;

	// Release the allocated memory and remove the ID.
	Memory.Free(info->addr);
	Emu.GetIdManager().RemoveID(mem_id);

	return CELL_OK;
}

int sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags)
{
	sc_mem.Warning("sys_mmapper_map_memory(start_addr=0x%x, mem_id=0x%x, flags=0x%llx)", start_addr, mem_id, flags);

	// Check if this mem ID is valid.
	mmapper_info* info;
	if(!sc_mem.CheckId(mem_id, info))
		return CELL_ESRCH;

	// Map the memory into the process address.
	if(!Memory.Map(start_addr, info->addr, info->size))
		sc_mem.Error("sys_mmapper_map_memory failed!");

	// Keep track of mapped addresses.
	mmapper_info_map[mem_id] = start_addr;

	return CELL_OK;
}

int sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, u32 alloc_addr)
{
	sc_mem.Warning("sys_mmapper_search_and_map(start_addr=0x%x, mem_id=0x%x, flags=0x%llx, alloc_addr=0x%x)",
		start_addr, mem_id, flags, alloc_addr);

	if(!Memory.IsGoodAddr(alloc_addr))
		return CELL_EFAULT;

	// Check if this mem ID is valid.
	mmapper_info* info;
	if(!sc_mem.CheckId(mem_id, info))
		return CELL_ESRCH;
	
	// Search for a mappable address.
	u32 addr;
	bool found;
	for (int i = 0; i < SYS_MMAPPER_FIXED_SIZE; i += 0x100000)
	{
		addr = start_addr + i;
		found = Memory.Map(addr, info->addr, info->size);
		if(found)
		{
			sc_mem.Warning("Found and mapped address 0x%x", addr);
			break;
		}
	}

	// Check if the address is valid.
	if (!Memory.IsGoodAddr(addr) || !found)
		return CELL_ENOMEM;
	
	// Write back the start address of the allocated area.
	Memory.Write32(alloc_addr, addr);

	// Keep track of mapped addresses.
	mmapper_info_map[mem_id] = addr;

	return CELL_OK;
}

int sys_mmapper_unmap_memory(u32 start_addr, u32 mem_id_addr)
{
	sc_mem.Warning("sys_mmapper_unmap_memory(start_addr=0x%x, mem_id_addr=0x%x)", start_addr, mem_id_addr);

	if (!Memory.IsGoodAddr(start_addr))
		return CELL_EINVAL;

	if (!Memory.IsGoodAddr(mem_id_addr))
		return CELL_EFAULT;

	// Write back the mem ID of the unmapped area.
	u32 mem_id = mmapper_info_map.find(start_addr)->first;
	Memory.Write32(mem_id_addr, mem_id);

	return CELL_OK;
}

int sys_mmapper_enable_page_fault_notification(u32 start_addr, u32 q_id)
{
	sc_mem.Warning("sys_mmapper_enable_page_fault_notification(start_addr=0x%x, q_id=0x%x)", start_addr, q_id);

	if (!Memory.IsGoodAddr(start_addr))
		return CELL_EINVAL;

	// TODO

	return CELL_OK;
}
