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

#ifndef _SAMPLPROF_H_
#define _SAMPLPROF_H_

#include "Common.h"

// The profiler does not have a Linux version yet.
// So for now we turn it into duds for non-Win32 platforms.

#ifdef WIN32

void ProfilerInit();
void ProfilerTerm();
void ProfilerSetEnabled(bool Enabled);
void ProfilerRegisterSource(const char* Name, const void* buff, u32 sz);
void ProfilerRegisterSource(const char* Name, const void* function);
void ProfilerTerminateSource( const char* Name );

void ProfilerRegisterSource(const wxString& Name, const void* buff, u32 sz);
void ProfilerRegisterSource(const wxString& Name, const void* function);
void ProfilerTerminateSource( const wxString& Name );

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
