#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_time.h"

#ifdef _WIN32

#include <Windows.h>

struct time_aux_info_t
{
	u64 perf_freq;
	u64 start_time;
	u64 start_ftime; // time in 100ns units since Epoch
}
const g_time_aux_info = []() -> time_aux_info_t // initialize time-related values (Windows-only)
{
	LARGE_INTEGER freq;
	if (!QueryPerformanceFrequency(&freq))
	{
		MessageBox(0, L"Your hardware doesn't support a high-resolution performance counter", L"Error", MB_OK | MB_ICONERROR);
		return{};
	}

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start); // get time in 1/perf_freq units from RDTSC

	FILETIME ftime;
	GetSystemTimeAsFileTime(&ftime); // get time in 100ns units since January 1, 1601 (UTC)

	time_aux_info_t result;
	result.perf_freq = freq.QuadPart;
	result.start_time = start.QuadPart;
	result.start_ftime = (ftime.dwLowDateTime | (u64)ftime.dwHighDateTime << 32) - 116444736000000000;

	return result;
}();

#else

#include "errno.h"

#endif

SysCallBase sys_time("sys_time");

static const u64 g_timebase_freq = /*79800000*/ 80000000; // 80 Mhz

// Auxiliary functions
u64 get_timebased_time()
{
#ifdef _WIN32
	LARGE_INTEGER count;
	if (!QueryPerformanceCounter(&count))
	{
		throw EXCEPTION("Unexpected");
	}

	const u64 time = count.QuadPart;
	const u64 freq = g_time_aux_info.perf_freq;

	return time / freq * g_timebase_freq + time % freq * g_timebase_freq / freq;
#else
	struct timespec ts;
	if (::clock_gettime(CLOCK_MONOTONIC, &ts))
	{
		throw EXCEPTION("System error %d", errno);
	}
	
	return static_cast<u64>(ts.tv_sec) * g_timebase_freq + static_cast<u64>(ts.tv_nsec) * g_timebase_freq / 1000000000u;
#endif
}

// Returns some relative time in microseconds, don't change this fact
u64 get_system_time()
{
	while (true)
	{
#ifdef _WIN32
		LARGE_INTEGER count;
		if (!QueryPerformanceCounter(&count))
		{
			throw EXCEPTION("Unexpected");
		}

		const u64 time = count.QuadPart;
		const u64 freq = g_time_aux_info.perf_freq;

		const u64 result = time / freq * 1000000u + (time % freq) * 1000000u / freq;
#else
		struct timespec ts;
		if (::clock_gettime(CLOCK_MONOTONIC, &ts))
		{
			throw EXCEPTION("System error %d", errno);
		}

		const u64 result = static_cast<u64>(ts.tv_sec) * 1000000u + static_cast<u64>(ts.tv_nsec) / 1000u;
#endif

		if (result) return result;
	}
}

// Functions
s32 sys_time_get_timezone(vm::ptr<s32> timezone, vm::ptr<s32> summertime)
{
	sys_time.Warning("sys_time_get_timezone(timezone=*0x%x, summertime=*0x%x)", timezone, summertime);

	*timezone = 180;
	*summertime = 0;

	return CELL_OK;
}

s32 sys_time_get_current_time(vm::ptr<s64> sec, vm::ptr<s64> nsec)
{
	sys_time.Log("sys_time_get_current_time(sec=*0x%x, nsec=*0x%x)", sec, nsec);

#ifdef _WIN32
	LARGE_INTEGER count;
	if (!QueryPerformanceCounter(&count))
	{
		throw EXCEPTION("Unexpected");
	}

	// get time difference in nanoseconds
	const u64 diff = (count.QuadPart - g_time_aux_info.start_time) * 1000000000u / g_time_aux_info.perf_freq;

	// get time since Epoch in nanoseconds
	const u64 time = g_time_aux_info.start_ftime * 100u + diff;

	*sec = time / 1000000000u;
	*nsec = time % 1000000000u;
#else
	struct timespec ts;
	if (::clock_gettime(CLOCK_REALTIME, &ts))
	{
		throw EXCEPTION("System error %d", errno);
	}

	*sec = ts.tv_sec;
	*nsec = ts.tv_nsec;
#endif

	return CELL_OK;
}

u64 sys_time_get_timebase_frequency()
{
	sys_time.Log("sys_time_get_timebase_frequency()");

	return g_timebase_freq;
}
