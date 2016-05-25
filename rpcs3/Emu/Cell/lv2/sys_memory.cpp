#include "stdafx.h"
#include "sys_memory.h"

namespace vm { using namespace ps3; }

logs::channel sys_memory("sys_memory", logs::level::notice);

ppu_error_code sys_memory_allocate(u32 size, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.warning("sys_memory_allocate(size=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, flags, alloc_addr);

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

	// Get "default" memory container
	const auto dct = fxm::get_always<lv2_memory_container>();

	// Try to get "physical memory"
	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	// Allocate memory, write back the start address of the allocated area
	VERIFY(*alloc_addr = vm::alloc(size, vm::user_space, flags == SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 : 0x10000));

	return CELL_OK;
}

ppu_error_code sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.warning("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, cid, flags, alloc_addr);
	
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

	ppu_error_code result{};

	const auto ct = idm::get<lv2_memory_container>(cid, [&](u32, lv2_memory_container& ct)
	{
		// Try to get "physical memory"
		if (!ct.take(size))
		{
			result = CELL_ENOMEM;
			return_ false;
		}

		return_ true;
	});

	if (!ct && !result)
	{
		return CELL_ESRCH;
	}

	if (!ct)
	{
		return result;
	}

	// Allocate memory, write back the start address of the allocated area, use cid as the supplementary info
	VERIFY(*alloc_addr = vm::alloc(size, vm::user_space, flags == SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 : 0x10000, cid));

	return CELL_OK;
}

ppu_error_code sys_memory_free(u32 addr)
{
	sys_memory.warning("sys_memory_free(addr=0x%x)", addr);

	const auto area = vm::get(vm::user_space);

	VERIFY(area);

	// Deallocate memory
	u32 cid, size = area->dealloc(addr, &cid);

	if (!size)
	{
		return CELL_EINVAL;
	}

	// Return "physical memory"
	if (cid == 0)
	{
		fxm::get<lv2_memory_container>()->used -= size;
	}
	else if (const auto ct = idm::get<lv2_memory_container>(cid))
	{
		ct->used -= size;
	}

	return CELL_OK;
}

ppu_error_code sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	sys_memory.error("sys_memory_get_page_attribute(addr=0x%x, attr=*0x%x)", addr, attr);

	// TODO: Implement per thread page attribute setting.
	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE
	attr->access_right = 0xFull; // SYS_MEMORY_ACCESS_RIGHT_ANY
	attr->page_size = 4096;

	return CELL_OK;
}

ppu_error_code sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info)
{
	sys_memory.warning("sys_memory_get_user_memory_size(mem_info=*0x%x)", mem_info);

	// Get "default" memory container
	const auto dct = fxm::get_always<lv2_memory_container>();

	mem_info->total_user_memory = dct->size;
	mem_info->available_user_memory = dct->size - dct->used;

	// Scan other memory containers
	idm::select<lv2_memory_container>([&](u32, lv2_memory_container& ct)
	{
		mem_info->total_user_memory -= ct.size;
	});

	return CELL_OK;
}

ppu_error_code sys_memory_container_create(vm::ptr<u32> cid, u32 size)
{
	sys_memory.warning("sys_memory_container_create(cid=*0x%x, size=0x%x)", cid, size);

	// Round down to 1 MB granularity
	size &= ~0xfffff;

	if (!size)
	{
		return CELL_ENOMEM;
	}

	const auto dct = fxm::get_always<lv2_memory_container>();

	// Try to obtain "physical memory" from the default container
	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	// Create the memory container
	*cid = idm::make<lv2_memory_container>(size);

	return CELL_OK;
}

ppu_error_code sys_memory_container_destroy(u32 cid)
{
	sys_memory.warning("sys_memory_container_destroy(cid=0x%x)", cid);

	ppu_error_code result{};

	const auto ct = idm::withdraw<lv2_memory_container>(cid, [&](u32, lv2_memory_container& ct)
	{
		// Check if some memory is not deallocated (the container cannot be destroyed in this case)
		if (!ct.used.compare_and_swap_test(0, ct.size))
		{
			result = CELL_EBUSY;
			return_ false;
		}

		return_ true;
	});

	if (!ct && !result)
	{
		return CELL_ESRCH;
	}

	if (!ct)
	{
		return result;
	}

	// Return "physical memory" to the default container
	fxm::get<lv2_memory_container>()->used -= ct->size;

	return CELL_OK;
}

ppu_error_code sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid)
{
	sys_memory.warning("sys_memory_container_get_size(mem_info=*0x%x, cid=0x%x)", mem_info, cid);

	const auto ct = idm::get<lv2_memory_container>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	mem_info->total_user_memory = ct->size; // Total container memory
	mem_info->available_user_memory = ct->size - ct->used; // Available container memory

	return CELL_OK;
}
