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
#include "App.h"

#include <wx/stackwalk.h>


static wxString pxGetStackTrace()
{
    wxString stackTrace;

    class StackDump : public wxStackWalker
    {
	protected:
		wxString m_stackTrace;

    public:
        StackDump() { }

        const wxString& GetStackTrace() const { return m_stackTrace; }

    protected:
        virtual void OnStackFrame(const wxStackFrame& frame)
        {
			wxString name( frame.GetName() );
			if( name.IsEmpty() )
				name = wxsFormat( L"%p ", frame.GetAddress() );

            m_stackTrace += wxString::Format( L"[%02d] %-46s ",
				wx_truncate_cast(int, frame.GetLevel()), name.c_str()
			);

            if ( frame.HasSourceLocation() )
                m_stackTrace += wxsFormat( L"%s:%d", frame.GetFileName().c_str(), frame.GetLine() );

            m_stackTrace += L'\n';
        }
    };

	// [TODO] : Replace this with a textbox dialog setup.
    static const int maxLines = 20;

    StackDump dump;
    dump.Walk(2, maxLines); // don't show OnAssert() call itself
    stackTrace = dump.GetStackTrace();

    const int count = stackTrace.Freq( L'\n' );
    for ( int i = 0; i < count - maxLines; i++ )
        stackTrace = stackTrace.BeforeLast( L'\n' );

    return stackTrace;
}

static __threadlocal bool _reentrant_lock = false;

#ifdef __WXDEBUG__

// This override of wx's implementation provides thread safe assertion message reporting.  If we aren't
// on the main gui thread then the assertion message box needs to be passed off to the main gui thread
// via messages.
void Pcsx2App::OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg )
{
	if( _reentrant_lock )
	{
		// Re-entrant assertions are bad mojo -- trap immediately.
		wxTrap();
	}

	_reentrant_lock = true;

	// Used to allow the user to suppress future assertions during this application's session.
	static bool disableAsserts = false;

	wxString dbgmsg;
	dbgmsg.reserve( 2048 );

	wxString message;
	if( msg == NULL )
		message = cond;
	else
		message.Printf( L"%s (%s)", msg, cond );

	// make life easier for people using VC++ IDE by using this format, which allows double-click
	// response times from the Output window...
	dbgmsg.Printf( L"%s(%d) : assertion failed%s%s: %s\n", file, line,
		(func==NULL) ? wxEmptyString : L" in ", 
		(func==NULL) ? wxEmptyString : func, 
		message.c_str()
	);

	wxString trace( L"Call stack:\n" + pxGetStackTrace() );

	wxMessageOutputDebug().Printf( dbgmsg );
	Console.Error( dbgmsg );
	Console.WriteLn( trace );

	int retval = Msgbox::Assertion( dbgmsg, trace );
	
	switch( retval )
	{
		case wxID_YES:
			wxTrap();
		break;
		
		case wxID_NO: break;
		
		case wxID_CANCEL:		// ignores future assertions.
			disableAsserts = true;
		break;
	}

	_reentrant_lock = false;
}
#endif
