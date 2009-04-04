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
#include "MainFrame.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include <wx/stdpaths.h>

IMPLEMENT_APP(Pcsx2App)

AppConfig g_Conf;

const wxRect wxDefaultRect( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord );

//////////////////////////////////////////////////////////////////////////////////////////
// PathDefs Namespace -- contains default values for various pcsx2 path names and locations.
//
// Note: The members of this namespace are intended for default value initialization only.
// Most of the time you should use the path folder assignments in Conf() instead, since those
// are user-configurable.
//
namespace PathDefs
{
	const wxString Snapshots( "snaps" );
	const wxString Savestates( "sstates" );
	const wxString MemoryCards( "memcards" );
	const wxString Configs( "inis" );
	const wxString Plugins( "plugins" );
	
	// Fetches the path location for user-consumable documents -- stuff users are likely to want to
	// share with other programs: screenshots, memory cards, and savestates.
	wxString GetDocuments()
	{
		return Path::Combine( wxStandardPaths::Get().GetDocumentsDir(), wxGetApp().GetAppName() );
	}

	wxString GetSnapshots()
	{
		return Path::Combine( GetDocuments(), Snapshots );
	}
	
	wxString GetBios()
	{
		return "bios";
	}
	
	wxString GetSavestates()
	{
		return Path::Combine( GetDocuments(), Savestates );
	}
	
	wxString GetMemoryCards()
	{
		return Path::Combine( GetDocuments(), MemoryCards );
	}
	
	wxString GetConfigs()
	{
		return Path::Combine( GetDocuments(), Configs );
	}

	wxString GetPlugins()
	{
		return Plugins;
	}
	
	wxString GetWorking()
	{
		return wxGetCwd();
	}
};

namespace FilenameDefs
{
	wxString GetConfig()
	{
		// TODO : ini extension on Win32 is normal.  Linux ini filename default might differ
		// from this?  like pcsx2_conf or something ... ?

		return wxGetApp().GetAppName() + ".ini";
	}
};

Pcsx2App::Pcsx2App()  :
	m_ConsoleFrame( NULL )
{
	SetAppName( "Pcsx2" );
}

wxFileConfig* OpenConfig( const wxString& filename )
{
	return new wxFileConfig( wxEmptyString, wxEmptyString, filename, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH );
}

// returns true if a configuration file is present in the current working dir (cwd).
// returns false if not (in which case the calling code should fall back on using OpenConfigUserLocal())
bool Pcsx2App::TryOpenConfigCwd()
{
	wxString inipath_cwd( Path::Combine( wxGetCwd(), PathDefs::Configs ) );
	if( !Path::isDirectory( inipath_cwd ) ) return false;

	wxString inifile_cwd( Path::Combine( inipath_cwd, FilenameDefs::GetConfig() ) );
	if( !Path::isFile( inifile_cwd ) ) return false;
	if( Path::getFileSize( inifile_cwd ) <= 1 ) return false;

	wxConfigBase::Set( OpenConfig( inifile_cwd ) );
	return true;
}

bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();

	// Ini Startup:  The ini file could be in one of two locations, depending on how Pcsx2 has
	// been installed or configured.  The first place we look is in our program's working
	// directory.  If the ini there exist, and is *not* empty, then we'll use it.  Otherwise
	// we fall back on the ini file in the user's Documents folder.
	
	if( !TryOpenConfigCwd() )
	{
		Path::CreateDirectory( PathDefs::GetDocuments() );
		Path::CreateDirectory( PathDefs::GetConfigs() );

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
