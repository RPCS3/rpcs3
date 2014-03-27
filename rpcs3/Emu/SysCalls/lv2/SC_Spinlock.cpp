#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Spinlock.h"

SysCallBase sys_spinlock("sys_spinlock");

void sys_spinlock_initialize(mem_ptr_t<spinlock> lock)
{
	sys_spinlock.Log("sys_spinlock_initialize(lock_addr=0x%x)", lock.GetAddr());

	lock->mutex.initialize();
}

void sys_spinlock_lock(mem_ptr_t<spinlock> lock)
{
	sys_spinlock.Log("sys_spinlock_lock(lock_addr=0x%x)", lock.GetAddr());

	be_t<u32> tid = GetCurrentPPUThread().GetId();
	switch (lock->mutex.lock(tid))
	{
	case SMR_ABORT: ConLog.Warning("sys_spinlock_lock(0x%x) aborted", lock.GetAddr()); break;
	case SMR_DEADLOCK: ConLog.Error("sys_spinlock_lock(0x%x) reached deadlock", lock.GetAddr()); break; // ???
	default: break;
	}
}

int sys_spinlock_trylock(mem_ptr_t<spinlock> lock)
{
	sys_spinlock.Log("sys_spinlock_trylock(lock_addr=0x%x)", lock.GetAddr());

	be_t<u32> tid = GetCurrentPPUThread().GetId();
	switch (lock->mutex.trylock(tid))
	{
	case SMR_FAILED: return CELL_EBUSY;
	case SMR_ABORT: ConLog.Warning("sys_spinlock_trylock(0x%x) aborted", lock.GetAddr()); break;
	case SMR_DEADLOCK: ConLog.Error("sys_spinlock_trylock(0x%x) reached deadlock", lock.GetAddr()); break;
	default: break;
	}

	return CELL_OK;
}

void sys_spinlock_unlock(mem_ptr_t<spinlock> lock)
{
	sys_spinlock.Log("sys_spinlock_unlock(lock_addr=0x%x)", lock.GetAddr());

	while(true)
	{
		if (lock->mutex.unlock(lock->mutex.GetOwner()) != SMR_PERMITTED)
			break;
	}
}