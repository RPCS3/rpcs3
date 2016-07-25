#include "stdafx.h"
#include "sys_vm.h"
#include "sys_memory.h"

namespace vm { using namespace ps3; }

logs::channel sys_vm("sys_vm", logs::level::notice);

ppu_error_code sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, vm::ptr<u32> addr)
{
	sys_vm.error("sys_vm_memory_map(vsize=0x%x, psize=0x%x, cid=0x%x, flags=0x%llx, policy=0x%llx, addr=*0x%x)", vsize, psize, cid, flag, policy, addr);

	if (!vsize || !psize || vsize & 0x2000000 || vsize > 0x10000000 || psize > 0x10000000 || policy != SYS_VM_POLICY_AUTO_RECOMMENDED)
	{
		return CELL_EINVAL;
	}

	if (cid != SYS_MEMORY_CONTAINER_ID_INVALID && !idm::check<lv2_memory_container>(cid))
	{
		return CELL_ESRCH;
	}

	// Look for unmapped space (roughly)
	for (u32 found = 0x60000000; found <= 0xC0000000 - vsize; found += 0x2000000)
	{
		// Try to map
		if (const auto area = vm::map(found, vsize, flag))
		{
			// Alloc all memory (shall not fail)
			VERIFY(area->alloc(vsize));

			// Write a pointer for the allocated memory
			*addr = found;
			return CELL_OK;
		}
	}

	return CELL_ENOMEM;
}

ppu_error_code sys_vm_unmap(u32 addr)
{
	sys_vm.warning("sys_vm_unmap(addr=0x%x)", addr);

	if (!vm::unmap(addr))
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

ppu_error_code sys_vm_append_memory(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_append_memory(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_return_memory(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_return_memory(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_lock(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_lock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_unlock(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_unlock(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_touch(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_touch(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_flush(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_flush(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_invalidate(u32 addr, u32 size)
{
	sys_vm.todo("sys_vm_invalidate(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_store(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_store(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_sync(u32 addr, u32 size)
{
	sys_vm.warning("sys_vm_sync(addr=0x%x, size=0x%x)", addr, size);

	return CELL_OK;
}

ppu_error_code sys_vm_test(u32 addr, u32 size, vm::ptr<u64> result)
{
	sys_vm.warning("sys_vm_test(addr=0x%x, size=0x%x, result=*0x%x)", addr, size, result);

	*result = SYS_VM_STATE_ON_MEMORY;

	return CELL_OK;
}

ppu_error_code sys_vm_get_statistics(u32 addr, vm::ptr<sys_vm_statistics_t> stat)
{
	sys_vm.warning("sys_vm_get_statistics(addr=0x%x, stat=*0x%x)", addr, stat);

	stat->page_fault_ppu = 0;
	stat->page_fault_spu = 0;
	stat->page_in = 0;
	stat->page_out = 0;
	stat->pmem_total = 0;
	stat->pmem_used = 0;
	stat->timestamp = 0;

	return CELL_OK;
}
