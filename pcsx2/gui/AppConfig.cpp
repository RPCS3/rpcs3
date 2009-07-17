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
	namespace Base
	{
		const wxDirName& Snapshots()
		{
			static const wxDirName retval( L"snaps" );
			return retval;
		}
		
		const wxDirName& Savestates()
		{
			static const wxDirName retval( L"sstates" );
			return retval;
		}

		const wxDirName& MemoryCards()
		{
			static const wxDirName retval( L"memcards" );
			return retval;
		}
		
		const wxDirName& Settings()
		{
			static const wxDirName retval( L"inis" );
			return retval;
		}
		
		const wxDirName& Plugins()
		{
			static const wxDirName retval( L"plugins" );
			return retval;
		}
		
		const wxDirName& Logs()
		{
			static const wxDirName retval( L"logs" );
			return retval;
		}

		const wxDirName& Dumps()
		{
			static const wxDirName retval( L"dumps" );
			return retval;
		}
		
		const wxDirName& Themes()
		{
			static const wxDirName retval( L"themes" );
			return retval;
		}

	};

	// Specifies the root folder for the application install.
	// (currently it's the CWD, but in the future I intend to move all binaries to a "bin"
	// sub folder, in which case the approot will become "..")
	const wxDirName& AppRoot()
	{
		static const wxDirName retval( L"." );
		return retval;
	}

	// Fetches the path location for user-consumable documents -- stuff users are likely to want to
	// share with other programs: screenshots, memory cards, and savestates.
	wxDirName GetDocuments()
	{
		if( g_Conf->UseAdminMode )
			return (wxDirName)wxGetCwd();
		else
			return (wxDirName)wxStandardPaths::Get().GetDocumentsDir() + (wxDirName)wxGetApp().GetAppName();
	}

	wxDirName GetSnapshots()
	{
		return GetDocuments() + Base::Snapshots();
	}

	wxDirName GetBios()
	{
		return GetDocuments() + wxDirName( L"bios" );
	}

	wxDirName GetSavestates()
	{
		return GetDocuments() + Base::Savestates();
	}

	wxDirName GetMemoryCards()
	{
		return GetDocuments() + Base::MemoryCards();
	}

	wxDirName GetPlugins()
	{
		return AppRoot() + Base::Plugins();
	}

	wxDirName GetSettings()
	{
		return GetDocuments() + Base::Settings();
	}

	wxDirName GetThemes()
	{
		return AppRoot() + Base::Themes();
	}

	wxDirName GetLogs()
	{
		return GetDocuments() + Base::Logs();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
namespace FilenameDefs
{
	wxFileName GetConfig()
	{
		// TODO : ini extension on Win32 is normal.  Linux ini filename default might differ
		// from this?  like pcsx2_conf or something ... ?

		return wxGetApp().GetAppName() + L".ini";
	}

	const wxFileName& Memcard( int slot )
	{
		static const wxFileName retval[2] =
		{
			wxFileName( L"Mcd001.ps2" ),
			wxFileName( L"Mcd002.ps2" )
		};
		
		if( IsDevBuild && ((uint)slot) < 2 )
			throw Exception::IndexBoundsFault( L"FilenameDefs::Memcard", slot, 2 );
			
		return retval[slot];
	}
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
	result.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, GetPath() );
	return result;
}

// ------------------------------------------------------------------------
wxDirName wxDirName::Combine( const wxDirName& right ) const
{
	wxASSERT_MSG( IsDir() && right.IsDir(), L"Warning: Malformed directory name detected during wDirName concatenation." );

	wxDirName result( right );
	result.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, GetPath() );
	return result;
}

// ------------------------------------------------------------------------
wxDirName& wxDirName::Normalize( int flags, const wxString& cwd )
{
	wxASSERT_MSG( IsDir(), L"Warning: Malformed directory name detected during wDirName normalization." );
	if( !wxFileName::Normalize( flags, cwd ) )
		throw Exception::RuntimeError( "wxDirName::Normalize operation failed." );
	return *this;
}

// ------------------------------------------------------------------------
wxDirName& wxDirName::MakeRelativeTo( const wxString& pathBase )
{
	wxASSERT_MSG( IsDir(), L"Warning: Malformed directory name detected during wDirName normalization." );
	if( !wxFileName::MakeRelativeTo( pathBase ) )
		throw Exception::RuntimeError( "wxDirName::MakeRelativeTo operation failed." );
	return *this;
}

// ------------------------------------------------------------------------
wxDirName& wxDirName::MakeAbsolute( const wxString& cwd )
{
	wxASSERT_MSG( IsDir(), L"Warning: Malformed directory name detected during wDirName normalization." );
	if( !wxFileName::MakeAbsolute( cwd ) )
		throw Exception::RuntimeError( "wxDirName::MakeAbsolute operation failed." );
	return *this;
}

// ------------------------------------------------------------------------
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
wxString AppConfig::FullpathTo( PluginsEnum_t pluginidx ) const
{
	return Path::Combine( Folders.Plugins, BaseFilenames[pluginidx] );
}

wxString AppConfig::FullpathToBios() const				{ return Path::Combine( Folders.Bios, BaseFilenames.Bios ); }
wxString AppConfig::FullpathToMcd( uint mcdidx ) const	{ return Path::Combine( Folders.MemoryCards, MemoryCards.Mcd[mcdidx].Filename ); }

// ------------------------------------------------------------------------
// GCC Note: wxT() macro is required when using string token pasting.  For some reason L generates
// syntax errors. >_<
//
#define IniEntry( varname, defval ) ini.Entry( wxT(#varname), varname, defval )

// ------------------------------------------------------------------------
void AppConfig::LoadSaveUserMode( IniInterface& ini )
{
	IniEntry( UseAdminMode,		false );
	ini.Entry( L"SettingsPath", Folders.Settings, PathDefs::GetSettings() );

	ini.Flush();
}

// ------------------------------------------------------------------------
void AppConfig::LoadSave( IniInterface& ini )
{
	IniEntry( MainGuiPosition,		wxDefaultPosition );
	IniEntry( LanguageId,			wxLANGUAGE_DEFAULT );
	IniEntry( RecentFileCount,		6 );
	IniEntry( DeskTheme,			L"default" );
	IniEntry( Listbook_ImageSize,	32 );
	IniEntry( Toolbar_ImageSize,	24 );
	IniEntry( Toolbar_ShowLabels,	true );

	IniEntry( CdvdVerboseReads,		false );

	// Process various sub-components:
	ConLogBox.LoadSave( ini );
	Speedhacks.LoadSave( ini );
	Folders.LoadSave( ini );
	BaseFilenames.LoadSave( ini );

	if( ini.IsSaving() && (g_RecentIsoList != NULL) )
		g_RecentIsoList->Save( ini.GetConfig() );

	ini.Flush();
}

// ------------------------------------------------------------------------
// Performs necessary operations to ensure that the current g_Conf settings (and other config-stored
// globals) are applied to the pcsx2 main window and primary emulation subsystems (if active).
//
void AppConfig::Apply()
{
	if( !i18n_SetLanguage( LanguageId ) )
	{
		if( !i18n_SetLanguage( wxLANGUAGE_DEFAULT ) )
		{
			i18n_SetLanguage( wxLANGUAGE_ENGLISH );
		}
	}

	// Always perform delete and reload of the Recent Iso List.  This handles cases where
	// the recent file count has been changed, and it's a helluva lot easier than trying
	// to make a clone copy of this complex object. ;)

	wxConfigBase* cfg = wxConfigBase::Get( false );
	wxASSERT( cfg != NULL );
	
	if( g_RecentIsoList != NULL )
		g_RecentIsoList->Save( *cfg );
	safe_delete( g_RecentIsoList );
	g_RecentIsoList = new wxFileHistory( RecentFileCount );
	g_RecentIsoList->Load( *cfg );

	cfg->Flush();
}

// ------------------------------------------------------------------------
void AppConfig::Load()
{
	// Note: Extra parenthesis resolves "I think this is a function" issues with C++.
	IniLoader loader( (IniLoader()) );
	LoadSave( loader );
}

void AppConfig::Save()
{
	IniSaver saver( (IniSaver()) );
	LoadSave( saver );
}

// ------------------------------------------------------------------------
void AppConfig::ConsoleLogOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"ConsoleLog" );

	IniEntry( Visible,			false );
	IniEntry( AutoDock,			true );
	IniEntry( DisplayPosition,	wxDefaultPosition );
	IniEntry( DisplaySize,		wxSize( 540, 540 ) );
	
	ini.SetPath( L".." );
}

// ------------------------------------------------------------------------
void AppConfig::SpeedhackOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Speedhacks" );

	ini.SetPath( L".." );
}

// ------------------------------------------------------------------------
void AppConfig::FolderOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Folders" );

	IniEntry( Plugins,		PathDefs::GetPlugins() );
	IniEntry( Settings,		PathDefs::GetSettings() );
	IniEntry( Bios,			PathDefs::GetBios() );
	IniEntry( Snapshots,	PathDefs::GetSnapshots() );
	IniEntry( Savestates,	PathDefs::GetSavestates() );
	IniEntry( MemoryCards,	PathDefs::GetMemoryCards() );
	IniEntry( Logs,			PathDefs::GetLogs() );
	
	IniEntry( RunIso,		PathDefs::GetDocuments() );			// raw default is always the Documents folder.

	ini.SetPath( L".." );
}

// ------------------------------------------------------------------------
const wxFileName& AppConfig::FilenameOptions::operator[]( PluginsEnum_t pluginidx ) const
{
	if( (uint)pluginidx >= PluginId_Count )
		throw Exception::IndexBoundsFault( L"Filename[Plugin]", pluginidx, PluginId_Count );

	return Plugins[pluginidx];
}

void AppConfig::FilenameOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Filenames" );

	static const wxFileName pc( L"Please Configure" );
	static const wxString g_PluginNames[] =
	{
		L"CDVD",
		L"GS",
		L"PAD1",
		L"PAD2",
		L"SPU2",
		L"USB",
		L"FW",
		L"DEV9"
	};

	for( int i=0; i<PluginId_Count; ++i )
	{
		ini.Entry( g_PluginNames[i], Plugins[i], pc );
	}

	ini.SetPath( L".." );
}

