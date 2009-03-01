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


#include "Win32.h"

#include "System.h"
#include "Threading.h"
#include "ix86/ix86.h"

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
		ptw32_smp_system = ( cpuinfo.LogicalCores > 1 ) ? TRUE : FALSE;
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

		pthread_win32_thread_attach_np();

		Thread& owner = *((Thread*)itsme);
		owner.m_returncode = owner.Callback();
		owner.m_terminated = true;

		pthread_win32_thread_detach_np();

		return NULL;
	}

	// Note: assuming multicore is safer because it forces the interlocked routines to use
	// the LOCK prefix.  The prefix works on single core CPUs fine (but is slow), but not
	// having the LOCK prefix is very bad indeed.

	//////////////////////////////////////////////////////////////////////
	// Win32 versions of InterlockedExchange.
	// These are much faster than the Win32 Kernel versions thanks to inlining.

	__forceinline long pcsx2_InterlockedExchange( volatile long* target, long srcval )
	{
		// Use the pthreads-win32 implementation...
		return ptw32_InterlockedExchange( target, srcval );
	}

	__forceinline long pcsx2_InterlockedCompareExchange( volatile long* target, long srcval, long comp )
	{
		// Use the pthreads-win32 implementation...
		return ptw32_InterlockedCompareExchange( target, srcval, comp );
	}

	__forceinline long pcsx2_InterlockedExchangeAdd( volatile long* target, long srcval )
	{
		long result;

		// Use our own implementation...
		// Pcsx2 won't use threads unless it's a multicore cpu, so no need to use
		// the optimized single-core method.

		if( true ) //ptw32_smp_system )
		{
			__asm
			{
				//PUSH         ecx
				mov          ecx,dword ptr [target]
				mov          eax,dword ptr [srcval]
				lock xadd    dword ptr [ecx],eax
				mov          dword ptr [result], eax
				//POP          ecx
			}
		}
		else
		{
			__asm
			{
				//PUSH         ecx
				//PUSH         edx
				mov          ecx,dword ptr [target]
			//L1:
				mov          eax,dword ptr [srcval]
				xadd         dword ptr [ecx],eax
				//jnz          L1
				mov          dword ptr [result], eax
				//POP          edx
				//POP          ecx
			}
		}
		return result;
	}
}
