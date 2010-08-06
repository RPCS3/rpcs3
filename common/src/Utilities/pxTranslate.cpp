/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

bool pxIsEnglish( int id )
{
	return ( id == wxLANGUAGE_ENGLISH || id == wxLANGUAGE_ENGLISH_US );
}

// --------------------------------------------------------------------------------------
//  pxExpandMsg  -- an Iconized Text Translator
// --------------------------------------------------------------------------------------
// This function provides two layers of translated lookups.  It puts the key through the
// current language first and, if the key is not resolved (meaning the language pack doesn't
// have a translation for it), it's put through our own built-in english translation.  This
// second step is needed to resolve some of our lengthy UI tooltips and descriptors, which
// use iconized GetText identifiers.
//
// (without this second pass many tooltips would just show up as "Savestate Tooltip" instead
//  of something meaningful).
//
// Rationale: Traditional gnu-style gnu_gettext stuff tends to stop translating strings when 
// the slightest change to a string is made (including punctuation and possibly even case).
// On long strings especially, this can be unwanted since future revisions of the app may have
// simple tyop fixes that *should not* break existing translations.  Furthermore
const wxChar* __fastcall pxExpandMsg( const wxChar* key, const wxChar* englishContent )
{
#ifdef PCSX2_DEVBUILD
	static const wxChar* tbl_pxE_Prefixes[] =
	{
		L".Panel:",
		L".Popup:",
		L".Error:",
		L".Wizard:",
		L".Tooltip:",
		NULL
	};

	// test the prefix of the key for consistency to valid/known prefix types.
	const wxChar** prefix = tbl_pxE_Prefixes;
	while( *prefix != NULL )
	{
		if( wxString(key).StartsWith(*prefix) ) break;
		++prefix;
	}
	pxAssertDev( *prefix != NULL,
		wxsFormat( L"Invalid pxE key prefix in key '%s'.  Prefix must be one of the valid prefixes listed in pxExpandMsg.", key )
	);
#endif

	const wxLanguageInfo* info = (wxGetLocale() != NULL) ? wxLocale::GetLanguageInfo( wxGetLocale()->GetLanguage() ) : NULL;

	if( ( info == NULL ) || pxIsEnglish( info->Language ) )
		return englishContent;

	const wxChar* retval = wxGetTranslation( key );

	// Check if the translation failed, and fall back on an english lookup.
	return ( wxStrcmp( retval, key ) == 0 ) ? englishContent : retval;
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
		if( wxStrlen( message ) > 128 )
		{
			Console.Warning( "pxGetTranslation: Long message detected, maybe use pxE() instead?" );
			Console.WriteLn( Color_Green, L"Message: %s", message );
		}
	}
	return wxGetTranslation( message );
}
