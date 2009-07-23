/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hopeb that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "MainFrame.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/AppIcon.h"

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesMenu()
{
	wxMenu* mnuStates = new wxMenu();

	m_LoadStatesSubmenu.Append( Menu_State_LoadOther, _("Other..."), wxEmptyString, wxITEM_NORMAL );
	m_SaveStatesSubmenu.Append( Menu_State_SaveOther, _("Other..."), wxEmptyString, wxITEM_NORMAL );

	mnuStates->Append( Menu_State_Load, _("Load"), &m_LoadStatesSubmenu, wxEmptyString );
	mnuStates->Append( Menu_State_Save, _("Save"), &m_SaveStatesSubmenu, wxEmptyString );
	return mnuStates;
}

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesSubMenu( int baseid ) const
{
	wxMenu* mnuSubstates = new wxMenu();

	mnuSubstates->Append( baseid,    _("Slot 0"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+1,  _("Slot 1"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+2,  _("Slot 2"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+3,  _("Slot 3"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+4,  _("Slot 4"), wxEmptyString, wxITEM_NORMAL );
	return mnuSubstates;
}

/*struct StringListNode
{
	wxString* item;
	StringListNode* next;
};*/

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeIsoMenu()
{
	wxMenu* mnuIso = new wxMenu();

	mnuIso->Append( Menu_IsoBrowse, _("Browse..."), _("Select an Iso image from your hard drive.") );

	if( g_RecentIsoList != NULL )
	{
		g_RecentIsoList->UseMenu( mnuIso );
		g_RecentIsoList->AddFilesToMenu( mnuIso );
	}
	return mnuIso;
}

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeCdvdMenu()
{
	wxMenu* mnuCdvd = new wxMenu();
	return mnuCdvd;
}

// ------------------------------------------------------------------------
void MainEmuFrame::PopulateVideoMenu()
{
	m_menuVideo.Append( Menu_Video_Basics,	_("Basic Settings..."),	wxEmptyString, wxITEM_CHECK );
	m_menuVideo.AppendSeparator();

	// Populate options from the plugin here.

	m_menuVideo.Append( Menu_Video_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
void MainEmuFrame::PopulateAudioMenu()
{
	// Populate options from the plugin here.

	m_menuAudio.Append( Menu_Audio_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
void MainEmuFrame::PopulatePadMenu()
{
	// Populate options from the plugin here.

	m_menuPad.Append( Menu_Pad_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
// Close out the console log windows along with the main emu window.
// Note: This event only happens after a close event has occurred and was *not* veto'd.  Ie,
// it means it's time to provide an unconditional closure of said window.
//
void MainEmuFrame::OnCloseWindow(wxCloseEvent& evt)
{
	wxCloseEvent conevt( wxEVT_CLOSE_WINDOW );
	conevt.SetCanVeto( false );	// tells the console to close rather than hide
	wxGetApp().ProgramLog_PostEvent( conevt );
	evt.Skip();
}

void MainEmuFrame::OnMoveAround( wxMoveEvent& evt )
{
	// Uncomment this when going logger stress testing (and them move the window around)
	// ... makes for a good test of the message pump's responsiveness.
	//Console::Notice( "Mess o' crashiness?  It can't be!" );

	// evt.GetPosition() returns the client area position, not the window frame position.
	// So read the window position directly... hope there's no problem with this too. :| --air

	g_Conf->MainGuiPosition = GetPosition();

	// wxGTK note: X sends gratuitous amounts of OnMove messages for various crap actions
	// like selecting or deselecting a window, which muck up docking logic.  We filter them
	// out using 'lastpos' here. :)

	static wxPoint lastpos( wxDefaultCoord, wxDefaultCoord );
	if( lastpos == evt.GetPosition() ) return;
	lastpos = evt.GetPosition();

	if( g_Conf->ConLogBox.AutoDock )
	{
		g_Conf->ConLogBox.DisplayPosition = GetRect().GetTopRight();
		wxCommandEvent conevt( wxEVT_DockConsole );
		wxGetApp().ProgramLog_PostEvent( conevt );
	}

	//evt.Skip();
}

void MainEmuFrame::OnLogBoxHidden()
{
	g_Conf->ConLogBox.Visible = false;
	m_MenuItem_Console.Check( false );
}

// ------------------------------------------------------------------------
void MainEmuFrame::ConnectMenus()
{
	#define ConnectMenu( id, handler ) \
		Connect( id, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainEmuFrame::handler) )

	ConnectMenu( Menu_Config_Settings,	Menu_ConfigSettings_Click );
	ConnectMenu( Menu_IsoBrowse,		Menu_RunIso_Click );
	ConnectMenu( Menu_RunWithoutDisc,	Menu_RunWithoutDisc_Click );

	Connect( wxID_FILE1, wxID_FILE1+20, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainEmuFrame::Menu_IsoRecent_Click) );

	ConnectMenu( Menu_RunELF,			Menu_OpenELF_Click );
	ConnectMenu( Menu_Run_Exit,			Menu_Exit_Click );

	ConnectMenu( Menu_SuspendExec,		Menu_Suspend_Click );
	ConnectMenu( Menu_ResumeExec,		Menu_Resume_Click );
	ConnectMenu( Menu_Reset,			Menu_Reset_Click );

	ConnectMenu( Menu_State_LoadOther,	Menu_LoadStateOther_Click );
	ConnectMenu( Menu_State_SaveOther,	Menu_SaveStateOther_Click );

	ConnectMenu( Menu_Debug_Open,		Menu_Debug_Open_Click );
	ConnectMenu( Menu_Debug_MemoryDump,	Menu_Debug_MemoryDump_Click );
	ConnectMenu( Menu_Debug_Logging,	Menu_Debug_Logging_Click );

	ConnectMenu( Menu_Console,			Menu_ShowConsole );

	ConnectMenu( Menu_About,			Menu_ShowAboutBox );
}

// ------------------------------------------------------------------------
MainEmuFrame::MainEmuFrame(wxWindow* parent, const wxString& title):
    wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER) ),

	m_statusbar( *CreateStatusBar(2, 0) ),
	m_background( this, wxID_ANY, wxGetApp().GetLogoBitmap() ),

	// All menu components must be created on the heap!

	m_menubar( *new wxMenuBar() ),

	m_menuRun( *new wxMenu() ),
	m_menuConfig( *new wxMenu() ),
	m_menuMisc( *new wxMenu() ),

	m_menuVideo( *new wxMenu() ),
	m_menuAudio( *new wxMenu() ),
	m_menuPad( *new wxMenu() ),
	m_menuDebug( *new wxMenu() ),

	m_LoadStatesSubmenu( *MakeStatesSubMenu( Menu_State_Load01 ) ),
	m_SaveStatesSubmenu( *MakeStatesSubMenu( Menu_State_Save01 ) ),

	m_MenuItem_Console( *new wxMenuItem( &m_menuMisc, Menu_Console, L"Show Console", wxEmptyString, wxITEM_CHECK ) )
{
	// ------------------------------------------------------------------------
	// Initial menubar setup.  This needs to be done first so that the menu bar's visible size
	// can be factored into the window size (which ends up being background+status+menus)

	m_menubar.Append( &m_menuRun,		_("Run") );
	m_menubar.Append( &m_menuConfig,	_("Config") );
	m_menubar.Append( &m_menuVideo,		_("Video") );
	m_menubar.Append( &m_menuAudio,		_("Audio") );
	m_menubar.Append( &m_menuPad,		_("Pad") );
	m_menubar.Append( &m_menuMisc,		_("Misc") );
	m_menubar.Append( &m_menuDebug,		_("Debug") );
	SetMenuBar( &m_menubar );

	// ------------------------------------------------------------------------

	wxSize backsize( m_background.GetSize() );

	SetTitle(_("Pcsx2"));

	wxIcon myIcon;
	myIcon.CopyFromBitmap( wxBitmap( EmbeddedImage<png_AppIcon>().Rescale( 32, 32 ) ) );
	SetIcon( myIcon );

	int m_statusbar_widths[] = { (int)(backsize.GetWidth()*0.73), (int)(backsize.GetWidth()*0.25) };
	m_statusbar.SetStatusWidths(2, m_statusbar_widths);
	m_statusbar.SetStatusText( _T("The Status is Good!"), 0);
	m_statusbar.SetStatusText( _T("Good Status"), 1);

	wxBoxSizer& joe( *new wxBoxSizer( wxVERTICAL ) );
	joe.Add( &m_background );
	SetSizerAndFit( &joe );

	// Valid zone for window positioning.
	// Top/Left boundaries are fairly strict, since any more offscreen and the window titlebar
	// would be obscured from being grabbable.

	wxRect screenzone( wxPoint(), wxGetDisplaySize() );

	// Use default window position if the configured windowpos is invalid (partially offscreen)
	if( g_Conf->MainGuiPosition == wxDefaultPosition || !screenzone.Contains( wxRect( g_Conf->MainGuiPosition, GetSize() ) ) )
		g_Conf->MainGuiPosition = GetPosition();
	else
		SetPosition( g_Conf->MainGuiPosition );

	// ------------------------------------------------------------------------
	// Sort out the console log window position (must be done after fitting the window
	// sizer, to ensure correct 'docked mode' positioning).

	g_Conf->ConLogBox.DisplaySize.Set(
		std::min( std::max( g_Conf->ConLogBox.DisplaySize.GetWidth(), 160 ), screenzone.GetWidth() ),
		std::min( std::max( g_Conf->ConLogBox.DisplaySize.GetHeight(), 160 ), screenzone.GetHeight() )
	);

	if( g_Conf->ConLogBox.AutoDock )
	{
		g_Conf->ConLogBox.DisplayPosition = GetPosition() + wxSize( GetSize().x, 0 );
	}
	else if( g_Conf->ConLogBox.DisplayPosition != wxDefaultPosition )
	{
		if( !screenzone.Contains( wxRect( g_Conf->ConLogBox.DisplayPosition, wxSize( 75, 150 ) ) ) )
			g_Conf->ConLogBox.DisplayPosition = wxDefaultPosition;
	}

	// ------------------------------------------------------------------------

	m_menuRun.Append(Menu_RunIso,		_("Run ISO"),				MakeIsoMenu() );
	m_menuRun.Append(Menu_BootCDVD,		_("Run CDVD"),				MakeCdvdMenu() );
	m_menuRun.Append(Menu_RunWithoutDisc,_("Run without Disc"),		_("Use this to access the PS2 system configuration menu"));
	m_menuRun.Append(Menu_RunELF,		_("Run ELF File..."),		_("For running raw PS2 binaries."));

	m_menuRun.AppendSeparator();
	m_menuRun.Append(Menu_SkipBiosToggle,_("Skip Bios on Boot"),	_("Enable this to skip PS2 bootup screens (may hurt compat)"));

	m_menuRun.AppendSeparator();
	m_menuRun.Append(Menu_SuspendExec,	_("Suspend"),	_("Stops emulation dead in its tracks"));
	m_menuRun.Append(Menu_ResumeExec,	_("Resume"),	_("Resumes suspended emulation"));
	m_menuRun.Append(Menu_States,		_("States"),	MakeStatesMenu(), wxEmptyString);
	m_menuRun.Append(Menu_Reset,		_("Reset"),		_("Resets emulation state and reloads plugins"));

	m_menuRun.AppendSeparator();
	m_menuRun.Append(Menu_Run_Exit,		_("Exit"),		_("Closing PCSX2 may be hazardous to your health"));

    // ------------------------------------------------------------------------

	m_menuConfig.Append(Menu_Config_Settings,	_("Core Settings..."),	wxEmptyString);
	m_menuConfig.AppendSeparator();

	// Query installed "tertiary" plugins for name and menu options.
	m_menuConfig.Append(Menu_Config_CDVD,		_("CDVD"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_DEV9,		_("Dev9"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_USB,		_("USB"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_FireWire,	_("Firewire"),	wxEmptyString);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(Menu_Config_Patches,	_("Patches"),		wxEmptyString);

	// ------------------------------------------------------------------------

	PopulateVideoMenu();
	PopulateAudioMenu();
	PopulatePadMenu();

	// ------------------------------------------------------------------------

	m_menuMisc.Append( &m_MenuItem_Console );
	m_menuMisc.Append(Menu_Patches,	_("Enable Patches"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.Append(Menu_Profiler,_("Enable Profiler"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	// Ref will want this re-added eventually.
	//m_menuMisc.Append(47, _T("Print CDVD Info..."), wxEmptyString, wxITEM_CHECK);

	m_menuMisc.Append(Menu_About,		_("About..."), wxEmptyString);
	m_menuMisc.Append(Menu_Website,		_("Pcsx2 Website..."), _("Opens your web-browser!"));

	m_menuDebug.Append(Menu_Debug_Open,			_("Open Debug Window..."), wxEmptyString);
	m_menuDebug.Append(Menu_Debug_MemoryDump,	_("Memory Dump..."), wxEmptyString);
	m_menuDebug.Append(Menu_Debug_Logging,		_("Logging..."), wxEmptyString);

	m_MenuItem_Console.Check( g_Conf->ConLogBox.Visible );

	ConnectMenus();
	Connect( wxEVT_MOVE,			wxMoveEventHandler (MainEmuFrame::OnMoveAround) );
	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(MainEmuFrame::OnCloseWindow) );
}

