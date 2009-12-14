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
#include "ConsoleLogger.h"

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed for windows-specific rich text messages to make scrolling not lame
#endif

void __evt_fastcall pxLogTextCtrl::OnCoreThreadStatusChanged( void* obj, wxCommandEvent& evt )
{
#ifdef __WXMSW__
	if( obj == NULL ) return;
	pxLogTextCtrl* mframe = (pxLogTextCtrl*)obj;

	// See ConcludeIssue for details on WM_VSCROLL

	if( mframe->HasWriteLock() ) return;
	::SendMessage((HWND)mframe->GetHWND(), WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
#endif
}

void __evt_fastcall pxLogTextCtrl::OnCorePluginStatusChanged( void* obj, PluginEventType& evt )
{
#ifdef __WXMSW__
	if( obj == NULL ) return;
	pxLogTextCtrl* mframe = (pxLogTextCtrl*)obj;

	// See ConcludeIssue for details on WM_VSCROLL

	if( mframe->HasWriteLock() ) return;
	::SendMessage((HWND)mframe->GetHWND(), WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
#endif
}

pxLogTextCtrl::pxLogTextCtrl( wxWindow* parent )
	: wxTextCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxHSCROLL | wxTE_READONLY | wxTE_RICH2
	)

	, m_Listener_CoreThreadStatus	( wxGetApp().Source_CoreThreadStatus(), CmdEvt_Listener					( this, OnCoreThreadStatusChanged ) )
	, m_Listener_CorePluginStatus	( wxGetApp().Source_CorePluginStatus(), EventListener<PluginEventType>	( this, OnCorePluginStatusChanged ) )
{
#ifdef __WXMSW__
	m_win32_LinesPerScroll	= 10;
	m_win32_LinesPerPage	= 0;
#endif
	m_IsPaused				= false;
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
#ifdef __WXMSW__
	// Windows has retarded console window update patterns.  This helps smarten them up.
	int ctrly = GetSize().y;
	int fonty;
	GetTextExtent( L"blaH yeah", NULL, &fonty );
	m_win32_LinesPerPage	= (ctrly / fonty) + 1;
	m_win32_LinesPerScroll	= m_win32_LinesPerPage * 0.40;
#endif

	evt.Skip();
}

void pxLogTextCtrl::OnThumbTrack(wxScrollWinEvent& evt)
{
	//Console.Warning( "Thumb Tracking!!!" );
	m_FreezeWrites = true;
	if( !m_IsPaused )
		m_IsPaused = CoreThread.Pause();

	evt.Skip();
}

void pxLogTextCtrl::OnThumbRelease(wxScrollWinEvent& evt)
{
	//Console.Warning( "Thumb Releasing!!!" );
	m_FreezeWrites = false;
	if( m_IsPaused )
	{
		CoreThread.Resume();
		m_IsPaused = false;
	}
	evt.Skip();
}

void pxLogTextCtrl::ConcludeIssue( int lines )
{
	if( HasWriteLock() ) return;
	SetInsertionPointEnd();

#ifdef __WXMSW__

	// EM_LINESCROLL avoids weird errors when the buffer reaches "max" and starts
	// clearing old history:
	::SendMessage((HWND)GetHWND(), EM_LINESCROLL, 0, 0xfffffff);

	// WM_VSCROLL makes the scrolling 'smooth' (such that the last line of the log contents
	// are always displayed as the last line of the log window).  Unfortunately this also
	// makes logging very slow, so we only send the message for status changes, so that the
	// log aligns itself nicely when we pause emulation or when errors occur.

	wxTextPos showpos = XYToPosition( 1, GetNumberOfLines()-m_win32_LinesPerScroll );
	if( showpos > 0 )
		ShowPosition( showpos );

	//::SendMessage((HWND)GetHWND(), WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
#endif

}

