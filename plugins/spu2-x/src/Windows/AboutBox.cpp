/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
 
#include "Dialogs.h"
#include <CommCtrl.h>

#include "svnrev.h"
#include "Hyperlinks.h"

static LRESULT WINAPI AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			ConvertStaticToHyperlink( hDlg, IDC_LINK_GOOGLECODE );
			ConvertStaticToHyperlink( hDlg, IDC_LINK_WEBSITE );
			
			wchar_t outstr[256];
			if( IsDevBuild )
				swprintf_s( outstr, L"Build r%d -- Compiled on " _T(__DATE__), SVN_REV );
			else
				swprintf_s( outstr, L"Release v%d.%d -- Compiled on "  _T(__DATE__),
					VersionInfo::Release, VersionInfo::Revision ); 

			SetWindowText( GetDlgItem(hDlg, IDC_LABEL_VERSION_INFO), outstr );
			ShowWindow( hDlg, true );
		}
		return TRUE;

		case WM_COMMAND:
			switch( LOWORD(wParam) )
			{
				case IDOK:
					EndDialog(hDlg, TRUE );
				return TRUE;

				case IDC_LINK_WEBSITE:
					ShellExecute(hDlg, L"open", L"http://www.pcsx2.net/",
						NULL, NULL, SW_SHOWNORMAL);
				return TRUE;

				case IDC_LINK_GOOGLECODE:
					ShellExecute(hDlg, L"open", L"http://code.google.com/p/pcsx2",
						NULL, NULL, SW_SHOWNORMAL);
				return TRUE;
			}
		break;
		
		default:
			return FALSE;
	}
	return TRUE;
}

void AboutBox()
{
	DialogBox( hInstance, MAKEINTRESOURCE(IDD_ABOUT), GetActiveWindow(), (DLGPROC)AboutProc );
}
