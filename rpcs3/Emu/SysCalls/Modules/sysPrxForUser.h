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

// SysCalls
vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size);
