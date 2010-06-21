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
#include "ConsoleLogger.h"
#include "MSWstuff.h"

#include "Utilities/IniInterface.h"
#include "DebugTools/Debug.h"
#include "Dialogs/ModalPopups.h"

#include <wx/cmdline.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>

using namespace pxSizerFlags;

static void CpuCheckSSE2()
{
	if( x86caps.hasStreamingSIMD2Extensions ) return;

	// Only check once per process session:
	static bool checked = false;
	if( checked ) return;
	checked = true;

	wxDialogWithHelpers exconf( NULL, _("PCSX2 - SSE2 Recommended") );

	exconf += exconf.Heading( pxE( ".Popup:Startup:NoSSE2",
		L"Warning: Your computer does not support SSE2, which is required by many PCSX2 recompilers and plugins. "
		L"Your options will be limited and emulation will be *very* slow." )
	);

	pxIssueConfirmation( exconf, MsgButtons().OK(), L"Error:Startup:NoSSE2" );

	// Auto-disable anything that needs SSE2:

	g_Conf->EmuOptions.Cpu.Recompiler.EnableEE	= false;
	g_Conf->EmuOptions.Cpu.Recompiler.EnableVU0	= false;
	g_Conf->EmuOptions.Cpu.Recompiler.EnableVU1	= false;
}

void Pcsx2App::WipeUserModeSettings()
{
	wxDirName usrlocaldir( wxStandardPaths::Get().GetUserLocalDataDir() );
	if( !usrlocaldir.Exists() ) return;

	wxString cwd( Path::Normalize( wxGetCwd() ) );
#ifdef __WXMSW__
	cwd.MakeLower();
#endif
	u32 hashres = HashTools::Hash( (char*)cwd.c_str(), cwd.Length()*sizeof(wxChar) );

	wxFileName usermodefile( FilenameDefs::GetUsermodeConfig() );
	usermodefile.SetPath( usrlocaldir.ToString() );
	ScopedPtr<wxFileConfig> conf_usermode( OpenFileConfig( usermodefile.GetFullPath() ) );

	wxString groupname( wxsFormat( L"CWD.%08x", hashres ) );
	Console.WriteLn( "(UserModeSettings) Removing entry:" );
	Console.Indent().WriteLn( L"Path: %s\nHash:%s", cwd.c_str(), groupname.c_str() );
	conf_usermode->DeleteGroup( groupname );
}

// User mode settings can't be stored in the CWD for two reasons:
//   (a) the user may not have permission to do so (most obvious)
//   (b) it would result in sloppy usermode.ini found all over a hard drive if people runs the
//       exe from many locations (ugh).
//
// So better to use the registry on Win32 and a "default ini location" config file under Linux,
// and store the usermode settings for the CWD based on the CWD's hash.
//
void Pcsx2App::ReadUserModeSettings()
{
	wxDirName usrlocaldir( wxStandardPaths::Get().GetUserLocalDataDir() );
	if( !usrlocaldir.Exists() )
	{
		Console.WriteLn( L"Creating UserLocalData folder: " + usrlocaldir.ToString() );
		usrlocaldir.Mkdir();
	}

	wxString cwd( Path::Normalize( wxGetCwd() ) );
#ifdef __WXMSW__
	cwd.MakeLower();
#endif

	u32 hashres = HashTools::Hash( (char*)cwd.c_str(), cwd.Length()*sizeof(wxChar) );

	wxFileName usermodefile( FilenameDefs::GetUsermodeConfig() );
	usermodefile.SetPath( usrlocaldir.ToString() );
	ScopedPtr<wxFileConfig> conf_usermode( OpenFileConfig( usermodefile.GetFullPath() ) );

	wxString groupname( wxsFormat( L"CWD.%08x", hashres ) );

	bool hasGroup = conf_usermode->HasGroup( groupname );
	bool forceWiz = Startup.ForceWizard || !hasGroup;
	
	if( !forceWiz )
	{
		conf_usermode->SetPath( groupname );
		forceWiz = !conf_usermode->HasEntry( L"DocumentsFolderMode" );
		conf_usermode->SetPath( L".." );
	}

	if( forceWiz )
	{
		// Beta Warning!
		#if 0
		if( !hasGroup )
		{
			wxDialogWithHelpers beta( NULL, wxsFormat(_("Welcome to %s %u.%u.%u (r%u)")), pxGetAppName(), PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo, SVN_REV );
			beta.SetMinWidth(480);

			beta += beta.Heading(
				L"PCSX2 0.9.7 is a work-in-progress.  We are in the middle of major rewrites of the user interface, and some parts "
				L"of the program have *NOT* been re-implemented yet.  Options will be missing or disabled.  Horrible crashes might be present.  Enjoy!"
			);
			beta += StdPadding*2;
			beta += new wxButton( &beta, wxID_OK ) | StdCenter();
			beta.ShowModal();
		}
		#endif
	
		// first time startup, so give the user the choice of user mode:
		FirstTimeWizard wiz( NULL );
		if( !wiz.RunWizard( wiz.GetUsermodePage() ) )
			throw Exception::StartupAborted( L"Startup aborted: User canceled FirstTime Wizard." );

		// Save user's new settings
		IniSaver saver( *conf_usermode );
		g_Conf->LoadSaveUserMode( saver, groupname );
		AppConfig_OnChangedSettingsFolder( true );
		AppSaveSettings();
	}
	else
	{
		// usermode.ini exists and is populated with valid data -- assume User Documents mode,
		// unless the ini explicitly specifies otherwise.

		DocsFolderMode = DocsFolder_User;

		IniLoader loader( *conf_usermode );
		g_Conf->LoadSaveUserMode( loader, groupname );

		if( !wxFile::Exists( GetSettingsFilename() ) )
		{
			// user wiped their pcsx2.ini -- needs a reconfiguration via wizard!
			// (we skip the first page since it's a usermode.ini thing)
			
			// Fixme : Skipping the first page is a bad idea, as it does a lot of file / directory checks on hitting Apply.
			// If anything is missing, the first page prompts to fix it.
			// If we skip this check, it's very likely that actions like creating Memory Cards will fail.
			FirstTimeWizard wiz( NULL );
			if( !wiz.RunWizard( /*wiz.GetPostUsermodePage()*/ wiz.GetUsermodePage() ) )
				throw Exception::StartupAborted( L"Startup aborted: User canceled Configuration Wizard." );

			// Save user's new settings
			IniSaver saver( *conf_usermode );
			g_Conf->LoadSaveUserMode( saver, groupname );
			AppConfig_OnChangedSettingsFolder( true );
			AppSaveSettings();
		}
	}
	
	// force unload plugins loaded by the wizard.  If we don't do this the recompilers might
	// fail to allocate the memory they need to function.
	UnloadPlugins();
}

void Pcsx2App::DetectCpuAndUserMode()
{
	AffinityAssert_AllowFrom_MainUI();
	
	x86caps.Identify();
	x86caps.CountCores();
	x86caps.SIMD_EstablishMXCSRmask();

	if( !x86caps.hasMultimediaExtensions )
	{
		// Note: due to memcpy_fast, we need minimum MMX even for interpreters.  This will
		// hopefully change later once we have a dynamically recompiled memcpy.
		throw Exception::HardwareDeficiency(L"MMX Extensions not available.", _("PCSX2 requires cpu with MMX instruction to run."));
	}

	ReadUserModeSettings();
	AppConfig_OnChangedSettingsFolder();
}

void Pcsx2App::OpenMainFrame()
{
	if( AppRpc_TryInvokeAsync( &Pcsx2App::OpenMainFrame ) ) return;

	if( GetMainFramePtr() != NULL ) return;

	MainEmuFrame* mainFrame = new MainEmuFrame( NULL, pxGetAppName() );
	m_id_MainFrame = mainFrame->GetId();

	PostIdleAppMethod( &Pcsx2App::OpenProgramLog );

	SetTopWindow( mainFrame );		// not really needed...
	SetExitOnFrameDelete( false );	// but being explicit doesn't hurt...
	mainFrame->Show();
}

void Pcsx2App::OpenProgramLog()
{
	if( AppRpc_TryInvokeAsync( &Pcsx2App::OpenProgramLog ) ) return;

	if( ConsoleLogFrame* frame = GetProgramLog() )
	{
		//pxAssume( );
		return;
	}

	wxWindow* m_current_focus = wxGetActiveWindow();

	ScopedLock lock( m_mtx_ProgramLog );
	m_ptr_ProgramLog	= new ConsoleLogFrame( GetMainFramePtr(), L"PCSX2 Program Log", g_Conf->ProgLogBox );
	m_id_ProgramLogBox	= m_ptr_ProgramLog->GetId();
	EnableAllLogging();

	if( m_current_focus ) m_current_focus->SetFocus();
}

void Pcsx2App::AllocateCoreStuffs()
{
	if( AppRpc_TryInvokeAsync( &Pcsx2App::AllocateCoreStuffs ) ) return;

	CpuCheckSSE2();
	SysLogMachineCaps();
	AppApplySettings();

	if( !m_CoreAllocs )
	{
		// FIXME : Some or all of SysCoreAllocations should be run from the SysExecutor thread,
		// so that the thread is safely blocked from being able to start emulation.

		m_CoreAllocs = new SysCoreAllocations();

		if( m_CoreAllocs->HadSomeFailures( g_Conf->EmuOptions.Cpu.Recompiler ) )
		{
			// HadSomeFailures only returns 'true' if an *enabled* cpu type fails to init.  If
			// the user already has all interps configured, for example, then no point in
			// popping up this dialog.
			
			wxDialogWithHelpers exconf( NULL, _("PCSX2 Recompiler Error(s)") );

			exconf += 12;
			exconf += exconf.Heading( pxE( ".Popup:RecompilerInit",
				L"Warning: Some of the configured PS2 recompilers failed to initialize and will not be available for this session:\n" )
			);

			wxTextCtrl* scrollableTextArea = new wxTextCtrl(
				&exconf, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxTE_READONLY | wxTE_MULTILINE | wxTE_WORDWRAP
			);

			exconf += scrollableTextArea	| pxSizerFlags::StdExpand();
			
			if( !m_CoreAllocs->IsRecAvailable_EE() )
			{
				scrollableTextArea->AppendText( L"* R5900 (EE)\n\n" );

				g_Conf->EmuOptions.Recompiler.EnableEE = false;
			}

			if( !m_CoreAllocs->IsRecAvailable_IOP() )
			{
				scrollableTextArea->AppendText( L"* R3000A (IOP)\n\n" );
				g_Conf->EmuOptions.Recompiler.EnableIOP = false;
			}

			if( !m_CoreAllocs->IsRecAvailable_MicroVU0() )			{
				scrollableTextArea->AppendText( L"* microVU0\n\n" );
				g_Conf->EmuOptions.Recompiler.UseMicroVU0	= false;
				g_Conf->EmuOptions.Recompiler.EnableVU0		= g_Conf->EmuOptions.Recompiler.EnableVU0 && m_CoreAllocs->IsRecAvailable_SuperVU0();
			}

			if( !m_CoreAllocs->IsRecAvailable_MicroVU1() )
			{
				scrollableTextArea->AppendText( L"* microVU1\n\n" );
				g_Conf->EmuOptions.Recompiler.UseMicroVU1	= false;
				g_Conf->EmuOptions.Recompiler.EnableVU1		= g_Conf->EmuOptions.Recompiler.EnableVU1 && m_CoreAllocs->IsRecAvailable_SuperVU1();
			}

			if( !m_CoreAllocs->IsRecAvailable_SuperVU0() )
			{
				scrollableTextArea->AppendText( L"* SuperVU0\n\n" );
				g_Conf->EmuOptions.Recompiler.UseMicroVU0	= m_CoreAllocs->IsRecAvailable_MicroVU0();
				g_Conf->EmuOptions.Recompiler.EnableVU0		= g_Conf->EmuOptions.Recompiler.EnableVU0 && g_Conf->EmuOptions.Recompiler.UseMicroVU0;
			}

			if( !m_CoreAllocs->IsRecAvailable_SuperVU1() )
			{
				scrollableTextArea->AppendText( L"* SuperVU1\n\n" );
				g_Conf->EmuOptions.Recompiler.UseMicroVU1	= m_CoreAllocs->IsRecAvailable_MicroVU1();
				g_Conf->EmuOptions.Recompiler.EnableVU1		= g_Conf->EmuOptions.Recompiler.EnableVU1 && g_Conf->EmuOptions.Recompiler.UseMicroVU1;
			}

			exconf += new ModalButtonPanel( &exconf, MsgButtons().OK() ) | pxSizerFlags::StdCenter();

			exconf.ShowModal();

			// Failures can be SSE-related OR memory related.  Should do per-cpu error reports instead...

			/*message += pxE( ".Popup Error:EmuCore:MemoryForRecs",
				L"These errors are the result of memory allocation failures (see the program log for details). "
				L"Closing out some memory hogging background tasks may resolve this error.\n\n"
				L"These recompilers have been disabled and interpreters will be used in their place.  "
				L"Interpreters can be very slow, so don't get too excited.  Press OK to continue or CANCEL to close PCSX2."
			);*/

			//if( !Msgbox::OkCancel( message, _("PCSX2 Initialization Error"), wxICON_ERROR ) )
			//	return false;
		}
	}

	LoadPluginsPassive();
}


void Pcsx2App::OnInitCmdLine( wxCmdLineParser& parser )
{
	parser.SetLogo( wxsFormat(L" >>  %s  --  A Playstation2 Emulator for the PC  <<", pxGetAppName()) + L"\n\n" +
		_("All options are for the current session only and will not be saved.\n")
	);

	wxString fixlist( L" " );
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
	{
		if( i != GamefixId_FIRST ) fixlist += L",";
		fixlist += EnumToString(i);
	}

	parser.AddParam( _("IsoFile"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL );
	parser.AddSwitch( L"h",			L"help",		_("displays this list of command line options"), wxCMD_LINE_OPTION_HELP );
	parser.AddSwitch( wxEmptyString,L"console",		_("forces the program log/console to be visible"), wxCMD_LINE_VAL_STRING );
	parser.AddSwitch( wxEmptyString,L"fullscreen",	_("use fullscreen GS mode") );
	parser.AddSwitch( wxEmptyString,L"windowed",	_("use windowed GS mode") );

	parser.AddSwitch( wxEmptyString,L"nogui",		_("disables display of the gui while running games") );
	parser.AddOption( wxEmptyString,L"elf",			_("executes an ELF image"), wxCMD_LINE_VAL_STRING );
	parser.AddSwitch( wxEmptyString,L"nodisc",		_("boots an empty dvd tray; use to enter the PS2 system menu") );
	parser.AddSwitch( wxEmptyString,L"usecd",		_("boots from the CDVD plugin (overrides IsoFile parameter)") );

	parser.AddSwitch( wxEmptyString,L"fullboot",	_("disables fast booting") );
	parser.AddSwitch( wxEmptyString,L"nohacks",		_("disables all speedhacks") );
	parser.AddSwitch( wxEmptyString,L"gamefixes",	_("use the specified comma or pipe-delimited list of gamefixes.") + fixlist );

	parser.AddOption( wxEmptyString,L"cfgpath",		_("changes the configuration file path"), wxCMD_LINE_VAL_STRING );
	parser.AddOption( wxEmptyString,L"cfg",			_("specifies the PCSX2 configuration file to use"), wxCMD_LINE_VAL_STRING );
	parser.AddSwitch( wxEmptyString,L"forcewiz",	wxsFormat(_("forces %s to start the First-time Wizard"), pxGetAppName()) );

	const PluginInfo* pi = tbl_PluginInfo; do {
		parser.AddOption( wxEmptyString, pi->GetShortname().Lower(),
			wxsFormat( _("specify the file to use as the %s plugin"), pi->GetShortname().c_str() )
		);
	} while( ++pi, pi->shortname != NULL );

	parser.SetSwitchChars( L"-" );
}

bool Pcsx2App::OnCmdLineError( wxCmdLineParser& parser )
{
	wxApp::OnCmdLineError( parser );
	return false;
}

bool Pcsx2App::ParseOverrides( wxCmdLineParser& parser )
{
	wxString dest;

	if (parser.Found( L"cfgpath", &dest ) && !dest.IsEmpty())
	{
		Console.Warning( L"Config path override: " + dest );
		Overrides.SettingsFolder = dest;
	}

	if (parser.Found( L"cfg", &dest ) && !dest.IsEmpty())
	{
		Console.Warning( L"Config file override: " + dest );
		Overrides.SettingsFile = dest;
	}

	Overrides.DisableSpeedhacks = parser.Found(L"nohacks");

	if (parser.Found(L"fullscreen"))	Overrides.GsWindowMode = GsWinMode_Fullscreen;
	if (parser.Found(L"windowed"))		Overrides.GsWindowMode = GsWinMode_Windowed;

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		if( !parser.Found( pi->GetShortname().Lower(), &dest ) ) continue;

		if( wxFileExists( dest ) )
			Console.Warning( pi->GetShortname() + L" override: " + dest );
		else
		{
			wxDialogWithHelpers okcan( NULL, wxsFormat(_("Plugin Override Error - %s"), pxGetAppName()) );

			okcan += okcan.Heading( wxsFormat(
				_("%s Plugin Override Error!  The following file does not exist or is not a valid %s plugin:\n\n"),
				pi->GetShortname().c_str(), pi->GetShortname().c_str()
			) );

			okcan += okcan.GetCharHeight();
			okcan += okcan.Text(dest);
			okcan += okcan.GetCharHeight();
			okcan += okcan.Heading(wxsFormat(_("Press OK to use the default configured plugin, or Cancel to close %s."), pxGetAppName()));

			if( wxID_CANCEL == pxIssueConfirmation( okcan, MsgButtons().OKCancel() ) ) return false;
		}
		
		Overrides.Filenames.Plugins[pi->id] = dest;

	} while( ++pi, pi->shortname != NULL );
	
	return true;
}

bool Pcsx2App::OnCmdLineParsed( wxCmdLineParser& parser )
{
	if( parser.Found(L"console") )
	{
		Startup.ForceConsole = true;
		OpenProgramLog();
	}

	// Suppress wxWidgets automatic options parsing since none of them pertain to PCSX2 needs.
	//wxApp::OnCmdLineParsed( parser );

	m_UseGUI	= !parser.Found(L"nogui");

	if( !ParseOverrides(parser) ) return false;

	// --- Parse Startup/Autoboot options ---

	Startup.NoFastBoot		= parser.Found(L"fullboot");
	Startup.ForceWizard		= parser.Found(L"forcewiz");

	if( parser.GetParamCount() >= 1 )
	{
		Startup.IsoFile		= parser.GetParam( 0 );
		Startup.SysAutoRun	= true;
	}
	
	if( parser.Found(L"usecd") )
	{
		Startup.CdvdSource	= CDVDsrc_Plugin;
		Startup.SysAutoRun	= true;
	}

	return true;
}

typedef void (wxEvtHandler::*pxInvokeAppMethodEventFunction)(Pcsx2AppMethodEvent&);
typedef void (wxEvtHandler::*pxStuckThreadEventHandler)(pxMessageBoxEvent&);

bool Pcsx2App::OnInit()
{
	EnableAllLogging();
	Console.WriteLn("Interface is initializing.  Entering Pcsx2App::OnInit!");

	InitCPUTicks();

	pxDoAssert = AppDoAssert;
	g_Conf = new AppConfig();
    wxInitAllImageHandlers();

	Console.WriteLn("Begin parsing commandline...");
	if( !_parent::OnInit() ) return false;

	wxLocale::AddCatalogLookupPathPrefix( wxGetCwd() );

#define pxAppMethodEventHandler(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxInvokeAppMethodEventFunction, &func )

	Connect( pxID_PadHandler_Keydown,	wxEVT_KEY_DOWN,		wxKeyEventHandler			(Pcsx2App::OnEmuKeyDown) );
	Connect(							wxEVT_DESTROY,		wxWindowDestroyEventHandler	(Pcsx2App::OnDestroyWindow) );

	// User/Admin Mode Dual Setup:
	//   PCSX2 now supports two fundamental modes of operation.  The default is Classic mode,
	//   which uses the Current Working Directory (CWD) for all user data files, and requires
	//   Admin access on Vista (and some Linux as well).  The second mode is the Vista-
	//   compatible \documents folder usage.  The mode is determined by the presence and
	//   contents of a usermode.ini file in the CWD.  If the ini file is missing, we assume
	//   the user is setting up a classic install.  If the ini is present, we read the value of
	//   the UserMode and SettingsPath vars.
	//
	//   Conveniently this dual mode setup applies equally well to most modern Linux distros.

	try
	{
		InitDefaultGlobalAccelerators();
		delete wxLog::SetActiveTarget( new pxLogConsole() );

		m_RecentIsoList = new RecentIsoList();

#ifdef __WXMSW__
		pxDwm_Load();
#endif
		SysExecutorThread.Start();
		DetectCpuAndUserMode();

		//   Set Manual Exit Handling
		// ----------------------------
		// PCSX2 has a lot of event handling logistics, so we *cannot* depend on wxWidgets automatic event
		// loop termination code.  We have a much safer system in place that continues to process messages
		// until all "important" threads are closed out -- not just until the main frame is closed(-ish).
		m_timer_Termination = new wxTimer( this, wxID_ANY );
		Connect( m_timer_Termination->GetId(), wxEVT_TIMER, wxTimerEventHandler(Pcsx2App::OnScheduledTermination) );
		SetExitOnFrameDelete( false );


		//   Start GUI and/or Direct Emulation
		// -------------------------------------
		if( Startup.ForceConsole ) g_Conf->ProgLogBox.Visible = true;
		OpenProgramLog();
		if( m_UseGUI ) OpenMainFrame();
		AllocateCoreStuffs();
		
		if( Startup.SysAutoRun )
		{
			// Notes: Saving/remembering the Iso file is probably fine and desired, so using
			// SysUpdateIsoSrcFile is good(ish).
			// Saving the cdvd plugin override isn't desirable, so we don't assign it into g_Conf.

			g_Conf->EmuOptions.UseBOOT2Injection = !Startup.NoFastBoot;
			SysUpdateIsoSrcFile( Startup.IsoFile );
			sApp.SysExecute( Startup.CdvdSource );
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::StartupAborted& ex )		// user-aborted, no popups needed.
	{
		Console.Warning( ex.FormatDiagnosticMessage() );
		CleanupOnExit();
		return false;
	}
	catch( Exception::HardwareDeficiency& ex )
	{
		Msgbox::Alert( ex.FormatDisplayMessage() + wxsFormat(_("\n\nPress OK to close %s."), pxGetAppName()), _("PCSX2 Error: Hardware Deficiency") );
		CleanupOnExit();
		return false;
	}
	// ----------------------------------------------------------------------------
	// Failures on the core initialization procedure (typically OutOfMemory errors) are bad,
	// since it means the emulator is completely non-functional.  Let's pop up an error and
	// exit gracefully-ish.
	//
	catch( Exception::RuntimeError& ex )
	{
		Console.Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage() + wxsFormat(_("\n\nPress OK to close %s."), pxGetAppName()),
			wxsFormat(_("%s Critical Error"), pxGetAppName()), wxICON_ERROR );
		CleanupOnExit();
		return false;
	}
    return true;
}

static int m_term_threshold = 20;

void Pcsx2App::OnScheduledTermination( wxTimerEvent& evt )
{
	if( !pxAssertDev( m_ScheduledTermination, "Scheduled Termination check is inconsistent with ScheduledTermination status." ) )
	{
		m_timer_Termination->Stop();
		return;
	}

	if( m_PendingSaves != 0 )
	{
		if( --m_term_threshold > 0 )
		{
			Console.WriteLn( "(App) %d saves are still pending; exit postponed...", m_PendingSaves );
			return;
		}
		
		Console.Error( "(App) %s pending saves have exceeded OnExit threshold and are being prematurely terminated!", m_PendingSaves );
	}

	m_timer_Termination->Stop();
	Exit();
}


// Common exit handler which can be called from any event (though really it should
// be called only from CloseWindow handlers since that's the more appropriate way
// to handle cancelable window closures)
//
// returns true if the app can close, or false if the close event was canceled by
// the glorious user, whomever (s)he-it might be.
void Pcsx2App::PrepForExit()
{
	if( m_ScheduledTermination ) return;
	m_ScheduledTermination = true;

	SysExecutorThread.ShutdownQueue();
	DispatchEvent( AppStatus_Exiting );

	m_timer_Termination->Start( 500 );

	// This should be called by OnExit(), but sometimes wxWidgets fails to call OnExit(), so
	// do it here just in case (no harm anyway -- OnExit is the next logical step after
	// CloseWindow returns true from the TopLevel window).
	//CleanupRestartable();
}

// This cleanup procedure can only be called when the App message pump is still active.
// OnExit() must use CleanupOnExit instead.
void Pcsx2App::CleanupRestartable()
{
	AffinityAssert_AllowFrom_MainUI();

	ShutdownPlugins();
	SysExecutorThread.ShutdownQueue();
	IdleEventDispatcher( L"Cleanup" );

	if( g_Conf ) AppSaveSettings();
}

// This cleanup handler can be called from OnExit (it doesn't need a running message pump),
// but should not be called from the App destructor.  It's needed because wxWidgets doesn't
// always call OnExit(), so I had to make CleanupRestartable, and then encapsulate it here
// to be friendly to the OnExit scenario (no message pump).
void Pcsx2App::CleanupOnExit()
{
	AffinityAssert_AllowFrom_MainUI();

	try
	{
		CleanupRestartable();
		CleanupResources();
	}
	catch( Exception::CancelEvent& )		{ throw; }
	catch( Exception::RuntimeError& ex )
	{
		// Handle runtime errors gracefully during shutdown.  Mostly these are things
		// that we just don't care about by now, and just want to "get 'er done!" so
		// we can exit the app. ;)

		Console.Error( L"Runtime exception handled during CleanupOnExit:\n" );
		Console.Indent().Error( ex.FormatDiagnosticMessage() );
	}

#ifdef __WXMSW__
	pxDwm_Unload();
#endif
	
	// Notice: deleting the plugin manager (unloading plugins) here causes Lilypad to crash,
	// likely due to some pending message in the queue that references lilypad procs.
	// We don't need to unload plugins anyway tho -- shutdown is plenty safe enough for
	// closing out all the windows.  So just leave it be and let the plugins get unloaded
	// during the wxApp destructor. -- air
	
	// FIXME: performing a wxYield() here may fix that problem. -- air

	pxDoAssert = pxAssertImpl_LogIt;
	Console_SetActiveHandler( ConsoleWriter_Stdout );
}

void Pcsx2App::CleanupResources()
{
	delete wxConfigBase::Set( NULL );

	while( wxGetLocale() != NULL )
		delete wxGetLocale();

	m_Resources = NULL;
}

int Pcsx2App::OnExit()
{
	CleanupOnExit();
	return wxApp::OnExit();
}

void Pcsx2App::OnDestroyWindow( wxWindowDestroyEvent& evt )
{
	// Precautions:
	//  * Whenever windows are destroyed, make sure to check if it matches our "active"
	//    console logger.  If so, we need to disable logging to the console window, or else
	//    it'll crash.  (this is because the console log system uses a cached window handle
	//    instead of looking the window up via it's ID -- fast but potentially unsafe).
	//
	//  * The virtual machine's plugins usually depend on the GS window handle being valid,
	//    so if the GS window is the one being shut down then we need to make sure to close
	//    out the Corethread before it vanishes completely from existence.
	

	OnProgramLogClosed( evt.GetId() );
	OnGsFrameClosed( evt.GetId() );
	evt.Skip();
}

// --------------------------------------------------------------------------------------
//  SysEventHandler
// --------------------------------------------------------------------------------------
class SysEvtHandler : public pxEvtHandler
{
public:
	wxString GetEvtHandlerName() const { return L"SysExecutor"; }

protected:
	// When the SysExec message queue is finally empty, we should check the state of
	// the menus and make sure they're all consistent to the current emulation states.
	void _DoIdle()
	{
		UI_UpdateSysControls();
	}
};


Pcsx2App::Pcsx2App() 
	: SysExecutorThread( new SysEvtHandler() )
{
	m_PendingSaves			= 0;
	m_ScheduledTermination	= false;

	m_id_MainFrame		= wxID_ANY;
	m_id_GsFrame		= wxID_ANY;
	m_id_ProgramLogBox	= wxID_ANY;
	m_ptr_ProgramLog	= NULL;

	SetAppName( L"pcsx2" );
	BuildCommandHash();
}

Pcsx2App::~Pcsx2App()
{
	pxDoAssert = pxAssertImpl_LogIt;
}

void Pcsx2App::CleanUp()
{
	CleanupResources();
	m_Resources		= NULL;
	m_RecentIsoList	= NULL;

	DisableDiskLogging();

	if( emuLog != NULL )
	{
		fclose( emuLog );
		emuLog = NULL;
	}

	_parent::CleanUp();
}


// ------------------------------------------------------------------------------------------
//  Using the MSVCRT to track memory leaks:
// ------------------------------------------------------------------------------------------
// When exiting PCSX2 normally, the CRT will make a list of all memory that's leaked.  The
// number inside {} can be pasted into the line below to cause MSVC to breakpoint on that
// allocation at the time it's made.  And then using a stacktrace you can figure out what
// leaked! :D
//
// Limitations: Unfortunately, wxWidgets gui uses a lot of heap allocations while handling
// messages, and so any mouse movements will pretty much screw up the leak value.  So to use
// this feature you need to execute pcsx in no-gui mode, and then not move the mouse or use
// the keyboard until you get to the leak. >_<
//
// (but this tool is still better than nothing!)

#ifdef PCSX2_DEBUG
struct CrtDebugBreak
{
	CrtDebugBreak( int spot )
	{
#ifdef __WXMSW__
		_CrtSetBreakAlloc( spot );
#endif
	}
};

//CrtDebugBreak breakAt( 2014 );

#endif
