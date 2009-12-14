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

#include "System.h"
#include "SysThreads.h"

// --------------------------------------------------------------------------------------
//  SysThreadBase *External Thread* Implementations
//    (Called form outside the context of this thread)
// --------------------------------------------------------------------------------------

SysThreadBase::SysThreadBase() :
	m_ExecMode( ExecMode_NoThreadYet )
,	m_ExecModeMutex()
{
}

SysThreadBase::~SysThreadBase() throw()
{
}

void SysThreadBase::Start()
{
	_parent::Start();

	Sleep( 1 );

	pxAssertDev( (m_ExecMode == ExecMode_Closing) || (m_ExecMode == ExecMode_Closed),
		"Unexpected thread status during SysThread startup."
	);

	m_sem_event.Post();
}


void SysThreadBase::OnStart()
{
	if( !pxAssertDev( m_ExecMode == ExecMode_NoThreadYet, "SysSustainableThread:Start(): Invalid execution mode" ) ) return;

	m_ResumeEvent.Reset();
	FrankenMutex( m_ExecModeMutex );
	FrankenMutex( m_RunningLock );

	_parent::OnStart();
}

// Suspends emulation and closes the emulation state (including plugins) at the next PS2 vsync,
// and returns control to the calling thread; or does nothing if the core is already suspended.
//
// Parameters:
//   isNonblocking - if set to true then the function will not block for emulation suspension.
//      Defaults to false if parameter is not specified.  Performing non-blocking suspension
//      is mostly useful for starting certain non-Emu related gui activities (improves gui
//      responsiveness).
//
// Returns:
//   The previous suspension state; true if the thread was running or false if it was
//   suspended.
//
// Exceptions:
//   CancelEvent  - thrown if the thread is already in a Paused or Closing state.  Because
//      actions that pause emulation typically rely on plugins remaining loaded/active,
//      Suspension must cansel itself forcefully or risk crashing whatever other action is
//      in progress.
//
bool SysThreadBase::Suspend( bool isBlocking )
{
	if( IsSelf() || !IsRunning() ) return false;

	// shortcut ExecMode check to avoid deadlocking on redundant calls to Suspend issued
	// from Resume or OnResumeReady code.
	if( m_ExecMode == ExecMode_Closed ) return false;

	bool retval = false;

	{
		ScopedLock locker( m_ExecModeMutex );

		switch( m_ExecMode )
		{
			// Check again -- status could have changed since above.
			case ExecMode_Closed: return false;

			case ExecMode_Pausing:
			case ExecMode_Paused:
				throw Exception::CancelEvent( "Another thread is pausing the VM state." );
	
			case ExecMode_Opened:
				m_ExecMode = ExecMode_Closing;
				retval = true;
			break;
		}

		pxAssumeDev( m_ExecMode == ExecMode_Closing, "ExecMode should be nothing other than Closing..." );
		m_sem_event.Post();
	}

	if( isBlocking )
	{
		if( !m_RunningLock.Wait( wxTimeSpan( 0,0,3,0 ) ) )
		{
			// [TODO] : Implement proper deadlock handler here that lets the user continue
			// to wait, or issue a cancel to the thread.

			throw Exception::ThreadTimedOut( *this,
				wxsFormat(L"Possible deadlock while suspending thread '%s'", m_name.c_str()),
				wxsFormat(L"'%s' thread is not responding to suspend requests.  It may be deadlocked or just running *really* slow.", m_name.c_str())
			);
		}
	}
	return retval;
}

// Returns:
//   The previous suspension state; true if the thread was running or false if it was
//   closed, not running, or paused.
//
bool SysThreadBase::Pause()
{
	if( IsSelf() || !IsRunning() ) return false;

	// shortcut ExecMode check to avoid deadlocking on redundant calls to Suspend issued
	// from Resume or OnResumeReady code.
	if( (m_ExecMode == ExecMode_Closed) || (m_ExecMode == ExecMode_Paused) ) return false;

	bool retval = false;

	{
		ScopedLock locker( m_ExecModeMutex );

		// Check again -- status could have changed since above.
		if( (m_ExecMode == ExecMode_Closed) || (m_ExecMode == ExecMode_Paused) ) return false;

		if( m_ExecMode == ExecMode_Opened )
		{
			m_ExecMode = ExecMode_Pausing;
			retval = true;
		}

		pxAssumeDev( m_ExecMode == ExecMode_Pausing, "ExecMode should be nothing other than Pausing..." );

		m_sem_event.Post();
	}

	m_RunningLock.Wait();

	return retval;
}

// Resumes the core execution state, or does nothing is the core is already running.  If
// settings were changed, resets will be performed as needed and emulation state resumed from
// memory savestates.
//
// Note that this is considered a non-blocking action.  Most times the state is safely resumed
// on return, but in the case of re-entrant or nested message handling the function may return
// before the thread has resumed.  If you need explicit behavior tied to the completion of the
// Resume, you'll need to bind callbacks to either OnResumeReady or OnResumeInThread.
//
// Exceptions:
//   PluginInitError     - thrown if a plugin fails init (init is performed on the current thread
//                         on the first time the thread is resumed from it's initial idle state)
//   ThreadCreationError - Insufficient system resources to create thread.
//
void SysThreadBase::Resume()
{
	if( IsSelf() ) return;
	if( m_ExecMode == ExecMode_Opened ) return;

	ScopedNonblockingLock resprotect( m_ResumeProtection );
	if( resprotect.Failed() ) return;

	ScopedLock locker( m_ExecModeMutex );

	// Implementation Note:
	// The entire state coming out of a Wait is indeterminate because of user input
	// and pending messages being handled.  So after each call we do some seemingly redundant
	// sanity checks against m_ExecMode/m_Running status, and if something doesn't feel
	// right, we should abort; the user may have canceled the action before it even finished.

	switch( m_ExecMode )
	{
		case ExecMode_Opened: return;

		case ExecMode_NoThreadYet:
		{
			Start();
			if( !m_running || (m_ExecMode == ExecMode_NoThreadYet) )
				throw Exception::ThreadCreationError();
			if( m_ExecMode == ExecMode_Opened ) return;
		}
		// fall through...

		case ExecMode_Closing:
		case ExecMode_Pausing:
			// we need to make sure and wait for the emuThread to enter a fully suspended
			// state before continuing...

			m_RunningLock.Wait();
			if( !m_running ) return;
			if( (m_ExecMode != ExecMode_Closed) && (m_ExecMode != ExecMode_Paused) ) return;
			if( g_plugins == NULL ) return;
		break;
	}

	pxAssertDev( (m_ExecMode == ExecMode_Closed) || (m_ExecMode == ExecMode_Paused),
		"SysThreadBase is not in a closed/paused state?  wtf!" );

	OnResumeReady();
	m_ExecMode = ExecMode_Opened;
	m_ResumeEvent.Post();
}


// --------------------------------------------------------------------------------------
//  SysThreadBase *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------

void SysThreadBase::OnStartInThread()
{
	m_RunningLock.Acquire();
	_parent::OnStartInThread();
	m_ExecMode = ExecMode_Closing;
}

void SysThreadBase::OnCleanupInThread()
{
	m_ExecMode = ExecMode_NoThreadYet;
	_parent::OnCleanupInThread();
	m_RunningLock.Release();
}

void SysThreadBase::OnSuspendInThread() {}
void SysThreadBase::OnResumeInThread( bool isSuspended ) {}

void SysThreadBase::StateCheckInThread()
{
	switch( m_ExecMode )
	{

	#ifdef PCSX2_DEVBUILD		// optimize out handlers for these cases in release builds.
		case ExecMode_NoThreadYet:
			// threads should never have this state set while the thread is in any way
			// active or alive. (for obvious reasons!!)
			pxFailDev( "Invalid execution state detected." );
		break;
	#endif

		case ExecMode_Opened:
			// Yup, need this a second time.  Variable state could have changed while we
			// were trying to acquire the lock above.
			TestCancel();
		break;

		// -------------------------------------
		case ExecMode_Pausing:
		{
			OnPauseInThread();
			m_ExecMode = ExecMode_Paused;
			m_RunningLock.Release();
		}
		// fallthrough...

		case ExecMode_Paused:
			while( m_ExecMode == ExecMode_Paused )
				m_ResumeEvent.WaitWithoutYield();

			m_RunningLock.Acquire();
			OnResumeInThread( false );
		break;

		// -------------------------------------
		case ExecMode_Closing:
		{
			OnSuspendInThread();
			m_ExecMode = ExecMode_Closed;
			m_RunningLock.Release();
		}
		// fallthrough...

		case ExecMode_Closed:
			while( m_ExecMode == ExecMode_Closed )
				m_ResumeEvent.WaitWithoutYield();

			m_RunningLock.Acquire();
			OnResumeInThread( true );
		break;

		jNO_DEFAULT;
	}
}
