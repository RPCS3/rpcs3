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

#include "Resources/EmbeddedImage.h"
#include "Resources/AppIcon.h"

#ifdef _WIN32
#include "svnrev.h"
#endif

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesSubMenu( int baseid ) const
{
	wxMenu* mnuSubstates = new wxMenu();

    for (int i = 0; i < 10; i++)
    {
        mnuSubstates->Append( baseid+i+1, wxsFormat(L"Slot %d", i) );
    }
	mnuSubstates->AppendSeparator();
	mnuSubstates->Append( baseid - 1,	_("Other...") );
	return mnuSubstates;
}

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeCdvdMenu()
{
	wxMenu* mnuCdvd = new wxMenu();
	mnuCdvd->Append( MenuId_Src_Iso,		_("Iso"),		wxEmptyString, wxITEM_RADIO );
	mnuCdvd->Append( MenuId_Src_Plugin,		_("Plugin"),	wxEmptyString, wxITEM_RADIO );
	mnuCdvd->Append( MenuId_Src_NoDisc,		_("No disc"),	wxEmptyString, wxITEM_RADIO );

	mnuCdvd->AppendSeparator();
	mnuCdvd->Append( MenuId_IsoBrowse,		_("Iso Browser..."), _("Select the Iso source image.") );
	mnuCdvd->Append( MenuId_Config_CDVD,	_("Plugin settings..."),
		_("Opens the CDVD plugin configuration dialog") );

	return mnuCdvd;
}

void MainEmuFrame::UpdateIsoSrcSelection()
{
	MenuIdentifiers cdsrc = MenuId_Src_Iso;

	switch( g_Conf->CdvdSource )
	{
		case CDVDsrc_Iso:		cdsrc = MenuId_Src_Iso;		break;
		case CDVDsrc_Plugin:	cdsrc = MenuId_Src_Plugin;	break;
		case CDVDsrc_NoDisc:	cdsrc = MenuId_Src_NoDisc;	break;

		jNO_DEFAULT
	}

	GetMenuBar()->Check( cdsrc, true );
	m_statusbar.SetStatusText( CDVD_SourceLabels[g_Conf->CdvdSource], 1 );
}

void MainEmuFrame::UpdateIsoSrcFile()
{
	UpdateIsoSrcSelection();
	const bool exists = wxFile::Exists( g_Conf->CurrentIso );
	if( !exists )
		g_Conf->CurrentIso.Clear();

	wxString label;
	label.Printf( L"%s -> %s", _("Iso"),
		exists ? g_Conf->CurrentIso.c_str() : _("Empty")
	);
	GetMenuBar()->SetLabel( MenuId_Src_Iso, label );
}

// ------------------------------------------------------------------------
//     Video / Audio / Pad "Extensible" Menus
// ------------------------------------------------------------------------

void MainEmuFrame::PopulateVideoMenu()
{
	m_menuVideo.Append( MenuId_Video_Basics,	_("Basic Settings..."),	wxEmptyString, wxITEM_CHECK );
	m_menuVideo.AppendSeparator();

	// Populate options from the plugin here.

	m_menuVideo.Append( MenuId_Video_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

void MainEmuFrame::PopulateAudioMenu()
{
	// Populate options from the plugin here.

	m_menuAudio.Append( MenuId_Audio_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

void MainEmuFrame::PopulatePadMenu()
{
	// Populate options from the plugin here.

	m_menuPad.Append( MenuId_Pad_Advanced,	_("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

// ------------------------------------------------------------------------
//     MainFrame OnEvent Handlers
// ------------------------------------------------------------------------

// Close out the console log windows along with the main emu window.
// Note: This event only happens after a close event has occurred and was *not* veto'd.  Ie,
// it means it's time to provide an unconditional closure of said window.
//
void MainEmuFrame::OnCloseWindow(wxCloseEvent& evt)
{
	// Note Closure Veting would be handled here (user prompt confirmation
	// of closing the app)

	wxGetApp().PrepForExit();
	evt.Skip();
}

void MainEmuFrame::OnMoveAround( wxMoveEvent& evt )
{
	// Uncomment this when doing logger stress testing (and then move the window around
	// while the logger spams itself)
	// ... makes for a good test of the message pump's responsiveness.
	if( EnableThreadedLoggingTest )
		Console::Notice( "Threaded Logging Test!  (a window move event)" );

	// evt.GetPosition() returns the client area position, not the window frame position.
	// So read the window's screen-relative position directly.
	g_Conf->MainGuiPosition = GetScreenPosition();

	// wxGTK note: X sends gratuitous amounts of OnMove messages for various crap actions
	// like selecting or deselecting a window, which muck up docking logic.  We filter them
	// out using 'lastpos' here. :)

	static wxPoint lastpos( wxDefaultCoord, wxDefaultCoord );
	if( lastpos == evt.GetPosition() ) return;
	lastpos = evt.GetPosition();

	if( g_Conf->ProgLogBox.AutoDock )
	{
		g_Conf->ProgLogBox.DisplayPosition = GetRect().GetTopRight();
		wxGetApp().GetProgramLog()->SetPosition( g_Conf->ProgLogBox.DisplayPosition );
	}

	//evt.Skip();
}

void MainEmuFrame::OnLogBoxHidden()
{
	g_Conf->ProgLogBox.Visible = false;
	m_MenuItem_Console.Check( false );
}

// ------------------------------------------------------------------------
void MainEmuFrame::ConnectMenus()
{
	#define ConnectMenu( id, handler ) \
		Connect( id, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainEmuFrame::handler) )


	#define ConnectMenuRange( id_start, inc, handler ) \
		Connect( id_start, id_start + inc, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainEmuFrame::handler) )

	ConnectMenu( MenuId_Config_Settings,	Menu_ConfigSettings_Click );
	ConnectMenu( MenuId_Config_BIOS,		Menu_SelectBios_Click );

    ConnectMenuRange(wxID_FILE1, 20, Menu_IsoRecent_Click);

    ConnectMenuRange(MenuId_Config_GS, PluginId_Count, Menu_IsoRecent_Click);

    ConnectMenuRange(MenuId_Src_Iso, 3, Menu_CdvdSource_Click);

	ConnectMenu( MenuId_Boot_CDVD,			Menu_BootCdvd_Click );
	ConnectMenu( MenuId_Boot_ELF,			Menu_OpenELF_Click );
	ConnectMenu( MenuId_IsoBrowse,			Menu_IsoBrowse_Click );
	ConnectMenu( MenuId_Exit,				Menu_Exit_Click );

	ConnectMenu( MenuId_Emu_Pause,			Menu_EmuPause_Click );
	ConnectMenu( MenuId_Emu_Close,			Menu_EmuClose_Click );
	ConnectMenu( MenuId_Emu_Reset,			Menu_EmuReset_Click );

	ConnectMenu( MenuId_State_LoadOther,	Menu_LoadStateOther_Click );

    ConnectMenuRange(MenuId_State_Load01+1, 10, Menu_LoadStates_Click);

	ConnectMenu( MenuId_State_SaveOther,	Menu_SaveStateOther_Click );

    ConnectMenuRange(MenuId_State_Save01+1, 10, Menu_SaveStates_Click);

	ConnectMenu( MenuId_Debug_Open,			Menu_Debug_Open_Click );
	ConnectMenu( MenuId_Debug_MemoryDump,	Menu_Debug_MemoryDump_Click );
	ConnectMenu( MenuId_Debug_Logging,		Menu_Debug_Logging_Click );

	ConnectMenu( MenuId_Console,			Menu_ShowConsole );

	ConnectMenu( MenuId_About,				Menu_ShowAboutBox );
}

void MainEmuFrame::InitLogBoxPosition( AppConfig::ConsoleLogOptions& conf )
{
	conf.DisplaySize.Set(
		std::min( std::max( conf.DisplaySize.GetWidth(), 160 ), wxGetDisplayArea().GetWidth() ),
		std::min( std::max( conf.DisplaySize.GetHeight(), 160 ), wxGetDisplayArea().GetHeight() )
	);

	if( conf.AutoDock )
	{
		conf.DisplayPosition = GetScreenPosition() + wxSize( GetSize().x, 0 );
	}
	else if( conf.DisplayPosition != wxDefaultPosition )
	{
		if( !wxGetDisplayArea().Contains( wxRect( conf.DisplayPosition, conf.DisplaySize ) ) )
			conf.DisplayPosition = wxDefaultPosition;
	}
}

// ------------------------------------------------------------------------
MainEmuFrame::MainEmuFrame(wxWindow* parent, const wxString& title):
    wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER) ),

	m_RecentIsoList( new wxFileHistory( g_Conf->RecentFileCount ) ),

	m_statusbar( *CreateStatusBar(2, 0) ),
	m_background( this, wxID_ANY, wxGetApp().GetLogoBitmap() ),

	// All menu components must be created on the heap!

	m_menubar( *new wxMenuBar() ),

	m_menuBoot	( *new wxMenu() ),
	m_menuEmu	( *new wxMenu() ),
	m_menuConfig( *new wxMenu() ),
	m_menuMisc	( *new wxMenu() ),

	m_menuVideo	( *new wxMenu() ),
	m_menuAudio	( *new wxMenu() ),
	m_menuPad	( *new wxMenu() ),
	m_menuDebug	( *new wxMenu() ),

	m_LoadStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Load01 ) ),
	m_SaveStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Save01 ) ),

	m_MenuItem_Console( *new wxMenuItem( &m_menuMisc, MenuId_Console, L"Show Console", wxEmptyString, wxITEM_CHECK ) )
{
	// ------------------------------------------------------------------------
	// Initial menubar setup.  This needs to be done first so that the menu bar's visible size
	// can be factored into the window size (which ends up being background+status+menus)

	m_menubar.Append( &m_menuBoot,		_("Boot") );
	m_menubar.Append( &m_menuEmu,		_("Emulation") );
	m_menubar.Append( &m_menuConfig,	_("Config") );
	m_menubar.Append( &m_menuVideo,		_("Video") );
	m_menubar.Append( &m_menuAudio,		_("Audio") );
	m_menubar.Append( &m_menuMisc,		_("Misc") );
	m_menubar.Append( &m_menuDebug,		_("Debug") );
	SetMenuBar( &m_menubar );

	// ------------------------------------------------------------------------

	wxSize backsize( m_background.GetSize() );

	wxString wintitle;
	if( PCSX2_VersionLo & 1 )
	{
#ifdef __LINUX__
		// Linux isn't set up for svn version numbers yet.
		wintitle.Printf( _("PCSX2  %d.%d.%d (svn)  %s"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			 wxString::FromUTF8(__DATE__).c_str() );
#else
		// Odd versions: beta / development editions, which feature revision number and compile date.
		wintitle.Printf( _("PCSX2  %d.%d.%d.%d%s (svn)  %s"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_REV, SVN_MODS ? L"m" : wxEmptyString, wxString::FromUTF8(__DATE__).c_str() );
#endif
	}
	else
	{
#ifdef __LINUX__
		wintitle.Printf( _("PCSX2  %d.%d.%d"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo);
#else
		// evens: stable releases, with a simpler title.
		wintitle.Printf( _("PCSX2  %d.%d.%d %s"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_MODS ? _("(modded)") : wxEmptyString
		);
#endif
	}

	SetTitle( wintitle );

	wxIcon myIcon;
	myIcon.CopyFromBitmap( wxBitmap( EmbeddedImage<res_AppIcon>().Rescale( 32, 32 ) ) );
	SetIcon( myIcon );

	int m_statusbar_widths[] = { (int)(backsize.GetWidth()*0.73), (int)(backsize.GetWidth()*0.25) };
	m_statusbar.SetStatusWidths(2, m_statusbar_widths);
	m_statusbar.SetStatusText( L"The Status is Good!", 0);
	m_statusbar.SetStatusText( wxEmptyString, 1);

	wxBoxSizer& joe( *new wxBoxSizer( wxVERTICAL ) );
	joe.Add( &m_background );
	SetSizerAndFit( &joe );

	// Use default window position if the configured windowpos is invalid (partially offscreen)
	if( g_Conf->MainGuiPosition == wxDefaultPosition || !pxIsValidWindowPosition( *this, g_Conf->MainGuiPosition) )
		g_Conf->MainGuiPosition = GetScreenPosition();
	else
		SetPosition( g_Conf->MainGuiPosition );

	// Updating console log positions after the main window has been fitted to its sizer ensures
	// proper docked positioning, since the main window's size is invalid until after the sizer
	// has been set/fit.

	InitLogBoxPosition( g_Conf->ProgLogBox );
	InitLogBoxPosition( g_Conf->Ps2ConBox );

	// ------------------------------------------------------------------------

	m_menuBoot.Append(MenuId_Boot_CDVD,		_("Run CDVD"),
		_("Use this to access the PS2 system configuration menu"));

	m_menuBoot.Append(MenuId_Boot_ELF,		_("Run ELF File..."),
		_("For running raw binaries"));

	wxMenu* recentRunMenu = new wxMenu();
	m_menuBoot.Append(MenuId_Boot_Recent,	_("Run Recent"), recentRunMenu);

	m_RecentIsoList->UseMenu( recentRunMenu );
	m_RecentIsoList->AddFilesToMenu( recentRunMenu );

	m_menuBoot.AppendSeparator();
	m_menuBoot.Append(MenuId_Cdvd_Source,	_("Select CDVD source"), MakeCdvdMenu() );
	m_menuBoot.Append(MenuId_SkipBiosToggle,_("BIOS Skip Hack"),
		_("Skips PS2 splash screens when booting from Iso or CDVD media"), wxITEM_CHECK );

	m_menuBoot.AppendSeparator();
	m_menuBoot.Append(MenuId_Exit,			_("Exit"),
		_("Closing PCSX2 may be hazardous to your health"));

	// ------------------------------------------------------------------------
	m_menuEmu.Append(MenuId_Emu_Pause,		_("Pause"),
		_("Stops emulation dead in its tracks"), wxITEM_CHECK );

	m_menuEmu.AppendSeparator();
	m_menuEmu.Append(MenuId_Emu_Reset,		_("Reset"),
		_("Resets emulation state and re-runs current image"));

	m_menuEmu.Append(MenuId_Emu_Close,		_("Close"),
		_("Stops emulation and closes the GS window."));

	m_menuEmu.AppendSeparator();
	m_menuEmu.Append(MenuId_Emu_LoadStates,	_("Load state"), &m_LoadStatesSubmenu);
	m_menuEmu.Append(MenuId_Emu_SaveStates,	_("Save state"), &m_SaveStatesSubmenu);

	m_menuEmu.AppendSeparator();
	m_menuEmu.Append(MenuId_EnablePatches,	_("Enable Patches"),
		wxEmptyString, wxITEM_CHECK);

    // ------------------------------------------------------------------------

	m_menuConfig.Append(MenuId_Config_Settings,	_("General Settings") );
	m_menuConfig.AppendSeparator();

	m_menuConfig.Append(MenuId_Config_CDVD,		_("PAD"),		&m_menuPad );

	// Query installed "tertiary" plugins for name and menu options.
	m_menuConfig.Append(MenuId_Config_CDVD,		_("CDVD"),		wxEmptyString);
	m_menuConfig.Append(MenuId_Config_DEV9,		_("Dev9"),		wxEmptyString);
	m_menuConfig.Append(MenuId_Config_USB,		_("USB"),		wxEmptyString);
	m_menuConfig.Append(MenuId_Config_FireWire,	_("Firewire"),	wxEmptyString);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_Patches,	_("Patches"),	wxEmptyString);
	m_menuConfig.Append(MenuId_Config_BIOS,		_("BIOS") );

	// ------------------------------------------------------------------------

	PopulateVideoMenu();
	PopulateAudioMenu();
	PopulatePadMenu();

	// ------------------------------------------------------------------------

	m_menuMisc.Append( &m_MenuItem_Console );
	m_menuMisc.Append(MenuId_Profiler,			_("Show Profiler"),	wxEmptyString, wxITEM_CHECK);
	m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	// Ref will want this re-added eventually.
	//m_menuMisc.Append(47, _T("Print CDVD Info..."), wxEmptyString, wxITEM_CHECK);

	m_menuMisc.Append(MenuId_Website,			_("Visit Website..."),
		_("Opens your web-browser to our favorite website."));
	m_menuMisc.Append(MenuId_About,				_("About...") );

	m_menuDebug.Append(MenuId_Debug_Open,		_("Open Debug Window..."),	wxEmptyString);
	m_menuDebug.Append(MenuId_Debug_MemoryDump,	_("Memory Dump..."),		wxEmptyString);
	m_menuDebug.Append(MenuId_Debug_Logging,	_("Logging..."),			wxEmptyString);

	m_menuDebug.AppendSeparator();
	m_menuDebug.Append(MenuId_Debug_Usermode,	_("Change Usermode..."),
		_(" Advanced feature for managing multiple concurrent PCSX2 environments."));

	m_MenuItem_Console.Check( g_Conf->ProgLogBox.Visible );

	ConnectMenus();
	Connect( wxEVT_MOVE,			wxMoveEventHandler (MainEmuFrame::OnMoveAround) );
	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(MainEmuFrame::OnCloseWindow) );

	UpdateIsoSrcFile();
}

MainEmuFrame::~MainEmuFrame()
{
	if( m_RecentIsoList != NULL )
	{
		m_RecentIsoList->Save( *wxConfigBase::Get( false ) );
		safe_delete( m_RecentIsoList );
	}
}

void MainEmuFrame::ApplySettings()
{
	// Always perform delete and reload of the Recent Iso List.  This handles cases where
	// the recent file count has been changed, and it's a helluva lot easier than trying
	// to make a clone copy of this complex object. ;)

	wxConfigBase* cfg = wxConfigBase::Get( false );
	wxASSERT( cfg != NULL );

	if( m_RecentIsoList != NULL )
		m_RecentIsoList->Save( *cfg );
	safe_delete( m_RecentIsoList );
	m_RecentIsoList = new wxFileHistory( g_Conf->RecentFileCount );
	m_RecentIsoList->Load( *cfg );

	UpdateIsoSrcFile();
	cfg->Flush();
}
