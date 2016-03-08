#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_mmapper.h"

SysCallBase sys_mmapper("sys_mmapper");

lv2_memory_t::lv2_memory_t(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container_t> ct)
	: size(size)
	, align(align)
	, id(idm::get_last_id())
	, flags(flags)
	, ct(ct)
{
}

s32 sys_mmapper_allocate_address(u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr)
{
	sys_mmapper.error("sys_mmapper_allocate_address(size=0x%llx, flags=0x%llx, alignment=0x%llx, alloc_addr=*0x%x)", size, flags, alignment, alloc_addr);

	LV2_LOCK;

	if (size % 0x10000000)
	{
		return CELL_EALIGN;
	}

	if (size > UINT32_MAX)
	{
		return CELL_ENOMEM;
	}

	// This is a 'hack' / workaround for psl1ght, which gives us an alignment of 0, which is technically invalid, 
	//  but apparently is allowed on actual ps3
	//  https://github.com/ps3dev/PSL1GHT/blob/534e58950732c54dc6a553910b653c99ba6e9edc/ppu/librt/sbrk.c#L71 
	if (!alignment)
	{
		alignment = 0x10000000;
	}

	switch (alignment)
	{
	case 0x10000000:
	case 0x20000000:
	case 0x40000000:
	case 0x80000000:
	{
		for (u64 addr = ::align<u64>(0x30000000, alignment); addr < 0xC0000000; addr += alignment)
		{
			if (const auto area = vm::map(static_cast<u32>(addr), static_cast<u32>(size), flags))
			{
				*alloc_addr = addr;

				return CELL_OK;
			}
		}

		return CELL_ENOMEM;
	}
	}

	return CELL_EALIGN;
}

s32 sys_mmapper_allocate_fixed_address()
{
	sys_mmapper.error("sys_mmapper_allocate_fixed_address()");

	LV2_LOCK;

	if (!vm::map(0xB0000000, 0x10000000)) // TODO: set correct flags (they aren't used currently though)
	{
		return CELL_EEXIST;
	}
	
	return CELL_OK;
}

// Allocate physical memory (create lv2_memory_t object)
s32 sys_mmapper_allocate_memory(u64 size, u64 flags, vm::ptr<u32> mem_id)
{
	sys_mmapper.warning("sys_mmapper_allocate_memory(size=0x%llx, flags=0x%llx, mem_id=*0x%x)", size, flags, mem_id);

	LV2_LOCK;

	// Check page granularity
	switch (flags & SYS_MEMORY_PAGE_SIZE_MASK)
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

	if (size > UINT32_MAX)
	{
		return CELL_ENOMEM;
	}

	const u32 align =
		flags & SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 :
		flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 :
		throw EXCEPTION("Unexpected");

	// Generate a new mem ID
	*mem_id = idm::make<lv2_memory_t>(static_cast<u32>(size), align, flags, nullptr);

	return CELL_OK;
}

s32 sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id)
{
	sys_mmapper.error("sys_mmapper_allocate_memory_from_container(size=0x%x, cid=0x%x, flags=0x%llx, mem_id=*0x%x)", size, cid, flags, mem_id);

	LV2_LOCK;

	// Check if this container ID is valid.
	const auto ct = idm::get<lv2_memory_container_t>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	// Check page granularity.
	switch (flags & SYS_MEMORY_PAGE_SIZE_MASK)
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

	if (ct->size - ct->used < size)
	{
		return CELL_ENOMEM;
	}

	const u32 align =
		flags & SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 :
		flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 :
		throw EXCEPTION("Unexpected");

	ct->used += size;

	// Generate a new mem ID
	*mem_id = idm::make<lv2_memory_t>(size, align, flags, ct);

	return CELL_OK;
}

s32 sys_mmapper_change_address_access_right(u32 addr, u64 flags)
{
	sys_mmapper.todo("sys_mmapper_change_address_access_right(addr=0x%x, flags=0x%llx)", addr, flags);

	return CELL_OK;
}

s32 sys_mmapper_free_address(u32 addr)
{
	sys_mmapper.error("sys_mmapper_free_address(addr=0x%x)", addr);

	LV2_LOCK;

	const auto area = vm::get(vm::any, addr);

	if (!area || addr != area->addr)
	{
		return CELL_EINVAL;
	}

	if (area->used)
	{
		return CELL_EBUSY;
	}

	if (!vm::unmap(addr))
	{
		throw EXCEPTION("Unexpected (failed to unmap memory ad 0x%x)", addr);
	}

	return CELL_OK;
}

s32 sys_mmapper_free_memory(u32 mem_id)
{
	sys_mmapper.warning("sys_mmapper_free_memory(mem_id=0x%x)", mem_id);

	LV2_LOCK;

	// Check if this mem ID is valid.
	const auto mem = idm::get<lv2_memory_t>(mem_id);

	if (!mem)
	{
		return CELL_ESRCH;
	}

	if (mem->addr)
	{
		return CELL_EBUSY;
	}

	// Return physical memory to the container if necessary
	if (mem->ct)
	{
		mem->ct->used -= mem->size;
	}

	// Release the allocated memory and remove the ID
	idm::remove<lv2_memory_t>(mem_id);

	return CELL_OK;
}

s32 sys_mmapper_map_memory(u32 addr, u32 mem_id, u64 flags)
{
	sys_mmapper.error("sys_mmapper_map_memory(addr=0x%x, mem_id=0x%x, flags=0x%llx)", addr, mem_id, flags);

	LV2_LOCK;

	const auto area = vm::get(vm::any, addr);

	if (!area || addr < 0x30000000 || addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::get<lv2_memory_t>(mem_id);

	if (!mem)
	{
		return CELL_ESRCH;
	}

	if (addr % mem->align)
	{
		return CELL_EALIGN;
	}

	if (const u32 old_addr = mem->addr)
	{
		sys_mmapper.warning("sys_mmapper_map_memory: Already mapped (mem_id=0x%x, addr=0x%x)", mem_id, old_addr);
		return CELL_OK;
	}

	if (!area->falloc(addr, mem->size))
	{
		return CELL_EBUSY;
	}

	mem->addr = addr;

	return CELL_OK;
}

s32 sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_mmapper.error("sys_mmapper_search_and_map(start_addr=0x%x, mem_id=0x%x, flags=0x%llx, alloc_addr=*0x%x)", start_addr, mem_id, flags, alloc_addr);

	LV2_LOCK;

	const auto area = vm::get(vm::any, start_addr);

	if (!area || start_addr != area->addr || start_addr < 0x30000000 || start_addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::get<lv2_memory_t>(mem_id);

	if (!mem)
	{
		return CELL_ESRCH;
	}

	const u32 addr = area->alloc(mem->size, mem->align);

	if (!addr)
	{
		return CELL_ENOMEM;
	}

	*alloc_addr = addr;

	return CELL_OK;
}

s32 sys_mmapper_unmap_memory(u32 addr, vm::ptr<u32> mem_id)
{
	sys_mmapper.error("sys_mmapper_unmap_memory(addr=0x%x, mem_id=*0x%x)", addr, mem_id);

	LV2_LOCK;

	const auto area = vm::get(vm::any, addr);

	if (!area || addr != area->addr || addr < 0x30000000 || addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	for (auto& mem : idm::get_all<lv2_memory_t>())
	{
		if (mem->addr == addr)
		{
			if (!area->dealloc(addr))
			{
				throw EXCEPTION("Deallocation failed (mem_id=0x%x, addr=0x%x)", mem->id, addr);
			}

			mem->addr = 0;

			*mem_id = mem->id;

			return CELL_OK;
		}
	}

	return CELL_EINVAL;
}

s32 sys_mmapper_enable_page_fault_notification(u32 addr, u32 eq)
{
	sys_mmapper.todo("sys_mmapper_enable_page_fault_notification(addr=0x%x, eq=0x%x)", addr, eq);

	return CELL_OK;
}
