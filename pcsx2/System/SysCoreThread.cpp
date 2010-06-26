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

#include "IopBios.h"

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

	m_hasActiveMachine		= false;
}

SysCoreThread::~SysCoreThread() throw()
{
	SysCoreThread::Cancel();
}

void SysCoreThread::Cancel( bool isBlocking )
{
	m_hasActiveMachine = false;
	_parent::Cancel();
}

bool SysCoreThread::Cancel( const wxTimeSpan& span )
{
	m_hasActiveMachine = false;
	if( _parent::Cancel( span ) )
		return true;

	return false;
}

void SysCoreThread::OnStart()
{
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
		m_hasActiveMachine = false;

	if( !m_hasActiveMachine )
		m_resetRecompilers = true;
}

// This function *will* reset the emulator in order to allow the specified elf file to
// take effect.  This is because it really doesn't make sense to change the elf file outside
// the context of a reset/restart.
void SysCoreThread::SetElfOverride( const wxString& elf )
{
	//pxAssertDev( !m_hasValidMachine, "Thread synchronization error while assigning ELF override." );
	m_elf_override = elf;


	Hle_SetElfPath(elf.ToUTF8());
}

void SysCoreThread::Reset()
{
	Suspend();
	m_resetVirtualMachine	= true;
	m_hasActiveMachine		= false;
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

void SysCoreThread::UploadStateCopy( const VmStateBuffer& copy )
{
	if( !pxAssertDev( IsPaused(), "CoreThread is not paused; new VM state cannot be uploaded." ) ) return;

	SysClearExecutionCache();
	memLoadingState( copy ).FreezeAll();
	m_resetVirtualMachine = false;
}

// --------------------------------------------------------------------------------------
//  SysCoreThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------
bool SysCoreThread::HasPendingStateChangeRequest() const
{
	return !m_hasActiveMachine || GetMTGS().HasPendingException() || _parent::HasPendingStateChangeRequest();
}

void SysCoreThread::_reset_stuff_as_needed()
{
	// Note that resetting recompilers along with the virtual machine is only really needed
	// because of changes to the TLB.  We don't actually support the TLB, however, so rec
	// resets aren't in fact *needed* ... yet.  But might as well, no harm.  --air

	if( m_resetVirtualMachine || m_resetRecompilers || m_resetProfilers )
	{
		SysClearExecutionCache();
		memBindConditionalHandlers();
		SetCPUState( EmuConfig.Cpu.sseMXCSR, EmuConfig.Cpu.sseVUMXCSR );

		m_resetRecompilers		= false;
		m_resetProfilers		= false;
	}

	if( m_resetVirtualMachine )
	{
		DoCpuReset();

		m_resetVirtualMachine	= false;
		m_resetVsyncTimers		= false;
	}

	if( m_resetVsyncTimers )
	{
		UpdateVSyncRate();
		frameLimitReset();

		m_resetVsyncTimers		= false;
	}
}

void SysCoreThread::DoCpuReset()
{
	AffinityAssert_AllowFromSelf( pxDiagSpot );
	cpuReset();
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
	if (EmuConfig.EnablePatches) ApplyPatch();
	if (EmuConfig.EnableCheats)  ApplyCheat();
}

void SysCoreThread::GameStartingInThread()
{
	GetMTGS().SendGameCRC(ElfCRC);

	if (EmuConfig.EnablePatches) ApplyPatch(0);
	if (EmuConfig.EnableCheats)  ApplyCheat(0);
}

bool SysCoreThread::StateCheckInThread()
{
	GetMTGS().RethrowException();
	return _parent::StateCheckInThread() && (_reset_stuff_as_needed(), true);
}

// Runs CPU cycles indefinitely, until the user or another thread requests execution to break.
// Rationale: This very short function allows an override point and solves an SEH
// "exception-type boundary" problem (can't mix SEH and C++ exceptions in the same function).
void SysCoreThread::DoCpuExecute()
{
	m_hasActiveMachine = true;
	Cpu->Execute();
}

void SysCoreThread::ExecuteTaskInThread()
{
	Threading::EnableHiresScheduler();
	m_sem_event.WaitWithoutYield();

	m_mxcsr_saved.bitmask = _mm_getcsr();

	PCSX2_PAGEFAULT_PROTECT {
		while(true) {
			StateCheckInThread();
			DoCpuExecute();
		}
	} PCSX2_PAGEFAULT_EXCEPT;
}

void SysCoreThread::OnSuspendInThread()
{
	GetCorePlugins().Close();
}

void SysCoreThread::OnResumeInThread( bool isSuspended )
{
	GetCorePlugins().Open();
}


// Invoked by the pthread_exit or pthread_cancel.
void SysCoreThread::OnCleanupInThread()
{
	m_hasActiveMachine = false;

	GetCorePlugins().Close();
	GetCorePlugins().Shutdown();

	_mm_setcsr( m_mxcsr_saved.bitmask );
	Threading::DisableHiresScheduler();
	_parent::OnCleanupInThread();
}

