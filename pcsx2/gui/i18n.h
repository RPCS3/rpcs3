/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
 
#pragma once

#include <wx/wx.h>


class LangPackEnumeration
{
public:
	wxLanguage wxLangId;
	wxString englishName;
	wxString xlatedName;
	
public:
	LangPackEnumeration( wxLanguage langId );
	LangPackEnumeration();
};

typedef std::vector<LangPackEnumeration> LangPackList;

extern bool i18n_SetLanguage( int wxLangId );
extern void i18n_EnumeratePackages( LangPackList& langs );

extern const wxChar* __fastcall pxExpandMsg( const wxChar* key, const wxChar* englishContent );
extern const wxChar* __fastcall pxGetTranslation( const wxChar* message );

//////////////////////////////////////////////////////////////////////////////////////////
// Translation Feature: pxE is used as a method of dereferencing very long english text
// descriptions via a "key" identifier.  In this way, the english text can be revised without
// it breaking existing translation bindings.  Make sure to add pxE to your PO catalog's
// source code identifiers, and then reference the source code to see what the current
// english version is.
//
#define pxE(key,english)		pxExpandMsg( wxT(key), english )

