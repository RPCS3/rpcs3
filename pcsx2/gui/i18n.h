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
 
#pragma once

#include <wx/wx.h>

//////////////////////////////////////////////////////////////////////////////////////////
//
enum ExpandedMsgEnum
{
	Msg_Dialog_AdvancedPaths,
	Msg_Popup_MissingPlugins,

	Msg_Tooltips_Savestates,
	Msg_Tooltips_Snapshots,
	Msg_Tooltips_Bios,
	Msg_Tooltips_Logs,
	Msg_Tooltips_Memorycards,
	Msg_Tooltips_SettingsPath,
	Msg_Tooltips_PluginsPath,
	
	ExpandedMsg_Count
};

//////////////////////////////////////////////////////////////////////////////////////////
// English Tables for "Translating" iconized UI descriptions.
//
struct HashedExpansionPair
{
	const wxChar* gettextKey;			// send this to wxGetTranslation()
	const wxChar* Expanded;
};

struct EnglishExpansionEntry
{
	ExpandedMsgEnum Key;
	const wxChar* gettextKey;			// send this to wxGetTranslation()
	const wxChar* Expanded;
};

extern void i18n_InitPlainEnglish();
extern bool i18n_SetLanguage( int wxLangId );

extern const wxChar* __fastcall pxExpandMsg( ExpandedMsgEnum key );
extern const wxChar* __fastcall pxGetTranslation( const wxChar* message );

#define pxE(n)		pxExpandMsg( n )

