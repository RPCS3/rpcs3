/*
 * ptw32_new.c
 *
 * Description:
 * This translation unit provides platform-specific cancelation functionality.
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

#include "ptw32pch.h"

#if defined(_M_IX86) || defined(_X86_)
#define PTW32_PROGCTR(Context)  ((Context).Eip)
#endif

#if defined (_M_IA64)
#define PTW32_PROGCTR(Context)  ((Context).StIIP)
#endif

#if defined(_MIPS_)
#define PTW32_PROGCTR(Context)  ((Context).Fir)
#endif

#if defined(_ALPHA_)
#define PTW32_PROGCTR(Context)  ((Context).Fir)
#endif

#if defined(_PPC_)
#define PTW32_PROGCTR(Context)  ((Context).Iar)
#endif

#if defined(_AMD64_)
#define PTW32_PROGCTR(Context)  ((Context).Rip)
#endif

#if !defined(PTW32_PROGCTR)
#error Module contains CPU-specific code; modify and recompile.
#endif

static void
ptw32_cancel_self (void)
{
  ptw32_throw (PTW32_EPS_CANCEL);

  /* Never reached */
}

static void CALLBACK
ptw32_cancel_callback (DWORD unused)
{
  ptw32_throw (PTW32_EPS_CANCEL);

  /* Never reached */
}

/*
 * ptw32_RegisterCancelation() -
 * Must have args of same type as QueueUserAPCEx because this function
 * is a substitute for QueueUserAPCEx if it's not available.
 */
DWORD
ptw32_RegisterCancelation (PAPCFUNC unused1, HANDLE threadH, DWORD unused2)
{
  CONTEXT context;

  context.ContextFlags = CONTEXT_CONTROL;
  GetThreadContext (threadH, &context);
  PTW32_PROGCTR (context) = (DWORD_PTR) ptw32_cancel_self;
  SetThreadContext (threadH, &context);
  return 0;
}

void ptw32_PrepCancel( ptw32_thread_t* tp )
{
	HANDLE threadH = tp->threadH;

	SuspendThread (threadH);

	if (WaitForSingleObject (threadH, 0) == WAIT_TIMEOUT)
	{
		tp->state = PThreadStateCanceling;
		tp->cancelState = PTHREAD_CANCEL_DISABLE;
		/*
		* If alertdrv and QueueUserAPCEx is available then the following
		* will result in a call to QueueUserAPCEx with the args given, otherwise
		* this will result in a call to ptw32_RegisterCancelation and only
		* the threadH arg will be used.
		*/
		ptw32_register_cancelation (ptw32_cancel_callback, threadH, 0);
		(void) pthread_mutex_unlock (&tp->cancelLock);
		ResumeThread (threadH);
	}
}