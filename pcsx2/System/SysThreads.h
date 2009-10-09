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

#include "Utilities/Threading.h"

using namespace Threading;

class ISysThread : public virtual IThread
{
public:
	ISysThread() {}
	virtual ~ISysThread() throw() {};

	virtual bool Suspend( bool isBlocking = true ) { return false; }
	virtual bool Pause() { return false; }
	virtual void Resume() {}
};


class SysSuspendableThread : public PersistentThread, public virtual ISysThread
{
	typedef PersistentThread _parent;

protected:
	enum ExecutionMode
	{
		ExecMode_NoThreadYet,
		ExecMode_Running,
		ExecMode_Suspending,
		ExecMode_Suspended,
		ExecMode_Pausing,
		ExecMode_Paused,
	};

	volatile ExecutionMode	m_ExecMode;
	MutexLock				m_lock_ExecMode;

	Semaphore				m_ResumeEvent;
	Semaphore				m_SuspendEvent;
	bool					m_ResumeProtection;
	
public:
	explicit SysSuspendableThread();
	virtual ~SysSuspendableThread() throw();

	bool IsExecMode_Running() const { return m_ExecMode == ExecMode_Running; }

	virtual bool Suspend( bool isBlocking = true );
	virtual void Resume();
	virtual bool Pause();
	
	virtual void StateCheck( bool isCancelable = true );
	virtual void OnCleanupInThread();

	// This function is called by Resume immediately prior to releasing the suspension of
	// the core emulation thread.  You should overload this rather than Resume(), since
	// Resume() has a lot of checks and balances to prevent re-entrance and race conditions.
	virtual void OnResumeReady() {}

	virtual void OnStart();

protected:

	// Used internally from Resume(), so let's make it private here.
	virtual void Start();

	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called *in thread* from StateCheck()
	// prior to suspending the thread (ie, when Suspend() has been called on a separate
	// thread, requesting this thread suspend itself temporarily).  After this is called,
	// the thread enters a waiting state on the m_ResumeEvent semaphore.
	virtual void OnSuspendInThread()=0;
	
	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called *in thread* from StateCheck()
	// prior to pausing the thread (ie, when Pause() has been called on a separate thread,
	// requesting this thread pause itself temporarily).  After this is called, the thread
	// enters a waiting state on the m_ResumeEvent semaphore.
	virtual void OnPauseInThread()=0;

	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called from StateCheck() after the
	// thread has been suspended and then subsequently resumed.
	// Parameter:
	//   isSuspended - set to TRUE if the thread is returning from a suspended state, or
	//     FALSE if it's returning from a paused state.
	virtual void OnResumeInThread( bool isSuspended )=0;
};

// --------------------------------------------------------------------------------------
//  EECoreThread class
// --------------------------------------------------------------------------------------
class SysCoreThread : public SysSuspendableThread
{
	typedef SysSuspendableThread _parent;

protected:
	bool			m_resetRecompilers;
	bool			m_resetProfilers;
	bool			m_resetVirtualMachine;
	bool			m_hasValidState;

public:
	static SysCoreThread& Get();

public:
	explicit SysCoreThread();
	virtual ~SysCoreThread() throw();

	virtual void ApplySettings( const Pcsx2Config& src );
	virtual void OnResumeReady();
	virtual void Reset();
	
	bool HasValidState()
	{
		return m_hasValidState;
	}

protected:
	void CpuInitializeMess();
	void CpuExecute();

	virtual void Start();
	virtual void OnSuspendInThread();
	virtual void OnPauseInThread() {}
	virtual void OnResumeInThread( bool IsSuspended );
	virtual void OnCleanupInThread();
	virtual void ExecuteTaskInThread();
};
