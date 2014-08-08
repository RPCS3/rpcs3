#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_trace.h"

SysCallBase sys_trace("sys_trace");

s32 sys_trace_create()
{
	sys_trace.Todo("sys_trace_create()");
	return CELL_OK;
}

s32 sys_trace_start()
{
	sys_trace.Todo("sys_trace_start()");
	return CELL_OK;
}

s32 sys_trace_stop()
{
	sys_trace.Todo("sys_trace_stop()");
	return CELL_OK;
}

s32 sys_trace_update_top_index()
{
	sys_trace.Todo("sys_trace_update_top_index()");
	return CELL_OK;
}

s32 sys_trace_destroy()
{
	sys_trace.Todo("sys_trace_destroy()");
	return CELL_OK;
}

s32 sys_trace_drain()
{
	sys_trace.Todo("sys_trace_drain()");
	return CELL_OK;
}

s32 sys_trace_attach_process()
{
	sys_trace.Todo("sys_trace_attach_process()");
	return CELL_OK;
}

s32 sys_trace_allocate_buffer()
{
	sys_trace.Todo("sys_trace_allocate_buffer()");
	return CELL_OK;
}

s32 sys_trace_free_buffer()
{
	sys_trace.Todo("sys_trace_free_buffer()");
	return CELL_OK;
}

s32 sys_trace_create2()
{
	sys_trace.Todo("sys_trace_create2()");
	return CELL_OK;
}
