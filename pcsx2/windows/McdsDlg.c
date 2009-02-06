/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>

#include "Common.h"
#include "PsxCommon.h"
#include "plugins.h"
#include "resource.h"
#include "Win32.h"

#include "Paths.h"


HWND mcdDlg;


void Open_Mcd_Proc(HWND hW, int mcd) {
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[1024];
	char *str;

	memset(szFileName,  0, sizeof(szFileName));
	memset(szFileTitle, 0, sizeof(szFileTitle));
	memset(szFilter,    0, sizeof(szFilter));


	strcpy(szFilter, _("Ps2 Memory Card (*.ps2)"));
	str = szFilter + strlen(szFilter) + 1; 
	strcpy(str, "*.ps2");

	str+= strlen(str) + 1;
	strcpy(str, _("All Files"));
	str+= strlen(str) + 1;
	strcpy(str, "*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hW;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= MEMCARDS_DIR;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "MC2";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		Edit_SetText(GetDlgItem(hW,mcd == 1 ? IDC_MCD1 : IDC_MCD2), szFileName);
	}
}

BOOL CALLBACK ConfigureMcdsDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			mcdDlg = hW;

			SetWindowText(hW, _("Memcard Manager"));

			Button_SetText(GetDlgItem(hW, IDOK),        _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL),    _("Cancel"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL1), _("Select Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL2), _("Select Mcd"));

			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD1), _("Memory Card 1"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD2), _("Memory Card 2"));

			if (!strlen(Config.Mcd1)) strcpy(Config.Mcd1, "memcards\\Mcd001.ps2");
			if (!strlen(Config.Mcd2)) strcpy(Config.Mcd2, "memcards\\Mcd002.ps2");
			Edit_SetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1);
			Edit_SetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_MCDSEL1: 
					Open_Mcd_Proc(hW, 1); 
					return TRUE;
				case IDC_MCDSEL2: 
					Open_Mcd_Proc(hW, 2); 
					return TRUE;
       			case IDCANCEL:
					EndDialog(hW,FALSE);

					return TRUE;
       			case IDOK:
					Edit_GetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1, 256);
					Edit_GetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2, 256);

					SaveConfig();

					EndDialog(hW,TRUE);

					return TRUE;
			}
		case WM_DESTROY:
			return TRUE;
	}
	return FALSE;
}

