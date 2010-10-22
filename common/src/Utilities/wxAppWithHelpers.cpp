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
#include "wxAppWithHelpers.h"

#include "ThreadingInternal.h"
#include "PersistentThread.h"

DEFINE_EVENT_TYPE( pxEvt_DeleteObject );
DEFINE_EVENT_TYPE( pxEvt_DeleteThread );
DEFINE_EVENT_TYPE( pxEvt_StartIdleEventTimer );
DEFINE_EVENT_TYPE( pxEvt_InvokeAction );
DEFINE_EVENT_TYPE( pxEvt_SynchronousCommand );


IMPLEMENT_DYNAMIC_CLASS( pxSimpleEvent, wxEvent )

ConsoleLogSource_App::ConsoleLogSource_App()
{
	static const TraceLogDescriptor myDesc =
	{
		L"AppEvents",		L"App Events",
		wxLt("Includes idle event processing and some other uncommon event usages.")
	};
	
	m_Descriptor = &myDesc;
}

ConsoleLogSource_App pxConLog_App;

void BaseDeletableObject::DoDeletion()
{
	wxAppWithHelpers* app = wxDynamicCast( wxApp::GetInstance(), wxAppWithHelpers );
	pxAssume( app != NULL );
	app->DeleteObject( *this );
}


// --------------------------------------------------------------------------------------
//  SynchronousActionState Implementations
// --------------------------------------------------------------------------------------

void SynchronousActionState::SetException( const BaseException& ex )
{
	m_exception = ex.Clone();
}

void SynchronousActionState::SetException( BaseException* ex )
{
	if( !m_posted )
	{
		m_exception = ex;
	}
	else if( wxTheApp )
	{
		// transport the exception to the main thread, since the message is fully
		// asynchronous, or has already entered an asynchronous state.  Message is sent
		// as a non-blocking action since proper handling of user errors on async messages
		// is *usually* to log/ignore it (hah), or to suspend emulation and issue a dialog
		// box to the user.

		pxExceptionEvent ev( ex );
		wxTheApp->AddPendingEvent( ev );
	}
}

void SynchronousActionState::RethrowException() const
{
	if( m_exception ) m_exception->Rethrow();
}

int SynchronousActionState::WaitForResult()
{
	m_sema.WaitNoCancel();
	RethrowException();
	return return_value;
}

int SynchronousActionState::WaitForResult_NoExceptions()
{
	m_sema.WaitNoCancel();
	return return_value;
}

void SynchronousActionState::PostResult( int res )
{
	return_value = res;
	PostResult();
}

void SynchronousActionState::ClearResult()
{
	m_posted	= false;
	m_exception	= NULL;
}

void SynchronousActionState::PostResult()
{
	if( m_posted ) return;
	m_posted = true;
	m_sema.Post();
}

// --------------------------------------------------------------------------------------
//  pxActionEvent Implementations
// --------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS( pxActionEvent, wxEvent )

pxActionEvent::pxActionEvent( SynchronousActionState* sema, int msgtype )
	: wxEvent( 0, msgtype )
{
	m_state = sema;
}

pxActionEvent::pxActionEvent( SynchronousActionState& sema, int msgtype )
	: wxEvent( 0, msgtype )
{
	m_state = &sema;
}

pxActionEvent::pxActionEvent( const pxActionEvent& src )
	: wxEvent( src )
{
	m_state = src.m_state;
}

void pxActionEvent::SetException( const BaseException& ex )
{
	SetException( ex.Clone() );
}

void pxActionEvent::SetException( BaseException* ex )
{
	const wxString& prefix( pxsFmt(L"(%s) ", GetClassInfo()->GetClassName()) );
	ex->DiagMsg() = prefix + ex->DiagMsg();

	if( !m_state )
	{
		ScopedExcept exptr( ex );		// auto-delete it after handling.
		ex->Rethrow();
	}

	m_state->SetException( ex );
}

// --------------------------------------------------------------------------------------
//  pxSynchronousCommandEvent
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( pxSynchronousCommandEvent, wxCommandEvent )

pxSynchronousCommandEvent::pxSynchronousCommandEvent(SynchronousActionState* sema, wxEventType commandType, int winid)
	: wxCommandEvent( pxEvt_SynchronousCommand, winid )
{
	m_sync		= sema;
	m_realEvent = commandType;
}

pxSynchronousCommandEvent::pxSynchronousCommandEvent(SynchronousActionState& sema, wxEventType commandType, int winid)
	: wxCommandEvent( pxEvt_SynchronousCommand )
{
	m_sync		= &sema;
	m_realEvent = commandType;
}

pxSynchronousCommandEvent::pxSynchronousCommandEvent(SynchronousActionState* sema, const wxCommandEvent& evt )
	: wxCommandEvent( evt )
{
	m_sync		= sema;
	m_realEvent = evt.GetEventType();
	SetEventType( pxEvt_SynchronousCommand );
}

pxSynchronousCommandEvent::pxSynchronousCommandEvent(SynchronousActionState& sema, const wxCommandEvent& evt )
	: wxCommandEvent( evt )
{
	m_sync		= &sema;
	m_realEvent = evt.GetEventType();
	SetEventType( pxEvt_SynchronousCommand );
}

pxSynchronousCommandEvent::pxSynchronousCommandEvent( const pxSynchronousCommandEvent& src )
	: wxCommandEvent( src )
{
	m_sync		= src.m_sync;
	m_realEvent	= src.m_realEvent;
}

void pxSynchronousCommandEvent::SetException( const BaseException& ex )
{
	if( !m_sync ) ex.Rethrow();
	m_sync->SetException( ex );
}

void pxSynchronousCommandEvent::SetException( BaseException* ex )
{
	if( !m_sync )
	{
		ScopedExcept exptr( ex );		// auto-delete it after handling.
		ex->Rethrow();
	}

	m_sync->SetException( ex );
}

// --------------------------------------------------------------------------------------
//  pxRpcEvent
// --------------------------------------------------------------------------------------
// Unlike pxPingEvent, the Semaphore belonging to this event is typically posted when the
// invoked method is completed.  If the method can be executed in non-blocking fashion then
// it should leave the semaphore postback NULL.
//
class pxRpcEvent : public pxActionEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxRpcEvent)

	typedef pxActionEvent _parent;

protected:
	void (*m_Method)();

public:
	virtual ~pxRpcEvent() throw() { }
	virtual pxRpcEvent *Clone() const { return new pxRpcEvent(*this); }

	explicit pxRpcEvent( void (*method)()=NULL, SynchronousActionState* sema=NULL )
		: pxActionEvent( sema )
	{
		m_Method = method;
	}

	explicit pxRpcEvent( void (*method)(), SynchronousActionState& sema )
		: pxActionEvent( sema )
	{
		m_Method = method;
	}
	
	pxRpcEvent( const pxRpcEvent& src )
		: pxActionEvent( src )
	{
		m_Method = src.m_Method;
	}

	void SetMethod( void (*method)() )
	{
		m_Method = method;
	}

protected:
	void InvokeEvent()
	{
		if( m_Method ) m_Method();
	}
};

IMPLEMENT_DYNAMIC_CLASS( pxRpcEvent, pxActionEvent )

// --------------------------------------------------------------------------------------
//  pxExceptionEvent implementations
// --------------------------------------------------------------------------------------
pxExceptionEvent::pxExceptionEvent( const BaseException& ex )
{
	m_except = ex.Clone();
}

void pxExceptionEvent::InvokeEvent()
{
	ScopedExcept deleteMe( m_except );
	if( deleteMe ) deleteMe->Rethrow();
}

// --------------------------------------------------------------------------------------
//  wxAppWithHelpers Implementation
// --------------------------------------------------------------------------------------
//
// TODO : Ping dispatch and IdleEvent dispatch can be unified into a single dispatch, which
// would mean checking only one list of events per idle event, instead of two.  (ie, ping
// events can be appended to the idle event list, instead of into their own custom list).
//
IMPLEMENT_DYNAMIC_CLASS( wxAppWithHelpers, wxApp )


// Posts a method to the main thread; non-blocking.  Post occurs even when called from the
// main thread.
void wxAppWithHelpers::PostMethod( FnType_Void* method )
{
	PostEvent( pxRpcEvent( method ) );
}

// Posts a method to the main thread; non-blocking.  Post occurs even when called from the
// main thread.
void wxAppWithHelpers::PostIdleMethod( FnType_Void* method )
{
	pxRpcEvent evt( method );
	AddIdleEvent( evt );
}

// Invokes the specified void method, or posts the method to the main thread if the calling
// thread is not Main.  Action is blocking.  For non-blocking method execution, use
// AppRpc_TryInvokeAsync.
//
// This function works something like setjmp/longjmp, in that the return value indicates if the
// function actually executed the specified method or not.
//
// Returns:
//   FALSE if the method was not invoked (meaning this IS the main thread!)
//   TRUE if the method was invoked.
//

bool wxAppWithHelpers::Rpc_TryInvoke( FnType_Void* method )
{
	if( wxThread::IsMain() ) return false;

	SynchronousActionState sync;
	PostEvent( pxRpcEvent( method, sync ) );
	sync.WaitForResult();

	return true;
}

// Invokes the specified void method, or posts the method to the main thread if the calling
// thread is not Main.  Action is non-blocking (asynchronous).  For blocking method execution,
// use AppRpc_TryInvoke.
//
// This function works something like setjmp/longjmp, in that the return value indicates if the
// function actually executed the specified method or not.
//
// Returns:
//   FALSE if the method was not posted to the main thread (meaning this IS the main thread!)
//   TRUE if the method was posted.
//
bool wxAppWithHelpers::Rpc_TryInvokeAsync( FnType_Void* method )
{
	if( wxThread::IsMain() ) return false;
	PostEvent( pxRpcEvent( method ) );
	return true;
}

void wxAppWithHelpers::ProcessMethod( FnType_Void* method )
{
	if( wxThread::IsMain() )
	{
		method();
		return;
	}

	SynchronousActionState sync;
	PostEvent( pxRpcEvent( method, sync ) );
	sync.WaitForResult();
}

void wxAppWithHelpers::PostEvent( const wxEvent& evt )
{
	// Const Cast is OK!
	// Truth is, AddPendingEvent should be a const-qualified parameter, as
	// it makes an immediate clone copy of the event -- but wxWidgets
	// fails again in structured C/C++ design design.  So I'm forcing it as such
	// here. -- air

	_parent::AddPendingEvent( const_cast<wxEvent&>(evt) );
}

bool wxAppWithHelpers::ProcessEvent( wxEvent& evt )
{
	// Note: We can't do an automatic blocking post of the message here, because wxWidgets
	// isn't really designed for it (some events return data to the caller via the event
	// struct, and posting the event would require a temporary clone, where changes would
	// be lost).

	AffinityAssert_AllowFrom_MainUI();
	return _parent::ProcessEvent( evt );
}

bool wxAppWithHelpers::ProcessEvent( wxEvent* evt )
{
	AffinityAssert_AllowFrom_MainUI();
	ScopedPtr<wxEvent> deleteMe( evt );
	return _parent::ProcessEvent( *deleteMe );
}

bool wxAppWithHelpers::ProcessEvent( pxActionEvent& evt )
{
	if( wxThread::IsMain() )
		return _parent::ProcessEvent( evt );
	else
	{
		SynchronousActionState sync;
		evt.SetSyncState( sync );
		AddPendingEvent( evt );
		sync.WaitForResult();
		return true;
	}
}

bool wxAppWithHelpers::ProcessEvent( pxActionEvent* evt )
{
	if( wxThread::IsMain() )
	{
		ScopedPtr<wxEvent> deleteMe( evt );
		return _parent::ProcessEvent( *deleteMe );
	}
	else
	{
		SynchronousActionState sync;
		evt->SetSyncState( sync );
		AddPendingEvent( *evt );
		sync.WaitForResult();
		return true;
	}
}


void wxAppWithHelpers::CleanUp()
{
	// I'm pretty sure the message pump is dead by now, which means we need to run through
	// idle event list by hand and process the pending Deletion messages (all others can be
	// ignored -- it's only deletions we want handled, and really we *could* ignore those too
	// but I like to be tidy. -- air

	//IdleEventDispatcher( "CleanUp" );
	//DeletionDispatcher();
	_parent::CleanUp();
}

// Executes the event with exception handling.  If the event throws an exception, the exception
// will be neatly packaged and transported back to the thread that posted the event.
// This function is virtual, however overloading it is not recommended.  Derrived classes
// should overload InvokeEvent() instead.
void pxActionEvent::_DoInvokeEvent()
{
	AffinityAssert_AllowFrom_MainUI();

	try {
		InvokeEvent();
	}
	catch( BaseException& ex )
	{
		SetException( ex );
	}
	catch( std::runtime_error& ex )
	{
		SetException( new Exception::RuntimeError( ex ) );
	}

	if( m_state ) m_state->PostResult();
}

void wxAppWithHelpers::OnSynchronousCommand( pxSynchronousCommandEvent& evt )
{
	AffinityAssert_AllowFrom_MainUI();

	pxAppLog.Write(L"(App) Executing command event synchronously...");
	evt.SetEventType( evt.GetRealEventType() );

	try {
		ProcessEvent( evt );
	}
	catch( BaseException& ex )
	{
		evt.SetException( ex );
	}
	catch( std::runtime_error& ex )
	{
		evt.SetException( new Exception::RuntimeError( ex, evt.GetClassInfo()->GetClassName() ) );
	}

	if( Semaphore* sema = evt.GetSemaphore() ) sema->Post();
}

void wxAppWithHelpers::AddIdleEvent( const wxEvent& evt )
{
	ScopedLock lock( m_IdleEventMutex );
	if( m_IdleEventQueue.size() == 0 )
		PostEvent( pxSimpleEvent( pxEvt_StartIdleEventTimer ) );

	m_IdleEventQueue.push_back( evt.Clone() );
}

void wxAppWithHelpers::OnStartIdleEventTimer( wxEvent& evt )
{
	ScopedLock lock( m_IdleEventMutex );
	if( m_IdleEventQueue.size() != 0 )
		m_IdleEventTimer.Start( 100, true );
}

void wxAppWithHelpers::IdleEventDispatcher( const wxChar* action )
{
	// Recursion is possible thanks to modal dialogs being issued from the idle event handler.
	// (recursion shouldn't hurt anything anyway, since the node system re-creates the iterator
	// on each pass)
	
	//static int __guard=0;
	//RecursionGuard guard(__guard);
	//if( !pxAssertDev(!guard.IsReentrant(), "Re-entrant call to IdleEventdispatcher caught on camera!") ) return;

	wxEventList postponed;
	wxEventList::iterator node;

	ScopedLock lock( m_IdleEventMutex );

	while( node = m_IdleEventQueue.begin(), node != m_IdleEventQueue.end() )
	{
		ScopedPtr<wxEvent> deleteMe(*node);
		m_IdleEventQueue.erase( node );

		lock.Release();
		if( !Threading::AllowDeletions() && (deleteMe->GetEventType() == pxEvt_DeleteThread) )
		{
			// Threads that have active semaphores or mutexes (other threads are waiting on them) cannot
			// be deleted because those mutex/sema objects will become invalid and cause the pending
			// thread to crash.  So we disallow deletions when those waits are in action, and continue
			// to postpone the deletion of the thread until such time that it is safe.

			pxThreadLog.Write( ((pxThread*)((wxCommandEvent*)deleteMe.GetPtr())->GetClientData())->GetName(), L"Deletion postponed due to mutex or semaphore dependency." );
			postponed.push_back(deleteMe.DetachPtr());
		}
		else
		{
			pxAppLog.Write( L"(AppIdleQueue%s) Dispatching event '%s'", action, deleteMe->GetClassInfo()->GetClassName() );
			ProcessEvent( *deleteMe );		// dereference to prevent auto-deletion by ProcessEvent
		}
		lock.Acquire();
	}

	m_IdleEventQueue = postponed;
	if( m_IdleEventQueue.size() > 0 )
		pxAppLog.Write( L"(AppIdleQueue%s) %d events postponed due to dependencies.", action, m_IdleEventQueue.size() );
}

void wxAppWithHelpers::OnIdleEvent( wxIdleEvent& evt )
{
	m_IdleEventTimer.Stop();
	IdleEventDispatcher();
}

void wxAppWithHelpers::OnIdleEventTimeout( wxTimerEvent& evt )
{
	IdleEventDispatcher( L"[Timeout]" );
}

void wxAppWithHelpers::Ping()
{
	pxThreadLog.Write( pxGetCurrentThreadName().c_str(), L"App Event Ping Requested." );

	SynchronousActionState sync;
	pxActionEvent evt( sync );
	AddIdleEvent( evt );
	sync.WaitForResult();
}

void wxAppWithHelpers::PostCommand( void* clientData, int evtType, int intParam, long longParam, const wxString& stringParam )
{
	wxCommandEvent evt( evtType );
	evt.SetClientData( clientData );
	evt.SetInt( intParam );
	evt.SetExtraLong( longParam );
	evt.SetString( stringParam );
	AddPendingEvent( evt );
}

void wxAppWithHelpers::PostCommand( int evtType, int intParam, long longParam, const wxString& stringParam )
{
	PostCommand( NULL, evtType, intParam, longParam, stringParam );
}

sptr wxAppWithHelpers::ProcessCommand( void* clientData, int evtType, int intParam, long longParam, const wxString& stringParam )
{
	SynchronousActionState sync;
	pxSynchronousCommandEvent evt( sync, evtType );

	evt.SetClientData( clientData );
	evt.SetInt( intParam );
	evt.SetExtraLong( longParam );
	evt.SetString( stringParam );
	AddPendingEvent( evt );
	sync.WaitForResult();

	return sync.return_value;
}

sptr wxAppWithHelpers::ProcessCommand( int evtType, int intParam, long longParam, const wxString& stringParam )
{
	return ProcessCommand( NULL, evtType, intParam, longParam, stringParam );
}

void wxAppWithHelpers::PostAction( const pxActionEvent& evt )
{
	PostEvent( evt );
}

void wxAppWithHelpers::ProcessAction( pxActionEvent& evt )
{
	if( !wxThread::IsMain() )
	{
		SynchronousActionState sync;
		evt.SetSyncState( sync );
		AddPendingEvent( evt );
		sync.WaitForResult();
	}
	else
		evt._DoInvokeEvent();
}


void wxAppWithHelpers::DeleteObject( BaseDeletableObject& obj )
{
	pxAssert( !obj.IsBeingDeleted() );
	wxCommandEvent evt( pxEvt_DeleteObject );
	evt.SetClientData( (void*)&obj );
	AddIdleEvent( evt );
}

void wxAppWithHelpers::DeleteThread( pxThread& obj )
{
	pxThreadLog.Write(obj.GetName(), L"Scheduling for deletion...");
	wxCommandEvent evt( pxEvt_DeleteThread );
	evt.SetClientData( (void*)&obj );
	AddIdleEvent( evt );
}

typedef void (wxEvtHandler::*pxInvokeActionEventFunction)(pxActionEvent&);

bool wxAppWithHelpers::OnInit()
{
#define pxActionEventHandler(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxInvokeActionEventFunction, &func )

	Connect( pxEvt_SynchronousCommand,	pxSynchronousEventHandler	(wxAppWithHelpers::OnSynchronousCommand) );
	Connect( pxEvt_InvokeAction,		pxActionEventHandler		(wxAppWithHelpers::OnInvokeAction) );

	Connect( pxEvt_StartIdleEventTimer,	wxEventHandler				(wxAppWithHelpers::OnStartIdleEventTimer) );

	Connect( pxEvt_DeleteObject,		wxCommandEventHandler		(wxAppWithHelpers::OnDeleteObject) );
	Connect( pxEvt_DeleteThread,		wxCommandEventHandler		(wxAppWithHelpers::OnDeleteThread) );

	Connect( wxEVT_IDLE,				wxIdleEventHandler			(wxAppWithHelpers::OnIdleEvent) );

	Connect( m_IdleEventTimer.GetId(),	wxEVT_TIMER, wxTimerEventHandler(wxAppWithHelpers::OnIdleEventTimeout) );

	return _parent::OnInit();
}

void wxAppWithHelpers::OnInvokeAction( pxActionEvent& evt )
{
	evt._DoInvokeEvent();		// wow this is easy!
}

void wxAppWithHelpers::OnDeleteObject( wxCommandEvent& evt )
{
	if( evt.GetClientData() == NULL ) return;
	delete (BaseDeletableObject*)evt.GetClientData();
}

// Threads have their own deletion handler that propagates exceptions thrown by the thread to the UI.
// (thus we have a fairly automatic threaded exception system!)
void wxAppWithHelpers::OnDeleteThread( wxCommandEvent& evt )
{
	ScopedPtr<pxThread> thr( (pxThread*)evt.GetClientData() );
	if( !thr )
	{
		pxThreadLog.Write( L"null", L"OnDeleteThread: NULL thread object received (and ignored)." );
		return;
	}

	pxThreadLog.Write(thr->GetName(), (wxString)L"Thread object deleted successfully" + (thr->HasPendingException() ? L" [exception pending!]" : wxEmptyString));
	thr->RethrowException();
}

wxAppWithHelpers::wxAppWithHelpers()
	: m_IdleEventTimer( this )
{
#ifdef __WXMSW__
	// This variable assignment ensures that MSVC links in the TLS setup stubs even in
	// full optimization builds.  Without it, DLLs that use TLS won't work because the
	// FS segment register won't have been initialized by the main exe, due to tls_insurance
	// being optimized away >_<  --air

	static __threadlocal int	tls_insurance = 0;
	tls_insurance = 1;
#endif
}

