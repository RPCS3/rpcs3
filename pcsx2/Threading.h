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

#ifndef _THREADING_H_
#define _THREADING_H_

#include <errno.h> // EBUSY
#include <semaphore.h>

#include "PS2Etypes.h"
#include "Exceptions.h"

namespace Threading
{
	///////////////////////////////////////////////////////////////
	// Define some useful object handles - wait events, mutexes.

	struct WaitEvent
	{
		pthread_cond_t cond;
		pthread_mutex_t mutex;

		WaitEvent();
		~WaitEvent();

		void Set();
		void Wait();
	};

	struct Semaphore
	{
		sem_t sema;

		Semaphore();
		~Semaphore();

		void Post();
		void Post( int multiple );
		void Wait();
		int Count();
	};

	struct MutexLock
	{
		pthread_mutex_t mutex;

		MutexLock();
		~MutexLock();

		void Lock();
		void Unlock();
	};

	// Returns the number of available logical CPUs (cores plus
	// hyperthreaded cpus)
	extern void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU );

	// Releases a timeslice to other threads.
	extern void Timeslice();

	// For use in spin/wait loops.
	extern void SpinWait();

	class Thread : NoncopyableObject
	{
	protected:
		typedef int (*PlainJoeFP)();
		pthread_t m_thread;
		int m_returncode;		// value returned from the thread on close.
		bool m_terminated;		// set true after the thread has been closed.
		Semaphore m_post_event;	// general wait event that's needed by most threads.

	public:
		virtual ~Thread();
		Thread();

		virtual void Start();
		virtual void Close();

		// Gets the return code of the thread.
		// Throws std::logic_error if the thread has not terminated.
		int GetReturnCode() const;

	protected:
		// Used to dispatch the thread callback function.
		// (handles some thread cleanup on Win32, and is basically a typecast
		// on linux).
		static void* _internal_callback( void* func );

		// Implemented by derrived class to handle threading actions!
		virtual int Callback()=0;
	};

	// Our fundamental interlocking functions.  All other useful interlocks can
	// be derrived from these little beasties!

	extern long pcsx2_InterlockedExchange(volatile long* Target, long srcval);
	extern long pcsx2_InterlockedCompareExchange( volatile long* target, long srcval, long comp );
	extern long pcsx2_InterlockedExchangeAdd( volatile long* target, long addval );

	extern void AtomicExchange( volatile u32& Target, u32 value );
	extern void AtomicExchangeAdd( volatile u32& Target, u32 value );
	extern void AtomicIncrement( volatile u32& Target );
	extern void AtomicDecrement( volatile u32& Target );
	extern void AtomicExchange( volatile s32& Target, s32 value );
	extern void AtomicExchangeAdd( volatile s32& Target, u32 value );
	extern void AtomicIncrement( volatile s32& Target );
	extern void AtomicDecrement( volatile s32& Target );

	extern void _AtomicExchangePointer( const void ** target, const void* value );
	extern void _AtomicCompareExchangePointer( const void ** target, const void* value, const void* comparand );

	#define AtomicExchangePointer( target, value ) \
		_AtomicExchangePointer( (const void**)(&target), (const void*)(value) )

	#define AtomicCompareExchangePointer( target, value, comparand ) \
		_AtomicCompareExchangePointer( (const void**)(&target), (const void*)(value), (const void*)(comparand) )

}

#endif
