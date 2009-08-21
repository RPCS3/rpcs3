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
#include "Dialogs/ModalPopups.h"

#include "Utilities/ScopedPtr.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include <wx/cmdline.h>
#include <wx/stdpaths.h>

IMPLEMENT_APP(Pcsx2App)

bool			UseAdminMode = false;
AppConfig*		g_Conf = NULL;
wxFileHistory*	g_RecentIsoList = NULL;
CoreEmuThread*	g_EmuThread = NULL;

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
		StartupAborted( const StartupAborted& src ) : BaseException( src ) {}

		explicit StartupAborted( const wxString& msg_eng=L"Startup initialization was aborted by the user." ) :
			BaseException( msg_eng, msg_eng ) { }	// english messages only for this exception.
	};
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

wxFrame* Pcsx2App::GetMainWindow() const { return m_MainFrame; }

#include "HashMap.h"

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
		FirstTimeWizard wiz( NULL );
		if( !wiz.RunWizard( wiz.GetFirstPage() ) )
			throw Exception::StartupAborted( L"Startup aborted: User canceled FirstTime Wizard." );

		// Save user's new settings
		IniSaver saver( *conf_usermode );
		g_Conf->LoadSaveUserMode( saver );
		g_Conf->Save();
	}
	else
	{
		// usermode.ini exists -- assume Documents mode, unless the ini explicitly
		// specifies otherwise.
		UseAdminMode = false;

		IniLoader loader( *conf_usermode );
		g_Conf->LoadSaveUserMode( loader );
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

void Pcsx2App::CleanupMess()
{
	safe_delete( g_RecentIsoList );
	safe_delete( m_Bitmap_Logo );
	safe_delete( g_Conf );
}

// ------------------------------------------------------------------------
bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();
	wxApp::OnInit();

	g_Conf = new AppConfig();
	g_EmuThread = new CoreEmuThread();

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

		AppConfig_ReloadGlobalSettings();

	    m_MainFrame		= new MainEmuFrame( NULL, L"PCSX2" );
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

	Connect( pxEVT_MSGBOX, wxCommandEventHandler( Pcsx2App::OnMessageBox ) );

    return true;
}

void Pcsx2App::OnMessageBox( wxCommandEvent& evt )
{
	Msgbox::OnEvent( evt );
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
	if( g_Conf != NULL )
		g_Conf->Save();

	CleanupMess();
	return wxApp::OnExit();
}

#include <wx/zipstrm.h>
#include <wx/wfstream.h>

// ------------------------------------------------------------------------
const wxImage& LoadImageAny(
	wxImage& dest, bool useTheme, wxFileName& base, const wxChar* filename, IEmbeddedImage& onFail )
{
	if( useTheme )
	{
		base.SetName( filename );

		base.SetExt( L"png" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}

		base.SetExt( L"jpg" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}

		base.SetExt( L"bmp" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}
	}

	return dest = onFail.Get();
}

// ------------------------------------------------------------------------
const wxBitmap& Pcsx2App::GetLogoBitmap()
{
	if( m_Bitmap_Logo != NULL )
		return *m_Bitmap_Logo;

	wxFileName mess;
	bool useTheme = (g_Conf->DeskTheme != L"default");

	if( useTheme )
	{
		wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
		wxFileName zipped( theme.GetFilename() );

		zipped.SetExt( L"zip" );
		if( zipped.FileExists() )
		{
			// loading theme from zipfile.
			//wxFileInputStream stream( zipped.ToString() )
			//wxZipInputStream zstream( stream );

			Console::Error( "Loading themes from zipfile is not supported yet.\nFalling back on default theme." );
		}

		// Overrides zipfile settings (fix when zipfile support added)
		mess = theme.ToString();
	}

	wxImage img;
	EmbeddedImage<png_BackgroundLogo> temp;	// because gcc can't allow non-const temporaries.
	LoadImageAny( img, useTheme, mess, L"BackgroundLogo", temp );
	m_Bitmap_Logo = new wxBitmap( img );

	return *m_Bitmap_Logo;
}

#include "Resources/ConfigIcon_Cpu.h"
#include "Resources/ConfigIcon_Video.h"
#include "Resources/ConfigIcon_Speedhacks.h"
#include "Resources/ConfigIcon_Gamefixes.h"
#include "Resources/ConfigIcon_Paths.h"
#include "Resources/ConfigIcon_Plugins.h"

// ------------------------------------------------------------------------
wxImageList& Pcsx2App::GetImgList_Config()
{
	if( !m_ConfigImagesAreLoaded )
	{
		wxFileName mess;
		bool useTheme = (g_Conf->DeskTheme != L"default");

		if( useTheme )
		{
			wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
			mess = theme.ToString();
		}

		wxImage img;

		// GCC Specific: wxT() macro is required when using string token pasting.  For some reason L
		// generates syntax errors. >_<

		#undef  FancyLoadMacro
		#define FancyLoadMacro( name ) \
		{ \
			EmbeddedImage<png_ConfigIcon_##name> temp( g_Conf->Listbook_ImageSize, g_Conf->Listbook_ImageSize ); \
			m_ImageId.Config.name = m_ConfigImages.Add( LoadImageAny( \
				img, useTheme, mess, L"ConfigIcon_" wxT(#name), temp ) \
			); \
		}

		FancyLoadMacro( Paths );
		FancyLoadMacro( Plugins );
		FancyLoadMacro( Gamefixes );
		FancyLoadMacro( Speedhacks );
		FancyLoadMacro( Video );
		FancyLoadMacro( Cpu );
	}
	m_ConfigImagesAreLoaded = true;
	return m_ConfigImages;
}

// ------------------------------------------------------------------------
wxImageList& Pcsx2App::GetImgList_Toolbars()
{
	if( m_ToolbarImages == NULL )
	{
		const int imgSize = g_Conf->Toolbar_ImageSize ? 64 : 32;
		m_ToolbarImages = new wxImageList( imgSize, imgSize );
		wxFileName mess;
		bool useTheme = (g_Conf->DeskTheme != L"default");

		if( useTheme )
		{
			wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
			mess = theme.ToString();
		}

		wxImage img;
		#undef  FancyLoadMacro
		#define FancyLoadMacro( name ) \
		{ \
			EmbeddedImage<png_ToolbarIcon_##name> temp( imgSize, imgSize ); \
			m_ImageId.Toolbars.name = m_ConfigImages.Add( LoadImageAny( img, useTheme, mess, L"ToolbarIcon" wxT(#name), temp ) ); \
		}

	}
	return *m_ToolbarImages;
}

