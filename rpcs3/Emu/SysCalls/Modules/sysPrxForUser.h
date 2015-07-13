#pragma once

struct HeapInfo
{
	const std::string name;

	HeapInfo(const char* name)
		: name(name)
	{
	}
};

using spu_printf_cb_t = vm::ptr<s32(u32 arg)>;

// Aux
extern spu_printf_cb_t spu_printf_agcb;
extern spu_printf_cb_t spu_printf_dgcb;
extern spu_printf_cb_t spu_printf_atcb;
extern spu_printf_cb_t spu_printf_dtcb;

// Functions
vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size);

struct sys_lwmutex_t;
struct sys_lwmutex_attribute_t;

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
s32 sys_lwmutex_lock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
s32 sys_lwmutex_trylock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_unlock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_destroy(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);

struct sys_lwcond_t;
struct sys_lwcond_attribute_t;

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr);
s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_all(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_to(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id);
s32 sys_lwcond_wait(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 timeout);
