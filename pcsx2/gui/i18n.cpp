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
	: wxLangId( langId )
	, englishName( wxLocale::GetLanguageName( wxLangId ) )
	, xlatedName( pxIsEnglish( wxLangId ) ? wxEmptyString : wxGetTranslation( L"NativeName" ) )
{
}

LangPackEnumeration::LangPackEnumeration()
	: wxLangId( wxLANGUAGE_DEFAULT )
	, englishName( L" System Default" )		// left-side space forces it to sort to the front of the lists
	, xlatedName()
{
	int sysLang( wxLocale::GetSystemLanguage() );
	if( sysLang != wxLANGUAGE_UNKNOWN )
		englishName += L"  [" + wxLocale::GetLanguageName( sysLang ) + L"]";
}


static void i18n_DoPackageCheck( wxLanguage wxLangId, LangPackList& langs )
{
	// Plain english is a special case that's built in, and we only want it added to the list
	// once, so we check for wxLANGUAGE_ENGLISH and then ignore other IsEnglish ids below.
	if( wxLangId == wxLANGUAGE_ENGLISH )
		langs.push_back( LangPackEnumeration( wxLangId ) );

	if( pxIsEnglish( wxLangId ) ) return;

	// Note: wx auto-preserves the current locale for us
	if( !wxLocale::IsAvailable( wxLangId ) ) return;
	wxLocale* locale = new wxLocale( wxLangId, wxLOCALE_CONV_ENCODING );

	// Force the msgIdLanguage param to wxLANGUAGE_UNKNOWN to disable wx's automatic english
	// matching logic, which will bypass the catalog loader for all english-based dialects, and
	// (wrongly) enumerate a bunch of locales that don't actually exist.

	if( locale->IsOk() && locale->AddCatalog( L"pcsx2ident", wxLANGUAGE_UNKNOWN, NULL ) )
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
// Because loading and hashing the entire pcsx2 translation for every possible language would
// asinine and slow, I've decided to use a two-file translation system.  One file (pcsx2ident.mo)
// is very small and simply contains the name of the language in the language native.  The
// second file (pcsx2.mo) is loaded only if the user picks the language (or if it's the default
// language of the user's OS installation).
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

	if( !pxIsEnglish(wxLangId) && !locale->AddCatalog( L"pcsx2main" ) )
	{
		/*Console.Warning( L"SetLanguage: Cannot find pcsx2main.mo file for language '%s' [%s]",
			wxLocale::GetLanguageName( locale->GetLanguage() ).c_str(), locale->GetCanonicalName().c_str()
		);*/
		Console.Warning("SetLanguage is not implemented yet, using English.");
		return false;
	}

	locale.DetachPtr();
	return true;
}
