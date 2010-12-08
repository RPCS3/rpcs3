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
#include "i18n.h"
#include "AppConfig.h"

#include "Utilities/SafeArray.h"

LangPackEnumeration::LangPackEnumeration( wxLanguage langId )
{
	wxLangId = langId;

	if (const wxLanguageInfo* info = wxLocale::GetLanguageInfo( wxLangId ))
	{
		canonicalName = info->CanonicalName;
		englishName = info->Description;
	}
}

LangPackEnumeration::LangPackEnumeration()
{
	wxLangId = wxLANGUAGE_DEFAULT;
	englishName = L" System Default";		// left-side space forces it to sort to the front of the lists
	canonicalName = L"default";

	int sysLang = wxLocale::GetSystemLanguage();

	if (sysLang == wxLANGUAGE_UNKNOWN)
		sysLang = wxLANGUAGE_ENGLISH;

	if (const wxLanguageInfo* info = wxLocale::GetLanguageInfo( sysLang ))
		englishName += L" [" + info->Description + L"]";
}

// Some of the codes provided by wxWidgets are 'obsolete' -- effectively replaced by more specific
// region-qualified language codes.  This function can be used to filter them out.
bool i18n_IsLegacyLanguageId( wxLanguage lang )
{
	return
		(lang == wxLANGUAGE_ENGLISH) ||
		(lang == wxLANGUAGE_CHINESE) ||
		(lang == wxLANGUAGE_CHINESE_TAIWAN) ||
		(lang == wxLANGUAGE_SERBIAN) ||
		(lang == wxLANGUAGE_SPANISH);
}

static void i18n_DoPackageCheck( wxLanguage wxLangId, LangPackList& langs )
{
	if( i18n_IsLegacyLanguageId( wxLangId ) ) return;

	// Note: wx auto-preserves the current locale for us
	if( !wxLocale::IsAvailable( wxLangId ) ) return;
	wxLocale* locale = new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING );

	// Force the msgIdLanguage param to wxLANGUAGE_UNKNOWN to disable wx's automatic english
	// matching logic, which will bypass the catalog loader for all english-based dialects, and
	// (wrongly) enumerate a bunch of locales that don't actually exist.

	if( locale->IsOk() && locale->AddCatalog( L"pcsx2_Main", wxLANGUAGE_UNKNOWN, NULL ) )
		langs.push_back( LangPackEnumeration( wxLangId ) );

	delete locale;
}

// Finds all valid PCSX2 language packs, and enumerates them for configuration selection.
// Note: On linux there's no easy way to reliably enumerate language packs, since every distro
// could use its own location for installing pcsx2.mo files (wtcrap?).  Furthermore wxWidgets
// doesn't give us a public API for checking what the language search paths are.  So the only
// safe way to enumerate the languages is by forcibly loading every possible locale in the wx
// database.  Anything which hasn't been installed will fail to load.
//
// Because loading and hashing the entire pcsx2 translation for every possible language would be
// asinine and slow, I've decided to use a file dedicated to being a translation detection anchor.
// This file is named pcsx2ident.mo, and contains only the name of the language in the language
// native form.  Additional translation files are only loaded if the user picks the language (or
// if it's the default language of the user's OS installation).
//
void i18n_EnumeratePackages( LangPackList& langs )
{
	wxDoNotLogInThisScope here;		// wx generates verbose errors if languages don't exist, so disable them here.
	langs.push_back( LangPackEnumeration() );

	for( int li=wxLANGUAGE_UNKNOWN+1; li<wxLANGUAGE_USER_DEFINED; ++li )
	{
		i18n_DoPackageCheck( (wxLanguage)li, langs );
	}

	// Brilliant.  Because someone in the wx world didn't think to move wxLANGUAGE_USER_DEFINED
	// to a place where it wasn't butt right up against the main languages (like, say, start user
	// defined values at 4000 or something?), they had to add new languages in at some arbitrary
	// value instead.  Let's handle them here:
	// fixme: these won't show up in alphabetical order if they're actually present (however
	// horribly unlikely that is)... do we care?  Probably not.

	// Note: These aren't even available in some packaged Linux distros anyway. >_<

	//i18n_DoPackageCheck( wxLANGUAGE_VALENCIAN, englishNames, xlatedNames );
	//i18n_DoPackageCheck( wxLANGUAGE_SAMI, englishNames, xlatedNames );
}

bool i18n_SetLanguage( const wxString& langCode )
{
	if (langCode.IsEmpty() || langCode.CmpNoCase(L"default"))
	{
		wxLanguage sysLang = (wxLanguage)wxLocale::GetSystemLanguage();

		if (sysLang == wxLANGUAGE_UNKNOWN)
			sysLang = wxLANGUAGE_ENGLISH;
	}

	const wxLanguageInfo* woot = wxLocale::FindLanguageInfo( langCode );
	if (!woot) return false;
	return i18n_SetLanguage( woot->Language );
}

bool i18n_SetLanguage( int wxLangId )
{
	if( !wxLocale::IsAvailable( wxLangId ) )
	{
		Console.Warning( "Invalid Language Identifier (wxID=%d).", wxLangId );
		return false;
	}

	ScopedPtr<wxLocale> locale( new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING ) );

	if( !locale->IsOk() )
	{
		Console.Warning( L"SetLanguage: '%s' [%s] is not supported by the operating system",
			wxLocale::GetLanguageName( locale->GetLanguage() ).c_str(), locale->GetCanonicalName().c_str()
		);
		return false;
	}

	wxLangId = locale->GetLanguage();
	
	if (wxLangId == wxLANGUAGE_UNKNOWN)
	{
		Console.WriteLn("System-default language is unknown?  Defaulting back to English/US.");
		wxLangId = wxLANGUAGE_ENGLISH;
		return true;
	}

	// English/US is built in, so no need to load MO/PO files.
	if( pxIsEnglish(wxLangId) ) return true;
	
	Console.WriteLn( Color_StrongBlack, L"Loading language translation databases for '%s' [%s]",
		wxLocale::GetLanguageName( locale->GetLanguage() ).c_str(), locale->GetCanonicalName().c_str()
	);

	static const wxChar* dictFiles[] =
	{
		L"pcsx2_Main",
		L"pcsx2_Iconized",
		L"pcsx2_Tertiary"
	};
	
	bool foundone = false;
	for (uint i=0; i<ArraySize(dictFiles); ++i)
	{
		if (!dictFiles[i]) continue;

		if (!locale->AddCatalog(dictFiles[i]))
			Console.Indent().WriteLn(Color_StrongYellow, "%s not found -- translation dictionary may be incomplete.", dictFiles[i]);
		else
			foundone = true;
	}

	if (!foundone)	
	{
		Console.Warning("SetLanguage: Requested translation is not implemented yet, using English.");
		return false;
	}

	locale.DetachPtr();
	return true;
}
