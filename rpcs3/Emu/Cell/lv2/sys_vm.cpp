#include "stdafx.h"
#include "sys_vm.h"
#include "sys_process.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/timers.hpp"

sys_vm_t::sys_vm_t(u32 _addr, u32 vsize, lv2_memory_container* ct, u32 psize)
	: ct(ct)
	, addr(_addr)
	, size(vsize)
	, psize(psize)
{
	// Write ID
	g_ids[addr >> 28].release(idm::last_id());
}

void sys_vm_t::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_vm);
	ar(ct->id, addr, size, psize);
}

sys_vm_t::~sys_vm_t()
{
	// Free ID
	g_ids[addr >> 28].release(id_manager::id_traits<sys_vm_t>::invalid);
}

LOG_CHANNEL(sys_vm);

struct sys_vm_global_t
{
	atomic_t<u32> total_vsize = 0;
};

sys_vm_t::sys_vm_t(utils::serial& ar)
	: ct(lv2_memory_container::search(ar))
	, addr(ar)
	, size(ar)
	, psize(ar)
{
	g_ids[addr >> 28].release(idm::last_id());
	g_fxo->need<sys_vm_global_t>();
	g_fxo->get<sys_vm_global_t>().total_vsize += size;
}

error_code sys_vm_memory_map(ppu_thread& ppu, u64 vsize, u64 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_memory_map(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%x, policy=0x%x, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);

	if (!vsize || !psize || vsize % 0x200'0000 || vsize > 0x1000'0000 || psize % 0x1'0000 || policy != SYS_VM_POLICY_AUTO_RECOMMENDED)
	{
		return CELL_EINVAL;
	}

	if (ppu.gpr[11] == 300 && psize < 0x10'0000)
	{
		return CELL_EINVAL;
	}

	const auto idm_ct = idm::get_unlocked<lv2_memory_container>(cid);

	const auto ct = cid == SYS_MEMORY_CONTAINER_ID_INVALID ? &g_fxo->get<lv2_memory_container>() : idm_ct.get();

	if (!ct)
	{
		return CELL_ESRCH;
	}

	if (!g_fxo->get<sys_vm_global_t>().total_vsize.fetch_op([vsize, has_root = g_ps3_process_info.has_root_perm()](u32& size)
	{
		// A single process can hold up to 256MB of virtual memory, even on DECR
		// VSH can hold more
		if ((has_root ? 0x1E000000 : 0x10000000) - size < vsize)
		{
			return false;
		}

		size += static_cast<u32>(vsize);
		return true;
	}).second)
	{
		return CELL_EBUSY;
	}

	if (!ct->take(psize))
	{
		g_fxo->get<sys_vm_global_t>().total_vsize -= static_cast<u32>(vsize);
		return CELL_ENOMEM;
	}

	// Look for unmapped space
	if (const auto area = vm::find_map(0x10000000, 0x10000000, 2 | (flag & SYS_MEMORY_PAGE_SIZE_MASK)))
	{
		sys_vm.warning("sys_vm_memory_map(): Found VM 0x%x area (vsize=0x%x)", addr, vsize);

		// Alloc all memory (shall not fail)
		ensure(area->alloc(static_cast<u32>(vsize)));
		vm::lock_sudo(area->addr, static_cast<u32>(vsize));

		idm::make<sys_vm_t>(area->addr, static_cast<u32>(vsize), ct, static_cast<u32>(psize));

		// Write a pointer for the allocated memory
		ppu.check_state();
		*addr = area->addr;
		return CELL_OK;
	}

	ct->free(psize);
	g_fxo->get<sys_vm_global_t>().total_vsize -= static_cast<u32>(vsize);
	return CELL_ENOMEM;
}

error_code sys_vm_memory_map_different(ppu_thread& ppu, u64 vsize, u64 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_memory_map_different(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);
	// TODO: if needed implement different way to map memory, unconfirmed.

	return sys_vm_memory_map(ppu, vsize, psize, cid, flag, policy, addr);
}

error_code sys_vm_unmap(ppu_thread& ppu, u32 addr)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_unmap(addr=0x%x)", addr);

	// Special case, check if its a start address by alignment
	if (addr % 0x10000000)
	{
		return CELL_EINVAL;
	}

	// Free block and info
	const auto vmo = idm::withdraw<sys_vm_t>(sys_vm_t::find_id(addr), [&](sys_vm_t& vmo)
	{
		// Free block
		ensure(vm::unmap(addr).second);

		// Return memory
		vmo.ct->free(vmo.psize);
		g_fxo->get<sys_vm_global_t>().total_vsize -= vmo.size;
	});

	if (!vmo)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_append_memory(ppu_thread& ppu, u32 addr, u64 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_append_memory(addr=0x%x, size=0x%x)", addr, size);

	if (!size || size % 0x100000)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::check<sys_vm_t>(sys_vm_t::find_id(addr), [&](sys_vm_t& vmo) -> CellError
	{
		if (vmo.addr != addr)
		{
			return CELL_EINVAL;
		}

		if (!vmo.ct->take(size))
		{
			return CELL_ENOMEM;
		}

		vmo.psize += static_cast<u32>(size);
		return {};
	});

	if (!block)
	{
		return CELL_EINVAL;
	}

	if (block.ret)
	{
		return block.ret;
	}

	return CELL_OK;
}

error_code sys_vm_return_memory(ppu_thread& ppu, u32 addr, u64 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_return_memory(addr=0x%x, size=0x%x)", addr, size);

	if (!size || size % 0x100000)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::check<sys_vm_t>(sys_vm_t::find_id(addr), [&](sys_vm_t& vmo) -> CellError
	{
		if (vmo.addr != addr)
		{
			return CELL_EINVAL;
		}

		auto [_, ok] = vmo.psize.fetch_op([&](u32& value)
		{
			if (value <= size || value - size < 0x100000ull)
			{
				return false;
			}

			value -= static_cast<u32>(size);
			return true;
		});

		if (!ok)
		{
			return CELL_EBUSY;
		}

		vmo.ct->free(size);
		return {};
	});

	if (!block)
	{
		return CELL_EINVAL;
	}

	if (block.ret)
	{
		return block.ret;
	}

	return CELL_OK;
}

error_code sys_vm_lock(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_lock(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_unlock(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_unlock(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_touch(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_touch(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_flush(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_flush(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_invalidate(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_invalidate(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_store(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_store(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_sync(ppu_thread& ppu, u32 addr, u32 size)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_sync(addr=0x%x, size=0x%x)", addr, size);

	if (!size)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_vm_test(ppu_thread& ppu, u32 addr, u32 size, vm::ptr<u64> result)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_test(addr=0x%x, size=0x%x, result=*0x%x)", addr, size, result);

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || u64{addr} + size > u64{block->addr} + block->size)
	{
		return CELL_EINVAL;
	}

	ppu.check_state();
	*result = SYS_VM_STATE_ON_MEMORY;

	return CELL_OK;
}

error_code sys_vm_get_statistics(ppu_thread& ppu, u32 addr, vm::ptr<sys_vm_statistics_t> stat)
{
	ppu.state += cpu_flag::wait;

	sys_vm.warning("sys_vm_get_statistics(addr=0x%x, stat=*0x%x)", addr, stat);

	const auto block = idm::get_unlocked<sys_vm_t>(sys_vm_t::find_id(addr));

	if (!block || block->addr != addr)
	{
		return CELL_EINVAL;
	}

	ppu.check_state();
	stat->page_fault_ppu = 0;
	stat->page_fault_spu = 0;
	stat->page_in = 0;
	stat->page_out = 0;
	stat->pmem_total = block->psize;
	stat->pmem_used = 0;
	stat->timestamp = get_timebased_time();

	return CELL_OK;
}

DECLARE(sys_vm_t::g_ids){};
