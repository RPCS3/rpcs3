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
#include "GSFrame.h"
#include "AppAccelerators.h"
#include "AppSaveStates.h"

#include "GS.h"
#include "MSWstuff.h"

#include <wx/utils.h>

static const KeyAcceleratorCode FULLSCREEN_TOGGLE_ACCELERATOR_GSPANEL=KeyAcceleratorCode( WXK_RETURN ).Alt();

void GSPanel::InitDefaultAccelerators()
{
	// Note!  These don't really work yet due to some hacks to get things working for
	// old legacy PAD plugins.  (the global accelerator tables are used instead) --air

	typedef KeyAcceleratorCode AAC;

	if( !m_Accels ) m_Accels = new AcceleratorDictionary;

	m_Accels->Map( AAC( WXK_F1 ),				"States_FreezeCurrentSlot" );
	m_Accels->Map( AAC( WXK_F3 ),				"States_DefrostCurrentSlot");
	m_Accels->Map( AAC( WXK_F3 ).Shift(),		"States_DefrostCurrentSlotBackup");
	m_Accels->Map( AAC( WXK_F2 ),				"States_CycleSlotForward" );
	m_Accels->Map( AAC( WXK_F2 ).Shift(),		"States_CycleSlotBackward" );

	m_Accels->Map( AAC( WXK_F4 ),				"Framelimiter_MasterToggle");
	m_Accels->Map( AAC( WXK_F4 ).Shift(),		"Frameskip_Toggle");
	m_Accels->Map( AAC( WXK_TAB ),				"Framelimiter_TurboToggle" );
	m_Accels->Map( AAC( WXK_TAB ).Shift(),		"Framelimiter_SlomoToggle" );

	m_Accels->Map( AAC( WXK_F6 ),				"GSwindow_CycleAspectRatio" );

	m_Accels->Map( AAC( WXK_NUMPAD_ADD ).Cmd(),			"GSwindow_ZoomIn" );	//CTRL on Windows/linux, CMD on OSX
	m_Accels->Map( AAC( WXK_NUMPAD_SUBTRACT ).Cmd(),	"GSwindow_ZoomOut" );
	m_Accels->Map( AAC( WXK_NUMPAD_MULTIPLY ).Cmd(),	"GSwindow_ZoomToggle" );

	m_Accels->Map( AAC( WXK_NUMPAD_ADD ).Cmd().Alt(),			"GSwindow_ZoomInY" );	//CTRL on Windows/linux, CMD on OSX
	m_Accels->Map( AAC( WXK_NUMPAD_SUBTRACT ).Cmd().Alt(),	"GSwindow_ZoomOutY" );
	m_Accels->Map( AAC( WXK_NUMPAD_MULTIPLY ).Cmd().Alt(),	"GSwindow_ZoomResetY" );

	m_Accels->Map( AAC( WXK_UP ).Cmd().Alt(),	"GSwindow_OffsetYminus" );
	m_Accels->Map( AAC( WXK_DOWN ).Cmd().Alt(),	"GSwindow_OffsetYplus" );
	m_Accels->Map( AAC( WXK_LEFT ).Cmd().Alt(),	"GSwindow_OffsetXminus" );
	m_Accels->Map( AAC( WXK_RIGHT ).Cmd().Alt(),	"GSwindow_OffsetXplus" );
	m_Accels->Map( AAC( WXK_NUMPAD_DIVIDE ).Cmd().Alt(),	"GSwindow_OffsetReset" );

	m_Accels->Map( AAC( WXK_ESCAPE ),			"Sys_Suspend" );
	m_Accels->Map( AAC( WXK_F8 ),				"Sys_TakeSnapshot" );
	m_Accels->Map( AAC( WXK_F8 ).Shift(),		"Sys_TakeSnapshot");
	m_Accels->Map( AAC( WXK_F8 ).Shift().Cmd(),	"Sys_TakeSnapshot");
	m_Accels->Map( AAC( WXK_F9 ),				"Sys_RenderswitchToggle");

	m_Accels->Map( AAC( WXK_F10 ),				"Sys_LoggingToggle" );
	m_Accels->Map( AAC( WXK_F11 ),				"Sys_FreezeGS" );
	m_Accels->Map( AAC( WXK_F12 ),				"Sys_RecordingToggle" );

	m_Accels->Map( FULLSCREEN_TOGGLE_ACCELERATOR_GSPANEL,		"FullscreenToggle" );
}

GSPanel::GSPanel( wxWindow* parent )
	: wxWindow()
	, m_HideMouseTimer( this )
{
	m_CursorShown	= true;
	m_HasFocus		= false;

	if ( !wxWindow::Create(parent, wxID_ANY) )
		throw Exception::RuntimeError().SetDiagMsg( L"GSPanel constructor explode!!" );

	SetName( L"GSPanel" );

	InitDefaultAccelerators();

	if( g_Conf->GSWindow.AlwaysHideMouse )
	{
		SetCursor( wxCursor(wxCURSOR_BLANK) );
		m_CursorShown = false;
	}

	Connect(wxEVT_CLOSE_WINDOW,		wxCloseEventHandler	(GSPanel::OnCloseWindow));
	Connect(wxEVT_SIZE,				wxSizeEventHandler	(GSPanel::OnResize));
	Connect(wxEVT_KEY_UP,			wxKeyEventHandler	(GSPanel::OnKeyDown));
	Connect(wxEVT_KEY_DOWN,			wxKeyEventHandler	(GSPanel::OnKeyDown));

	Connect(wxEVT_SET_FOCUS,		wxFocusEventHandler	(GSPanel::OnFocus));
	Connect(wxEVT_KILL_FOCUS,		wxFocusEventHandler	(GSPanel::OnFocusLost));

	Connect(m_HideMouseTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler(GSPanel::OnHideMouseTimeout) );

	// Any and all events which should result in the mouse cursor being made visible
	// are connected here.  If I missed one, feel free to add it in! --air

	Connect(wxEVT_LEFT_DOWN,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_LEFT_UP,			wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_MIDDLE_DOWN,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_MIDDLE_UP,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_RIGHT_DOWN,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_RIGHT_UP,			wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_MOTION,			wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_LEFT_DCLICK,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_LEFT_DCLICK,		wxMouseEventHandler	(GSPanel::OnLeftDclick));
	Connect(wxEVT_MIDDLE_DCLICK,	wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_RIGHT_DCLICK,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
	Connect(wxEVT_MOUSEWHEEL,		wxMouseEventHandler	(GSPanel::OnMouseEvent));
}

GSPanel::~GSPanel() throw()
{
	//CoreThread.Suspend( false );		// Just in case...!
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

	if ( !client.GetHeight() || !client.GetWidth() )
		return;

	double clientAr = (double)client.GetWidth()/(double)client.GetHeight();

	double targetAr = clientAr;
	if( g_Conf->GSWindow.AspectRatio == AspectRatio_4_3 )
		targetAr = 4.0/3.0;
	else if( g_Conf->GSWindow.AspectRatio == AspectRatio_16_9 )
		targetAr = 16.0/9.0;

	double arr = targetAr / clientAr;

	if( arr < 1 )
		viewport.x = (int)( (double)viewport.x*arr + 0.5);
	else if( arr > 1 )
		viewport.y = (int)( (double)viewport.y/arr + 0.5);

	float zoom = g_Conf->GSWindow.Zoom.ToFloat()/100.0;
	if( zoom == 0 )//auto zoom in untill black-bars are gone (while keeping the aspect ratio).
		zoom = max( (float)arr, (float)(1.0/arr) );

	viewport.Scale(zoom, zoom*g_Conf->GSWindow.StretchY.ToFloat()/100.0 );
	SetSize( viewport );
	CenterOnParent();
	
	int cx, cy;
	GetPosition(&cx, &cy);
	float unit = .01*(float)min(viewport.x, viewport.y);
	SetPosition( wxPoint( cx + unit*g_Conf->GSWindow.OffsetX.ToFloat(), cy + unit*g_Conf->GSWindow.OffsetY.ToFloat() ) );
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
	CoreThread.Suspend();
	evt.Skip();		// and close it.
}

void GSPanel::OnMouseEvent( wxMouseEvent& evt )
{
	if( IsBeingDeleted() ) return;

	// Do nothing for left-button event
	if (!evt.Button(1)) {
		evt.Skip();
		DoShowMouse();
	}

#ifdef __LINUX__
	// HACK2: In gsopen2 there is one event buffer read by both wx/gui and pad plugin. Wx deletes
	// the event before the pad see it. So you send key event directly to the pad.
	if( (PADWriteEvent != NULL) && (GSopen2 != NULL) ) {
		keyEvent event;
		// FIXME how to handle double click ???
		if (evt.ButtonDown()) {
			event.evt = 4; // X equivalent of ButtonPress
			event.key = evt.GetButton();
		} else if (evt.ButtonUp()) {
			event.evt = 5; // X equivalent of ButtonRelease
			event.key = evt.GetButton();
		} else if (evt.Moving() || evt.Dragging()) {
			event.evt = 6; // X equivalent of MotionNotify
			long x,y;
			evt.GetPosition(&x, &y);

			wxCoord w, h;
			wxWindowDC dc( this );
			dc.GetSize(&w, &h);

			// Special case to allow continuous mouvement near the border
			if (x < 10)
				x = 0;
			else if (x > (w-10))
				x = 0xFFFF;

			if (y < 10)
				y = 0;
			else if (y > (w-10))
				y = 0xFFFF;

			// For compatibility purpose with the existing structure. I decide to reduce
			// the position to 16 bits.
			event.key = ((y & 0xFFFF) << 16) | (x & 0xFFFF);

		} else {
			event.key = 0;
			event.evt = 0;
		}

		PADWriteEvent(event);
	}
#endif
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

#ifdef __LINUX__
	// HACK2: In gsopen2 there is one event buffer read by both wx/gui and pad plugin. Wx deletes
	// the event before the pad see it. So you send key event directly to the pad.
	if( (PADWriteEvent != NULL) && (GSopen2 != NULL) ) {
		keyEvent event;
		event.key = evt.GetRawKeyCode();
		if (evt.GetEventType() == wxEVT_KEY_UP)
			event.evt = 3; // X equivalent of KEYRELEASE;
		else if (evt.GetEventType() == wxEVT_KEY_DOWN)
			event.evt = 2; // X equivalent of KEYPRESS;
		else
			event.evt = 0;

		PADWriteEvent(event);
	}
#endif

	if( (PADopen != NULL) && CoreThread.IsOpen() ) return;
	DirectKeyCommand( evt );
}

void GSPanel::DirectKeyCommand( const KeyAcceleratorCode& kac )
{
	const GlobalCommandDescriptor* cmd = NULL;
	m_Accels->TryGetValue( kac.val32, cmd );
	if( cmd == NULL ) return;

	DbgCon.WriteLn( "(gsFrame) Invoking command: %s", cmd->Id );
	cmd->Invoke();
	
	if( cmd->AlsoApplyToGui && !g_ConfigPanelChanged)
		AppApplySettings();
}

void GSPanel::DirectKeyCommand( wxKeyEvent& evt )
{
	DirectKeyCommand(KeyAcceleratorCode( evt ));
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

#ifdef __LINUX__
	// HACK2: In gsopen2 there is one event buffer read by both wx/gui and pad plugin. Wx deletes
	// the event before the pad see it. So you send key event directly to the pad.
	if( (PADWriteEvent != NULL) && (GSopen2 != NULL) ) {
		keyEvent event = {0, 9}; // X equivalent of FocusIn;
		PADWriteEvent(event);
	}
#endif
	//Console.Warning("GS frame > focus set");
}

void GSPanel::OnFocusLost( wxFocusEvent& evt )
{
	evt.Skip();
	m_HasFocus = false;
	DoShowMouse();
#ifdef __LINUX__
	// HACK2: In gsopen2 there is one event buffer read by both wx/gui and pad plugin. Wx deletes
	// the event before the pad see it. So you send key event directly to the pad.
	if( (PADWriteEvent != NULL) && (GSopen2 != NULL) ) {
		keyEvent event = {0, 10}; // X equivalent of FocusOut
		PADWriteEvent(event);
	}
#endif
	//Console.Warning("GS frame > focus lost");
}

void GSPanel::AppStatusEvent_OnSettingsApplied()
{
	if( IsBeingDeleted() ) return;
	DoResize();
	DoShowMouse();
	Show( !EmuConfig.GS.DisableOutput );
}

void GSPanel::OnLeftDclick(wxMouseEvent& evt)
{
	if( !g_Conf->GSWindow.IsToggleFullscreenOnDoubleClick )
		return;

	//Console.WriteLn("GSPanel::OnDoubleClick: Invoking Fullscreen-Toggle accelerator.");
	DirectKeyCommand(FULLSCREEN_TOGGLE_ACCELERATOR_GSPANEL);
}



// --------------------------------------------------------------------------------------
//  GSFrame Implementation
// --------------------------------------------------------------------------------------

static const uint TitleBarUpdateMs = 333;


GSFrame::GSFrame(wxWindow* parent, const wxString& title)
	: wxFrame(parent, wxID_ANY, title,
		g_Conf->GSWindow.WindowPos, wxSize( 640, 480 ),
		(g_Conf->GSWindow.DisableResizeBorders ? 0 : wxRESIZE_BORDER) | wxCAPTION | wxCLIP_CHILDREN |
			wxSYSTEM_MENU | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX
	)
	, m_timer_UpdateTitle( this )
{
	SetIcons( wxGetApp().GetIconBundle() );
	SetClientSize( g_Conf->GSWindow.WindowSize );
	SetBackgroundColour( *wxBLACK );

	wxStaticText* label = new wxStaticText( this, wxID_ANY, _("GS Output is Disabled!") );
	m_id_OutputDisabled = label->GetId();
	label->SetFont( wxFont( 20, wxDEFAULT, wxNORMAL, wxBOLD ) );
	label->SetForegroundColour( *wxWHITE );
	label->Show( EmuConfig.GS.DisableOutput );

	GSPanel* gsPanel = new GSPanel( this );
	gsPanel->Show( !EmuConfig.GS.DisableOutput );
	m_id_gspanel = gsPanel->GetId();

	// TODO -- Implement this GS window status window!  Whee.
	// (main concern is retaining proper client window sizes when closing/re-opening the window).
	//m_statusbar = CreateStatusBar( 2 );

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler		(GSFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE,			wxMoveEventHandler		(GSFrame::OnMove) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler		(GSFrame::OnResize) );
	Connect( wxEVT_ACTIVATE,		wxActivateEventHandler	(GSFrame::OnActivate) );

	Connect(m_timer_UpdateTitle.GetId(), wxEVT_TIMER, wxTimerEventHandler(GSFrame::OnUpdateTitle) );
}

GSFrame::~GSFrame() throw()
{
}

void GSFrame::OnCloseWindow(wxCloseEvent& evt)
{
	sApp.OnGsFrameClosed( GetId() );
	evt.Skip();		// and close it.
}

bool GSFrame::ShowFullScreen(bool show, long style)
{
	/*if( show != IsFullScreen() )
		Console.WriteLn( Color_StrongMagenta, "(gsFrame) Switching to %s mode...", show ? "Fullscreen" : "Windowed" );*/

	if( g_Conf->GSWindow.IsFullscreen != show )
	{
		g_Conf->GSWindow.IsFullscreen = show;
		wxGetApp().PostIdleMethod( AppSaveSettings );
	}

	// IMPORTANT!  On MSW platforms you must ALWAYS show the window prior to calling
	// ShowFullscreen(), otherwise the window will be oddly unstable (lacking input and unable
	// to properly flip back into fullscreen mode after alt-enter).  I don't know if that
	// also happens on Linux.

	if( !IsShown() ) Show();
	bool retval = _parent::ShowFullScreen( show );
	
	return retval;
}



wxStaticText* GSFrame::GetLabel_OutputDisabled() const
{
	return (wxStaticText*)FindWindowById( m_id_OutputDisabled );
}

void GSFrame::CoreThread_OnResumed()
{
	m_timer_UpdateTitle.Start( TitleBarUpdateMs );
}

void GSFrame::CoreThread_OnSuspended()
{
	if( !IsBeingDeleted() && g_Conf->GSWindow.CloseOnEsc ) Hide();
}

void GSFrame::CoreThread_OnStopped()
{
	//if( !IsBeingDeleted() ) Destroy();
}

void GSFrame::CorePlugins_OnShutdown()
{
	if( !IsBeingDeleted() ) Destroy();
}

// overrides base Show behavior.
bool GSFrame::Show( bool shown )
{
	if( shown )
	{
		GSPanel* gsPanel = GetViewport();

		if( !gsPanel || gsPanel->IsBeingDeleted() )
		{
			gsPanel = new GSPanel( this );
			m_id_gspanel = gsPanel->GetId();
		}

		gsPanel->Show( !EmuConfig.GS.DisableOutput );
		gsPanel->DoResize();
		gsPanel->SetFocus();

		if( wxStaticText* label = GetLabel_OutputDisabled() )
			label->Show( EmuConfig.GS.DisableOutput );

		if( !m_timer_UpdateTitle.IsRunning() )
			m_timer_UpdateTitle.Start( TitleBarUpdateMs );
	}
	else
	{
		m_timer_UpdateTitle.Stop();
	}

	return _parent::Show( shown );
}

void GSFrame::AppStatusEvent_OnSettingsApplied()
{
	if( IsBeingDeleted() ) return;

	if( g_Conf->GSWindow.CloseOnEsc )
	{
		if( IsShown() && !CorePlugins.IsOpen(PluginId_GS) )
			Show( false );
	}

	if( wxStaticText* label = GetLabel_OutputDisabled() )
		label->Show( EmuConfig.GS.DisableOutput );
}

GSPanel* GSFrame::GetViewport()
{
	return (GSPanel*)FindWindowById( m_id_gspanel );
}


void GSFrame::OnUpdateTitle( wxTimerEvent& evt )
{
	double fps = wxGetApp().FpsManager.GetFramerate();

	char gsDest[128];
	GSgetTitleInfo2( gsDest, sizeof(gsDest) );

	const wxChar* limiterStr = L"None";

	if( g_Conf->EmuOptions.GS.FrameLimitEnable )
	{
		switch( g_LimiterMode )
		{
			case Limit_Nominal:	limiterStr = L"Normal"; break;
			case Limit_Turbo:	limiterStr = L"Turbo"; break;
			case Limit_Slomo:	limiterStr = L"Slomo"; break;
		}
	}

	FastFormatUnicode cpuUsage;
	if (m_CpuUsage.IsImplemented()) {
		m_CpuUsage.UpdateStats();
		if (THREAD_VU1) { // Display VU thread's usage
			cpuUsage.Write(L" | EE: %3d%% | GS: %3d%% | VU: %3d%% | UI: %3d%%",
				m_CpuUsage.GetEEcorePct(),	m_CpuUsage.GetGsPct(),
				m_CpuUsage.GetVUPct(),		m_CpuUsage.GetGuiPct());
		}
		else {
			cpuUsage.Write(L" | EE: %3d%% | GS: %3d%% | UI: %3d%%",
				m_CpuUsage.GetEEcorePct(),	m_CpuUsage.GetGsPct(),
				m_CpuUsage.GetGuiPct());
		}
	}

	const u64& smode2 = *(u64*)PS2GS_BASE(GS_SMODE2);

	SetTitle( pxsFmt( L"%s | %s (%s) | Limiter: %s | fps: %6.02f%s | State %d",
		fromUTF8(gsDest).c_str(),
		(smode2 & 1) ? L"Interlaced" : L"Progressive",
		(smode2 & 2) ? L"frame" : L"field",
		limiterStr, fps, cpuUsage.c_str(), States_GetCurrentSlot() )
	);

	//States_GetCurrentSlot()
}

void GSFrame::OnActivate( wxActivateEvent& evt )
{
	if( IsBeingDeleted() ) return;

	evt.Skip();
	if( wxWindow* gsPanel = GetViewport() ) gsPanel->SetFocus();
}

void GSFrame::OnMove( wxMoveEvent& evt )
{
	if( IsBeingDeleted() ) return;

	evt.Skip();

	g_Conf->GSWindow.IsMaximized = IsMaximized();

	// evt.GetPosition() returns the client area position, not the window frame position.
	if( !g_Conf->GSWindow.IsMaximized && !IsFullScreen() && !IsIconized() && IsVisible() )
		g_Conf->GSWindow.WindowPos = GetScreenPosition();

	// wxGTK note: X sends gratuitous amounts of OnMove messages for various crap actions
	// like selecting or deselecting a window, which muck up docking logic.  We filter them
	// out using 'lastpos' here. :)

	//static wxPoint lastpos( wxDefaultCoord, wxDefaultCoord );
	//if( lastpos == evt.GetPosition() ) return;
	//lastpos = evt.GetPosition();
}

void GSFrame::SetFocus()
{
	_parent::SetFocus();
	if( GSPanel* gsPanel = GetViewport() )
		gsPanel->SetFocusFromKbd();
}

void GSFrame::OnResize( wxSizeEvent& evt )
{
	if( IsBeingDeleted() ) return;

	if( !IsFullScreen() && !IsMaximized() && IsVisible() )
	{
		g_Conf->GSWindow.WindowSize	= GetClientSize();
	}

	if( wxStaticText* label = GetLabel_OutputDisabled() )
		label->CentreOnParent();

	if( GSPanel* gsPanel = GetViewport() )
	{
		gsPanel->DoResize();
		gsPanel->SetFocus();
	}

	//wxPoint hudpos = wxPoint(-10,-10) + (GetClientSize() - m_hud->GetSize());
	//m_hud->SetPosition( hudpos ); //+ GetScreenPosition() + GetClientAreaOrigin() );

	// if we skip, the panel is auto-sized to fit our window anyway, which we do not want!
	//evt.Skip();
}
