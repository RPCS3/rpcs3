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
#include "App.h"

using namespace pxSizerFlags;

wxString SysExecEvent::GetEventName() const
{
	pxFail( "Warning: Unnamed SysExecutor Event!  Please overload GetEventName() in all SysExecEvent derived classes." );
	return wxEmptyString;
}

wxString SysExecEvent::GetEventMessage() const
{
	return GetEventName();
}

// This is called from _DoInvokeEvent after various affinity and state checks have verified the
// message as executable.  Override this when possible.  Only override _DoInvokeEvent if you
// need some kind of additional low-level ability.
void SysExecEvent::InvokeEvent()
{
}

// This is called by _DoInvokeEvent *always* -- even when exceptions occur during InvokeEvent(),
// making this function a bit like a C# 'finally' block (try/catch/finally -- a nice feature lacking
// from C++ prior to the new C++0x standard).
//
// This function calls PostResult by default, and should be invoked by derived classes overriding
// CleanupEvent(), unless you want to change the PostResult behavior.
void SysExecEvent::CleanupEvent()
{
	PostResult();
}

// Transports the specified exception to the thread/context that invoked this event.
// Events are run from a try/catch block in the event handler that automatically transports
// exceptions as neeeded, so there shouldn't be much need to use this method directly.
void SysExecEvent::SetException( BaseException* ex )
{
	if( !ex ) return;

	ex->DiagMsg() += wxsFormat(L"(%s) ", GetEventName().c_str());
	//ex->UserMsg() = prefix + ex->UserMsg();

	if( m_sync )
	{
		// Throws the exception inline with the message handler (this makes the exception
		// look like it was thrown quite naturally).
		m_sync->SetException( ex );
	}
	else
	{
		// transport the exception to the main thread, since the message is fully
		// asynchronous, or has already entered an asynchronous state.  Message is sent
		// as a non-blocking action since proper handling of user errors on async messages
		// is *usually* to log/ignore it (hah), or to suspend emulation and issue a dialog
		// box to the user.

		wxGetApp().PostEvent( pxExceptionEvent( ex ) );
	}
}

void SysExecEvent::SetException( const BaseException& ex )
{
	SetException( ex.Clone() );
}


// This method calls _DoInvoke after performing some setup, exception handling, and
// affinity checks.  For implementing behavior of your event, override _DoInvoke
// instead, which is the intended method of implementing derived class invocation.
void SysExecEvent::_DoInvokeEvent()
{
	AffinityAssert_AllowFrom_SysExecutor();

	try {
		InvokeEvent();
	}
	catch( BaseException& ex )
	{
		SetException( ex );
	}
	catch( std::runtime_error& ex )
	{
		SetException( new Exception::RuntimeError(ex) );
	}

	// Cleanup Execution -- performed regardless of exception or not above.
	try {
		CleanupEvent();
	}
	catch( BaseException& ex )
	{
		SetException( ex );
	}
	catch( std::runtime_error& ex )
	{
		SetException( new Exception::RuntimeError(ex) );
	}
}

// Posts an empty result to the invoking context/thread of this message, if one exists.
// If the invoking thread posted the event in non-blocking fashion then no action is
// taken.
void SysExecEvent::PostResult() const 
{
	if( m_sync ) m_sync->PostResult();
}

// --------------------------------------------------------------------------------------
//  pxEvtHandler Implementations
// --------------------------------------------------------------------------------------
pxEvtHandler::pxEvtHandler()
{
	AtomicExchange( m_Quitting, false );
	m_qpc_Start = 0;
}

// Puts the event queue into Shutdown mode, which does *not* immediately stop nor cancel
// the queue's processing.  Instead it marks the queue inaccessible to all new events
// and continues procesing queued events for critical events that should not be ignored.
// (typically these are shutdown events critical to closing the app cleanly). Once
// all such events have been processed, the thread is stopped.
//
void pxEvtHandler::ShutdownQueue()
{
	if( m_Quitting ) return;
	AtomicExchange( m_Quitting, true );
	m_wakeup.Post();
}

struct ScopedThreadCancelDisable
{
	ScopedThreadCancelDisable()
	{
		int oldstate;
		pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &oldstate );
	}
	
	~ScopedThreadCancelDisable() throw()
	{
		int oldstate;
		pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldstate );
	}
};

void pxEvtHandler::ProcessEvents( pxEvtList& list )
{
	ScopedLock synclock( m_mtx_pending );
    
    pxEvtList::iterator node;
    while( node = list.begin(), node != list.end() )
    {
        ScopedPtr<SysExecEvent> deleteMe(*node);

		list.erase( node );
		if( !m_Quitting || deleteMe->IsCriticalEvent() )
		{
			// Some messages can be blocking, so we should release the mutex lock while
			// processing, to avoid having cases where the main thread deadlocks simply
			// trying to add a message to the queue due to the basic mutex acquire needed.

			m_qpc_Start = GetCPUTicks();

			synclock.Release();

			DevCon.WriteLn( L"(pxEvtHandler) Executing Event: %s [%s]", deleteMe->GetEventName().c_str(), deleteMe->AllowCancelOnExit() ? L"Cancelable" : L"Noncancelable" );

			if( deleteMe->AllowCancelOnExit() )
				deleteMe->_DoInvokeEvent();
			else
			{
				ScopedThreadCancelDisable thr_cancel_scope;
				deleteMe->_DoInvokeEvent();
			}

			u64 qpc_end = GetCPUTicks();
			DevCon.WriteLn( L"(pxEvtHandler) Event '%s' completed in %dms", deleteMe->GetEventName().c_str(), ((qpc_end-m_qpc_Start)*1000) / GetTickFrequency() );

			synclock.Acquire();
			m_qpc_Start = 0;		// lets the main thread know the message completed.
		}
		else
		{
			Console.WriteLn( L"(pxEvtHandler) Skipping Event: %s", deleteMe->GetEventName().c_str() );
			deleteMe->PostResult();
		}
	}
}

void pxEvtHandler::ProcessIdleEvents()
{
	ProcessEvents( m_idleEvents );
}

void pxEvtHandler::ProcessPendingEvents()
{
	ProcessEvents( m_pendingEvents );
}

// This method is provided for wxWidgets API conformance.  I like to use PostEvent instead
// since it's remenicient of PostMessage in Windows (and behaves rather similarly).
void pxEvtHandler::AddPendingEvent( SysExecEvent& evt )
{
	PostEvent( evt );
}

// Adds an event to the event queue in non-blocking fashion.  The thread executing this
// event queue will be woken up if it's idle/sleeping.
// IMPORTANT:  The pointer version of this function will *DELETE* the event object passed
// to it automatically when the event has been executed.  If you are using a scoped event
// you should use the Reference/Handle overload instead!
//
void pxEvtHandler::PostEvent( SysExecEvent* evt )
{
	ScopedPtr<SysExecEvent> sevt( evt );
	if( !sevt ) return;

	if( m_Quitting )
	{
		sevt->PostResult();
		return;
	}

	ScopedLock synclock( m_mtx_pending );
	
	//DbgCon.WriteLn( L"(%s) Posting event: %s  (queue count=%d)", GetEventHandlerName().c_str(), evt->GetEventName().c_str(), m_pendingEvents.size() );
	
	m_pendingEvents.push_back( sevt.DetachPtr() );
	if( m_pendingEvents.size() == 1)
		m_wakeup.Post();
}

void pxEvtHandler::PostEvent( const SysExecEvent& evt )
{
	PostEvent( evt.Clone() );
}

void pxEvtHandler::PostIdleEvent( SysExecEvent* evt )
{
	if( !evt ) return;

	if( m_Quitting )
	{
		evt->PostResult();
		return;
	}

	ScopedLock synclock( m_mtx_pending );

	if( m_idleEvents.size() == 0)
	{
		m_pendingEvents.push_back( evt );
		m_wakeup.Post();
	}
	else
		m_idleEvents.push_back( evt );
}

void pxEvtHandler::PostIdleEvent( const SysExecEvent& evt )
{
	PostIdleEvent( evt.Clone() );
}

void pxEvtHandler::ProcessEvent( SysExecEvent& evt )
{
	if( wxThread::GetCurrentId() != m_OwnerThreadId )
	{
		SynchronousActionState sync;
		evt.SetSyncState( sync );
		PostEvent( evt );
		sync.WaitForResult();
	}
	else
		evt._DoInvokeEvent();
}

void pxEvtHandler::ProcessEvent( SysExecEvent* evt )
{
	if( !evt ) return;

	if( wxThread::GetCurrentId() != m_OwnerThreadId )
	{
		SynchronousActionState sync;
		evt->SetSyncState( sync );
		PostEvent( evt );
		sync.WaitForResult();
	}
	else
	{
		ScopedPtr<SysExecEvent> deleteMe( evt );
		deleteMe->_DoInvokeEvent();
	}
}

bool pxEvtHandler::ProcessMethodSelf( FnType_Void* method )
{
	if( wxThread::GetCurrentId() != m_OwnerThreadId )
	{
		SynchronousActionState sync;
		SysExecEvent_MethodVoid evt(method);
		evt.SetSyncState( sync );
		PostEvent( evt );
		sync.WaitForResult();

		return true;
	}
	
	return false;
}

// This method invokes the derived class Idle implementations (if any) and then enters
// the sleep state until such time that new messages are received.
//
// FUTURE: Processes idle messages from the idle message queue (not implemented yet).
//
// Extending: Derived classes should override _DoIdle instead, unless it is necessary
// to implement post-wakeup behavior.
//
void pxEvtHandler::Idle()
{
	ProcessIdleEvents();
	_DoIdle();
	m_wakeup.WaitWithoutYield();
}

void pxEvtHandler::SetActiveThread()
{
	m_OwnerThreadId = wxThread::GetCurrentId();
}

// --------------------------------------------------------------------------------------
//  WaitingForThreadedTaskDialog
// --------------------------------------------------------------------------------------
// Note: currently unused (legacy code).  May be utilized at a later date, so I'm leaving
// it in (for now!)
//
class WaitingForThreadedTaskDialog
	: public wxDialogWithHelpers
{
private:
	typedef wxDialogWithHelpers _parent;

protected:
	pxThread*	m_thread;
	
public:
	WaitingForThreadedTaskDialog( pxThread* thr, wxWindow* parent, const wxString& title, const wxString& content );
	virtual ~WaitingForThreadedTaskDialog() throw() {}

protected:
	void OnCancel_Clicked( wxCommandEvent& evt );
	void OnTerminateApp_Clicked( wxCommandEvent& evt );
};

// --------------------------------------------------------------------------------------
//  WaitingForThreadedTaskDialog Implementations
// --------------------------------------------------------------------------------------
WaitingForThreadedTaskDialog::WaitingForThreadedTaskDialog( pxThread* thr, wxWindow* parent, const wxString& title, const wxString& content )
	: wxDialogWithHelpers( parent, title )
{
	SetMinWidth( 500 );

	m_thread		= thr;
	
	*this += Text( content )	| StdExpand();
	*this += 15;
	*this += Heading(_("Press Cancel to attempt to cancel the action."));
	*this += Heading(wxsFormat(_("Press Terminate to kill %s immediately."), pxGetAppName()));
	
	*this += new wxButton( this, wxID_CANCEL );
	*this += new wxButton( this, wxID_ANY, _("Terminate App") );
}

void WaitingForThreadedTaskDialog::OnCancel_Clicked( wxCommandEvent& evt )
{
	evt.Skip();
	if( !m_thread ) return;
	m_thread->Cancel( false );
	
	if( wxWindow* cancel = FindWindowById( wxID_CANCEL ) ) cancel->Disable();
}

void WaitingForThreadedTaskDialog::OnTerminateApp_Clicked( wxCommandEvent& evt )
{
	// (note: SIGTERM is a "handled" kill that performs shutdown stuff, which typically just crashes anyway)
	wxKill( wxGetProcessId(), wxSIGKILL );
}

// --------------------------------------------------------------------------------------
//  ExecutorThread Implementations
// --------------------------------------------------------------------------------------
ExecutorThread::ExecutorThread( pxEvtHandler* evthandler )
{
	m_EvtHandler = evthandler;
}

// Exposes the internal pxEvtHandler::ShutdownQueue API.  See pxEvtHandler for details.
void ExecutorThread::ShutdownQueue()
{
	if( !m_EvtHandler ) return;
	
	if( !m_EvtHandler->IsShuttingDown() )
		m_EvtHandler->ShutdownQueue();

	Block();
}

// Exposes the internal pxEvtHandler::PostEvent API.  See pxEvtHandler for details.
void ExecutorThread::PostEvent( SysExecEvent* evt )
{
	if( !pxAssert( m_EvtHandler ) ) { delete evt; return; }
	m_EvtHandler->PostEvent( evt );
}

void ExecutorThread::PostEvent( const SysExecEvent& evt )
{
	if( !pxAssert( m_EvtHandler ) ) return;
	m_EvtHandler->PostEvent( evt );
}

// Exposes the internal pxEvtHandler::PostIdleEvent API.  See pxEvtHandler for details.
void ExecutorThread::PostIdleEvent( SysExecEvent* evt )
{
	if( !pxAssert( m_EvtHandler ) ) return;
	m_EvtHandler->PostIdleEvent( evt );
}

void ExecutorThread::PostIdleEvent( const SysExecEvent& evt )
{
	if( !pxAssert( m_EvtHandler ) ) return;
	m_EvtHandler->PostIdleEvent( evt );
}

// Exposes the internal pxEvtHandler::ProcessEvent API.  See pxEvtHandler for details.
void ExecutorThread::ProcessEvent( SysExecEvent* evt )
{
	if( m_EvtHandler )
		m_EvtHandler->ProcessEvent( evt );
	else
	{
		ScopedPtr<SysExecEvent> deleteMe( evt );
		deleteMe->_DoInvokeEvent();
	}
}

void ExecutorThread::ProcessEvent( SysExecEvent& evt )
{
	if( m_EvtHandler )
		m_EvtHandler->ProcessEvent( evt );
	else
		evt._DoInvokeEvent();
}

void ExecutorThread::OnStart()
{
	//if( !m_ExecutorTimer )
	//	m_ExecutorTimer = new wxTimer( wxTheApp, pxEvt_ThreadTaskTimeout_SysExec );

	m_name		= L"SysExecutor";
	_parent::OnStart();
}

void ExecutorThread::ExecuteTaskInThread()
{
	if( !pxAssertDev( m_EvtHandler, "Gimme a damn Event Handler first, object whore." ) ) return;

	m_EvtHandler->SetActiveThread();

	while( true )
	{
		if( !pxAssertDev( m_EvtHandler, "Event handler has been deallocated during SysExecutor thread execution." ) ) return;

		m_EvtHandler->Idle();
		m_EvtHandler->ProcessPendingEvents();
		if( m_EvtHandler->IsShuttingDown() ) break;
	}
}

void ExecutorThread::OnCleanupInThread()
{
	//wxGetApp().PostCommand( m_task, pxEvt_ThreadTaskComplete );
	_parent::OnCleanupInThread();
}

// --------------------------------------------------------------------------------------
//  Threaded Event Handlers (Pcsx2App)
// --------------------------------------------------------------------------------------

// This event is called when the SysExecutorThread's timer triggers, which means the
// VM/system task has taken an oddly long period of time to complete. The task is able
// to invoke a modal dialog from here that will grant the user some options for handling
// the unresponsive task.
void Pcsx2App::OnSysExecutorTaskTimeout( wxTimerEvent& evt )
{
	if( !SysExecutorThread.IsRunning() ) return;

	//BaseThreadedInvocation* task = SysExecutorThread.GetTask();
	//if( !task ) return;

	//task->ShowModalStatus();
}
