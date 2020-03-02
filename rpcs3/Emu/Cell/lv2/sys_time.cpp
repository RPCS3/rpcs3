#include "stdafx.h"
#include "sys_time.h"

#include "Emu/system_config.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Utilities/asm.h"

#ifdef _WIN32

#include <Windows.h>

struct time_aux_info_t
{
	u64 perf_freq;
	u64 start_time;
	u64 start_ftime; // time in 100ns units since Epoch
};

// Initialize time-related values
const auto s_time_aux_info = []() -> time_aux_info_t
{
	LARGE_INTEGER freq;
	if (!QueryPerformanceFrequency(&freq))
	{
		MessageBox(0, L"Your hardware doesn't support a high-resolution performance counter", L"Error", MB_OK | MB_ICONERROR);
		return {};
	}

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start); // get time in 1/perf_freq units from RDTSC

	FILETIME ftime;
	GetSystemTimeAsFileTime(&ftime); // get time in 100ns units since January 1, 1601 (UTC)

	time_aux_info_t result;
	result.perf_freq   = freq.QuadPart;
	result.start_time  = start.QuadPart;
	result.start_ftime = (ftime.dwLowDateTime | (u64)ftime.dwHighDateTime << 32) - 116444736000000000;

	return result;
}();

#elif __APPLE__

// XXX only supports a single timer
#define TIMER_ABSTIME -1
// The opengroup spec isn't clear on the mapping from REALTIME to CALENDAR being appropriate or not.
// http://pubs.opengroup.org/onlinepubs/009695299/basedefs/time.h.html
#define CLOCK_REALTIME  1 // #define CALENDAR_CLOCK 1 from mach/clock_types.h
#define CLOCK_MONOTONIC 0 // #define SYSTEM_CLOCK 0

// the mach kernel uses struct mach_timespec, so struct timespec is loaded from <sys/_types/_timespec.h> for compatability
// struct timespec { time_t tv_sec; long tv_nsec; };

#include <sys/types.h>
#include <sys/_types/_timespec.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#undef CPU_STATE_MAX

#define MT_NANO (+1.0E-9)
#define MT_GIGA UINT64_C(1000000000)

// TODO create a list of timers,
static double mt_timebase = 0.0;
static uint64_t mt_timestart = 0;

static int clock_gettime(int clk_id, struct timespec* tp)
{
	kern_return_t retval = KERN_SUCCESS;
	if (clk_id == TIMER_ABSTIME)
	{
		if (!mt_timestart)
		{
			// only one timer, initilized on the first call to the TIMER
			mach_timebase_info_data_t tb = {0};
			mach_timebase_info(&tb);
			mt_timebase = tb.numer;
			mt_timebase /= tb.denom;
			mt_timestart = mach_absolute_time();
		}

		double diff = (mach_absolute_time() - mt_timestart) * mt_timebase;
		tp->tv_sec  = diff * MT_NANO;
		tp->tv_nsec = diff - (tp->tv_sec * MT_GIGA);
	}
	else // other clk_ids are mapped to the coresponding mach clock_service
	{
		clock_serv_t cclock;
		mach_timespec_t mts;

		host_get_clock_service(mach_host_self(), clk_id, &cclock);
		retval = clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);

		tp->tv_sec  = mts.tv_sec;
		tp->tv_nsec = mts.tv_nsec;
	}

	return retval;
}

#endif

#ifndef _WIN32

static struct timespec start_time = []()
{
	struct timespec ts;

	if (::clock_gettime(CLOCK_REALTIME, &ts) != 0)
	{
		// Fatal error
		std::terminate();
	}

	return ts;
}();

#endif

LOG_CHANNEL(sys_time);

static constexpr u64 g_timebase_freq = /*79800000*/ 80000000ull; // 80 Mhz

// Auxiliary functions
u64 get_timebased_time()
{
#ifdef _WIN32
	LARGE_INTEGER count;
	verify(HERE), QueryPerformanceCounter(&count);

	const u64 time = count.QuadPart;
	const u64 freq = s_time_aux_info.perf_freq;

	return (time / freq * g_timebase_freq + time % freq * g_timebase_freq / freq) * g_cfg.core.clocks_scale / 100u;
#else
	struct timespec ts;
	verify(HERE), ::clock_gettime(CLOCK_MONOTONIC, &ts) == 0;

	return (static_cast<u64>(ts.tv_sec) * g_timebase_freq + static_cast<u64>(ts.tv_nsec) * g_timebase_freq / 1000000000ull) * g_cfg.core.clocks_scale / 100u;
#endif
}

// Returns some relative time in microseconds, don't change this fact
u64 get_system_time()
{
	while (true)
	{
#ifdef _WIN32
		LARGE_INTEGER count;
		verify(HERE), QueryPerformanceCounter(&count);

		const u64 time = count.QuadPart;
		const u64 freq = s_time_aux_info.perf_freq;

		const u64 result = time / freq * 1000000ull + (time % freq) * 1000000ull / freq;
#else
		struct timespec ts;
		verify(HERE), ::clock_gettime(CLOCK_MONOTONIC, &ts) == 0;

		const u64 result = static_cast<u64>(ts.tv_sec) * 1000000ull + static_cast<u64>(ts.tv_nsec) / 1000u;
#endif

		if (result) return result;
	}
}

// As get_system_time but obeys Clocks scaling setting
u64 get_guest_system_time()
{
	return get_system_time() * g_cfg.core.clocks_scale / 100;
}

// Functions
error_code sys_time_get_timezone(vm::ptr<s32> timezone, vm::ptr<s32> summertime)
{
	sys_time.warning("sys_time_get_timezone(timezone=*0x%x, summertime=*0x%x)", timezone, summertime);

	*timezone   = 180;
	*summertime = 0;

	return CELL_OK;
}

error_code sys_time_get_current_time(vm::ptr<s64> sec, vm::ptr<s64> nsec)
{
	sys_time.trace("sys_time_get_current_time(sec=*0x%x, nsec=*0x%x)", sec, nsec);

	if (!sec)
	{
		return CELL_EFAULT;
	}

#ifdef _WIN32
	LARGE_INTEGER count;
	verify(HERE), QueryPerformanceCounter(&count);

	const u64 diff_base = count.QuadPart - s_time_aux_info.start_time;

	// Get time difference in nanoseconds (using 128 bit accumulator)
	const u64 diff_sl = diff_base * 1000000000ull;
	const u64 diff_sh = utils::umulh64(diff_base, 1000000000ull);
	const u64 diff = utils::udiv128(diff_sh, diff_sl, s_time_aux_info.perf_freq);

	// get time since Epoch in nanoseconds
	const u64 time = s_time_aux_info.start_ftime * 100u + (diff * g_cfg.core.clocks_scale / 100u);

	*sec  = time / 1000000000ull;

	if (!nsec)
	{
		return CELL_EFAULT;
	}

	*nsec = time % 1000000000ull;
#else
	struct timespec ts;
	verify(HERE), ::clock_gettime(CLOCK_REALTIME, &ts) == 0;

	if (g_cfg.core.clocks_scale == 100)
	{
		*sec  = ts.tv_sec;

		if (!nsec)
		{
			return CELL_EFAULT;
		}

		*nsec = ts.tv_nsec;
		return CELL_OK;
	}

	u64 tv_sec = ts.tv_sec, stv_sec = start_time.tv_sec;
	u64 tv_nsec = ts.tv_nsec, stv_nsec = start_time.tv_nsec;

	// Substruct time since Epoch and since start time
	tv_sec -= stv_sec;

	if (tv_nsec < stv_nsec)
	{
		// Correct value if borrow encountered
		tv_sec -= 1;
		tv_nsec = 1'000'000'000ull - (stv_nsec - tv_nsec);
	}
	else
	{
		tv_nsec -= stv_nsec;
	}

	// Scale nanocseconds
	tv_nsec = stv_nsec + (tv_nsec * g_cfg.core.clocks_scale / 100);

	// Scale seconds and add from nanoseconds / 1'000'000'000
	*sec  = stv_sec + (tv_sec * g_cfg.core.clocks_scale / 100u) + (tv_nsec / 1000000000ull);

	if (!nsec)
	{
		return CELL_EFAULT;
	}

	// Set nanoseconds
	*nsec = tv_nsec % 1000000000ull;
#endif

	return CELL_OK;
}

u64 sys_time_get_timebase_frequency()
{
	sys_time.trace("sys_time_get_timebase_frequency()");

	return g_timebase_freq;
}

error_code sys_time_get_rtc(vm::ptr<u64> rtc)
{
	sys_time.todo("sys_time_get_rtc(rtc=*0x%x)", rtc);

	return CELL_OK;
}
