#ifndef _SAMPLPROF_H_
#define _SAMPLPROF_H_

#include "Common.h"

// The profiler does not have a Linux version yet.
// So for now we turn it into duds for non-Win32 platforms.

#ifdef _WIN32

void ProfilerInit();
void ProfilerTerm();
void ProfilerSetEnabled(bool Enabled);
void ProfilerRegisterSource(const char* Name, const void* buff, u32 sz);
void ProfilerRegisterSource(const char* Name, const void* function);
void ProfilerTerminateSource( const char* Name );

#else

// Disables the profiler in Debug & Linux builds.
// Profiling info in debug builds isn't much use anyway and the console
// popups are annoying when you're trying to trace debug logs and stuff.

#define ProfilerInit() (void)0
#define ProfilerTerm() (void)0
#define ProfilerSetEnabled 0&&
#define ProfilerRegisterSource 0&&
#define ProfilerTerminateSource 0&&

#endif

#endif
