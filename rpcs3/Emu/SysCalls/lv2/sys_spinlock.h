#pragma once

struct spinlock
{
	SMutexBE mutex;
};

// SysCalls
void sys_spinlock_initialize(mem_ptr_t<spinlock> lock);
void sys_spinlock_lock(mem_ptr_t<spinlock> lock);
s32 sys_spinlock_trylock(mem_ptr_t<spinlock> lock);
void sys_spinlock_unlock(mem_ptr_t<spinlock> lock);
