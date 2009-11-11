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
#include "ConsoleLogger.h"

#include "Dialogs/ModalPopups.h"
#include "IsoDropTarget.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/AppIcon16.h"
#include "Resources/AppIcon32.h"
#include "Resources/AppIcon64.h"

#include <wx/iconbndl.h>

#if _MSC_VER
#	include "svnrev.h"
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
	sMenuBar.Check( cdsrc, true );
	m_statusbar.SetStatusText( CDVD_SourceLabels[g_Conf->CdvdSource], 1 );

	//sMenuBar.SetLabel( MenuId_Src_Iso, wxsFormat( L"%s -> %s", _("Iso"),
	//	exists ? Path::GetFilename(g_Conf->CurrentIso).c_str() : _("Empty") ) );
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
	bool isClosing = false;

	if( !evt.CanVeto() )
	{
		// Mandatory destruction...
		isClosing = true;
	}
	else
	{
		isClosing = wxGetApp().PrepForExit( evt.CanVeto() );
		if( !isClosing ) evt.Veto( true );
	}

	sApp.OnMainFrameClosed();

	evt.Skip();
}

void MainEmuFrame::OnMoveAround( wxMoveEvent& evt )
{
	// Uncomment this when doing logger stress testing (and then move the window around
	// while the logger spams itself)
	// ... makes for a good test of the message pump's responsiveness.
	if( EnableThreadedLoggingTest )
		Console.Warning( "Threaded Logging Test!  (a window move event)" );

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

	ConnectMenu( MenuId_Config_Multitap0Toggle,	Menu_MultitapToggle_Click );
	ConnectMenu( MenuId_Config_Multitap1Toggle,	Menu_MultitapToggle_Click );

	ConnectMenuRange(MenuId_Config_GS, PluginId_Count, Menu_ConfigPlugin_Click);
	ConnectMenuRange(MenuId_Src_Iso, 3, Menu_CdvdSource_Click);

	for( int i=0; i<PluginId_Count; ++i )
		ConnectMenu( MenuId_PluginBase_Settings + (i*PluginMenuId_Interval), Menu_ConfigPlugin_Click);

	ConnectMenu( MenuId_Boot_CDVD,			Menu_BootCdvd_Click );
	ConnectMenu( MenuId_Boot_ELF,			Menu_OpenELF_Click );
	ConnectMenu( MenuId_IsoBrowse,			Menu_IsoBrowse_Click );
	ConnectMenu( MenuId_SkipBiosToggle,		Menu_SkipBiosToggle_Click );
	ConnectMenu( MenuId_EnablePatches,      Menu_EnablePatches_Click );
	ConnectMenu( MenuId_Exit,				Menu_Exit_Click );

	ConnectMenu( MenuId_Sys_SuspendResume,	Menu_SuspendResume_Click );
	ConnectMenu( MenuId_Sys_Reset,			Menu_SysReset_Click );

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

void __evt_fastcall MainEmuFrame::OnCoreThreadStatusChanged( void* obj, wxCommandEvent& evt )
{
	if( obj == NULL ) return;
	MainEmuFrame* mframe = (MainEmuFrame*)obj;
	if( !pxAssertMsg( mframe->GetMenuBar()!=NULL, "Mainframe menu bar is NULL!" ) ) return;

	mframe->ApplyCoreStatus();
}

void __evt_fastcall MainEmuFrame::OnCorePluginStatusChanged( void* obj, PluginEventType& evt )
{
	if( obj == NULL ) return;
	if( (evt != PluginsEvt_Loaded) && (evt != PluginsEvt_Unloaded) ) return;		// everything else we don't care about

	MainEmuFrame& mframe = *(MainEmuFrame*)obj;
	if( !pxAssertMsg( mframe.GetMenuBar()!=NULL, "Mainframe menu bar is NULL!" ) ) return;

	//mframe.ApplyCoreStatus();
	mframe.ApplyPluginStatus();
}

void __evt_fastcall MainEmuFrame::OnSettingsApplied( void* obj, int& evt )
{
	if( obj == NULL ) return;
	MainEmuFrame* mframe = (MainEmuFrame*)obj;
	if( !pxAssertMsg( mframe->GetMenuBar()!=NULL, "Mainframe menu bar is NULL!" ) ) return;

	mframe->ApplySettings();
}

void __evt_fastcall MainEmuFrame::OnSettingsLoadSave( void* obj, IniInterface& evt )
{
	if( obj == NULL ) return;
	//MainEmuFrame* mframe = (MainEmuFrame*)obj;

	// nothing to do here right now.
}

static int GetPluginMenuId_Settings( PluginsEnum_t pid )
{
	return MenuId_PluginBase_Settings + ((int)pid * PluginMenuId_Interval);
}

static int GetPluginMenuId_Name( PluginsEnum_t pid )
{
	return MenuId_PluginBase_Name + ((int)pid * PluginMenuId_Interval);
}

// ------------------------------------------------------------------------
MainEmuFrame::MainEmuFrame(wxWindow* parent, const wxString& title):
    wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER) ),

	m_statusbar( *CreateStatusBar(2, 0) ),
	m_background( this, wxID_ANY, wxGetApp().GetLogoBitmap() ),

	// All menu components must be created on the heap!

	m_menubar( *new wxMenuBar() ),

	m_menuBoot	( *new wxMenu() ),
	m_menuCDVD	( *new wxMenu() ),
	m_menuSys	( *new wxMenu() ),
	m_menuConfig( *new wxMenu() ),
	m_menuMisc	( *new wxMenu() ),
	m_menuDebug	( *new wxMenu() ),

	m_LoadStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Load01 ) ),
	m_SaveStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Save01 ) ),

	m_MenuItem_Console( *new wxMenuItem( &m_menuMisc, MenuId_Console, L"Show Console", wxEmptyString, wxITEM_CHECK ) ),

	m_Listener_CoreThreadStatus( wxGetApp().Source_CoreThreadStatus(), CmdEvt_Listener( this, OnCoreThreadStatusChanged ) ),
	m_Listener_CorePluginStatus( wxGetApp().Source_CorePluginStatus(), EventListener<PluginEventType>( this, OnCorePluginStatusChanged ) ),
	m_Listener_SettingsApplied( wxGetApp().Source_SettingsApplied(), EventListener<int>( this, OnSettingsApplied ) ),
	m_Listener_SettingsLoadSave( wxGetApp().Source_SettingsLoadSave(), EventListener<IniInterface>( this, OnSettingsLoadSave ) )
{
	for( int i=0; i<PluginId_Count; ++i )
		m_PluginMenuPacks[i].Populate( (PluginsEnum_t)i );

	// ------------------------------------------------------------------------
	// Initial menubar setup.  This needs to be done first so that the menu bar's visible size
	// can be factored into the window size (which ends up being background+status+menus)

	m_menubar.Append( &m_menuBoot,		_("&Boot") );
	m_menubar.Append( &m_menuCDVD,		_("CD&VD") );
	m_menubar.Append( &m_menuSys,		_("&System") );
	m_menubar.Append( &m_menuConfig,	_("&Config") );
	m_menubar.Append( &m_menuMisc,		_("&Misc") );
#ifdef PCSX2_DEVBUILD
	m_menubar.Append( &m_menuDebug,		_("&Debug") );
#endif
	SetMenuBar( &m_menubar );

	// ------------------------------------------------------------------------

	wxSize backsize( m_background.GetSize() );

	wxString wintitle;
	if( PCSX2_VersionLo & 1 )
	{
		// Odd versions: beta / development editions, which feature revision number and compile date.
		wintitle.Printf( _("PCSX2  %d.%d.%d.%d%s (svn)  %s"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_REV, SVN_MODS ? L"m" : wxEmptyString, fromUTF8(__DATE__).c_str() );
	}
	else
	{
		// evens: stable releases, with a simpler title.
		wintitle.Printf( _("PCSX2  %d.%d.%d %s"), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_MODS ? _("(modded)") : wxEmptyString
		);
	}

	SetTitle( wintitle );

	// Ideally the __WXMSW__ port should use the embedded IDI_ICON2 icon, because wxWidgets sucks and
	// loses the transparency information when loading bitmaps into icons.  But for some reason
	// I cannot get it to work despite following various examples to the letter.

	wxIconBundle bundle;
	bundle.AddIcon( EmbeddedImage<res_AppIcon32>().GetIcon() );
	bundle.AddIcon( EmbeddedImage<res_AppIcon64>().GetIcon() );
	bundle.AddIcon( EmbeddedImage<res_AppIcon16>().GetIcon() );
	SetIcons( bundle );

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

	m_menuBoot.AppendSeparator();
	m_menuBoot.Append(MenuId_Exit,			_("Exit"),
		_("Closing PCSX2 may be hazardous to your health"));

	// ------------------------------------------------------------------------
	wxMenu& isoRecents( wxGetApp().GetRecentIsoMenu() );

	//m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append( MenuId_IsoSelector,	_("Iso Selector"), &isoRecents );
	m_menuCDVD.Append( GetPluginMenuId_Settings(PluginId_CDVD), _("Plugin Menu"), m_PluginMenuPacks[PluginId_CDVD] );

	m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append( MenuId_Src_Iso,		_("Iso"),		_("Makes the specified ISO image the CDVD source."), wxITEM_RADIO );
	m_menuCDVD.Append( MenuId_Src_Plugin,	_("Plugin"),	_("Uses an external plugin as the CDVD source."), wxITEM_RADIO );
	m_menuCDVD.Append( MenuId_Src_NoDisc,	_("No disc"),	_("Use this to boot into your virtual PS2's BIOS configuration."), wxITEM_RADIO );

	m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append( MenuId_SkipBiosToggle,_("Enable Skip BIOS Hack"),
		_("Skips PS2 splash screens when booting from Iso or CDVD media"), wxITEM_CHECK );

	// ------------------------------------------------------------------------
	m_menuSys.Append(MenuId_Sys_SuspendResume,		_("Suspend") )->Enable( SysHasValidState() );

	m_menuSys.AppendSeparator();

	//m_menuSys.Append(MenuId_Sys_Close,		_("Close"),
	//	_("Stops emulation and closes the GS window."));

	m_menuSys.Append(MenuId_Sys_LoadStates,	_("Load state"), &m_LoadStatesSubmenu);
	m_menuSys.Append(MenuId_Sys_SaveStates,	_("Save state"), &m_SaveStatesSubmenu);

	m_menuSys.AppendSeparator();
	
	m_menuSys.Append(MenuId_EnablePatches,	_("Enable Patches"),
		wxEmptyString, wxITEM_CHECK);

	m_menuSys.AppendSeparator();
	m_menuSys.Append(MenuId_Sys_Reset,		_("Reset"),
		_("Resets emulation state and re-runs current image"));

    // ------------------------------------------------------------------------

	m_menuConfig.Append(MenuId_Config_Settings,	_("General &Settings") );
	m_menuConfig.AppendSeparator();


	m_menuConfig.Append(MenuId_Config_GS,		_("Video (GS)"),		m_PluginMenuPacks[PluginId_GS]);
	m_menuConfig.Append(MenuId_Config_SPU2,		_("Audio (SPU2)"),		m_PluginMenuPacks[PluginId_SPU2]);
	m_menuConfig.Append(MenuId_Config_PAD,		_("Controllers (PAD)"),	m_PluginMenuPacks[PluginId_PAD]);
	m_menuConfig.Append(MenuId_Config_DEV9,		_("Dev9"),				m_PluginMenuPacks[PluginId_DEV9]);
	m_menuConfig.Append(MenuId_Config_USB,		_("USB"),				m_PluginMenuPacks[PluginId_USB]);
	m_menuConfig.Append(MenuId_Config_FireWire,	_("Firewire"),			m_PluginMenuPacks[PluginId_FW]);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_Patches,	_("Patches"),	wxEmptyString);
	m_menuConfig.Append(MenuId_Config_BIOS,		_("BIOS") );

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_Multitap0Toggle,	_("Multitap 1"),	wxEmptyString, wxITEM_CHECK );
	m_menuConfig.Append(MenuId_Config_Multitap1Toggle,	_("Multitap 2"),	wxEmptyString, wxITEM_CHECK );

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_ResetAll,	_("Reset all..."),
		_("Clears all PCSX2 settings and re-runs the startup wizard."));

	// ------------------------------------------------------------------------

	m_menuMisc.Append( &m_MenuItem_Console );
	//Todo: Though not many people need this one :p
	//m_menuMisc.Append(MenuId_Profiler,			_("Show Profiler"),	wxEmptyString, wxITEM_CHECK);
	m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	// Ref will want this re-added eventually.
	//m_menuMisc.Append(47, _T("Print CDVD Info..."), wxEmptyString, wxITEM_CHECK);

	//Todo:
	//There's a great working "open website" in the about panel. Less clutter by just using that.
	//m_menuMisc.Append(MenuId_Website,			_("Visit Website..."),
	//	_("Opens your web-browser to our favorite website."));
	m_menuMisc.Append(MenuId_About,				_("About...") );
#ifdef PCSX2_DEVBUILD
	m_menuDebug.Append(MenuId_Debug_Open,		_("Open Debug Window..."),	wxEmptyString);
	m_menuDebug.Append(MenuId_Debug_MemoryDump,	_("Memory Dump..."),		wxEmptyString);
	m_menuDebug.Append(MenuId_Debug_Logging,	_("Logging..."),			wxEmptyString);
#endif
	m_MenuItem_Console.Check( g_Conf->ProgLogBox.Visible );

	ConnectMenus();
	Connect( wxEVT_MOVE,			wxMoveEventHandler (MainEmuFrame::OnMoveAround) );
	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(MainEmuFrame::OnCloseWindow) );

	SetDropTarget( new IsoDropTarget( this ) );
}

MainEmuFrame::~MainEmuFrame() throw()
{
	m_menuCDVD.Remove( MenuId_IsoSelector );
}

// This should be called whenever major changes to the ini configs have occurred,
// or when the recent file count mismatches the max filecount.
void MainEmuFrame::ReloadRecentLists()
{
}

void MainEmuFrame::ApplyCoreStatus()
{
	wxMenuBar& menubar( *GetMenuBar() );

	wxMenuItem& susres( *menubar.FindItem( MenuId_Sys_SuspendResume ) );
	if( !pxAssertMsg( &susres!=NULL, "Suspend/Resume Menubar Item is NULL!" ) ) return;

	if( SysHasValidState() )
	{
		susres.Enable();
		if( CoreThread.IsOpen() )
		{
			susres.SetHelp( _("Safely pauses emulation and preserves the PS2 state.") );
			susres.SetText( _("Suspend") );
		}
		else
		{
			susres.SetHelp( _("Resumes the suspended emulation state.") );
			susres.SetText( _("Resume") );
		}
	}
	else
	{
		susres.Enable( false );
		susres.SetHelp( _("No emulation state is active; cannot suspend or resume.") );
	}

	menubar.Enable( MenuId_Sys_Reset, SysHasValidState() || (g_plugins!=NULL) );
}

void MainEmuFrame::ApplyPluginStatus()
{
	if( g_plugins == NULL )
	{
		for( int i=0; i<PluginId_Count; ++i )
			m_PluginMenuPacks[i].OnUnloaded();
	}
	else
	{
		for( int i=0; i<PluginId_Count; ++i )
			m_PluginMenuPacks[i].OnLoaded();

		// bleh this makes the menu too cluttered. --air
		//m_menuCDVD.SetLabel( MenuId_Src_Plugin, wxsFormat( L"%s (%s)", _("Plugin"),
		//	g_plugins->GetName( PluginId_CDVD ).c_str() ) );
	}
}

void MainEmuFrame::ApplySettings()
{
	wxMenuBar& menubar( *GetMenuBar() );

	menubar.Check( MenuId_SkipBiosToggle, g_Conf->EmuOptions.SkipBiosSplash );
	menubar.Check( MenuId_EnablePatches, g_Conf->EmuOptions.EnablePatches );

	menubar.Check( MenuId_Config_Multitap0Toggle, g_Conf->EmuOptions.MultitapPort0_Enabled );
	menubar.Check( MenuId_Config_Multitap1Toggle, g_Conf->EmuOptions.MultitapPort1_Enabled );

	UpdateIsoSrcSelection();
}

// ------------------------------------------------------------------------
//   "Extensible" Plugin Menus
// ------------------------------------------------------------------------

PerPluginMenuInfo::~PerPluginMenuInfo() throw()
{
}

void PerPluginMenuInfo::Populate( PluginsEnum_t pid )
{
	if( !pxAssert(pid < PluginId_Count) ) return;

	PluginId = pid;

	MyMenu.Append( GetPluginMenuId_Name(PluginId), _("No plugin loaded") )->Enable( false );
	MyMenu.AppendSeparator();

	if( PluginId == PluginId_GS )
	{
		MyMenu.Append( MenuId_Video_CoreSettings, _("Core Settings..."),
			_("Modify video emulation settings regulated by the PCSX2 core virtual machine."), wxITEM_CHECK );
		MyMenu.AppendSeparator();
	}

	// Populate options from the plugin here.

	MyMenu.Append( GetPluginMenuId_Settings(PluginId), _("Plugin Settings..."),
		wxsFormat( _("Opens the %s plugin's advanced settings dialog."), tbl_PluginInfo[pid].GetShortname().c_str() )
	);
}

// deletes menu items belonging to (created by) the plugin.  Leaves menu items created
// by the PCSX2 core intact.
void PerPluginMenuInfo::OnUnloaded()
{
	// Delete any menu options added by plugins (typically a plugin will have already
	// done its own proper cleanup when the plugin was shutdown or unloaded, but lets
	// not trust them, shall we?)

	MenuItemAddonList& curlist( m_PluginMenuItems );
	for( uint mx=0; mx<curlist.size(); ++mx )
		MyMenu.Delete( curlist[mx].Item );

	curlist.clear();

	MyMenu.SetLabel( GetPluginMenuId_Name(PluginId), _("No plugin loaded") );
	MyMenu.Enable( GetPluginMenuId_Settings(PluginId), false );
}

void PerPluginMenuInfo::OnLoaded()
{
	MyMenu.SetLabel( GetPluginMenuId_Name(PluginId),
		g_plugins->GetName( PluginId ) + L" " + g_plugins->GetVersion( PluginId )
	);
	MyMenu.Enable( GetPluginMenuId_Settings(PluginId), true );
}

