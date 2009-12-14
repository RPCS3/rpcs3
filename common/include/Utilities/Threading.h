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

#undef Yield		// release th burden of windows.h global namespace spam.

#define AffinityAssert_AllowFromMain() \
	pxAssertMsg( wxThread::IsMain(), "Thread affinity violation: Call allowed from main thread only." )

class wxTimeSpan;

namespace Threading
{
	class PersistentThread;

	PersistentThread* pxGetCurrentThread();
	wxString pxGetCurrentThreadName();
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
//  ThreadTimedOut Exception
// --------------------------------------------------------------------------------------
// This exception is thrown by Semaphore and Mutex Wait/Acquire functions if a blocking wait is
// needed due to gui Yield recursion, and the timeout period for deadlocking (usually 3 seconds)
// is reached before the lock becomes available. This exception cannot occur in the following
// conditions:
//  * If the user-specified timeout is less than the deadlock timeout.
//  * If the method is run from a thread *other* than the MainGui thread.
//
	class ThreadTimedOut : public virtual BaseThreadError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( ThreadTimedOut )

		explicit ThreadTimedOut( Threading::PersistentThread* _thread=NULL, const char* msg="Blocking action timed out waiting for '%s' (potential thread deadlock)." )
		{
			m_thread = _thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadTimedOut( Threading::PersistentThread& _thread, const char* msg="Blocking action timed out waiting for '%s' (potential thread deadlock)." )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg );
		}

		ThreadTimedOut( Threading::PersistentThread& _thread, const wxString& msg_diag, const wxString& msg_user )
		{
			m_thread = &_thread;
			BaseException::InitBaseEx( msg_diag, msg_user );
		}
	};
#endif
}

// Yields this thread against the main thread *if* the main thread's message pump has pending
// messages.  If the main thread is idle then no yield is performed.
extern void pxYieldToMain();

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
// IThread - Interface for the public access to PersistentThread.
// --------------------------------------------------------------------------------------
// Class usage: Can be used for allowing safe nullification of a thread handle.  Rather
// than being NULL'd, the handle can be mapped to an IThread implementation which acts
// as a do-nothing placebo or an assertion generator.
//
	class IThread
	{
		DeclareNoncopyableObject(IThread);

	public:
		IThread() {}
		virtual ~IThread() throw() {}

		virtual bool IsSelf() const { return false; }
		virtual bool IsRunning() { return false; }

		virtual void Start() {}
		virtual void Cancel( bool isBlocking = true ) {}
		virtual void Block() {}
		virtual bool Detach() { return false; }
	};

// --------------------------------------------------------------------------------------
// PersistentThread - Helper class for the basics of starting/managing persistent threads.
// --------------------------------------------------------------------------------------
// This class is meant to be a helper for the typical threading model of "start once and
// reuse many times."  This class incorporates a lot of extra overhead in stopping and
// starting threads, but in turn provides most of the basic thread-safety and event-handling
// functionality needed for a threaded operation.  In practice this model is usually an
// ideal one for efficiency since Operating Systems themselves typically subscribe to a
// design where sleeping, suspending, and resuming threads is very efficient, but starting
// new threads has quite a bit of overhead.
//
// To use this as a base class for your threaded procedure, overload the following virtual
// methods:
//  void OnStart();
//  void ExecuteTaskInThread();
//  void OnCleanupInThread();
//
// Use the public methods Start() and Cancel() to start and shutdown the thread, and use
// m_sem_event internally to post/receive events for the thread (make a public accessor for
// it in your derived class if your thread utilizes the post).
//
// Notes:
//  * Constructing threads as static global vars isn't recommended since it can potentially
//    confuse w32pthreads, if the static initializers are executed out-of-order (C++ offers
//    no dependency options for ensuring correct static var initializations).  Use heap
//    allocation to create thread objects instead.
//
	class PersistentThread : public virtual IThread
	{
		DeclareNoncopyableObject(PersistentThread);

	protected:
		wxString	m_name;				// diagnostic name for our thread.

		pthread_t	m_thread;
		Semaphore	m_sem_event;		// general wait event that's needed by most threads
		Semaphore	m_sem_startup;		// startup sync tool
		Mutex		m_lock_InThread;		// used for canceling and closing threads in a deadlock-safe manner
		MutexLockRecursive	m_lock_start;	// used to lock the Start() code from starting simultaneous threads accidentally.

		volatile long m_detached;		// a boolean value which indicates if the m_thread handle is valid
		volatile long m_running;		// set true by Start(), and set false by Cancel(), Block(), etc.

		// exception handle, set non-NULL if the thread terminated with an exception
		// Use RethrowException() to re-throw the exception using its original exception type.
		ScopedPtr<Exception::BaseException> m_except;

	public:
		virtual ~PersistentThread() throw();
		PersistentThread();
		PersistentThread( const char* name );

		virtual void Start();
		virtual void Cancel( bool isBlocking = true );
		virtual bool Cancel( const wxTimeSpan& timeout );
		virtual bool Detach();
		virtual void Block();
		virtual void RethrowException() const;

		void WaitOnSelf( Semaphore& mutex ) const;
		void WaitOnSelf( Mutex& mutex ) const;
		bool WaitOnSelf( Semaphore& mutex, const wxTimeSpan& timeout ) const;
		bool WaitOnSelf( Mutex& mutex, const wxTimeSpan& timeout ) const;

		bool IsRunning() const;
		bool IsSelf() const;
		wxString GetName() const;
		bool HasPendingException() const { return !!m_except; }

	protected:
		// Extending classes should always implement your own OnStart(), which is called by
		// Start() once necessary locks have been obtained.  Do not override Start() directly
		// unless you're really sure that's what you need to do. ;)
		virtual void OnStart();

		virtual void OnStartInThread();

		// This is called when the thread has been canceled or exits normally.  The PersistentThread
		// automatically binds it to the pthread cleanup routines as soon as the thread starts.
		virtual void OnCleanupInThread();

		// Implemented by derived class to perform actual threaded task!
		virtual void ExecuteTaskInThread()=0;

		void TestCancel() const;
		void YieldToMain() const;

		// Yields this thread to other threads and checks for cancellation.  A sleeping thread should
		// always test for cancellation, however if you really don't want to, you can use Threading::Sleep()
		// or better yet, disable cancellation of the thread completely with DisableCancellation().
		//
		// Parameters:
		//   ms - 'minimum' yield time in milliseconds (rough -- typically yields are longer by 1-5ms
		//         depending on operating system/platform).  If ms is 0 or unspecified, then a single
		//         timeslice is yielded to other contending threads.  If no threads are contending for
		//         time when ms==0, then no yield is done, but cancellation is still tested.
		void Yield( int ms = 0 )
		{
			pxAssert( IsSelf() );
			Threading::Sleep( ms );
			TestCancel();
		}
		
		void FrankenMutex( Mutex& mutex );

		bool AffinityAssert_AllowFromSelf( const DiagnosticOrigin& origin ) const;
		bool AffinityAssert_DisallowFromSelf( const DiagnosticOrigin& origin ) const;

		// ----------------------------------------------------------------------------
		// Section of methods for internal use only.

		bool _basecancel();
		void _selfRunningTest( const wxChar* name ) const;
		void _DoSetThreadName( const wxString& name );
		void _DoSetThreadName( const char* name );
		void _internal_execute();
		void _try_virtual_invoke( void (PersistentThread::*method)() );
		void _ThreadCleanup();

		static void* _internal_callback( void* func );
		static void _pt_callback_cleanup( void* handle );
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
		bool		m_IsLocked;

	public:
		virtual ~ScopedLock() throw()
		{
			if( m_IsLocked )
				m_lock.Release();
		}

		ScopedLock( Mutex& locker ) :
			m_lock( locker )
		,	m_IsLocked( true )
		{
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
		,	m_IsLocked( isTryLock ? m_lock.TryAcquire() : false )
		{
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

// --------------------------------------------------------------------------------------
//  BaseTaskThread 
// --------------------------------------------------------------------------------------
// an abstract base class which provides simple parallel execution of single tasks.
//
// FIXME: This class is incomplete and untested!  Don't use, unless you want to fix it
// while you're at it. :D
//
// Implementation:
//   To use this class your derived class will need to implement its own Task() function
//   and also a "StartTask( parameters )" function which suits the need of your task, along
//   with any local variables your task needs to do its job.  You may additionally want to
//   implement a "GetResult()" function, which would be a combination of WaitForResult()
//   and a return value of the computational result.
//
// Thread Safety:
//   If operating on local variables, you must execute WaitForResult() before leaving the
//   variable scope -- or alternatively have your StartTask() implementation make full
//   copies of dependent data.  Also, by default PostTask() always assumes the previous
//   task has completed.  If your system can post a new task before the previous one has
//   completed, then it needs to explicitly call WaitForResult() or provide a mechanism
//   to cancel the previous task (which is probably more work than it's worth).
//
// Performance notes:
//  * Remember that thread creation is generally slow, so you should make your object
//    instance once early and then feed it tasks repeatedly over the course of program
//    execution.
//
//  * For threading to be a successful speedup, the task being performed should be as lock
//    free as possible.  For example using STL containers in parallel usually fails to
//    yield any speedup due to the gratuitous amount of locking that the STL performs
//    internally.
//
//  * The best application of tasking threads is to divide a large loop over a linear array
//    into smaller sections.  For example, if you have 20,000 items to process, the task
//    can be divided into two threads of 10,000 items each.
//
	class BaseTaskThread : public PersistentThread
	{
	protected:
		volatile bool m_Done;
		volatile bool m_TaskPending;
		Semaphore m_post_TaskComplete;
		Mutex m_lock_TaskComplete;

	public:
		virtual ~BaseTaskThread() throw() {}
		BaseTaskThread() :
			m_Done( false )
		,	m_TaskPending( false )
		,	m_post_TaskComplete()
		{
		}

		void Block();
		void PostTask();
		void WaitForResult();

	protected:
		// Abstract method run when a task has been posted.  Implementing classes should do
		// all your necessary processing work here.
		virtual void Task()=0;

		virtual void ExecuteTaskInThread();
	};
}

