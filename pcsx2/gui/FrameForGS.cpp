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
#include "MainFrame.h"
#include "GS.h"

#include "wx/utils.h"


void GSPanel::InitDefaultAccelerators()
{
	typedef KeyAcceleratorCode AAC;

	m_Accels.Map( AAC( WXK_F1 ),				"States_FreezeCurrentSlot" );
	m_Accels.Map( AAC( WXK_F3 ),				"States_DefrostCurrentSlot");
	m_Accels.Map( AAC( WXK_F2 ),				"States_CycleSlotForward" );
	m_Accels.Map( AAC( WXK_F2 ).Shift(),		"States_CycleSlotBackward" );
	
	m_Accels.Map( AAC( WXK_F4 ),				"Frameskip_Toggle" );
	m_Accels.Map( AAC( WXK_TAB ),				"Framelimiter_TurboToggle" );
	m_Accels.Map( AAC( WXK_TAB ).Shift(),		"Framelimiter_MasterToggle" );
	
	m_Accels.Map( AAC( WXK_ESCAPE ),			"Sys_Suspend" );
	m_Accels.Map( AAC( WXK_F8 ),				"Sys_TakeSnapshot" );
	m_Accels.Map( AAC( WXK_F9 ),				"Sys_RenderswitchToggle" );
	
	//m_Accels.Map( AAC( WXK_F10 ),				"Sys_LoggingToggle" );
	m_Accels.Map( AAC( WXK_F11 ),				"Sys_FreezeGS" );
	m_Accels.Map( AAC( WXK_F12 ),				"Sys_RecordingToggle" );

	m_Accels.Map( AAC( WXK_RETURN ).Alt(),		"FullscreenToggle" );
}

GSPanel::GSPanel( wxWindow* parent )
	: wxWindow()
	, m_Listener_SettingsApplied( wxGetApp().Source_SettingsApplied(), EventListener<int>	( this, OnSettingsApplied ) )
	, m_HideMouseTimer( this )
{
	m_CursorShown	= true;
	m_HasFocus		= false;
	
	if ( !wxWindow::Create(parent, wxID_ANY) )
		throw Exception::RuntimeError( "GSPanel constructor esplode!!" );

	SetName( L"GSPanel" );

	InitDefaultAccelerators();

	if( g_Conf->GSWindow.AlwaysHideMouse )
	{
		SetCursor( wxCursor(wxCURSOR_BLANK) );
		m_CursorShown = false;
	}

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler	(GSPanel::OnCloseWindow) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler	(GSPanel::OnResize) );
	Connect( wxEVT_KEY_DOWN,		wxKeyEventHandler	(GSPanel::OnKeyDown) );

	Connect( wxEVT_SET_FOCUS,		wxFocusEventHandler(GSPanel::OnFocus) );
	Connect( wxEVT_KILL_FOCUS,		wxFocusEventHandler(GSPanel::OnFocusLost) );

	Connect(wxEVT_MIDDLE_DOWN,		wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_MIDDLE_UP,		wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_RIGHT_DOWN,		wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_RIGHT_UP,			wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_MOTION,			wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_LEFT_DCLICK,		wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_MIDDLE_DCLICK,	wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_RIGHT_DCLICK,		wxMouseEventHandler(GSPanel::OnShowMouse) );
	Connect(wxEVT_MOUSEWHEEL,		wxMouseEventHandler(GSPanel::OnShowMouse) );

	Connect(m_HideMouseTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(GSPanel::OnHideMouseTimeout) );
}

GSPanel::~GSPanel() throw()
{
	CoreThread.Suspend();		// Just in case...!
}

void GSPanel::DoShowMouse()
{
	if( g_Conf->GSWindow.AlwaysHideMouse ) return;

	if( !m_CursorShown )
	{
		SetCursor( wxCursor( wxCURSOR_DEFAULT ) );
		m_CursorShown = true;
	}
	m_HideMouseTimer.Start( 1750, true );
}

void GSPanel::DoResize()
{
	if( GetParent() == NULL ) return;
	wxSize client = GetParent()->GetClientSize();
	wxSize viewport = client;

	switch( g_Conf->GSWindow.AspectRatio )
	{
		case AspectRatio_Stretch:
			// just default to matching client size.
		break;

		case AspectRatio_4_3:
			if( client.x/4 <= client.y/3 )
				viewport.y = (int)(client.x * (3.0/4.0));
			else
				viewport.x = (int)(client.y * (4.0/3.0));
		break;

		case AspectRatio_16_9:
			if( client.x/16 <= client.y/9 )
				viewport.y = (int)(client.x * (9.0/16.0));
			else
				viewport.x = (int)(client.y * (16.0/9.0));
		break;
	}

	SetSize( viewport );
	CenterOnParent();
}

void GSPanel::OnResize(wxSizeEvent& event)
{
	if( IsBeingDeleted() ) return;
	DoResize();
	//Console.Error( "Size? %d x %d", GetSize().x, GetSize().y );
	//event.
}

void GSPanel::OnCloseWindow(wxCloseEvent& evt)
{
	wxGetApp().OnGsFrameClosed();
	evt.Skip();		// and close it.
}

void GSPanel::OnShowMouse( wxMouseEvent& evt )
{
	if( IsBeingDeleted() ) return;
	evt.Skip();
	DoShowMouse();
}

void GSPanel::OnHideMouseTimeout( wxTimerEvent& evt )
{
	if( IsBeingDeleted() || !m_HasFocus ) return;
	if( CoreThread.GetExecutionMode() != SysThreadBase::ExecMode_Opened ) return;

	SetCursor( wxCursor( wxCURSOR_BLANK ) );
	m_CursorShown = false;
}

void GSPanel::OnKeyDown( wxKeyEvent& evt )
{
	// HACK: Legacy PAD plugins expect PCSX2 to ignore keyboard messages on the GS Window while
	// the PAD plugin is open, so ignore here (PCSX2 will direct messages routed from PAD directly
	// to the APP level message handler, which in turn routes them right back here -- yes it's
	// silly, but oh well).

	if( (PADopen != NULL) && CoreThread.IsOpen() ) return;

	const GlobalCommandDescriptor* cmd = NULL;
	m_Accels.TryGetValue( KeyAcceleratorCode( evt ).val32, cmd );
	if( cmd == NULL )
	{
		evt.Skip();		// Let the global APP handle it if it wants
		return;
	}
	
	if( cmd != NULL )
	{
		DbgCon.WriteLn( "(gsFrame) Invoking command: %s", cmd->Id );
		cmd->Invoke();
	}
}

void GSPanel::OnFocus( wxFocusEvent& evt )
{
	evt.Skip();
	m_HasFocus = true;
	
	if( g_Conf->GSWindow.AlwaysHideMouse )
	{
		SetCursor( wxCursor(wxCURSOR_BLANK) );
		m_CursorShown = false;
	}
	else
		DoShowMouse();
}

void GSPanel::OnFocusLost( wxFocusEvent& evt )
{
	evt.Skip();
	m_HasFocus = false;
	DoShowMouse();
}

void __evt_fastcall GSPanel::OnSettingsApplied( void* obj, int& evt )
{
	if( obj == NULL ) return;
	GSPanel* panel = (GSPanel*)obj;
	
	if( panel->IsBeingDeleted() ) return;
	panel->DoResize();
	panel->DoShowMouse();
}

// --------------------------------------------------------------------------------------
//  GSFrame
// --------------------------------------------------------------------------------------

GSFrame::GSFrame(wxWindow* parent, const wxString& title)
	: wxFrame(parent, wxID_ANY, title,
		g_Conf->GSWindow.WindowPos, wxSize( 640, 480 ), 
		(g_Conf->GSWindow.DisableResizeBorders ? 0 : wxRESIZE_BORDER) | wxCAPTION | wxCLIP_CHILDREN |
			wxSYSTEM_MENU | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX
	)
{
	SetIcons( wxGetApp().GetIconBundle() );

	SetClientSize( g_Conf->GSWindow.WindowSize );

	m_gspanel	= new GSPanel( this );

	//Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler		(GSFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE,			wxMoveEventHandler		(GSFrame::OnMove) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler		(GSFrame::OnResize) );
	Connect( wxEVT_ACTIVATE,		wxActivateEventHandler	(GSFrame::OnActivate) );
}

GSFrame::~GSFrame() throw()
{
}

void __evt_fastcall GSFrame::OnSettingsApplied( void* obj, int& evt )
{
	if( obj == NULL ) return;
	GSFrame* frame = (GSFrame*)obj;

	if( frame->IsBeingDeleted() ) return;
	ShowFullScreen( g_Conf->GSWindow.DefaultToFullscreen );
}

wxWindow* GSFrame::GetViewport()
{
	return m_gspanel;
}

void GSFrame::OnActivate( wxActivateEvent& evt )
{
	if( IsBeingDeleted() ) return;

	evt.Skip();
	if( wxWindow* gsPanel = FindWindowByName(L"GSPanel") ) gsPanel->SetFocus();
}

void GSFrame::OnMove( wxMoveEvent& evt )
{
	if( IsBeingDeleted() ) return;

	evt.Skip();

	// evt.GetPosition() returns the client area position, not the window frame position.
	if( !IsMaximized() && IsVisible() )
		g_Conf->GSWindow.WindowPos	= GetScreenPosition();

	// wxGTK note: X sends gratuitous amounts of OnMove messages for various crap actions
	// like selecting or deselecting a window, which muck up docking logic.  We filter them
	// out using 'lastpos' here. :)

	//static wxPoint lastpos( wxDefaultCoord, wxDefaultCoord );
	//if( lastpos == evt.GetPosition() ) return;
	//lastpos = evt.GetPosition();
}

void GSFrame::OnResize( wxSizeEvent& evt )
{
	if( !IsMaximized() && IsVisible() )
		g_Conf->GSWindow.WindowSize	= GetClientSize();

	if( GSPanel* gsPanel = (GSPanel*)FindWindowByName(L"GSPanel") )
	{
		gsPanel->DoResize();
		gsPanel->SetFocus();
	}

	//wxPoint hudpos = wxPoint(-10,-10) + (GetClientSize() - m_hud->GetSize());
	//m_hud->SetPosition( hudpos ); //+ GetScreenPosition() + GetClientAreaOrigin() );

	// if we skip, the panel is auto-sized to fit our window anyway, which we do not want!
	//evt.Skip();
}
