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

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include <wx/cmdline.h>

IMPLEMENT_APP(Pcsx2App)

AppConfig g_Conf;

const wxRect wxDefaultRect( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord );

Pcsx2App::Pcsx2App()  :
	m_ConsoleFrame( NULL )
{
	SetAppName( L"Pcsx2" );
}

wxFrame* Pcsx2App::GetMainWindow() const { return m_MainFrame; }

wxFileConfig* OpenConfig( const wxString& filename )
{
	return new wxFileConfig( wxEmptyString, wxEmptyString, filename, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH );
}

// returns true if a configuration file is present in the current working dir (cwd).
// returns false if not (in which case the calling code should fall back on using OpenConfigUserLocal())
bool Pcsx2App::TryOpenConfigCwd()
{
	wxDirName inipath_cwd( (wxDirName)wxGetCwd() + PathDefs::Configs );
	if( !inipath_cwd.IsReadable() ) return false;

	wxString inifile_cwd( Path::Combine( inipath_cwd, FilenameDefs::GetConfig() ) );
	if( !Path::IsFile( inifile_cwd ) ) return false;
	if( Path::GetFileSize( inifile_cwd ) <= 1 ) return false;

	wxConfigBase::Set( OpenConfig( inifile_cwd ) );
	return true;
}

void Pcsx2App::OnInitCmdLine( wxCmdLineParser& parser )
{
	parser.SetLogo( _(
		" >> Pcsx2  --  A Playstation2 Emulator for the PC\n"
	) );

	parser.AddParam( _( "CDVD/ELF" ), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL );

	parser.AddSwitch( wxT("h"),		wxT("help"),	_("displays this list of command line options"), wxCMD_LINE_OPTION_HELP );
	parser.AddSwitch( wxT("nogui"),	wxT("nogui"),	_("disables display of the gui and enables the Escape Hack.") );

	parser.AddOption( wxT("bootmode"),	wxEmptyString,	_("0 - quick (default), 1 - bios, 2 - load elf"), wxCMD_LINE_VAL_NUMBER );
	parser.AddOption( wxEmptyString,	wxT("cfg"),		_("configuration file override"), wxCMD_LINE_VAL_STRING );

	parser.AddOption( wxEmptyString,wxT("cdvd"),	_("uses filename as the CDVD plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("gs"),		_("uses filename as the GS plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("spu"),		_("uses filename as the SPU2 plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("pad"),		_("uses filename as *both* PAD plugins for this session only.") );
	parser.AddOption( wxEmptyString,wxT("pad1"),	_("uses filename as the PAD1 plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("pad2"),	_("uses filename as the PAD2 plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("dev9"),	_("uses filename as the DEV9 plugin for this session only.") );
	parser.AddOption( wxEmptyString,wxT("usb"),		_("uses filename as the USB plugin for this session only.") );

	parser.SetSwitchChars( wxT("-") );
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

	bool yay = parser.Found( wxT("nogui") );

	return true;
}

bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();

	wxApp::OnInit();

	// Ini Startup:  The ini file could be in one of two locations, depending on how Pcsx2 has
	// been installed or configured.  The first place we look is in our program's working
	// directory.  If the ini there exist, and is *not* empty, then we'll use it.  Otherwise
	// we fall back on the ini file in the user's Documents folder.
	
	if( !TryOpenConfigCwd() )
	{
		PathDefs::GetDocuments().Mkdir();
		PathDefs::GetConfigs().Mkdir();

		// Allow wx to use our config, and enforces auto-cleanup as well
		wxConfigBase::Set( OpenConfig( Path::Combine( PathDefs::GetConfigs(), FilenameDefs::GetConfig() ) ) );
		wxConfigBase::Get()->SetRecordDefaults();
	}

	g_Conf.LoadSave( IniLoader() );

	m_Bitmap_Logo = new wxBitmap( EmbeddedImage<png_BackgroundLogo>().GetImage() );

    m_MainFrame = new MainEmuFrame( NULL, wxID_ANY, wxEmptyString );
    SetTopWindow( m_MainFrame );
    m_MainFrame->Show();

    return true;
}

int Pcsx2App::OnExit()
{
	g_Conf.LoadSave( IniSaver() );
	return wxApp::OnExit();
}

const wxBitmap& Pcsx2App::GetLogoBitmap() const
{
	wxASSERT( m_Bitmap_Logo != NULL );
	return *m_Bitmap_Logo;
}
