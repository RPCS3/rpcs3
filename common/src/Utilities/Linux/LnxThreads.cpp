/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "Threading.h"
#include "x86emitter/tools.h"

#if !defined(__LINUX__) || !defined(__WXMAC__)

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

void pxYieldToMain()
{
	// Linux/GTK+ Implementation Notes:
	//  I have no idea if wxEventLoop::Pending() is thread safe or not, nor do I have
	//  any idea how to properly obtain the message queue status of GTK+.  So let's
	//  just play dumb (and slow) and sleep for a couple milliseconds regardless, until
	// a better fix is found. --air

	// (FIXME : Find a more correct implementation for this?)
	
	Sleep( 2 );
}

#endif
