#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sysPrxForUser.h"

extern Module sysPrxForUser;

s32 sys_mempool_allocate_block()
{
	throw EXCEPTION("");
}

s32 sys_mempool_create()
{
	throw EXCEPTION("");
}

s32 sys_mempool_destroy()
{
	throw EXCEPTION("");
}

s32 sys_mempool_free_block()
{
	throw EXCEPTION("");
}

s32 sys_mempool_get_count()
{
	throw EXCEPTION("");
}

s32 sys_mempool_try_allocate_block()
{
	throw EXCEPTION("");
}


void sysPrxForUser_sys_mempool_init()
{
	REG_FUNC(sysPrxForUser, sys_mempool_allocate_block);
	REG_FUNC(sysPrxForUser, sys_mempool_create);
	REG_FUNC(sysPrxForUser, sys_mempool_destroy);
	REG_FUNC(sysPrxForUser, sys_mempool_free_block);
	REG_FUNC(sysPrxForUser, sys_mempool_get_count);
	REG_FUNC(sysPrxForUser, sys_mempool_try_allocate_block);
}
