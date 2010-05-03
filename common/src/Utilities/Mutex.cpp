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

#include "Threading.h"
#include "wxBaseTools.h"
#include "wxGuiTools.h"
#include "ThreadingInternal.h"

namespace Threading
{
	static long					_attr_refcount = 0;
	static pthread_mutexattr_t	_attr_recursive;
}

// --------------------------------------------------------------------------------------
//  Mutex Implementations
// --------------------------------------------------------------------------------------

Threading::Mutex::Mutex()
{
	pthread_mutex_init( &m_mutex, NULL );
}

static wxTimeSpan def_detach_timeout( 0, 0, 6, 0 );

void Threading::Mutex::Detach()
{
	if( EBUSY != pthread_mutex_destroy(&m_mutex) ) return;

	if( IsRecursive() )
	{
		// Sanity check: Recursive locks could be held by our own thread, which would
		// be considered an assertion failure, but can also be handled gracefully.
		// (note: if the mutex is locked recursively more than twice then this assert won't
		//  detect it)

		Release(); Release();		// in case of double recursion.
		int result = pthread_mutex_destroy( &m_mutex );
		if( pxAssertDev( result != EBUSY, "Detachment of a recursively-locked mutex (self-locked!)." ) ) return;
	}

	if( Wait(def_detach_timeout) )
		pthread_mutex_destroy( &m_mutex );
	else
		Console.Error( "(Thread Log) Mutex cleanup failed due to possible deadlock.");
}

Threading::Mutex::~Mutex() throw()
{
	try {
		Mutex::Detach();
	} DESTRUCTOR_CATCHALL;
}

Threading::MutexRecursive::MutexRecursive() : Mutex( false )
{
	if( _InterlockedIncrement( &_attr_refcount ) == 1 )
	{
		if( 0 != pthread_mutexattr_init( &_attr_recursive ) )
			throw Exception::OutOfMemory( "Out of memory error initializing the Mutex attributes for recursive mutexing." );

		pthread_mutexattr_settype( &_attr_recursive, PTHREAD_MUTEX_RECURSIVE );
	}

	int err = 0;
	err = pthread_mutex_init( &m_mutex, &_attr_recursive );
}

Threading::MutexRecursive::~MutexRecursive() throw()
{
	if( _InterlockedDecrement( &_attr_refcount ) == 0 )
		pthread_mutexattr_destroy( &_attr_recursive );
}

// This is a bit of a hackish function, which is technically unsafe, but can be useful for allowing
// the application to survive unexpected or inconvenient failures, where a mutex is deadlocked by
// a rogue thread.  This function allows us to Recreate the mutex and let the deadlocked one ponder
// the deeper meanings of the universe for eternity.
void Threading::Mutex::Recreate()
{
	Detach();
	pthread_mutex_init( &m_mutex, NULL );
}

// Returns:
//   true if the mutex had to be recreated due to lock contention, or false if the mutex is safely
//   unlocked.
bool Threading::Mutex::RecreateIfLocked()
{
	if( !Wait(def_detach_timeout) )
	{
		Recreate();
		return true;
	}
	return false;
}


// This is a direct blocking action -- very fast, very efficient, and generally very dangerous
// if used from the main GUI thread, since it typically results in an unresponsive program.
// Call this method directly only if you know the code in question will be run from threads
// other than the main thread.
void Threading::Mutex::AcquireWithoutYield()
{
	pxAssertMsg( !wxThread::IsMain(), "Unyielding mutex acquire issued from the main/gui thread.  Please use Acquire() instead." );
	pthread_mutex_lock( &m_mutex );
}

bool Threading::Mutex::AcquireWithoutYield( const wxTimeSpan& timeout )
{
	wxDateTime megafail( wxDateTime::UNow() + timeout );
	const timespec fail = { megafail.GetTicks(), megafail.GetMillisecond() * 1000000 };
	return pthread_mutex_timedlock( &m_mutex, &fail ) == 0;
}

void Threading::Mutex::Release()
{
	pthread_mutex_unlock( &m_mutex );
}

bool Threading::Mutex::TryAcquire()
{
	return EBUSY != pthread_mutex_trylock( &m_mutex );
}

// This is a wxApp-safe rendition of AcquireWithoutYield, which makes sure to execute pending app events
// and messages *if* the lock is performed from the main GUI thread.
//
void Threading::Mutex::Acquire()
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		pthread_mutex_lock( &m_mutex );
	}
	else if( _WaitGui_RecursionGuard( L"Mutex::Acquire" ) )
	{
		ScopedBusyCursor hourglass( Cursor_ReallyBusy );
		pthread_mutex_lock( &m_mutex );
	}
	else
	{
		//ScopedBusyCursor hourglass( Cursor_KindaBusy );
		while( !AcquireWithoutYield(def_yieldgui_interval) )
			YieldToMain();
	}
#else
	pthread_mutex_lock( &m_mutex );
#endif
}

bool Threading::Mutex::Acquire( const wxTimeSpan& timeout )
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		return AcquireWithoutYield(timeout);
	}
	else if( _WaitGui_RecursionGuard( L"Mutex::TimedAcquire" ) )
	{
		ScopedBusyCursor hourglass( Cursor_ReallyBusy );
		return AcquireWithoutYield( timeout );
	}
	else
	{
		//ScopedBusyCursor hourglass( Cursor_KindaBusy );
		wxTimeSpan countdown( (timeout) );

		do {
			if( AcquireWithoutYield( def_yieldgui_interval ) ) break;
			YieldToMain();
			countdown -= def_yieldgui_interval;
		} while( countdown.GetMilliseconds() > 0 );

		return countdown.GetMilliseconds() > 0;
	}

#else
	return AcquireWithoutYield();
#endif
}

// Performs a wait on a locked mutex, or returns instantly if the mutex is unlocked.
// Typically this action is used to determine if a thread is currently performing some
// specific task, and to block until the task is finished (PersistentThread uses it to
// determine if the thread is running or completed, for example).
//
// Implemented internally as a simple Acquire/Release pair.
//
void Threading::Mutex::Wait()
{
	Acquire();
	Release();
}

void Threading::Mutex::WaitWithoutYield()
{
	AcquireWithoutYield();
	Release();
}

// Performs a wait on a locked mutex, or returns instantly if the mutex is unlocked.
// (Implemented internally as a simple Acquire/Release pair.)
//
// Returns:
//   true if the mutex was freed and is in an unlocked state; or false if the wait timed out
//   and the mutex is still locked by another thread.
//
bool Threading::Mutex::Wait( const wxTimeSpan& timeout )
{
	if( Acquire(timeout) )
	{
		Release();
		return true;
	}
	return false;
}

bool Threading::Mutex::WaitWithoutYield( const wxTimeSpan& timeout )
{
	if( AcquireWithoutYield(timeout) )
	{
		Release();
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------
//  ScopedLock Implementations
// --------------------------------------------------------------------------------------

Threading::ScopedLock::~ScopedLock() throw()
{
	if( m_IsLocked && m_lock )
		m_lock->Release();
}

Threading::ScopedLock::ScopedLock( const Mutex* locker )
{
	AssignAndLock( locker );
}

Threading::ScopedLock::ScopedLock( const Mutex& locker )
{
	AssignAndLock( locker );
}

void Threading::ScopedLock::AssignAndLock( const Mutex& locker )
{
	AssignAndLock( &locker );
}

void Threading::ScopedLock::AssignAndLock( const Mutex* locker )
{
	m_lock = const_cast<Mutex*>(locker);
	if( !m_lock ) return;

	m_IsLocked = true;
	m_lock->Acquire();		
}

void Threading::ScopedLock::Assign( const Mutex& locker )
{
	m_lock = const_cast<Mutex*>(&locker);
}

void Threading::ScopedLock::Assign( const Mutex* locker )
{
	m_lock = const_cast<Mutex*>(locker);
}

// Provides manual unlocking of a scoped lock prior to object destruction.
void Threading::ScopedLock::Release()
{
	if( !m_IsLocked ) return;
	m_IsLocked = false;
	if( m_lock ) m_lock->Release();
}

// provides manual locking of a scoped lock, to re-lock after a manual unlocking.
void Threading::ScopedLock::Acquire()
{
	if( m_IsLocked || !m_lock ) return;
	m_lock->Acquire();
	m_IsLocked = true;
}

Threading::ScopedLock::ScopedLock( const Mutex& locker, bool isTryLock )
{
	m_lock = const_cast<Mutex*>(&locker);
	if( !m_lock ) return;
	m_IsLocked = isTryLock ? m_lock->TryAcquire() : false;
}
