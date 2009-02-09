/*
 * ptw32_InterlockedCompareExchange.c
 *
 * Description:
 * This translation unit implements routines which are private to
 * the implementation and may be used throughout it.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 * 
 *      Contact Email: rpj@callisto.canberra.edu.au
 * 
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "pthread.h"
#include "implement.h"


/*
 * ptw32_InterlockedCompareExchange --
 *
 * Originally needed because W9x doesn't support InterlockedCompareExchange.
 * We now use this version wherever possible so we can inline it.
 */

INLINE PTW32_INTERLOCKED_LONG WINAPI
ptw32_InterlockedCompareExchange (volatile PTW32_INTERLOCKED_LPLONG location,
				  PTW32_INTERLOCKED_LONG value,
				  PTW32_INTERLOCKED_LONG comparand)
{

#if defined(__WATCOMC__)
/* Don't report that result is not assigned a value before being referenced */
#pragma disable_message (200)
#endif

  PTW32_INTERLOCKED_LONG result;

  /*
   * Using the LOCK prefix on uni-processor machines is significantly slower
   * and it is not necessary. The overhead of the conditional below is
   * negligible in comparison. Since an optimised DLL will inline this
   * routine, this will be faster than calling the system supplied
   * Interlocked routine, which appears to avoid the LOCK prefix on
   * uniprocessor systems. So one DLL works for all systems.
   */
  if (1) //ptw32_smp_system)

/* *INDENT-OFF* */

#if defined(_M_IX86) || defined(_X86_)

#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && defined(HAVE_TASM32))
#define HAVE_INLINABLE_INTERLOCKED_CMPXCHG
    {
      _asm {
	//PUSH         ecx
	//PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
	MOV          eax,dword ptr [comparand]
	LOCK CMPXCHG dword ptr [ecx],edx
	MOV          dword ptr [result], eax
	//POP          edx
	//POP          ecx
      }
    }
  else
    {
      _asm {
	//PUSH         ecx
	//PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
	MOV          eax,dword ptr [comparand]
	CMPXCHG      dword ptr [ecx],edx
	MOV          dword ptr [result], eax
	//POP          edx
	//POP          ecx
      }
    }

#elif defined(__GNUC__)
#define HAVE_INLINABLE_INTERLOCKED_CMPXCHG

    {
      __asm__ __volatile__
	(
	 "lock\n\t"
	 "cmpxchgl       %2,%1"      /* if (EAX == [location])  */
	                             /*   [location] = value    */
                                     /* else                    */
                                     /*   EAX = [location]      */
	 :"=a" (result)
	 :"m"  (*location), "r" (value), "a" (comparand));
    }
  else
    {
      __asm__ __volatile__
	(
	 "cmpxchgl       %2,%1"      /* if (EAX == [location])  */
	                             /*   [location] = value    */
                                     /* else                    */
                                     /*   EAX = [location]      */
	 :"=a" (result)
	 :"m"  (*location), "r" (value), "a" (comparand));
    }

#endif

#else

  /*
   * If execution gets to here then we're running on a currently
   * unsupported processor or compiler.
   */

  result = 0;

#endif

/* *INDENT-ON* */

  return result;

#if defined(__WATCOMC__)
#pragma enable_message (200)
#endif

}

/*
 * ptw32_InterlockedExchange --
 *
 * We now use this version wherever possible so we can inline it.
 */

INLINE LONG WINAPI
ptw32_InterlockedExchange (volatile PTW32_INTERLOCKED_LPLONG location,
			   LONG value)
{

#if defined(__WATCOMC__)
/* Don't report that result is not assigned a value before being referenced */
#pragma disable_message (200)
#endif

  LONG result;

  /*
   * The XCHG instruction always locks the bus with or without the
   * LOCKED prefix. This makes it significantly slower than CMPXCHG on
   * uni-processor machines. The Windows InterlockedExchange function
   * is nearly 3 times faster than the XCHG instruction, so this routine
   * is not yet very useful for speeding up pthreads.
   */
  if (1) //ptw32_smp_system)

/* *INDENT-OFF* */

#if defined(_M_IX86) || defined(_X86_)

#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && defined(HAVE_TASM32))
#define HAVE_INLINABLE_INTERLOCKED_XCHG

    {
      _asm {
	//PUSH         ecx
	MOV          ecx,dword ptr [location]
	MOV          eax,dword ptr [value]
	XCHG         dword ptr [ecx],eax
	MOV          dword ptr [result], eax
        //POP          ecx
      }
    }
  else
    {
      /*
       * Faster version of XCHG for uni-processor systems because
       * it doesn't lock the bus. If an interrupt or context switch
       * occurs between the MOV and the CMPXCHG then the value in
       * 'location' may have changed, in which case we will loop
       * back to do the MOV again.
       *
       * FIXME! Need memory barriers for the MOV+CMPXCHG combo?
       *
       * Tests show that this routine has almost identical timing
       * to Win32's InterlockedExchange(), which is much faster than
       * using the inlined 'xchg' instruction above, so it's probably
       * doing something similar to this (on UP systems).
       *
       * Can we do without the PUSH/POP instructions?
       */
      _asm {
	//PUSH         ecx
	//PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
L1:	MOV          eax,dword ptr [ecx]
	CMPXCHG      dword ptr [ecx],edx
	JNZ          L1
	MOV          dword ptr [result], eax
	//POP          edx
        //POP          ecx
      }
    }

#elif defined(__GNUC__)
#define HAVE_INLINABLE_INTERLOCKED_XCHG

    {
      __asm__ __volatile__
	(
	 "xchgl          %2,%1"
	 :"=r" (result)
	 :"m"  (*location), "0" (value));
    }
  else
    {
      /*
       * Faster version of XCHG for uni-processor systems because
       * it doesn't lock the bus. If an interrupt or context switch
       * occurs between the movl and the cmpxchgl then the value in
       * 'location' may have changed, in which case we will loop
       * back to do the movl again.
       *
       * FIXME! Need memory barriers for the MOV+CMPXCHG combo?
       *
       * Tests show that this routine has almost identical timing
       * to Win32's InterlockedExchange(), which is much faster than
       * using the an inlined 'xchg' instruction, so it's probably
       * doing something similar to this (on UP systems).
       */
      __asm__ __volatile__
	(
	 "0:\n\t"
	 "movl           %1,%%eax\n\t"
	 "cmpxchgl       %2,%1\n\t"
	 "jnz            0b"
	 :"=&a" (result)
	 :"m"  (*location), "r" (value));
    }

#endif

#else

  /*
   * If execution gets to here then we're running on a currently
   * unsupported processor or compiler.
   */

  result = 0;

#endif

/* *INDENT-ON* */

  return result;

#if defined(__WATCOMC__)
#pragma enable_message (200)
#endif

}


#if 1

#if defined(PTW32_BUILD_INLINED) && defined(HAVE_INLINABLE_INTERLOCKED_CMPXCHG)
#undef PTW32_INTERLOCKED_COMPARE_EXCHANGE
#define PTW32_INTERLOCKED_COMPARE_EXCHANGE ptw32_InterlockedCompareExchange
#endif

#if defined(PTW32_BUILD_INLINED) && defined(HAVE_INLINABLE_INTERLOCKED_XCHG)
#undef PTW32_INTERLOCKED_EXCHANGE
#define PTW32_INTERLOCKED_EXCHANGE ptw32_InterlockedExchange
#endif

#endif
