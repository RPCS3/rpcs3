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

#include "PrecompiledHeader.h"
#include "App.h"


// This is called from InvokeAction after various affinity and state checks have verified the
// message as executable.  Override this when possible.  Only override InvokeAction if you
// need some kind of additional low-level ability.
void SysExecEvent::_DoInvoke()
{
}

void SysExecEvent::SetException( BaseException* ex )
{
	if( !ex ) return;

	const wxString& prefix( wxsFormat(L"(%s) ", GetEventName()) );
	ex->DiagMsg() = prefix + ex->DiagMsg();
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
		// asynchronous.  Message is sent as a non-blocking action since proper handling
		// of user errors on async messages is *usually* to log/ignore it (hah), or to
		// suspend emulation and issue a dialog box to the user.

		wxGetApp().PostEvent( pxExceptionEvent( ex ) );
	}
}

void SysExecEvent::SetException( const BaseException& ex )
{
	SetException( ex.Clone() );
}


// This method calls _DoInvoke after performing some setup and affinity checks.
// Override _DoInvoke instead, which is the intended method of implementing derived class invocation.
void SysExecEvent::InvokeAction()
{
	//pxAssumeDev( !IsBeingDeleted(), "Attempted to process a deleted SysExecutor event." );
	AffinityAssert_AllowFrom_SysExecutor();
	try {
		_DoInvoke();
	}
	catch( BaseException& ex )
	{
		SetException( ex );
	}
	catch( std::runtime_error& ex )
	{
		SetException( new Exception::RuntimeError(ex) );
	}

	PostResult();
}

void SysExecEvent::PostResult() const 
{
	if( m_sync ) m_sync->PostResult();
}

pxEvtHandler::pxEvtHandler()
{
	AtomicExchange( m_Quitting, false );
}

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

void pxEvtHandler::ProcessPendingEvents()
{
	ScopedLock synclock( m_mtx_pending );
    
    pxEvtList::iterator node;
    while( node = m_pendingEvents.begin(), node != m_pendingEvents.end() )
    {
        ScopedPtr<SysExecEvent> deleteMe(wx_static_cast(SysExecEvent *, *node));

		m_pendingEvents.erase( node );
		if( !m_Quitting || deleteMe->IsCriticalEvent() )
		{
			// Some messages can be blocking, so we should release the mutex lock
			// to avoid having cases where the main thread deadlocks simply trying
			// to add a message to the queue.

			synclock.Release();

			if( deleteMe->AllowCancelOnExit() )
				deleteMe->InvokeAction();
			else
			{
				ScopedThreadCancelDisable thr_cancel_scope;
				deleteMe->InvokeAction();
			}

			synclock.Acquire();
		}
		else
		{
			Console.WriteLn( L"(pxEvtHandler:Skipping Event) %s", deleteMe->GetEventName().c_str() );
			deleteMe->PostResult();
		}
	}
}

// This method is provided for wxWidgets API conformance.
void pxEvtHandler::AddPendingEvent( SysExecEvent& evt )
{
	PostEvent( evt );
}

void pxEvtHandler::PostEvent( SysExecEvent* evt )
{
	if( !evt ) return;
	
	if( m_Quitting )
	{
		evt->PostResult();
		return;
	}

	ScopedLock synclock( m_mtx_pending );
	
	//DbgCon.WriteLn( L"(%s) Posting event: %s  (queue count=%d)", GetEventHandlerName().c_str(), evt->GetEventName().c_str(), m_pendingEvents.size() );
	
	m_pendingEvents.push_back( evt );
	if( m_pendingEvents.size() == 1)
		m_wakeup.Post();
}

void pxEvtHandler::PostEvent( const SysExecEvent& evt )
{
	if( m_Quitting )
	{
		evt.PostResult();
		return;
	}

	ScopedLock synclock( m_mtx_pending );
	m_pendingEvents.push_back( evt.Clone() );
	if( m_pendingEvents.size() == 1)
		m_wakeup.Post();
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
		evt.InvokeAction();
}

bool pxEvtHandler::SelfProcessMethod( FnType_Void* method )
{
	if( wxThread::GetCurrentId() != m_OwnerThreadId )
	{
		SynchronousActionState sync;
		SysExecEvent_Method evt(method);
		evt.SetSyncState( sync );
		PostEvent( evt );
		sync.WaitForResult();

		return true;
	}
	
	return false;
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
		deleteMe->InvokeAction();
	}
}

void pxEvtHandler::Idle()
{
	DoIdle();
	m_wakeup.WaitWithoutYield();
}

void pxEvtHandler::SetActiveThread()
{
	m_OwnerThreadId = wxThread::GetCurrentId();
}

// --------------------------------------------------------------------------------------
//  WaitingForThreadedTaskDialog
// --------------------------------------------------------------------------------------
class WaitingForThreadedTaskDialog
	: public wxDialogWithHelpers
{
private:
	typedef wxDialogWithHelpers _parent;

protected:
	PersistentThread*	m_thread;
	
public:
	WaitingForThreadedTaskDialog( PersistentThread* thr, wxWindow* parent, const wxString& title, const wxString& content );
	virtual ~WaitingForThreadedTaskDialog() throw() {}

protected:
	void OnCancel_Clicked( wxCommandEvent& evt );
	void OnTerminateApp_Clicked( wxCommandEvent& evt );
};

// --------------------------------------------------------------------------------------
//  WaitingForThreadedTaskDialog Implementations
// --------------------------------------------------------------------------------------
WaitingForThreadedTaskDialog::WaitingForThreadedTaskDialog( PersistentThread* thr, wxWindow* parent, const wxString& title, const wxString& content )
	: wxDialogWithHelpers( parent, title, wxVERTICAL )
{
	m_thread		= thr;
	m_idealWidth	= 500;
	
	*this += Heading( content );
	*this += 15;
	*this += Heading( _("Press Cancel to attempt to cancel the action.") );
	*this += Heading( _("Press Terminate to kill PCSX2 immediately.") );
	
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

void ExecutorThread::ShutdownQueue()
{
	if( !m_EvtHandler || m_EvtHandler->IsShuttingDown() ) return;
	m_EvtHandler->ShutdownQueue();
	Block();
}

void ExecutorThread::PostEvent( SysExecEvent* evt )
{
	if( !pxAssert( m_EvtHandler ) ) return;
	m_EvtHandler->PostEvent( evt );
}

void ExecutorThread::PostEvent( const SysExecEvent& evt )
{
	if( !pxAssert( m_EvtHandler ) ) return;
	m_EvtHandler->PostEvent( evt );
}

void ExecutorThread::ProcessEvent( SysExecEvent* evt )
{
	if( m_EvtHandler )
		m_EvtHandler->ProcessEvent( evt );
	else
	{
		ScopedPtr<SysExecEvent> deleteMe( evt );
		deleteMe->InvokeAction();
	}
}

void ExecutorThread::ProcessEvent( SysExecEvent& evt )
{
	if( m_EvtHandler )
		m_EvtHandler->ProcessEvent( evt );
	else
		evt.InvokeAction();
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
// the unresponsive thread.
void Pcsx2App::OnSysExecutorTaskTimeout( wxTimerEvent& evt )
{
	if( !SysExecutorThread.IsRunning() ) return;

	//BaseThreadedInvocation* task = SysExecutorThread.GetTask();
	//if( !task ) return;

	//task->ShowModalStatus();
}
