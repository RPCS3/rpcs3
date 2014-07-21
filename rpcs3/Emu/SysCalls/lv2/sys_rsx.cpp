#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_rsx.h"

SysCallBase sys_rsx("sys_rsx");

s32 sys_rsx_device_open()
{
	sys_rsx.Todo("sys_rsx_device_open()");
	return CELL_OK;
}

s32 sys_rsx_device_close()
{
	sys_rsx.Todo("sys_rsx_device_close()");
	return CELL_OK;
}

s32 sys_rsx_memory_allocate()
{
	sys_rsx.Todo("sys_rsx_memory_allocate()");
	return CELL_OK;
}

s32 sys_rsx_memory_free()
{
	sys_rsx.Todo("sys_rsx_memory_free()");
	return CELL_OK;
}

s32 sys_rsx_context_allocate()
{
	sys_rsx.Todo("sys_rsx_context_allocate()");
	return CELL_OK;
}

s32 sys_rsx_context_free()
{
	sys_rsx.Todo("sys_rsx_context_free()");
	return CELL_OK;
}

s32 sys_rsx_context_iomap()
{
	sys_rsx.Todo("sys_rsx_context_iomap()");
	return CELL_OK;
}

s32 sys_rsx_context_iounmap()
{
	sys_rsx.Todo("sys_rsx_context_iounmap()");
	return CELL_OK;
}

s32 sys_rsx_context_attribute(s32 context_id, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	sys_rsx.Todo("sys_rsx_context_attribute(context_id=%d, a2=%llu, a3=%llu, a4=%llu, a5=%llu, a6=%llu)", context_id, a2, a3, a4, a5, a6);
	return CELL_OK;
}

s32 sys_rsx_device_map(mem32_t a1, mem32_t a2, u32 a3)
{
	sys_rsx.Todo("sys_rsx_device_map(a1_addr=0x%x, a2_addr=0x%x, a3=%d)", a1.GetAddr(), a2.GetAddr(), a3);
	return CELL_OK;
}

s32 sys_rsx_device_unmap()
{
	sys_rsx.Todo("sys_rsx_device_unmap()");
	return CELL_OK;
}

s32 sys_rsx_attribute()
{
	sys_rsx.Todo("sys_rsx_attribute()");
	return CELL_OK;
}
