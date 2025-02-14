#include "stdafx.h"
#include "sys_memory.h"

#include "Emu/Memory/vm_locking.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/IdManager.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_memory);

//
static shared_mutex s_memstats_mtx;

lv2_memory_container::lv2_memory_container(u32 size, bool from_idm) noexcept
	: size(size)
	, id{from_idm ? idm::last_id() : SYS_MEMORY_CONTAINER_ID_INVALID}
{
}

lv2_memory_container::lv2_memory_container(utils::serial& ar, bool from_idm) noexcept
	: size(ar)
	, id{from_idm ? idm::last_id() : SYS_MEMORY_CONTAINER_ID_INVALID}
	, used(ar)
{
}

std::function<void(void*)> lv2_memory_container::load(utils::serial& ar)
{
	// Use idm::last_id() only for the instances at IDM
	return [ptr = make_shared<lv2_memory_container>(stx::exact_t<utils::serial&>(ar), true)](void* storage)
	{
		*static_cast<atomic_ptr<lv2_memory_container>*>(storage) = ptr;
	};
}

void lv2_memory_container::save(utils::serial& ar)
{
	ar(size, used);
}

lv2_memory_container* lv2_memory_container::search(u32 id)
{
	if (id != SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		return idm::check_unlocked<lv2_memory_container>(id);
	}

	return &g_fxo->get<lv2_memory_container>();
}

struct sys_memory_address_table
{
	atomic_t<lv2_memory_container*> addrs[65536]{};

	sys_memory_address_table() = default;

	SAVESTATE_INIT_POS(id_manager::id_map<lv2_memory_container>::savestate_init_pos + 0.1);

	sys_memory_address_table(utils::serial& ar)
	{
		// First: address, second: conatiner ID (SYS_MEMORY_CONTAINER_ID_INVALID for global FXO memory container)
		std::unordered_map<u16, u32> mm;
		ar(mm);

		for (const auto& [addr, id] : mm)
		{
			addrs[addr] = ensure(lv2_memory_container::search(id));
		}
	}

	void save(utils::serial& ar)
	{
		std::unordered_map<u16, u32> mm;

		for (auto& ctr : addrs)
		{
			if (const auto ptr = +ctr)
			{
				mm[static_cast<u16>(&ctr - addrs)] = ptr->id;
			}
		}

		ar(mm);
	}
};

std::shared_ptr<vm::block_t> reserve_map(u32 alloc_size, u32 align)
{
	return vm::reserve_map(align == 0x10000 ? vm::user64k : vm::user1m, 0, align == 0x10000 ? 0x20000000 : utils::align(alloc_size, 0x10000000)
		, align == 0x10000 ? (vm::page_size_64k | vm::bf0_0x1) : (vm::page_size_1m | vm::bf0_0x1));
}

// Todo: fix order of error checks

error_code sys_memory_allocate(cpu_thread& cpu, u64 size, u64 flags, vm::ptr<u32> alloc_addr)
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
	auto& dct = g_fxo->get<lv2_memory_container>();

	// Try to get "physical memory"
	if (!dct.take(size))
	{
		return {CELL_ENOMEM, dct.size - dct.used};
	}

	if (const auto area = reserve_map(static_cast<u32>(size), align))
	{
		if (const u32 addr = area->alloc(static_cast<u32>(size), nullptr, align))
		{
			ensure(!g_fxo->get<sys_memory_address_table>().addrs[addr >> 16].exchange(&dct));

			if (alloc_addr)
			{
				sys_memory.notice("sys_memory_allocate(): Allocated 0x%x address (size=0x%x)", addr, size);

				vm::lock_sudo(addr, static_cast<u32>(size));
				cpu.check_state();
				*alloc_addr = addr;
				return CELL_OK;
			}

			// Dealloc using the syscall
			sys_memory_free(cpu, addr);
			return CELL_EFAULT;
		}
	}

	dct.free(size);
	return CELL_ENOMEM;
}

error_code sys_memory_allocate_from_container(cpu_thread& cpu, u64 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr)
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
		return {ct.ret, ct->size - ct->used};
	}

	if (const auto area = reserve_map(static_cast<u32>(size), align))
	{
		if (const u32 addr = area->alloc(static_cast<u32>(size)))
		{
			ensure(!g_fxo->get<sys_memory_address_table>().addrs[addr >> 16].exchange(ct.ptr.get()));

			if (alloc_addr)
			{
				vm::lock_sudo(addr, static_cast<u32>(size));
				cpu.check_state();
				*alloc_addr = addr;
				return CELL_OK;
			}

			// Dealloc using the syscall
			sys_memory_free(cpu, addr);
			return CELL_EFAULT;
		}
	}

	ct->free(size);
	return CELL_ENOMEM;
}

error_code sys_memory_free(cpu_thread& cpu, u32 addr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_free(addr=0x%x)", addr);

	const auto ct = addr % 0x10000 ? nullptr : g_fxo->get<sys_memory_address_table>().addrs[addr >> 16].exchange(nullptr);

	if (!ct)
	{
		return {CELL_EINVAL, addr};
	}

	const auto size = (ensure(vm::dealloc(addr)));
	reader_lock{id_manager::g_mutex}, ct->free(size);
	return CELL_OK;
}

error_code sys_memory_get_page_attribute(cpu_thread& cpu, u32 addr, vm::ptr<sys_page_attr_t> attr)
{
	cpu.state += cpu_flag::wait;

	sys_memory.trace("sys_memory_get_page_attribute(addr=0x%x, attr=*0x%x)", addr, attr);

	vm::writer_lock rlock;

	if (!vm::check_addr(addr) || addr >= SPU_FAKE_BASE_ADDR)
	{
		return CELL_EINVAL;
	}

	if (!vm::check_addr(attr.addr(), vm::page_readable, attr.size()))
	{
		return CELL_EFAULT;
	}

	attr->attribute = 0x40000ull; // SYS_MEMORY_PROT_READ_WRITE (TODO)
	attr->access_right = addr >> 28 == 0xdu ? SYS_MEMORY_ACCESS_RIGHT_PPU_THR : SYS_MEMORY_ACCESS_RIGHT_ANY;// (TODO)

	if (vm::check_addr(addr, vm::page_1m_size))
	{
		attr->page_size = 0x100000;
	}
	else if (vm::check_addr(addr, vm::page_64k_size))
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
	auto& dct = g_fxo->get<lv2_memory_container>();

	sys_memory_info_t out{};
	{
		::reader_lock lock(s_memstats_mtx);

		out.total_user_memory = dct.size;
		out.available_user_memory = dct.size - dct.used;

		// Scan other memory containers
		idm::select<lv2_memory_container>([&](u32, lv2_memory_container& ct)
		{
			out.total_user_memory -= ct.size;
		});
	}

	cpu.check_state();
	*mem_info = out;
	return CELL_OK;
}

error_code sys_memory_get_user_memory_stat(cpu_thread& cpu, vm::ptr<sys_memory_user_memory_stat_t> mem_stat)
{
	cpu.state += cpu_flag::wait;

	sys_memory.todo("sys_memory_get_user_memory_stat(mem_stat=*0x%x)", mem_stat);

	return CELL_OK;
}

error_code sys_memory_container_create(cpu_thread& cpu, vm::ptr<u32> cid, u64 size)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_container_create(cid=*0x%x, size=0x%x)", cid, size);

	// Round down to 1 MB granularity
	size &= ~0xfffff;

	if (!size)
	{
		return CELL_ENOMEM;
	}

	auto& dct = g_fxo->get<lv2_memory_container>();

	std::lock_guard lock(s_memstats_mtx);

	// Try to obtain "physical memory" from the default container
	if (!dct.take(size))
	{
		return CELL_ENOMEM;
	}

	// Create the memory container
	if (const u32 id = idm::make<lv2_memory_container>(static_cast<u32>(size), true))
	{
		cpu.check_state();
		*cid = id;
		return CELL_OK;
	}

	dct.free(size);
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
	g_fxo->get<lv2_memory_container>().free(ct->size);

	return CELL_OK;
}

error_code sys_memory_container_get_size(cpu_thread& cpu, vm::ptr<sys_memory_info_t> mem_info, u32 cid)
{
	cpu.state += cpu_flag::wait;

	sys_memory.warning("sys_memory_container_get_size(mem_info=*0x%x, cid=0x%x)", mem_info, cid);

	const auto ct = idm::get_unlocked<lv2_memory_container>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	cpu.check_state();
	mem_info->total_user_memory = ct->size; // Total container memory
	mem_info->available_user_memory = ct->size - ct->used; // Available container memory

	return CELL_OK;
}

error_code sys_memory_container_destroy_parent_with_childs(cpu_thread& cpu, u32 cid, u32 must_0, vm::ptr<u32> mc_child)
{
	sys_memory.warning("sys_memory_container_destroy_parent_with_childs(cid=0x%x, must_0=%d, mc_child=*0x%x)", cid, must_0, mc_child);

	if (must_0)
	{
		return CELL_EINVAL;
	}

	// Multi-process is not supported yet so child containers mean nothing at the moment
	// Simply destroy parent
	return sys_memory_container_destroy(cpu, cid);
}
