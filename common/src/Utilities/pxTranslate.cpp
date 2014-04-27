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
//  Was replaced by a standard implementation of wxGetTranslation
// --------------------------------------------------------------------------------------
const wxChar* __fastcall pxExpandMsg( const wxChar* englishContent )
{
	return wxGetTranslation( englishContent );
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
