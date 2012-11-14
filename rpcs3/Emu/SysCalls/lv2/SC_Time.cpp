#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include <sys/timeb.h>

SysCallBase sys_time("sys_time");
static const u64 timebase_frequency = 79800000;

int sys_time_get_current_time(u32 sec_addr, u32 nsec_addr)
{
	sys_time.Log("sys_time_get_current_time(sec_addr=0x%x, nsec_addr=0x%x)", sec_addr, nsec_addr);
	__timeb64 cur_time;
	_ftime64_s(&cur_time);

	Memory.Write64(sec_addr, cur_time.time);
	Memory.Write64(nsec_addr, (u64)cur_time.millitm * 1000000);

	return CELL_OK;
}

s64 sys_time_get_system_time()
{
	sys_time.Log("sys_time_get_system_time()");
	LARGE_INTEGER cycle;
	QueryPerformanceCounter(&cycle);
	return cycle.QuadPart;
}

u64 sys_time_get_timebase_frequency()
{
	sys_time.Log("sys_time_get_timebase_frequency()");

	static LARGE_INTEGER frequency = {0ULL};

	if(!frequency.QuadPart) QueryPerformanceFrequency(&frequency);

	return frequency.QuadPart;
}