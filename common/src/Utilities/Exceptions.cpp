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

#include <wx/app.h>

wxString GetEnglish( const char* msg )
{
	return wxString::FromAscii(msg);
}

wxString GetTranslation( const char* msg )
{
	return wxGetTranslation( wxString::FromAscii(msg).c_str() );
}

// ------------------------------------------------------------------------
// Force DevAssert to *not* inline for devel builds (allows using breakpoints to trap assertions,
// and force it to inline for release builds (optimizes it out completely since IsDevBuild is a
// const false).
//
#ifdef PCSX2_DEVBUILD
#	define DEVASSERT_INLINE __noinline
#else
#	define DEVASSERT_INLINE __forceinline
#endif

// Using a threadlocal assertion guard.  Separate threads can assert at the same time.
// That's ok.  What we don't want is the *same* thread recurse-asserting.
static __threadlocal int s_assert_guard = 0;

DEVASSERT_INLINE void pxOnAssert( const wxChar* file, int line, const char* func, const wxChar* cond, const wxChar* msg)
{
#ifdef PCSX2_DEVBUILD
	RecursionGuard guard( s_assert_guard );
	if( guard.IsReentrant() ) return;

	if( wxTheApp == NULL )
	{
		// Note: Format uses MSVC's syntax for output window hotlinking.
		wxsFormat( L"%s(%d): Assertion failed in %s: %s\n",
			file, line, fromUTF8(func).c_str(), msg );

		wxLogError( msg );
	}
	else
	{
	#ifdef __WXDEBUG__
		wxTheApp->OnAssertFailure( file, line, fromUTF8(func), cond, msg );
	#elif wxUSE_GUI
		// FIXME: this should create a popup dialog for devel builds.
		wxLogError( msg );
	#else
		wxLogError( msg );
	#endif
	}
#endif
}

__forceinline void pxOnAssert( const wxChar* file, int line, const char* func, const wxChar* cond, const char* msg)
{
	pxOnAssert( file, line, func, cond, fromUTF8(msg) );
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
    Console.Error( msg_eng );
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
    Console.Error( msg_eng );
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
