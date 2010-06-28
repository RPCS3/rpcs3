/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <semaphore.h>
#include <errno.h> // EBUSY
#include <pthread.h>

#include "Pcsx2Defs.h"
#include "ScopedPtr.h"

#undef Yield		// release the burden of windows.h global namespace spam.

#define AffinityAssert_AllowFrom_MainUI() \
	pxAssertMsg( wxThread::IsMain(), "Thread affinity violation: Call allowed from main thread only." )

// --------------------------------------------------------------------------------------
//  PCSX2_THREAD_LOCAL - Defines platform/operating system support for Thread Local Storage
// --------------------------------------------------------------------------------------
// For complimentary support for TLS, include Utilities/TlsVariable.inl, and use the
// DeclareTls macro in the place of __threadlocal.
//
//#define PCSX2_THREAD_LOCAL 0		// uncomment this line to force-disable native TLS (useful for testing TlsVariabel on windows/linux)

#ifndef PCSX2_THREAD_LOCAL
#	ifdef __WXMAC__
#		define PCSX2_THREAD_LOCAL 0
#	else
#		define PCSX2_THREAD_LOCAL 1
#	endif
#endif

class wxTimeSpan;

namespace Threading
{
	class pxThread;
	class RwMutex;

	extern void pxTestCancel();
	extern pxThread* pxGetCurrentThread();
	extern wxString pxGetCurrentThreadName();
	extern u64 GetThreadCpuTime();
	extern u64 GetThreadTicksPerSecond();

	// Yields the current thread and provides cancellation points if the thread is managed by
	// pxThread.  Unmanaged threads use standard Sleep.
	extern void pxYield( int ms );
}

namespace Exception
{
	class BaseThreadError : public RuntimeError
	{
		DEFINE_EXCEPTION_COPYTORS( BaseThreadError, RuntimeError )
		DEFINE_EXCEPTION_MESSAGES( BaseThreadError )

	public:
		Threading::pxThread*	m_thread;

	protected:
		BaseThreadError() {
			m_thread = NULL;
		}

	public:
		explicit BaseThreadError( Threading::pxThread* _thread )
		{
			m_thread = _thread;
			m_message_diag = L"An unspecified thread-related error occurred (thread=%s)";
		}

		explicit BaseThreadError( Threading::pxThread& _thread )
		{
			m_thread = &_thread;
			m_message_diag = L"An unspecified thread-related error occurred (thread=%s)";
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;

		Threading::pxThread& Thread();
		const Threading::pxThread& Thread() const;
	};

	class ThreadCreationError : public BaseThreadError
	{
		DEFINE_EXCEPTION_COPYTORS( ThreadCreationError, BaseThreadError )

	public:
		explicit ThreadCreationError( Threading::pxThread* _thread )
		{
			m_thread = _thread;
			SetBothMsgs( "Thread creation failure.  An unspecified error occurred while trying to create the %s thread." );
		}

		explicit ThreadCreationError( Threading::pxThread& _thread )
		{
			m_thread = &_thread;
			SetBothMsgs( "Thread creation failure.  An unspecified error occurred while trying to create the %s thread." );
		}
	};
}


namespace Threading
{
// --------------------------------------------------------------------------------------
//  Platform Specific External APIs
// --------------------------------------------------------------------------------------
// The following set of documented functions have Linux/Win32 specific implementations,
// which are found in WinThreads.cpp and LnxThreads.cpp

	// Releases a timeslice to other threads.
	extern void Timeslice();

	// For use in spin/wait loops.
	extern void SpinWait();

	// Optional implementation to enable hires thread/process scheduler for the operating system.
	// Needed by Windows, but might not be relevant to other platforms.
	extern void EnableHiresScheduler();
	extern void DisableHiresScheduler();

	// sleeps the current thread for the given number of milliseconds.
	extern void Sleep( int ms );

// --------------------------------------------------------------------------------------
//  AtomicExchange / AtomicIncrement
// --------------------------------------------------------------------------------------
// Our fundamental interlocking functions.  All other useful interlocks can be derived
// from these little beasties!  (these are all implemented internally using cross-platform
// implementations of _InterlockedExchange and such)

	extern u32 AtomicExchange( volatile u32& Target, u32 value );
	extern u32 AtomicExchangeAdd( volatile u32& Target, u32 value );
	extern u32 AtomicIncrement( volatile u32& Target );
	extern u32 AtomicDecrement( volatile u32& Target );
	extern s32 AtomicExchange( volatile s32& Target, s32 value );
	extern s32 AtomicExchangeAdd( volatile s32& Target, s32 value );
	extern s32 AtomicExchangeSub( volatile s32& Target, s32 value );
	extern s32 AtomicIncrement( volatile s32& Target );
	extern s32 AtomicDecrement( volatile s32& Target );

	extern bool AtomicBitTestAndReset( volatile u32& bitset, u8 bit );

	extern void* _AtomicExchangePointer( volatile uptr& target, uptr value );
	extern void* _AtomicCompareExchangePointer( volatile uptr& target, uptr value, uptr comparand );

#define AtomicExchangePointer( dest, src )				_AtomicExchangePointer( (uptr&)dest, (uptr)src )
#define AtomicCompareExchangePointer( dest, comp, src )	_AtomicExchangePointer( (uptr&)dest, (uptr)comp, (uptr)src )

	// pthread Cond is an evil api that is not suited for Pcsx2 needs.
	// Let's not use it. Use mutexes and semaphores instead to create waits. (Air)
#if 0
	struct WaitEvent
	{
		pthread_cond_t cond;
		pthread_mutex_t mutex;

		WaitEvent();
		~WaitEvent() throw();

		void Set();
		void Wait();
	};
#endif

// --------------------------------------------------------------------------------------
//  NonblockingMutex
// --------------------------------------------------------------------------------------
// This is a very simple non-blocking mutex, which behaves similarly to pthread_mutex's
// trylock(), but without any of the extra overhead needed to set up a structure capable
// of blocking waits.  It basically optimizes to a single InterlockedExchange.
//
// Simple use: if TryAcquire() returns false, the Bool is already interlocked by another thread.
// If TryAcquire() returns true, you've locked the object and are *responsible* for unlocking
// it later.
//
	class NonblockingMutex
	{
	protected:
		volatile int val;

	public:
		NonblockingMutex() : val( false ) {}
		virtual ~NonblockingMutex() throw() {}

		bool TryAcquire() throw()
		{
			return !AtomicExchange( val, true );
		}

		bool IsLocked()
		{ return !!val; }

		void Release()
		{
			AtomicExchange( val, false );
		}
	};

	class Semaphore
	{
	protected:
		sem_t m_sema;

	public:
		Semaphore();
		virtual ~Semaphore() throw();

		void Reset();
		void Post();
		void Post( int multiple );

		void WaitWithoutYield();
		bool WaitWithoutYield( const wxTimeSpan& timeout );
		void WaitNoCancel();
		void WaitNoCancel( const wxTimeSpan& timeout );
		int  Count();

		void Wait();
		bool Wait( const wxTimeSpan& timeout );
	};

	class Mutex
	{
	protected:
		pthread_mutex_t m_mutex;

	public:
		Mutex();
		virtual ~Mutex() throw();
		virtual bool IsRecursive() const { return false; }

		void Recreate();
		bool RecreateIfLocked();
		void Detach();

		void Acquire();
		bool Acquire( const wxTimeSpan& timeout );
		bool TryAcquire();
		void Release();

		void AcquireWithoutYield();
		bool AcquireWithoutYield( const wxTimeSpan& timeout );

		void Wait();
		bool Wait( const wxTimeSpan& timeout );
		void WaitWithoutYield();
		bool WaitWithoutYield( const wxTimeSpan& timeout );

	protected:
		// empty constructor used by MutexLockRecursive
		Mutex( bool ) {}
	};

	class MutexRecursive : public Mutex
	{
	public:
		MutexRecursive();
		virtual ~MutexRecursive() throw();
		virtual bool IsRecursive() const { return true; }
	};

	// --------------------------------------------------------------------------------------
	//  ScopedLock
	// --------------------------------------------------------------------------------------
	// Helper class for using Mutexes.  Using this class provides an exception-safe (and
	// generally clean) method of locking code inside a function or conditional block.  The lock
	// will be automatically released on any return or exit from the function.
	//
	// Const qualification note:
	//  ScopedLock takes const instances of the mutex, even though the mutex is modified 
	//  by locking and unlocking.  Two rationales:
	//
	//  1) when designing classes with accessors (GetString, GetValue, etc) that need mutexes,
	//     this class needs a const hack to allow those accessors to be const (which is typically
	//     *very* important).
	//
	//  2) The state of the Mutex is guaranteed to be unchanged when the calling function or
	//     scope exits, by any means.  Only via manual calls to Release or Acquire does that
	//     change, and typically those are only used in very special circumstances of their own.
	//
	class ScopedLock
	{
		DeclareNoncopyableObject(ScopedLock);

	protected:
		Mutex*	m_lock;
		bool	m_IsLocked;

	public:
		virtual ~ScopedLock() throw();
		explicit ScopedLock( const Mutex* locker=NULL );
		explicit ScopedLock( const Mutex& locker );
		void AssignAndLock( const Mutex& locker );
		void AssignAndLock( const Mutex* locker );

		void Assign( const Mutex& locker );
		void Assign( const Mutex* locker );

		void Release();
		void Acquire();

		bool IsLocked() const { return m_IsLocked; }

	protected:
		// Special constructor used by ScopedTryLock
		ScopedLock( const Mutex& locker, bool isTryLock );
	};

	class ScopedTryLock : public ScopedLock
	{
	public:
		ScopedTryLock( const Mutex& locker ) : ScopedLock( locker, true ) { }
		virtual ~ScopedTryLock() throw() {}
		bool Failed() const { return !m_IsLocked; }
	};

// --------------------------------------------------------------------------------------
//  ScopedNonblockingLock
// --------------------------------------------------------------------------------------
// A ScopedTryLock branded for use with Nonblocking mutexes.  See ScopedTryLock for details.
//
	class ScopedNonblockingLock
	{
		DeclareNoncopyableObject(ScopedNonblockingLock);

	protected:
		NonblockingMutex&	m_lock;
		bool				m_IsLocked;

	public:
		ScopedNonblockingLock( NonblockingMutex& locker ) :
			m_lock( locker )
		,	m_IsLocked( m_lock.TryAcquire() )
		{
		}

		virtual ~ScopedNonblockingLock() throw()
		{
			if( m_IsLocked )
				m_lock.Release();
		}

		bool Failed() const { return !m_IsLocked; }
	};
}

