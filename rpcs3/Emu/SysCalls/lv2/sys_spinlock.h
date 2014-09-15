#pragma once

// SysCalls
void sys_spinlock_initialize(vm::ptr<std::atomic<be_t<u32>>> lock);
void sys_spinlock_lock(vm::ptr<std::atomic<be_t<u32>>> lock);
s32 sys_spinlock_trylock(vm::ptr<std::atomic<be_t<u32>>> lock);
void sys_spinlock_unlock(vm::ptr<std::atomic<be_t<u32>>> lock);
