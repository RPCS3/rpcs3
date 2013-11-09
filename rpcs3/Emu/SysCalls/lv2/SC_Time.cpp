#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include <sys/timeb.h>

SysCallBase sys_time("sys_time");
static const u64 timebase_frequency = 79800000;
extern int cellSysutilGetSystemParamInt(int id, mem32_t value);

int sys_time_get_timezone(mem32_t timezone, mem32_t summertime)
{
	int ret;
	ret = cellSysutilGetSystemParamInt(0x0116, timezone);   //0x0116 = CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE
	if (ret != CELL_OK) return ret;
	ret = cellSysutilGetSystemParamInt(0x0117, summertime); //0x0117 = CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE
	if (ret != CELL_OK) return ret;
	return CELL_OK;
}

int sys_time_get_current_time(u32 sec_addr, u32 nsec_addr)
{
	sys_time.Log("sys_time_get_current_time(sec_addr=0x%x, nsec_addr=0x%x)", sec_addr, nsec_addr);

	u64 time = sys_time_get_system_time();

	Memory.Write64(sec_addr, time / 1000000);
	Memory.Write64(nsec_addr, time % 1000000);

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