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


class SysThreadBase : public PersistentThread, public virtual ISysThread
{
	typedef PersistentThread _parent;

protected:
	// Important: The order of these enumerations matters.  All "not-open" statuses must
	// be listed before ExecMode_Closed, since there are "optimized" tests that rely on the
	// assumption that "ExecMode <= ExecMode_Closed" equates to a closed thread status.
	enum ExecutionMode
	{
		// Thread has not been created yet.  Typically this is the same as IsRunning()
		// returning FALSE.
		ExecMode_NoThreadYet,

		// Close signal has been sent to the thread, but the thread's response is still
		// pending (thread is busy/running).
		ExecMode_Closing,

		// Thread is safely paused, with plugins in a "closed" state, and waiting for a
		// resume/open signal.
		ExecMode_Closed,

		// Thread is active and running, with pluigns in an "open" state.
		ExecMode_Opened,

		// Pause signal has been sent to the thread, but the thread's response is still
		// pending (thread is busy/running).
		ExecMode_Pausing,

		// Thread is safely paused, with plugins in an "open" state, and waiting for a
		// resume/open signal.
		ExecMode_Paused,
	};

	volatile ExecutionMode	m_ExecMode;

	// This lock is used to avoid simultaneous requests to Suspend/Resume/Pause from
	// contending threads.
	MutexLockRecursive		m_ExecModeMutex;

	// Used to wake up the thread from sleeping when it's in a suspended state.
	Semaphore				m_ResumeEvent;

	// Locked whenever the thread is not in a suspended state (either closed or paused).
	// Issue a Wait against this mutex for performing actions that require the thread
	// to be suspended.
	MutexLock				m_RunningLock;

public:
	explicit SysThreadBase();
	virtual ~SysThreadBase() throw();

	// Thread safety for IsOpen / IsClosed: The execution mode can change at any time on
	// any thread, so the actual status may have already changed by the time this function
	// returns its result.  Typically this isn't of major concern.  However if you need
	// more assured execution mode status, issue a lock against the ExecutionModeMutex()
	// first.
	bool IsOpen() const
	{
		return m_ExecMode > ExecMode_Closed;
	}

	bool IsClosed() const { return !IsOpen(); }

	ExecutionMode GetExecutionMode() const { return m_ExecMode; }
	MutexLock& ExecutionModeMutex() { return m_ExecModeMutex; }

	virtual bool Suspend( bool isBlocking = true );
	virtual void Resume();
	virtual bool Pause();

	virtual void StateCheckInThread( bool isCancelable = true );

protected:
	virtual void OnStart();

	// This function is called by Resume immediately prior to releasing the suspension of
	// the core emulation thread.  You should overload this rather than Resume(), since
	// Resume() has a lot of checks and balances to prevent re-entrance and race conditions.
	virtual void OnResumeReady() {}

	virtual void OnCleanupInThread();
	virtual void OnStartInThread();

	// Used internally from Resume(), so let's make it private here.
	virtual void Start();

	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called *in thread* from StateCheckInThread()
	// prior to suspending the thread (ie, when Suspend() has been called on a separate
	// thread, requesting this thread suspend itself temporarily).  After this is called,
	// the thread enters a waiting state on the m_ResumeEvent semaphore.
	virtual void OnSuspendInThread()=0;

	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called *in thread* from StateCheckInThread()
	// prior to pausing the thread (ie, when Pause() has been called on a separate thread,
	// requesting this thread pause itself temporarily).  After this is called, the thread
	// enters a waiting state on the m_ResumeEvent semaphore.
	virtual void OnPauseInThread()=0;

	// Extending classes should implement this, but should not call it.  The parent class
	// handles invocation by the following guidelines: Called from StateCheckInThread() after the
	// thread has been suspended and then subsequently resumed.
	// Parameter:
	//   isSuspended - set to TRUE if the thread is returning from a suspended state, or
	//     FALSE if it's returning from a paused state.
	virtual void OnResumeInThread( bool isSuspended )=0;
};

// --------------------------------------------------------------------------------------
//  EECoreThread class
// --------------------------------------------------------------------------------------
class SysCoreThread : public SysThreadBase
{
	typedef SysThreadBase _parent;

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

extern int sys_resume_lock;
