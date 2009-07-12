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
#include "i18n.h"
#include "HashMap.h"

#include "Utilities/SafeArray.h"


using namespace HashTools;

//////////////////////////////////////////////////////////////////////////////////////////
// Notes to Translators:
//  * The first line of each entry consists of an enumerated index (used internally), and
//    the gettext lookup string (which you'll find in the PO data).  The resulting translation
//    should match the text underneath.
//
//  * Text marked as Tooltips are usually tertiary information that is not critical to PCSX2
//    use, and translations are not required.
//
const EnglishExpansionEntry m_tbl_English[] =
{
	{ Msg_Dialog_AdvancedPaths, wxLt(L"Settings Dialog:Advanced Paths"),
		L"Warning!! These advanced options are provided for developers and advanced testers only. "
		L"Changing these settings can cause program errors, so please be weary."
	},

	{ Msg_Tooltips_SettingsPath, wxLt(L"Setting Tooltip:Settings Path"),
		L"This is the folder where PCSX2 saves all settings, including settings generated "
		L"by most plugins.\n\nWarning: Some older versions of plugins may not respect this value."
	},

	// ------------------------------------------------------------------------
	// Begin Tooltips Section
	// (All following texts are non-critical for a functional PCSX2 translation).

	{ Msg_Tooltips_PluginsPath, wxLt(L"Setting Tooltip:Plugins Path"),
		L"This is the location where PCSX2 will expect to find its plugins. Plugins found in this folder "
		L"will be enumerated and are selectable from the Plugins panel."
	},

	{ Msg_Tooltips_Savestates,	wxLt(L"Setting Tooltip:Savestates Folder"),
		L"This folder is where PCSX2 records savestates; which are recorded either by using "
		L"menus/toolbars, or by pressing F1/F3 (load/save)."
	},

	{ Msg_Tooltips_Snapshots,	wxLt(L"Setting Tooltip:Snapshots Folder"),
		L"This folder is where PCSX2 saves screenshots.  Actual screenshot image format and style "
		L"may vary depending on the GS plugin being used."
	},

	{ Msg_Tooltips_Bios,		wxLt(L"Setting Tooltip:Bios Folder"),
		L"This folder is where PCSX2 looks to find PS2 bios files.  The actual bios used can be "
		L"selected from the CPU dialog."
	},

	{ Msg_Tooltips_Logs,		wxLt(L"Setting Tooltip:Logs Folder"),
		L"This folder is where PCSX2 saves its logfiles and diagnostic dumps.  Most plugins will "
		L"also adhere to this folder, however some older plugins may ignore it."
	},

	{ Msg_Tooltips_Memorycards,	wxLt(L"Setting Tooltip:Memorycards Folder"),
		L"This is the default path where PCSX2 loads or creates its memory cards, and can be "
		L"overridden in the MemoryCard Configuration by using absolute filenames."
	},

};

C_ASSERT( ArraySize( m_tbl_English ) == ExpandedMsg_Count );

static HashMap<int,HashedExpansionPair> m_EnglishExpansions( -1, 0xcdcdcd, ArraySize( m_tbl_English ) );

// ------------------------------------------------------------------------
// Builds an internal hashtable for English iconized description lookups.
//
void i18n_InitPlainEnglish()
{
	static bool IsInitialized = false;

	IsInitialized = true;
	for( int i=0; i<ExpandedMsg_Count; ++i )
	{
		HashedExpansionPair silly = { m_tbl_English[i].gettextKey, m_tbl_English[i].Expanded };
		m_EnglishExpansions[m_tbl_English[i].Key] = silly;
	}
}

// ------------------------------------------------------------------------
// PCSX2's Iconized Text Translator.
// This i18n version provides two layers of translated lookups.  It puts the key through the
// current language first and, if the key is not resolved (meaning the language pack doesn't
// have a translation for it), it's put through our own built-in english translation.  This
// second step is needed to resolve some of our lengthy UI tooltips and descriptors, which
// use iconized GetText identifiers.
//
// (without this second pass many tooltips would just show up as "Savestate Tooltip" instead
//  of something meaningful).
//
const wxChar* __fastcall pxExpandMsg( ExpandedMsgEnum key )
{
	const HashedExpansionPair& data( m_EnglishExpansions[key] );

	int curlangid = wxLocale::GetLanguageInfo( g_Conf.LanguageId )->Language;
	if( curlangid == wxLANGUAGE_ENGLISH || curlangid == wxLANGUAGE_ENGLISH_US )
		return data.Expanded;

	const wxChar* retval = wxGetTranslation( data.gettextKey );

	// Check if the translation failed, and fall back on an english lookup.
	return ( wxStrcmp( retval, data.gettextKey ) == 0 ) ? data.Expanded : retval;
}

// ------------------------------------------------------------------------
// Alternative implementation for wxGetTranslation.
// This version performs a string length check in devel builds, which issues a warning
// if the string seems too long for gettext lookups.  Longer complicated strings should
// usually be implemented used the pxMsgExpand system instead.
//
const wxChar* __fastcall pxGetTranslation( const wxChar* message )
{
	if( IsDevBuild )
	{
		if( wxStrlen( message ) > 96 )
		{
			Console::Notice( "pxGetTranslation: Long message detected, maybe use pxExpandMsg instead?" );
			Console::Status( wxsFormat( L"\tMessage: %s", message ) );
		}
	}
	return wxGetTranslation( message );
}

// ------------------------------------------------------------------------
bool i18n_SetLanguage( int wxLangId )
{
	if( !wxLocale::IsAvailable( wxLangId ) )
	{
		Console::Notice( "Invalid Language Identifier (wxID=%d).", params wxLangId );
		return false;
	}

	wxLocale* locale = new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING );

	if( !locale->IsOk() )
	{
		Console::Notice( wxsFormat( L"SetLanguage: '%s' [%s] is not supported by the operating system",
			wxLocale::GetLanguageName( locale->GetLanguage() ).c_str(), locale->GetCanonicalName().c_str() )
		);

		safe_delete( locale );
		return false;
	}

	if( !locale->AddCatalog( L"pcsx2main" ) ) //, wxLANGUAGE_UNKNOWN, NULL ) )
	{
		Console::Notice( wxsFormat( L"SetLanguage: Cannot find pcsx2main.mo file for language '%s' [%s]",
			wxLocale::GetLanguageName( locale->GetLanguage() ).c_str(), locale->GetCanonicalName().c_str() )
		);
		safe_delete( locale );
	}
	//return locale;
	return true;
}
