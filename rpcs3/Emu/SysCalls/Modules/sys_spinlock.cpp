#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sysPrxForUser.h"

extern Module sysPrxForUser;

void sys_spinlock_initialize(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_initialize(lock=*0x%x)", lock);

	lock->exchange(0);
}

void sys_spinlock_lock(PPUThread& ppu, vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_lock(lock=*0x%x)", lock);

	// prx: exchange with 0xabadcafe, repeat until exchanged with 0
	vm::wait_op(ppu, lock.addr(), 4, WRAP_EXPR(!lock->exchange(0xabadcafe).data()));
}

s32 sys_spinlock_trylock(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_trylock(lock=*0x%x)", lock);

	if (lock->exchange(0xabadcafe).data())
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

void sys_spinlock_unlock(vm::ptr<atomic_be_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_unlock(lock=*0x%x)", lock);

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
