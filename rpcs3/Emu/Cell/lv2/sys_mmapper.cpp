#include "stdafx.h"
#include "sys_mmapper.h"

namespace vm { using namespace ps3; }

logs::channel sys_mmapper("sys_mmapper");

error_code sys_mmapper_allocate_address(u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr)
{
	sys_mmapper.error("sys_mmapper_allocate_address(size=0x%llx, flags=0x%llx, alignment=0x%llx, alloc_addr=*0x%x)", size, flags, alignment, alloc_addr);

	if (size % 0x10000000)
	{
		return CELL_EALIGN;
	}

	if (size > UINT32_MAX)
	{
		return CELL_ENOMEM;
	}

	// This is a workaround for psl1ght, which gives us an alignment of 0, which is technically invalid, but apparently is allowed on actual ps3
	// https://github.com/ps3dev/PSL1GHT/blob/534e58950732c54dc6a553910b653c99ba6e9edc/ppu/librt/sbrk.c#L71 
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
				*alloc_addr = static_cast<u32>(addr);
				return CELL_OK;
			}
		}

		return CELL_ENOMEM;
	}
	}

	return CELL_EALIGN;
}

error_code sys_mmapper_allocate_fixed_address()
{
	sys_mmapper.error("sys_mmapper_allocate_fixed_address()");

	if (!vm::map(0xB0000000, 0x10000000, SYS_MEMORY_PAGE_SIZE_1M))
	{
		return CELL_EEXIST;
	}
	
	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory(u64 unk, u32 size, u64 flags, vm::ptr<u32> mem_id)
{
	sys_mmapper.warning("sys_mmapper_allocate_shared_memory(0x%llx, size=0x%x, flags=0x%llx, mem_id=*0x%x)", unk, size, flags, mem_id);

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

	// Get "default" memory container
	const auto dct = fxm::get_always<lv2_memory_container>();

	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	// Generate a new mem ID
	*mem_id = idm::make<lv2_obj, lv2_memory>(size, flags & SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 : 0x10000, flags, dct);

	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory_from_container(u64 unk, u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id)
{
	sys_mmapper.error("sys_mmapper_allocate_shared_memory_from_container(0x%llx, size=0x%x, cid=0x%x, flags=0x%llx, mem_id=*0x%x)", unk, size, cid, flags, mem_id);

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

	const auto ct = idm::get<lv2_memory_container>(cid, [&](lv2_memory_container& ct) -> CellError
	{
		// Try to get "physical memory"
		if (!ct.take(size))
		{
			return CELL_ENOMEM;
		}

		return {};
	});

	if (!ct)
	{
		return CELL_ESRCH;
	}

	if (ct.ret)
	{
		return ct.ret;
	}

	// Generate a new mem ID
	*mem_id = idm::make<lv2_obj, lv2_memory>(size, flags & SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 : 0x10000, flags, ct.ptr);

	return CELL_OK;
}

error_code sys_mmapper_change_address_access_right(u32 addr, u64 flags)
{
	sys_mmapper.todo("sys_mmapper_change_address_access_right(addr=0x%x, flags=0x%llx)", addr, flags);

	return CELL_OK;
}

error_code sys_mmapper_free_address(u32 addr)
{
	sys_mmapper.error("sys_mmapper_free_address(addr=0x%x)", addr);

	// Try to unmap area
	const auto area = vm::unmap(addr, true);

	if (!area)
	{
		return CELL_EINVAL;
	}

	if (!area.unique())
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

error_code sys_mmapper_free_shared_memory(u32 mem_id)
{
	sys_mmapper.warning("sys_mmapper_free_shared_memory(mem_id=0x%x)", mem_id);

	// Conditionally remove memory ID
	const auto mem = idm::withdraw<lv2_obj, lv2_memory>(mem_id, [&](lv2_memory& mem) -> CellError
	{
		if (!mem.addr.compare_and_swap_test(0, -1))
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!mem)
	{
		return CELL_ESRCH;
	}

	if (mem.ret)
	{
		return mem.ret;
	}

	// Return "physical memory" to the memory container
	mem->ct->used -= mem->size;

	return CELL_OK;
}

error_code sys_mmapper_map_shared_memory(u32 addr, u32 mem_id, u64 flags)
{
	sys_mmapper.warning("sys_mmapper_map_shared_memory(addr=0x%x, mem_id=0x%x, flags=0x%llx)", addr, mem_id, flags);

	const auto area = vm::get(vm::any, addr);

	if (!area || addr < 0x30000000 || addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::get<lv2_obj, lv2_memory>(mem_id);

	if (!mem)
	{
		return CELL_ESRCH;
	}

	if (addr % mem->align)
	{
		return CELL_EALIGN;
	}

	if (const u32 old_addr = mem->addr.compare_and_swap(0, -1))
	{
		sys_mmapper.warning("sys_mmapper_map_shared_memory(): Already mapped (mem_id=0x%x, addr=0x%x)", mem_id, old_addr);
		return CELL_OK;
	}

	if (!area->falloc(addr, mem->size))
	{
		mem->addr = 0;
		return CELL_EBUSY;
	}

	mem->addr = addr;
	return CELL_OK;
}

error_code sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_mmapper.warning("sys_mmapper_search_and_map(start_addr=0x%x, mem_id=0x%x, flags=0x%llx, alloc_addr=*0x%x)", start_addr, mem_id, flags, alloc_addr);

	const auto area = vm::get(vm::any, start_addr);

	if (!area || start_addr < 0x30000000 || start_addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::get<lv2_obj, lv2_memory>(mem_id);

	if (!mem)
	{
		return CELL_ESRCH;
	}

	if (const u32 old_addr = mem->addr.compare_and_swap(0, -1))
	{
		sys_mmapper.warning("sys_mmapper_search_and_map(): Already mapped (mem_id=0x%x, addr=0x%x)", mem_id, old_addr);
		return CELL_OK;
	}

	const u32 addr = area->alloc(mem->size, mem->align);

	if (!addr)
	{
		mem->addr = 0;
		return CELL_ENOMEM;
	}

	*alloc_addr = mem->addr = addr;
	return CELL_OK;
}

error_code sys_mmapper_unmap_shared_memory(u32 addr, vm::ptr<u32> mem_id)
{
	sys_mmapper.warning("sys_mmapper_unmap_shared_memory(addr=0x%x, mem_id=*0x%x)", addr, mem_id);

	const auto area = vm::get(vm::any, addr);

	if (!area || addr < 0x30000000 || addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::select<lv2_obj, lv2_memory>([&](u32 id, lv2_memory& mem)
	{
		if (mem.addr == addr)
		{
			*mem_id = id;
			return true;
		}
		
		return false;
	});

	if (!mem)
	{
		return CELL_EINVAL;
	}

	verify(HERE), area->dealloc(addr), mem->addr.exchange(0) == addr;
	return CELL_OK;
}

error_code sys_mmapper_enable_page_fault_notification(u32 addr, u32 eq)
{
	sys_mmapper.todo("sys_mmapper_enable_page_fault_notification(addr=0x%x, eq=0x%x)", addr, eq);

	return CELL_OK;
}
