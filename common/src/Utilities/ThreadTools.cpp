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


#include "PrecompiledHeader.h"

#ifdef __LINUX__
#	include <signal.h>		// for pthread_kill, which is in pthread.h on w32-pthreads
#endif

#include "PersistentThread.h"
#include "wxBaseTools.h"
#include "ThreadingInternal.h"
#include "EventSource.inl"

using namespace Threading;

template class EventSource< EventListener_Thread >;

// 100ms interval for waitgui (issued from blocking semaphore waits on the main thread,
// to avoid gui deadlock).
const wxTimeSpan	Threading::def_yieldgui_interval( 0, 0, 0, 100 );

ConsoleLogSource_Threading::ConsoleLogSource_Threading()
{
	static const TraceLogDescriptor myDesc =
	{
		L"pxThread",	L"pxThread",
		pxLt("Threading activity: start, detach, sync, deletion, etc.")
	};

	m_Descriptor = &myDesc;
}

ConsoleLogSource_Threading pxConLog_Thread;


class StaticMutex : public Mutex
{
protected:
	bool&	m_DeletedFlag;

public:
	StaticMutex( bool& deletedFlag )
		: m_DeletedFlag( deletedFlag )
	{
	}

	virtual ~StaticMutex() throw()
	{
		m_DeletedFlag = true;
	}
};

static pthread_key_t	curthread_key = 0;
static s32				total_key_count = 0;

static bool				tkl_destructed = false;
static StaticMutex		total_key_lock( tkl_destructed );

static void make_curthread_key( const pxThread* thr )
{
	pxAssumeDev( !tkl_destructed, "total_key_lock is destroyed; program is shutting down; cannot create new thread key." );

	ScopedLock lock( total_key_lock );
	if( total_key_count++ != 0 ) return;

	if( 0 != pthread_key_create(&curthread_key, NULL) )
	{
		pxThreadLog.Error( thr->GetName(), L"Thread key creation failed (probably out of memory >_<)" );
		curthread_key = 0;
	}
}

static void unmake_curthread_key()
{
	ScopedLock lock;
	if( !tkl_destructed )
		lock.AssignAndLock( total_key_lock );

	if( --total_key_count > 0 ) return;

	if( curthread_key )
		pthread_key_delete( curthread_key );

	curthread_key = 0;
}

void Threading::pxTestCancel()
{
	pthread_testcancel();
}

// Returns a handle to the current persistent thread.  If the current thread does not belong
// to the pxThread table, NULL is returned.  Since the main/ui thread is not created
// through pxThread it will also return NULL.  Callers can use wxThread::IsMain() to
// test if the NULL thread is the main thread.
pxThread* Threading::pxGetCurrentThread()
{
	return !curthread_key ? NULL : (pxThread*)pthread_getspecific( curthread_key );
}

// returns the name of the current thread, or "Unknown" if the thread is neither a pxThread
// nor the Main/UI thread.
wxString Threading::pxGetCurrentThreadName()
{
	if( pxThread* thr = pxGetCurrentThread() )
	{
		return thr->GetName();
	}
	else if( wxThread::IsMain() )
	{
		return L"Main/UI";
	}

	return L"Unknown";
}

void Threading::pxYield( int ms )
{
	if( pxThread* thr = pxGetCurrentThread() )
		thr->Yield( ms );
	else
		Sleep( ms );
}

// (intended for internal use only)
// Returns true if the Wait is recursive, or false if the Wait is safe and should be
// handled via normal yielding methods.
bool Threading::_WaitGui_RecursionGuard( const wxChar* name )
{
	AffinityAssert_AllowFrom_MainUI();
	
	// In order to avoid deadlock we need to make sure we cut some time to handle messages.
	// But this can result in recursive yield calls, which would crash the app.  Protect
	// against them here and, if recursion is detected, perform a standard blocking wait.

	static int __Guard = 0;
	RecursionGuard guard( __Guard );

	//if( pxAssertDev( !guard.IsReentrant(), "Recursion during UI-bound threading wait object." ) ) return false;

	if( !guard.IsReentrant() ) return false;
	pxThreadLog.Write( pxGetCurrentThreadName(),
		pxsFmt(L"Yield recursion in %s; opening modal dialog.", name)
	);
	return true;
}

__fi void Threading::Timeslice()
{
	sched_yield();
}

void Threading::pxThread::_pt_callback_cleanup( void* handle )
{
	((pxThread*)handle)->_ThreadCleanup();
}


Threading::pxThread::pxThread( const wxString& name )
	: m_name( name )
{
	m_detached	= true;		// start out with m_thread in detached/invalid state
	m_running	= false;

	m_native_id		= 0;
	m_native_handle	= 0;
}

// This destructor performs basic "last chance" cleanup, which is a blocking join
// against the thread. Extending classes should almost always implement their own
// thread closure process, since any pxThread will, by design, not terminate
// unless it has been properly canceled (resulting in deadlock).
//
// Thread safety: This class must not be deleted from its own thread.  That would be
// like marrying your sister, and then cheating on her with your daughter.
Threading::pxThread::~pxThread() throw()
{
	try
	{
		pxThreadLog.Write( GetName(), L"Executing default destructor!" );

		if( m_running )
		{
			pxThreadLog.Write( GetName(), L"Waiting for running thread to end...");
			m_mtx_InThread.Wait();
			pxThreadLog.Write( GetName(), L"Thread ended gracefully.");
		}
		Threading::Sleep( 1 );
		Detach();
	}
	DESTRUCTOR_CATCHALL
}

bool Threading::pxThread::AffinityAssert_AllowFromSelf( const DiagnosticOrigin& origin ) const
{
	if( IsSelf() ) return true;

	if( IsDevBuild )
		pxOnAssert( origin, pxsFmt( L"Thread affinity violation: Call allowed from '%s' thread only.", GetName().c_str() ) );

	return false;
}

bool Threading::pxThread::AffinityAssert_DisallowFromSelf( const DiagnosticOrigin& origin ) const
{
	if( !IsSelf() ) return true;

	if( IsDevBuild )
		pxOnAssert( origin, pxsFmt( L"Thread affinity violation: Call is *not* allowed from '%s' thread.", GetName().c_str() ) );

	return false;
}

void Threading::pxThread::FrankenMutex( Mutex& mutex )
{
	if( mutex.RecreateIfLocked() )
	{
		// Our lock is bupkis, which means  the previous thread probably deadlocked.
		// Let's create a new mutex lock to replace it.

		pxThreadLog.Error( GetName(), L"Possible deadlock detected on restarted mutex!" );
	}
}

// Main entry point for starting or e-starting a persistent thread.  This function performs necessary
// locks and checks for avoiding race conditions, and then calls OnStart() immediately before
// the actual thread creation.  Extending classes should generally not override Start(), and should
// instead override DoPrepStart instead.
//
// This function should not be called from the owner thread.
void Threading::pxThread::Start()
{
	// Prevents sudden parallel startup, and or parallel startup + cancel:
	ScopedLock startlock( m_mtx_start );
	if( m_running )
	{
		pxThreadLog.Write(GetName(), L"Start() called on running thread; ignorning...");
		return;
	}

	Detach();		// clean up previous thread handle, if one exists.
	OnStart();

	m_except = NULL;

	pxThreadLog.Write(GetName(), L"Calling pthread_create...");
	if( pthread_create( &m_thread, NULL, _internal_callback, this ) != 0 )
		throw Exception::ThreadCreationError( this );

	if( !m_sem_startup.WaitWithoutYield( wxTimeSpan( 0, 0, 3, 0 ) ) )
	{
		RethrowException();

		// And if the thread threw nothing of its own:
		throw Exception::ThreadCreationError( this ).SetDiagMsg( L"Thread creation error: %s thread never posted startup semaphore." );
	}

	// Event Rationale (above): Performing this semaphore wait on the created thread is "slow" in the
	// sense that it stalls the calling thread completely until the new thread is created
	// (which may not always be desirable).  But too bad.  In order to safely use 'running' locks
	// and detachment management, this *has* to be done.  By rule, starting new threads shouldn't
	// be done very often anyway, hence the concept of Threadpooling for rapidly rotating tasks.
	// (and indeed, this semaphore wait might, in fact, be very swift compared to other kernel
	// overhead in starting threads).

	// (this could also be done using operating system specific calls, since any threaded OS has
	// functions that allow us to see if a thread is running or not, and to block against it even if
	// it's been detached -- removing the need for m_mtx_InThread and the semaphore wait above.  But
	// pthreads kinda lacks that stuff, since pthread_join() has no timeout option making it im-
	// possible to safely block against a running thread)
}

// Returns: TRUE if the detachment was performed, or FALSE if the thread was
// already detached or isn't running at all.
// This function should not be called from the owner thread.
bool Threading::pxThread::Detach()
{
	AffinityAssert_DisallowFromSelf(pxDiagSpot);

	if( _InterlockedExchange( &m_detached, true ) ) return false;
	pthread_detach( m_thread );
	return true;
}

bool Threading::pxThread::_basecancel()
{
	if( !m_running ) return false;

	if( m_detached )
	{
		pxThreadLog.Warn(GetName(), L"Ignoring attempted cancellation of detached thread.");
		return false;
	}

	pthread_cancel( m_thread );
	return true;
}

// Remarks:
//   Provision of non-blocking Cancel() is probably academic, since destroying a pxThread
//   object performs a blocking Cancel regardless of if you explicitly do a non-blocking Cancel()
//   prior, since the ExecuteTaskInThread() method requires a valid object state.  If you really need
//   fire-and-forget behavior on threads, use pthreads directly for now.
//
// This function should not be called from the owner thread.
//
// Parameters:
//   isBlocking - indicates if the Cancel action should block for thread completion or not.
//
// Exceptions raised by the blocking thread will be re-thrown into the main thread.  If isBlocking
// is false then no exceptions will occur.
//
void Threading::pxThread::Cancel( bool isBlocking )
{
	AffinityAssert_DisallowFromSelf( pxDiagSpot );

	// Prevent simultaneous startup and cancel, necessary to avoid
	ScopedLock startlock( m_mtx_start );

	if( !_basecancel() ) return;

	if( isBlocking )
	{
		WaitOnSelf( m_mtx_InThread );
		Detach();
	}
}

bool Threading::pxThread::Cancel( const wxTimeSpan& timespan )
{
	AffinityAssert_DisallowFromSelf( pxDiagSpot );

	// Prevent simultaneous startup and cancel:
	ScopedLock startlock( m_mtx_start );

	if( !_basecancel() ) return true;

	if( !WaitOnSelf( m_mtx_InThread, timespan ) ) return false;
	Detach();
	return true;
}


// Blocks execution of the calling thread until this thread completes its task.  The
// caller should make sure to signal the thread to exit, or else blocking may deadlock the
// calling thread.  Classes which extend pxThread should override this method
// and signal any necessary thread exit variables prior to blocking.
//
// Returns the return code of the thread.
// This method is roughly the equivalent of pthread_join().
//
// Exceptions raised by the blocking thread will be re-thrown into the main thread.
//
void Threading::pxThread::Block()
{
	AffinityAssert_DisallowFromSelf(pxDiagSpot);
	WaitOnSelf( m_mtx_InThread );
}

bool Threading::pxThread::Block( const wxTimeSpan& timeout )
{
	AffinityAssert_DisallowFromSelf(pxDiagSpot);
	return WaitOnSelf( m_mtx_InThread, timeout );
}

bool Threading::pxThread::IsSelf() const
{
	// Detached threads may have their pthread handles recycled as newer threads, causing
	// false IsSelf reports.
	return !m_detached && (pthread_self() == m_thread);
}

bool Threading::pxThread::IsRunning() const
{
    return !!m_running;
}

void Threading::pxThread::AddListener( EventListener_Thread& evt )
{
	evt.SetThread( this );
	m_evtsrc_OnDelete.Add( evt );
}

// Throws an exception if the thread encountered one.  Uses the BaseException's Rethrow() method,
// which ensures the exception type remains consistent.  Debuggable stacktraces will be lost, since
// the thread will have allowed itself to terminate properly.
void Threading::pxThread::RethrowException() const
{
	// Thread safety note: always detach the m_except pointer.  If we checked it for NULL, the
	// pointer might still be invalid after detachment, so might as well just detach and check
	// after.

	ScopedExcept ptr( const_cast<pxThread*>(this)->m_except.DetachPtr() );
	if (ptr)
		ptr->Rethrow();
}

static bool m_BlockDeletions = false;

bool Threading::AllowDeletions()
{
	AffinityAssert_AllowFrom_MainUI();
	return !m_BlockDeletions;
}

void Threading::YieldToMain()
{
	m_BlockDeletions = true;
	wxTheApp->Yield( true );
	m_BlockDeletions = false;
}

void Threading::pxThread::_selfRunningTest( const wxChar* name ) const
{
	if( HasPendingException() )
	{
		pxThreadLog.Error( GetName(), pxsFmt(L"An exception was thrown while waiting on a %s.", name) );
		RethrowException();
	}

	if( !m_running )
	{
		throw Exception::CancelEvent( pxsFmt(
			L"Blocking thread %s was terminated while another thread was waiting on a %s.",
			GetName().c_str(), name )
		);
	}

	// Thread is still alive and kicking (for now) -- yield to other messages and hope
	// that impending chaos does not ensue.  [it shouldn't since we block pxThread
	// objects from being deleted until outside the scope of a mutex/semaphore wait).

	if( (wxTheApp != NULL) && wxThread::IsMain() && !_WaitGui_RecursionGuard( L"WaitForSelf" ) )
		Threading::YieldToMain();
}

// This helper function is a deadlock-safe method of waiting on a semaphore in a pxThread.  If the
// thread is terminated or canceled by another thread or a nested action prior to the semaphore being
// posted, this function will detect that and throw a CancelEvent exception is thrown.
//
// Note: Use of this function only applies to semaphores which are posted by the worker thread.  Calling
// this function from the context of the thread itself is an error, and a dev assertion will be generated.
//
// Exceptions:
//   This function will rethrow exceptions raised by the persistent thread, if it throws an error
//   while the calling thread is blocking (which also means the persistent thread has terminated).
//
void Threading::pxThread::WaitOnSelf( Semaphore& sem ) const
{
	if( !AffinityAssert_DisallowFromSelf(pxDiagSpot) ) return;

	while( true )
	{
		if( sem.WaitWithoutYield( wxTimeSpan(0, 0, 0, 333) ) ) return;
		_selfRunningTest( L"semaphore" );
	}
}

// This helper function is a deadlock-safe method of waiting on a mutex in a pxThread.
// If the thread is terminated or canceled by another thread or a nested action prior to the
// mutex being unlocked, this function will detect that and a CancelEvent exception is thrown.
//
// Note: Use of this function only applies to mutexes which are acquired by a worker thread.
// Calling this function from the context of the thread itself is an error, and a dev assertion
// will be generated.
//
// Exceptions:
//   This function will rethrow exceptions raised by the persistent thread, if it throws an
//   error while the calling thread is blocking (which also means the persistent thread has
//   terminated).
//
void Threading::pxThread::WaitOnSelf( Mutex& mutex ) const
{
	if( !AffinityAssert_DisallowFromSelf(pxDiagSpot) ) return;

	while( true )
	{
		if( mutex.WaitWithoutYield( wxTimeSpan(0, 0, 0, 333) ) ) return;
		_selfRunningTest( L"mutex" );
	}
}

static const wxTimeSpan SelfWaitInterval( 0,0,0,333 );

bool Threading::pxThread::WaitOnSelf( Semaphore& sem, const wxTimeSpan& timeout ) const
{
	if( !AffinityAssert_DisallowFromSelf(pxDiagSpot) ) return true;

	wxTimeSpan runningout( timeout );

	while( runningout.GetMilliseconds() > 0 )
	{
		const wxTimeSpan interval( (SelfWaitInterval < runningout) ? SelfWaitInterval : runningout );
		if( sem.WaitWithoutYield( interval ) ) return true;
		_selfRunningTest( L"semaphore" );
		runningout -= interval;
	}
	return false;
}

bool Threading::pxThread::WaitOnSelf( Mutex& mutex, const wxTimeSpan& timeout ) const
{
	if( !AffinityAssert_DisallowFromSelf(pxDiagSpot) ) return true;

	wxTimeSpan runningout( timeout );

	while( runningout.GetMilliseconds() > 0 )
	{
		const wxTimeSpan interval( (SelfWaitInterval < runningout) ? SelfWaitInterval : runningout );
		if( mutex.WaitWithoutYield( interval ) ) return true;
		_selfRunningTest( L"mutex" );
		runningout -= interval;
	}
	return false;
}

// Inserts a thread cancellation point.  If the thread has received a cancel request, this
// function will throw an SEH exception designed to exit the thread (so make sure to use C++
// object encapsulation for anything that could leak resources, to ensure object unwinding
// and cleanup, or use the DoThreadCleanup() override to perform resource cleanup).
void Threading::pxThread::TestCancel() const
{
	AffinityAssert_AllowFromSelf(pxDiagSpot);
	pthread_testcancel();
}

// Executes the virtual member method
void Threading::pxThread::_try_virtual_invoke( void (pxThread::*method)() )
{
	try {
		(this->*method)();
	}

	// ----------------------------------------------------------------------------
	// Neat repackaging for STL Runtime errors...
	//
	catch( std::runtime_error& ex )
	{
		m_except = new Exception::RuntimeError( ex, GetName().c_str() );
	}

	// ----------------------------------------------------------------------------
	catch( Exception::RuntimeError& ex )
	{
		BaseException* woot = ex.Clone();
		woot->DiagMsg() += pxsFmt( L"(thread:%s)", GetName().c_str() );
		m_except = woot;
	}
#ifndef PCSX2_DEVBUILD
	// ----------------------------------------------------------------------------
	// Bleh... don't bother with std::exception.  runtime_error should catch anything
	// useful coming out of the core STL libraries anyway, and these are best handled by
	// the MSVC debugger (or by silent random annoying fail on debug-less linux).
	/*catch( std::logic_error& ex )
	{
		throw BaseException( pxsFmt( L"STL Logic Error (thread:%s): %s",
			GetName().c_str(), fromUTF8( ex.what() ).c_str() )
		);
	}
	catch( std::exception& ex )
	{
		throw BaseException( pxsFmt( L"STL exception (thread:%s): %s",
			GetName().c_str(), fromUTF8( ex.what() ).c_str() )
		);
	}*/
	// ----------------------------------------------------------------------------
	// BaseException --  same deal as LogicErrors.
	//
	catch( BaseException& ex )
	{
		BaseException* woot = ex.Clone();
		woot->DiagMsg() += pxsFmt( L"(thread:%s)", GetName().c_str() );
		m_except = woot;
	}
#endif
}

// invoked internally when canceling or exiting the thread.  Extending classes should implement
// OnCleanupInThread() to extend cleanup functionality.
void Threading::pxThread::_ThreadCleanup()
{
	AffinityAssert_AllowFromSelf(pxDiagSpot);
	_try_virtual_invoke( &pxThread::OnCleanupInThread );
	m_mtx_InThread.Release();

	// Must set m_running LAST, as thread destructors depend on this value (it is used
	// to avoid destruction of the thread until all internal data use has stopped.
	m_running = false;
}

wxString Threading::pxThread::GetName() const
{
	ScopedLock lock(m_mtx_ThreadName);
	return m_name;
}

void Threading::pxThread::SetName( const wxString& newname )
{
	ScopedLock lock(m_mtx_ThreadName);
	m_name = newname;
}

// This override is called by PeristentThread when the thread is first created, prior to
// calling ExecuteTaskInThread, and after the initial InThread lock has been claimed.
// This code is also executed within a "safe" environment, where the creating thread is
// blocked against m_sem_event.  Make sure to do any necessary variable setup here, without
// worry that the calling thread might attempt to test the status of those variables
// before initialization has completed.
//
void Threading::pxThread::OnStartInThread()
{
	m_detached	= false;
	m_running	= true;

	_platform_specific_OnStartInThread();
}

void Threading::pxThread::_internal_execute()
{
	m_mtx_InThread.Acquire();

	_DoSetThreadName( GetName() );
	make_curthread_key(this);
	if( curthread_key )
		pthread_setspecific( curthread_key, this );

	OnStartInThread();
	m_sem_startup.Post();

	_try_virtual_invoke( &pxThread::ExecuteTaskInThread );
}

// Called by Start, prior to actual starting of the thread, and after any previous
// running thread has been canceled or detached.
void Threading::pxThread::OnStart()
{
	m_native_handle	= 0;
	m_native_id		= 0;

	FrankenMutex( m_mtx_InThread );
	m_sem_event.Reset();
	m_sem_startup.Reset();
}

// Extending classes that override this method should always call it last from their
// personal implementations.
void Threading::pxThread::OnCleanupInThread()
{
	if( curthread_key )
		pthread_setspecific( curthread_key, NULL );

	unmake_curthread_key();

	_platform_specific_OnCleanupInThread();

	m_native_handle = 0;
	m_native_id		= 0;
	
	m_evtsrc_OnDelete.Dispatch( 0 );
}

// passed into pthread_create, and is used to dispatch the thread's object oriented
// callback function
void* Threading::pxThread::_internal_callback( void* itsme )
{
	if( !pxAssertDev( itsme != NULL, wxNullChar ) ) return NULL;
	pxThread& owner = *((pxThread*)itsme);

	pthread_cleanup_push( _pt_callback_cleanup, itsme );
	owner._internal_execute();
	pthread_cleanup_pop( true );
	return NULL;
}

void Threading::pxThread::_DoSetThreadName( const wxString& name )
{
	_DoSetThreadName( name.ToUTF8() );
}

// --------------------------------------------------------------------------------------
//  BaseTaskThread Implementations
// --------------------------------------------------------------------------------------

// Tells the thread to exit and then waits for thread termination.
void Threading::BaseTaskThread::Block()
{
	if( !IsRunning() ) return;
	m_Done = true;
	m_sem_event.Post();
	pxThread::Block();
}

// Initiates the new task.  This should be called after your own StartTask has
// initialized internal variables / preparations for task execution.
void Threading::BaseTaskThread::PostTask()
{
	pxAssert( !m_detached );

	ScopedLock locker( m_lock_TaskComplete );
	m_TaskPending = true;
	m_post_TaskComplete.Reset();
	m_sem_event.Post();
}

// Blocks current thread execution pending the completion of the parallel task.
void Threading::BaseTaskThread::WaitForResult()
{
	if( m_detached || !m_running ) return;
	if( m_TaskPending )
	#if wxUSE_GUI
		m_post_TaskComplete.Wait();
	#else
		m_post_TaskComplete.WaitWithoutYield();
	#endif

	m_post_TaskComplete.Reset();
}

void Threading::BaseTaskThread::ExecuteTaskInThread()
{
	while( !m_Done )
	{
		// Wait for a job -- or get a pthread_cancel.  I'm easy.
		m_sem_event.WaitWithoutYield();

		Task();
		m_lock_TaskComplete.Acquire();
		m_TaskPending = false;
		m_post_TaskComplete.Post();
		m_lock_TaskComplete.Release();
	};

	return;
}

// --------------------------------------------------------------------------------------
//  pthread Cond is an evil api that is not suited for Pcsx2 needs.
//  Let's not use it. (Air)
// --------------------------------------------------------------------------------------

#if 0
Threading::WaitEvent::WaitEvent()
{
	int err = 0;

	err = pthread_cond_init(&cond, NULL);
	err = pthread_mutex_init(&mutex, NULL);
}

Threading::WaitEvent::~WaitEvent() throw()
{
	pthread_cond_destroy( &cond );
	pthread_mutex_destroy( &mutex );
}

void Threading::WaitEvent::Set()
{
	pthread_mutex_lock( &mutex );
	pthread_cond_signal( &cond );
	pthread_mutex_unlock( &mutex );
}

void Threading::WaitEvent::Wait()
{
	pthread_mutex_lock( &mutex );
	pthread_cond_wait( &cond, &mutex );
	pthread_mutex_unlock( &mutex );
}
#endif

// --------------------------------------------------------------------------------------
//  InterlockedExchanges / AtomicExchanges (PCSX2's Helper versions)
// --------------------------------------------------------------------------------------
// define some overloads for InterlockedExchanges for commonly used types, like u32 and s32.
// Note: For all of these atomic operations below to be atomic, the variables need to be 4-byte
// aligned. Read: http://msdn.microsoft.com/en-us/library/ms684122%28v=vs.85%29.aspx

__fi u32 Threading::AtomicRead(volatile u32& Target) {
	return Target; // Properly-aligned 32-bit reads are atomic
}
__fi s32 Threading::AtomicRead(volatile s32& Target) {
	return Target; // Properly-aligned 32-bit reads are atomic
}

__fi bool Threading::AtomicBitTestAndReset( volatile u32& bitset, u8 bit ) {
	return _interlockedbittestandreset( (volatile long*)& bitset, bit ) != 0;
}
__fi bool Threading::AtomicBitTestAndReset( volatile s32& bitset, u8 bit ) {
	return _interlockedbittestandreset( (volatile long*)& bitset, bit ) != 0;
}

__fi u32 Threading::AtomicExchange(volatile u32& Target, u32 value ) {
	return _InterlockedExchange( (volatile long*)&Target, value );
}
__fi s32 Threading::AtomicExchange( volatile s32& Target, s32 value ) {
	return _InterlockedExchange( (volatile long*)&Target, value );
}

__fi u32 Threading::AtomicExchangeAdd( volatile u32& Target, u32 value ) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, value );
}
__fi s32 Threading::AtomicExchangeAdd( volatile s32& Target, s32 value ) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, value );
}

__fi s32 Threading::AtomicExchangeSub( volatile s32& Target, s32 value ) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, -value );
}

__fi u32 Threading::AtomicIncrement( volatile u32& Target ) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, 1 );
}
__fi s32 Threading::AtomicIncrement( volatile s32& Target) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, 1 );
}

__fi u32 Threading::AtomicDecrement( volatile u32& Target ) {
	return _InterlockedExchangeAdd( (volatile long*)&Target, -1 );
}
__fi s32 Threading::AtomicDecrement(volatile s32& Target) {
	return _InterlockedExchangeAdd((volatile long*)&Target, -1);
}

__fi void* Threading::_AtomicExchangePointer(volatile uptr& target, uptr value)
{
#ifdef _M_AMD64		// high-level atomic ops, please leave these 64 bit checks in place.
	return (void*)_InterlockedExchange64(&(volatile s64&)target, value);
#else
	return (void*)_InterlockedExchange((volatile long*)&target, value);
#endif
}

__fi void* Threading::_AtomicCompareExchangePointer(volatile uptr& target, uptr value, uptr comparand)
{
#ifdef _M_AMD64		// high-level atomic ops, please leave these 64 bit checks in place.
	return (void*)_InterlockedCompareExchange64(&(volatile s64&)target, value);
#else
	return (void*)_InterlockedCompareExchange(&(volatile long&)target, value, comparand);
#endif
}

// --------------------------------------------------------------------------------------
//  BaseThreadError
// --------------------------------------------------------------------------------------

wxString Exception::BaseThreadError::FormatDiagnosticMessage() const
{
    return pxsFmt( m_message_diag, (m_thread==NULL) ? L"Null Thread Object" : m_thread->GetName().c_str());
}

wxString Exception::BaseThreadError::FormatDisplayMessage() const
{
    return pxsFmt( m_message_user, (m_thread==NULL) ? L"Null Thread Object" : m_thread->GetName().c_str());
}

pxThread& Exception::BaseThreadError::Thread()
{
	pxAssertDev( m_thread != NULL, "NULL thread object on ThreadError exception." );
	return *m_thread;
}
const pxThread& Exception::BaseThreadError::Thread() const
{
	pxAssertDev( m_thread != NULL, "NULL thread object on ThreadError exception." );
	return *m_thread;
}
