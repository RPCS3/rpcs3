/*
 * pthread_testcancel.c
 *
 * Description:
 * POSIX thread functions related to thread cancellation.
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

INLINE void
pthread_testcancel (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a deferred cancellation point
      *      in the calling thread. The call has no effect if the
      *      current cancelability state is
      *              PTHREAD_CANCEL_DISABLE
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function creates a deferred cancellation point
      *      in the calling thread. The call has no effect if the
      *      current cancelability state is
      *              PTHREAD_CANCEL_DISABLE
      *
      *      NOTES:
      *      1)      Cancellation is asynchronous. Use pthread_join
      *              to wait for termination of thread if necessary
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  ptw32_thread_t * sp;
  if( ptw32_testcancel_enable == 0 ) return;

  // Don't use pthread_self here, to avoid unnecessary object query or creation.
  // If ptw32_selfThread is NULL, then it's a sure bet no one's tried to cancel
  // this thread, since that would have spawned a selfThread object! -- air

	sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

  if (sp == NULL)
    {
      return;
    }

  /*
   * Pthread_cancel() will have set sp->state to PThreadStateCancelPending
   * and set an event, so no need to enter kernel space if
   * sp->state != PThreadStateCancelPending - that only slows us down.
   */
  if (sp->state != PThreadStateCancelPending)
    {
      return;
    }

  (void) pthread_mutex_lock (&sp->cancelLock);

  if (sp->cancelState != PTHREAD_CANCEL_DISABLE)
    {
      ResetEvent(sp->cancelEvent);
      sp->state = PThreadStateCanceling;
      (void) pthread_mutex_unlock (&sp->cancelLock);
      sp->cancelState = PTHREAD_CANCEL_DISABLE;
      (void) pthread_mutex_unlock (&sp->cancelLock);
      ptw32_throw (PTW32_EPS_CANCEL);
    }

  (void) pthread_mutex_unlock (&sp->cancelLock);
}				/* pthread_testcancel */
