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
#include "App.h"
#include "Utilities/TlsVariable.inl"

#include <wx/stackwalk.h>

static wxString pxGetStackTrace( const FnChar_t* calledFrom )
{
    wxString stackTrace;

    class StackDump : public wxStackWalker
    {
	protected:
		wxString		m_stackTrace;
		wxString		m_srcFuncName;
		bool			m_ignoreDone;
		int				m_skipped;

    public:
        StackDump( const FnChar_t* src_function_name )
        {
			if( src_function_name != NULL )
				m_srcFuncName = fromUTF8(src_function_name);

			m_ignoreDone	= false;
			m_skipped		= 0;
        }

        const wxString& GetStackTrace() const { return m_stackTrace; }

    protected:
        virtual void OnStackFrame(const wxStackFrame& frame)
        {
			wxString name( frame.GetName() );
			if( name.IsEmpty() )
			{
				name = wxsFormat( L"%p ", frame.GetAddress() );
			}
			/*else if( m_srcFuncName.IsEmpty() || m_srcFuncName == name )
			{
				// FIXME: This logic isn't reliable yet.
				// It's possible for our debug information to not match the function names returned by
				// __pxFUNCTION__ (might happen in linux a lot, and could happen in win32 due to
				// inlining on Dev aserts).  The better approach is a system the queues up all the
				// stacktrace info in individual wxStrings, and does a two-pass check -- first pass
				// for the function name and, if not found, a second pass that just skips the first
				// few stack entries.

				// It's important we only walk the stack once because Linux (argh, always linux!) has
				// a really god aweful slow stack walker.

				// I'm not doing it right now because I've worked on this mess enough for one week. --air

				m_ignoreDone = true;
			}

			if( !m_ignoreDone )
			{
				m_skipped++;
				return;
			}*/

			//wxString briefName;
			wxString essenName;

			if( frame.HasSourceLocation() )
			{
				wxFileName wxfn(frame.GetFileName());
				//briefName.Printf( L"(%s:%d)", wxfn.GetFullName().c_str(), frame.GetLine() );

				wxfn.SetVolume( wxEmptyString );
				int count = wxfn.GetDirCount();
				for( int i=0; i<2; ++i )
					wxfn.RemoveDir(0);

				essenName.Printf( L"%s:%d", wxfn.GetFullPath().c_str(), frame.GetLine() );
			}

            m_stackTrace += wxString::Format( L"[%02d] %-44s %s\n",
				frame.GetLevel()-m_skipped,
				name.c_str(),
				essenName.c_str()
			);
        }
    };

    StackDump dump( calledFrom );
    dump.Walk( 3 );
    return dump.GetStackTrace();
}

#ifdef __WXDEBUG__

// This override of wx's implementation provides thread safe assertion message reporting.
// If we aren't on the main gui thread then the assertion message box needs to be passed
// off to the main gui thread via messages.
void Pcsx2App::OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg )
{
	// Re-entrant assertions are bad mojo -- trap immediately.
	static DeclareTls(int) _reentrant_lock( 0 );
	RecursionGuard guard( _reentrant_lock );
	if( guard.IsReentrant() ) wxTrap();

	wxCharBuffer bleh( wxString(func).ToUTF8() );
	if( AppDoAssert( DiagnosticOrigin( file, line, bleh, cond ), msg ) )
	{
		wxTrap();
	}
}

#endif

bool AppDoAssert( const DiagnosticOrigin& origin, const wxChar *msg )
{
	// Used to allow the user to suppress future assertions during this application's session.
	static bool disableAsserts = false;
	if( disableAsserts ) return false;

	wxString trace( pxGetStackTrace(origin.function) );
	wxString dbgmsg( origin.ToString( msg ) );

	wxMessageOutputDebug().Printf( L"%s", dbgmsg.c_str() );

	Console.Error( L"%s", dbgmsg.c_str() );
	Console.WriteLn( L"%s", trace.c_str() );

	wxString windowmsg( L"Assertion failed: " );
	if( msg != NULL )
		windowmsg += msg;
	else if( origin.condition != NULL )
		windowmsg += origin.condition;

	int retval = Msgbox::Assertion( windowmsg, dbgmsg + L"\nStacktrace:\n" + trace );

	if( retval == wxID_YES ) return true;
	if( retval == wxID_IGNORE ) disableAsserts = true;

	return false;
}
