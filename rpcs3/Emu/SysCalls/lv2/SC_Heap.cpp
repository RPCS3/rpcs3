#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_heap("sys_heap");

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

int sys_heap_create_heap(const u32 heap_addr, const u32 align, const u32 size)
{
	sc_heap.Warning("sys_heap_create_heap(heap_addr=0x%x, align=0x%x, size=0x%x)", heap_addr, align, size);
	return Emu.GetIdManager().GetNewID(sc_heap.GetName(), new HeapInfo(heap_addr, align, size));
}

int sys_heap_malloc(const u32 heap_id, const u32 size)
{
	sc_heap.Warning("sys_heap_malloc(heap_id=0x%x, size=0x%x)", heap_id, size);
	if(!Emu.GetIdManager().CheckID(heap_id)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(heap_id);
	if(!!id.m_name.Cmp(sc_heap.GetName())) return CELL_ESRCH;
	const HeapInfo& heap = *(HeapInfo*)id.m_data;

	return Memory.Alloc(size, heap.align);
}