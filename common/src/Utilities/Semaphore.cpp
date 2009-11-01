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

void Threading::Semaphore::WaitWithoutYield()
{
	pxAssertMsg( !wxThread::IsMain(), "Unyielding semaphore wait issued from the main/gui thread.  Please use Wait() instead." );
	sem_wait( &m_sema );
}

bool Threading::Semaphore::WaitWithoutYield( const wxTimeSpan& timeout )
{
	wxDateTime megafail( wxDateTime::UNow() + timeout );
	const timespec fail = { megafail.GetTicks(), megafail.GetMillisecond() * 1000000 };
	return sem_timedwait( &m_sema, &fail ) == 0;
}


// This is a wxApp-safe implementation of Wait, which makes sure and executes the App's
// pending messages *if* the Wait is performed on the Main/GUI thread.  This ensures that
// user input continues to be handled and that windoes continue to repaint.  If the Wait is
// called from another thread, no message pumping is performed.
//
// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
void Threading::Semaphore::Wait()
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		sem_wait( &m_sema );
	}
	else if( _WaitGui_RecursionGuard( "Semaphore::Wait" ) )
	{
		ScopedBusyCursor hourglass( Cursor_ReallyBusy );
		if( !WaitWithoutYield(def_yieldgui_interval) )	// default is 4 seconds
			throw Exception::ThreadTimedOut();
	}
	else
	{
		ScopedBusyCursor hourglass( Cursor_KindaBusy );
		while( !WaitWithoutYield( def_yieldgui_interval ) )
			wxTheApp->Yield( true );
	}
#else
	sem_wait( &m_sema );
#endif
}

// This is a wxApp-safe implementation of WaitWithoutYield, which makes sure and executes the App's
// pending messages *if* the Wait is performed on the Main/GUI thread.  This ensures that
// user input continues to be handled and that windows continue to repaint.  If the Wait is
// called from another thread, no message pumping is performed.
//
// Returns:
//   false if the wait timed out before the semaphore was signaled, or true if the signal was
//   reached prior to timeout.
//
// Exceptions:
//   ThreadTimedOut - See description of ThreadTimedOut for details
//
bool Threading::Semaphore::Wait( const wxTimeSpan& timeout )
{
#if wxUSE_GUI
	if( !wxThread::IsMain() || (wxTheApp == NULL) )
	{
		return WaitWithoutYield( timeout );
	}
	else if( _WaitGui_RecursionGuard( "Semaphore::Wait(timeout)" ) )
	{
		ScopedBusyCursor hourglass( Cursor_ReallyBusy );
		if( timeout > def_deadlock_timeout )
		{
			if( WaitWithoutYield(def_deadlock_timeout) ) return true;
			throw Exception::ThreadTimedOut();
		}
		return WaitWithoutYield( timeout );
	}
	else
	{
		ScopedBusyCursor hourglass( Cursor_KindaBusy );
		wxTimeSpan countdown( (timeout) );

		do {
			if( WaitWithoutYield( def_yieldgui_interval ) ) break;
			wxTheApp->Yield(true);
			countdown -= def_yieldgui_interval;
		} while( countdown.GetMilliseconds() > 0 );

		return countdown.GetMilliseconds() > 0;
	}
#else
	return WaitWithoutYield( timeout );
#endif
}

// Performs an uncancellable wait on a semaphore; restoring the thread's previous cancel state
// after the wait has completed.  Useful for situations where the semaphore itself is stored on
// the stack and passed to another thread via GUI message or such, avoiding complications where
// the thread might be canceled and the stack value becomes invalid.
//
// Performance note: this function has quite a bit more overhead compared to Semaphore::WaitWithoutYield(), so
// consider manually specifying the thread as uncancellable and using WaitWithoutYield() instead if you need
// to do a lot of no-cancel waits in a tight loop worker thread, for example.
void Threading::Semaphore::WaitNoCancel()
{
	int oldstate;
	pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &oldstate );
	WaitWithoutYield();
	pthread_setcancelstate( oldstate, NULL );
}

void Threading::Semaphore::WaitNoCancel( const wxTimeSpan& timeout )
{
	int oldstate;
	pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &oldstate );
	WaitWithoutYield( timeout );
	pthread_setcancelstate( oldstate, NULL );
}

int Threading::Semaphore::Count()
{
	int retval;
	sem_getvalue( &m_sema, &retval );
	return retval;
}
