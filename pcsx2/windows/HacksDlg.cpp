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


#include "win32.h"

static _TCHAR *VUCycleHackLevels[] = {
	_T("Speedup for 3D games.\nCurrently off"),
	_T("Slight speedup for 3D geometry, should work with most games."),
	_T("Moderate speedup for 3D geometry, should work with most games with minor problems."),
	_T("Large speedup for 3D geometry, may break many games and make others skip frames."),
	_T("Very large speedup for 3D geometry, will break games in interesting ways."),
};

static void CheckVUCycleHack(HWND hDlg, int &vucyclehack)
{
	if (vucyclehack < 0 || vucyclehack > 4) {
		vucyclehack = 0;
		SendDlgItemMessage(hDlg, IDC_VUCYCLE, TBM_SETPOS, TRUE, vucyclehack);
	}
	SetDlgItemText(hDlg, IDC_VUCYCLEDESC, VUCycleHackLevels[vucyclehack]);
}

BOOL APIENTRY HacksProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:

			CheckRadioButton( hDlg, IDC_EESYNC_DEFAULT, IDC_EESYNC3, IDC_EESYNC_DEFAULT + CHECK_EE_CYCLERATE );

			if(CHECK_IOP_CYCLERATE) CheckDlgButton(hDlg, IDC_IOPSYNC, TRUE);
			if(CHECK_WAITCYCLE_HACK) CheckDlgButton(hDlg, IDC_WAITCYCLES, TRUE);
			if(CHECK_INTC_STAT_HACK) CheckDlgButton(hDlg, IDC_INTCSTATHACK, TRUE);
			if(CHECK_ESCAPE_HACK) CheckDlgButton(hDlg, IDC_ESCHACK, TRUE);

			SendDlgItemMessage(hDlg, IDC_VUCYCLE, TBM_SETRANGE, TRUE, MAKELONG(0, 4));
			CheckVUCycleHack(hDlg, Config.VUCycleHack);
			SendDlgItemMessage(hDlg, IDC_VUCYCLE, TBM_SETPOS, TRUE, Config.VUCycleHack);

		return TRUE;

		case WM_HSCROLL: {
			HWND slider = (HWND)lParam;
			int curpos = HIWORD(wParam);
			switch (LOWORD(wParam)) {
				case TB_THUMBTRACK:
				case TB_THUMBPOSITION:
					break;
				default:
					curpos = SendMessage(slider, TBM_GETPOS, 0, 0);
			}
			CheckVUCycleHack(hDlg, curpos);
			return FALSE;
		}

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
					newhacks |= IsDlgButtonChecked(hDlg, IDC_INTCSTATHACK) << 5;
					newhacks |= IsDlgButtonChecked(hDlg, IDC_ESCHACK) << 10;

					int newvucyclehack = SendDlgItemMessage(hDlg, IDC_VUCYCLE, TBM_GETPOS, 0, 0);
					CheckVUCycleHack(hDlg, newvucyclehack);

					EndDialog(hDlg, TRUE);

					if( newhacks != Config.Hacks || newvucyclehack != Config.VUCycleHack)
					{
						SysRestorableReset();
						Config.Hacks = newhacks;
						Config.VUCycleHack = newvucyclehack;
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
