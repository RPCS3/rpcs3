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

#pragma once

#include "System/SysThreads.h"
#include "AppCommon.h"
#include "AppCorePlugins.h"

#define AffinityAssert_AllowFrom_CoreThread() \
	pxAssertMsg( GetCoreThread().IsSelf(), "Thread affinity violation: Call allowed from SysCoreThread only." )

#define AffinityAssert_DisallowFrom_CoreThread() \
	pxAssertMsg( !GetCoreThread().IsSelf(), "Thread affinity violation: Call is *not* allowed from SysCoreThread." )

// --------------------------------------------------------------------------------------
//  AppCoreThread class
// --------------------------------------------------------------------------------------
class AppCoreThread : public SysCoreThread
{
	typedef SysCoreThread _parent;

public:
	AppCoreThread();
	virtual ~AppCoreThread() throw();

	virtual void Suspend( bool isBlocking=false );
	virtual void Resume();
	virtual void Shutdown();
	virtual void Cancel( bool isBlocking=true );
	virtual void StateCheckInThread();
	virtual void ApplySettings( const Pcsx2Config& src );
	virtual void ChangeCdvdSource();

protected:
	virtual void OnResumeReady();
	virtual void OnResumeInThread( bool IsSuspended );
	virtual void OnSuspendInThread();
	virtual void OnCleanupInThread();
	virtual void PostVsyncToUI();
	virtual void ExecuteTaskInThread();
	virtual void DoCpuReset();
	virtual void CpuInitializeMess();

};

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
	
protected:
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

struct ScopedCoreThreadPause  : public BaseScopedCoreThread
{
	typedef BaseScopedCoreThread _parent;

public:
	ScopedCoreThreadPause();
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
