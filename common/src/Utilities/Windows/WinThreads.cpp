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
 
#include "PrecompiledHeader.h"
#include "RedtapeWindows.h"
#include "x86emitter/tools.h"
#include "Threading.h"


#ifdef _WIN32
#include "implement.h"		// win32 pthreads implementations.
#endif

namespace Threading
{
	void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU )
	{
		DWORD vProcessCPUs;
		DWORD vSystemCPUs;

		x86caps.LogicalCores = 1;

		if( !GetProcessAffinityMask (GetCurrentProcess (),
			&vProcessCPUs, &vSystemCPUs) ) return;

		int CPUs = 0;
		DWORD bit;

		for (bit = 1; bit != 0; bit <<= 1)
		{
			if (vSystemCPUs & bit)
				CPUs++;
		}

		x86caps.LogicalCores = CPUs;
		if( LogicalCoresPerPhysicalCPU > CPUs) // for 1-socket HTT-disabled machines
			LogicalCoresPerPhysicalCPU = CPUs;

		x86caps.PhysicalCores = ( CPUs / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
		//ptw32_smp_system = ( x86caps.LogicalCores > 1 ) ? TRUE : FALSE;
	}

	__forceinline void Timeslice()
	{
		::Sleep(0);
	}

	__forceinline void Sleep( int ms )
	{
		::Sleep( ms );
	}

	// For use in spin/wait loops,  Acts as a hint to Intel CPUs and should, in theory
	// improve performance and reduce cpu power consumption.
	__forceinline void SpinWait()
	{
		__asm { pause };
	}
}

