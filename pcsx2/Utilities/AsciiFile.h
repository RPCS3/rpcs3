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

#include <wx/file.h>

//////////////////////////////////////////////////////////////////////////////////////////
// Helper class for wxFile which provides old-school ASCII interfaces (char* style),
// for saving some scrodom-kicking pain involved in converting log dumps and stuff over
// to unicode.
//
// This is an ideal solution on several fronts since it is both faster, and fully func-
// tional (since the dumps are only ever english/ascii only).
//
class AsciiFile : public wxFile
{
public:
	using wxFile::Write;

	AsciiFile( const wxString& src, OpenMode mode = read ) :
		wxFile( src, mode ) {}
	
	void Printf( const char* fmt, ... );

	void Write( const char* fmt )
	{
		Write( fmt, strlen( fmt ) );
	}

};
