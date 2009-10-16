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
#include "IniInterface.h"
#include "MainFrame.h"
#include "ConsoleLogger.h"
#include "DebugTools/Debug.h"
#include "Dialogs/ModalPopups.h"

#include <wx/cmdline.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>

static bool m_ForceWizard = false;

namespace Exception
{
	// --------------------------------------------------------------------------
	// Exception used to perform an "errorless" termination of the app during OnInit
	// procedures.  This happens when a user cancels out of startup prompts/wizards.
	//
	class StartupAborted : public BaseException
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( StartupAborted )

		StartupAborted( const wxString& msg_eng=L"Startup initialization was aborted by the user." )
		{
			// english messages only for this exception.
			BaseException::InitBaseEx( msg_eng, msg_eng );
		}
	};
}

void Pcsx2App::OpenWizardConsole()
{
	if( !IsDebugBuild ) return;
	g_Conf->ProgLogBox.Visible = true;
	m_ProgramLogBox	= new ConsoleLogFrame( NULL, L"PCSX2 Program Log", g_Conf->ProgLogBox );
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
	wxString cwd( wxGetCwd() );
	u32 hashres = HashTools::Hash( (char*)cwd.c_str(), cwd.Length() );

	wxDirName usrlocaldir( wxStandardPaths::Get().GetUserLocalDataDir() );
	if( !usrlocaldir.Exists() )
	{
		Console.Status( L"Creating UserLocalData folder: " + usrlocaldir.ToString() );
		usrlocaldir.Mkdir();
	}

	wxFileName usermodefile( FilenameDefs::GetUsermodeConfig() );
	usermodefile.SetPath( usrlocaldir.ToString() );
	ScopedPtr<wxFileConfig> conf_usermode( OpenFileConfig( usermodefile.GetFullPath() ) );

	wxString groupname( wxsFormat( L"CWD.%08x", hashres ) );

	if( m_ForceWizard || !conf_usermode->HasGroup( groupname ) )
	{
		// first time startup, so give the user the choice of user mode:
		OpenWizardConsole();
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
		// usermode.ini exists -- assume Documents mode, unless the ini explicitly
		// specifies otherwise.
		UseAdminMode = false;

		IniLoader loader( *conf_usermode );
		g_Conf->LoadSaveUserMode( loader, groupname );

		if( !wxFile::Exists( GetSettingsFilename() ) )
		{
			// user wiped their pcsx2.ini -- needs a reconfiguration via wizard!
			// (we skip the first page since it's a usermode.ini thing)

			OpenWizardConsole();
			FirstTimeWizard wiz( NULL );
			if( !wiz.RunWizard( wiz.GetPostUsermodePage() ) )
				throw Exception::StartupAborted( L"Startup aborted: User canceled Configuration Wizard." );

			// Save user's new settings
			IniSaver saver( *conf_usermode );
			g_Conf->LoadSaveUserMode( saver, groupname );
			AppConfig_OnChangedSettingsFolder( true );
			AppSaveSettings();
		}
	}
	
	// force a reset here to unload plugins loaded by the wizard.  If we don't do this
	// the recompilers might fail to allocate the memory they need to function.
	SysReset();
}

void Pcsx2App::OnInitCmdLine( wxCmdLineParser& parser )
{
	parser.SetLogo( (wxString)L" >>  PCSX2  --  A Playstation2 Emulator for the PC  <<\n\n" +
		_("All options are for the current session only and will not be saved.\n")
	);

	parser.AddParam( _("IsoFile"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL );

	parser.AddSwitch( L"h",			L"help",	_("displays this list of command line options"), wxCMD_LINE_OPTION_HELP );

	parser.AddSwitch( wxEmptyString,L"nogui",	_("disables display of the gui while running games") );
	parser.AddSwitch( wxEmptyString,L"skipbios",_("skips standard BIOS splash screens and software checks") );
	parser.AddOption( wxEmptyString,L"elf",		_("executes an ELF image"), wxCMD_LINE_VAL_STRING );
	parser.AddSwitch( wxEmptyString,L"nodisc",	_("boots an empty dvd tray; use to enter the PS2 system menu") );
	parser.AddSwitch( wxEmptyString,L"usecd",	_("boots from the configured CDVD plugin (ignores IsoFile parameter)") );

	parser.AddOption( wxEmptyString,L"cfgpath",	_("changes the configuration file path"), wxCMD_LINE_VAL_STRING );
	parser.AddOption( wxEmptyString,L"cfg",		_("specifies the PCSX2 configuration file to use [not implemented]"), wxCMD_LINE_VAL_STRING );

	parser.AddSwitch( wxEmptyString,L"forcewiz",_("Forces PCSX2 to start the First-time Wizard") );

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

bool Pcsx2App::OnCmdLineParsed( wxCmdLineParser& parser )
{
	if( parser.GetParamCount() >= 1 )
	{
		// [TODO] : Unnamed parameter is taken as an "autorun" option for a cdvd/iso.
		parser.GetParam( 0 );
	}

	// Suppress wxWidgets automatic options parsing since none of them pertain to PCSX2 needs.
	//wxApp::OnCmdLineParsed( parser );

	//bool yay = parser.Found(L"nogui");
	m_ForceWizard = parser.Found( L"forcewiz" );

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		wxString dest;
		if( !parser.Found( pi->GetShortname().Lower(), &dest ) ) continue;

		OverrideOptions.Filenames.Plugins[pi->id] = dest;

		if( wxFileExists( dest ) )
			Console.Notice( pi->GetShortname() + L" override: " + dest );
		else
		{
			bool result = Msgbox::OkCancel(
				wxsFormat( _("Plugin Override Error!  Specified %s plugin does not exist:\n\n"), pi->GetShortname().c_str() ) +
				dest +
				_("Press OK to use the default configured plugin, or Cancel to close."),
				_("Plugin Override Error - PCSX2"), wxICON_ERROR
			);

			if( !result ) return false;
		}
	} while( ++pi, pi->shortname != NULL );

	parser.Found( L"cfgpath", &OverrideOptions.SettingsFolder );

	return true;
}

typedef void (wxEvtHandler::*pxMessageBoxEventFunction)(pxMessageBoxEvent&);

// ------------------------------------------------------------------------
bool Pcsx2App::OnInit()
{
	g_Conf = new AppConfig();
	EnableAllLogging();

    wxInitAllImageHandlers();
	if( !wxApp::OnInit() ) return false;

	m_StdoutRedirHandle = NewPipeRedir(stdout);
	m_StderrRedirHandle = NewPipeRedir(stderr);
	wxLocale::AddCatalogLookupPathPrefix( wxGetCwd() );

#define pxMessageBoxEventThing(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxMessageBoxEventFunction, &func )

	Connect( pxEVT_MSGBOX,			pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );
	Connect( pxEVT_CallStackBox,	pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );
	Connect( pxEVT_SemaphorePing,	wxCommandEventHandler( Pcsx2App::OnSemaphorePing ) );
	Connect( pxEVT_OpenModalDialog,	wxCommandEventHandler( Pcsx2App::OnOpenModalDialog ) );
	Connect( pxEVT_ReloadPlugins,	wxCommandEventHandler( Pcsx2App::OnReloadPlugins ) );
	Connect( pxEVT_SysExecute,		wxCommandEventHandler( Pcsx2App::OnSysExecute ) );

	Connect( pxEVT_LoadPluginsComplete,	wxCommandEventHandler( Pcsx2App::OnLoadPluginsComplete ) );

	Connect( pxEVT_CoreThreadStatus,	wxCommandEventHandler( Pcsx2App::OnCoreThreadStatus ) );
	Connect( pxEVT_FreezeThreadFinished,	wxCommandEventHandler( Pcsx2App::OnFreezeThreadFinished ) );

	Connect( pxID_PadHandler_Keydown, wxEVT_KEY_DOWN, wxKeyEventHandler( Pcsx2App::OnEmuKeyDown ) );

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
		ReadUserModeSettings();

		AppConfig_OnChangedSettingsFolder();

	    m_MainFrame		= new MainEmuFrame( NULL, L"PCSX2" );

	    if( m_ProgramLogBox )
	    {
			wxWindow* deleteme = m_ProgramLogBox;
			OnProgramLogClosed();
			delete deleteme;
			g_Conf->ProgLogBox.Visible = true;
		}

		m_ProgramLogBox	= new ConsoleLogFrame( m_MainFrame, L"PCSX2 Program Log", g_Conf->ProgLogBox );
		EnableAllLogging();
		
		SetTopWindow( m_MainFrame );	// not really needed...
		SetExitOnFrameDelete( true );	// but being explicit doesn't hurt...
	    m_MainFrame->Show();

		SysDetect();
		AppApplySettings();

		m_CoreAllocs = new SysCoreAllocations();

		if( m_CoreAllocs->HadSomeFailures( g_Conf->EmuOptions.Cpu.Recompiler ) )
		{
			wxString message( _("The following cpu recompilers failed to initialize and will not be available:\n\n") );

			if( !m_CoreAllocs->RecSuccess_EE )
			{
				message += L"\t* R5900 (EE)\n";
				g_Session.ForceDisableEErec = true;
			}

			if( !m_CoreAllocs->RecSuccess_IOP )
			{
				message += L"\t* R3000A (IOP)\n";
				g_Session.ForceDisableIOPrec = true;
			}

			if( !m_CoreAllocs->RecSuccess_VU0 )
			{
				message += L"\t* VU0\n";
				g_Session.ForceDisableVU0rec = true;
			}

			if( !m_CoreAllocs->RecSuccess_VU1 )
			{
				message += L"\t* VU1\n";
				g_Session.ForceDisableVU1rec = true;
			}

			message += pxE( ".Popup Error:EmuCore:MemoryForRecs",
				L"These errors are the result of memory allocation failures (see the program log for details). "
				L"Closing out some memory hogging background tasks may resolve this error.\n\n"
				L"These recompilers have been disabled and interpreters will be used in their place.  "
				L"Interpreters can be very slow, so don't get too excited.  Press OK to continue or CANCEL to close PCSX2."
			);

			if( !Msgbox::OkCancel( message, _("PCSX2 Initialization Error"), wxICON_ERROR ) )
				return false;
		}

		LoadPluginsPassive( NULL );
	}
	// ----------------------------------------------------------------------------
	catch( Exception::StartupAborted& ex )
	{
		Console.Notice( ex.FormatDiagnosticMessage() );
		CleanupMess();
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
		Msgbox::Alert( ex.FormatDisplayMessage() + L"\n\nPress OK to close PCSX2.",
			_("PCSX2 Critical Error"), wxICON_ERROR );
		return false;
	}
    return true;
}

void Pcsx2App::CleanupMess()
{
	CoreThread.Cancel();

	if( m_CorePlugins )
		m_CorePlugins->Shutdown();

	// Notice: deleting the plugin manager (unloading plugins) here causes Lilypad to crash,
	// likely due to some pending message in the queue that references lilypad procs.
	// We don't need to unload plugins anyway tho -- shutdown is plenty safe enough for
	// closing out all the windows.  So just leave it be and let the plugins get unloaded
	// during the wxApp destructor. -- air
	
	// FIXME: performing a wxYield() here may fix that problem. -- air

	while( wxGetLocale() != NULL )
		delete wxGetLocale();
}

Pcsx2App::Pcsx2App()  :
	m_MainFrame( NULL )
,	m_gsFrame( NULL )
,	m_ProgramLogBox( NULL )
,	m_ConfigImages( 32, 32 )
,	m_ConfigImagesAreLoaded( false )
,	m_ToolbarImages( NULL )
,	m_Bitmap_Logo( NULL )
{
	SetAppName( L"pcsx2" );
	BuildCommandHash();
}

Pcsx2App::~Pcsx2App()
{
	// Typically OnExit cleans everything up before we get here, *unless* we cancel
	// out of program startup in OnInit (return false) -- then remaining cleanup needs
	// to happen here in the destructor.

	CleanupMess();
	
	delete wxConfigBase::Set( NULL );
	
	Console_SetActiveHandler( ConsoleWriter_Null );
	
	if( emuLog != NULL )
	{
		fclose( emuLog );
		emuLog = NULL;
	}
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
// (but this tool is still still better than nothing!)

#ifdef PCSX2_DEBUG
struct CrtDebugBreak
{
	CrtDebugBreak( int spot )
	{
#ifndef __LINUX__
		_CrtSetBreakAlloc( spot );
#endif
	}
};

//CrtDebugBreak breakAt( 20603 );

#endif