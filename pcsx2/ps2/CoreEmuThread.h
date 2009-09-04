/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include "Utilities/Threading.h"

using namespace Threading;

/////////////////////////////////////////////////////////////////////////////////////////
// CoreEmuThread
//
class CoreEmuThread : public PersistentThread
{
public:
	enum ExecutionMode
	{
		ExecMode_Idle,
		ExecMode_Running,
		ExecMode_Suspending,
		ExecMode_Suspended
	};

protected:
	volatile ExecutionMode m_ExecMode;

	Semaphore m_ResumeEvent;
	Semaphore m_SuspendEvent;

	bool m_resetRecompilers;
	bool m_resetProfilers;
	
	const wxString m_elf_file;

	MutexLock m_lock_ExecMode;

public:
	static CoreEmuThread& Get();

public:
	explicit CoreEmuThread( const wxString& elf_file=wxEmptyString );
	virtual ~CoreEmuThread();

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
