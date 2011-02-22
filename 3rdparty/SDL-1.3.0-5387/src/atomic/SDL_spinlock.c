/*
  SDL - Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Sam Lantinga
  slouken@libsdl.org
*/
#include "SDL_stdinc.h"

#include "SDL_atomic.h"
#include "SDL_mutex.h"
#include "SDL_timer.h"

/* Don't do the check for Visual Studio 2005, it's safe here */
#ifdef __WIN32__
#include "../core/windows/SDL_windows.h"
#endif

/* This function is where all the magic happens... */
SDL_bool
SDL_AtomicTryLock(SDL_SpinLock *lock)
{
#if SDL_ATOMIC_DISABLED
    /* Terrible terrible damage */
    static SDL_mutex *_spinlock_mutex;

    if (!_spinlock_mutex) {
        /* Race condition on first lock... */
        _spinlock_mutex = SDL_CreateMutex();
    }
    SDL_mutexP(_spinlock_mutex);
    if (*lock == 0) {
        *lock = 1;
        SDL_mutexV(_spinlock_mutex);
        return SDL_TRUE;
    } else {
        SDL_mutexV(_spinlock_mutex);
        return SDL_FALSE;
    }

#elif defined(_MSC_VER)
    SDL_COMPILE_TIME_ASSERT(locksize, sizeof(*lock) == sizeof(long));
    return (InterlockedExchange((long*)lock, 1) == 0);

#elif defined(__MACOSX__)
    return OSAtomicCompareAndSwap32Barrier(0, 1, lock);

#elif HAVE_GCC_ATOMICS || HAVE_GCC_SYNC_LOCK_TEST_AND_SET
    return (__sync_lock_test_and_set(lock, 1) == 0);

#elif defined(__GNUC__) && defined(__arm__) && \
        (defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__) || \
         defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5TE__))
    int result;
    __asm__ __volatile__ (
        "swp %0, %1, [%2]\n"
        : "=&r,&r" (result) : "r,0" (1), "r,r" (lock) : "memory");
    return (result == 0);

#elif defined(__GNUC__) && defined(__arm__)
    int result;
    __asm__ __volatile__ (
        "ldrex %0, [%2]\nteq   %0, #0\nstrexeq %0, %1, [%2]"
        : "=&r" (result) : "r" (1), "r" (lock) : "cc", "memory");
    return (result == 0);

#else
    /* Need CPU instructions for spinlock here! */
    __need_spinlock_implementation__
#endif
}

void
SDL_AtomicLock(SDL_SpinLock *lock)
{
    /* FIXME: Should we have an eventual timeout? */
    while (!SDL_AtomicTryLock(lock)) {
        SDL_Delay(0);
    }
}

void
SDL_AtomicUnlock(SDL_SpinLock *lock)
{
#if defined(_MSC_VER)
    _ReadWriteBarrier();
    *lock = 0;

#elif HAVE_GCC_ATOMICS || HAVE_GCC_SYNC_LOCK_TEST_AND_SET
    __sync_lock_release(lock);

#else
    *lock = 0;
#endif
}

/* vi: set ts=4 sw=4 expandtab: */
