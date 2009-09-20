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

wxString GetEnglish( const char* msg )
{
	return wxString::FromAscii(msg);
}

wxString GetTranslation( const char* msg )
{
	return wxGetTranslation( wxString::FromAscii(msg).c_str() );
}

// ------------------------------------------------------------------------
// Force DevAssert to *not* inline for devel/debug builds (allows using breakpoints to trap
// assertions), and force it to inline for release builds (optimizes it out completely since
// IsDevBuild is false).  Since Devel builds typically aren't enabled with Global Optimization/
// LTCG, this currently isn't even necessary.  But might as well, in case we decide at a later
// date to re-enable LTCG for devel.
//
#ifdef PCSX2_DEVBUILD
#	define DEVASSERT_INLINE __noinline
#else
#	define DEVASSERT_INLINE __forceinline
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Assertion tool for Devel builds, intended for sanity checking and/or bounds checking
// variables in areas which are not performance critical.
//
// How it works: This function throws an exception of type Exception::AssertionFailure if
// the assertion conditional is false.  Typically for the end-user, this exception is handled
// by the general handler, which (should eventually) create some state dumps and other
// information for troubleshooting purposes.
//
// From a debugging environment, you can trap your DevAssert by either breakpointing the
// exception throw below, or by adding Exception::LogicError to your First-Chance Exception
// catch list (Visual Studio, under the Debug->Exceptions menu/dialog).  You should have
// LogicErrors enabled as First-Chance exceptions regardless, so do it now. :)
//
// Returns:
//   FALSE if the assertion succeeded (condition is valid), or true if the assertion
//   failed.  The true clause is only reachable in release builds, and can be used by code
//   to provide a "stable" escape clause for unexpected behavior.
//
DEVASSERT_INLINE bool DevAssert( bool condition, const char* msg )
{
	if( condition ) return false;
	if( IsDevBuild )
		throw Exception::LogicError( msg );

	wxASSERT_MSG_A( false, msg );
	return true;
}

// --------------------------------------------------------------------------------------
//  Exception Namespace Implementations  (Format message handlers for general exceptions)
// --------------------------------------------------------------------------------------

Exception::BaseException::~BaseException() throw() {}

void Exception::BaseException::InitBaseEx( const wxString& msg_eng, const wxString& msg_xlt )
{
	m_message_diag = msg_eng;
	m_message_user = msg_xlt;

	// Linux/GCC exception handling is still suspect (this is likely to do with GCC more
	// than linux), and fails to propagate exceptions up the stack from EErec code.  This
	// could likely be because of the EErec using EBP.  So to ensure the user at least
	// gets a log of the error, we output to console here in the constructor.

#ifdef __LINUX__
    //wxLogError( msg_eng.c_str() );
    Console::Error( msg_eng );
#endif
}

// given message is assumed to be a translation key, and will be stored in translated
// and untranslated forms.
void Exception::BaseException::InitBaseEx( const char* msg_eng )
{
	m_message_diag = GetEnglish( msg_eng );
	m_message_user = GetTranslation( msg_eng );

#ifdef __LINUX__
    //wxLogError( m_message_diag.c_str() );
    Console::Error( msg_eng );
#endif
}

wxString Exception::BaseException::FormatDiagnosticMessage() const
{
	return m_message_diag + L"\n\n" + m_stacktrace;
}

// ------------------------------------------------------------------------
wxString Exception::ObjectIsNull::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"An attempted reference to the %s has failed; the frame does not exist or it's handle is null.",
		m_message_diag.c_str()
	) + m_stacktrace;
}

wxString Exception::ObjectIsNull::FormatDisplayMessage() const
{
	return wxsFormat(
		L"An attempted reference to the %s has failed; the frame does not exist or it's handle is null.",
		m_message_diag.c_str()
	);
}

// ------------------------------------------------------------------------
wxString Exception::Stream::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"Stream exception: %s\n\tFile/Object: %s",
		m_message_diag.c_str(), StreamName.c_str()
	) + m_stacktrace;
}

wxString Exception::Stream::FormatDisplayMessage() const
{
	return m_message_user + L"\n\n" +
		wxsFormat( _("Name: %s"), StreamName.c_str() );
}

// ------------------------------------------------------------------------
wxString Exception::UnsupportedStateVersion::FormatDiagnosticMessage() const
{
	// Note: no stacktrace needed for this one...
	return wxsFormat( L"Unknown or unsupported savestate version: 0x%x", Version );
}

wxString Exception::UnsupportedStateVersion::FormatDisplayMessage() const
{
	// m_message_user contains a recoverable savestate error which is helpful to the user.
	return wxsFormat(
		m_message_user + L"\n\n" +
		wxsFormat( _("Cannot load savestate.  It is of an unknown or unsupported version."), Version )
	);
}

// ------------------------------------------------------------------------
wxString Exception::StateCrcMismatch::FormatDiagnosticMessage() const
{
	// Note: no stacktrace needed for this one...
	return wxsFormat(
		L"Game/CDVD does not match the savestate CRC.\n"
		L"\tCdvd CRC: 0x%X\n\tGame CRC: 0x%X\n",
		Crc_Savestate, Crc_Cdvd
	);
}

wxString Exception::StateCrcMismatch::FormatDisplayMessage() const
{
	return wxsFormat(
		m_message_user + L"\n\n" +
		wxsFormat( 
			L"Savestate game/crc mismatch. Cdvd CRC: 0x%X Game CRC: 0x%X\n",
			Crc_Savestate, Crc_Cdvd
		)
	);
}

// ------------------------------------------------------------------------
wxString Exception::IndexBoundsFault::FormatDiagnosticMessage() const
{
	return L"Index out of bounds on SafeArray: " + ArrayName +
		wxsFormat( L"(index=%d, size=%d)", BadIndex, ArrayLength );
}

wxString Exception::IndexBoundsFault::FormatDisplayMessage() const
{
	return m_message_user;
}
