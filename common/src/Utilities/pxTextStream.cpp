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
#include "wxBaseTools.h"
#include <wx/stream.h>


// Returns TRUE if the source is UTF8, or FALSE if it's just ASCII crap.
bool pxReadLine( wxInputStream& input, std::string& dest )
{
	dest.clear();
	bool isUTF8 = false;
	while ( true )
	{
		char c;
		input.Read(&c, sizeof(c));
		if( c == 0 )			break;
		if( input.Eof() )		break;
		if( c == '\n' )			break; // eat on UNIX
		if( c == '\r' )
		{
			input.Read(&c, sizeof(c));
			if( c == 0 )			break;
			if( input.Eof() )		break;
			if( c == '\n' )			break;

			input.Ungetch(c);
			break;
		}
		dest += c;
		if( c & 0x80 ) isUTF8 = true;
	}
	
	return isUTF8;
}

void pxReadLine( wxInputStream& input, wxString& dest, std::string& intermed )
{
	dest.clear();
	if( pxReadLine( input, intermed ) )
		dest = fromUTF8(intermed.c_str());
	else
	{
		// Optimized ToAscii conversion.
		// wx3.0 : NOT COMPATIBLE!!  (on linux anyway)
		const char* ascii = intermed.c_str();
		while( *ascii != 0 ) dest += (wchar_t)(unsigned char)*ascii++;
	}
}

void pxReadLine( wxInputStream& input, wxString& dest )
{
	std::string line;
	pxReadLine( input, dest, line );
}

wxString pxReadLine( wxInputStream& input )
{
	wxString result;
	pxReadLine( input, result );
	return result;
}

void pxWriteLine( wxOutputStream& output )
{
	output.Write( "\n", 1 );
}

void pxWriteLine( wxOutputStream& output, const wxString& text )
{
	if( !text.IsEmpty() )
	{
		pxToUTF8 utf8(text);
		output.Write(utf8, utf8.Length());
	}
	pxWriteLine( output );
}

void pxWriteMultiline( wxOutputStream& output, const wxString& src )
{
	if( src.IsEmpty() ) return;

	wxString result( src );
	result.Replace( L"\r\n", L"\n" );
	result.Replace( L"\r", L"\n" );
	
	pxToUTF8 utf8(result);
	output.Write(utf8, utf8.Length());
}
