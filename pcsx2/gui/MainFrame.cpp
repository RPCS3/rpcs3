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
#include "MainFrame.h"
#include "AppSaveStates.h"
#include "ConsoleLogger.h"
#include "MSWstuff.h"

#include "Dialogs/ModalPopups.h"
#include "IsoDropTarget.h"

#include <wx/iconbndl.h>

#if _MSC_VER || defined(LINUX_PRINT_SVN_NUMBER)
#	include "svnrev.h"
#endif

// ------------------------------------------------------------------------
wxMenu* MainEmuFrame::MakeStatesSubMenu( int baseid, int loadBackupId ) const
{
	wxMenu* mnuSubstates = new wxMenu();

	for (int i = 0; i < 10; i++)
	{
		mnuSubstates->Append( baseid+i+1, wxsFormat(_("Slot %d"), i) );
	}
	if( loadBackupId>=0 )
	{
		mnuSubstates->AppendSeparator();

		wxMenuItem* m = mnuSubstates->Append( loadBackupId,	_("Backup") );
		m->Enable( false );
		States_registerLoadBackupMenuItem( m );
	}

	//mnuSubstates->Append( baseid - 1,	_("Other...") );
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

bool MainEmuFrame::Destroy()
{
	// Sigh: wxWidgets doesn't issue Destroy() calls for children windows when the parent
	// is destroyed (it just deletes them, quite suddenly).  So let's do it for them, since
	// our children have configuration stuff they like to do when they're closing.

	for (
		wxWindowList::const_iterator
			i	= wxTopLevelWindows.begin(),
			end = wxTopLevelWindows.end();
		i != end; ++i
		)
	{
		wxTopLevelWindow * const win = wx_static_cast(wxTopLevelWindow *, *i);
		if (win == this) continue;
		if (win->GetParent() != this) continue;

		win->Destroy();
	}
	
	return _parent::Destroy();
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
	if( IsBeingDeleted() ) return;

	CoreThread.Suspend();

	bool isClosing = false;

	if( !evt.CanVeto() )
	{
		// Mandatory destruction...
		isClosing = true;
	}
	else
	{
		// TODO : Add confirmation prior to exit here!
		// Problem: Suspend is often slow because it needs to wait until the current EE frame
		// has finished processing (if the GS or logging has incurred severe overhead this makes
		// closing PCSX2 difficult).  A non-blocking suspend with modal dialog might suffice
		// however. --air

		//evt.Veto( true );

	}

	sApp.OnMainFrameClosed( GetId() );

	if( m_menubar.FindItem(MenuId_IsoSelector) )
		m_menuCDVD.Remove(MenuId_IsoSelector);

	RemoveEventHandler( &wxGetApp().GetRecentIsoManager() );
	wxGetApp().PostIdleAppMethod( &Pcsx2App::PrepForExit );

	evt.Skip();
}

void MainEmuFrame::OnMoveAround( wxMoveEvent& evt )
{
	if( IsBeingDeleted() || !IsVisible() || IsIconized() ) return;

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
		if( ConsoleLogFrame* proglog = wxGetApp().GetProgramLog() )
			proglog->SetPosition( g_Conf->ProgLogBox.DisplayPosition );
	}

	evt.Skip();
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

	ConnectMenu( MenuId_Config_SysSettings,	Menu_SysSettings_Click );
	ConnectMenu( MenuId_Config_McdSettings,	Menu_McdSettings_Click );
	ConnectMenu( MenuId_Config_AppSettings,	Menu_WindowSettings_Click );
	ConnectMenu( MenuId_Config_GameDatabase,Menu_GameDatabase_Click );
	ConnectMenu( MenuId_Config_BIOS,		Menu_SelectPluginsBios_Click );
	ConnectMenu( MenuId_Config_Language,	Menu_Language_Click );
	ConnectMenu( MenuId_Config_ResetAll,	Menu_ResetAllSettings_Click );

	ConnectMenu( MenuId_Config_Multitap0Toggle,	Menu_MultitapToggle_Click );
	ConnectMenu( MenuId_Config_Multitap1Toggle,	Menu_MultitapToggle_Click );

	ConnectMenu( MenuId_Video_WindowSettings,	Menu_WindowSettings_Click );
	ConnectMenu( MenuId_Video_CoreSettings,		Menu_GSSettings_Click );


	ConnectMenuRange(MenuId_Config_GS, PluginId_Count, Menu_ConfigPlugin_Click);
	ConnectMenuRange(MenuId_Src_Iso, 3, Menu_CdvdSource_Click);

	for( int i=0; i<PluginId_Count; ++i )
		ConnectMenu( MenuId_PluginBase_Settings + (i*PluginMenuId_Interval), Menu_ConfigPlugin_Click);

	ConnectMenu( MenuId_Boot_CDVD,			Menu_BootCdvd_Click );
	ConnectMenu( MenuId_Boot_CDVD2,			Menu_BootCdvd2_Click );
	ConnectMenu( MenuId_Boot_ELF,			Menu_OpenELF_Click );
	ConnectMenu( MenuId_IsoBrowse,			Menu_IsoBrowse_Click );
	ConnectMenu( MenuId_EnableBackupStates, Menu_EnableBackupStates_Click );
	ConnectMenu( MenuId_EnablePatches,		Menu_EnablePatches_Click );
	ConnectMenu( MenuId_EnableCheats,		Menu_EnableCheats_Click );
	ConnectMenu( MenuId_EnableHostFs,		Menu_EnableHostFs_Click );
	ConnectMenu( MenuId_Exit,				Menu_Exit_Click );

	ConnectMenu( MenuId_Sys_SuspendResume,	Menu_SuspendResume_Click );
	ConnectMenu( MenuId_Sys_Restart,		Menu_SysReset_Click );
	ConnectMenu( MenuId_Sys_Shutdown,		Menu_SysShutdown_Click );

	//ConnectMenu( MenuId_State_LoadOther,	Menu_LoadStateOther_Click );

	ConnectMenuRange(MenuId_State_Load01+1, 10, Menu_LoadStates_Click);
	ConnectMenu( MenuId_State_LoadBackup,	Menu_LoadStates_Click );
	

	//ConnectMenu( MenuId_State_SaveOther,	Menu_SaveStateOther_Click );

	ConnectMenuRange(MenuId_State_Save01+1, 10, Menu_SaveStates_Click);

	ConnectMenu( MenuId_Debug_Open,			Menu_Debug_Open_Click );
	ConnectMenu( MenuId_Debug_MemoryDump,	Menu_Debug_MemoryDump_Click );
	ConnectMenu( MenuId_Debug_Logging,		Menu_Debug_Logging_Click );

	ConnectMenu( MenuId_Console,			Menu_ShowConsole );
	ConnectMenu( MenuId_Console_Stdio,		Menu_ShowConsole_Stdio );

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

void MainEmuFrame::DispatchEvent( const PluginEventType& plugin_evt )
{
	if( !pxAssertMsg( GetMenuBar()!=NULL, "Mainframe menu bar is NULL!" ) ) return;

	//ApplyCoreStatus();

	if( plugin_evt == CorePlugins_Unloaded )
	{
		for( int i=0; i<PluginId_Count; ++i )
			m_PluginMenuPacks[i].OnUnloaded();
	}
	else if( plugin_evt == CorePlugins_Loaded )
	{
		for( int i=0; i<PluginId_Count; ++i )
			m_PluginMenuPacks[i].OnLoaded();

		// bleh this makes the menu too cluttered. --air
		//m_menuCDVD.SetLabel( MenuId_Src_Plugin, wxsFormat( L"%s (%s)", _("Plugin"),
		//	GetCorePlugins().GetName( PluginId_CDVD ).c_str() ) );
	}
}

void MainEmuFrame::DispatchEvent( const CoreThreadStatus& status )
{
	if( !pxAssertMsg( GetMenuBar()!=NULL, "Mainframe menu bar is NULL!" ) ) return;
	ApplyCoreStatus();
}

void MainEmuFrame::AppStatusEvent_OnSettingsApplied()
{
	ApplySettings();
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
MainEmuFrame::MainEmuFrame(wxWindow* parent, const wxString& title)
	: wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER) )

	, m_statusbar( *CreateStatusBar(2, 0) )
	, m_background( this, wxID_ANY, wxGetApp().GetLogoBitmap() )

	// All menu components must be created on the heap!

	, m_menubar( *new wxMenuBar() )

	, m_menuCDVD	( *new wxMenu() )
	, m_menuSys		( *new wxMenu() )
	, m_menuConfig	( *new wxMenu() )
	, m_menuMisc	( *new wxMenu() )
	, m_menuDebug	( *new wxMenu() )

	, m_LoadStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Load01, MenuId_State_LoadBackup ) )
	, m_SaveStatesSubmenu( *MakeStatesSubMenu( MenuId_State_Save01 ) )

	, m_MenuItem_Console( *new wxMenuItem( &m_menuMisc, MenuId_Console, _("Show Console"), wxEmptyString, wxITEM_CHECK ) )
	, m_MenuItem_Console_Stdio( *new wxMenuItem( &m_menuMisc, MenuId_Console_Stdio, _("Console to Stdio"), wxEmptyString, wxITEM_CHECK ) )

{
	m_RestartEmuOnDelete = false;

	for( int i=0; i<PluginId_Count; ++i )
		m_PluginMenuPacks[i].Populate( (PluginsEnum_t)i );

	// ------------------------------------------------------------------------
	// Initial menubar setup.  This needs to be done first so that the menu bar's visible size
	// can be factored into the window size (which ends up being background+status+menus)

	//m_menubar.Append( &m_menuBoot,		_("&Boot") );
	m_menubar.Append( &m_menuSys,		_("&System") );
	m_menubar.Append( &m_menuCDVD,		_("CD&VD") );
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
		wintitle.Printf( _("%s  %d.%d.%d.%d%s (svn)  %s"), pxGetAppName().c_str(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_REV, SVN_MODS ? L"m" : wxEmptyString, fromUTF8(__DATE__).c_str() );
	}
	else
	{
		// evens: stable releases, with a simpler title.
		wintitle.Printf( _("%s  %d.%d.%d %s"), pxGetAppName().c_str(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_MODS ? _("(modded)") : wxEmptyString
		);
	}

	SetTitle( wintitle );

	// Ideally the __WXMSW__ port should use the embedded IDI_ICON2 icon, because wxWidgets sucks and
	// loses the transparency information when loading bitmaps into icons.  But for some reason
	// I cannot get it to work despite following various examples to the letter.


	SetIcons( wxGetApp().GetIconBundle() );

	int m_statusbar_widths[] = { (int)(backsize.GetWidth()*0.73), (int)(backsize.GetWidth()*0.25) };
	m_statusbar.SetStatusWidths(2, m_statusbar_widths);
	//m_statusbar.SetStatusText( L"The Status is Good!", 0);
	m_statusbar.SetStatusText( wxEmptyString, 0);

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

	// ------------------------------------------------------------------------
	// Some of the items in the System menu are configured by the UpdateCoreStatus() method.
	
	m_menuSys.Append(MenuId_Boot_CDVD,		_("Initializing..."));

	m_menuSys.Append(MenuId_Boot_CDVD2,		_("Initializing..."));

	m_menuSys.Append(MenuId_Boot_ELF,		_("Run ELF..."),
		_("For running raw PS2 binaries directly"));

	m_menuSys.AppendSeparator();
	m_menuSys.Append(MenuId_Sys_SuspendResume,	_("Initializing..."));
	m_menuSys.AppendSeparator();

	//m_menuSys.Append(MenuId_Sys_Close,		_("Close"),
	//	_("Stops emulation and closes the GS window."));

	m_menuSys.Append(MenuId_Sys_LoadStates,	_("Load state"), &m_LoadStatesSubmenu);
	m_menuSys.Append(MenuId_Sys_SaveStates,	_("Save state"), &m_SaveStatesSubmenu);

	m_menuSys.Append(MenuId_EnableBackupStates,	_("Backup before save"),
		wxEmptyString, wxITEM_CHECK);

	m_menuSys.AppendSeparator();

	m_menuSys.Append(MenuId_EnablePatches,	_("Automatic Gamefixes"),
		_("Automatically applies needed Gamefixes to known problematic games"), wxITEM_CHECK);

	m_menuSys.Append(MenuId_EnableCheats,	_("Enable Cheats"),
		wxEmptyString, wxITEM_CHECK);

	m_menuSys.Append(MenuId_EnableHostFs,	_("Enable Host Filesystem"),
		wxEmptyString, wxITEM_CHECK);

	m_menuSys.AppendSeparator();

	m_menuSys.Append(MenuId_Sys_Shutdown,	_("Shutdown"),
		_("Wipes all internal VM states and shuts down plugins."));

	m_menuSys.Append(MenuId_Exit,			_("Exit"),
		AddAppName(_("Closing %s may be hazardous to your health")));


	// ------------------------------------------------------------------------
	wxMenu& isoRecents( wxGetApp().GetRecentIsoMenu() );

	//m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append( MenuId_IsoSelector,	_("Iso Selector"), &isoRecents );
	m_menuCDVD.Append( GetPluginMenuId_Settings(PluginId_CDVD), _("Plugin Menu"), m_PluginMenuPacks[PluginId_CDVD] );

	m_menuCDVD.AppendSeparator();
	m_menuCDVD.Append( MenuId_Src_Iso,		_("Iso"),		_("Makes the specified ISO image the CDVD source."), wxITEM_RADIO );
	m_menuCDVD.Append( MenuId_Src_Plugin,	_("Plugin"),	_("Uses an external plugin as the CDVD source."), wxITEM_RADIO );
	m_menuCDVD.Append( MenuId_Src_NoDisc,	_("No disc"),	_("Use this to boot into your virtual PS2's BIOS configuration."), wxITEM_RADIO );

	//m_menuCDVD.AppendSeparator();
	//m_menuCDVD.Append( MenuId_SkipBiosToggle,_("Enable BOOT2 injection"),
	//	_("Skips PS2 splash screens when booting from Iso or DVD media"), wxITEM_CHECK );

	// ------------------------------------------------------------------------

	m_menuConfig.Append(MenuId_Config_SysSettings,	_("Emulation &Settings") );
	m_menuConfig.Append(MenuId_Config_McdSettings,	_("&Memory cards") );
	m_menuConfig.Append(MenuId_Config_BIOS,			_("&Plugin/BIOS Selector") );
	if (IsDebugBuild) m_menuConfig.Append(MenuId_Config_GameDatabase,	_("Game Database Editor") );
	// Empty menu
	// m_menuConfig.Append(MenuId_Config_Language,		_("Appearance...") );

	m_menuConfig.AppendSeparator();

	m_menuConfig.Append(MenuId_Config_GS,		_("&Video (GS)"),		m_PluginMenuPacks[PluginId_GS]);
	m_menuConfig.Append(MenuId_Config_SPU2,		_("&Audio (SPU2)"),		m_PluginMenuPacks[PluginId_SPU2]);
	m_menuConfig.Append(MenuId_Config_PAD,		_("&Controllers (PAD)"),m_PluginMenuPacks[PluginId_PAD]);
	m_menuConfig.Append(MenuId_Config_DEV9,		_("Dev9"),				m_PluginMenuPacks[PluginId_DEV9]);
	m_menuConfig.Append(MenuId_Config_USB,		_("USB"),				m_PluginMenuPacks[PluginId_USB]);
	m_menuConfig.Append(MenuId_Config_FireWire,	_("Firewire"),			m_PluginMenuPacks[PluginId_FW]);

	//m_menuConfig.AppendSeparator();
	//m_menuConfig.Append(MenuId_Config_Patches,	_("Patches (unimplemented)"),	wxEmptyString);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_Multitap0Toggle,	_("Multitap 1"),	wxEmptyString, wxITEM_CHECK );
	m_menuConfig.Append(MenuId_Config_Multitap1Toggle,	_("Multitap 2"),	wxEmptyString, wxITEM_CHECK );

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(MenuId_Config_ResetAll,	_("Clear all settings..."),
		AddAppName(_("Clears all %s settings and re-runs the startup wizard.")));

	// ------------------------------------------------------------------------

	m_menuMisc.Append( &m_MenuItem_Console );
#ifdef __LINUX__
	m_menuMisc.Append( &m_MenuItem_Console_Stdio );
#endif
	//Todo: Though not many people need this one :p
	//m_menuMisc.Append(MenuId_Profiler,			_("Show Profiler"),	wxEmptyString, wxITEM_CHECK);
	//m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	m_menuMisc.AppendSeparator();

	//Todo:
	//There's a great working "open website" in the about panel. Less clutter by just using that.
	//m_menuMisc.Append(MenuId_Website,			_("Visit Website..."),
	//	_("Opens your web-browser to our favorite website."));
	m_menuMisc.Append(MenuId_About,				_("About...") );
#ifdef PCSX2_DEVBUILD
	//m_menuDebug.Append(MenuId_Debug_Open,		_("Open Debug Window..."),	wxEmptyString);
	//m_menuDebug.Append(MenuId_Debug_MemoryDump,	_("Memory Dump..."),		wxEmptyString);
	m_menuDebug.Append(MenuId_Debug_Logging,	_("Logging..."),			wxEmptyString);
#endif
	m_MenuItem_Console.Check( g_Conf->ProgLogBox.Visible );

	ConnectMenus();
	Connect( wxEVT_MOVE,			wxMoveEventHandler			(MainEmuFrame::OnMoveAround) );
	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler			(MainEmuFrame::OnCloseWindow) );
	Connect( wxEVT_SET_FOCUS,		wxFocusEventHandler			(MainEmuFrame::OnFocus) );
	Connect( wxEVT_ACTIVATE,		wxActivateEventHandler		(MainEmuFrame::OnActivate) );

	PushEventHandler( &wxGetApp().GetRecentIsoManager() );
	SetDropTarget( new IsoDropTarget( this ) );

	ApplyCoreStatus();
	ApplySettings();
}

MainEmuFrame::~MainEmuFrame() throw()
{
	if( m_RestartEmuOnDelete )
	{
		sApp.SetExitOnFrameDelete( false );
		sApp.PostAppMethod( &Pcsx2App::DetectCpuAndUserMode );
		sApp.WipeUserModeSettings();
	}
}

void MainEmuFrame::DoGiveHelp(const wxString& text, bool show)
{
	_parent::DoGiveHelp(text, show);
	wxGetApp().GetProgramLog()->DoGiveHelp(text, show);
}

// ----------------------------------------------------------------------------
// OnFocus / OnActivate : Special implementation to "connect" the console log window
// with the main frame window.  When one is clicked, the other is assured to be brought
// to the foreground with it.  (Currently only MSW only, as wxWidgets appears to have no
// equivalent to this).  Both OnFocus and OnActivate are handled because Focus events do
// not propagate up the window hierarchy, and on Activate events don't always get sent
// on the first focusing event after PCSX2 starts.

void MainEmuFrame::OnFocus( wxFocusEvent& evt )
{
	if( ConsoleLogFrame* logframe = wxGetApp().GetProgramLog() )
		MSW_SetWindowAfter( logframe->GetHandle(), GetHandle() );

	evt.Skip();
}

void MainEmuFrame::OnActivate( wxActivateEvent& evt )
{
	if( ConsoleLogFrame* logframe = wxGetApp().GetProgramLog() )
		MSW_SetWindowAfter( logframe->GetHandle(), GetHandle() );

	evt.Skip();
}
// ----------------------------------------------------------------------------

void MainEmuFrame::ApplyCoreStatus()
{
	wxMenuBar& menubar( *GetMenuBar() );

	wxMenuItem* susres	= menubar.FindItem( MenuId_Sys_SuspendResume );
	wxMenuItem* cdvd	= menubar.FindItem( MenuId_Boot_CDVD );
	wxMenuItem* cdvd2	= menubar.FindItem( MenuId_Boot_CDVD2 );
	wxMenuItem* restart	= menubar.FindItem( MenuId_Sys_Restart );

	// [TODO] : Ideally each of these items would bind a listener instance to the AppCoreThread
	// dispatcher, and modify their states accordingly.  This is just a hack (for now) -- air

	bool vm = SysHasValidState();

	if( susres )
	{
		if( !CoreThread.IsClosing() )
		{
			susres->Enable();
			susres->SetText(_("Pause"));
			susres->SetHelp(_("Safely pauses emulation and preserves the PS2 state."));
		}
		else
		{
			susres->Enable(vm);
			if( vm )
			{
				susres->SetText(_("Resume"));
				susres->SetHelp(_("Resumes the suspended emulation state."));
			}
			else
			{
				susres->SetText(_("Pause/Resume"));
				susres->SetHelp(_("No emulation state is active; cannot suspend or resume."));
			}
		}
	}

	if( restart )
	{
		if( vm )	
		{
			restart->SetText(_("Restart"));
			restart->SetHelp(_("Simulates hardware reset of the PS2 virtual machine."));
		}
		else
		{
			restart->Enable( false );
			restart->SetHelp(_("No emulation state is active; boot something first."));
		}
	}

	if( cdvd )
	{
		if( vm )
		{
			cdvd->SetText(_("Reboot CDVD (full)"));
			cdvd->SetHelp(_("Hard reset of the active VM."));
		}
		else
		{
			cdvd->SetText(_("Boot CDVD (full)"));
			cdvd->SetHelp(_("Boot the VM using the current DVD or Iso source media"));
		}	
	}

	if( cdvd2 )
	{
		if( vm )
		{
			cdvd2->SetText(_("Reboot CDVD (fast)"));
			cdvd2->SetHelp(_("Reboot using fast BOOT (skips splash screens)"));
		}
		else
		{
			cdvd2->SetText(_("Boot CDVD (fast)"));
			cdvd2->SetHelp(_("Use fast boot to skip PS2 startup and splash screens"));
		}
	}

	menubar.Enable( MenuId_Sys_Shutdown, SysHasValidState() || CorePlugins.AreAnyInitialized() );
}

//Apply a config to the menu such that the menu reflects it properly
void MainEmuFrame::ApplySettings()
{
	ApplyConfigToGui(*g_Conf);
}

//MainEmuFrame needs to be aware which items are affected by presets if AppConfig::APPLY_FLAG_FROM_PRESET is on.
//currently only EnablePatches is affected when the settings come from a preset.
void MainEmuFrame::ApplyConfigToGui(AppConfig& configToApply, int flags)
{
	wxMenuBar& menubar( *GetMenuBar() );

	menubar.Check(	MenuId_EnablePatches, configToApply.EmuOptions.EnablePatches );
	menubar.Enable(	MenuId_EnablePatches, !configToApply.EnablePresets );

	if ( !(flags & AppConfig::APPLY_FLAG_FROM_PRESET) )
	{//these should not be affected by presets
		menubar.Check( MenuId_EnableBackupStates, configToApply.EmuOptions.BackupSavestate );
		menubar.Check( MenuId_EnableCheats,  configToApply.EmuOptions.EnableCheats );
		menubar.Check( MenuId_EnableHostFs,  configToApply.EmuOptions.HostFs );
#ifdef __LINUX__
		menubar.Check( MenuId_Console_Stdio, configToApply.EmuOptions.ConsoleToStdio );
#endif

		menubar.Check( MenuId_Config_Multitap0Toggle, configToApply.EmuOptions.MultitapPort0_Enabled );
		menubar.Check( MenuId_Config_Multitap1Toggle, configToApply.EmuOptions.MultitapPort1_Enabled );
	}

	UpdateIsoSrcSelection();	//shouldn't be affected by presets but updates from g_Conf anyway and not from configToApply, so no problem here.
}

//write pending preset settings from the gui to g_Conf,
//	without triggering an overall "settingsApplied" event.
void MainEmuFrame::CommitPreset_noTrigger()
{
	wxMenuBar& menubar( *GetMenuBar() );
	g_Conf->EmuOptions.EnablePatches = menubar.IsChecked( MenuId_EnablePatches );
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
		MyMenu.Append( MenuId_Video_CoreSettings, _("Core GS Settings..."),
			_("Modify hardware emulation settings regulated by the PCSX2 core virtual machine.") );

		MyMenu.Append( MenuId_Video_WindowSettings, _("Window Settings..."),
			_("Modify window and appearance options, including aspect ratio.") );

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
	if( !CorePlugins.IsLoaded(PluginId) ) return;
	MyMenu.SetLabel( GetPluginMenuId_Name(PluginId),
		CorePlugins.GetName( PluginId ) + L" " + CorePlugins.GetVersion( PluginId )
	);
	MyMenu.Enable( GetPluginMenuId_Settings(PluginId), true );
}

