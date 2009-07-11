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
	const wxDirName Snapshots	( L"snaps" );
	const wxDirName Savestates	( L"sstates" );
	const wxDirName MemoryCards	( L"memcards" );
	const wxDirName Settings	( L"inis" );
	const wxDirName Plugins		( L"plugins" );
	const wxDirName Logs		( L"logs" );
	const wxDirName Dumps		( L"dumps" );
	const wxDirName Themes		( L"themes" );

	// Specifies the root folder for the application install.
	// (currently it's the CWD, but in the future I intend to move all binaries to a "bin"
	// sub folder, in which case the approot will become "..")
	const wxDirName AppRoot( "." ); // L".." );

	// Fetches the path location for user-consumable documents -- stuff users are likely to want to
	// share with other programs: screenshots, memory cards, and savestates.
	wxDirName GetDocuments()
	{
		if( g_Conf.UseAdminMode )
			return (wxDirName)wxGetCwd();
		else
			return (wxDirName)wxStandardPaths::Get().GetDocumentsDir() + (wxDirName)wxGetApp().GetAppName();
	}

	wxDirName GetSnapshots()
	{
		return GetDocuments() + Snapshots;
	}

	wxDirName GetBios()
	{
		return GetDocuments() + wxDirName( L"bios" );
	}

	wxDirName GetSavestates()
	{
		return GetDocuments() + Savestates;
	}

	wxDirName GetMemoryCards()
	{
		return GetDocuments() + MemoryCards;
	}

	wxDirName GetPlugins()
	{
		return AppRoot + Plugins;
	}
	
	wxDirName GetSettings()
	{
		return GetDocuments() + Settings;
	}

	wxDirName GetThemes()
	{
		return AppRoot + Themes;
	}

	wxDirName GetLogs()
	{
		return GetDocuments() + Logs;
	}

	wxDirName GetDumps()
	{
		return GetDocuments() + Dumps;
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
wxString AppConfig::FullpathHelpers::operator[]( PluginsEnum_t pluginidx ) const
{
	return Path::Combine( m_conf.Folders.Plugins, m_conf.BaseFilenames[pluginidx] );
}

wxString AppConfig::FullpathHelpers::Bios() const	{ return Path::Combine( m_conf.Folders.Bios, m_conf.BaseFilenames.Bios ); }
wxString AppConfig::FullpathHelpers::Mcd( uint mcdidx ) const { return Path::Combine( m_conf.Folders.MemoryCards, m_conf.MemoryCards.Mcd[mcdidx].Filename ); }

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
//
void i18n_DoPackageCheck( int wxLangId, wxArrayString& destEng, wxArrayString& destTrans )
{
	// Note: wx auto-preserves the current locale for us

	if( !wxLocale::IsAvailable( wxLangId ) ) return;
	wxLocale* locale = new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING );

	if( locale->IsOk() && locale->AddCatalog( L"pcsx2ident" ) )
	{
		// Should be a valid language, so add it to the list.

		destEng.Add( wxLocale::GetLanguageName( wxLangId ) );
		destTrans.Add( wxGetTranslation( L"NativeName" ) );
	}
	delete locale;
}

// ------------------------------------------------------------------------
// Finds all valid PCSX2 language packs, and enumerates them for configuration selection.
// Note: On linux there's no easy way to reliably enumerate language packs, since every distro
// could use its own location for installing pcsx2.mo files (wtcrap?).  Furthermore wxWidgets
// doesn't give us a public API for checking what the language search paths are.  So the only
// safe way to enumerate the languages is by forcibly loading every possible locale in the wx
// database.  Anything which hasn't been installed will fail to load.
//
// Because loading and hashing the entire pcsx2 translation for every possible language would
// assinine and slow, I've decided to use a two-file translation system.  One file is very
// small and simply contains the name of the language in the language native.  The second file
// is loaded only if the user picks it (or if it's the default language of the OS).
//
void i18n_EnumeratePackages( wxArrayString& englishNames, wxArrayString& xlatedNames)
{
	for( int li=wxLANGUAGE_ABKHAZIAN; li<wxLANGUAGE_ZULU; ++li )
	{
		i18n_DoPackageCheck( li, englishNames, xlatedNames );
	}
	
	// Brilliant.  Because someone in the wx world didn't think to move wxLANGUAGE_USER_DEFINED
	// to a place where it wasn't butt right up against the main languages (like, say, start user
	// defined values at 4000 or something?), they had to add new languages in at some arbitrary
	// value instead.  Let's handle them here:
	// fixme: these won't show up in alphabetical order if they're actually present (however
	// horribly unlikely that is)... do we care?  Probably not.
	
	i18n_DoPackageCheck( wxLANGUAGE_VALENCIAN, englishNames, xlatedNames );
	i18n_DoPackageCheck( wxLANGUAGE_SAMI, englishNames, xlatedNames );
}

// ------------------------------------------------------------------------
void AppConfig::LoadSave( IniInterface& ini )
{
	IniEntry( MainGuiPosition,		wxDefaultPosition );
	IniEntry( CdvdVerboseReads,		false );

	// Process various sub-components:
	ConLogBox.LoadSave( ini );
	Speedhacks.LoadSave( ini );
	Folders.LoadSave( ini );
	BaseFilenames.LoadSave( ini );

	ini.Flush();
}


// ------------------------------------------------------------------------
//
void AppConfig::Apply()
{
	// Language Application:
	// Three stages.  First we try and configure the requested language. If that fails,
	// we fall back on the default language for the user's operating system.  If that
	// fails we fall back on good old english.

	//wxLocale*	locale;

	if( !i18n_SetLanguage( LanguageId ) )
	{
		if( !i18n_SetLanguage( wxLANGUAGE_DEFAULT ) )
		{
			//wxGetTranslation();
		}
	}
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

	const wxDirName def( L"default" );

	IniEntry( Plugins,		PathDefs::GetPlugins() );
	IniEntry( Settings,		PathDefs::GetSettings() );
	IniEntry( Bios,			PathDefs::GetBios() );
	IniEntry( Snapshots,	PathDefs::GetSnapshots() );
	IniEntry( Savestates,	PathDefs::GetSavestates() );
	IniEntry( MemoryCards,	PathDefs::GetMemoryCards() );
	IniEntry( Logs,			PathDefs::GetLogs() );

	ini.SetPath( L".." );
}

const wxString g_PluginNames[] =
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

// ------------------------------------------------------------------------
const wxFileName& AppConfig::FilenameOptions::operator[]( PluginsEnum_t pluginidx ) const
{
	if( (uint)pluginidx >= Plugin_Count )
		throw Exception::IndexBoundsFault( L"Filename[Plugin]", pluginidx, Plugin_Count );

	return Plugins[pluginidx];
}

void AppConfig::FilenameOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Filenames" );

	const wxFileName pc( L"Please Configure" );

	for( int i=0; i<Plugin_Count; ++i )
	{
		ini.Entry( g_PluginNames[i], Plugins[i], pc );
	}

	ini.SetPath( L".." );
}

