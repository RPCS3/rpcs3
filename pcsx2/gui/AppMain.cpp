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
#include "Plugins.h"
#include "SaveState.h"
#include "ConsoleLogger.h"

#include "Dialogs/ModalPopups.h"
#include "Dialogs/ConfigurationDialog.h"
#include "Dialogs/LogOptionsDialog.h"

#include "Utilities/ScopedPtr.h"
#include "Utilities/HashMap.h"

#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/intl.h>

IMPLEMENT_APP(Pcsx2App)

DEFINE_EVENT_TYPE( pxEVT_SemaphorePing );
DEFINE_EVENT_TYPE( pxEVT_OpenModalDialog );
DEFINE_EVENT_TYPE( pxEVT_ReloadPlugins );
DEFINE_EVENT_TYPE( pxEVT_LoadPluginsComplete );

bool					UseAdminMode = false;
wxDirName				SettingsFolder;
bool					UseDefaultSettingsFolder = true;

wxScopedPtr<AppConfig>	g_Conf;
ConfigOverrides			OverrideOptions;

namespace Exception
{
	// --------------------------------------------------------------------------
	// Exception used to perform an "errorless" termination of the app during OnInit
	// procedures.  This happens when a user cancels out of startup prompts/wizards.
	//
	class StartupAborted : public BaseException
	{
	public:
		virtual ~StartupAborted() throw() {}

		StartupAborted( const wxString& msg_eng=L"Startup initialization was aborted by the user." )
		{
			// english messages only for this exception.
			BaseException::InitBaseEx( msg_eng, msg_eng );
		}
	};
}

AppEmuThread::AppEmuThread( PluginManager& plugins ) :
	SysCoreThread( plugins )
,	m_kevt()
{
}

AppEmuThread::~AppEmuThread() throw()
{
	AppInvoke( MainFrame, ApplySettings() );
}

void AppEmuThread::Suspend( bool isBlocking )
{
	SysCoreThread::Suspend( isBlocking );
	AppInvoke( MainFrame, ApplySettings() );
}

void AppEmuThread::Resume()
{
	// Clear the sticky key statuses, because hell knows what's changed while the PAD
	// plugin was suspended.

	m_kevt.m_shiftDown		= false;
	m_kevt.m_controlDown	= false;
	m_kevt.m_altDown		= false;

	ApplySettings( g_Conf->EmuOptions );

	if( GSopen2 != NULL )
		wxGetApp().OpenGsFrame();

	SysCoreThread::Resume();
	AppInvoke( MainFrame, ApplySettings() );
}

// This is used when the GS plugin is handling its own window.  Messages from the PAD
// are piped through to an app-level message handler, which dispatches them through
// the universal Accelerator table.
static const int pxID_PadHandler_Keydown = 8030;

#ifdef __WXGTK__
	extern int TranslateGDKtoWXK( u32 keysym );
#endif

void AppEmuThread::StateCheck()
{
	SysCoreThread::StateCheck();

	const keyEvent* ev = PADkeyEvent();

	if( ev == NULL || (ev->key == 0) ) return;

	GetPluginManager().KeyEvent( *ev );

	m_kevt.SetEventType( ( ev->evt == KEYPRESS ) ? wxEVT_KEY_DOWN : wxEVT_KEY_UP );

	const bool isDown = (ev->evt == KEYPRESS);

	#ifdef __WXMSW__
		const int vkey = wxCharCodeMSWToWX( ev->key );
	#elif defined( __WXGTK__ )
		const int vkey = TranslateGDKtoWXK( ev->key );
	#else
	#	error Unsupported Target Platform.
	#endif

	switch (vkey)
	{
		case WXK_SHIFT:		m_kevt.m_shiftDown		= isDown; return;
		case WXK_CONTROL:	m_kevt.m_controlDown	= isDown; return;
		case WXK_MENU:		m_kevt.m_altDown		= isDown; return;
	}

	m_kevt.m_keyCode = vkey;
	wxGetApp().PostPadKey( m_kevt );
}

__forceinline bool EmulationInProgress()
{
	return wxGetApp().EmuInProgress();
}

__forceinline bool SysHasValidState()
{
	return wxGetApp().EmuInProgress() || StateRecovery::HasState();
}


bool HandlePluginError( Exception::PluginError& ex )
{
	if( pxDialogExists( DialogId_CoreSettings ) ) return true;

	bool result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
		_("\n\nPress Ok to go to the Plugin Configuration Panel.")
	);

	if( result )
	{
		g_Conf->SettingsTabName = L"Plugins";

		// fixme: Send a message to the panel to select the failed plugin.
		if( Dialogs::ConfigurationDialog().ShowModal() == wxID_CANCEL )
			return false;
	}
	return result;
}

sptr AppEmuThread::ExecuteTask()
{
	try
	{
		SysCoreThread::ExecuteTask();
	}
	// ----------------------------------------------------------------------------
	catch( Exception::FileNotFound& ex )
	{
		m_plugins.Close();
		if( ex.StreamName == g_Conf->FullpathToBios() )
		{
			GetPluginManager().Close();
			bool result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
				_("\n\nPress Ok to go to the BIOS Configuration Panel.") );

			if( result )
			{
				if( wxGetApp().ThreadedModalDialog( DialogId_BiosSelector ) == wxID_CANCEL )
				{
					// fixme: handle case where user cancels the settings dialog. (should return FALSE).
				}
				else
				{
					// fixme: automatically re-try emu startup here...
				}
			}
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::PluginError& ex )
	{
		m_plugins.Close();
		Console::Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage(), _("Plugin Open Error") );

		/*if( HandlePluginError( ex ) )
		{
			// fixme: automatically re-try emu startup here...
		}*/
	}
	// ----------------------------------------------------------------------------
	// [TODO] : Add exception handling here for debuggable PS2 exceptions that allows
	// invocation of the PCSX2 debugger and such.
	//
	catch( Exception::BaseException& ex )
	{
		// Sent the exception back to the main gui thread?
		m_plugins.Close();
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}

	return 0;
}


void Pcsx2App::OpenWizardConsole()
{
	if( !IsDebugBuild ) return;
	g_Conf->ProgLogBox.Visible = true;
	m_ProgramLogBox	= new ConsoleLogFrame( NULL, L"PCSX2 Program Log", g_Conf->ProgLogBox );
}

static bool m_ForceWizard = false;

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
		Console::Status( L"Creating UserLocalData folder: " + usrlocaldir.ToString() );
		usrlocaldir.Mkdir();
	}

	wxFileName usermodefile( FilenameDefs::GetUsermodeConfig() );
	usermodefile.SetPath( usrlocaldir.ToString() );
	wxScopedPtr<wxFileConfig> conf_usermode( OpenFileConfig( usermodefile.GetFullPath() ) );

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
			Console::Notice( pi->GetShortname() + L" override: " + dest );
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
    wxInitAllImageHandlers();
	if( !wxApp::OnInit() ) return false;

	g_Conf.reset( new AppConfig() );

	m_StdoutRedirHandle.reset( NewPipeRedir(stdout) );
	m_StderrRedirHandle.reset( NewPipeRedir(stderr) );
	wxLocale::AddCatalogLookupPathPrefix( wxGetCwd() );

#define pxMessageBoxEventThing(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxMessageBoxEventFunction, &func )

	Connect( pxEVT_MSGBOX,			pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );
	Connect( pxEVT_CallStackBox,	pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );
	Connect( pxEVT_SemaphorePing,	wxCommandEventHandler( Pcsx2App::OnSemaphorePing ) );
	Connect( pxEVT_OpenModalDialog,	wxCommandEventHandler( Pcsx2App::OnOpenModalDialog ) );
	Connect( pxEVT_ReloadPlugins,	wxCommandEventHandler( Pcsx2App::OnReloadPlugins ) );
	Connect( pxEVT_LoadPluginsComplete,	wxCommandEventHandler( Pcsx2App::OnLoadPluginsComplete ) );

	Connect( pxID_PadHandler_Keydown, wxEVT_KEY_DOWN, wxKeyEventHandler( Pcsx2App::OnEmuKeyDown ) );

	// User/Admin Mode Dual Setup:
	//   Pcsx2 now supports two fundamental modes of operation.  The default is Classic mode,
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
			delete m_ProgramLogBox;
			g_Conf->ProgLogBox.Visible = true;
		}

		m_ProgramLogBox	= new ConsoleLogFrame( m_MainFrame, L"PCSX2 Program Log", g_Conf->ProgLogBox );

		SetTopWindow( m_MainFrame );	// not really needed...
		SetExitOnFrameDelete( true );	// but being explicit doesn't hurt...
	    m_MainFrame->Show();

		SysDetect();
		AppApplySettings();

		m_CoreAllocs.reset( new SysCoreAllocations() );

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

		LoadPluginsPassive();
	}
	// ----------------------------------------------------------------------------
	catch( Exception::StartupAborted& ex )
	{
		Console::Notice( ex.FormatDiagnosticMessage() );
		return false;
	}
	// ----------------------------------------------------------------------------
	// Failures on the core initialization procedure (typically OutOfMemory errors) are bad,
	// since it means the emulator is completely non-functional.  Let's pop up an error and
	// exit gracefully-ish.
	//
	catch( Exception::RuntimeError& ex )
	{
		Console::Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage() + L"\n\nPress OK to close PCSX2.",
			_("PCSX2 Critical Error"), wxICON_ERROR );
		return false;
	}
    return true;
}

// Allows for activating menu actions from anywhere in PCSX2.
// And it's Thread Safe!
void Pcsx2App::PostMenuAction( MenuIdentifiers menu_id ) const
{
	wxASSERT( m_MainFrame != NULL );
	if( m_MainFrame == NULL ) return;

	wxCommandEvent joe( wxEVT_COMMAND_MENU_SELECTED, menu_id );
	if( wxThread::IsMain() )
		m_MainFrame->GetEventHandler()->ProcessEvent( joe );
	else
		m_MainFrame->GetEventHandler()->AddPendingEvent( joe );
}

void Pcsx2App::PostPadKey( wxKeyEvent& evt )
{
	if( m_gsFrame == NULL )
	{
		evt.SetId( pxID_PadHandler_Keydown );
		wxGetApp().AddPendingEvent( evt );
	}
	else
	{
		evt.SetId( m_gsFrame->GetId() );
		m_gsFrame->AddPendingEvent( evt );
	}
}

int Pcsx2App::ThreadedModalDialog( DialogIdentifiers dialogId )
{
	wxASSERT( !wxThread::IsMain() );		// don't call me from MainThread!

	MsgboxEventResult result;
	wxCommandEvent joe( pxEVT_OpenModalDialog, dialogId );
	joe.SetClientData( &result );
	AddPendingEvent( joe );
	result.WaitForMe.WaitNoCancel();
	return result.result;
}

// Waits for the main GUI thread to respond.  If run from the main GUI thread, returns
// immediately without error.  Use this on non-GUI threads to have them sleep until
// the GUI has processed all its pending messages.
void Pcsx2App::Ping() const
{
	if( wxThread::IsMain() ) return;

	Semaphore sema;
	wxCommandEvent bean( pxEVT_SemaphorePing );
	bean.SetClientData( &sema );
	wxGetApp().AddPendingEvent( bean );
	sema.WaitNoCancel();
}

// ----------------------------------------------------------------------------
//         Pcsx2App Event Handlers
// ----------------------------------------------------------------------------

void Pcsx2App::OnSemaphorePing( wxCommandEvent& evt )
{
	((Semaphore*)evt.GetClientData())->Post();
}

void Pcsx2App::OnOpenModalDialog( wxCommandEvent& evt )
{
	using namespace Dialogs;

	MsgboxEventResult& evtres( *((MsgboxEventResult*)evt.GetClientData()) );
	switch( evt.GetId() )
	{
		case DialogId_CoreSettings:
		{
			static int _guard = 0;
			EntryGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = ConfigurationDialog().ShowModal();
		}
		break;

		case DialogId_BiosSelector:
		{
			static int _guard = 0;
			EntryGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = BiosSelectorDialog().ShowModal();
		}
		break;

		case DialogId_LogOptions:
		{
			static int _guard = 0;
			EntryGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = LogOptionsDialog().ShowModal();
		}
		break;

		case DialogId_About:
		{
			static int _guard = 0;
			EntryGuard guard( _guard );
			if( guard.IsReentrant() ) return;
			evtres.result = AboutBoxDialog().ShowModal();
		}
		break;
	}

	evtres.WaitForMe.Post();
}

void Pcsx2App::OnMessageBox( pxMessageBoxEvent& evt )
{
	Msgbox::OnEvent( evt );
}

HashTools::HashMap<int, const GlobalCommandDescriptor*> GlobalAccels( 0, 0xffffffff );

void Pcsx2App::OnEmuKeyDown( wxKeyEvent& evt )
{
	const GlobalCommandDescriptor* cmd = NULL;
	GlobalAccels.TryGetValue( KeyAcceleratorCode( evt ).val32, cmd );
	if( cmd == NULL )
	{
		evt.Skip();
		return;
	}

	if( cmd != NULL )
	{
		DbgCon::WriteLn( "(app) Invoking command: %s", cmd->Id );
		cmd->Invoke();
	}

}

void Pcsx2App::CleanupMess()
{
	if( m_CorePlugins )
	{
		m_CorePlugins->Close();
		m_CorePlugins->Shutdown();
	}

	// Notice: deleting the plugin manager (unloading plugins) here causes Lilypad to crash,
	// likely due to some pending message in the queue that references lilypad procs.
	// We don't need to unload plugins anyway tho -- shutdown is plenty safe enough for
	// closing out all the windows.  So just leave it be and let the plugins get unloaded
	// during the wxApp destructor. -- air

	m_ProgramLogBox = NULL;
	m_MainFrame = NULL;
}

void Pcsx2App::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const
{
	try
	{
		(handler->*func)(event);
	}
	// ----------------------------------------------------------------------------
	catch( Exception::PluginError& ex )
	{
		Console::Error( ex.FormatDiagnosticMessage() );
		if( !HandlePluginError( ex ) )
		{
			Console::Error( L"User-canceled plugin configuration after load failure.  Plugins not loaded!" );
			Msgbox::Alert( _("Warning!  Plugins have not been loaded.  PCSX2 will be inoperable.") );
		}
	}
	// ----------------------------------------------------------------------------
	catch( Exception::RuntimeError& ex )
	{
		// Runtime errors which have been unhandled should still be safe to recover from,
		// so lets issue a message to the user and then continue the message pump.

		Console::Error( ex.FormatDiagnosticMessage() );
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}
}


// Common exit handler which can be called from any event (though really it should
// be called only from CloseWindow handlers since that's the more appropriate way
// to handle cancelable window closures)
//
// returns true if the app can close, or false if the close event was canceled by
// the glorious user, whomever (s)he-it might be.
bool Pcsx2App::PrepForExit()
{
	m_CoreThread.reset();
	CleanupMess();

	return true;
}

int Pcsx2App::OnExit()
{
	PrepForExit();

	if( g_Conf )
		AppSaveSettings();

	while( wxGetLocale() != NULL )
		delete wxGetLocale();

	return wxApp::OnExit();
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
}

void AppApplySettings( const AppConfig* oldconf )
{
	DevAssert( wxThread::IsMain(), "ApplySettings valid from the GUI thread only." );

	// Ensure existence of necessary documents folders.  Plugins and other parts
	// of PCSX2 rely on them.

	g_Conf->Folders.MemoryCards.Mkdir();
	g_Conf->Folders.Savestates.Mkdir();
	g_Conf->Folders.Snapshots.Mkdir();

	g_Conf->EmuOptions.BiosFilename = g_Conf->FullpathToBios();

	// Update the compression attribute on the Memcards folder.
	// Memcards generally compress very well via NTFS compression.

	NTFS_CompressFile( g_Conf->Folders.MemoryCards.ToString(), g_Conf->McdEnableNTFS );

	if( (oldconf == NULL) || (oldconf->LanguageId != g_Conf->LanguageId) )
	{
		wxDoNotLogInThisScope please;
		if( !i18n_SetLanguage( g_Conf->LanguageId ) )
		{
			if( !i18n_SetLanguage( wxLANGUAGE_DEFAULT ) )
			{
				i18n_SetLanguage( wxLANGUAGE_ENGLISH );
			}
		}
	}

	AppInvoke( MainFrame, ApplySettings() );
	AppInvoke( CoreThread, ApplySettings( g_Conf->EmuOptions ) );
}

void AppLoadSettings()
{
	wxConfigBase* conf = wxConfigBase::Get( false );
	if( NULL == conf ) return;

	IniLoader loader( *conf );
	g_Conf->LoadSave( loader );

	AppInvoke( MainFrame, LoadRecentIsoList( *conf ) );
}

void AppSaveSettings()
{
	wxConfigBase* conf = wxConfigBase::Get( false );
	if( NULL == conf ) return;

	IniSaver saver( *conf );
	g_Conf->LoadSave( saver );

	AppInvoke( MainFrame, SaveRecentIsoList( *conf ) );
}

// --------------------------------------------------------------------------------------
//  Sys/Core API and Shortcuts (for wxGetApp())
// --------------------------------------------------------------------------------------

// Executes the emulator using a saved/existing virtual machine state and currently
// configured CDVD source device.
void Pcsx2App::SysExecute()
{
	SysReset();
	LoadPluginsImmediate();
	m_CoreThread.reset( new AppEmuThread( *m_CorePlugins ) );
	m_CoreThread->Resume();
}

// Executes the specified cdvd source and optional elf file.  This command performs a
// full closure of any existing VM state and starts a fresh VM with the requested
// sources.
void Pcsx2App::SysExecute( CDVD_SourceType cdvdsrc )
{
	SysReset();
	LoadPluginsImmediate();
	CDVDsys_SetFile( CDVDsrc_Iso, g_Conf->CurrentIso );
	CDVDsys_ChangeSource( cdvdsrc );

	m_CoreThread.reset( new AppEmuThread( *m_CorePlugins ) );
	m_CoreThread->Resume();
}

void Pcsx2App::OpenGsFrame()
{
	if( m_gsFrame != NULL ) return;

	m_gsFrame = new GSFrame( m_MainFrame, L"PCSX2" );
	m_gsFrame->SetFocus();
	pDsp = (uptr)m_gsFrame->GetHandle();
	m_gsFrame->Show();

	// The "in the main window" quickie hack...
	//pDsp = (uptr)m_MainFrame->m_background.GetHandle();
}

void Pcsx2App::OnGsFrameClosed()
{
	if( m_CoreThread != NULL )
		m_CoreThread->Suspend();
	m_gsFrame = NULL;
}


// Writes text to console and updates the window status bar and/or HUD or whateverness.
void SysStatus( const wxString& text )
{
	// mirror output to the console!
	Console::Status( text.c_str() );
	AppInvoke( MainFrame, SetStatusText( text ) );
}

// Executes the emulator using a saved/existing virtual machine state and currently
// configured CDVD source device.
void SysExecute()
{
	wxGetApp().SysExecute();
}

void SysExecute( CDVD_SourceType cdvdsrc )
{
	wxGetApp().SysExecute( cdvdsrc );
}

void SysResume()
{
	AppInvoke( CoreThread, Resume() );
}

void SysSuspend()
{
	AppInvoke( CoreThread, Suspend() );
}

void SysReset()
{
	wxGetApp().SysReset();
}
