#include "stdafx.h"
#include "sys_mmapper.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Memory/vm_var.h"
#include "sys_memory.h"
#include "sys_sync.h"
#include "sys_process.h"

#include <span>

#include "util/vm.hpp"

LOG_CHANNEL(sys_mmapper);

template <>
void fmt_class_string<lv2_mem_container_id>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case SYS_MEMORY_CONTAINER_ID_INVALID: return "Global";
		}

		// Resort to hex formatting for other values
		return unknown;
	});
}

lv2_memory::lv2_memory(u32 size, u32 align, u64 flags, u64 key, bool pshared, lv2_memory_container* ct)
	: size(size)
	, align(align)
	, flags(flags)
	, key(key)
	, pshared(pshared)
	, ct(ct)
	, shm(null_ptr)
{
}

lv2_memory::lv2_memory(utils::serial& ar)
	: size(ar)
	, align(ar)
	, flags(ar)
	, key(ar)
	, pshared(ar)
	, ct(lv2_memory_container::search(ar.pop<u32>()))
	, shm([&](u32 addr) -> shared_ptr<std::shared_ptr<utils::shm>>
	{
		if (addr)
		{
			return make_single_value(ensure(vm::get(vm::any, addr)->peek(addr).second));
		}

		return null_ptr;
	}(ar.pop<u32>()))
	, counter(ar)
{
}

CellError lv2_memory::on_id_create()
{
	if (!exists && !ct->take(size))
	{
		sys_mmapper.error("lv2_memory::on_id_create(): Cannot allocate 0x%x bytes (0x%x available)", size, ct->size - ct->used);
		return CELL_ENOMEM;
	}

	exists++;
	return {};
}

std::function<void(void*)> lv2_memory::load(utils::serial& ar)
{
	auto mem = make_shared<lv2_memory>(stx::exact_t<utils::serial&>(ar));
	mem->exists++; // Disable on_id_create()
	auto func = load_func(mem, +mem->pshared);
	mem->exists--;
	return func;
}

void lv2_memory::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_memory);

	ar(size, align, flags, key, pshared, ct->id);
	ar(counter ? vm::get_shm_addr(*shm.load()) : 0);
	ar(counter);
}

page_fault_notification_entries::page_fault_notification_entries(utils::serial& ar)
{
	ar(entries);
}

void page_fault_notification_entries::save(utils::serial& ar)
{
	ar(entries);
}

template <bool exclusive = false>
error_code create_lv2_shm(bool pshared, u64 ipc_key, u64 size, u32 align, u64 flags, lv2_memory_container* ct)
{
	const u32 _pshared = pshared ? SYS_SYNC_PROCESS_SHARED : SYS_SYNC_NOT_PROCESS_SHARED;

	if (!pshared)
	{
		ipc_key = 0;
	}

	if (auto error = lv2_obj::create<lv2_memory>(_pshared, ipc_key, exclusive ? SYS_SYNC_NEWLY_CREATED : SYS_SYNC_NOT_CARE, [&]()
	{
		return make_shared<lv2_memory>(
			static_cast<u32>(size),
			align,
			flags,
			ipc_key,
			pshared,
			ct);
	}, false))
	{
		return error;
	}

	return CELL_OK;
}

error_code sys_mmapper_allocate_address(ppu_thread& ppu, u64 size, u64 flags, u64 alignment, vm::ptr<u32> alloc_addr)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_allocate_address(size=0x%x, flags=0x%x, alignment=0x%x, alloc_addr=*0x%x)", size, flags, alignment, alloc_addr);

	if (size % 0x10000000)
	{
		return CELL_EALIGN;
	}

	if (size > u32{umax})
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
		if (const auto area = vm::find_map(static_cast<u32>(size), static_cast<u32>(alignment), flags & SYS_MEMORY_PAGE_SIZE_MASK))
		{
			sys_mmapper.warning("sys_mmapper_allocate_address(): Found VM 0x%x area (vsize=0x%x)", area->addr, size);

			ppu.check_state();
			*alloc_addr = area->addr;
			return CELL_OK;
		}

		return CELL_ENOMEM;
	}
	}

	return CELL_EALIGN;
}

error_code sys_mmapper_allocate_fixed_address(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_allocate_fixed_address()");

	if (!vm::map(0xB0000000, 0x10000000, SYS_MEMORY_PAGE_SIZE_1M))
	{
		return CELL_EEXIST;
	}

	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory(ppu_thread& ppu, u64 ipc_key, u64 size, u64 flags, vm::ptr<u32> mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_allocate_shared_memory(ipc_key=0x%x, size=0x%x, flags=0x%x, mem_id=*0x%x)", ipc_key, size, flags, mem_id);

	if (size == 0)
	{
		return CELL_EALIGN;
	}

	// Check page granularity
	switch (flags & SYS_MEMORY_GRANULARITY_MASK)
	{
	case 0:
	case SYS_MEMORY_GRANULARITY_1M:
	{
		if (size % 0x100000)
		{
			return CELL_EALIGN;
		}

		break;
	}
	case SYS_MEMORY_GRANULARITY_64K:
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
	auto& dct = g_fxo->get<lv2_memory_container>();

	if (auto error = create_lv2_shm(ipc_key != SYS_MMAPPER_NO_SHM_KEY, ipc_key, size, flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000, flags, &dct))
	{
		return error;
	}

	ppu.check_state();
	*mem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory_from_container(ppu_thread& ppu, u64 ipc_key, u64 size, u32 cid, u64 flags, vm::ptr<u32> mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_allocate_shared_memory_from_container(ipc_key=0x%x, size=0x%x, cid=0x%x, flags=0x%x, mem_id=*0x%x)", ipc_key, size, cid, flags, mem_id);

	if (size == 0)
	{
		return CELL_EALIGN;
	}

	// Check page granularity.
	switch (flags & SYS_MEMORY_GRANULARITY_MASK)
	{
	case 0:
	case SYS_MEMORY_GRANULARITY_1M:
	{
		if (size % 0x100000)
		{
			return CELL_EALIGN;
		}

		break;
	}
	case SYS_MEMORY_GRANULARITY_64K:
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

	const auto ct = idm::get_unlocked<lv2_memory_container>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	if (auto error = create_lv2_shm(ipc_key != SYS_MMAPPER_NO_SHM_KEY, ipc_key, size, flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000, flags, ct.get()))
	{
		return error;
	}

	ppu.check_state();
	*mem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory_ext(ppu_thread& ppu, u64 ipc_key, u64 size, u32 flags, vm::ptr<mmapper_unk_entry_struct0> entries, s32 entry_count, vm::ptr<u32> mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.todo("sys_mmapper_allocate_shared_memory_ext(ipc_key=0x%x, size=0x%x, flags=0x%x, entries=*0x%x, entry_count=0x%x, mem_id=*0x%x)", ipc_key, size, flags, entries, entry_count, mem_id);

	if (size == 0)
	{
		return CELL_EALIGN;
	}

	switch (flags & SYS_MEMORY_GRANULARITY_MASK)
	{
	case SYS_MEMORY_GRANULARITY_1M:
	case 0:
	{
		if (size % 0x100000)
		{
			return CELL_EALIGN;
		}

		break;
	}
	case SYS_MEMORY_GRANULARITY_64K:
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

	if (flags & ~SYS_MEMORY_PAGE_SIZE_MASK)
	{
		return CELL_EINVAL;
	}

	if (entry_count <= 0 || entry_count > 0x10)
	{
		return CELL_EINVAL;
	}

	if constexpr (bool to_perm_check = false; true)
	{
		for (s32 i = 0; i < entry_count; i++)
		{
			const u64 type = entries[i].type;

			// The whole structure contents are unknown
			sys_mmapper.todo("sys_mmapper_allocate_shared_memory_ext(): entry type = 0x%x", type);

			switch (type)
			{
			case 0:
			case 1:
			case 3:
			{
				break;
			}
			case 5:
			{
				to_perm_check = true;
				break;
			}
			default:
			{
				return CELL_EPERM;
			}
			}
		}

		if (to_perm_check)
		{
			if (flags != SYS_MEMORY_PAGE_SIZE_64K || !g_ps3_process_info.debug_or_root())
			{
				return CELL_EPERM;
			}
		}
	}

	// Get "default" memory container
	auto& dct = g_fxo->get<lv2_memory_container>();

	if (auto error = create_lv2_shm<true>(true, ipc_key, size, flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000, flags, &dct))
	{
		return error;
	}

	ppu.check_state();
	*mem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mmapper_allocate_shared_memory_from_container_ext(ppu_thread& ppu, u64 ipc_key, u64 size, u64 flags, u32 cid, vm::ptr<mmapper_unk_entry_struct0> entries, s32 entry_count, vm::ptr<u32> mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.todo("sys_mmapper_allocate_shared_memory_from_container_ext(ipc_key=0x%x, size=0x%x, flags=0x%x, cid=0x%x, entries=*0x%x, entry_count=0x%x, mem_id=*0x%x)", ipc_key, size, flags, cid, entries,
		entry_count, mem_id);

	switch (flags & SYS_MEMORY_PAGE_SIZE_MASK)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
	case 0:
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

	if (flags & ~SYS_MEMORY_PAGE_SIZE_MASK)
	{
		return CELL_EINVAL;
	}

	if (entry_count <= 0 || entry_count > 0x10)
	{
		return CELL_EINVAL;
	}

	if constexpr (bool to_perm_check = false; true)
	{
		for (s32 i = 0; i < entry_count; i++)
		{
			const u64 type = entries[i].type;

			sys_mmapper.todo("sys_mmapper_allocate_shared_memory_from_container_ext(): entry type = 0x%x", type);

			switch (type)
			{
			case 0:
			case 1:
			case 3:
			{
				break;
			}
			case 5:
			{
				to_perm_check = true;
				break;
			}
			default:
			{
				return CELL_EPERM;
			}
			}
		}

		if (to_perm_check)
		{
			if (flags != SYS_MEMORY_PAGE_SIZE_64K || !g_ps3_process_info.debug_or_root())
			{
				return CELL_EPERM;
			}
		}
	}

	const auto ct = idm::get_unlocked<lv2_memory_container>(cid);

	if (!ct)
	{
		return CELL_ESRCH;
	}

	if (auto error = create_lv2_shm<true>(true, ipc_key, size, flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000, flags, ct.get()))
	{
		return error;
	}

	ppu.check_state();
	*mem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mmapper_change_address_access_right(ppu_thread& ppu, u32 addr, u64 flags)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.todo("sys_mmapper_change_address_access_right(addr=0x%x, flags=0x%x)", addr, flags);

	return CELL_OK;
}

error_code sys_mmapper_free_address(ppu_thread& ppu, u32 addr)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_free_address(addr=0x%x)", addr);

	if (addr < 0x20000000 || addr >= 0xC0000000)
	{
		return {CELL_EINVAL, addr};
	}

	// If page fault notify exists and an address in this area is faulted, we can't free the memory.
	auto& pf_events = g_fxo->get<page_fault_event_entries>();
	std::lock_guard pf_lock(pf_events.pf_mutex);

	const auto mem = vm::get(vm::any, addr);

	if (!mem || mem->addr != addr)
	{
		return {CELL_EINVAL, addr};
	}

	for (const auto& ev : pf_events.events)
	{
		if (addr <= ev.second && ev.second <= addr + mem->size - 1)
		{
			return CELL_EBUSY;
		}
	}

	// Try to unmap area
	const auto [area, success] = vm::unmap(addr, true, &mem);

	if (!area)
	{
		return {CELL_EINVAL, addr};
	}

	if (!success)
	{
		return CELL_EBUSY;
	}

	// If a memory block is freed, remove it from page notification table.
	auto& pf_entries = g_fxo->get<page_fault_notification_entries>();
	std::lock_guard lock(pf_entries.mutex);

	auto ind_to_remove = pf_entries.entries.begin();
	for (; ind_to_remove != pf_entries.entries.end(); ++ind_to_remove)
	{
		if (addr == ind_to_remove->start_addr)
		{
			break;
		}
	}
	if (ind_to_remove != pf_entries.entries.end())
	{
		pf_entries.entries.erase(ind_to_remove);
	}

	return CELL_OK;
}

error_code sys_mmapper_free_shared_memory(ppu_thread& ppu, u32 mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_free_shared_memory(mem_id=0x%x)", mem_id);

	// Conditionally remove memory ID
	const auto mem = idm::withdraw<lv2_obj, lv2_memory>(mem_id, [&](lv2_memory& mem) -> CellError
	{
		if (mem.counter)
		{
			return CELL_EBUSY;
		}

		lv2_obj::on_id_destroy(mem, mem.key, +mem.pshared);

		if (!mem.exists)
		{
			// Return "physical memory" to the memory container
			mem.ct->free(mem.size);
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

	return CELL_OK;
}

error_code sys_mmapper_map_shared_memory(ppu_thread& ppu, u32 addr, u32 mem_id, u64 flags)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_map_shared_memory(addr=0x%x, mem_id=0x%x, flags=0x%x)", addr, mem_id, flags);

	const auto area = vm::get(vm::any, addr);

	if (!area || addr < 0x20000000 || addr >= 0xC0000000)
	{
		return CELL_EINVAL;
	}

	const auto mem = idm::get<lv2_obj, lv2_memory>(mem_id, [&](lv2_memory& mem) -> CellError
	{
		const u32 page_alignment = area->flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000;

		if (mem.align < page_alignment)
		{
			return CELL_EINVAL;
		}

		if (addr % page_alignment)
		{
			return CELL_EALIGN;
		}

		for (stx::shared_ptr<std::shared_ptr<utils::shm>> to_insert, null; !mem.shm;)
		{
			// Insert atomically the memory handle (laziliy allocated)
			if (!to_insert)
			{
				to_insert = make_single_value(std::make_shared<utils::shm>(mem.size, 1 /* shareable flag */));
			}

			null.reset();

			if (mem.shm.compare_exchange(null, to_insert))
			{
				break;
			}
		}

		mem.counter++;
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

	auto shm_ptr = *mem->shm.load();

	if (!area->falloc(addr, mem->size, &shm_ptr, mem->align == 0x10000 ? SYS_MEMORY_PAGE_SIZE_64K : SYS_MEMORY_PAGE_SIZE_1M))
	{
		mem->counter--;

		if (!area->is_valid())
		{
			return {CELL_EINVAL, addr};
		}

		return CELL_EBUSY;
	}

	vm::lock_sudo(addr, mem->size);
	return CELL_OK;
}

error_code sys_mmapper_search_and_map(ppu_thread& ppu, u32 start_addr, u32 mem_id, u64 flags, vm::ptr<u32> alloc_addr)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_search_and_map(start_addr=0x%x, mem_id=0x%x, flags=0x%x, alloc_addr=*0x%x)", start_addr, mem_id, flags, alloc_addr);

	const auto area = vm::get(vm::any, start_addr);

	if (!area || start_addr != area->addr || start_addr < 0x20000000 || start_addr >= 0xC0000000)
	{
		return {CELL_EINVAL, start_addr};
	}

	const auto mem = idm::get<lv2_obj, lv2_memory>(mem_id, [&](lv2_memory& mem) -> CellError
	{
		const u32 page_alignment = area->flags & SYS_MEMORY_PAGE_SIZE_64K ? 0x10000 : 0x100000;

		if (mem.align < page_alignment)
		{
			return CELL_EALIGN;
		}

		for (stx::shared_ptr<std::shared_ptr<utils::shm>> to_insert, null; !mem.shm;)
		{
			// Insert atomically the memory handle (laziliy allocated)
			if (!to_insert)
			{
				to_insert = make_single_value(std::make_shared<utils::shm>(mem.size, 1 /* shareable flag */));
			}

			null.reset();

			if (mem.shm.compare_exchange(null, to_insert))
			{
				break;
			}
		}

		mem.counter++;
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

	auto shm_ptr = *mem->shm.load();

	const u32 addr = area->alloc(mem->size, &shm_ptr, mem->align, mem->align == 0x10000 ? SYS_MEMORY_PAGE_SIZE_64K : SYS_MEMORY_PAGE_SIZE_1M);

	if (!addr)
	{
		mem->counter--;

		if (!area->is_valid())
		{
			return {CELL_EINVAL, start_addr};
		}

		return CELL_ENOMEM;
	}

	sys_mmapper.notice("sys_mmapper_search_and_map(): Found 0x%x address", addr);

	vm::lock_sudo(addr, mem->size);
	ppu.check_state();
	*alloc_addr = addr;
	return CELL_OK;
}

error_code sys_mmapper_unmap_shared_memory(ppu_thread& ppu, u32 addr, vm::ptr<u32> mem_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_unmap_shared_memory(addr=0x%x, mem_id=*0x%x)", addr, mem_id);

	const auto area = vm::get(vm::any, addr);

	if (!area || addr < 0x20000000 || addr >= 0xC0000000)
	{
		return {CELL_EINVAL, addr};
	}

	const auto shm = area->peek(addr);

	if (!shm.second)
	{
		return {CELL_EINVAL, addr};
	}

	const auto mem = idm::select<lv2_obj, lv2_memory>([&](u32 id, lv2_memory& mem) -> u32
	{
		if (auto shm0 = mem.shm.load(); shm0 && shm0->get() == shm.second.get())
		{
			return id;
		}

		return 0;
	});

	if (!mem)
	{
		return {CELL_EINVAL, addr};
	}

	if (!area->dealloc(addr, &shm.second))
	{
		return {CELL_EINVAL, addr};
	}

	// Write out the ID
	ppu.check_state();
	*mem_id = mem.ret;

	// Acknowledge
	mem->counter--;

	return CELL_OK;
}

error_code sys_mmapper_enable_page_fault_notification(ppu_thread& ppu, u32 start_addr, u32 event_queue_id)
{
	ppu.state += cpu_flag::wait;

	sys_mmapper.warning("sys_mmapper_enable_page_fault_notification(start_addr=0x%x, event_queue_id=0x%x)", start_addr, event_queue_id);

	auto mem = vm::get(vm::any, start_addr);
	if (!mem || start_addr != mem->addr || start_addr < 0x20000000 || start_addr >= 0xC0000000)
	{
		return {CELL_EINVAL, start_addr};
	}

	// TODO: Check memory region's flags to make sure the memory can be used for page faults.

	auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(event_queue_id);

	if (!queue)
	{ // Can't connect the queue if it doesn't exist.
		return CELL_ESRCH;
	}

	vm::var<u32> port_id(0);
	error_code res = sys_event_port_create(ppu, port_id, SYS_EVENT_PORT_LOCAL, SYS_MEMORY_PAGE_FAULT_EVENT_KEY);
	sys_event_port_connect_local(ppu, *port_id, event_queue_id);

	if (res + 0u == CELL_EAGAIN)
	{
		// Not enough system resources.
		return CELL_EAGAIN;
	}

	auto& pf_entries = g_fxo->get<page_fault_notification_entries>();
	std::unique_lock lock(pf_entries.mutex);

	// Return error code if page fault notifications are already enabled
	for (const auto& entry : pf_entries.entries)
	{
		if (entry.start_addr == start_addr)
		{
			lock.unlock();
			sys_event_port_disconnect(ppu, *port_id);
			sys_event_port_destroy(ppu, *port_id);
			return CELL_EBUSY;
		}
	}

	page_fault_notification_entry entry{ start_addr, event_queue_id, port_id->value() };
	pf_entries.entries.emplace_back(entry);

	return CELL_OK;
}

error_code mmapper_thread_recover_page_fault(cpu_thread* cpu)
{
	// We can only wake a thread if it is being suspended for a page fault.
	auto& pf_events = g_fxo->get<page_fault_event_entries>();
	{
		std::lock_guard pf_lock(pf_events.pf_mutex);
		const auto pf_event_ind = pf_events.events.find(cpu);

		if (pf_event_ind == pf_events.events.end())
		{
			// if not found...
			return CELL_EINVAL;
		}

		pf_events.events.erase(pf_event_ind);

		if (cpu->get_class() == thread_class::ppu)
		{
			lv2_obj::awake(cpu);
		}
		else
		{
			cpu->state += cpu_flag::signal;
		}
	}

	if (cpu->state & cpu_flag::signal)
	{
		cpu->state.notify_one();
	}

	return CELL_OK;
}
