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

// Some of the codes provided by wxWidgets are 'obsolete' -- effectively replaced by more specific
// region-qualified language codes.  This function can be used to filter them out.
bool i18n_IsLegacyLanguageId( wxLanguage lang )
{
	return
		(lang == wxLANGUAGE_ENGLISH) ||
		(lang == wxLANGUAGE_CHINESE) ||
		(lang == wxLANGUAGE_CHINESE_TRADITIONAL) ||
		(lang == wxLANGUAGE_SERBIAN) ||
		(lang == wxLANGUAGE_SPANISH);
}

static wxString i18n_GetBetterLanguageName( const wxLanguageInfo* info )
{
	switch (info->Language)
	{
		case wxLANGUAGE_CHINESE:				return L"Chinese (Traditional)";
		case wxLANGUAGE_CHINESE_TRADITIONAL:	return L"Chinese (Traditional)";
		case wxLANGUAGE_CHINESE_TAIWAN:			return L"Chinese (Traditional)";
		case wxLANGUAGE_CHINESE_HONGKONG:		return L"Chinese (Traditional, Hong Kong)";
		case wxLANGUAGE_CHINESE_MACAU:			return L"Chinese (Traditional, Macau)";
	}

	return info->Description;
}

LangPackEnumeration::LangPackEnumeration( wxLanguage langId )
{
	wxLangId = langId;

	if (const wxLanguageInfo* info = wxLocale::GetLanguageInfo( wxLangId ))
	{
		canonicalName = info->CanonicalName;
		englishName = i18n_GetBetterLanguageName(info);
	}
}

LangPackEnumeration::LangPackEnumeration()
{
	wxLangId = wxLANGUAGE_DEFAULT;
	englishName = L" System Default";		// left-side space forces it to sort to the front of the lists
	englishName += _(" (default)");
	canonicalName = L"default";

	int sysLang = wxLocale::GetSystemLanguage();

	if (sysLang == wxLANGUAGE_UNKNOWN)
		sysLang = wxLANGUAGE_ENGLISH_US;

	//if (const wxLanguageInfo* info = wxLocale::GetLanguageInfo( sysLang ))
	//	englishName += L" [" + i18n_GetBetterLanguageName(info) + L"]";
}

static void i18n_DoPackageCheck( wxLanguage wxLangId, LangPackList& langs, bool& valid_stat )
{
	valid_stat = false;
	if( i18n_IsLegacyLanguageId( wxLangId ) ) return;

	// note: wx preserves the current locale for us, so creating a new locale and deleting
	// will not affect program status.
	ScopedPtr<wxLocale> locale( new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING ) );

	// Force the msgIdLanguage param to wxLANGUAGE_UNKNOWN to disable wx's automatic english
	// matching logic, which will bypass the catalog loader for all english-based dialects, and
	// (wrongly) enumerate a bunch of locales that don't actually exist.

	if ((locale->GetLanguage() == wxLANGUAGE_ENGLISH_US) ||
		(locale->IsOk() && locale->AddCatalog( L"pcsx2_Main", wxLANGUAGE_UNKNOWN, NULL )) )
	{
		langs.push_back( LangPackEnumeration( wxLangId ) );
		valid_stat = true;
	}
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
	bool valid_stat_dummy;

	for( int li=wxLANGUAGE_UNKNOWN+1; li<wxLANGUAGE_USER_DEFINED; ++li )
	{
		i18n_DoPackageCheck( (wxLanguage)li, langs, valid_stat_dummy );
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

#if 0
// warning: wxWidgets uses duplicated canonical codes for many languages, and has some bizarre
// matching heuristics.  Using this function doesn't really match the language and sublanguage
// (dialect) that the user selected.
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
#endif

// Multiple language ID are actually mostly identical languages. In case a translation is not
// yet provided for the dialect fallback to the main one. It would reduce the burden of the
// translators and reduce the language pack.
static wxLanguage i18n_FallbackToAnotherLang( wxLanguage wxLangId )
{
	// if some translations are provided, do not change the language
	LangPackList dummy_lang;
	bool valid_stat = false;
	i18n_DoPackageCheck(wxLangId, dummy_lang, valid_stat);
	if (valid_stat) return wxLangId;

	switch(wxLangId)
	{
		case wxLANGUAGE_CHINESE_HONGKONG     : 
		case wxLANGUAGE_CHINESE_MACAU        : return wxLANGUAGE_CHINESE_TRADITIONAL;

		case wxLANGUAGE_CHINESE_SINGAPORE    : return wxLANGUAGE_CHINESE_SIMPLIFIED;

		case wxLANGUAGE_SAMI                 : 
		case wxLANGUAGE_SWEDISH_FINLAND      : return wxLANGUAGE_SWEDISH;

		case wxLANGUAGE_PORTUGUESE           : return wxLANGUAGE_PORTUGUESE_BRAZILIAN;

		// Overkill 9000?
		case wxLANGUAGE_GERMAN_AUSTRIAN      : 
		case wxLANGUAGE_GERMAN_BELGIUM       : 
		case wxLANGUAGE_GERMAN_LIECHTENSTEIN : 
		case wxLANGUAGE_GERMAN_LUXEMBOURG    : 
		case wxLANGUAGE_GERMAN_SWISS         : return wxLANGUAGE_GERMAN;

		case wxLANGUAGE_ITALIAN_SWISS        : return wxLANGUAGE_ITALIAN;
		
		default                              : break;
	}
	return wxLangId;
}

// This method sets the requested language, based on wxLanguage id and an optional 'confirmation'
// canonical code.  If the canonical code is provided, it is used to confirm that the ID matches
// the intended language/dialect.  If the ID and canonical do not match, this method will use
// wx's FindLAnguageInfo to provide a "best guess" canonical match (usually relying on the user's
// operating system default).
//
// Rationale: wxWidgets language IDs are just simple enums, and not especially unique.  Future
// versions of PCSX2 may have language ID changes if built against new/different versions of wx.
// To prevent PCSX2 from selecting a completely wrong language when upgraded, we double-check
// the wxLanguage code against the canonical name.  We can't simply use canonical names either
// because those are not unique (dialects of chinese, for example), and wx returns the generic
// form over a specific dialect, when given a choice.  Thus a two-tier check is required.
//
//  wxLanguage for specific dialect, and canonical as a fallback/safeguard in case the wxLanguage
//  id appears to be out of date.
//
//
bool i18n_SetLanguage( wxLanguage wxLangId, const wxString& langCode )
{
	const wxLanguageInfo* info = wxLocale::GetLanguageInfo(wxLangId);

	// Check if you can load a similar language in case the current one is not yet provided
	if (info) {
		wxLanguage LangId_fallback = i18n_FallbackToAnotherLang((wxLanguage)info->Language);
		if (LangId_fallback != (wxLanguage)info->Language)
			info = wxLocale::GetLanguageInfo(LangId_fallback);
	}

	// note: language canonical name mismatch probably means wxWidgets version changed since 
	// the user's ini file was provided.  Missing/invalid ID probably means the same thing.
	// If either is true, and the caller provided a canonical name, then let wx do a best
	// match based on the canonical name.

	if (!info || (!langCode.IsEmpty() && (langCode.CmpNoCase(info->CanonicalName) != 0)))
	{
		if (!info)
			Console.Warning( "Invalid language identifier (wxID=%d)", wxLangId );

		if (!langCode.IsEmpty() && (langCode.CmpNoCase(L"default")!=0))
		{
			info = wxLocale::FindLanguageInfo(langCode);
			if (!info)
				Console.Warning( "Unrecognized language canonical name '%ls'", langCode.c_str() );
		}
	}

	if (!info) return false;
	if (wxGetLocale() && (info->Language == wxGetLocale()->GetLanguage())) return true;
	
	ScopedPtr<wxLocale> locale( new wxLocale(info->Language) );

	if( !locale->IsOk() )
	{
		Console.Warning( L"SetLanguage: '%s' [%s] is not supported by the operating system",
			i18n_GetBetterLanguageName(info).c_str(), locale->GetCanonicalName().c_str()
		);
		return false;
	}

	wxLangId = (wxLanguage)locale->GetLanguage();
	
	if (wxLangId == wxLANGUAGE_UNKNOWN)
	{
		Console.WriteLn("System-default language is unknown?  Defaulting back to English/US.");
		wxLangId = wxLANGUAGE_ENGLISH_US;
	}

	// English/US is built in, so no need to load MO/PO files.
	if( pxIsEnglish(wxLangId) )
	{
		locale.DetachPtr();
		return true;
	}
	
	Console.WriteLn( L"Loading language translation databases for '%s' [%s]",
		i18n_GetBetterLanguageName(info).c_str(), locale->GetCanonicalName().c_str()
	);

	static const wxChar* dictFiles[] =
	{
		L"pcsx2_Main",
		L"pcsx2_Iconized"
	};
	
	bool foundone = false;
	for (uint i=0; i<ArraySize(dictFiles); ++i)
	{
		if (!dictFiles[i]) continue;

		if (!locale->AddCatalog(dictFiles[i]))
			Console.Indent().WriteLn(Color_StrongYellow, "%ls not found -- translation dictionary may be incomplete.", dictFiles[i]);
		else
			foundone = true;
	}

	if (!foundone)	
	{
		Console.Warning("SetLanguage: Requested translation is not implemented yet.");
		return false;
	}

	locale.DetachPtr();
	return true;
}

// This method sets the lookup path to search l10n files
void i18n_SetLanguagePath()
{
	// default location for windows
	wxLocale::AddCatalogLookupPathPrefix( wxGetCwd() );
	// additional location for linux
#ifdef __LINUX__
	wxLocale::AddCatalogLookupPathPrefix( PathDefs::GetLangs().ToString() );
#endif

}
