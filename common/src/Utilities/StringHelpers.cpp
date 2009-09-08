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
 
#include "PrecompiledHeader.h"

const wxRect wxDefaultRect( wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, wxDefaultCoord );

//////////////////////////////////////////////////////////////////////////////////////////
// Splits a string into parts and adds the parts into the given SafeList.
// This list is not cleared, so concatenating many splits into a single large list is
// the 'default' behavior, unless you manually clear the SafeList prior to subsequent calls.
//
// Note: wxWidgets 2.9 / 3.0 has a wxSplit function, but we're using 2.8 so I had to make
// my own.
void SplitString( SafeList<wxString>& dest, const wxString& src, const wxString& delims )
{
	wxStringTokenizer parts( src, delims );
	while( parts.HasMoreTokens() )
		dest.Add( parts.GetNextToken() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Joins a list of strings into one larger string, using the given string concatenation
// character as a separator.  If you want to be able to split the string later then the
// concatenation string needs to be a single character.
//
// Note: wxWidgets 2.9 / 3.0 has a wxJoin function, but we're using 2.8 so I had to make
// my own.
void JoinString( wxString& dest, const SafeList<wxString>& src, const wxString& separator )
{
	for( int i=0, len=src.GetLength(); i<len; ++i )
	{
		if( i != 0 )
			dest += separator;
		dest += src[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Attempts to parse and return a value for the given template type, and throws a ParseError
// exception if the parse fails.  The template type can be anything that is supported/
// implemented via one of the TryParse() method overloads.
//
// This, so far, include types such as wxPoint, wxRect, and wxSize.
//
template< typename T >
T Parse( const wxString& src, const wxString& separators=L",")
{
	T retval;
	if( !TryParse( retval, src, separators ) )
		throw Exception::ParseError( "Parse failure on call to " + wxString::FromAscii(__WXFUNCTION__) + ": " + src );
	return retval;
}


//////////////////////////////////////////////////////////////////////////////////////////
// ToString helpers for wxString!
//

// Converts a wxPoint into a comma-delimited string!
wxString ToString( const wxPoint& src, const wxString& separator )
{
	return wxString() << src.x << separator << src.y;
}

wxString ToString( const wxSize& src, const wxString& separator )
{
	return wxString() << src.GetWidth() << separator << src.GetHeight();
}

// Converts a wxRect into a comma-delimited string!
// Example: 32,64,128,5
wxString ToString( const wxRect& src, const wxString& separator )
{
	return ToString( src.GetLeftTop(), separator ) << separator << ToString( src.GetSize(), separator );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Parse helpers for wxString!
//

bool TryParse( wxPoint& dest, wxStringTokenizer& parts )
{
	long result[2];

	if( !parts.HasMoreTokens() || !parts.GetNextToken().ToLong( &result[0] ) ) return false;
	if( !parts.HasMoreTokens() || !parts.GetNextToken().ToLong( &result[1] ) ) return false;
	dest.x = result[0];
	dest.y = result[1];

	return true;
}

bool TryParse( wxSize& dest, wxStringTokenizer& parts )
{
	long result[2];

	if( !parts.HasMoreTokens() || !parts.GetNextToken().ToLong( &result[0] ) ) return false;
	if( !parts.HasMoreTokens() || !parts.GetNextToken().ToLong( &result[1] ) ) return false;
	dest.SetWidth( result[0] );
	dest.SetHeight( result[1] );

	return true;
}

// Tries to parse the given string into a wxPoint value at 'dest.'  If the parse fails, the
// method aborts and returns false.
bool TryParse( wxPoint& dest, const wxString& src, const wxPoint& defval, const wxString& separators )
{
	dest = defval;
	wxStringTokenizer parts( src, separators );
	return TryParse( dest, parts );
}

bool TryParse( wxSize& dest, const wxString& src, const wxSize& defval, const wxString& separators )
{
	dest = defval;
	wxStringTokenizer parts( src, separators );
	return TryParse( dest, parts );
}

bool TryParse( wxRect& dest, const wxString& src, const wxRect& defval, const wxString& separators )
{
	dest = defval;

	wxStringTokenizer parts( src, separators );
	
	wxPoint point;
	wxSize size;

	if( !TryParse( point, parts ) ) return false;
	if( !TryParse( size, parts ) ) return false;

	dest = wxRect( point, size );
	return true;
}
