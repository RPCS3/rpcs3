# pragma once

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

// SysCalls
vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size);
