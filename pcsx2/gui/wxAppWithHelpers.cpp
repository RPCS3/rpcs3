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
#include "wxAppWithHelpers.h"

DEFINE_EVENT_TYPE( pxEvt_Ping );
DEFINE_EVENT_TYPE( pxEvt_MessageBox );
DEFINE_EVENT_TYPE( pxEvt_Assertion );

// --------------------------------------------------------------------------------------
//  pxPingEvent Implementations
// --------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS( pxPingEvent, wxEvent )

pxPingEvent::pxPingEvent( int msgtype, Semaphore* sema )
	: wxEvent( 0, msgtype )
{
	m_PostBack = sema;
}

pxPingEvent::pxPingEvent( Semaphore* sema )
	: wxEvent( 0, pxEvt_Ping )
{
	m_PostBack = sema;
}

pxPingEvent::pxPingEvent( const pxPingEvent& src )
	: wxEvent( src )
{
	m_PostBack = src.m_PostBack;
}

void wxAppWithHelpers::OnPingEvent( pxPingEvent& evt )
{
	// Ping events are dispatched during the idle event handler, which ensures
	// the ping is posted only after all other pending messages behind the ping
	// are also processed.

	m_PingWhenIdle.push_back( evt.GetSemaphore() );
	m_PingTimer.Start( 200, true );
}

void wxAppWithHelpers::PingDispatch( const char* action )
{
	size_t size = m_PingWhenIdle.size();
	if( size == 0 ) return;

	DbgCon.WriteLn( Color_Gray, "App Event Ping (%s) -> %u listeners.", action, size );

	for( size_t i=0; i<size; ++i )
	{
		if( Semaphore* sema = m_PingWhenIdle[i] ) sema->Post();
	}

	m_PingWhenIdle.clear();
}

void wxAppWithHelpers::OnIdleEvent( wxIdleEvent& evt )
{
	evt.Skip();
	m_PingTimer.Stop();
	PingDispatch( "Idle" );
}

void wxAppWithHelpers::OnPingTimeout( wxTimerEvent& evt )
{
	PingDispatch( "Timeout" );
}

void wxAppWithHelpers::Ping()
{
	DbgCon.WriteLn( Color_Gray, L"App Event Ping Requested from %s thread.", pxGetCurrentThreadName().c_str() );

	Semaphore sema;
	pxPingEvent evt( &sema );
	AddPendingEvent( evt );
	sema.WaitNoCancel();
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


typedef void (wxEvtHandler::*pxMessageBoxEventFunction)(pxMessageBoxEvent&);
typedef void (wxEvtHandler::*pxPingEventFunction)(pxPingEvent&);

bool wxAppWithHelpers::OnInit()
{
#define pxMessageBoxEventThing(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxMessageBoxEventFunction, &func )

#define pxPingEventHandler(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxPingEventFunction, &func )

	Connect( pxEvt_MessageBox,		pxMessageBoxEventThing	(wxAppWithHelpers::OnMessageBox) );
	Connect( pxEvt_Assertion,		pxMessageBoxEventThing	(wxAppWithHelpers::OnMessageBox) );
	Connect( pxEvt_Ping,			pxPingEventHandler		(wxAppWithHelpers::OnPingEvent) );
	Connect( wxEvt_Idle,			wxIdleEventHandler		(wxAppWithHelpers::OnIdleEvent) );

	Connect( m_PingTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(wxAppWithHelpers::OnPingTimeout) );

	return _parent::OnInit();
}

void wxAppWithHelpers::OnMessageBox( pxMessageBoxEvent& evt )
{
	evt.IssueDialog();
}

wxAppWithHelpers::wxAppWithHelpers()
	: m_PingTimer( this )
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

