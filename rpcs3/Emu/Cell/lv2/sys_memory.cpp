#include "stdafx.h"
#include "Utilities/VirtualMemory.h"
#include "Emu/IdManager.h"
#include "sys_memory.h"

LOG_CHANNEL(sys_memory);

lv2_memory_alloca::lv2_memory_alloca(u32 size, u32 align, u64 flags, const std::shared_ptr<lv2_memory_container>& ct)
	: size(size)
	, align(align)
	, flags(flags)
	, ct(ct)
	, shm(std::make_shared<utils::shm>(size))
{
}

// Todo: fix order of error checks

error_code sys_memory_allocate(u32 size, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.warning("sys_memory_allocate(size=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, flags, alloc_addr);

	// Check allocation size
	const u32 align =
		flags == SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 :
		flags == SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 :
		flags == 0 ? 0x100000 : 0;

	if (!align)
	{
		return {CELL_EINVAL, flags};
	}

	if (size % align)
	{
		return {CELL_EALIGN, size};
	}

	// Get "default" memory container
	const auto dct = fxm::get_always<lv2_memory_container>();

	// Try to get "physical memory"
	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	if (!alloc_addr)
	{
		dct->used -= size;
		return CELL_EFAULT;
	}

	// Allocate memory, write back the start address of the allocated area
	*alloc_addr = verify(HERE, vm::alloc(size, align == 0x10000 ? vm::user64k : vm::user1m, align));

	return CELL_OK;
}

error_code sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr)
{
	sys_memory.warning("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, cid, flags, alloc_addr);

	// Check allocation size
	const u32 align =
		flags == SYS_MEMORY_PAGE_SIZE_1M ? 0x100000 :
		flags == SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 :
		flags == 0 ? 0x100000 : 0;

	if (!align)
	{
		return {CELL_EINVAL, flags};
	}

	if (size % align)
	{
		return {CELL_EALIGN, size};
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

	if (!alloc_addr)
	{
		ct->used -= size;
		return CELL_EFAULT;
	}

	// Create phantom memory object
	const auto mem = idm::make_ptr<lv2_memory_alloca>(size, align, flags, ct.ptr);

	// Allocate memory
	*alloc_addr = verify(HERE, vm::get(align == 0x10000 ? vm::user64k : vm::user1m)->alloc(size, mem->align, &mem->shm));

	return CELL_OK;
}

error_code sys_memory_free(u32 addr)
{
	sys_memory.warning("sys_memory_free(addr=0x%x)", addr);

	const auto area = vm::get(vm::any, addr);

	if (!area || (area->flags & 3) != 1)
	{
		return {CELL_EINVAL, addr};
	}

	const auto shm = area->get(addr);

	if (!shm.second)
	{
		return {CELL_EINVAL, addr};
	}

	// Retrieve phantom memory object
	const auto mem = idm::select<lv2_memory_alloca>([&](u32 id, lv2_memory_alloca& mem) -> u32
	{
		if (mem.shm.get() == shm.second.get())
		{
			return id;
		}

		return 0;
	});

	if (!mem)
	{
		// Deallocate memory (simple)
		if (!area->dealloc(addr))
		{
			return {CELL_EINVAL, addr};
		}

		// Return "physical memory" to the default container
		fxm::get_always<lv2_memory_container>()->used -= shm.second->size();

		return CELL_OK;
	}

	// Deallocate memory
	if (!area->dealloc(addr, &shm.second))
	{
		return {CELL_EINVAL, addr};
	}

	// Return "physical memory"
	mem->ct->used -= mem->size;

	// Remove phantom memory object
	verify(HERE), idm::remove<lv2_memory_alloca>(mem.ret);

	return CELL_OK;
}

error_code sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	sys_memory.trace("sys_memory_get_page_attribute(addr=0x%x, attr=*0x%x)", addr, attr);

	if (!vm::check_addr(addr))
	{
		return CELL_EINVAL;
	}

	if (!vm::check_addr(attr.addr(), attr.size()))
	{
		return CELL_EFAULT;
	}

	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE (TODO)
	attr->access_right = 0xFull; // SYS_MEMORY_ACCESS_RIGHT_ANY (TODO)

	if (vm::check_addr(addr, 1, vm::page_1m_size))
	{
		attr->page_size = 0x100000;
	}
	else if (vm::check_addr(addr, 1, vm::page_64k_size))
	{
		attr->page_size = 0x10000;
	}
	else
	{
		attr->page_size = 4096;
	}

	return CELL_OK;
}

error_code sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info)
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

error_code sys_memory_container_create(vm::ptr<u32> cid, u32 size)
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

error_code sys_memory_container_destroy(u32 cid)
{
	sys_memory.warning("sys_memory_container_destroy(cid=0x%x)", cid);

	const auto ct = idm::withdraw<lv2_memory_container>(cid, [](lv2_memory_container& ct) -> CellError
	{
		// Check if some memory is not deallocated (the container cannot be destroyed in this case)
		if (!ct.used.compare_and_swap_test(0, ct.size))
		{
			return CELL_EBUSY;
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

	// Return "physical memory" to the default container
	fxm::get<lv2_memory_container>()->used -= ct->size;

	return CELL_OK;
}

error_code sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid)
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
