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
	wxLanguage	wxLangId;
	wxString	canonicalName;
	wxString	englishName;

	// [TODO] : Might be nice, but have no idea how to implement it...
	//wxString	xlatedName;

public:
	LangPackEnumeration( wxLanguage langId );
	LangPackEnumeration();
};

typedef std::vector<LangPackEnumeration> LangPackList;

extern bool i18n_SetLanguage( wxLanguage wxLangId, const wxString& langCode=wxEmptyString );
extern void i18n_EnumeratePackages( LangPackList& langs );
extern bool i18n_IsLegacyLanguageId( wxLanguage lang );
extern void i18n_SetLanguagePath();

