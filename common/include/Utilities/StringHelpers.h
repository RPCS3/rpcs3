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

#pragma once

#include "Dependencies.h"
#include "SafeArray.h"

#include <wx/tokenzr.h>


// --------------------------------------------------------------------------------------
//  pxToUTF8
// --------------------------------------------------------------------------------------
// Converts a string to UTF8 and provides an interface for getting its length.
class pxToUTF8
{
	DeclareNoncopyableObject( pxToUTF8 );

protected:
	wxCharBuffer	m_result;
	int				m_length;

public:
	explicit pxToUTF8(const wxString& src)
		: m_result( src.ToUTF8() )
	{
		m_length = -1;
	}

	size_t Length()
	{
		if( -1 == m_length )
			m_length = strlen( m_result );
		return m_length;
	}

	void Convert( const wxString& src )
	{
		m_result = src.ToUTF8();
		m_length = -1;
	}
	
	const char* data() const { return m_result; }
	
	operator const char*() const
	{
		return m_result.data();
	}
};

extern void px_fputs( FILE* fp, const char* src );

extern wxString fromUTF8( const char* src );
extern wxString fromAscii( const char* src );

// wxWidgets lacks one of its own...
extern const wxRect wxDefaultRect;

extern void SplitString( wxArrayString& dest, const wxString& src, const wxString& delims, wxStringTokenizerMode mode = wxTOKEN_RET_EMPTY_ALL );
extern wxString JoinString( const wxArrayString& src, const wxString& separator );
extern wxString JoinString( const wxChar** src, const wxString& separator );

extern wxString ToString( const wxPoint& src, const wxString& separator=L"," );
extern wxString ToString( const wxSize& src, const wxString& separator=L"," );
extern wxString ToString( const wxRect& src, const wxString& separator=L"," );

extern bool TryParse( wxPoint& dest, const wxStringTokenizer& parts );
extern bool TryParse( wxSize& dest, const wxStringTokenizer& parts );

extern bool TryParse( wxPoint& dest, const wxString& src, const wxPoint& defval=wxDefaultPosition, const wxString& separators=L",");
extern bool TryParse( wxSize& dest, const wxString& src, const wxSize& defval=wxDefaultSize, const wxString& separators=L",");
extern bool TryParse( wxRect& dest, const wxString& src, const wxRect& defval=wxDefaultRect, const wxString& separators=L",");

// --------------------------------------------------------------------------------------
//  ParsedAssignmentString
// --------------------------------------------------------------------------------------
// This class is a simple helper for parsing INI-style assignments, in the typical form of:
//    variable = value
//    filename = SomeString.txt
//    integer  = 15
//
// This parser supports both '//' and ';' at the head of a line as indicators of a commented
// line, and such a line will return empty strings for l- and r-value.
//
// No type handling is performed -- the user must manually parse strings into integers, etc.
// For advanced "fully functional" ini file parsing, consider using wxFileConfig instead.
//
struct ParsedAssignmentString
{
	wxString	lvalue;
	wxString	rvalue;
	bool		IsComment;

	ParsedAssignmentString( const wxString& src );
};

// --------------------------------------------------------------------------------------
//  FastFormatAscii
// --------------------------------------------------------------------------------------
// Fast formatting of ASCII text.  This class uses a process-wide format buffer that is
// allocated only once and grown to accommodate string formatting needs.  The buffer is
// thread-safe.  This technique reduces the overhead of formatting strings by eliminating
// most or all heap allocation actions.
//
class FastFormatAscii
{
protected:
	SafeArray<char>*	m_dest;
	bool				m_deleteDest;
	
public:
	FastFormatAscii();
	~FastFormatAscii() throw();
	FastFormatAscii& Write( const char* fmt, ... );
	FastFormatAscii& WriteV( const char* fmt, va_list argptr );

	const char* GetResult() const;
	operator const char*() const;

	const wxString GetString() const;
	operator wxString() const;
};

class FastFormatUnicode
{
protected:
	SafeArray<char>*	m_dest;
	bool				m_deleteDest;

public:
	FastFormatUnicode();
	~FastFormatUnicode() throw();

	FastFormatUnicode& Write( const char* fmt, ... );
	FastFormatUnicode& Write( const wxChar* fmt, ... );
	FastFormatUnicode& WriteV( const char* fmt, va_list argptr );
	FastFormatUnicode& WriteV( const wxChar* fmt, va_list argptr );

	const wxChar* GetResult() const;
	const wxString GetString() const;

	operator const wxChar*() const
	{
		return (const wxChar*)m_dest->GetPtr();
	}

	operator wxString() const
	{
		return (const wxChar*)m_dest->GetPtr();
	}
};

extern bool pxParseAssignmentString( const wxString& src, wxString& ldest, wxString& rdest );

#define pxsFmt FastFormatUnicode().Write
#define pxsFmtV FastFormatUnicode().WriteV

//////////////////////////////////////////////////////////////////////////////////////////
// Custom internal sprintf functions, which are ASCII only (even in UNICODE builds)
//
// These functions are useful since they are ASCII always, even under Unicode.  Typically
// even in a unicode app.

extern void ssprintf(std::string& dest, const char* fmt, ...);
extern void ssappendf(std::string& dest, const char* format, ...);
extern void vssprintf(std::string& dest, const char* format, va_list args);
extern void vssappendf(std::string& dest, const char* format, va_list args);

extern std::string fmt_string( const char* fmt, ... );
extern std::string vfmt_string( const char* fmt, va_list args );
