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
#include "MainFrame.h"
#include "MSWstuff.h"
#include <wx/listbook.h>
#include <wx/listctrl.h>

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed for OutputDebugString
#	include <commctrl.h>
#endif

void MSW_SetWindowAfter( WXWidget hwnd, WXWidget hwndAfter )
{
#ifdef __WXMSW__
	SetWindowPos( (HWND)hwnd, (HWND)hwndAfter, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE );
#endif
}

// Writes text to the Visual Studio Output window (Microsoft Windows only).
// On all other platforms this pipes to StdErr instead.
void MSW_OutputDebugString( const wxString& text )
{
#if defined(__WXMSW__) && !defined(__WXMICROWIN__)
	// don't prepend debug/trace here: it goes to the
	// debug window anyhow
	OutputDebugString( text + L"\r\n" );
#else
	// send them to stderr
	wxFprintf(stderr, L"%s\n", text.c_str());
	fflush(stderr);
#endif
}

void MSW_ListView_SetIconSpacing( wxListbook& listbook, int width )
{
#ifdef __WXMSW__
	// Depending on Windows version and user appearance settings, the default icon spacing can be
	// way over generous.  This little bit of Win32-specific code ensures proper icon spacing, scaled
	// to the size of the frame's ideal width.

	ListView_SetIconSpacing( (HWND)listbook.GetListView()->GetHWND(),
		(width / listbook.GetPageCount()) - 6, g_Conf->Listbook_ImageSize+32		// y component appears to be ignored
	);
#endif
}

#ifdef __WXMSW__
WXLRESULT GSPanel::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
	switch ( message )
	{
	case SC_SCREENSAVE:
	case SC_MONITORPOWER:
		if( m_HasFocus && g_Conf->GSWindow.DisableScreenSaver)
		{
			DevCon.WriteLn("Omg Screensaver adverted!");
			return 0;
		}
		break;
	}
	return _parent::MSWWindowProc(message, wParam, lParam);
}
#endif
