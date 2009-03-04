/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "Win32.h"

#include "AboutDlg.h"
#include "Common.h"

#include "Hyperlinks.h"

#define IDC_STATIC	(-1)

static HWND hW;
static HBITMAP hSilverBMP;

LRESULT WINAPI AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			hSilverBMP = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_PS2SILVER));

			SetWindowText(hDlg, _("About PCSX2"));

			Button_SetText(GetDlgItem(hDlg, IDOK), _("OK"));
			Static_SetText(GetDlgItem(hDlg, IDC_PCSX_ABOUT_AUTHORS), _(LabelAuthors));
			Static_SetText(GetDlgItem(hDlg, IDC_PCSX_ABOUT_GREETS), _(LabelGreets));

			ConvertStaticToHyperlink( hDlg, IDC_LINK_GOOGLECODE );
			ConvertStaticToHyperlink( hDlg, IDC_LINK_WEBSITE );

		return TRUE;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					EndDialog( hDlg, TRUE );
				return TRUE;

				case IDC_LINK_WEBSITE:
					ShellExecute(hDlg, "open", "http://www.pcsx2.net/",
						NULL, NULL, SW_SHOWNORMAL);
					return TRUE;

				case IDC_LINK_GOOGLECODE:
					ShellExecute(hDlg, "open", "http://code.google.com/p/pcsx2",
						NULL, NULL, SW_SHOWNORMAL);
					return TRUE;
			}
		return FALSE;

		case WM_CLOSE:
			EndDialog( hDlg, TRUE );
		return TRUE;
	}
	return FALSE;
}
