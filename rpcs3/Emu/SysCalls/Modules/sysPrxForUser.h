#pragma once

struct HeapInfo
{
	u32 heap_addr;
	u32 align;
	u32 size;

	HeapInfo(u32 _heap_addr, u32 _align, u32 _size)
		: heap_addr(_heap_addr)
		, align(_align)
		, size(_size)
	{
	}
};

typedef s32(spu_printf_cb_t)(u32 arg);

// Aux
extern vm::ptr<spu_printf_cb_t> spu_printf_agcb;
extern vm::ptr<spu_printf_cb_t> spu_printf_dgcb;
extern vm::ptr<spu_printf_cb_t> spu_printf_atcb;
extern vm::ptr<spu_printf_cb_t> spu_printf_dtcb;

// Functions
vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size);

struct sys_lwmutex_t;
struct sys_lwmutex_attribute_t;

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
s32 sys_lwmutex_lock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
s32 sys_lwmutex_trylock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_unlock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_destroy(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
