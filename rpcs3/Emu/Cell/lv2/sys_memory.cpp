#include "stdafx.h"
#include "sys_memory.h"

#include "Utilities/VirtualMemory.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/IdManager.h"
#include <shared_mutex>

LOG_CHANNEL(sys_memory);

//
static shared_mutex s_memstats_mtx;

struct sys_memory_address_table
{
	atomic_t<lv2_memory_container*> addrs[65536]{};
};

// Todo: fix order of error checks

error_code sys_memory_allocate(cpu_thread& cpu, u32 size, u64 flags, vm::ptr<u32> alloc_addr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_allocate(size=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, flags, alloc_addr);

	if (!size)
	{
		return {CELL_EALIGN, size};
	}

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
	const auto dct = g_fxo->get<lv2_memory_container>();

	// Try to get "physical memory"
	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	if (const auto area = vm::reserve_map(align == 0x10000 ? vm::user64k : vm::user1m, 0, ::align(size, 0x10000000), 0x401))
	{
		if (u32 addr = area->alloc(size, align))
		{
			verify(HERE), !g_fxo->get<sys_memory_address_table>()->addrs[addr >> 16].exchange(dct);

			if (alloc_addr)
			{
				*alloc_addr = addr;
				return CELL_OK;
			}

			// Dealloc using the syscall
			sys_memory_free(cpu, addr);
			return CELL_EFAULT;
		}
	}

	dct->used -= size;
	return CELL_ENOMEM;
}

error_code sys_memory_allocate_from_container(cpu_thread& cpu, u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_allocate_from_container(size=0x%x, cid=0x%x, flags=0x%llx, alloc_addr=*0x%x)", size, cid, flags, alloc_addr);

	if (!size)
	{
		return {CELL_EALIGN, size};
	}

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

	if (const auto area = vm::reserve_map(align == 0x10000 ? vm::user64k : vm::user1m, 0, ::align(size, 0x10000000), 0x401))
	{
		if (u32 addr = area->alloc(size))
		{
			verify(HERE), !g_fxo->get<sys_memory_address_table>()->addrs[addr >> 16].exchange(ct.ptr.get());

			if (alloc_addr)
			{
				*alloc_addr = addr;
				return CELL_OK;
			}

			// Dealloc using the syscall
			sys_memory_free(cpu, addr);
			return CELL_EFAULT;
		}
	}

	ct->used -= size;
	return CELL_ENOMEM;
}

error_code sys_memory_free(cpu_thread& cpu, u32 addr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_free(addr=0x%x)", addr);

	const auto ct = addr % 0x10000 ? nullptr : g_fxo->get<sys_memory_address_table>()->addrs[addr >> 16].exchange(nullptr);

	if (!ct)
	{
		return {CELL_EINVAL, addr};
	}

	const auto size = verify(HERE, vm::dealloc(addr));
	std::shared_lock{id_manager::g_mutex}, ct->used -= size;
	return CELL_OK;
}

error_code sys_memory_get_page_attribute(cpu_thread& cpu, u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.trace("sys_memory_get_page_attribute(addr=0x%x, attr=*0x%x)", addr, attr);

	vm::reader_lock rlock;

	if (!vm::check_addr(addr))
	{
		return CELL_EINVAL;
	}

	if (!vm::check_addr(attr.addr(), attr.size()))
	{
		return CELL_EFAULT;
	}

	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE (TODO)
	attr->access_right = addr >> 28 == 0xdu ? SYS_MEMORY_ACCESS_RIGHT_PPU_THR : SYS_MEMORY_ACCESS_RIGHT_ANY;// (TODO)

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

	attr->pad = 0; // Always write 0
	return CELL_OK;
}

error_code sys_memory_get_user_memory_size(cpu_thread& cpu, vm::ptr<sys_memory_info_t> mem_info)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_get_user_memory_size(mem_info=*0x%x)", mem_info);

	// Get "default" memory container
	const auto dct = g_fxo->get<lv2_memory_container>();

	::reader_lock lock(s_memstats_mtx);

	mem_info->total_user_memory = dct->size;
	mem_info->available_user_memory = dct->size - dct->used;

	// Scan other memory containers
	idm::select<lv2_memory_container>([&](u32, lv2_memory_container& ct)
	{
		mem_info->total_user_memory -= ct.size;
	});

	return CELL_OK;
}

error_code sys_memory_get_user_memory_stat(cpu_thread& cpu, vm::ptr<sys_memory_user_memory_stat_t> mem_stat)
{
	cpu.state += cpu_flag::wait;

	sys_memory.todo("sys_memory_get_user_memory_stat(mem_stat=*0x%x)", mem_stat);

	return CELL_OK;
}

error_code sys_memory_container_create(cpu_thread& cpu, vm::ptr<u32> cid, u32 size)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_container_create(cid=*0x%x, size=0x%x)", cid, size);

	// Round down to 1 MB granularity
	size &= ~0xfffff;

	if (!size)
	{
		return CELL_ENOMEM;
	}

	const auto dct = g_fxo->get<lv2_memory_container>();

	std::lock_guard lock(s_memstats_mtx);

	// Try to obtain "physical memory" from the default container
	if (!dct->take(size))
	{
		return CELL_ENOMEM;
	}

	// Create the memory container
	if (const u32 id = idm::make<lv2_memory_container>(size))
	{
		*cid = id;
		return CELL_OK;
	}

	dct->used -= size;
	return CELL_EAGAIN;
}

error_code sys_memory_container_destroy(cpu_thread& cpu, u32 cid)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_container_destroy(cid=0x%x)", cid);

	std::lock_guard lock(s_memstats_mtx);

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
	g_fxo->get<lv2_memory_container>()->used -= ct->size;

	return CELL_OK;
}

error_code sys_memory_container_get_size(cpu_thread& cpu, vm::ptr<sys_memory_info_t> mem_info, u32 cid)
{
	cpu.state += cpu_flag::wait;

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
