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

/* Note that we undefine the atomic operations here, in case they are
   defined as compiler intrinsics while building SDL but the library user
   doesn't have that compiler.  That way we always have a working set of
   atomic operations built into the library.
*/
 
/* 
  If any of the operations are not provided then we must emulate some
  of them. That means we need a nice implementation of spin locks
  that avoids the "one big lock" problem. We use a vector of spin
  locks and pick which one to use based on the address of the operand
  of the function.

  To generate the index of the lock we first shift by 3 bits to get
  rid on the zero bits that result from 32 and 64 bit allignment of
  data. We then mask off all but 5 bits and use those 5 bits as an
  index into the table. 

  Picking the lock this way insures that accesses to the same data at
  the same time will go to the same lock. OTOH, accesses to different
  data have only a 1/32 chance of hitting the same lock. That should
  pretty much eliminate the chances of several atomic operations on
  different data from waiting on the same "big lock". If it isn't
  then the table of locks can be expanded to a new size so long as
  the new size is a power of two.

  Contributed by Bob Pendleton, bob@pendleton.com
*/

static SDL_SpinLock locks[32];

static __inline__ void
enterLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

    SDL_AtomicLock(&locks[index]);
}

static __inline__ void
leaveLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

    SDL_AtomicUnlock(&locks[index]);
}

SDL_bool
SDL_AtomicCAS_(SDL_atomic_t *a, int oldval, int newval)
{
    SDL_bool retval = SDL_FALSE;

    enterLock(a);
    if (a->value == oldval) {
        a->value = newval;
        retval = SDL_TRUE;
    }
    leaveLock(a);

    return retval;
}

SDL_bool
SDL_AtomicCASPtr_(void **a, void *oldval, void *newval)
{
    SDL_bool retval = SDL_FALSE;

    enterLock(a);
    if (*a == oldval) {
        *a = newval;
        retval = SDL_TRUE;
    }
    leaveLock(a);

    return retval;
}

/* vi: set ts=4 sw=4 expandtab: */
