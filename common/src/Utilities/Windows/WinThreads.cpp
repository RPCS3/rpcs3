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

		cpuinfo.LogicalCores = 1;

		if( !GetProcessAffinityMask (GetCurrentProcess (),
			&vProcessCPUs, &vSystemCPUs) ) return;

		int CPUs = 0;
		DWORD bit;

		for (bit = 1; bit != 0; bit <<= 1)
		{
			if (vSystemCPUs & bit)
				CPUs++;
		}

		cpuinfo.LogicalCores = CPUs;
		if( LogicalCoresPerPhysicalCPU > CPUs) // for 1-socket HTT-disabled machines
			LogicalCoresPerPhysicalCPU = CPUs;

		cpuinfo.PhysicalCores = ( CPUs / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
		//ptw32_smp_system = ( cpuinfo.LogicalCores > 1 ) ? TRUE : FALSE;
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

	void* Thread::_internal_callback( void* itsme )
	{
		jASSUME( itsme != NULL );

		//pthread_win32_thread_attach_np();

		Thread& owner = *((Thread*)itsme);
		owner.m_returncode = owner.Callback();
		owner.m_terminated = true;

		//pthread_win32_thread_detach_np();

		return NULL;
	}
}

