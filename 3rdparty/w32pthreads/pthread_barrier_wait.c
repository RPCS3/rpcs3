/*
 * pthread_barrier_wait.c
 *
 * Description:
 * This translation unit implements barrier primitives.
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


int
pthread_barrier_wait (pthread_barrier_t * barrier)
{
  int result;
  int step;
  pthread_barrier_t b;

  if (barrier == NULL || *barrier == (pthread_barrier_t) PTW32_OBJECT_INVALID)
    {
      return EINVAL;
    }

  b = *barrier;
  step = b->iStep;

  if (0 == InterlockedDecrement ((long *) &(b->nCurrentBarrierHeight)))
    {
      /* Must be done before posting the semaphore. */
      b->nCurrentBarrierHeight = b->nInitialBarrierHeight;

      /*
       * There is no race condition between the semaphore wait and post
       * because we are using two alternating semas and all threads have
       * entered barrier_wait and checked nCurrentBarrierHeight before this
       * barrier's sema can be posted. Any threads that have not quite
       * entered sem_wait below when the multiple_post has completed
       * will nevertheless continue through the semaphore (barrier)
       * and will not be left stranded.
       */
      result = (b->nInitialBarrierHeight > 1
		? sem_post_multiple (&(b->semBarrierBreeched[step]),
				     b->nInitialBarrierHeight - 1) : 0);
    }
  else
    {
      /*
       * Use the non-cancelable version of sem_wait().
       */
      result = ptw32_semwait (&(b->semBarrierBreeched[step]));
    }

  /*
   * The first thread across will be the PTHREAD_BARRIER_SERIAL_THREAD.
   * This also sets up the alternate semaphore as the next barrier.
   */
  if (0 == result)
    {
      result = ((PTW32_INTERLOCKED_LONG) step ==
		PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG)
						    & (b->iStep),
						    (PTW32_INTERLOCKED_LONG)
						    (1L - step),
						    (PTW32_INTERLOCKED_LONG)
						    step) ?
		PTHREAD_BARRIER_SERIAL_THREAD : 0);
    }

  return (result);
}
