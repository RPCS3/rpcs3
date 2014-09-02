#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_spinlock.h"

SysCallBase sys_spinlock("sys_spinlock");

void sys_spinlock_initialize(vm::ptr<std::atomic<be_t<u32>>> lock)
{
	sys_spinlock.Log("sys_spinlock_initialize(lock_addr=0x%x)", lock.addr());

	// prx: set 0 and sync
	*lock = be_t<u32>::MakeFromBE(0);
}

void sys_spinlock_lock(vm::ptr<std::atomic<be_t<u32>>> lock)
{
	sys_spinlock.Log("sys_spinlock_lock(lock_addr=0x%x)", lock.addr());

	// prx: exchange with 0xabadcafe, repeat until exchanged with 0
	while (lock->exchange(be_t<u32>::MakeFromBE(se32(0xabadcafe))).ToBE())
	{
		while (lock->load(std::memory_order_relaxed).ToBE())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				break;
			}
		}

		if (Emu.IsStopped())
		{
			sys_spinlock.Warning("sys_spinlock_lock(0x%x) aborted", lock.addr());
			break;
		}
	}
}

s32 sys_spinlock_trylock(vm::ptr<std::atomic<be_t<u32>>> lock)
{
	sys_spinlock.Log("sys_spinlock_trylock(lock_addr=0x%x)", lock.addr());

	// prx: exchange with 0xabadcafe, translate exchanged value
	if (lock->exchange(be_t<u32>::MakeFromBE(se32(0xabadcafe))).ToBE())
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

void sys_spinlock_unlock(vm::ptr<std::atomic<be_t<u32>>> lock)
{
	sys_spinlock.Log("sys_spinlock_unlock(lock_addr=0x%x)", lock.addr());

	// prx: sync and set 0
	*lock = be_t<u32>::MakeFromBE(0);
}