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

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/gdicmn.h>		// for wxPoint/wxRect stuff

//////////////////////////////////////////////////////////////////////////////////////////
// Helpers for wxWidgets stuff!
//

// wxWidgets lacks one of its own...
extern const wxRect wxDefaultRect;

// This should prove useful....
#define wxsFormat wxString::Format

extern void SplitString( wxArrayString& dest, const wxString& src, const wxString& delims );
extern void JoinString( wxString& dest, const wxArrayString& src, const wxString& separator );

extern wxString ToString( const wxPoint& src, const wxString& separator=L"," );
extern wxString ToString( const wxSize& src, const wxString& separator=L"," );
extern wxString ToString( const wxRect& src, const wxString& separator=L"," );

extern bool TryParse( wxPoint& dest, const wxStringTokenizer& parts );
extern bool TryParse( wxSize& dest, const wxStringTokenizer& parts );

extern bool TryParse( wxPoint& dest, const wxString& src, const wxPoint& defval=wxDefaultPosition, const wxString& separators=L",");
extern bool TryParse( wxSize& dest, const wxString& src, const wxSize& defval=wxDefaultSize, const wxString& separators=L",");
extern bool TryParse( wxRect& dest, const wxString& src, const wxRect& defval=wxDefaultRect, const wxString& separators=L",");


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
