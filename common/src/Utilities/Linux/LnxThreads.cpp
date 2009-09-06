/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "../PrecompiledHeader.h"
#include "Threading.h"
#include "x86emitter/tools.h"

// Note: assuming multicore is safer because it forces the interlocked routines to use
// the LOCK prefix.  The prefix works on single core CPUs fine (but is slow), but not
// having the LOCK prefix is very bad indeed.

static bool isMultiCore = true;		// assume more than one CPU (safer)

namespace Threading
{
	// Note: Apparently this solution is Linux/Solaris only.
	// FreeBSD/OsX need something far more complicated (apparently)
	void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU )
	{
		const uint numCPU = sysconf( _SC_NPROCESSORS_ONLN );
		if( numCPU > 0 )
		{
			isMultiCore = numCPU > 1;
			x86caps.LogicalCores = numCPU;
			x86caps.PhysicalCores = ( numCPU / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
		}
		else
		{
			// Indeterminate?
			x86caps.LogicalCores = 1;
			x86caps.PhysicalCores = 1;
		}
	}

	__forceinline void Timeslice()
	{
		usleep(500);
	}

	__forceinline void Sleep( int ms )
	{
		usleep( 1000*ms );
	}

	// For use in spin/wait loops,  Acts as a hint to Intel CPUs and should, in theory
	// improve performance and reduce cpu power consumption.
	__forceinline void SpinWait()
	{
		// If this doesn't compile you can just comment it out (it only serves as a
		// performance hint and isn't required).
		__asm__ ( "pause" );
	}
}
