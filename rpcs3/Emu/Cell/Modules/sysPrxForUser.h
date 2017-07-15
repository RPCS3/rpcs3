#pragma once

namespace vm { using namespace ps3; }

using spu_printf_cb_t = vm::ptr<s32(u32 arg)>;

// Aux

extern spu_printf_cb_t g_spu_printf_agcb;
extern spu_printf_cb_t g_spu_printf_dgcb;
extern spu_printf_cb_t g_spu_printf_atcb;
extern spu_printf_cb_t g_spu_printf_dtcb;

// Functions

vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size);

struct sys_lwmutex_t;
struct sys_lwmutex_attribute_t;

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
s32 sys_lwmutex_lock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
s32 sys_lwmutex_trylock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_unlock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_destroy(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);

struct sys_lwmutex_locker
{
	ppu_thread& ppu;
	vm::ptr<sys_lwmutex_t> mutex;

	sys_lwmutex_locker(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> mutex)
		: ppu(ppu)
		, mutex(mutex)
	{
		verify(HERE), sys_lwmutex_lock(ppu, mutex, 0) == CELL_OK;
	}

	~sys_lwmutex_locker() noexcept(false)
	{
		verify(HERE), sys_lwmutex_unlock(ppu, mutex) == CELL_OK;
	}
};

struct sys_lwcond_t;
struct sys_lwcond_attribute_t;

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr);
s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_all(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_to(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id);
s32 sys_lwcond_wait(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 timeout);

void sys_ppu_thread_exit(ppu_thread& CPU, u64 val);
