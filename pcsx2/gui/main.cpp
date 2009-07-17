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

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include <wx/cmdline.h>

IMPLEMENT_APP(Pcsx2App)

AppConfig* g_Conf = NULL;
wxFileHistory* g_RecentIsoList = NULL;

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

wxFileConfig* OpenConfig( const wxString& filename )
{
	return new wxFileConfig( wxEmptyString, wxEmptyString, filename, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH );
}

void Pcsx2App::ReadUserModeSettings()
{
	wxString configfile( Path::Combine( wxGetCwd(), L"usermode.ini" ) );

	if( !wxFile::Exists( configfile ) )
	{
		Dialogs::PickUserModeDialog( m_MainFrame ).ShowModal();
	}

	wxFileConfig* conf_usermode = OpenConfig( Path::Combine( wxGetCwd(), L"usermode.ini" ) );

	// Ensure proper scoping (IniLoader gets closed prior to delete)
	{
		IniLoader loader( *conf_usermode );
		g_Conf->LoadSaveUserMode( loader );
	}

	delete conf_usermode;
}

// ------------------------------------------------------------------------
// returns true if a configuration file is present in the current working dir (cwd).
// returns false if not (in which case the calling code should fall back on using OpenConfigUserLocal())
//
bool Pcsx2App::TryOpenConfigCwd()
{
	wxDirName inipath_cwd( (wxDirName)wxGetCwd() + PathDefs::Base::Settings() );
	if( !inipath_cwd.IsReadable() ) return false;

	wxString inifile_cwd( Path::Combine( inipath_cwd, FilenameDefs::GetConfig() ) );
	if( !Path::IsFile( inifile_cwd ) ) return false;
	if( Path::GetFileSize( inifile_cwd ) <= 1 ) return false;

	wxConfigBase::Set( OpenConfig( inifile_cwd ) );
	return true;
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
	parser.AddOption( wxEmptyString, L"pad",		L"uses filename as *both* PAD plugins for this session only." );
	parser.AddOption( wxEmptyString, L"pad1",		L"uses filename as the PAD1 plugin for this session only." );
	parser.AddOption( wxEmptyString, L"pad2",		L"uses filename as the PAD2 plugin for this session only." );
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

//////////////////////////////////////////////////////////////////////////////////////////
// ConsoleThreadTest -- useful class for unit testing the thread safety and general performance
// of the console logger.
#if 0
class ConsoleTestThread : public Thread
{
	int Callback()
	{
		static int numtrack = 0;
		
		while( true )
		{
			Console::WriteLn( wxsFormat( L"This is a threaded logging test.  Something bad could happen... %d", ++numtrack ) );
			Sleep( 0 );
		}
	}
};
#endif

// ------------------------------------------------------------------------
bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();
	wxApp::OnInit();

	g_Conf = new AppConfig();

	i18n_InitPlainEnglish();
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

	if( !TryOpenConfigCwd() )
	{
		PathDefs::GetDocuments().Mkdir();
		PathDefs::GetSettings().Mkdir();

		// Allow wx to use our config, and enforces auto-cleanup as well
		wxString confile( Path::Combine( PathDefs::GetSettings(), FilenameDefs::GetConfig() ) );
		wxConfigBase::Set( OpenConfig( confile ) );
		wxConfigBase::Get()->SetRecordDefaults();
	}

	g_Conf->Load();
	g_Conf->Apply();
	
	m_ProgramLogBox = new ConsoleLogFrame( NULL, L"PCSX2 Program Log" );
	m_Ps2ConLogBox = m_ProgramLogBox;
	//m_Ps2ConLogBox = new ConsoleLogFrame( NULL, L"PS2 Console Log" );

    m_MainFrame = new MainEmuFrame( NULL, wxID_ANY, wxEmptyString );
    SetTopWindow( m_MainFrame );
    SetExitOnFrameDelete( true );
    m_MainFrame->Show();
    
    //ConsoleTestThread* woo = new ConsoleTestThread();
    //woo->Start();

	// Check to see if the user needs to perform initial setup:

	/*bool needsConfigured = false;
	const wxString pc( L"Please Configure" );
	for( int pidx=0; pidx<Plugin_Count; ++pidx )
	{
		if( g_Conf->BaseFilenames[(PluginsEnum_t)pidx] == pc )
		{
			needsConfigured = true;
			break;
		}
	}

	if( needsConfigured )
	{
		Dialogs::ConfigurationDialog( m_MainFrame ).ShowModal();
	}*/

    return true;
}

int Pcsx2App::OnExit()
{
	g_Conf->Save();
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

