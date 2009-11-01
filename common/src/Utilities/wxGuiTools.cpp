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
