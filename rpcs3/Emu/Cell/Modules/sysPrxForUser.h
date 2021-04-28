#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

enum
{
	RANDOM_NUMBER_MAX_SIZE = 4096
};

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

error_code sys_lwmutex_create(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
error_code sys_lwmutex_lock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
error_code sys_lwmutex_trylock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
error_code sys_lwmutex_unlock(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
error_code sys_lwmutex_destroy(ppu_thread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);

struct sys_lwmutex_locker
{
	ppu_thread& ppu;
	vm::ptr<sys_lwmutex_t> mutex;

	sys_lwmutex_locker(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> mutex)
		: ppu(ppu)
		, mutex(mutex)
	{
		ensure(sys_lwmutex_lock(ppu, mutex, 0) == CELL_OK);
	}

	~sys_lwmutex_locker() noexcept(false)
	{
		ensure(sys_lwmutex_unlock(ppu, mutex) == CELL_OK);
	}
};


struct sys_lwcond_t;
struct sys_lwcond_attribute_t;

error_code sys_lwcond_create(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr);
error_code sys_lwcond_destroy(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond);
error_code sys_lwcond_signal(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond);
error_code sys_lwcond_signal_all(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond);
error_code sys_lwcond_signal_to(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 ppu_thread_id);
error_code sys_lwcond_wait(ppu_thread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 timeout);

error_code sys_ppu_thread_create(ppu_thread& ppu, vm::ptr<u64> thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname);
error_code sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih);

void sys_ppu_thread_exit(ppu_thread& CPU, u64 val);
void sys_game_process_exitspawn(ppu_thread& ppu, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags);
void sys_game_process_exitspawn2(ppu_thread& ppu, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags);
