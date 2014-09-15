/*
* This file contains Nt monotonic counter code, taken from wine which is:
* Copyright 2002 Rex Jolliff (rex@lvcablemodem.com)
* Copyright 1999 Juergen Schmied
* Copyright 2007 Dmitry Timoshkov
* GNU LGPL 2.1 license
* */
#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include "sys_time.h"

SysCallBase sys_time("sys_time");

static const u64 timebase_frequency = /*79800000*/ 80000000; // 80 Mhz
extern int cellSysutilGetSystemParamInt(int id, vm::ptr<be_t<u32>> value);

// Auxiliary functions
u64 get_time()
{
#ifdef _WIN32
	static struct PerformanceFreqHolder
	{
		u64 value;

		PerformanceFreqHolder()
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			value = freq.QuadPart;
		}
	} freq;

	LARGE_INTEGER cycle;
	QueryPerformanceCounter(&cycle);
	u64 sec = cycle.QuadPart / freq.value;
	return sec * timebase_frequency + (cycle.QuadPart % freq.value) * timebase_frequency / freq.value;
#else
	struct timespec ts;
	if (!clock_gettime(CLOCK_MONOTONIC, &ts))
		return ts.tv_sec * (s64)timebase_frequency + (s64)ts.tv_nsec * (s64)timebase_frequency / 1000000000;

	// Should never occur.
	assert(0);
	return 0;
#endif
}

// Returns some relative time in microseconds, don't change this fact
u64 get_system_time()
{
	return get_time() / (timebase_frequency / MHZ);
}


// Functions
s32 sys_time_get_timezone(vm::ptr<be_t<u32>> timezone, vm::ptr<be_t<u32>> summertime)
{
	sys_time.Warning("sys_time_get_timezone(timezone_addr=0x%x, summertime_addr=0x%x)", timezone.addr(), summertime.addr());

	int ret;
	ret = cellSysutilGetSystemParamInt(0x0116, timezone);   //0x0116 = CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE
	if (ret != CELL_OK) return ret;
	ret = cellSysutilGetSystemParamInt(0x0117, summertime); //0x0117 = CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE
	if (ret != CELL_OK) return ret;
	return CELL_OK;
}

s32 sys_time_get_current_time(u32 sec_addr, u32 nsec_addr)
{
	sys_time.Log("sys_time_get_current_time(sec_addr=0x%x, nsec_addr=0x%x)", sec_addr, nsec_addr);

	u64 time = get_time();

	vm::write64(sec_addr, time / timebase_frequency);
	vm::write64(nsec_addr, (time % timebase_frequency) * 1000000000 / (s64)(timebase_frequency));

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
	return timebase_frequency;
}
