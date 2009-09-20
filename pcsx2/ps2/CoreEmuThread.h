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

// --------------------------------------------------------------------------------------
//  SysCoreThread class
// --------------------------------------------------------------------------------------
class SysCoreThread : public PersistentThread
{
protected:
	enum ExecutionMode
	{
		ExecMode_NoThreadYet,
		ExecMode_Idle,
		ExecMode_Running,
		ExecMode_Suspending,
		ExecMode_Suspended
	};

protected:
	volatile ExecutionMode	m_ExecMode;
	MutexLock				m_lock_ExecMode;

	bool			m_resetRecompilers;
	bool			m_resetProfilers;
	
	PluginManager&	m_plugins;
	Semaphore		m_ResumeEvent;
	Semaphore		m_SuspendEvent;

public:
	static SysCoreThread& Get();

public:
	explicit SysCoreThread( PluginManager& plugins );
	virtual ~SysCoreThread() throw();

	bool IsSuspended() const { return (m_ExecMode == ExecMode_Suspended); }
	virtual void Suspend( bool isBlocking = true );
	virtual void Resume();
	virtual void ApplySettings( const Pcsx2Config& src );
	virtual void StateCheck();

	virtual void DoThreadCleanup();

protected:
	void CpuInitializeMess();
	void CpuExecute();
	virtual sptr ExecuteTask();
};
