#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_mmapper.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

void sysPrxForUser_sys_mmapper_init()
{
	// TODO: split syscalls and liblv2 functions
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory_from_container);
	REG_FUNC(sysPrxForUser, sys_mmapper_map_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_unmap_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_free_memory);
}
