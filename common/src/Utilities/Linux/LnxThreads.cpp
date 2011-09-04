/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "../PrecompiledHeader.h"
#include "PersistentThread.h"
#include <sys/prctl.h>

// We wont need this until we actually have this more then just stubbed out, so I'm commenting this out
// to remove an unneeded dependency.
//#include "x86emitter/tools.h"

#if !defined(__LINUX__) && !defined(__WXMAC__)

#	pragma message( "LnxThreads.cpp should only be compiled by projects or makefiles targeted at Linux/Mac distros.")

#else

// Note: assuming multicore is safer because it forces the interlocked routines to use
// the LOCK prefix.  The prefix works on single core CPUs fine (but is slow), but not
// having the LOCK prefix is very bad indeed.

__forceinline void Threading::Sleep( int ms )
{
	usleep( 1000*ms );
}

// For use in spin/wait loops,  Acts as a hint to Intel CPUs and should, in theory
// improve performance and reduce cpu power consumption.
__forceinline void Threading::SpinWait()
{
	// If this doesn't compile you can just comment it out (it only serves as a
	// performance hint and isn't required).
	__asm__ ( "pause" );
}

__forceinline void Threading::EnableHiresScheduler()
{
	// Don't know if linux has a customizable scheduler resolution like Windows (doubtful)
}

__forceinline void Threading::DisableHiresScheduler()
{
}

u64 Threading::GetThreadTicksPerSecond()
{
	// Note the value is not correct but I'm not sure we can do better because 
	// most modern system use a tickless system anyway
	// Besides x86 architecture always returns 100;
	/* A Forum post extract
	   sysconf(SC_CLK_TCK) does not give the frequency of the timer interrupts in Linux. It gives
	   the frequency of jiffies which is visible to userspace in things like the counters in various directories in /proc

	   The actual frequency is hidden from userspace, deliberately. Indeed, some systems
	   use dynamic ticks or "tickless" systems, so there aren't really any at all.
	 */
	u32 hertz = sysconf(_SC_CLK_TCK);
	return hertz;
}

u64 Threading::GetThreadCpuTime()
{
	// Get the cpu time for the current thread.  Should be a measure of total time the
	// thread has used on the CPU (scaled by the value returned by GetThreadTicksPerSecond(),
	// which typically would be an OS-provided scalar or some sort).

	clockid_t cid;
	int err = pthread_getcpuclockid(pthread_self(), &cid);
	if (err) return 0;

	struct timespec ts;
	err = clock_gettime(cid, &ts);
	if (err) return 0;
    unsigned long timeJiff = (ts.tv_sec*1e6 + ts.tv_nsec / 1000)/1e6 * GetThreadTicksPerSecond();

	return timeJiff;
}

u64 Threading::pxThread::GetCpuTime() const
{
	// Get the cpu time for the thread belonging to this object.  Use m_native_id and/or
	// m_native_handle to implement it. Return value should be a measure of total time the
	// thread has used on the CPU (scaled by the value returned by GetThreadTicksPerSecond(),
	// which typically would be an OS-provided scalar or some sort).

	if (!m_native_id) return 0;

	clockid_t cid;
	int err = pthread_getcpuclockid(m_native_id, &cid);
	if (err) return 0;

	struct timespec ts;
	err = clock_gettime(cid, &ts);
	if (err) return 0;
    unsigned long timeJiff = (ts.tv_sec*1e6 + ts.tv_nsec / 1000)/1e6 * GetThreadTicksPerSecond();

	return timeJiff;
}

void Threading::pxThread::_platform_specific_OnStartInThread()
{
	// Obtain linux-specific thread IDs or Handles here, which can be used to query
	// kernel scheduler performance information.
	m_native_id = (uptr) pthread_self();
}

void Threading::pxThread::_platform_specific_OnCleanupInThread()
{
	// Cleanup handles here, which were opened above.
}

void Threading::pxThread::_DoSetThreadName( const char* name )
{
	// Extract of manpage: "The name can be up to 16 bytes long, and should be
	//						null-terminated if it contains fewer bytes."
	prctl(PR_SET_NAME, name, 0, 0, 0);
}

#endif
