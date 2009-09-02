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
#include "IniInterface.h"
#include "MainFrame.h"
#include "MemoryCard.h"
#include "Plugins.h"

#include "Dialogs/ModalPopups.h"
#include "Utilities/ScopedPtr.h"

#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/evtloop.h>

IMPLEMENT_APP(Pcsx2App)

bool			UseAdminMode = false;
AppConfig*		g_Conf = NULL;
wxFileHistory*	g_RecentIsoList = NULL;

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

AppEmuThread::AppEmuThread( const wxString& elf_file ) :
	CoreEmuThread( elf_file )
{
	MemoryCard::Init();
}

void AppEmuThread::Resume()
{
	if( wxGetApp().GetMainFrame().IsPaused() ) return;
	CoreEmuThread::Resume();
}

sptr AppEmuThread::ExecuteTask()
{
	try
	{
		CoreEmuThread::ExecuteTask();
	}
	// ----------------------------------------------------------------------------
	catch( Exception::FileNotFound& ex )
	{
		if( ex.StreamName == g_Conf->FullpathToBios() )
		{
			Msgbox::OkCancel( ex.FormatDisplayMessage() +
				_("\n\nPress Ok to go to the BIOS Configuration Panel.") );
		}
		else
		{
			// Probably a plugin.  Find out which one!
			
			const PluginInfo* pi = tbl_PluginInfo-1;
			while( ++pi, pi->shortname != NULL )
			{
				const PluginsEnum_t pid = pi->id;
				if( g_Conf->FullpathTo( pid ) == ex.StreamName ) break;
			}

			if( pi->shortname == NULL )
			{
				// Some other crap file failure >_<
			}
			
			int result = Msgbox::OkCancel( ex.FormatDisplayMessage() +
				_("\n\nPress Ok to go to the Plugin Configuration Panel.") );
				
			if( result == wxID_OK )
			{
			}
		}
	}
	// ----------------------------------------------------------------------------
	// [TODO] : Add exception handling here for debuggable PS2 exceptions that allows
	// invocation of the PCSX2 debugger and such.
	catch( Exception::BaseException& ex )
	{
		// Sent the exception back to the main gui thread?
		GetPluginManager().Close();
		Msgbox::Alert( ex.FormatDisplayMessage() );
	}
	
	return 0;
}


wxFrame* Pcsx2App::GetMainWindow() const { return m_MainFrame; }

#include "HashMap.h"

void Pcsx2App::OpenWizardConsole()
{
	if( !IsDebugBuild ) return;
	g_Conf->ProgLogBox.Visible = true;
	m_ProgramLogBox	= new ConsoleLogFrame( NULL, L"PCSX2 Program Log", g_Conf->ProgLogBox );
}

// User mode settings can't be stores in the CWD for two reasons:
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
		usrlocaldir.Mkdir();

	wxFileName usermodefile( FilenameDefs::GetUsermodeConfig() );
	usermodefile.SetPath( usrlocaldir.ToString() );
	wxScopedPtr<wxFileConfig> conf_usermode( OpenFileConfig( usermodefile.GetFullPath() ) );

	wxString groupname( wxsFormat( L"CWD.%08x", hashres ) );

	if( !conf_usermode->HasGroup( groupname ) )
	{
		// first time startup, so give the user the choice of user mode:
		OpenWizardConsole();
		FirstTimeWizard wiz( NULL );
		if( !wiz.RunWizard( wiz.GetUsermodePage() ) )
			throw Exception::StartupAborted( L"Startup aborted: User canceled FirstTime Wizard." );

		// Save user's new settings
		IniSaver saver( *conf_usermode );
		g_Conf->LoadSaveUserMode( saver, groupname );
		g_Conf->Save();
	}
	else
	{
		// usermode.ini exists -- assume Documents mode, unless the ini explicitly
		// specifies otherwise.
		UseAdminMode = false;

		IniLoader loader( *conf_usermode );
		g_Conf->LoadSaveUserMode( loader, groupname );
		
		if( !wxFile::Exists( g_Conf->FullPathToConfig() ) )
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
			g_Conf->Save();
		}
	}
}

void Pcsx2App::OnInitCmdLine( wxCmdLineParser& parser )
{
	parser.SetLogo( L" >> Pcsx2  --  A Playstation2 Emulator for the PC\n");

	parser.AddParam( L"CDVD/ELF", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL );

	parser.AddSwitch( L"h",		L"help",	L"displays this list of command line options", wxCMD_LINE_OPTION_HELP );
	parser.AddSwitch( L"nogui",	L"nogui",	L"disables display of the gui and enables the Escape Hack." );

	parser.AddOption( L"bootmode",	wxEmptyString,	L"0 - quick (default), 1 - bios, 2 - load elf", wxCMD_LINE_VAL_NUMBER );
	parser.AddOption( wxEmptyString,L"cfg",			L"configuration file override", wxCMD_LINE_VAL_STRING );

	parser.AddOption( wxEmptyString, L"cdvd",		L"uses filename as the CDVD plugin for this session only." );
	parser.AddOption( wxEmptyString, L"gs",			L"uses filename as the GS plugin for this session only." );
	parser.AddOption( wxEmptyString, L"spu",		L"uses filename as the SPU2 plugin for this session only." );
	parser.AddOption( wxEmptyString, L"pad",		L"uses filename as the PAD plugin for this session only." );
	parser.AddOption( wxEmptyString, L"dev9",		L"uses filename as the DEV9 plugin for this session only." );
	parser.AddOption( wxEmptyString, L"usb",		L"uses filename as the USB plugin for this session only." );

	parser.SetSwitchChars( L"-" );
}

bool Pcsx2App::OnCmdLineParsed(wxCmdLineParser& parser)
{
	if( parser.GetParamCount() >= 1 )
	{
		// [TODO] : Unnamed parameter is taken as an "autorun" option for a cdvd/iso.
		parser.GetParam( 0 );
	}

	// Suppress wxWidgets automatic options parsing since none of them pertain to Pcsx2 needs.
	//wxApp::OnCmdLineParsed( parser );

	bool yay = parser.Found(L"nogui");

	return true;
}

typedef void (wxEvtHandler::*pxMessageBoxEventFunction)(pxMessageBoxEvent&);

// ------------------------------------------------------------------------
bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();
	wxApp::OnInit();

	g_Conf = new AppConfig();

	wxLocale::AddCatalogLookupPathPrefix( wxGetCwd() );

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
		ReadUserModeSettings();
		delete wxLog::SetActiveTarget( new pxLogConsole() );

		AppConfig_ReloadGlobalSettings();

	    m_MainFrame		= new MainEmuFrame( NULL, L"PCSX2" );

	    if( m_ProgramLogBox )
	    {
			delete m_ProgramLogBox;
			g_Conf->ProgLogBox.Visible = true;
		}

		m_ProgramLogBox	= new ConsoleLogFrame( m_MainFrame, L"PCSX2 Program Log", g_Conf->ProgLogBox );

		m_Ps2ConLogBox	= m_ProgramLogBox;		// just use a single logger for now.
		//m_Ps2ConLogBox = new ConsoleLogFrame( NULL, L"PS2 Console Log" );

		SetTopWindow( m_MainFrame );	// not really needed...
		SetExitOnFrameDelete( true );	// but being explicit doesn't hurt...
	    m_MainFrame->Show();

		SysInit();
	}
	catch( Exception::StartupAborted& )
	{
		// Note: wx does not call OnExit() when returning false.
		CleanupMess();
		return false;
	}

	#define pxMessageBoxEventThing(func) \
		(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxMessageBoxEventFunction, &func )

	Connect( pxEVT_MSGBOX,		pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );
	Connect( pxEVT_CallStackBox,pxMessageBoxEventThing( Pcsx2App::OnMessageBox ) );

    return true;
}

void Pcsx2App::OnMessageBox( pxMessageBoxEvent& evt )
{
	Msgbox::OnEvent( evt );
}

void Pcsx2App::CleanupMess()
{
	safe_delete( g_RecentIsoList );
	safe_delete( m_Bitmap_Logo );
	safe_delete( g_Conf );
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
// to handle window closures)
bool Pcsx2App::PrepForExit()
{
	return true;
}

int Pcsx2App::OnExit()
{
	m_ProgramLogBox = NULL;
	MemoryCard::Shutdown();

	if( g_Conf != NULL )
		g_Conf->Save();

	CleanupMess();
	return wxApp::OnExit();
}

Pcsx2App::Pcsx2App()  :
	m_ProgramLogBox( NULL )
,	m_Ps2ConLogBox( NULL )
,	m_ConfigImages( 32, 32 )
,	m_ConfigImagesAreLoaded( false )
,	m_ToolbarImages( NULL )
,	m_Bitmap_Logo( NULL )
{
	SetAppName( L"pcsx2" );
}

Pcsx2App::~Pcsx2App()
{
	CleanupMess();
}


