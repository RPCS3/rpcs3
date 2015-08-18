#include "GNU.h"

#ifdef __APPLE__
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

// TODO be more careful in a multithreaded environement
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    kern_return_t retval = KERN_SUCCESS;
    if( clk_id == TIMER_ABSTIME)
    {
        if (!mt_timestart) { // only one timer, initilized on the first call to the TIMER
            mach_timebase_info_data_t tb = { 0 };
            mach_timebase_info(&tb);
            mt_timebase = tb.numer;
            mt_timebase /= tb.denom;
            mt_timestart = mach_absolute_time();
        }

        double diff = (mach_absolute_time() - mt_timestart) * mt_timebase;
        tp->tv_sec = diff * MT_NANO;
        tp->tv_nsec = diff - (tp->tv_sec * MT_GIGA);
    }
    else // other clk_ids are mapped to the coresponding mach clock_service
    {
        clock_serv_t cclock;
        mach_timespec_t mts;

        host_get_clock_service(mach_host_self(), clk_id, &cclock);
        retval = clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);

        tp->tv_sec = mts.tv_sec;
        tp->tv_nsec = mts.tv_nsec;
    }

    return retval;
}
#endif /* __APPLE__ */
#if defined(__GNUG__)

void * _aligned_malloc(size_t size, size_t alignment) {
	void *buffer;
	return (posix_memalign(&buffer, alignment, size) == 0) ? buffer : 0;
}
#endif

