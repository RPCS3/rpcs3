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


#include "PrecompiledHeader.h"
#include "win32.h"

BOOL APIENTRY HacksProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:

			CheckRadioButton( hDlg, IDC_EESYNC_DEFAULT, IDC_EESYNC3, IDC_EESYNC_DEFAULT + CHECK_EE_CYCLERATE );

			if(CHECK_IOP_CYCLERATE) CheckDlgButton(hDlg, IDC_IOPSYNC, TRUE);
			if(CHECK_WAITCYCLE_HACK) CheckDlgButton(hDlg, IDC_WAITCYCLES, TRUE);
			if(CHECK_ESCAPE_HACK) CheckDlgButton(hDlg, IDC_ESCHACK, TRUE);

		return TRUE;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					int newhacks = 0;
					for( int i=1; i<4; i++ )
					{
						if( IsDlgButtonChecked(hDlg, IDC_EESYNC_DEFAULT+i) )
						{
							newhacks = i;
							break;
						}
					}

					newhacks |= IsDlgButtonChecked(hDlg, IDC_IOPSYNC) << 3;
					newhacks |= IsDlgButtonChecked(hDlg, IDC_WAITCYCLES) << 4;
					newhacks |= IsDlgButtonChecked(hDlg, IDC_ESCHACK) << 10;

					EndDialog(hDlg, TRUE);

					if( newhacks != Config.Hacks )
					{
						SysRestorableReset();
						Config.Hacks = newhacks;
						SaveConfig();
					}
				}
				return FALSE;

				case IDCANCEL:
					EndDialog(hDlg, FALSE);
				return FALSE;
			}
		return TRUE;
    }

    return FALSE;
}
