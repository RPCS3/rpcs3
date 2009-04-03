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

#include <wx/image.h>

#include <ShlObj.h>


IMPLEMENT_APP(Pcsx2App)

const wxRect wxDefaultRect( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord );

namespace PathDefs
{
	const wxString Screenshots( "snaps" );
	const wxString Savestates( "sstates" );
	const wxString MemoryCards( "memcards" );
	const wxString Configs( "inis" );
	const wxString Plugins( "plugins" );
	
	wxString Working;

	void Initialize()
	{
		char temp[g_MaxPath];
		_getcwd( temp, g_MaxPath );
		Working = temp;
	}

	// Fetches the path location for user-consumable documents -- stuff users are likely to want to
	// share with other programs: screenshots, memory cards, and savestates.
	// [TODO] : Ideally this should be optional, configurable, or conditional such that the Pcsx2
	//   executable working directory can be used.  I'm not entirely sure yet the best way to go
	//   about implementating that toggle, though.
	wxString GetDocuments()
	{
		#ifdef _WIN32
			wxChar path[MAX_PATH];
			SHGetFolderPath( NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path );
			return Path::Combine( path, wxGetApp().GetAppName().c_str() );
		#else
			return wxGetHomeDir();
		#endif

		return "";
	}

	wxString GetScreenshots()
	{
		return Path::Combine( GetDocuments(), Screenshots );
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
};

Pcsx2App::Pcsx2App()  :
	m_ConsoleFrame( NULL ),
	m_GlobalConfig( NULL )
{
	SetAppName( "Pcsx2" );
}

bool Pcsx2App::OnInit()
{
    wxInitAllImageHandlers();

	// wxWidgets fails badly on Windows, when it comes to picking "default" locations for ini files.
	// So for now I have to specify the ini file location manually.

	Path::CreateDirectory( PathDefs::GetDocuments() );
	Path::CreateDirectory( PathDefs::GetConfigs() );

	// FIXME: I think that linux might adhere to a different extension standard than .ini -- I forget which tho.
	m_GlobalConfig = new AppConfig( Path::Combine( PathDefs::GetConfigs(), GetAppName() ) + ".ini" );
	m_GlobalConfig->LoadSettings();

	m_Bitmap_Logo = new wxBitmap( _T("./pcsxAbout.png"), wxBITMAP_TYPE_PNG );

    m_MainFrame = new MainEmuFrame( NULL, wxID_ANY, wxEmptyString );
    SetTopWindow( m_MainFrame );
    m_MainFrame->Show();

    return true;
}

int Pcsx2App::OnExit()
{
	m_GlobalConfig->SaveSettings();
	return wxApp::OnExit();
}

const wxBitmap& Pcsx2App::GetLogoBitmap() const
{
	wxASSERT( m_Bitmap_Logo != NULL );
	return *m_Bitmap_Logo;
}

AppConfig& Pcsx2App::GetActiveConfig() const
{
	wxASSERT( m_GlobalConfig != NULL );
	return *m_GlobalConfig;
}
