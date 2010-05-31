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
// Valid prefix types:
//
// .Panel:   Key-based translation of a panel or dialog text; usually either a header or
//           checkbox description, by may also include some controls with long labels.
//           These have the highest translation priority.
//
// .Popup:   Key-based translation of a popup dialog box; either a notice, confirmation,
//           or error.  These typically have very high translation priority (roughly equal
//           or slightly less than pxE_Panel).
//
// .Error    Key-based translation of error messages, typically used when throwing exceptions
//           that have end-user errors.  These are normally (but not always) displayed as popups
//           to the user.  Translation priority is medium.
//
// .Wizard   Key-based translation of a heading, checkbox item, description, or other text
//           associated with the First-time wizard.  Translation of these items is considered
//           lower-priority to most other messages; but equal or higher priority to tooltips.
//
// .Tooltip: Key-based translation of a tooltip for a control on a dialog/panel.  Translation
//           of these items is typically considered "lowest priority" as they usually provide
//           the most tertiary of info to the user.
//

#define pxE(key, english)			pxExpandMsg( wxT(key),						english )
