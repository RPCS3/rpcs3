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

#include "PrecompiledHeader.h"
#include "MainFrame.h"
#include "Dialogs/LogOptionsDialog.h"
#include "Dialogs/ModalPopups.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/AppIcon.h"

#include "Dialogs/ConfigurationDialog.h"

using namespace Dialogs;

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeLanguagesMenu() const
{
	wxMenu* menuLangs = new wxMenu();

	// TODO : Index languages out of the resources and Langs folder.

	menuLangs->Append(Menu_Language_Start, _T("English"), wxEmptyString, wxITEM_RADIO);

	return menuLangs;
}

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesMenu()
{
	wxMenu* mnuStates = new wxMenu();

	m_LoadStatesSubmenu.Append( Menu_State_LoadOther, _T("Other..."), wxEmptyString, wxITEM_NORMAL );
	m_SaveStatesSubmenu.Append( Menu_State_SaveOther, _T("Other..."), wxEmptyString, wxITEM_NORMAL );

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
	//mnuIso->AppendSeparator();

	// Add in the recent files!

	/*const StringListNode* cruise = g_Conf->RecentIsos;
	
	int i = 0;
	int threshold = 15;
	while( cruise != NULL && (--threshold >= 0) )
	{
		wxString ellipsized;

		if( cruise->item->Length() > 64 )
		{
			// Ellipsize it!
			wxFileName src( *cruise->item );
			ellipsized = src.GetVolume() + wxFileName::GetVolumeSeparator() + wxFileName::GetPathSeparator() + L"...";
			
			const wxArrayString& dirs( src.GetDirs() );
			int totalLen = ellipsized.Length();
			int i=dirs.Count()-1;

			for( ; i; --i )
			{
				if( totalLen + dirs[i].Length() < 56 )
					totalLen += dirs[i];
			}

			for( ; i<dirs.Count(); ++i )
				ellipsized += wxFileName::GetPathSeparator() + dirs[i];

			ellipsized += wxFileName::GetPathSeparator() + src.GetFullName();
		}
		else
			ellipsized = *cruise->item;

		mnuIso->Append( Menu_Iso_Recent+i, Path::GetFilename( ellipsized ), *cruise->item );
	}*/
	
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
	m_menuVideo.Append( Menu_Video_Basics,	_T("Basic Settings..."),	wxEmptyString, wxITEM_CHECK );
	m_menuVideo.AppendSeparator();

	// Populate options from the plugin here.

	m_menuVideo.Append( Menu_Video_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
void MainEmuFrame::PopulateAudioMenu()
{
	// Populate options from the plugin here.

	m_menuAudio.Append( Menu_Audio_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
void MainEmuFrame::PopulatePadMenu()
{
	// Populate options from the plugin here.

	m_menuPad.Append( Menu_Pad_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
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
	if( g_Conf->ConLogBox.AutoDock )
	{
		g_Conf->ConLogBox.DisplayPosition = GetRect().GetTopRight();

		// Send the move event our window ID, which allows the logbox to know that this
		// move event comes from us, and needs a special handler.
		wxCommandEvent evt( wxEVT_DockConsole );
		wxGetApp().ConsoleLog_PostEvent( evt );
	}
	
	g_Conf->MainGuiPosition = GetPosition();

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
	ConnectMenu( Menu_RunWithoutDisc,	Menu_RunWithoutDisc_Click );
	
	ConnectMenu( Menu_IsoBrowse,		Menu_IsoBrowse_Click );

	Connect( wxID_FILE1, wxID_FILE1+20, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainEmuFrame::Menu_IsoRecent_Click) );

	ConnectMenu( Menu_RunELF,			Menu_OpenELF_Click );
	ConnectMenu( Menu_Run_Exit,			Menu_Exit_Click );

	ConnectMenu( Menu_SuspendExec,		Menu_Suspend_Click );
	ConnectMenu( Menu_ResumeExec,		Menu_Resume_Click );
	ConnectMenu( Menu_Reset,			Menu_Reset_Click );

	ConnectMenu( Menu_State_LoadOther,	Menu_LoadStateOther_Click );
	ConnectMenu( Menu_State_SaveOther,	Menu_SaveStateOther_Click );

	ConnectMenu( Menu_Config_Gamefixes,	Menu_Gamefixes_Click );
	ConnectMenu( Menu_Config_SpeedHacks,Menu_Speedhacks_Click );


	ConnectMenu( Menu_Debug_Open,		Menu_Debug_Open_Click );
	ConnectMenu( Menu_Debug_MemoryDump,	Menu_Debug_MemoryDump_Click );
	ConnectMenu( Menu_Debug_Logging,	Menu_Debug_Logging_Click );

	ConnectMenu( Menu_Console,			Menu_ShowConsole );

	ConnectMenu( Menu_About,			Menu_ShowAboutBox );
}

// ------------------------------------------------------------------------
MainEmuFrame::MainEmuFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxFrame(parent, id, title, pos, size, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER) ),

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

	m_menuRun.Append(Menu_BootIso,		_("Run ISO"),				MakeIsoMenu(),	_("Performs a complete bootup sequence (recommended for best compat)"));
	m_menuRun.Append(Menu_BootIsoFast,	_("Run ISO (skip Bios)"),	MakeIsoMenu(),	_("Skips PS2 startup screens when booting; may cause compat issues"));
	m_menuRun.Append(Menu_BootQuickCDVD,_("Run CDVD"),				MakeCdvdMenu(),	_("Skips PS2 init screens when running cdvd images"));
	m_menuRun.Append(Menu_BootFullCDVD,	_("Run CDVD (skip Bios)"),	MakeCdvdMenu(),	_("Skips PS2 startup screens when booting; may cause compat issues"));
	m_menuRun.Append(Menu_RunWithoutDisc,_("Run without Disc"),						_("Use this to access the PS2 system configuration menu"));
	m_menuRun.Append(Menu_RunELF,		_("Run ELF File..."),		wxEmptyString);
	m_menuRun.AppendSeparator();

	m_menuRun.Append(Menu_SuspendExec,	_("Suspend"),	_T("Stops emulation dead in its tracks"));
	m_menuRun.Append(Menu_ResumeExec,	_("Resume"),	_T("Resumes suspended emulation"));
	m_menuRun.Append(Menu_States,		_("States"),	MakeStatesMenu(), wxEmptyString);
	m_menuRun.Append(Menu_Reset,		_("Reset"),		_T("Resets emulation state and reloads plugins"));

	m_menuRun.AppendSeparator();
	m_menuRun.Append(Menu_Run_Exit,		_("Exit"),		_T("Closing PCSX2 may be hazardous to your health"));

    // ------------------------------------------------------------------------

	m_menuConfig.Append(Menu_Config_Settings,	_("Settings..."),	wxEmptyString);
	m_menuConfig.AppendSeparator();

	// Query installed "tertiary" plugins for name and menu options.
	m_menuConfig.Append(Menu_Config_CDVD,		_("CDVD"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_DEV9,		_("Dev9"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_USB,		_("USB"),		wxEmptyString);
	m_menuConfig.Append(Menu_Config_FireWire,	_("Firewire"),	wxEmptyString);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(Menu_SelectPlugins,		_("Plugin Selector..."), wxEmptyString);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(Menu_Config_Memcards,	_("Memcards"),		_T("Memory card file locations and options"));
	m_menuConfig.Append(Menu_Config_Gamefixes,	_("Gamefixes"),		_T("God of War and TriAce fixes are found here"));
	m_menuConfig.Append(Menu_Config_SpeedHacks,	_("Speed Hacks"),	_T("Options to make Pcsx2 emulate faster, but less accurately"));
	m_menuConfig.Append(Menu_Config_Patches,	_("Patches"),		wxEmptyString);
	//m_menuConfig.Append(Menu_Config_Advanced,	_("Advanced"),		_T("Cpu, Fpu, and Recompiler options."), wxITEM_NORMAL);

	// ------------------------------------------------------------------------

	PopulateVideoMenu();
	PopulateAudioMenu();
	PopulatePadMenu();

	// ------------------------------------------------------------------------

	m_menuMisc.Append( &m_MenuItem_Console );
	m_menuMisc.Append(Menu_Patches,	_T("Enable Patches"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.Append(Menu_Profiler,_T("Enable Profiler"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	// Ref will want this re-added eventually.
	//m_menuMisc.Append(47, _T("Print CDVD Info..."), wxEmptyString, wxITEM_CHECK);

	m_menuMisc.Append(Menu_Languages,	_T("Languages"),	MakeLanguagesMenu(), wxEmptyString);
	m_menuMisc.AppendSeparator();

	m_menuMisc.Append(Menu_About,		_T("About..."), wxEmptyString, wxITEM_NORMAL);
	m_menuMisc.Append(Menu_Website,		_T("Pcsx2 Website..."), _T("Opens your web-browser!"), wxITEM_NORMAL);


	m_menuDebug.Append(Menu_Debug_Open,			_T("Open Debug Window..."), wxEmptyString, wxITEM_NORMAL);
	m_menuDebug.Append(Menu_Debug_MemoryDump,	_T("Memory Dump..."), wxEmptyString, wxITEM_NORMAL);
	m_menuDebug.Append(Menu_Debug_Logging,		_T("Logging..."), wxEmptyString, wxITEM_NORMAL);

	m_MenuItem_Console.Check( g_Conf->ConLogBox.Visible );
	
	ConnectMenus();
	Connect( wxEVT_MOVE,			wxMoveEventHandler (MainEmuFrame::OnMoveAround) );
	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(MainEmuFrame::OnCloseWindow) );
}

void MainEmuFrame::Menu_ConfigSettings_Click(wxCommandEvent &event)
{
	Dialogs::ConfigurationDialog( this ).ShowModal();
}

void MainEmuFrame::Menu_RunWithoutDisc_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_IsoBrowse_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_IsoRecent_Click(wxCommandEvent &event)
{
	Console::Status( "%d", params event.GetId() - g_RecentIsoList->GetBaseId() );
	Console::WriteLn( Color_Magenta, g_RecentIsoList->GetHistoryFile( event.GetId() - g_RecentIsoList->GetBaseId() ) );
}

void MainEmuFrame::Menu_OpenELF_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_LoadStateOther_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_SaveStateOther_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Exit_Click(wxCommandEvent &event)
{
	Close();
}

void MainEmuFrame::Menu_Suspend_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Resume_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Reset_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Gamefixes_Click( wxCommandEvent& event )
{
	//GameFixesDialog( this, wxID_ANY ).ShowModal();
}

void MainEmuFrame::Menu_Speedhacks_Click( wxCommandEvent& event )
{
	//SpeedHacksDialog( this, wxID_ANY ).ShowModal();
}

void MainEmuFrame::Menu_Debug_Open_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_MemoryDump_Click(wxCommandEvent &event)
{
}

void MainEmuFrame::Menu_Debug_Logging_Click(wxCommandEvent &event)
{
	LogOptionsDialog( this, wxID_ANY ).ShowModal();
}

void MainEmuFrame::Menu_ShowConsole(wxCommandEvent &event)
{
	// Use messages to relay open/close commands (thread-safe)
	
	g_Conf->ConLogBox.Visible = event.IsChecked();
	wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, g_Conf->ConLogBox.Visible ? wxID_OPEN : wxID_CLOSE );
	wxGetApp().ProgramLog_PostEvent( evt );
}

void MainEmuFrame::Menu_ShowAboutBox(wxCommandEvent &event)
{
	AboutBoxDialog( this, wxID_ANY ).ShowModal();
}
