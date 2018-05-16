#include "stdafx.h"
#include "sys_vm.h"



logs::channel sys_vm("sys_vm");

sys_vm_block_t::sys_vm_block_t(const std::shared_ptr<lv2_memory_container>& ct, u32 addr, u32 psize)
	: ct(ct)
	, addr(addr)
	, psize(psize)
{
}

error_code sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	sys_vm.error("sys_vm_memory_map(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);

	if (!vsize || !psize || vsize % 0x2000000 || psize % 0x10000 || vsize > 0x10000000 || psize > 0x10000000 || psize < 0x100000 || policy != SYS_VM_POLICY_AUTO_RECOMMENDED)
	{
		return CELL_EINVAL;
	}

	if (cid != SYS_MEMORY_CONTAINER_ID_INVALID && !idm::check<lv2_memory_container>(cid))
	{
		return CELL_ESRCH;
	}

	// Look for unmapped space
	if (const auto area = vm::find_map(vsize, vsize == 0x10000000 ? 0x10000000 : 0x1000000, 2 | (flag & SYS_MEMORY_PAGE_SIZE_MASK)))
	{
		// Alloc all memory (shall not fail)
		verify(HERE), area->alloc(vsize);
		
		const auto ct = cid == SYS_MEMORY_CONTAINER_ID_INVALID ? fxm::get_always<lv2_memory_container>() : idm::get<lv2_memory_container>(cid);
		if (!ct->take(psize))
		{
			return CELL_ENOMEM;
		}

		idm::make<sys_vm_block_t>(ct, area->addr, psize);
		
		// Write a pointer for the allocated memory
		*addr = area->addr;
		return CELL_OK;
	}

	return CELL_ENOMEM;
}

error_code sys_vm_memory_map_different(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	sys_vm.warning("sys_vm_memory_map_different(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);
	// TODO: if needed implement different way to map memory, unconfirmed.

	return sys_vm_memory_map(vsize, psize, cid, flag, policy, addr);
}

error_code sys_vm_unmap(u32 addr)
{
	sys_vm.warning("sys_vm_unmap(addr=0x%x)", addr);

	if (!vm::unmap(addr))
	{
		return {CELL_EINVAL, addr};
	}

	const auto block = idm::select<sys_vm_block_t>([&](u32 id, sys_vm_block_t& block) -> u32
	{
		if (block.addr == addr)
		{
			return id;
		}

		return 0;
	});

	block->ct->used -= block->psize;

	idm::remove<sys_vm_block_t>(block.ret);

	return CELL_OK;
}

error_code sys_vm_append_memory(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_append_memory(addr=0x%x, size=0x%x)", addr, size);

	if (!size || size % 0x100000)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::select<sys_vm_block_t>([&](u32 id, sys_vm_block_t& block) -> u32
	{
		if (block.addr == addr)
		{
			return id;
		}

		return 0;
	});

	if (!block)
	{
		return CELL_EINVAL;
	}

	if (!block->ct->take(size))
	{
		return CELL_ENOMEM;
	}

	block->psize += size;

	return CELL_OK;
}

error_code sys_vm_return_memory(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_return_memory(addr=0x%x, size=0x%x)", addr, size);

	if (!size || size % 0x100000)
	{
		return CELL_EINVAL;
	}

	const auto block = idm::select<sys_vm_block_t>([&](u32 id, sys_vm_block_t& block) -> u32
	{
		if (block.addr == addr)
		{
			return id;
		}

		return 0;
	});

	if (!block)
	{
		return CELL_EINVAL;
	}

	if (block->psize < 0x100000 + size)
	{
		return CELL_EBUSY;
	}

	block->ct->used -= size;

	block->psize -= size;

	return CELL_OK;
}

error_code sys_vm_lock(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_lock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_unlock(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_unlock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_touch(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_touch(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_flush(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_flush(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_invalidate(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_invalidate(addr=0x%x, size=0x%x)", addr, size);

	std::memset(vm::base(addr), 0, size);
	return CELL_OK;
}

error_code sys_vm_store(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_store(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_sync(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_sync(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

error_code sys_vm_test(u32 addr, u32 size, vm::ptr<u64> result)
{
	sys_vm.warning("sys_vm_test(addr=0x%x, size=0x%x, result=*0x%x)", addr, size, result);

	*result = SYS_VM_STATE_ON_MEMORY;

	return CELL_OK;
}

error_code sys_vm_get_statistics(u32 addr, vm::ptr<sys_vm_statistics_t> stat)
{
	sys_vm.warning("sys_vm_get_statistics(addr=0x%x, stat=*0x%x)", addr, stat);

	const auto block = idm::select<sys_vm_block_t>([&](u32 id, sys_vm_block_t& block) -> u32
	{
		if (block.addr == addr)
		{
			return id;
		}

		return 0;
	});

	if (!block)
	{
		return CELL_EFAULT;
	}

	stat->page_fault_ppu = 0;
	stat->page_fault_spu = 0;
	stat->page_in = 0;
	stat->page_out = 0;
	stat->pmem_total = block->psize;
	stat->pmem_used = 0;
	stat->timestamp = 0;

	return CELL_OK;
}
