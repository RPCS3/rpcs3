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

static wxString GetTranslation( const char* msg )
{
	return msg ? wxGetTranslation( fromUTF8(msg) ) : wxEmptyString;
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
//  BaseException  (implementations)
// --------------------------------------------------------------------------------------

BaseException::~BaseException() throw() {}

BaseException& BaseException::SetBothMsgs( const char* msg_diag )
{
	m_message_user = GetTranslation( msg_diag );
	return SetDiagMsg( fromUTF8(msg_diag) );
}

BaseException& BaseException::SetDiagMsg( const wxString& msg_diag )
{
	m_message_diag = msg_diag;
	return *this;
}

BaseException& BaseException::SetUserMsg( const wxString& msg_user )
{
	m_message_user = msg_user;
	return *this;
}

wxString BaseException::FormatDiagnosticMessage() const
{
	return m_message_diag;
}

wxString BaseException::FormatDisplayMessage() const
{
	return m_message_user.IsEmpty() ? m_message_diag : m_message_user;
}

// --------------------------------------------------------------------------------------
//  Exception::RuntimeError   (implementations)
// --------------------------------------------------------------------------------------
Exception::RuntimeError::RuntimeError( const std::runtime_error& ex, const wxString& prefix )
{
	IsSilent = false;

	const wxString msg( wxsFormat( L"STL Runtime Error%s: %s",
		(prefix.IsEmpty() ? prefix.c_str() : wxsFormat(L" (%s)", prefix.c_str()).c_str()),
		fromUTF8( ex.what() ).c_str()
	) );

	SetDiagMsg( msg );
}

Exception::RuntimeError::RuntimeError( const std::exception& ex, const wxString& prefix )
{
	IsSilent = false;

	const wxString msg( wxsFormat( L"STL Exception%s: %s",
		(prefix.IsEmpty() ? prefix.c_str() : wxsFormat(L" (%s)", prefix.c_str()).c_str()),
		fromUTF8( ex.what() ).c_str()
	) );

	SetDiagMsg( msg );
}

// --------------------------------------------------------------------------------------
//  Exception::OutOfMemory   (implementations)
// --------------------------------------------------------------------------------------
Exception::OutOfMemory::OutOfMemory( const wxString& allocdesc )
{
	AllocDescription	= allocdesc;
	m_message_diag		= L"Out of memory exception, while allocating the %s.";
	m_message_user		= _("Memory allocation failure!  Your system has insufficient memory or resources to meet PCSX2's lofty needs.");
}

wxString Exception::OutOfMemory::FormatDiagnosticMessage() const
{
	return wxsFormat(m_message_diag, AllocDescription.c_str());
}

wxString Exception::OutOfMemory::FormatDisplayMessage() const
{
	if (m_message_user.IsEmpty()) return FormatDisplayMessage();
	return m_message_user + wxsFormat( L"\n\nInternal allocation descriptor: %s", AllocDescription.c_str());
}


// ------------------------------------------------------------------------
wxString Exception::CancelEvent::FormatDiagnosticMessage() const
{
	return L"Action canceled: " + m_message_diag;
}

wxString Exception::CancelEvent::FormatDisplayMessage() const
{
	return L"Action canceled: " + m_message_diag;
}

wxString Exception::Stream::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"%s\n\tFile/Object: %s",
		m_message_diag.c_str(), StreamName.c_str()
	);
}

wxString Exception::Stream::FormatDisplayMessage() const
{
	wxString retval( m_message_user );
	if (!StreamName.IsEmpty())
		retval += L"\n\n" + wxsFormat( _("Path: %s"), StreamName.c_str() );

	return retval;
}

