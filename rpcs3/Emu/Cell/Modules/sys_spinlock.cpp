#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "sysPrxForUser.h"

#include "Emu/Memory/wait_engine.h"

extern logs::channel sysPrxForUser;

void sys_spinlock_initialize(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.trace("sys_spinlock_initialize(lock=*0x%x)", lock);

	lock->exchange(0);
}

void sys_spinlock_lock(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.trace("sys_spinlock_lock(lock=*0x%x)", lock);

	// prx: exchange with 0xabadcafe, repeat until exchanged with 0
	vm::wait_op(lock.addr(), 4, WRAP_EXPR(!lock->exchange(0xabadcafe)));
}

s32 sys_spinlock_trylock(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.trace("sys_spinlock_trylock(lock=*0x%x)", lock);

	if (lock->exchange(0xabadcafe))
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

void sys_spinlock_unlock(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.trace("sys_spinlock_unlock(lock=*0x%x)", lock);

	lock->exchange(0);

	vm::notify_at(lock.addr(), 4);
}

void sysPrxForUser_sys_spinlock_init()
{
	REG_FUNC(sysPrxForUser, sys_spinlock_initialize);
	REG_FUNC(sysPrxForUser, sys_spinlock_lock);
	REG_FUNC(sysPrxForUser, sys_spinlock_trylock);
	REG_FUNC(sysPrxForUser, sys_spinlock_unlock);
}
