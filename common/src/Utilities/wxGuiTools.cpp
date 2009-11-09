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
#include "wxGuiTools.h"

#include <wx/app.h>
#include <wx/window.h>

// Returns FALSE if the window position is considered invalid, which means that it's title
// bar is most likely not easily grabble.  Such a window should be moved to a valid or
// default position.
bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos )
{
	// The height of the window is only revlevant to the height of a title bar, which is
	// all we need visible for the user to be able to drag the window into view.  But
	// there's no way to get that info from wx, so we'll just have to guesstimate...

	wxSize sizeMatters( window.GetSize().GetWidth(), 32 );		// if some gui has 32 pixels of undraggable title bar, the user deserves to suffer.
	return wxGetDisplayArea().Contains( wxRect( windowPos, sizeMatters ) );
}

// Retrieves the area of the screen, which can be used to enforce a valid zone for
// window positioning. (top/left coords are almost always (0,0) and bottom/right
// is the resolution of the desktop).
wxRect wxGetDisplayArea()
{
	return wxRect( wxPoint(), wxGetDisplaySize() );
}

// --------------------------------------------------------------------------------------
//  pxTextWrapper / pxTextWrapperBase Implementations
// --------------------------------------------------------------------------------------

pxTextWrapperBase& pxTextWrapperBase::Wrap( const wxWindow& win, const wxString& text, int widthMax )
{
    const wxChar *lastSpace = NULL;
    wxString line;
    line.Alloc( widthMax+12 );

    const wxChar *lineStart = text.c_str();
    for ( const wxChar *p = lineStart; ; p++ )
    {
        if ( IsStartOfNewLine() )
        {
            OnNewLine();

            lastSpace = NULL;
            line.clear();
            lineStart = p;
        }

        if ( *p == L'\n' || *p == L'\0' )
        {
            DoOutputLine(line);

            if ( *p == L'\0' )
                break;
        }
        else // not EOL
        {
            if ( *p == L' ' )
                lastSpace = p;

            line += *p;

            if ( widthMax >= 0 && lastSpace )
            {
                int width;
                win.GetTextExtent(line, &width, NULL);

                if ( width > widthMax )
                {
                    // remove the last word from this line
                    line.erase(lastSpace - lineStart, p + 1 - lineStart);
                    DoOutputLine(line);

                    // go back to the last word of this line which we didn't
                    // output yet
                    p = lastSpace;
                }
            }
            //else: no wrapping at all or impossible to wrap
        }
    }

    return *this;
}

void pxTextWrapperBase::DoOutputLine(const wxString& line)
{
	OnOutputLine(line);
	m_linecount++;
	m_eol = true;
}

// this function is a destructive inspector: when it returns true it also
// resets the flag to false so calling it again wouldn't return true any
// more
bool pxTextWrapperBase::IsStartOfNewLine()
{
	if ( !m_eol )
		return false;

	m_eol = false;
	return true;
}

pxTextWrapper& pxTextWrapper::Wrap( const wxWindow& win, const wxString& text, int widthMax )
{
	_parent::Wrap( win, text, widthMax );
	return *this;
}

void pxTextWrapper::OnOutputLine(const wxString& line)
{
	m_text += line;
}

void pxTextWrapper::OnNewLine()
{
	m_text += L'\n';
}

// --------------------------------------------------------------------------------------
//  ScopedBusyCursor Implementations
// --------------------------------------------------------------------------------------

std::stack<BusyCursorType>	ScopedBusyCursor::m_cursorStack;
BusyCursorType				ScopedBusyCursor::m_defBusyType;

ScopedBusyCursor::ScopedBusyCursor( BusyCursorType busytype )
{
	pxAssert( wxTheApp != NULL );

	BusyCursorType curtype = Cursor_NotBusy;
	if( !m_cursorStack.empty() )
		curtype = m_cursorStack.top();

	if( curtype < busytype )
		SetManualBusyCursor( curtype=busytype );

	m_cursorStack.push( curtype );
}

ScopedBusyCursor::~ScopedBusyCursor() throw()
{
	if( !pxAssert( wxTheApp != NULL ) ) return;

	if( !pxAssert( !m_cursorStack.empty() ) )
	{
		SetManualBusyCursor( m_defBusyType );
		return;
	}

	BusyCursorType curtype = m_cursorStack.top();
	m_cursorStack.pop();

	if( m_cursorStack.empty() )
		SetManualBusyCursor( m_defBusyType );
	else if( m_cursorStack.top() != curtype )
		SetManualBusyCursor( m_cursorStack.top() );
}

void ScopedBusyCursor::SetDefault( BusyCursorType busytype )
{
	if( busytype == m_defBusyType ) return;
	m_defBusyType = busytype;
	
	if( m_cursorStack.empty() )
		SetManualBusyCursor( busytype );
}

void ScopedBusyCursor::SetManualBusyCursor( BusyCursorType busytype )
{
	switch( busytype )
	{
		case Cursor_NotBusy:	wxSetCursor( wxNullCursor ); break;
		case Cursor_KindaBusy:	wxSetCursor( StockCursors.GetArrowWait() ); break;
		case Cursor_ReallyBusy:	wxSetCursor( *wxHOURGLASS_CURSOR ); break;
	}
}

const wxCursor& MoreStockCursors::GetArrowWait()
{
	if( !m_arrowWait )
		m_arrowWait = new wxCursor( wxCURSOR_ARROWWAIT );
	return *m_arrowWait;
}

MoreStockCursors StockCursors;

// --------------------------------------------------------------------------------------
//  pxSetToolTip
// --------------------------------------------------------------------------------------
// This is the preferred way to assign tooltips to wxWindow-based objects, as it performs the
// necessary text wrapping on platforms that need it.  On windows tooltips are wrapped at 600
// pixels, or 66% of the screen width, whichever is smaller.  GTK and MAC perform internal
// wrapping, so this function does a regular assignment there.
void pxSetToolTip( wxWindow* wind, const wxString& src )
{
	if( !pxAssert( wind != NULL ) ) return;

	// Windows needs manual tooltip word wrapping (sigh).
	// GTK and Mac are a wee bit more clever (except in GTK tooltips don't show at all
	// half the time because of some other bug .. sigh)
#ifdef __WXMSW__
	int whee = wxGetDisplaySize().GetWidth() * 0.75;
	wind->SetToolTip( pxTextWrapper().Wrap( *wind, src, std::min( whee, 600 ) ).GetResult() );
#else
	wind->SetToolTip( src );
#endif
}

void pxSetToolTip( wxWindow& wind, const wxString& src )
{
	pxSetToolTip( &wind, src );
}
