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
	if( !DevAssert( m_ExecMode == ExecMode_NoThreadYet, "SysSustainableThread:Start(): Invalid execution mode" ) ) return;

	m_ResumeEvent.Reset();
	m_SuspendEvent.Reset();

	_parent::Start();
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
void SysSuspendableThread::Suspend( bool isBlocking )
{
	if( IsSelf() || !IsRunning() ) return;

	{
		ScopedLock locker( m_lock_ExecMode );

		if( m_ExecMode == ExecMode_Suspended )
			return;

		if( m_ExecMode == ExecMode_Running )
			m_ExecMode = ExecMode_Suspending;

		DevAssert( m_ExecMode == ExecMode_Suspending, "ExecMode should be nothing other than Suspended..." );
	}
	m_sem_event.Post();
	m_SuspendEvent.WaitGui();
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
				// we need to make sure and wait for the emuThread to enter a fully suspended
				// state before continuing...

				locker.Unlock();		// no deadlocks please, thanks. :)
				m_SuspendEvent.WaitGui();
			break;
		}
	}

	DevAssert( m_ExecMode == ExecMode_Suspended,
		"SysSuspendableThread is not in a suspended/idle state?  wtf!" );

	m_ExecMode = ExecMode_Running;
	m_ResumeProtection = true;
	OnResumeReady();
	m_ResumeProtection = false;
	m_ResumeEvent.Post();
}


// --------------------------------------------------------------------------------------
//  SysSuspendableThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------

void SysSuspendableThread::DoThreadCleanup()
{
	ScopedLock locker( m_lock_ExecMode );
	m_ExecMode = ExecMode_NoThreadYet;
	_parent::DoThreadCleanup();
}

void SysSuspendableThread::StateCheck( bool isCancelable )
{
	// Shortcut for the common case, to avoid unnecessary Mutex locks:
	if( m_ExecMode == ExecMode_Running )
	{
		if( isCancelable ) pthread_testcancel();
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
			DevAssert( false, "Invalid execution state detected." );
		break;
	#endif

		case ExecMode_Running:
			// Yup, need this a second time.  Variable state could have changed while we
			// were trying to acquire the lock above.
			if( isCancelable )
				pthread_testcancel();
		break;

		case ExecMode_Suspending:
		{
			OnSuspendInThread();
			m_ExecMode = ExecMode_Suspended;
			m_SuspendEvent.Post();
		} 
		// fall through...

		case ExecMode_Suspended:
			m_lock_ExecMode.Unlock();
			while( m_ExecMode == ExecMode_Suspended )
				m_ResumeEvent.WaitGui();

			OnResumeInThread();
		break;
		
		jNO_DEFAULT;
	}
}

// --------------------------------------------------------------------------------------
//  EECoreThread *External Thread* Implementations
//    (Called form outside the context of this thread)
// --------------------------------------------------------------------------------------

SysCoreThread::SysCoreThread( PluginManager& plugins ) :
	m_resetRecompilers( false )
,	m_resetProfilers( false )
,	m_shortSuspend( false )
,	m_plugins( plugins )
{
	m_name = L"EE Core";
}

SysCoreThread::~SysCoreThread() throw()
{
	_parent::Cancel();
}

void SysCoreThread::Start()
{
	m_plugins.Init();
	_parent::Start();
}

// Suspends the system without closing plugins or updating GUI status.
// Should be used for savestates or other actions which happen very quickly.
void SysCoreThread::ShortSuspend()
{
	m_shortSuspend = true;
	Suspend();
	m_shortSuspend = false;
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
	if( m_shortSuspend ) return;

	if( m_resetRecompilers || m_resetProfilers )
	{
		SysClearExecutionCache();
		m_resetRecompilers = false;
		m_resetProfilers = false;
	}
}

// Applies a full suite of new settings, which will automatically facilitate the necessary
// resets of the core and components (including plugins, if needed).  The scope of resetting
// is determined by comparing the current settings against the new settings.
void SysCoreThread::ApplySettings( const Pcsx2Config& src )
{
	if( src == EmuConfig ) return;

	const bool resumeWhenDone = !m_ResumeProtection && !IsSuspended();
	if( !m_ResumeProtection ) Suspend();

	m_resetRecompilers = ( src.Cpu != EmuConfig.Cpu ) || ( src.Gamefixes != EmuConfig.Gamefixes ) || ( src.Speedhacks != EmuConfig.Speedhacks );
	m_resetProfilers = (src.Profiler != EmuConfig.Profiler );
	const_cast<Pcsx2Config&>(EmuConfig) = src;

	if( resumeWhenDone ) Resume();
}

// --------------------------------------------------------------------------------------
//  EECoreThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------
SysCoreThread& SysCoreThread::Get()
{
	wxASSERT_MSG( tls_coreThread != NULL, L"This function must be called from the context of a running SysCoreThread." );
	return *tls_coreThread;
}

void SysCoreThread::CpuInitializeMess()
{
	cpuReset();
	SysClearExecutionCache();

	if( StateCopy_IsValid() )
	{
		// no need to boot bios or detect CDs when loading savestates.
		// [TODO] : It might be useful to detect game SLUS/CRC and compare it against
		// the savestate info, and issue a warning to the user since, chances are, they
		// don't really want to run a game with the wrong ISO loaded into the emu.
		StateCopy_ThawFromMem();
	}
	else
	{
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
	}
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

static void _cet_callback_cleanup( void* handle )
{
	((SysCoreThread*)handle)->DoThreadCleanup();
}

sptr SysCoreThread::ExecuteTask()
{
	tls_coreThread = this;

	StateCheck();
	CpuInitializeMess();
	StateCheck();
	CpuExecute();

	return 0;
}

void SysCoreThread::OnSuspendInThread()
{
	if( !m_shortSuspend )
		m_plugins.Close();
}

void SysCoreThread::OnResumeInThread()
{
	m_plugins.Open();
}


// Invoked by the pthread_exit or pthread_cancel
void SysCoreThread::DoThreadCleanup()
{
	m_plugins.Shutdown();
	_parent::DoThreadCleanup();
}

