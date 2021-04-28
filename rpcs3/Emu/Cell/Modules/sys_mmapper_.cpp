#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_mmapper.h"

LOG_CHANNEL(sysPrxForUser);

error_code sys_mmapper_allocate_memory(ppu_thread& ppu, u32 size, u64 flags, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_allocate_memory(size=0x%x, flags=0x%llx, mem_id=*0x%x)", size, flags, mem_id);

	return sys_mmapper_allocate_shared_memory(ppu, SYS_MMAPPER_NO_SHM_KEY, size, flags, mem_id);
}

error_code sys_mmapper_allocate_memory_from_container(ppu_thread& ppu, u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_allocate_memory_from_container(size=0x%x, cid=0x%x, flags=0x%llx, mem_id=*0x%x)", size, cid, flags, mem_id);

	return sys_mmapper_allocate_shared_memory_from_container(ppu, SYS_MMAPPER_NO_SHM_KEY, size, cid, flags, mem_id);
}

error_code sys_mmapper_map_memory(ppu_thread& ppu, u32 addr, u32 mem_id, u64 flags)
{
	sysPrxForUser.notice("sys_mmapper_map_memory(addr=0x%x, mem_id=0x%x, flags=0x%llx)", addr, mem_id, flags);

	return sys_mmapper_map_shared_memory(ppu, addr, mem_id, flags);
}

error_code sys_mmapper_unmap_memory(ppu_thread& ppu, u32 addr, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_unmap_memory(addr=0x%x, mem_id=*0x%x)", addr, mem_id);

	return sys_mmapper_unmap_shared_memory(ppu, addr, mem_id);
}

error_code sys_mmapper_free_memory(ppu_thread& ppu, u32 mem_id)
{
	sysPrxForUser.notice("sys_mmapper_free_memory(mem_id=0x%x)", mem_id);

	return sys_mmapper_free_shared_memory(ppu, mem_id);
}

extern void sysPrxForUser_sys_mmapper_init()
{
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory_from_container);
	REG_FUNC(sysPrxForUser, sys_mmapper_map_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_unmap_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_free_memory);
}
