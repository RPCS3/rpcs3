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

#include "Threading.h"
#include "Linux.h"
#include "../x86/ix86/ix86.h"

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
			cpuinfo.LogicalCores = numCPU;
			cpuinfo.PhysicalCores = ( numCPU / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
		}
		else
		{
			// Indeterminate?
			cpuinfo.LogicalCores = 1;
			cpuinfo.PhysicalCores = 1;
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
	
	void* Thread::_internal_callback( void* itsme )
	{
		jASSUME( itsme != NULL );

		Thread& owner = *((Thread*)itsme);
		owner.m_returncode = owner.Callback();
		owner.m_terminated = true;

		return NULL;
	}

	/////////////////////////////////////////////////////////////////////////
	// Cross-platform atomic operations for GCC.
	// These are much faster than the old versions for single core CPUs.
	// Note, I've disabled the single core optimization, because pcsx2 shouldn't
	// ever create threads on single core CPUs anyway.

	__forceinline long pcsx2_InterlockedExchange(volatile long* Target, long Value)
	{
		long result;
		/*
		 * The XCHG instruction always locks the bus with or without the
		 * LOCKED prefix. This makes it significantly slower than CMPXCHG on
		 * uni-processor machines. The Windows InterlockedExchange function
		 * is nearly 3 times faster than the XCHG instruction, so this routine
		 * is not yet very useful for speeding up pthreads.
		 */


		if( true ) //isMultiCore )
		{
			__asm__ __volatile__ (
				"xchgl          %2,%1"
				:"=r" (result)
				:"m"  (*Target), "0" (Value));
		}
		else
		{
		  /*
		   * Faster version of XCHG for uni-processor systems because
		   * it doesn't lock the bus. If an interrupt or context switch
		   * occurs between the movl and the cmpxchgl then the value in
		   * 'location' may have changed, in which case we will loop
		   * back to do the movl again.
		   */

			__asm__ __volatile__ (
				"0:\n\t"
				"movl           %1,%%eax\n\t"
				"cmpxchgl       %2,%1\n\t"
				"jnz            0b"
				:"=&a" (result)
				:"m"  (*Target), "r" (Value));
		}
		
		return result;
	}

	__forceinline long pcsx2_InterlockedExchangeAdd(volatile long* Addend, long Value)
	{
		if( true ) //isMultiCore )
		{
			__asm__ __volatile__(
				".intel_syntax noprefix\n"
				 "lock xadd [%0], eax\n"
				 ".att_syntax\n" : : "r"(Addend), "a"(Value) : "memory");
		}
		else
		{
			__asm__ __volatile__(
				".intel_syntax noprefix\n"
				 "xadd [%0], eax\n"
				 ".att_syntax\n" : : "r"(Addend), "a"(Value) : "memory");
		}
	}

	__forceinline long pcsx2_InterlockedCompareExchange(volatile long *dest, long value, long comp)
	{
		long result;

		if( true ) //isMultiCore )
		{
			__asm__ __volatile__ (
				"lock\n\t"
				"cmpxchgl       %2,%1"      /* if (EAX == [location])  */
											/*   [location] = value    */
											/* else                    */
											/*   EAX = [location]      */
			:"=a" (result)
			:"m"  (*dest), "r" (value), "a" (comp));
		}
		else
		{
		  __asm__ __volatile__ (
				"cmpxchgl       %2,%1"      /* if (EAX == [location])  */
											/*   [location] = value    */
											/* else                    */
											/*   EAX = [location]      */
				:"=a" (result)
				:"m"  (*dest), "r" (value), "a" (comp));
		}
		
		return result;
	}

	#ifdef __x86_64__
	__forceinline void pcsx2_InterlockedExchange64(volatile s64* Target, s64 Value)
	{
		__asm__ __volatile__(
			".intel_syntax noprefix\n"
			 "lock xchg [%0], rax\n"
			 ".att_syntax\n" : : "r"(Target), "a"(Value) : "memory"
		);
		return 0;
	}

	__forceinline s64 pcsx2_InterlockedCompareExchange64(volatile s64* dest, s64 exch, s64 comp)
	{
		s64 old;
		__asm__ __volatile__( 
			"lock; cmpxchgq %q2, %q1"
			: "=a" (old)
			: "r" (exch), "m" (*dest), "a" (comp)
		);
		return old;
	}
#endif
	
}
