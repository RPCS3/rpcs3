#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_memory.h"

SysCallBase sys_memory("sys_memory");

lv2_memory_container_t::lv2_memory_container_t(u32 size)
	: size(size)
	, id(Emu.GetIdManager().get_current_id())
{
}

s32 sys_memory_allocate(u32 size, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.Warning("sys_memory_allocate(size=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, flags, alloc_addr);

	LV2_LOCK;

	// Check allocation size
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
	{
		if (size % 0x100000)
		{
			return CELL_EALIGN;
		}

		break;
	}
		
	case SYS_MEMORY_PAGE_SIZE_64K:
	{
		if (size % 0x10000)
		{
			return CELL_EALIGN;
		}

		break;
	}

	default:
	{
		return CELL_EINVAL;
	}
	}

	// Available memory reserved for containers
	u32 available = 0;

	// Check all containers
	for (auto& ct : Emu.GetIdManager().get_all<lv2_memory_container_t>())
	{
		available += ct->size - ct->used;
	}

	const auto area = vm::get(vm::user_space);

	// Check available memory
	if (area->size < area->used.load() + available + size)
	{
		return CELL_ENOMEM;
	}

	// Allocate memory
	const u32 addr =
		flags == SYS_MEMORY_PAGE_SIZE_1M ? area->alloc(size, 0x100000) :
		flags == SYS_MEMORY_PAGE_SIZE_64K ? area->alloc(size, 0x10000) :
		throw EXCEPTION("Unexpected flags");

	if (!addr)
	{
		return CELL_ENOMEM;
	}
	
	// Write back the start address of the allocated area
	*alloc_addr = addr;

	return CELL_OK;
}

s32 sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.Warning("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, cid, flags, alloc_addr);

	LV2_LOCK;

	// Check if this container ID is valid
	const auto ct = Emu.GetIdManager().get<lv2_memory_container_t>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}
	
	// Check allocation size
	switch (flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
	{
		if (size % 0x100000)
		{
			return CELL_EALIGN;
		}

		break;
	}

	case SYS_MEMORY_PAGE_SIZE_64K:
	{
		if (size % 0x10000)
		{
			return CELL_EALIGN;
		}

		break;
	}

	default:
	{
		return CELL_EINVAL;
	}
	}

	if (ct->used > ct->size)
	{
		throw EXCEPTION("Unexpected amount of memory taken (0x%x, size=0x%x)", ct->used.load(), ct->size);
	}

	// Check memory availability
	if (size > ct->size - ct->used)
	{
		return CELL_ENOMEM;
	}

	// Allocate memory
	const u32 addr =
		flags == SYS_MEMORY_PAGE_SIZE_1M ? vm::alloc(size, vm::user_space, 0x100000) :
		flags == SYS_MEMORY_PAGE_SIZE_64K ? vm::alloc(size, vm::user_space, 0x10000) :
		throw EXCEPTION("Unexpected flags");

	if (!addr)
	{
		throw EXCEPTION("Memory not allocated (ct=0x%x, size=0x%x)", cid, size);
	}

	// Store the address and size in the container
	ct->allocs.emplace(addr, size);
	ct->used += size;

	// Write back the start address of the allocated area.
	*alloc_addr = addr;

	return CELL_OK;
}

s32 sys_memory_free(u32 addr)
{
	sys_memory.Warning("sys_memory_free(addr=0x%x)", addr);

	LV2_LOCK;

	// Check all memory containers
	for (auto& ct : Emu.GetIdManager().get_all<lv2_memory_container_t>())
	{
		auto found = ct->allocs.find(addr);

		if (found != ct->allocs.end())
		{
			if (!vm::dealloc(addr, vm::user_space))
			{
				throw EXCEPTION("Memory not deallocated (cid=0x%x, addr=0x%x, size=0x%x)", ct->id, addr, found->second);
			}

			// Return memory size
			ct->used -= found->second;
			ct->allocs.erase(found);

			return CELL_OK;
		}
	}

	if (!vm::dealloc(addr, vm::user_space))
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	sys_memory.Error("sys_memory_get_page_attribute(addr=0x%x, attr=*0x%x)", addr, attr);

	LV2_LOCK;

	// TODO: Implement per thread page attribute setting.
	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE
	attr->access_right = 0xFull; // SYS_MEMORY_ACCESS_RIGHT_ANY
	attr->page_size = 4096;

	return CELL_OK;
}

s32 sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info)
{
	sys_memory.Warning("sys_memory_get_user_memory_size(mem_info=*0x%x)", mem_info);

	LV2_LOCK;

	u32 reserved = 0;
	u32 available = 0;

	// Check all memory containers
	for (auto& ct : Emu.GetIdManager().get_all<lv2_memory_container_t>())
	{
		reserved += ct->size;
		available += ct->size - ct->used;
	}

	const auto area = vm::get(vm::user_space);
	
	// Fetch the user memory available
	mem_info->total_user_memory = area->size - reserved;
	mem_info->available_user_memory = area->size - area->used.load() - available;

	return CELL_OK;
}

s32 sys_memory_container_create(vm::ptr<u32> cid, u32 size)
{
	sys_memory.Warning("sys_memory_container_create(cid=*0x%x, size=0x%x)", cid, size);

	LV2_LOCK;

	// Round down to 1 MB granularity
	size &= ~0xfffff;

	if (!size)
	{
		return CELL_ENOMEM;
	}

	u32 reserved = 0;
	u32 available = 0;

	// Check all memory containers
	for (auto& ct : Emu.GetIdManager().get_all<lv2_memory_container_t>())
	{
		reserved += ct->size;
		available += ct->size - ct->used;
	}

	const auto area = vm::get(vm::user_space);

	if (area->size < reserved + size ||
		area->size - area->used.load() < available + size)
	{
		return CELL_ENOMEM;
	}

	// Create the memory container
	*cid = Emu.GetIdManager().make<lv2_memory_container_t>(size);

	return CELL_OK;
}

s32 sys_memory_container_destroy(u32 cid)
{
	sys_memory.Warning("sys_memory_container_destroy(cid=0x%x)", cid);

	LV2_LOCK;

	const auto ct = Emu.GetIdManager().get<lv2_memory_container_t>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	// Check if some memory is not deallocated (the container cannot be destroyed in this case)
	if (ct->used.load())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_memory_container_t>(cid);

	return CELL_OK;
}

s32 sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid)
{
	sys_memory.Warning("sys_memory_container_get_size(mem_info=*0x%x, cid=0x%x)", mem_info, cid);

	LV2_LOCK;

	const auto ct = Emu.GetIdManager().get<lv2_memory_container_t>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	mem_info->total_user_memory = ct->size; // total container memory
	mem_info->available_user_memory = ct->size - ct->used.load(); // available container memory

	return CELL_OK;
}
