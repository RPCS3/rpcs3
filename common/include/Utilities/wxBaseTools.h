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

#include "Dependencies.h"

// --------------------------------------------------------------------------------------
// wxBaseTools.h
//
// This file is meant to contain utility classes for users of the wxWidgets library.
// All classes in this file are strictly dependent on wxBase libraries only, meaning
// you don't need to include or link against wxCore (GUI) to build them.  For tools
// which require wxCore, see wxGuiTools.h
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
//  wxDoNotLogInThisScope
// --------------------------------------------------------------------------------------
// This class is used to disable wx's sometimes inappropriate amount of forced error logging
// during specific activities.  For example, when using wxDynamicLibrary to detect the
// validity of DLLs, wx will log errors for missing symbols. (sigh)
//
// Usage: Basic auto-cleanup destructor class.  Create an instance inside a scope, and
// logging will be re-enabled when scope is terminated. :)
//
class wxDoNotLogInThisScope
{
	DeclareNoncopyableObject(wxDoNotLogInThisScope);

protected:
	bool m_prev;

public:
	wxDoNotLogInThisScope() :
		m_prev( wxLog::EnableLogging( false ) )
	{
	}

	~wxDoNotLogInThisScope()
	{
		wxLog::EnableLogging( m_prev );
	}
};

// --------------------------------------------------------------------------------------
//  wxToUTF8  -  shortcut for str.ToUTF8().data()
// --------------------------------------------------------------------------------------
class wxToUTF8
{
	DeclareNoncopyableObject( wxToUTF8 );

protected:
	wxCharBuffer m_charbuffer;

public:
	wxToUTF8( const wxString& str ) : m_charbuffer( str.ToUTF8().data() ) { }
	virtual ~wxToUTF8() throw() {}
	
	operator const char*() { return m_charbuffer.data(); }
};

// This class provided for completeness sake.  You probably should use ToUTF8 instead.
class wxToAscii
{
	DeclareNoncopyableObject( wxToAscii );

protected:
	wxCharBuffer m_charbuffer;

public:
	wxToAscii( const wxString& str ) : m_charbuffer( str.ToAscii().data() ) { }
	virtual ~wxToAscii() throw() {}

	operator const char*() { return m_charbuffer.data(); }
};

