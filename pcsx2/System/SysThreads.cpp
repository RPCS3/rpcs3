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
#include "Common.h"
#include "System.h"
#include "SysThreads.h"
#include "SaveState.h"
#include "Elfheader.h"
#include "Plugins.h"

#include "R5900.h"
#include "R3000A.h"
#include "VUmicro.h"

static __threadlocal SysCoreThread* tls_coreThread = NULL;

// --------------------------------------------------------------------------------------
//  SysSuspendableThread *External Thread* Implementations
//    (Called form outside the context of this thread)
// --------------------------------------------------------------------------------------

SysSuspendableThread::SysSuspendableThread() :
	m_ExecMode( ExecMode_NoThreadYet )
,	m_lock_ExecMode()
,	m_ResumeEvent()
,	m_SuspendEvent()
,	m_ResumeProtection( false )
{
}

SysSuspendableThread::~SysSuspendableThread() throw()
{
}

void SysSuspendableThread::Start()
{
	_parent::Start();
	m_ExecMode = ExecMode_Suspending;
	m_sem_event.Post();
}


void SysSuspendableThread::OnStart()
{
	if( !pxAssertDev( m_ExecMode == ExecMode_NoThreadYet, "SysSustainableThread:Start(): Invalid execution mode" ) ) return;

	m_ResumeEvent.Reset();
	m_SuspendEvent.Reset();

	_parent::OnStart();
}

// Pauses the emulation state at the next PS2 vsync, and returns control to the calling
// thread; or does nothing if the core is already suspended.  Calling this thread from the
// Core thread will result in deadlock.
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
//   CancelEvent  - thrown if the thread is already in a Paused state.  Because actions that
//      pause emulation typically rely on plugins remaining loaded/active, Suspension must
//      cansel itself forcefully or risk crashing whatever other action is in progress.
//
bool SysSuspendableThread::Suspend( bool isBlocking )
{
	if( IsSelf() || !IsRunning() ) return false;

	bool retval = false;

	{
		ScopedLock locker( m_lock_ExecMode );

		if( m_ExecMode == ExecMode_Suspended )
			return false;

		if( m_ExecMode == ExecMode_Pausing || m_ExecMode == ExecMode_Paused )
			throw Exception::CancelEvent( "Another thread is pausing the VM state." );

		if( m_ExecMode == ExecMode_Running )
		{
			m_ExecMode = ExecMode_Suspending;
			retval = true;
		}

		pxAssertDev( m_ExecMode == ExecMode_Suspending, "ExecMode should be nothing other than Suspending..." );
	}
	m_SuspendEvent.Reset();
	m_sem_event.Post();
	if( isBlocking ) m_SuspendEvent.WaitGui();
	return retval;
}

bool SysSuspendableThread::Pause()
{
	if( IsSelf() || !IsRunning() ) return false;

	bool retval = false;

	{
		ScopedLock locker( m_lock_ExecMode );

		if( (m_ExecMode == ExecMode_Suspended) || (m_ExecMode == ExecMode_Paused) ) return false;

		if( m_ExecMode == ExecMode_Running )
		{
			m_ExecMode = ExecMode_Pausing;
			retval = true;
		}

		pxAssertDev( m_ExecMode == ExecMode_Pausing, "ExecMode should be nothing other than Pausing..." );
	}
	m_SuspendEvent.Reset();
	m_sem_event.Post();
	m_SuspendEvent.WaitGui();

	return retval;
}

// Resumes the core execution state, or does nothing is the core is already running.  If
// settings were changed, resets will be performed as needed and emulation state resumed from
// memory savestates.
//
// Exceptions (can occur on first call only):
//   PluginInitError     - thrown if a plugin fails init (init is performed on the current thread
//                         on the first time the thread is resumed from it's initial idle state)
//   ThreadCreationError - Insufficient system resources to create thread.
//
void SysSuspendableThread::Resume()
{
	if( IsSelf() ) return;
	
	{
		ScopedLock locker( m_lock_ExecMode );

		switch( m_ExecMode )
		{
			case ExecMode_Running: return;

			case ExecMode_NoThreadYet:
				Start();
				m_ExecMode = ExecMode_Suspending;
			// fall through...

			case ExecMode_Suspending:
			case ExecMode_Pausing:
				// we need to make sure and wait for the emuThread to enter a fully suspended
				// state before continuing...

				locker.Unlock();		// no deadlocks please, thanks. :)
				m_SuspendEvent.WaitGui();
			break;
		}
	}

	pxAssertDev( (m_ExecMode == ExecMode_Suspended) || (m_ExecMode == ExecMode_Paused),
		"SysSuspendableThread is not in a suspended/paused state?  wtf!" );

	m_ResumeProtection = true;
	OnResumeReady();
	m_ResumeProtection = false;
	m_ExecMode = ExecMode_Running;
	m_ResumeEvent.Post();
}


// --------------------------------------------------------------------------------------
//  SysSuspendableThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------

void SysSuspendableThread::OnCleanupInThread()
{
	ScopedLock locker( m_lock_ExecMode );
	m_ExecMode = ExecMode_NoThreadYet;
	_parent::OnCleanupInThread();
}

void SysSuspendableThread::StateCheck( bool isCancelable )
{
	// Shortcut for the common case, to avoid unnecessary Mutex locks:
	if( m_ExecMode == ExecMode_Running )
	{
		if( isCancelable ) TestCancel();
		return;
	}

	// Oh, seems we need a full lock, because something special is happening!
	ScopedLock locker( m_lock_ExecMode );

	switch( m_ExecMode )
	{

	#ifdef PCSX2_DEVBUILD		// optimize out handlers for these cases in release builds.
		case ExecMode_NoThreadYet:
			// threads should never have this state set while the thread is in any way
			// active or alive. (for obvious reasons!!)
			pxFailDev( "Invalid execution state detected." );
		break;
	#endif

		case ExecMode_Running:
			// Yup, need this a second time.  Variable state could have changed while we
			// were trying to acquire the lock above.
			if( isCancelable )
				TestCancel();
		break;

		// -------------------------------------
		case ExecMode_Pausing:
		{
			OnPauseInThread();
			m_ExecMode = ExecMode_Paused;
			m_SuspendEvent.Post();
		}
		// fallthrough...

		case ExecMode_Paused:
			m_lock_ExecMode.Unlock();
			while( m_ExecMode == ExecMode_Paused )
				m_ResumeEvent.WaitGui();

			OnResumeInThread( false );
		break;

		// -------------------------------------
		case ExecMode_Suspending:
		{
			OnSuspendInThread();
			m_ExecMode = ExecMode_Suspended;
			m_SuspendEvent.Post();
		} 
		// fallthrough...
		
		case ExecMode_Suspended:
			m_lock_ExecMode.Unlock();
			while( m_ExecMode == ExecMode_Suspended )
				m_ResumeEvent.WaitGui();

			OnResumeInThread( true );
		break;
		
		jNO_DEFAULT;
	}
}

// --------------------------------------------------------------------------------------
//  EECoreThread *External Thread* Implementations
//    (Called form outside the context of this thread)
// --------------------------------------------------------------------------------------

SysCoreThread::SysCoreThread() :
	m_resetRecompilers( true )
,	m_resetProfilers( true )
,	m_resetVirtualMachine( true )
,	m_hasValidState( false )
{
	m_name = L"EE Core";
}

SysCoreThread::~SysCoreThread() throw()
{
	_parent::Cancel();
}

void SysCoreThread::Start()
{
	if( g_plugins == NULL ) return;
	g_plugins->Init();
	_parent::Start();
}

// Resumes the core execution state, or does nothing is the core is already running.  If
// settings were changed, resets will be performed as needed and emulation state resumed from
// memory savestates.
//
// Exceptions (can occur on first call only):
//   PluginInitError     - thrown if a plugin fails init (init is performed on the current thread
//                         on the first time the thread is resumed from it's initial idle state)
//   ThreadCreationError - Insufficient system resources to create thread.
//
void SysCoreThread::OnResumeReady()
{
	if( m_resetVirtualMachine )
	{
		cpuReset();
		m_resetVirtualMachine	= false;
		m_hasValidState			= false;
	}

	if( m_resetRecompilers || m_resetProfilers || !m_hasValidState )
	{
		SysClearExecutionCache();
		memBindConditionalHandlers();
		m_resetRecompilers = false;
		m_resetProfilers = false;
	}
}

void SysCoreThread::Reset()
{
	Suspend();
	m_resetVirtualMachine = true;
}

// Applies a full suite of new settings, which will automatically facilitate the necessary
// resets of the core and components (including plugins, if needed).  The scope of resetting
// is determined by comparing the current settings against the new settings.
void SysCoreThread::ApplySettings( const Pcsx2Config& src )
{
	if( src == EmuConfig ) return;

	// Suspend only if ResumeProtection is false:
	const bool resumeWhenDone = !m_ResumeProtection && Suspend();

	m_resetRecompilers		= ( src.Cpu != EmuConfig.Cpu ) || ( src.Gamefixes != EmuConfig.Gamefixes ) || ( src.Speedhacks != EmuConfig.Speedhacks );
	m_resetProfilers		= (src.Profiler != EmuConfig.Profiler );
 
	const_cast<Pcsx2Config&>(EmuConfig) = src;

	if( resumeWhenDone ) Resume();
}

// --------------------------------------------------------------------------------------
//  EECoreThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------
SysCoreThread& SysCoreThread::Get()
{
	pxAssertMsg( tls_coreThread != NULL, L"This function must be called from the context of a running SysCoreThread." );
	return *tls_coreThread;
}

void SysCoreThread::CpuInitializeMess()
{
	if( m_hasValidState ) return;
	
	wxString elf_file;
	if( EmuConfig.SkipBiosSplash )
	{
		// Fetch the ELF filename and CD type from the CDVD provider.
		wxString ename;
		int result = GetPS2ElfName( ename );
		switch( result )
		{
			case 0:
				throw Exception::RuntimeError( wxLt("Fast Boot failed: CDVD image is not a PS1 or PS2 game.") );

			case 1:
				throw Exception::RuntimeError( wxLt("Fast Boot failed: PCSX2 does not support emulation of PS1 games.") );

			case 2:
				// PS2 game.  Valid!
				elf_file = ename;
			break;

			jNO_DEFAULT
		}
	}

	if( !elf_file.IsEmpty() )
	{
		// Skip Bios Hack -- Runs the PS2 BIOS stub, and then manually loads the ELF
		// executable data, and injects the cpuRegs.pc with the address of the
		// execution start point.
		//
		// This hack is necessary for non-CD ELF files, and is optional for game CDs
		// (though not recommended for games because of rare ill side effects).

		cpuExecuteBios();
		loadElfFile( elf_file );
	}
	m_hasValidState = true;
}

// special macro which disables inlining on functions that require their own function stackframe.
// This is due to how Win32 handles structured exception handling.  Linux uses signals instead
// of SEH, and so these functions can be inlined.
#ifdef _WIN32
#	define __unique_stackframe __noinline
#else
#	define __unique_stackframe
#endif

// On Win32 this function invokes SEH, which requires it be in a function all by itself
// with inlining disabled.
__unique_stackframe
void SysCoreThread::CpuExecute()
{
	PCSX2_MEM_PROTECT_BEGIN();
	Cpu->Execute();
	PCSX2_MEM_PROTECT_END();
}

void SysCoreThread::ExecuteTaskInThread()
{
	tls_coreThread = this;

	m_sem_event.Wait();
	StateCheck();
	CpuExecute();
}

void SysCoreThread::OnSuspendInThread()
{
	if( g_plugins == NULL ) return;
	g_plugins->Close();
}

void SysCoreThread::OnResumeInThread( bool isSuspended )
{
	if( isSuspended && g_plugins != NULL )
	{
		g_plugins->Open();
		CpuInitializeMess();
	}
}


// Invoked by the pthread_exit or pthread_cancel
void SysCoreThread::OnCleanupInThread()
{
	//if( g_plugins != NULL )
	//	g_plugins->Shutdown();
	_parent::OnCleanupInThread();
}

