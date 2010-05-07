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

#include <wx/app.h>
#include "Threading.h"
#include "TlsVariable.inl"

#if defined(__UNIX__)
#include <signal.h>
#endif

wxString GetEnglish( const char* msg )
{
	return fromUTF8(msg);
}

wxString GetTranslation( const char* msg )
{
	return wxGetTranslation( fromUTF8(msg) );
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
static DeclareTls(int) s_assert_guard( 0 );

pxDoAssertFnType* pxDoAssert = pxAssertImpl_LogIt;

// make life easier for people using VC++ IDE by using this format, which allows double-click
// response times from the Output window...
wxString DiagnosticOrigin::ToString( const wxChar* msg ) const
{
	wxString message;
	message.reserve( 2048 );

	message.Printf( L"%s(%d) : assertion failed:\n", srcfile, line );

	if( function != NULL )
		message	+= L"    Function:  " + fromUTF8(function) + L"\n";

	message		+= L"    Thread:    " + Threading::pxGetCurrentThreadName() + L"\n";

	if( condition != NULL )
		message	+= L"    Condition: " + wxString(condition) + L"\n";

	if( msg != NULL )
		message += L"    Message:   " + wxString(msg) + L"\n";

	return message;
}


bool pxAssertImpl_LogIt( const DiagnosticOrigin& origin, const wxChar *msg )
{
	wxLogError( origin.ToString( msg ) );
	return false;
}

// Because wxTrap isn't available on Linux builds of wxWidgets (non-Debug, typically)
void pxTrap()
{
#if defined(__WXMSW__) && !defined(__WXMICROWIN__)
    __debugbreak();
#elif defined(__WXMAC__) && !defined(__DARWIN__)
    #if __powerc
        Debugger();
    #else
        SysBreak();
    #endif
#elif defined(_MSL_USING_MW_C_HEADERS) && _MSL_USING_MW_C_HEADERS
    Debugger();
#elif defined(__UNIX__)
    raise(SIGTRAP);
#else
    // TODO
#endif // Win/Unix
}

DEVASSERT_INLINE void pxOnAssert( const DiagnosticOrigin& origin, const wxChar* msg )
{
	RecursionGuard guard( s_assert_guard );
	if( guard.IsReentrant() ) { return wxTrap(); }

	// wxWidgets doesn't come with debug builds on some Linux distros, and other distros make
	// it difficult to use the debug build (compilation failures).  To handle these I've had to
	// bypass the internal wxWidgets assertion handler entirely, since it may not exist even if
	// PCSX2 itself is compiled in debug mode (assertions enabled).

	bool trapit;

	if( pxDoAssert == NULL )
	{
		// Note: Format uses MSVC's syntax for output window hotlinking.
		trapit = pxAssertImpl_LogIt( origin, msg );
	}
	else
	{
		trapit = pxDoAssert( origin, msg );
	}

	if( trapit ) { pxTrap(); }
}

__forceinline void pxOnAssert( const DiagnosticOrigin& origin, const char* msg)
{
	pxOnAssert( origin, fromUTF8(msg) );
}


// --------------------------------------------------------------------------------------
//  Exception Namespace Implementations  (Format message handlers for general exceptions)
// --------------------------------------------------------------------------------------

BaseException::~BaseException() throw() {}

void BaseException::InitBaseEx( const wxString& msg_eng, const wxString& msg_xlt )
{
	m_message_diag = msg_eng;
	m_message_user = msg_xlt.IsEmpty() ? msg_eng : msg_xlt;

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
void BaseException::InitBaseEx( const char* msg_eng )
{
	m_message_diag = GetEnglish( msg_eng );
	m_message_user = GetTranslation( msg_eng );

#ifdef __LINUX__
    //wxLogError( m_message_diag.c_str() );
    Console.Error( msg_eng );
#endif
}

wxString BaseException::FormatDiagnosticMessage() const
{
	return m_message_diag + L"\n\n" + m_stacktrace;
}

// ------------------------------------------------------------------------
Exception::RuntimeError::RuntimeError( const std::runtime_error& ex, const wxString& prefix )
{
	const wxString msg( wxsFormat( L"STL Runtime Error%s: %s",
		(prefix.IsEmpty() ? prefix.c_str() : wxsFormat(L" (%s)", prefix.c_str()).c_str()),
		fromUTF8( ex.what() ).c_str()
	) );

	BaseException::InitBaseEx( msg, msg );
}

// ------------------------------------------------------------------------
Exception::RuntimeError::RuntimeError( const std::exception& ex, const wxString& prefix )
{
	const wxString msg( wxsFormat( L"STL Exception%s: %s",
		(prefix.IsEmpty() ? prefix.c_str() : wxsFormat(L" (%s)", prefix.c_str()).c_str()),
		fromUTF8( ex.what() ).c_str()
	) );

	BaseException::InitBaseEx( msg, msg );
}

// ------------------------------------------------------------------------
wxString Exception::CancelEvent::FormatDiagnosticMessage() const
{
	// FIXME: This should probably just log a single line from the stacktrace.. ?
	return L"Action canceled: " + m_message_diag;
}

wxString Exception::CancelEvent::FormatDisplayMessage() const
{
	return L"Action canceled: " + m_message_diag;
}

// ------------------------------------------------------------------------
wxString Exception::ObjectIsNull::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"reference to '%s' failed : handle is null.",
		m_message_diag.c_str()
	) + m_stacktrace;
}

wxString Exception::ObjectIsNull::FormatDisplayMessage() const
{
	return wxsFormat(
		L"reference to '%s' failed : handle is null.",
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

