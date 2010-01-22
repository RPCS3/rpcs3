/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#define AffinityAssert_AllowFromMain() \
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
	class PersistentThread;

	extern PersistentThread* pxGetCurrentThread();
	extern wxString pxGetCurrentThreadName();

	// Yields the current thread and provides cancellation points if the thread is managed by
	// PersistentThread.  Unmanaged threads use standard Sleep.
	extern void pxYield( int ms );
}

namespace Exception
{
	class BaseThreadError : public virtual RuntimeError
	{
	public:
		Threading::PersistentThread*	m_thread;

		DEFINE_EXCEPTION_COPYTORS( BaseThreadError )

		explicit BaseThreadError( Threading::PersistentThread* _thread=NULL )
		{
			m_thread = _thread;
			BaseException::InitBaseEx( "Unspecified thread error" );
		}

		BaseThreadError( Threading::PersistentThread& _thread )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( "Unspecified thread error" );
		}
		
		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;

		Threading::PersistentThread& Thread();
		const Threading::PersistentThread& Thread() const;
	};

	class ThreadCreationError : public virtual BaseThreadError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( ThreadCreationError )

		explicit ThreadCreationError( Threading::PersistentThread* _thread=NULL, const char* msg="Creation of thread '%s' failed." )
		{
			m_thread = _thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadCreationError( Threading::PersistentThread& _thread, const char* msg="Creation of thread '%s' failed." )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadCreationError( Threading::PersistentThread& _thread, const wxString& msg_diag, const wxString& msg_user )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg_diag, msg_user );
		}
	};

#if wxUSE_GUI

// --------------------------------------------------------------------------------------
//  ThreadDeadlock Exception
// --------------------------------------------------------------------------------------
// This exception is thrown by Semaphore and Mutex Wait/Acquire functions if a blocking wait is
// needed due to gui Yield recursion, and the timeout period for deadlocking (usually 3 seconds)
// is reached before the lock becomes available. This exception cannot occur in the following
// conditions:
//  * If the user-specified timeout is less than the deadlock timeout.
//  * If the method is run from a thread *other* than the MainGui thread.
//
	class ThreadDeadlock : public virtual BaseThreadError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( ThreadDeadlock )

		explicit ThreadDeadlock( Threading::PersistentThread* _thread=NULL, const char* msg="Blocking action timed out waiting for '%s' (potential thread deadlock)." )
		{
			m_thread = _thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadDeadlock( Threading::PersistentThread& _thread, const char* msg="Blocking action timed out waiting for '%s' (potential thread deadlock)." )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadDeadlock( Threading::PersistentThread& _thread, const wxString& msg_diag, const wxString& msg_user )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg_diag, msg_user );
		}
	};
#endif
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

	extern void* _AtomicExchangePointer( void * volatile * const target, void* const value );
	extern void* _AtomicCompareExchangePointer( void * volatile * const target, void* const value, void* const comparand );

#define AtomicExchangePointer( target, value ) \
	_InterlockedExchangePointer( &target, value )

#define AtomicCompareExchangePointer( target, value, comparand ) \
	_InterlockedCompareExchangePointer( &target, value, comparand )


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

	protected:
		// empty constructor used by MutexLockRecursive
		Mutex( bool ) {}
	};

	class MutexLockRecursive : public Mutex
	{
	public:
		MutexLockRecursive();
		virtual ~MutexLockRecursive() throw();
		virtual bool IsRecursive() const { return true; }
	};

	// --------------------------------------------------------------------------------------
	//  ScopedLock
	// --------------------------------------------------------------------------------------
	// Helper class for using Mutexes.  Using this class provides an exception-safe (and
	// generally clean) method of locking code inside a function or conditional block.  The lock
	// will be automatically released on any return or exit from the function.
	//
	class ScopedLock
	{
		DeclareNoncopyableObject(ScopedLock);

	protected:
		Mutex&	m_lock;
		bool	m_IsLocked;

	public:
		virtual ~ScopedLock() throw()
		{
			if( m_IsLocked )
				m_lock.Release();
		}

		ScopedLock( Mutex& locker ) :
			m_lock( locker )
		{
			m_IsLocked = true;
			m_lock.Acquire();
		}

		// Provides manual unlocking of a scoped lock prior to object destruction.
		void Release()
		{
			if( !m_IsLocked ) return;
			m_IsLocked = false;
			m_lock.Release();
		}

		// provides manual locking of a scoped lock, to re-lock after a manual unlocking.
		void Acquire()
		{
			if( m_IsLocked ) return;
			m_lock.Acquire();
			m_IsLocked = true;
		}

		bool IsLocked() const { return m_IsLocked; }

	protected:
		// Special constructor used by ScopedTryLock
		ScopedLock( Mutex& locker, bool isTryLock ) :
			m_lock( locker )
		{
			m_IsLocked = isTryLock ? m_lock.TryAcquire() : false;
		}

	};

	class ScopedTryLock : public ScopedLock
	{
	public:
		ScopedTryLock( Mutex& locker ) : ScopedLock( locker, true ) { }
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

