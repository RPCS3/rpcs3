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


#include "PrecompiledHeader.h"

#ifdef _WIN32
#	include <wx/msw/wrapwin.h>	// for thread renaming features
#endif
#include <wx/app.h>

#ifdef __LINUX__
#	include <signal.h>		// for pthread_kill, which is in pthread.h on w32-pthreads
#endif

#include "Threading.h"
#include "wxBaseTools.h"

#include <wx/datetime.h>
#include <wx/thread.h>

namespace Threading
{
	// 100ms interval for waitgui (issued from blocking semaphore waits on the main thread,
	// to avoid gui deadlock).
	static const wxTimeSpan	ts_waitgui_interval( 0, 0, 0, 100 );

	// Four second interval for deadlock protection on waitgui.
	static const wxTimeSpan	ts_waitgui_deadlock( 0, 0, 4, 0 );

	static long					_attr_refcount = 0;
	static pthread_mutexattr_t	_attr_recursive;
}

__forceinline void Threading::Timeslice()
{
	sched_yield();
}

void Threading::PersistentThread::_pt_callback_cleanup( void* handle )
{
	((PersistentThread*)handle)->_ThreadCleanup();
}

Threading::PersistentThread::PersistentThread() :
	m_name( L"PersistentThread" )
,	m_thread()
,	m_sem_event()
,	m_sem_finished()
,	m_lock_start()
,	m_detached( true )		// start out with m_thread in detached/invalid state
,	m_running( false )
{
}

// This destructor performs basic "last chance" cleanup, which is a blocking join
// against the thread. Extending classes should almost always implement their own
// thread closure process, since any PersistentThread will, by design, not terminate
// unless it has been properly canceled.
//
// Thread safety: This class must not be deleted from its own thread.  That would be
// like marrying your sister, and then cheating on her with your daughter.
Threading::PersistentThread::~PersistentThread() throw()
{
	try
	{
		DevCon.WriteLn( L"(Thread Log) Executing destructor for " + m_name );

		if( m_running )
		{
			DevCon.WriteLn( L"\tWaiting for running thread to end...");
#if wxUSE_GUI
			m_sem_finished.Wait();
#else
			m_sem_finished.WaitRaw();
#endif
			// Need to lock here so that the thread can finish shutting down before
			// it gets destroyed, otherwise th mutex handle would become invalid.
			ScopedLock locker( m_lock_start );
		}
		Threading::Sleep( 1 );
		Detach();
	}
	catch( Exception::ThreadTimedOut& ex )
	{
		// Windows allows for a thread to be terminated forcefully, but it's not really
		// a safe thing to do since typically threads are acquiring and releasing locks
		// and semaphores all the time.  And terminating threads isn't really cross-platform
		// either so let's just not bother.

		// Additionally since this is a destructor most of our derived class info is lost,
		// so we can't allow for customized deadlock handlers, least not in any useful
		// context.  So let's just log the condition and move on.

		Console.Error( wxsFormat(L"\tThread destructor for '%s' timed out with error:\n\t",
			m_name.c_str(), ex.FormatDiagnosticMessage().c_str() ) );
	}
	DESTRUCTOR_CATCHALL
}

// Main entry point for starting or e-starting a persistent thread.  This function performs necessary
// locks and checks for avoiding race conditions, and then calls OnStart() immeediately before
// the actual thread creation.  Extending classes should generally not override Start(), and should
// instead override DoPrepStart instead.
//
// This function should not be called from the owner thread.
void Threading::PersistentThread::Start()
{
	ScopedLock startlock( m_lock_start );		// Prevents sudden parallel startup
	if( m_running ) return;

	Detach();		// clean up previous thread handle, if one exists.
	m_sem_finished.Reset();

	OnStart();

	if( pthread_create( &m_thread, NULL, _internal_callback, this ) != 0 )
		throw Exception::ThreadCreationError();

	m_detached = false;
}

// Returns: TRUE if the detachment was performed, or FALSE if the thread was
// already detached or isn't running at all.
// This function should not be called from the owner thread.
bool Threading::PersistentThread::Detach()
{
	pxAssertMsg( !IsSelf(), "Thread affinity error." );		// not allowed from our own thread.

	if( _InterlockedExchange( &m_detached, true ) ) return false;
	pthread_detach( m_thread );
	return true;
}

// Remarks:
//   Provision of non-blocking Cancel() is probably academic, since destroying a PersistentThread
//   object performs a blocking Cancel regardless of if you explicitly do a non-blocking Cancel()
//   prior, since the ExecuteTaskInThread() method requires a valid object state.  If you really need
//   fire-and-forget behavior on threads, use pthreads directly for now.
//
// This function should not be called from the owner thread.
//
// Parameters:
//   isBlocking - indicates if the Cancel action should block for thread completion or not.
//
void Threading::PersistentThread::Cancel( bool isBlocking )
{
	pxAssertMsg( !IsSelf(), "Thread affinity error." );

	if( !m_running ) return;

	if( m_detached )
	{
		Console.Notice( "(Thread Warning) Ignoring attempted cancelation of detached thread." );
		return;
	}

	pthread_cancel( m_thread );

	if( isBlocking )
	{
#if wxUSE_GUI
		m_sem_finished.Wait();
#else
		m_sem_finished.WaitRaw();
#endif
	}
}

// Blocks execution of the calling thread until this thread completes its task.  The
// caller should make sure to signal the thread to exit, or else blocking may deadlock the
// calling thread.  Classes which extend PersistentThread should override this method
// and signal any necessary thread exit variables prior to blocking.
//
// Returns the return code of the thread.
// This method is roughly the equivalent of pthread_join().
//
void Threading::PersistentThread::Block()
{
	pxAssertDev( !IsSelf(), "Thread deadlock detected; Block() should never be called by the owner thread." );

	if( m_running )
#if wxUSE_GUI
		m_sem_finished.Wait();
#else
		m_sem_finished.WaitRaw();
#endif
}

bool Threading::PersistentThread::IsSelf() const
{
	return pthread_self() == m_thread;
}

bool Threading::PersistentThread::IsRunning() const
{
    return !!m_running;
}

// Throws an exception if the thread encountered one.  Uses the BaseException's Rethrow() method,
// which ensures the exception type remains consistent.  Debuggable stacktraces will be lost, since
// the thread will have allowed itself to terminate properly.
void Threading::PersistentThread::RethrowException() const
{
	if( !m_except ) return;
	m_except->Rethrow();
}

void Threading::PersistentThread::TestCancel()
{
	pxAssert( IsSelf() );
	pthread_testcancel();
}

// Executes the virtual member method
void Threading::PersistentThread::_try_virtual_invoke( void (PersistentThread::*method)() )
{
	try {
		(this->*method)();
	}

	// ----------------------------------------------------------------------------
	// Neat repackaging for STL Runtime errors...
	//
	catch( std::runtime_error& ex )
	{
		m_except = new Exception::RuntimeError(
			// Diagnostic message:
			wxsFormat( L"(thread: %s) STL Runtime Error: %s\n\t%s",
				GetName().c_str(), fromUTF8( ex.what() ).c_str()
			),

			// User Message (not translated, std::exception doesn't have that kind of fancy!
			wxsFormat( L"A runtime error occurred in %s:\n\n%s (STL)",
				GetName().c_str(), fromUTF8( ex.what() ).c_str()
			)
		);
	}

	// ----------------------------------------------------------------------------
	catch( Exception::RuntimeError& ex )
	{
		m_except = ex.Clone();
		m_except->DiagMsg() = wxsFormat( L"(thread:%s) ", GetName().c_str() ) + m_except->DiagMsg();
	}
#ifndef PCSX2_DEVBUILD
	// ----------------------------------------------------------------------------
	// Allow logic errors to propagate out of the thread in release builds, so that they might be
	// handled in non-fatal ways.  On Devbuilds let them loose, so that they produce debug stack
	// traces and such.
	catch( std::logic_error& ex )
	{
		throw Exception::LogicError( wxsFormat( L"(thread: %s) STL Logic Error: %s\n\t%s",
			GetName().c_str(), fromUTF8( ex.what() ).c_str() )
		);
	}
	catch( Exception::LogicError& ex )
	{
		m_except = ex.Clone();
		m_except->DiagMsg() = wxsFormat( L"(thread:%s) ", GetName().c_str() ) + m_except->DiagMsg();
	}
	// ----------------------------------------------------------------------------
	// Bleh... don't bother with std::exception.  std::logic_error and runtime_error should catch
	// anything coming out of the core STL libraries anyway.
	/*catch( std::exception& ex )
	{
		throw Exception::BaseException( wxsFormat( L"(thread: %s) STL exception: %s\n\t%s",
			GetName().c_str(), fromUTF8( ex.what() ).c_str() )
		);
	}*/
	// ----------------------------------------------------------------------------
	// BaseException --  same deal as LogicErrors.
	//
	catch( Exception::BaseException& ex )
	{
		m_except = ex.Clone();
		m_except->DiagMsg() = wxsFormat( L"(thread:%s) ", GetName().c_str() ) + m_except->DiagMsg();
	}
#endif
}

// invoked internally when canceling or exiting the thread.  Extending classes should implement
// OnCleanupInThread() to extend cleanup functionality.
void Threading::PersistentThread::_ThreadCleanup()
{
	pxAssertMsg( IsSelf(), "Thread affinity error." );	// only allowed from our own thread, thanks.

	// Typically thread cleanup needs to lock against thread startup, since both
	// will perform some measure of variable inits or resets, depending on how the
	// derived class is implemented.
	ScopedLock startlock( m_lock_start );

	_try_virtual_invoke( &PersistentThread::OnCleanupInThread );

	m_running = false;
	m_sem_finished.Post();
}

wxString Threading::PersistentThread::GetName() const
{
	return m_name;
}

void Threading::PersistentThread::_internal_execute()
{
	m_running = true;
	_DoSetThreadName( m_name );
	_try_virtual_invoke( &PersistentThread::ExecuteTaskInThread );
}

void Threading::PersistentThread::OnStart() {}
void Threading::PersistentThread::OnCleanupInThread() {}

// passed into pthread_create, and is used to dispatch the thread's object oriented
// callback function
void* Threading::PersistentThread::_internal_callback( void* itsme )
{
	jASSUME( itsme != NULL );
	PersistentThread& owner = *((PersistentThread*)itsme);

	pthread_cleanup_push( _pt_callback_cleanup, itsme );
	owner._internal_execute();
	pthread_cleanup_pop( true );
	return NULL;
}

void Threading::PersistentThread::_DoSetThreadName( const wxString& name )
{
	_DoSetThreadName( name.ToUTF8() );
}

void Threading::PersistentThread::_DoSetThreadName( __unused const char* name )
{
	pxAssertMsg( IsSelf(), "Thread affinity error." );	// only allowed from our own thread, thanks.

	// This feature needs Windows headers and MSVC's SEH support:

#if defined(_WINDOWS_) && defined (_MSC_VER)

	// This code sample was borrowed form some obscure MSDN article.
	// In a rare bout of sanity, it's an actual Micrsoft-published hack
	// that actually works!

	static const int MS_VC_EXCEPTION = 0x406D1388;

	#pragma pack(push,8)
	struct THREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	};
	#pragma pack(pop)

	THREADNAME_INFO info;
	info.dwType		= 0x1000;
	info.szName		= name;
	info.dwThreadID	= GetCurrentThreadId();
	info.dwFlags	= 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
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
	PersistentThread::Block();
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
	#ifdef wxUSE_GUI
		m_post_TaskComplete.Wait();
	#else
		m_post_TaskComplete.WaitRaw();
	#endif

	m_post_TaskComplete.Reset();
}

void Threading::BaseTaskThread::ExecuteTaskInThread()
{
	while( !m_Done )
	{
		// Wait for a job -- or get a pthread_cancel.  I'm easy.
		m_sem_event.WaitRaw();

		Task();
		m_lock_TaskComplete.Lock();
		m_TaskPending = false;
		m_post_TaskComplete.Post();
		m_lock_TaskComplete.Unlock();
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
//  Semaphore Implementations
// --------------------------------------------------------------------------------------

Threading::Semaphore::Semaphore()
{
	sem_init( &m_sema, false, 0 );
}

Threading::Semaphore::~Semaphore() throw()
{
	sem_destroy( &m_sema );
}

void Threading::Semaphore::Reset()
{
	sem_destroy( &m_sema );
	sem_init( &m_sema, false, 0 );
}

void Threading::Semaphore::Post()
{
	sem_post( &m_sema );
}

void Threading::Semaphore::Post( int multiple )
{
#if defined(_MSC_VER)
	sem_post_multiple( &m_sema, multiple );
#else
	// Only w32pthreads has the post_multiple, but it's easy enough to fake:
	while( multiple > 0 )
	{
		multiple--;
		sem_post( &m_sema );
	}
#endif
}

#if wxUSE_GUI
// (intended for internal use only)
// Returns true if the Wait is recursive, or false if the Wait is safe and should be
// handled via normal yielding methods.
bool Threading::Semaphore::_WaitGui_RecursionGuard()
{
	// In order to avoid deadlock we need to make sure we cut some time to handle
	// messages.  But this can result in recursive yield calls, which would crash
	// the app.  Protect against them here and, if recursion is detected, perform
	// a standard blocking wait.

	static int __Guard = 0;
	RecursionGuard guard( __Guard );

	if( guard.Counter > 4 )
	{
		Console.WriteLn( "(Thread Log) Possible yield recursion detected in Semaphore::Wait; performing blocking wait." );
		//while( wxTheApp->Pending() ) wxTheApp->Dispatch();		// ensures console gets updated.
		return true;
	}
	return false;
}

// This is a wxApp-safe implementation of Wait, which makes sure and executes the App's
// pending messages *if* the Wait is performed on the Main/GUI thread.  This ensures that
// user input continues to be handled and that windoes continue to repaint.  If the Wait is
// called from another thread, no message pumping is performed.
//
// Exceptions:
//   ThreadTimedOut - thrown if a blocking wait was needed due to recursion and the default
//      timeout period (usually 4 seconds) was reached, indicating likely deadlock.  If
//      the method is run from a thread *other* than the MainGui thread, this exception
//      cannot occur.
//
void Threading::Semaphore::Wait()
{
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		WaitRaw();
	}
	else if( _WaitGui_RecursionGuard() )
	{
		if( !WaitRaw(ts_waitgui_deadlock) )	// default is 4 seconds
			throw Exception::ThreadTimedOut();
	}
	else
	{
		do {
			wxTheApp->Yield( true );
		} while( !WaitRaw( ts_waitgui_interval ) );
	}
}

// This is a wxApp-safe implementation of Wait, which makes sure and executes the App's
// pending messages *if* the Wait is performed on the Main/GUI thread.  This ensures that
// user input continues to be handled and that windows continue to repaint.  If the Wait is
// called from another thread, no message pumping is performed.
//
// Returns:
//   false if the wait timed out before the semaphore was signaled, or true if the signal was
//   reached prior to timeout.
//
// Exceptions:
//   ThreadTimedOut - thrown if a blocking wait was needed due to recursion and the default
//      timeout period (usually 4 seconds) was reached, indicating likely deadlock.  If the
//      user-specified timeout is less than four seconds, this exception cannot occur.  If
//      the method is run from a thread *other* than the MainGui thread, this exception
//      cannot occur.
//
bool Threading::Semaphore::Wait( const wxTimeSpan& timeout )
{
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		return WaitRaw( timeout );
	}
	else if( _WaitGui_RecursionGuard() )
	{
		if( timeout > ts_waitgui_deadlock )
		{
			if( WaitRaw(ts_waitgui_deadlock) ) return true;
			throw Exception::ThreadTimedOut();
		}
		return WaitRaw( timeout );
	}
	else
	{
		wxTimeSpan countdown( (timeout) );

		do {
			wxTheApp->Yield();
			if( WaitRaw( ts_waitgui_interval ) ) break;
			countdown -= ts_waitgui_interval;
		} while( countdown.GetMilliseconds() > 0 );

		return countdown.GetMilliseconds() > 0;
	}
}
#endif

void Threading::Semaphore::WaitRaw()
{
	sem_wait( &m_sema );
}

bool Threading::Semaphore::WaitRaw( const wxTimeSpan& timeout )
{
	wxDateTime megafail( wxDateTime::UNow() + timeout );
	const timespec fail = { megafail.GetTicks(), megafail.GetMillisecond() * 1000000 };
	return sem_timedwait( &m_sema, &fail ) != -1;
}

// Performs an uncancellable wait on a semaphore; restoring the thread's previous cancel state
// after the wait has completed.  Useful for situations where the semaphore itself is stored on
// the stack and passed to another thread via GUI message or such, avoiding complications where
// the thread might be canceled and the stack value becomes invalid.
//
// Performance note: this function has quite a bit more overhead compared to Semaphore::WaitRaw(), so
// consider manually specifying the thread as uncancellable and using WaitRaw() instead if you need
// to do a lot of no-cancel waits in a tight loop worker thread, for example.
void Threading::Semaphore::WaitNoCancel()
{
	int oldstate;
	pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &oldstate );
	WaitRaw();
	pthread_setcancelstate( oldstate, NULL );
}

int Threading::Semaphore::Count()
{
	int retval;
	sem_getvalue( &m_sema, &retval );
	return retval;
}

// --------------------------------------------------------------------------------------
//  MutexLock Implementations
// --------------------------------------------------------------------------------------

Threading::MutexLock::MutexLock()
{
	int err = 0;
	err = pthread_mutex_init( &mutex, NULL );
}

Threading::MutexLock::~MutexLock() throw()
{
	pthread_mutex_destroy( &mutex );
}

Threading::MutexLockRecursive::MutexLockRecursive() : MutexLock( false )
{
	if( _InterlockedIncrement( &_attr_refcount ) == 1 )
	{
		if( 0 != pthread_mutexattr_init( &_attr_recursive ) )
			throw Exception::OutOfMemory( "Out of memory error initializing the Mutex attributes for recursive mutexing." );

		pthread_mutexattr_settype( &_attr_recursive, PTHREAD_MUTEX_RECURSIVE );
	}

	int err = 0;
	err = pthread_mutex_init( &mutex, &_attr_recursive );
}

Threading::MutexLockRecursive::~MutexLockRecursive() throw()
{
	if( _InterlockedDecrement( &_attr_refcount ) == 0 )
		pthread_mutexattr_destroy( &_attr_recursive );
}

void Threading::MutexLock::Lock()
{
	pthread_mutex_lock( &mutex );
}

void Threading::MutexLock::Unlock()
{
	pthread_mutex_unlock( &mutex );
}

bool Threading::MutexLock::TryLock()
{
	return EBUSY != pthread_mutex_trylock( &mutex );
}

// --------------------------------------------------------------------------------------
//  InterlockedExchanges / AtomicExchanges (PCSX2's Helper versions)
// --------------------------------------------------------------------------------------
// define some overloads for InterlockedExchanges for commonly used types, like u32 and s32.

__forceinline void Threading::AtomicExchange( volatile u32& Target, u32 value )
{
	_InterlockedExchange( (volatile long*)&Target, value );
}

__forceinline void Threading::AtomicExchangeAdd( volatile u32& Target, u32 value )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, value );
}

__forceinline void Threading::AtomicIncrement( volatile u32& Target )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, 1 );
}

__forceinline void Threading::AtomicDecrement( volatile u32& Target )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, -1 );
}

__forceinline void Threading::AtomicExchange( volatile s32& Target, s32 value )
{
	_InterlockedExchange( (volatile long*)&Target, value );
}

__forceinline void Threading::AtomicExchangeAdd( volatile s32& Target, u32 value )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, value );
}

__forceinline void Threading::AtomicIncrement( volatile s32& Target )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, 1 );
}

__forceinline void Threading::AtomicDecrement( volatile s32& Target )
{
	_InterlockedExchangeAdd( (volatile long*)&Target, -1 );
}

__forceinline void Threading::_AtomicExchangePointer( const void ** target, const void* value )
{
	_InterlockedExchange( (volatile long*)target, (long)value );
}

__forceinline void Threading::_AtomicCompareExchangePointer( const void ** target, const void* value, const void* comparand )
{
	_InterlockedCompareExchange( (volatile long*)target, (long)value, (long)comparand );
}
