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

#include "Utilities/PersistentThread.h"
#include "Utilities/pxEvents.h"


// TODO!!  Make the system defined in this header system a bit more generic, and then move
// it to the Utilities library.

class pxEvtQueue;
class SysExecEvent;

// --------------------------------------------------------------------------------------
//  pxEvtLog / ConsoleLogSource_Event
// --------------------------------------------------------------------------------------

class ConsoleLogSource_Event : ConsoleLogSource
{
	typedef ConsoleLogSource _parent;

public:
	using _parent::IsActive;

	ConsoleLogSource_Event();

	bool Write( const pxEvtQueue* evtHandler, const SysExecEvent* evt, const wxChar* msg );
	bool Warn( const pxEvtQueue* evtHandler, const SysExecEvent* evt, const wxChar* msg );
	bool Error( const pxEvtQueue* evtHandler, const SysExecEvent* evt, const wxChar* msg );
};

extern ConsoleLogSource_Event pxConLog_Event;

#define pxEvtLog pxConLog_Event.IsActive() && pxConLog_Event


// --------------------------------------------------------------------------------------
//  SysExecEvent
// --------------------------------------------------------------------------------------
// Base class for all pxEvtQueue processable events.
//
// Rules for deriving:
//  * Override InvokeEvent(), *NOT* _DoInvokeEvent().  _DoInvokeEvent() performs setup and
//    wraps exceptions for transport to the invoking context/thread, and then itself calls
//    InvokeEvent() to perform the derived class implementation.
//
//  * Derived classes must implement their own versions of an empty constructor and
//    Clone(), or else the class will fail to be copied to the event handler's thread
//    context correctly.
//
//  * This class is not abstract, and gives no error if the invocation method is not
//    overridden:  It can be used as a simple ping device against the event queue,  Re-
//    awaking the invoking thread as soon as the queue has caught up to and processed
//    the event.
//
//  * Avoid using virtual class inheritence to 'cleverly' bind a SysExecEvent to another
//    existing class.  Multiple inheritence is unreliable at best, and virtual inheritence
//    is even worse.  Create a SysExecEvent wrapper class instead, and embed an instance of
//    the other class you want to turn into an event within it.  It might feel like more work
//    but it *will* be less work in the long run.
//
class SysExecEvent : public ICloneable
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

	SysExecEvent& SetSyncState( SynchronousActionState* obj ) { m_sync = obj;	return *this; }
	SysExecEvent& SetSyncState( SynchronousActionState& obj ) { m_sync = &obj;	return *this; }

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

	virtual void _DoInvokeEvent();
	virtual void PostResult() const;

	virtual wxString GetEventName() const;
	virtual wxString GetEventMessage() const;
	
	virtual int GetResult()
	{
		if( !pxAssertDev( m_sync != NULL, "SysEvent: Expecting return value, but no sync object provided." ) ) return 0;
		return m_sync->return_value;
	}

	virtual void SetException( BaseException* ex );

	void SetException( const BaseException& ex );

protected:
	virtual void InvokeEvent();
	virtual void CleanupEvent();
};

// --------------------------------------------------------------------------------------
//  SysExecEvent_MethodVoid
// --------------------------------------------------------------------------------------
class SysExecEvent_MethodVoid : public SysExecEvent
{
protected:
	FnType_Void*	m_method;
	bool			m_IsCritical;
	wxString		m_TraceName;

public:
	wxString GetEventName() const { return m_TraceName; }

	virtual ~SysExecEvent_MethodVoid() throw() {}
	SysExecEvent_MethodVoid* Clone() const { return new SysExecEvent_MethodVoid( *this ); }

	bool AllowCancelOnExit() const { return !m_IsCritical; }
	bool IsCriticalEvent() const { return m_IsCritical; }

	explicit SysExecEvent_MethodVoid( FnType_Void* method = NULL, const wxChar* traceName=NULL )	
		: m_TraceName( traceName ? traceName : L"VoidMethod" )
	{
		m_method = method;
		m_IsCritical = false;
	}
	
	SysExecEvent_MethodVoid& Critical()
	{
		m_IsCritical = true;
		return *this;
	}
	
protected:
	void InvokeEvent()
	{
		if( m_method ) m_method();
	}
};

#ifdef _MSC_VER
	typedef std::list< SysExecEvent*, wxObjectAllocator<SysExecEvent*> > pxEvtList;
#else
	typedef std::list<SysExecEvent*> pxEvtList;
#endif

// --------------------------------------------------------------------------------------
//  pxEvtQueue
// --------------------------------------------------------------------------------------
// Thread-safe platform-independent message queue, intended to act as a proxy between
// control threads (such as the Main/UI thread) and worker threads.
//
// Proxy message queues provide a safe environment for queuing tasks that must be executed
// in sequential order (in blocking fashion) on one or more worker threads.  The queue is
// deadlock-free, which means the Main/UI thread can queue events into this EventQueue without
// fear of causing unresponsiveness within the user interface.
//
// Rationales:
//  * Using the main event handler of wxWidgets is dangerous because it must call itself
//    recursively when waiting on threaded events such as semaphore and mutexes.  Thus,
//    tasks such as suspending the VM would invoke the event queue while waiting,
//    running events that expect the suspend to be complete while the suspend was still
//    pending.
//
//  * wxWidgets Event Queue (wxEvtHandler) isn't thread-safe and isn't even intended for
//    use for anything other than wxWindow events (it uses static vars and checks/modifies
//    wxApp globals while processing), so it's useless to us.  Have to roll our own. -_-
//
class pxEvtQueue
{
protected:
	pxEvtList					m_pendingEvents;
	pxEvtList					m_idleEvents;

	Threading::MutexRecursive	m_mtx_pending;
	Threading::Semaphore		m_wakeup;
	wxThreadIdType				m_OwnerThreadId;
	volatile u32				m_Quitting;

	// Used for performance measuring the execution of individual events,
	// and also for detecting deadlocks during message processing.
	volatile u64				m_qpc_Start;

public:
	pxEvtQueue();
	virtual ~pxEvtQueue() throw() {}

	virtual wxString GetEventHandlerName() const { return L"pxEvtQueue"; }

	virtual void ShutdownQueue();
	bool IsShuttingDown() const { return !!m_Quitting; }

	void ProcessEvents( pxEvtList& list, bool isIdle=false );
	void ProcessPendingEvents();
	void ProcessIdleEvents();
	void Idle();

	void AddPendingEvent( SysExecEvent& evt );
	void PostEvent( SysExecEvent* evt );
	void PostEvent( const SysExecEvent& evt );
	void PostIdleEvent( SysExecEvent* evt );
	void PostIdleEvent( const SysExecEvent& evt );

	void ProcessEvent( SysExecEvent* evt );
	void ProcessEvent( SysExecEvent& evt );

	bool Rpc_TryInvokeAsync( FnType_Void* method, const wxChar* traceName=NULL );
	bool Rpc_TryInvoke( FnType_Void* method, const wxChar* traceName=NULL );
	void SetActiveThread();

protected:
	virtual void _DoIdle() {}
};

// --------------------------------------------------------------------------------------
//  ExecutorThread
// --------------------------------------------------------------------------------------
// Threaded wrapper class for implementing a pxEvtQueue on its own thread, to serve as a
// proxy between worker threads and response threads (which cannot be allowed to stall or
// deadlock).  Simply create the desired EvtHandler, start the thread, and enjoy queued
// event execution in fully blocking fashion.
//
// Deadlock Protection Notes:
//  The ExecutorThread utilizes an underlying pxEventQueue, which only locks the queue for
//  adding and removing items only.  Events are processed during an unlocked queue state,
//  which allows other threads to add events with a guarantee that the add operation will
//  take very little time.
//
// Implementation Notes:
//  We use object encapsulation instead of multiple inheritance in order to avoid the many
//  unavoidable catastrophes and pitfalls involved in multi-inheritance that would ruin our
//  will to live.  The alternative requires manually exposing the interface of the underlying
//  instance of pxEventQueue; and that's ok really when you consider the alternative. --air
//
class ExecutorThread : public Threading::pxThread
{
	typedef Threading::pxThread _parent;

protected:
	ScopedPtr<wxTimer>		m_ExecutorTimer;
	ScopedPtr<pxEvtQueue>	m_EvtHandler;

public:
	ExecutorThread( pxEvtQueue* evtandler = NULL );
	virtual ~ExecutorThread() throw() { }

	virtual void ShutdownQueue();
	bool IsRunning() const;
	
	void PostEvent( SysExecEvent* evt );
	void PostEvent( const SysExecEvent& evt );

	void PostIdleEvent( SysExecEvent* evt );
	void PostIdleEvent( const SysExecEvent& evt );

	void ProcessEvent( SysExecEvent* evt );
	void ProcessEvent( SysExecEvent& evt );

	bool Rpc_TryInvokeAsync( void (*evt)(), const wxChar* traceName=NULL )
	{
		return m_EvtHandler ? m_EvtHandler->Rpc_TryInvokeAsync( evt, traceName ) : false;
	}

	bool Rpc_TryInvoke( void (*evt)(), const wxChar* traceName=NULL )
	{
		return m_EvtHandler ? m_EvtHandler->Rpc_TryInvoke( evt, traceName ) : false;
	}

protected:
	void OnStart();
	void ExecuteTaskInThread();
	void OnCleanupInThread();
};

