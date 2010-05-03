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
#include "Common.h"

#include "Counters.h"
#include "GS.h"
#include "Elfheader.h"
#include "Patch.h"
#include "PageFaultSource.h"
#include "SysThreads.h"

#include "Utilities/TlsVariable.inl"

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>
#endif

#include <xmmintrin.h>

static DeclareTls(SysCoreThread*) tls_coreThread( NULL );

// --------------------------------------------------------------------------------------
//  SysCoreThread *External Thread* Implementations
//    (Called from outside the context of this thread)
// --------------------------------------------------------------------------------------

SysCoreThread::SysCoreThread()
{
	m_name					= L"EE Core";
	m_resetRecompilers		= true;
	m_resetProfilers		= true;
	m_resetVsyncTimers		= true;
	m_resetVirtualMachine	= true;
	m_hasValidState			= false;
}

SysCoreThread::~SysCoreThread() throw()
{
	SysCoreThread::Cancel();
}

void SysCoreThread::Cancel( bool isBlocking )
{
	m_CoreCancelDamnit = true;
	_parent::Cancel();
}

bool SysCoreThread::Cancel( const wxTimeSpan& span )
{
	m_CoreCancelDamnit = true;
	if( _parent::Cancel( span ) )
		return true;

	return false;
}

void SysCoreThread::OnStart()
{
	m_CoreCancelDamnit = false;
	_parent::OnStart();
}

void SysCoreThread::Start()
{
	if( !GetCorePlugins().AreLoaded() ) return;
	GetCorePlugins().Init();
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
		m_hasValidState = false;

	if( !m_hasValidState )
		m_resetRecompilers = true;
}

// Tells the thread to recover from the in-memory state copy when it resumes.  (thread must be
// resumed manually).
void SysCoreThread::RecoverState()
{
	pxAssumeDev( IsPaused(), "Unsafe use of RecoverState function; Corethread is not paused/closed." );
	m_resetRecompilers		= true;
	m_hasValidState			= false;
}

void SysCoreThread::Reset()
{
	Suspend();
	m_resetVirtualMachine	= true;
	m_hasValidState			= false;
}

// This function *will* reset the emulator in order to allow the specified elf file to
// take effect.  This is because it really doesn't make sense to change the elf file outside
// the context of a reset/restart.
void SysCoreThread::SetElfOverride( const wxString& elf )
{
	pxAssertDev( !m_hasValidState, "Thread synchronization error while assigning ELF override." );
	m_elf_override = elf;
}


// Applies a full suite of new settings, which will automatically facilitate the necessary
// resets of the core and components (including plugins, if needed).  The scope of resetting
// is determined by comparing the current settings against the new settings, so that only
// real differences are applied.
void SysCoreThread::ApplySettings( const Pcsx2Config& src )
{
	if( src == EmuConfig ) return;

	if( !pxAssertDev( IsPaused(), "CoreThread is not paused; settings cannot be applied." ) ) return;
	
	m_resetRecompilers		= ( src.Cpu != EmuConfig.Cpu ) || ( src.Recompiler != EmuConfig.Recompiler ) ||
							  ( src.Gamefixes != EmuConfig.Gamefixes ) || ( src.Speedhacks != EmuConfig.Speedhacks );
	m_resetProfilers		= ( src.Profiler != EmuConfig.Profiler );
	m_resetVsyncTimers		= ( src.GS != EmuConfig.GS );

	const_cast<Pcsx2Config&>(EmuConfig) = src;
}

// --------------------------------------------------------------------------------------
//  SysCoreThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------
SysCoreThread& SysCoreThread::Get()
{
	pxAssertMsg( tls_coreThread != NULL, L"This function must be called from the context of a running SysCoreThread." );
	return *tls_coreThread;
}

bool SysCoreThread::HasPendingStateChangeRequest() const
{
	return m_CoreCancelDamnit || GetMTGS().HasPendingException() || _parent::HasPendingStateChangeRequest();
}

struct ScopedBool_ClearOnError
{
	bool&	m_target;
	bool	m_success;

	ScopedBool_ClearOnError( bool& target ) :
		m_target( target ), m_success( false )
	{
		m_target = true;
	}

	virtual ~ScopedBool_ClearOnError()
	{
		m_target = m_success;
	}

	void Success() { m_success = true; }
};

void SysCoreThread::_reset_stuff_as_needed()
{
	if( m_resetVirtualMachine || m_resetRecompilers || m_resetProfilers )
	{
		SysClearExecutionCache();
		memBindConditionalHandlers();
		m_resetRecompilers		= false;
		m_resetProfilers		= false;
	}

	if( m_resetVirtualMachine )
	{
		DoCpuReset();
		m_resetVirtualMachine	= false;
		m_resetRecompilers		= true;
	}

	if( m_resetVsyncTimers )
	{
		UpdateVSyncRate();
		frameLimitReset();
		m_resetVsyncTimers = false;
	}

	SetCPUState( EmuConfig.Cpu.sseMXCSR, EmuConfig.Cpu.sseVUMXCSR );
}

void SysCoreThread::DoCpuReset()
{
	AffinityAssert_AllowFromSelf( pxDiagSpot );
	cpuReset();
}

void SysCoreThread::CpuInitializeMess()
{
	if( m_hasValidState ) return;

	_reset_stuff_as_needed();

	ScopedBool_ClearOnError sbcoe( m_hasValidState );

	sbcoe.Success();
}

void SysCoreThread::PostVsyncToUI()
{
}

// This is called from the PS2 VM at the start of every vsync (either 59.94 or 50 hz by PS2
// clock scale, which does not correlate to the actual host machine vsync).
//
// Default tasks: Updates PADs and applies vsync patches.  Derived classes can override this
// to change either PAD and/or Patching behaviors.
//
// [TODO]: Should probably also handle profiling and debugging updates, once those are
// re-implemented.
//
void SysCoreThread::VsyncInThread()
{
	PostVsyncToUI();
	if (EmuConfig.EnablePatches) ApplyPatch();
}

void SysCoreThread::StateCheckInThread()
{
	GetMTGS().RethrowException();
	_parent::StateCheckInThread();

	if( !m_hasValidState )
		throw Exception::RuntimeError( "Invalid emulation state detected; Virtual machine threads have been cancelled." );

	_reset_stuff_as_needed();		// kinda redundant but could catch unexpected threaded state changes...
}

// Allows an override point and solves an SEH "exception-type boundary" problem (can't mix
// SEH and C++ exceptions in the same function).
void SysCoreThread::DoCpuExecute()
{
	Cpu->Execute();
}

void SysCoreThread::ExecuteTaskInThread()
{
	Threading::EnableHiresScheduler();
	tls_coreThread = this;
	m_sem_event.WaitWithoutYield();

	m_mxcsr_saved.bitmask = _mm_getcsr();

	PCSX2_PAGEFAULT_PROTECT {
		do {
			StateCheckInThread();
			DoCpuExecute();
		} while( true );
	} PCSX2_PAGEFAULT_EXCEPT;
}

void SysCoreThread::OnSuspendInThread()
{
	GetCorePlugins().Close();
}

void SysCoreThread::OnResumeInThread( bool isSuspended )
{
	GetCorePlugins().Open();

	CpuInitializeMess();
}


// Invoked by the pthread_exit or pthread_cancel.
void SysCoreThread::OnCleanupInThread()
{
	m_hasValidState = false;

	_mm_setcsr( m_mxcsr_saved.bitmask );

	Threading::DisableHiresScheduler();

	GetCorePlugins().Close();

	tls_coreThread = NULL;
	_parent::OnCleanupInThread();
}

