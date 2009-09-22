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

#include <errno.h> // EBUSY
#include <pthread.h>
#include <semaphore.h>

#include "Pcsx2Defs.h"

namespace Exception
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// Thread termination exception, used to quickly terminate threads from anywhere in the
	// thread's call stack.  This exception is handled by the PCSX2 PersistentThread class.  Threads
	// not derived from that class will not handle this exception.
	//
	class ThreadTermination
	{
	};
}

class wxTimeSpan;

namespace Threading
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// Define some useful object handles - wait events, mutexes.

	// pthread Cond is an evil api that is not suited for Pcsx2 needs.
	// Let's not use it. Use mutexes and semaphores instead to create waits. (Air)
#if 0
	struct WaitEvent
	{
		pthread_cond_t cond;
		pthread_mutex_t mutex;

		WaitEvent();
		~WaitEvent();

		void Set();
		void Wait();
	};
#endif

	struct Semaphore
	{
		sem_t sema;

		Semaphore();
		~Semaphore();

		void Reset();
		void Post();
		void Post( int multiple );
		
#if wxUSE_GUI
		void WaitGui();
		bool WaitGui( const wxTimeSpan& timeout );
#endif
		void Wait();
		bool Wait( const wxTimeSpan& timeout );
		void WaitNoCancel();
		int  Count();
	};

	struct MutexLock
	{
		pthread_mutex_t mutex;

		MutexLock();
		MutexLock( bool isRecursive );
		~MutexLock();

		void Lock();
		void Unlock();
	};

	// Returns the number of available logical CPUs (cores plus hyperthreaded cpus)
	extern void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU );

	// Releases a timeslice to other threads.
	extern void Timeslice();

	// For use in spin/wait loops.
	extern void SpinWait();

	// sleeps the current thread for the given number of milliseconds.
	extern void Sleep( int ms );

	//////////////////////////////////////////////////////////////////////////////////////////
	// PersistentThread - Helper class for the basics of starting/managing persistent threads.
	//
	// Use this as a base class for your threaded procedure, and implement the 'int ExecuteTask()'
	// method.  Use Start() and Cancel() to start and shutdown the thread, and use m_sem_event
	// internally to post/receive events for the thread (make a public accessor for it in your
	// derived class if your thread utilizes the post).
	//
	// Notes:
	//  * Constructing threads as static global vars isn't recommended since it can potentially
	//    confuse w32pthreads, if the static initializers are executed out-of-order (C++ offers
	//    no dependency options for ensuring correct static var initializations).  Use heap
	//    allocation to create thread objects instead.
	//
	class PersistentThread
	{
		DeclareNoncopyableObject(PersistentThread);

	protected:
		typedef int (*PlainJoeFP)();
		pthread_t	m_thread;
		Semaphore	m_sem_event;		// general wait event that's needed by most threads.
		Semaphore	m_sem_finished;		// used for canceling and closing threads in a deadlock-safe manner
		sptr		m_returncode;		// value returned from the thread on close.

		volatile long m_detached;		// a boolean value which indicates if the m_thread handle is valid
		volatile long m_running;		// set true by Start(), and set false by Cancel(), Block(), etc.
		
	public:
		virtual ~PersistentThread() throw();
		PersistentThread();

		virtual void Start();
		virtual void Cancel( bool isBlocking = true );
		virtual void Detach();

		// Gets the return code of the thread.
		// Throws std::logic_error if the thread has not terminated.
		virtual int GetReturnCode() const;

		virtual bool IsRunning() const;
		virtual sptr Block();
		
		bool IsSelf() const;

		virtual void DoThreadCleanup();

	protected:
		void SetName( __unused const char* name );
		
		// Used to dispatch the thread callback function.
		// (handles some thread cleanup on Win32, and is basically a typecast
		// on linux).
		static void* _internal_callback( void* func );

		// Implemented by derived class to handle threading actions!
		virtual sptr ExecuteTask()=0;
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// ScopedLock: Helper class for using Mutexes.
	// Using this class provides an exception-safe (and generally clean) method of locking
	// code inside a function or conditional block.
	//
	class ScopedLock
	{
		DeclareNoncopyableObject(ScopedLock);

	protected:
		MutexLock& m_lock;
		bool m_IsLocked;

	public:
		virtual ~ScopedLock() throw()
		{
			if( m_IsLocked )
				m_lock.Unlock();
		}

		ScopedLock( MutexLock& locker ) :
			m_lock( locker )
		,	m_IsLocked( true )
		{
			m_lock.Lock();
		}

		// Provides manual unlocking of a scoped lock prior to object destruction.
		void Unlock()
		{
			if( !m_IsLocked ) return;
			m_IsLocked = false;
			m_lock.Unlock();
		}

		// provides manual locking of a scoped lock, to re-lock after a manual unlocking.
		void Lock()
		{
			if( m_IsLocked ) return;
			m_lock.Lock();
			m_IsLocked = true;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// BaseTaskThread - an abstract base class which provides simple parallel execution of
	// single tasks.
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
		MutexLock m_lock_TaskComplete;

	public:
		virtual ~BaseTaskThread() throw() {}
		BaseTaskThread() :
			m_Done( false )
		,	m_TaskPending( false )
		,	m_post_TaskComplete()
		{
		}

		sptr Block();
		void PostTask();
		void WaitForResult();

	protected:
		// Abstract method run when a task has been posted.  Implementing classes should do
		// all your necessary processing work here.
		virtual void Task()=0;

		sptr ExecuteTask();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Our fundamental interlocking functions.  All other useful interlocks can be derived
	// from these little beasties!

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

