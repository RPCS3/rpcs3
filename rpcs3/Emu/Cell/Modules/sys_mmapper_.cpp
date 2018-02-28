#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_mmapper.h"



extern logs::channel sysPrxForUser;

s32 sys_mmapper_allocate_memory(u32 size, u64 flags, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_allocate_memory(size=0x%x, flags=0x%llx, mem_id=*0x%x)", size, flags, mem_id);

	return sys_mmapper_allocate_shared_memory(0xffff000000000000ull, size, flags, mem_id);
}

s32 sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_allocate_memory_from_container(size=0x%x, cid=0x%x, flags=0x%llx, mem_id=*0x%x)", size, cid, flags, mem_id);

	return sys_mmapper_allocate_shared_memory_from_container(0xffff000000000000ull, size, cid, flags, mem_id);
}

s32 sys_mmapper_map_memory(u32 addr, u32 mem_id, u64 flags)
{
	sysPrxForUser.notice("sys_mmapper_map_memory(addr=0x%x, mem_id=0x%x, flags=0x%llx)", addr, mem_id, flags);

	return sys_mmapper_map_shared_memory(addr, mem_id, flags);
}

s32 sys_mmapper_unmap_memory(u32 addr, vm::ptr<u32> mem_id)
{
	sysPrxForUser.notice("sys_mmapper_unmap_memory(addr=0x%x, mem_id=*0x%x)", addr, mem_id);

	return sys_mmapper_unmap_shared_memory(addr, mem_id);
}

s32 sys_mmapper_free_memory(u32 mem_id)
{
	sysPrxForUser.notice("sys_mmapper_free_memory(mem_id=0x%x)", mem_id);

	return sys_mmapper_free_shared_memory(mem_id);
}

extern void sysPrxForUser_sys_mmapper_init()
{
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory_from_container);
	REG_FUNC(sysPrxForUser, sys_mmapper_map_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_unmap_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_free_memory);
}
