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
#include "ThreadingInternal.h"

namespace Threading
{
	static long					_attr_refcount = 0;
	static pthread_mutexattr_t	_attr_recursive;
}

// --------------------------------------------------------------------------------------
//  MutexLock Implementations
// --------------------------------------------------------------------------------------

Threading::MutexLock::MutexLock()
{
	pthread_mutex_init( &m_mutex, NULL );
}

void Threading::MutexLock::Detach()
{
	if( EBUSY != pthread_mutex_destroy(&m_mutex) ) return;

	if( IsRecursive() )
	{
		// Sanity check: Recursive locks could be held by our own thread, which would
		// be considered an assertion failure, but can also be handled gracefully.
		// (note: if the mutex is locked recursively more than twice then this assert won't
		//  detect it)

		Unlock(); Unlock();		// in case of double recursion.
		int result = pthread_mutex_destroy( &m_mutex );
		if( pxAssertDev( result != EBUSY, "Detachment of a recursively-locked mutex (self-locked!)." ) ) return;
	}

	if( Wait(def_deadlock_timeout) )
		pthread_mutex_destroy( &m_mutex );
	else
		Console.Error( "(Thread Log) Mutex cleanup failed due to possible deadlock.");
}

Threading::MutexLock::~MutexLock() throw()
{
	try {
		MutexLock::Detach();
	} DESTRUCTOR_CATCHALL;
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
	err = pthread_mutex_init( &m_mutex, &_attr_recursive );
}

Threading::MutexLockRecursive::~MutexLockRecursive() throw()
{
	if( _InterlockedDecrement( &_attr_refcount ) == 0 )
		pthread_mutexattr_destroy( &_attr_recursive );
}

// This is a bit of a hackish function, which is technically unsafe, but can be useful for allowing
// the application to survive unexpected or inconvenient failures, where a mutex is deadlocked by
// a rogue thread.  This function allows us to Recreate the mutex and let the deadlocked one ponder
// the deeper meanings of the universe for eternity.
void Threading::MutexLock::Recreate()
{
	Detach();
	pthread_mutex_init( &m_mutex, NULL );
}

// Returns:
//   true if the mutex had to be recreated due to lock contention, or false if the mutex is safely
//   unlocked.
bool Threading::MutexLock::RecreateIfLocked()
{
	if( !Wait(def_deadlock_timeout) )
	{
		Recreate();
		return true;
	}
	return false;
}


void Threading::MutexLock::LockRaw()
{
	pthread_mutex_lock( &m_mutex );
}

bool Threading::MutexLock::LockRaw( const wxTimeSpan& timeout )
{
	wxDateTime megafail( wxDateTime::UNow() + timeout );
	const timespec fail = { megafail.GetTicks(), megafail.GetMillisecond() * 1000000 };
	return pthread_mutex_timedlock( &m_mutex, &fail ) == 0;
}

void Threading::MutexLock::Unlock()
{
	pthread_mutex_unlock( &m_mutex );
}

bool Threading::MutexLock::TryLock()
{
	return EBUSY != pthread_mutex_trylock( &m_mutex );
}

// This is a wxApp-safe rendition of LockRaw, which makes sure to execute pending app events
// and messages *if* the lock is performed from the main GUI thread.
//
// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
void Threading::MutexLock::Lock()
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		LockRaw();
	}
	else if( _WaitGui_RecursionGuard( "Mutex::Lock" ) )
	{
		if( !LockRaw(def_deadlock_timeout) )
			throw Exception::ThreadTimedOut();
	}
	else
	{
		while( !LockRaw(def_yieldgui_interval) )
			wxTheApp->Yield( true );
	}
#else
	LockRaw();
#endif
}

// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
bool Threading::MutexLock::Lock( const wxTimeSpan& timeout )
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		return LockRaw(timeout);
	}
	else if( _WaitGui_RecursionGuard( "Mutex::Lock(timeout)" ) )
	{
		if( timeout > def_deadlock_timeout )
		{
			if( LockRaw(def_deadlock_timeout) ) return true;
			throw Exception::ThreadTimedOut();
		}
		return LockRaw( timeout );
	}
	else
	{
		wxTimeSpan countdown( (timeout) );

		do {
			if( LockRaw( def_yieldgui_interval ) ) break;
			wxTheApp->Yield(true);
			countdown -= def_yieldgui_interval;
		} while( countdown.GetMilliseconds() > 0 );

		return countdown.GetMilliseconds() > 0;
	}

	// Looks like a potential deadlock; throw an exception!
	throw Exception::ThreadTimedOut();

#else
	return LockRaw();
#endif
}

// Performs a wait on a locked mutex, or returns instantly if the mutex is unlocked.
// Typically this action is used to determine if a thread is currently performing some
// specific task, and to block until the task is finished (PersistentThread uses it to
// determine if the thread is running or completed, for example).
//
// Implemented internally as a simple Lock/Unlock pair.
//
// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
void Threading::MutexLock::Wait()
{
	Lock();
	Unlock();
}

// Performs a wait on a locked mutex, or returns instantly if the mutex is unlocked.
// (Implemented internally as a simple Lock/Unlock pair.)
//
// Returns:
//   true if the mutex was freed and is in an unlocked state; or false if the wait timed out
//   and the mutex is still locked by another thread.
//
// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
bool Threading::MutexLock::Wait( const wxTimeSpan& timeout )
{
	if( Lock(timeout) )
	{
		Unlock();
		return true;
	}
	return false;
}

