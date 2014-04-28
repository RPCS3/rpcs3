/*
* This file contains Nt monotonic counter code, taken from wine which is:
* Copyright 2002 Rex Jolliff (rex@lvcablemodem.com)
* Copyright 1999 Juergen Schmied
* Copyright 2007 Dmitry Timoshkov
* GNU LGPL 2.1 license
* */
#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Time.h"

SysCallBase sys_time("sys_time");

//static const u64 timebase_frequency = 79800000;
extern int cellSysutilGetSystemParamInt(int id, mem32_t value);

// Auxiliary functions
u64 get_time()
{
#ifdef _WIN32
	LARGE_INTEGER cycle;
	LARGE_INTEGER freq;
	QueryPerformanceCounter(&cycle);
	QueryPerformanceFrequency(&freq);
	return cycle.QuadPart * 10000000 / freq.QuadPart;
#else
	struct timespec ts;
	if (!clock_gettime(CLOCK_MONOTONIC, &ts))
		return ts.tv_sec * (s64)10000000 + (s64)ts.tv_nsec / (s64)100;

	// Should never occur.
	assert(0);
	return 0;
#endif
}

// Returns some relative time in microseconds, don't change this fact
u64 get_system_time()
{
	return get_time() / 10;
}


// Functions
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

	u64 time = get_time();

	Memory.Write64(sec_addr, time / 10000000);
	Memory.Write64(nsec_addr, (time % 10000000) * 100);

	return CELL_OK;
}

s64 sys_time_get_system_time()
{
	sys_time.Log("sys_time_get_system_time()");
	return get_system_time();
}

u64 sys_time_get_timebase_frequency()
{
	sys_time.Log("sys_time_get_timebase_frequency()");
	return 10000000;
}
