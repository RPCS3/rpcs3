/*
 * ptw32_throw.c
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

#include "ptw32pch.h"

/*
 * ptw32_throw
 *
 * All canceled and explicitly exited POSIX threads go through
 * here. This routine knows how to exit both POSIX initiated threads and
 * 'implicit' POSIX threads for each of the possible language modes (C,
 * C++, and SEH).
 */
void
ptw32_throw (DWORD exception)
{
  /*
   * Don't use pthread_self() to avoid creating an implicit POSIX thread handle
   * unnecessarily.
   */
  ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

#ifdef __CLEANUP_SEH
  DWORD exceptionInformation[3];
#endif

  if (exception != PTW32_EPS_CANCEL && exception != PTW32_EPS_EXIT)
    {
      /* Should never enter here */
      exit (1);
    }

  if (NULL == sp || sp->implicit)
    {
      /*
       * We're inside a non-POSIX initialised Win32 thread
       * so there is no point to jump or throw back to. Just do an
       * explicit thread exit here after cleaning up POSIX
       * residue (i.e. cleanup handlers, POSIX thread handle etc).
       */
      void* exitCode = 0;

      switch (exception)
	{
	case PTW32_EPS_CANCEL:
	  exitCode = PTHREAD_CANCELED;
	  break;
	case PTW32_EPS_EXIT:
	  exitCode = sp->exitStatus;
	  break;
	}

#if defined(PTW32_STATIC_LIB)

      pthread_win32_thread_detach_np ();

#endif

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
      _endthreadex ((unsigned)exitCode);
#else
      _endthread ();
#endif

    }

#ifdef __CLEANUP_SEH


  exceptionInformation[0] = (DWORD) (exception);
  exceptionInformation[1] = (DWORD) (0);
  exceptionInformation[2] = (DWORD) (0);

  RaiseException (EXCEPTION_PTW32_SERVICES, 0, 3, exceptionInformation);

#else /* __CLEANUP_SEH */

#ifdef __CLEANUP_C

  ptw32_pop_cleanup_all (1);
  longjmp (sp->start_mark, exception);

#else /* __CLEANUP_C */

#ifdef __CLEANUP_CXX

  switch (exception)
    {
    case PTW32_EPS_CANCEL:
      throw ptw32_exception_cancel ();
      break;
    case PTW32_EPS_EXIT:
      throw ptw32_exception_exit ();
      break;
    }

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* __CLEANUP_CXX */

#endif /* __CLEANUP_C */

#endif /* __CLEANUP_SEH */

  /* Never reached */
}


#ifdef __CLEANUP_C
void
ptw32_pop_cleanup_all (int execute)
{
  while (NULL != ptw32_pop_cleanup (execute))
    {
    }
}
#endif

DWORD
ptw32_get_exception_services_code (void)
{
#ifdef __CLEANUP_SEH

  return EXCEPTION_PTW32_SERVICES;

#else

  return (DWORD) NULL;

#endif
}
