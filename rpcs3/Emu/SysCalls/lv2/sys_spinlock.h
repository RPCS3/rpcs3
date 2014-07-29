#pragma once

// SysCalls
void sys_spinlock_initialize(mem_ptr_t<std::atomic<be_t<u32>>> lock);
void sys_spinlock_lock(mem_ptr_t<std::atomic<be_t<u32>>> lock);
s32 sys_spinlock_trylock(mem_ptr_t<std::atomic<be_t<u32>>> lock);
void sys_spinlock_unlock(mem_ptr_t<std::atomic<be_t<u32>>> lock);
