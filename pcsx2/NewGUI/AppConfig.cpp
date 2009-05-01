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
#include "App.h"
#include "IniInterface.h"

#include <wx/stdpaths.h>

//////////////////////////////////////////////////////////////////////////////////////////
// PathDefs Namespace -- contains default values for various pcsx2 path names and locations.
//
// Note: The members of this namespace are intended for default value initialization only.
// Most of the time you should use the path folder assignments in Conf() instead, since those
// are user-configurable.
//
namespace PathDefs
{
	const wxDirName Snapshots( L"snaps" );
	const wxDirName Savestates( L"sstates" );
	const wxDirName MemoryCards( L"memcards" );
	const wxDirName Configs( L"inis" );
	const wxDirName Plugins( L"plugins" );

	// Fetches the path location for user-consumable documents -- stuff users are likely to want to
	// share with other programs: screenshots, memory cards, and savestates.
	wxDirName GetDocuments()
	{
		return (wxDirName)wxStandardPaths::Get().GetDocumentsDir() + (wxDirName)wxGetApp().GetAppName();
	}

	wxDirName GetSnapshots()
	{
		return (wxDirName)GetDocuments() + Snapshots;
	}

	wxDirName GetBios()
	{
		return (wxDirName)L"bios";
	}

	wxDirName GetSavestates()
	{
		return (wxDirName)GetDocuments() + Savestates;
	}

	wxDirName GetMemoryCards()
	{
		return (wxDirName)GetDocuments() + MemoryCards;
	}

	wxDirName GetConfigs()
	{
		return (wxDirName)GetDocuments()+ Configs;
	}

	wxDirName GetPlugins()
	{
		return (wxDirName)Plugins;
	}
};

namespace FilenameDefs
{
	wxFileName GetConfig()
	{
		// TODO : ini extension on Win32 is normal.  Linux ini filename default might differ
		// from this?  like pcsx2_conf or something ... ?

		return wxGetApp().GetAppName() + L".ini";
	}

	const wxFileName Memcard[2] =
	{
		wxFileName( L"Mcd001.ps2" ),
		wxFileName( L"Mcd002.ps2" )
	};
};

// ------------------------------------------------------------------------
wxFileName wxDirName::Combine( const wxFileName& right ) const
{
	wxASSERT_MSG( IsDir(), L"Warning: Malformed directory name detected during wxDirName concatenation." );
	if( right.IsAbsolute() )
		return right;

	// Append any directory parts from right, and then set the filename.
	// Except we can't do that because our m_members are private (argh!) and there is no API
	// for getting each component of the path.  So instead let's use Normalize:

	wxFileName result( right );
	result.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS, GetPath() );
	return result;
}

wxDirName wxDirName::Combine( const wxDirName& right ) const
{
	wxASSERT_MSG( IsDir() && right.IsDir(), L"Warning: Malformed directory name detected during wDirName concatenation." );

	wxDirName result( right );
	result.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS, GetPath() );
	return result;
}

void wxDirName::Rmdir()
{
	if( !Exists() ) return;
	wxFileName::Rmdir();
	// TODO : Throw exception if operation failed?  Do we care?
}

bool wxDirName::Mkdir()
{
	if( Exists() ) return true;
	return wxFileName::Mkdir();
}

// ------------------------------------------------------------------------
wxString AppConfig::FullpathHelpers::Bios() const	{ return Path::Combine( m_conf.Folders.Bios, m_conf.BaseFilenames.Bios ); }
wxString AppConfig::FullpathHelpers::CDVD() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.CDVD ); }
wxString AppConfig::FullpathHelpers::GS() const		{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.GS ); }
wxString AppConfig::FullpathHelpers::PAD1() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.PAD1 ); }
wxString AppConfig::FullpathHelpers::PAD2() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.PAD2 ); }
wxString AppConfig::FullpathHelpers::SPU2() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.SPU2 ); }
wxString AppConfig::FullpathHelpers::DEV9() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.DEV9 ); }
wxString AppConfig::FullpathHelpers::USB() const	{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.USB ); }
wxString AppConfig::FullpathHelpers::FW() const		{ return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames.FW ); }

wxString AppConfig::FullpathHelpers::Mcd( uint mcdidx ) const { return Path::Combine( m_conf.Folders.MemoryCards, m_conf.MemoryCards.Mcd[mcdidx].Filename ); }

//////////////////////////////////////////////////////////////////////////////////////////
//

#define IniEntry( varname, defval ) ini.Entry( wxT(#varname), varname, defval )

void AppConfig::LoadSave( IniInterface& ini )
{
	IniEntry( MainGuiPosition,		wxDefaultPosition );
	IniEntry( CdvdVerboseReads,		false );

	// Process various sub-components:
	ConLogBox.LoadSave( ini );
	Speedhacks.LoadSave( ini );

	ini.Flush();
}

void AppConfig::Load()
{
	// Note: Extra parenthisis resolves "I think this is a function" issues with C++.
	IniLoader loader( (IniLoader()) );
	LoadSave( loader );
}

void AppConfig::Save()
{
	IniSaver saver( (IniSaver()) );
	LoadSave( saver );
}

void AppConfig::ConsoleLogOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"ConsoleLog" );

	IniEntry( Visible,			false );
	IniEntry( AutoDock,			true );
	IniEntry( DisplayPosition,	wxDefaultPosition );
	IniEntry( DisplaySize,		wxSize( 540, 540 ) );

	ini.SetPath( L".." );
}

void AppConfig::SpeedhackOptions::LoadSave( IniInterface& ini )
{
}
