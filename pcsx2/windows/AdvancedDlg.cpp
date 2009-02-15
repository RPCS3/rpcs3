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

#include "Win32.h"

#include "Common.h"
#include "ix86/ix86.h"

// the EE and VU roundmodes are passed in so that we can "retain" the config values during
// default button action.  The checkbox statuses are checked against the Config settings when
// the user hits OK, and the game only resets if they differ.

static void InitRoundClampModes( HWND hDlg, u32 new_eeopt, u32 new_vuopt )
{
	CheckRadioButton(hDlg, IDC_EE_ROUNDMODE0, IDC_EE_ROUNDMODE3, IDC_EE_ROUNDMODE0 + ((Config.sseMXCSR & 0x6000) >> 13));
	CheckRadioButton(hDlg, IDC_VU_ROUNDMODE0, IDC_VU_ROUNDMODE3, IDC_VU_ROUNDMODE0 + ((Config.sseVUMXCSR & 0x6000) >> 13));
	CheckRadioButton(hDlg, IDC_EE_CLAMPMODE0, IDC_EE_CLAMPMODE2, IDC_EE_CLAMPMODE0 + ((new_eeopt & 0x2) ? 2 : (new_eeopt & 0x1)));

	if		(new_vuopt & 0x4)	CheckRadioButton(hDlg, IDC_VU_CLAMPMODE0, IDC_VU_CLAMPMODE3, IDC_VU_CLAMPMODE0 + 3);
	else if (new_vuopt & 0x2)	CheckRadioButton(hDlg, IDC_VU_CLAMPMODE0, IDC_VU_CLAMPMODE3, IDC_VU_CLAMPMODE0 + 2);
	else if (new_vuopt & 0x1)	CheckRadioButton(hDlg, IDC_VU_CLAMPMODE0, IDC_VU_CLAMPMODE3, IDC_VU_CLAMPMODE0 + 1);
	else						CheckRadioButton(hDlg, IDC_VU_CLAMPMODE0, IDC_VU_CLAMPMODE3, IDC_VU_CLAMPMODE0 + 0);

	if (Config.sseMXCSR & 0x8000)	CheckDlgButton(hDlg, IDC_EE_CHECK1, TRUE);
	if (Config.sseVUMXCSR & 0x8000) CheckDlgButton(hDlg, IDC_VU_CHECK1, TRUE);
}

BOOL APIENTRY AdvancedOptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:

			InitRoundClampModes( hDlg, Config.eeOptions, Config.vuOptions );

			if( !cpucaps.hasStreamingSIMD2Extensions ) {	
				// SSE1 cpus do not support Denormals Are Zero flag.
				Config.sseMXCSR &= ~0x0040;
				Config.sseVUMXCSR &= ~0x0040;
				EnableWindow( GetDlgItem( hDlg, IDC_EE_CHECK2 ), FALSE );
				EnableWindow( GetDlgItem( hDlg, IDC_VU_CHECK2 ), FALSE );
				CheckDlgButton( hDlg, IDC_EE_CHECK2, FALSE );
				CheckDlgButton( hDlg, IDC_VU_CHECK2, FALSE );
			}
			else {
				if (Config.sseMXCSR & 0x0040)	CheckDlgButton(hDlg, IDC_EE_CHECK2, TRUE);
				if (Config.sseVUMXCSR & 0x0040) CheckDlgButton(hDlg, IDC_VU_CHECK2, TRUE);
			}

			return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
			{
				case IDOK:
				{
					u32 new_eeopt = 0;
					u32 new_vuopt = 0;

					Config.sseMXCSR		&= 0x1fbf;
					Config.sseVUMXCSR	&= 0x1fbf;

					Config.sseMXCSR |= IsDlgButtonChecked(hDlg, IDC_EE_ROUNDMODE0) ? 0x0000 : 0; // Round Nearest
					Config.sseMXCSR |= IsDlgButtonChecked(hDlg, IDC_EE_ROUNDMODE1) ? 0x2000 : 0; // Round Negative
					Config.sseMXCSR |= IsDlgButtonChecked(hDlg, IDC_EE_ROUNDMODE2) ? 0x4000 : 0; // Round Postive
					Config.sseMXCSR |= IsDlgButtonChecked(hDlg, IDC_EE_ROUNDMODE3) ? 0x6000 : 0; // Round Zero / Chop

					Config.sseVUMXCSR |= IsDlgButtonChecked(hDlg, IDC_VU_ROUNDMODE0) ? 0x0000 : 0; // Round Nearest
					Config.sseVUMXCSR |= IsDlgButtonChecked(hDlg, IDC_VU_ROUNDMODE1) ? 0x2000 : 0; // Round Negative
					Config.sseVUMXCSR |= IsDlgButtonChecked(hDlg, IDC_VU_ROUNDMODE2) ? 0x4000 : 0; // Round Postive
					Config.sseVUMXCSR |= IsDlgButtonChecked(hDlg, IDC_VU_ROUNDMODE3) ? 0x6000 : 0; // Round Zero / Chop

					new_eeopt |= IsDlgButtonChecked(hDlg, IDC_EE_CLAMPMODE0) ? 0x0 : 0;
					new_eeopt |= IsDlgButtonChecked(hDlg, IDC_EE_CLAMPMODE1) ? 0x1 : 0;
					new_eeopt |= IsDlgButtonChecked(hDlg, IDC_EE_CLAMPMODE2) ? 0x3 : 0;

					new_vuopt |= IsDlgButtonChecked(hDlg, IDC_VU_CLAMPMODE0) ? 0x0 : 0;
					new_vuopt |= IsDlgButtonChecked(hDlg, IDC_VU_CLAMPMODE1) ? 0x1 : 0;
					new_vuopt |= IsDlgButtonChecked(hDlg, IDC_VU_CLAMPMODE2) ? 0x3 : 0;
					new_vuopt |= IsDlgButtonChecked(hDlg, IDC_VU_CLAMPMODE3) ? 0x7 : 0;

					Config.sseMXCSR		|= IsDlgButtonChecked(hDlg, IDC_EE_CHECK1) ? 0x8000 : 0; // FtZ
					Config.sseVUMXCSR	|= IsDlgButtonChecked(hDlg, IDC_VU_CHECK1) ? 0x8000 : 0; // FtZ

					Config.sseMXCSR		|= IsDlgButtonChecked(hDlg, IDC_EE_CHECK2) ? 0x0040 : 0; // DaZ
					Config.sseVUMXCSR	|= IsDlgButtonChecked(hDlg, IDC_VU_CHECK2) ? 0x0040 : 0; // DaZ

					EndDialog(hDlg, TRUE);

					// Roundmode options do not require CPU resets to take effect.
					SetCPUState(Config.sseMXCSR, Config.sseVUMXCSR);

					if( new_eeopt != Config.eeOptions || new_vuopt != Config.vuOptions )
					{
						// these do, however...
						Config.eeOptions = new_eeopt;
						Config.vuOptions = new_vuopt;
						SysRestorableReset();
					}

					SaveConfig();
					break;
				}

				case IDCANCEL:
					
					EndDialog(hDlg, FALSE);
					break;

				case IDDEFAULT:

					Config.sseMXCSR		= DEFAULT_sseMXCSR;
					Config.sseVUMXCSR	= DEFAULT_sseVUMXCSR;

					// SSE1 cpus do not support Denormals Are Zero flag.
					if( !cpucaps.hasStreamingSIMD2Extensions ) {
						Config.sseMXCSR &= ~0x0040;
						Config.sseVUMXCSR &= ~0x0040;
					}

					InitRoundClampModes( hDlg, DEFAULT_eeOptions, DEFAULT_vuOptions );

					CheckDlgButton(hDlg, IDC_EE_CHECK1, (Config.sseMXCSR & 0x8000) ? TRUE : FALSE);
					CheckDlgButton(hDlg, IDC_VU_CHECK1, (Config.sseVUMXCSR & 0x8000) ? TRUE : FALSE);

					CheckDlgButton(hDlg, IDC_EE_CHECK2, (Config.sseMXCSR & 0x0040) ? TRUE : FALSE);
					CheckDlgButton(hDlg, IDC_VU_CHECK2, (Config.sseVUMXCSR & 0x0040) ? TRUE : FALSE);
					break;
					
				case IDC_EE_ROUNDMODE0:
				case IDC_EE_ROUNDMODE1:
				case IDC_EE_ROUNDMODE2:
				case IDC_EE_ROUNDMODE3:

					CheckRadioButton(hDlg, IDC_EE_ROUNDMODE0, IDC_EE_ROUNDMODE3, IDC_EE_ROUNDMODE0 + ( LOWORD(wParam) % IDC_EE_ROUNDMODE0 )  );
					break;

				case IDC_VU_ROUNDMODE0:
				case IDC_VU_ROUNDMODE1:
				case IDC_VU_ROUNDMODE2:
				case IDC_VU_ROUNDMODE3:

					CheckRadioButton(hDlg, IDC_VU_ROUNDMODE0, IDC_VU_ROUNDMODE3, IDC_VU_ROUNDMODE0 + ( LOWORD(wParam) % IDC_VU_ROUNDMODE0 )  );
					break;

				case IDC_EE_CLAMPMODE0:
				case IDC_EE_CLAMPMODE1:
				case IDC_EE_CLAMPMODE2:

					CheckRadioButton(hDlg, IDC_EE_CLAMPMODE0, IDC_EE_CLAMPMODE2, IDC_EE_CLAMPMODE0 + ( LOWORD(wParam) % IDC_EE_CLAMPMODE0 )  );
					break;

				case IDC_VU_CLAMPMODE0:
				case IDC_VU_CLAMPMODE1:
				case IDC_VU_CLAMPMODE2:
				case IDC_VU_CLAMPMODE3:

					CheckRadioButton(hDlg, IDC_VU_CLAMPMODE0, IDC_VU_CLAMPMODE3, IDC_VU_CLAMPMODE0 + ( LOWORD(wParam) % IDC_VU_CLAMPMODE0 )  );
					break;
			}

			return TRUE;
    }

    return FALSE;
}
