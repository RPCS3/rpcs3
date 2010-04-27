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

#include "Utilities/PersistentThread.h"
#include "Utilities/pxEvents.h"

// TODO!!  Make this system a bit more generic, and then move it to the Utilities library.

// --------------------------------------------------------------------------------------
//  SysExecEvent
// --------------------------------------------------------------------------------------
class SysExecEvent
	: public IActionInvocation
	, public ICloneable
{
protected:
	SynchronousActionState*		m_sync;

public:
	virtual ~SysExecEvent() throw() {}
	SysExecEvent* Clone() const { return new SysExecEvent( *this ); }

	SysExecEvent( SynchronousActionState* sync=NULL )
	{
		m_sync = sync;
	}

	SysExecEvent( SynchronousActionState& sync )
	{
		m_sync = &sync;
	}

	const SynchronousActionState* GetSyncState() const { return m_sync; }
	SynchronousActionState* GetSyncState() { return m_sync; }

	void SetSyncState( SynchronousActionState* obj ) { m_sync = obj; }
	void SetSyncState( SynchronousActionState& obj ) { m_sync = &obj; }

	// Tells the Event Handler whether or not this event can be skipped when the system
	// is being quit or reset.  Typically set this to true for events which shut down the
	// system, since program crashes can occur if the program tries to exit while threads
	// are running.
	virtual bool IsCriticalEvent() const { return false; }
	
	// Tells the Event Handler whether or not this event can be canceled.  Typically events
	// should not prohibit cancellation, since it expedites program termination and helps
	// avoid permanent deadlock.  Some actions like saving states and shutdown procedures
	// should not allow cancellation since they could result in program crashes or corrupted
	// data.
	virtual bool AllowCancelOnExit() const { return true; }

	virtual void InvokeAction();
	virtual void PostResult() const;

	virtual wxString GetEventName() const	{ return wxEmptyString; }
	virtual wxString GetEventMessage() const { return wxEmptyString; }
	
	virtual int GetResult()
	{
		if( !pxAssertDev( m_sync != NULL, "SysEvent: Expecting return value, but no sync object provided." ) ) return 0;
		return m_sync->return_value;
	}

	virtual void SetException( BaseException* ex );

	void SetException( const BaseException& ex );

protected:
	virtual void _DoInvoke();
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_Method
// --------------------------------------------------------------------------------------
class SysExecEvent_Method : public SysExecEvent
{
protected:
	FnType_Void*	m_method;

public:
	virtual ~SysExecEvent_Method() throw() {}
	SysExecEvent_Method* Clone() const { return new SysExecEvent_Method( *this ); }

	explicit SysExecEvent_Method( FnType_Void* method = NULL )	
	{
		m_method = method;
	}
	
protected:
	void _DoInvoke()
	{
		if( m_method ) m_method();
	}
};


typedef std::list<SysExecEvent*> pxEvtList;

// --------------------------------------------------------------------------------------
//  pxEvtHandler
// --------------------------------------------------------------------------------------
// wxWidgets Event Queue (wxEvtHandler) isn't thread-safe (uses static vars and checks/modifies wxApp globals
// while processing), so it's useless to us.  Have to roll our own. -_-
//
class pxEvtHandler
{
protected:
	pxEvtList					m_pendingEvents;
	Threading::MutexRecursive	m_mtx_pending;
	Threading::Semaphore		m_wakeup;
	wxThreadIdType				m_OwnerThreadId;
	volatile u32				m_Quitting;

public:
	pxEvtHandler();
	virtual ~pxEvtHandler() throw() {}

	virtual wxString GetEventHandlerName() const { return L"pxEvtHandler"; }

	virtual void ShutdownQueue();
	bool IsShuttingDown() const { return !!m_Quitting; }

	void ProcessPendingEvents();
	void AddPendingEvent( SysExecEvent& evt );
	void PostEvent( SysExecEvent* evt );
	void PostEvent( const SysExecEvent& evt );

	void ProcessEvent( SysExecEvent* evt );
	void ProcessEvent( SysExecEvent& evt );
	
	bool SelfProcessMethod( FnType_Void* method );

	void Idle();
	
	void SetActiveThread();

protected:
	virtual void DoIdle() {}
};

// --------------------------------------------------------------------------------------
//  ExecutorThread
// --------------------------------------------------------------------------------------
class ExecutorThread : public Threading::PersistentThread
{
	typedef Threading::PersistentThread _parent;

protected:
	ScopedPtr<wxTimer>		m_ExecutorTimer;
	ScopedPtr<pxEvtHandler>	m_EvtHandler;

public:
	ExecutorThread( pxEvtHandler* evtandler = NULL );
	virtual ~ExecutorThread() throw() { }

	virtual void ShutdownQueue();

	void PostEvent( SysExecEvent* evt );
	void PostEvent( const SysExecEvent& evt );

	void ProcessEvent( SysExecEvent* evt );
	void ProcessEvent( SysExecEvent& evt );

	bool SelfProcessMethod( void (*evt)() )
	{
		return m_EvtHandler ? m_EvtHandler->SelfProcessMethod( evt ) : false;
	}

protected:
	void OnStart();
	void ExecuteTaskInThread();
	void OnCleanupInThread();
};

