#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Memory.h"

SysCallBase sc_mem("memory");

int sys_memory_container_create(u32 cid_addr, u32 yield_size)
{
	sc_mem.Warning("sys_memory_container_create(cid_addr=0x%x,yield_size=0x%x)", cid_addr, yield_size);

	if(!Memory.IsGoodAddr(cid_addr, 4))
	{
		return CELL_EFAULT;
	}

	yield_size &= ~0xfffff; //round down to 1 MB granularity

	u64 addr = Memory.Alloc(yield_size, 0x100000); //1 MB alignment

	if(!addr)
	{
		return CELL_ENOMEM;
	}

	Memory.Write32(cid_addr, sc_mem.GetNewId(new MemoryContainerInfo(addr, yield_size)));
	return CELL_OK;
}

int sys_memory_container_destroy(u32 cid)
{
	sc_mem.Warning("sys_memory_container_destroy(cid=0x%x)", cid);

	MemoryContainerInfo* ct;

	if(!sc_mem.CheckId(cid, ct))
	{
		return CELL_ESRCH;
	}

	Memory.Free(ct->addr);
	Emu.GetIdManager().RemoveID(cid);
	return CELL_OK;
}

int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr)
{
	//0x30000100;
	sc_mem.Warning("sys_memory_allocate(size=0x%x, flags=0x%x)", size, flags);
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

	if(!addr) return CELL_ENOMEM;
	sc_mem.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", addr, size);
	Memory.Write32(alloc_addr_addr, addr);

	return CELL_OK;
}

int sys_memory_free(u32 start_addr)
{
	sc_mem.Log("sys_memory_free(start_addr=0x%x)", start_addr);

	if(!Memory.Free(start_addr)) return CELL_EFAULT;

	return CELL_OK;
}

int sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr)
{
	sc_mem.Warning("sys_mmapper_allocate_address(size=0x%x, flags=0x%llx, alignment=0x%x, alloc_addr=0x%x)", size, flags, alignment, alloc_addr);

	if(!Memory.IsGoodAddr(alloc_addr)) return CELL_EFAULT;

	if(!alignment)
		alignment = 1;

	u32 addr;

	switch(flags & (SYS_MEMORY_PAGE_SIZE_1M | SYS_MEMORY_PAGE_SIZE_64K))
	{
	default:
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(Memory.AlignAddr(size, alignment) & 0xfffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(Memory.AlignAddr(size, alignment) & 0xffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;
	}

	Memory.Write32(alloc_addr, addr);

	return CELL_OK;
}

int sys_mmapper_allocate_memory(u32 size, u64 flags, u32 mem_id_addr)
{
	sc_mem.Warning("sys_mmapper_allocate_memory(size=0x%x, flags=0x%llx, mem_id_addr=0x%x)", size, flags, mem_id_addr);

	if(!Memory.IsGoodAddr(mem_id_addr)) return CELL_EFAULT;

	u32 addr;
	switch(flags & (SYS_MEMORY_PAGE_SIZE_1M | SYS_MEMORY_PAGE_SIZE_64K))
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;

	default:
		return CELL_EINVAL;
	}

	if(!addr)
		return CELL_ENOMEM;

	Memory.Write32(mem_id_addr, sc_mem.GetNewId(new mmapper_info(addr, size, flags)));

	return CELL_OK;
}

int sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags)
{
	sc_mem.Warning("sys_mmapper_map_memory(start_addr=0x%x, mem_id=0x%x, flags=0x%llx)", start_addr, mem_id, flags);

	mmapper_info* info;
	if(!sc_mem.CheckId(mem_id, info)) return CELL_ESRCH;

	if(!Memory.Map(start_addr, info->addr, info->size))
	{
		sc_mem.Error("sys_mmapper_map_memory failed!");
	}

	return CELL_OK;
}

int sys_memory_get_user_memory_size(u32 mem_info_addr)
{
	sc_mem.Warning("sys_memory_get_user_memory_size(mem_info_addr=0x%x)", mem_info_addr);
	sys_memory_info info;
	info.total_user_memory = re(Memory.GetUserMemTotalSize());
	info.available_user_memory = re(Memory.GetUserMemAvailSize());
	Memory.WriteData(mem_info_addr, info);
	return CELL_OK;
}

int sys_memory_get_page_attribute(u32 addr, mem_ptr_t<sys_page_attr_t> attr)
{
	sc_mem.Warning("sys_memory_get_page_attribute(addr=0x%x, attr_addr=0x%x)", addr, attr.GetAddr());

	if (!attr.IsGood())
		return CELL_EFAULT;

	attr->attribute = 0;
	attr->page_size = 0;
	attr->access_right = 0;
	attr->pad = 0;

	return CELL_OK;
}