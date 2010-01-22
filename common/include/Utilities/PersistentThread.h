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

#include "Threading.h"
#include "EventSource.h"

namespace Threading
{

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
//  ThreadDeleteEvent
// --------------------------------------------------------------------------------------
	class EventListener_Thread : public IEventDispatcher<int>
	{
	public:
		typedef int EvtParams;

	protected:
		PersistentThread* m_thread;

	public:
		EventListener_Thread()
		{
			m_thread = NULL;
		}

		virtual ~EventListener_Thread() throw() {}

		void SetThread( PersistentThread& thr ) { m_thread = &thr; }
		void SetThread( PersistentThread* thr ) { m_thread = thr; }

		void DispatchEvent( const int& params )
		{
			OnThreadCleanup();
		}

	protected:
		// Invoked by the PersistentThread when the thread execution is ending.  This is
		// typically more useful than a delete listener since the extended thread information
		// provided by virtualized functions/methods will be available.
		// Important!  This event is executed *by the thread*, so care must be taken to ensure
		// thread sync when necessary (posting messages to the main thread, etc).
		virtual void OnThreadCleanup()=0;
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

		friend void pxYield( int ms );

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

		EventSource<EventListener_Thread> m_evtsrc_OnDelete;

	public:
		virtual ~PersistentThread() throw();
		PersistentThread();
		PersistentThread( const char* name );

		pthread_t GetId() const { return m_thread; }

		virtual void Start();
		virtual void Cancel( bool isBlocking = true );
		virtual bool Cancel( const wxTimeSpan& timeout );
		virtual bool Detach();
		virtual void Block();
		virtual void RethrowException() const;
		
		void AddListener( EventListener_Thread& evt );
		void AddListener( EventListener_Thread* evt )
		{
			if( evt == NULL ) return;
			AddListener( *evt );
		}

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
