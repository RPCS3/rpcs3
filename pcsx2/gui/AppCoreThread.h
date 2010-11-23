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

#pragma once

#include "System/SysThreads.h"
#include "pxEventThread.h"

#include "AppCommon.h"
#include "AppCorePlugins.h"
#include "SaveState.h"

#define AffinityAssert_AllowFrom_CoreThread() \
	pxAssertMsg( GetCoreThread().IsSelf(), "Thread affinity violation: Call allowed from SysCoreThread only." )

#define AffinityAssert_DisallowFrom_CoreThread() \
	pxAssertMsg( !GetCoreThread().IsSelf(), "Thread affinity violation: Call is *not* allowed from SysCoreThread." )

class IScopedCoreThread;
class BaseScopedCoreThread;

enum ScopedCoreResumeType
{
	ScopedCore_BlockingResume
,	ScopedCore_NonblockingResume
,	ScopedCore_SkipResume
};


// --------------------------------------------------------------------------------------
//  BaseSysExecEvent_ScopedCore
// --------------------------------------------------------------------------------------
class BaseSysExecEvent_ScopedCore : public SysExecEvent
{
protected:
	SynchronousActionState*		m_resume;
	Threading::Mutex*			m_mtx_resume;

public:
	virtual ~BaseSysExecEvent_ScopedCore() throw() {}

	BaseSysExecEvent_ScopedCore& SetResumeStates( SynchronousActionState* sync, Threading::Mutex* mutex )
	{
		m_resume = sync;
		m_mtx_resume = mutex;
		return *this;
	}

	BaseSysExecEvent_ScopedCore& SetResumeStates( SynchronousActionState& sync, Threading::Mutex& mutex )
	{
		m_resume = &sync;
		m_mtx_resume = &mutex;
		return *this;
	}

protected:
	BaseSysExecEvent_ScopedCore( SynchronousActionState* sync=NULL, SynchronousActionState* resume_sync=NULL, Threading::Mutex* mtx_resume=NULL )
		: SysExecEvent( sync )
	{
		m_resume		= resume_sync;
		m_mtx_resume	= mtx_resume;
	}
	
	void _post_and_wait( IScopedCoreThread& core );

	virtual void DoScopedTask() {}
};


// --------------------------------------------------------------------------------------
//  SysExecEvent_CoreThreadClose
// --------------------------------------------------------------------------------------
class SysExecEvent_CoreThreadClose : public BaseSysExecEvent_ScopedCore
{
public:
	wxString GetEventName() const { return L"CloseCoreThread"; }

	virtual ~SysExecEvent_CoreThreadClose() throw() {}
	SysExecEvent_CoreThreadClose* Clone() const { return new SysExecEvent_CoreThreadClose( *this ); }
	
	SysExecEvent_CoreThreadClose( SynchronousActionState* sync=NULL, SynchronousActionState* resume_sync=NULL, Threading::Mutex* mtx_resume=NULL )
		: BaseSysExecEvent_ScopedCore( sync, resume_sync, mtx_resume ) { }

protected:
	void InvokeEvent();
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_CoreThreadPause
// --------------------------------------------------------------------------------------
class SysExecEvent_CoreThreadPause : public BaseSysExecEvent_ScopedCore
{
public:
	wxString GetEventName() const { return L"PauseCoreThread"; }

	virtual ~SysExecEvent_CoreThreadPause() throw() {}
	SysExecEvent_CoreThreadPause* Clone() const { return new SysExecEvent_CoreThreadPause( *this ); }
	
	SysExecEvent_CoreThreadPause( SynchronousActionState* sync=NULL, SynchronousActionState* resume_sync=NULL, Threading::Mutex* mtx_resume=NULL )
		: BaseSysExecEvent_ScopedCore( sync, resume_sync, mtx_resume ) { }

protected:
	void InvokeEvent();
};

// --------------------------------------------------------------------------------------
//  AppCoreThread class
// --------------------------------------------------------------------------------------
class AppCoreThread : public SysCoreThread
{
	typedef SysCoreThread _parent;

protected:
	volatile bool m_resetCdvd;
	
public:
	AppCoreThread();
	virtual ~AppCoreThread() throw();
	
	void ResetCdvd() { m_resetCdvd = true; }

	virtual void Suspend( bool isBlocking=false );
	virtual void Resume();
	virtual void Reset();
	virtual void ResetQuick();
	virtual void Cancel( bool isBlocking=true );
	virtual bool StateCheckInThread();
	virtual void ChangeCdvdSource();

	virtual void ApplySettings( const Pcsx2Config& src );
	virtual void UploadStateCopy( const VmStateBuffer& copy );

protected:
	virtual void DoCpuExecute();

	virtual void OnResumeReady();
	virtual void OnResumeInThread( bool IsSuspended );
	virtual void OnSuspendInThread();
	virtual void OnCleanupInThread();
	virtual void VsyncInThread();
	virtual void GameStartingInThread();
	virtual void ExecuteTaskInThread();
	virtual void DoCpuReset();
};

// --------------------------------------------------------------------------------------
//  IScopedCoreThread / BaseScopedCoreThread
// --------------------------------------------------------------------------------------
class IScopedCoreThread
{
protected:
	IScopedCoreThread() {}

public:
	virtual ~IScopedCoreThread() throw() {};
	virtual void AllowResume()=0;
	virtual void DisallowResume()=0;
};

class BaseScopedCoreThread : public IScopedCoreThread
{
	DeclareNoncopyableObject( BaseScopedCoreThread );

protected:
	bool					m_allowResume;
	bool					m_alreadyStopped;
	bool					m_alreadyScoped;
	SynchronousActionState	m_sync;
	SynchronousActionState	m_sync_resume;
	Threading::Mutex		m_mtx_resume;

	BaseScopedCoreThread();

public:
	virtual ~BaseScopedCoreThread() throw()=0;
	virtual void AllowResume();
	virtual void DisallowResume();

	virtual bool PostToSysExec( BaseSysExecEvent_ScopedCore* msg );

protected:
	// Called from destructors -- do not make virtual!!
	void DoResume();
};

// --------------------------------------------------------------------------------------
//  ScopedCoreThreadClose / ScopedCoreThreadPause / ScopedCoreThreadPopup
// --------------------------------------------------------------------------------------
// This class behaves a bit differently from other scoped classes due to the "standard"
// assumption that we actually do *not* want to resume CoreThread operations when an
// exception occurs.  Because of this, the destructor of this class does *not* unroll the
// suspend operation.  Instead you must manually instruct the class to resume using a call
// to the provisioned AllowResume() method.
//
// If the class leaves scope without having been resumed, a log is written to the console.
// This can be useful for troubleshooting, and also allows the log a second line of info
// indicating the status of CoreThread execution at the time of the exception.
//
// ScopedCoreThreadPopup is intended for use where message boxes are popped up to the user.
// The old style GUI (without GSopen2) must use a full close of the CoreThread, in order to
// ensure that the GS window isn't blocking the popup, and to avoid crashes if the GS window
// is maximized or fullscreen.
//
class ScopedCoreThreadClose : public BaseScopedCoreThread
{
	typedef BaseScopedCoreThread _parent;

public:
	ScopedCoreThreadClose();
	virtual ~ScopedCoreThreadClose() throw();

	void LoadPlugins();
};

struct ScopedCoreThreadPause : public BaseScopedCoreThread
{
	typedef BaseScopedCoreThread _parent;

public:
	ScopedCoreThreadPause( BaseSysExecEvent_ScopedCore* abuse_me=NULL );
	virtual ~ScopedCoreThreadPause() throw();
};

struct ScopedCoreThreadPopup : public IScopedCoreThread
{
protected:
	ScopedPtr<BaseScopedCoreThread>		m_scoped_core;

public:
	ScopedCoreThreadPopup();
	virtual ~ScopedCoreThreadPopup() throw() {}

	virtual void AllowResume();
	virtual void DisallowResume();
};
