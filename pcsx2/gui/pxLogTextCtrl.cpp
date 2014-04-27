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
#include "ConsoleLogger.h"

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed for windows-specific rich text messages to make scrolling not lame
#endif

void pxLogTextCtrl::DispatchEvent( const CoreThreadStatus& status )
{
	// See ConcludeIssue for details on WM_VSCROLL
	if( HasWriteLock() ) return;
	ConcludeIssue();
}

void pxLogTextCtrl::DispatchEvent( const PluginEventType& evt )
{
	if( HasWriteLock() ) return;
	ConcludeIssue();
}

pxLogTextCtrl::pxLogTextCtrl( wxWindow* parent )
	: wxTextCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2
	)
{
	m_FreezeWrites			= false;

	Connect( wxEVT_SCROLLWIN_THUMBTRACK,	wxScrollWinEventHandler(pxLogTextCtrl::OnThumbTrack) );
	Connect( wxEVT_SCROLLWIN_THUMBRELEASE,	wxScrollWinEventHandler(pxLogTextCtrl::OnThumbRelease) );

	Connect( wxEVT_SIZE,					wxSizeEventHandler(pxLogTextCtrl::OnResize) );
}

#ifdef __WXMSW__
void pxLogTextCtrl::WriteText(const wxString& text)
{
	// Don't need the update message -- saves some overhead.
	DoWriteText( text, SetValue_SelectionOnly );
}
#endif

void pxLogTextCtrl::OnResize( wxSizeEvent& evt )
{
	evt.Skip();
}

void pxLogTextCtrl::OnThumbTrack(wxScrollWinEvent& evt)
{
	//Console.Warning( "Thumb Tracking!!!" );
	m_FreezeWrites = true;
	if( !m_IsPaused )
		m_IsPaused = new ScopedCoreThreadPause();

	evt.Skip();
}

void pxLogTextCtrl::OnThumbRelease(wxScrollWinEvent& evt)
{
	//Console.Warning( "Thumb Releasing!!!" );
	m_FreezeWrites = false;
	if( m_IsPaused )
	{
		m_IsPaused->AllowResume();
		m_IsPaused.Delete();
	}
	evt.Skip();
}

pxLogTextCtrl::~pxLogTextCtrl() throw()
{
}

void pxLogTextCtrl::ConcludeIssue()
{
	if( HasWriteLock() ) return;

	ScrollLines(1);
	ShowPosition( GetLastPosition() );

#ifdef __WXMSW__
	// This is needed to keep the scrolling "nice" when the textbox doesn't
	// have the focus.
	::SendMessage((HWND)GetHWND(), WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
#endif
}

