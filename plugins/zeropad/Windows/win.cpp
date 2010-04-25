/*  ZeroPAD - author: zerofrog(@gmail.com)
 *  Copyright (C) 2006-2007
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "win.h"

using namespace std;

extern u16 status[2];

extern string s_strIniPath;
WNDPROC GSwndProc = NULL;
HWND GShwnd = NULL;
HINSTANCE hInst = NULL;

extern keyEvent event;

s32  _PADopen(void *pDsp)
{
	LoadConfig();

	if (GShwnd != NULL && GSwndProc != NULL)
	{
		// revert
		SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(GSwndProc));
	}

	GShwnd = (HWND) * (long*)pDsp;
	GSwndProc = (WNDPROC)GetWindowLongPtr(GShwnd, GWLP_WNDPROC);
	GSwndProc = ((WNDPROC)SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(PADwndProc)));

	return 0;
}

void _PADclose()
{
	if (GShwnd != NULL && GSwndProc != NULL)
	{
		SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(GSwndProc));
		GSwndProc = NULL;
		GShwnd = NULL;
	}
}

// Yes, let's not do anything when pcsx2 asks for us for an update.
// We certainly don't want to update the gamepad information...
void CALLBACK PADupdate(int pad)
{
}

// Most of the values look off in here by either a factor of 10 or 100.
string GetKeyLabel(const int pad, const int index)
{
	const int key = conf.keys[pad][index];
	char buff[16] = "NONE)";
	if (key < 0x100) //IS_KEYBOARD; should be 0x10000
	{
		if (key == 0)
			strcpy(buff, "NONE");
		else
		{
			if (key >= 0x60 && key <= 0x69)
				sprintf(buff, "NumPad %c", '0' + key - 0x60);
			else
				sprintf(buff, "%c", key);
		}
	}
	else if (key >= 0x1000 && key < 0x2000) //IS_JOYBUTTONS; 0x10000 - 0x20000
	{
		sprintf(buff, "J%d_%d", (key & 0xfff) / 0x100, (key & 0xff) + 1);
	}
	else if (key >= 0x2000 && key < 0x3000) // IS_JOYSTICK; 0x20000 - 0x30000
	{
		static const char name[][4] = { "MIN", "MAX" };
		const int axis = (key & 0xff);

		sprintf(buff, "J%d_AXIS%d_%s", (key & 0xfff) / 0x100, axis / 2, name[axis % 2]);
		if (index >= 17 && index <= 20) buff[strlen(buff) -4] = '\0';
	}
	else if (key >= 0x3000 && key < 0x4000) // IS_POV; 0x30000 - 0x50000
	{
		static const char name[][8] = { "FORWARD", "RIGHT", "BACK", "LEFT" };
		const int pov = (key & 0xff);

		sprintf(buff, "J%d_POV%d_%s", (key & 0xfff) / 0x100, pov / 4, name[pov % 4]);
	}

	return buff;
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWC;
	TCITEM tcI;
	int i, key;
	int numkeys;
	//u8* pkeyboard;
	static int disabled = 0;
	static int padn = 0;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			LoadConfig();
			padn = 0;
			if (conf.log) CheckDlgButton(hW, IDC_LOG, TRUE);

			for (i = 0; i < PADKEYS; i++)
			{
				hWC = GetDlgItem(hW, IDC_L2 + i * 2);
				Button_SetText(hWC, GetKeyLabel(padn, i).c_str());
			}

			hWC = GetDlgItem(hW, IDC_TABC);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 1";

			TabCtrl_InsertItem(hWC, 0, &tcI);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 2";

			TabCtrl_InsertItem(hWC, 1, &tcI);
			return TRUE;

		case WM_TIMER:
			if (disabled)
			{
				//pkeyboard = SDL_GetKeyState(&numkeys);
				// Well, this doesn't exactly work either, but it's close...
				numkeys = 256;
				for (int i = 0; i < numkeys; ++i)
				{
					if (GetAsyncKeyState(i))
					{
						key = i;
						break;
					}
				}


				if (key == 0)
				{
					// check joystick
				}
				else
				{
					// Get rid of the expired timer, try to configure the keys, fail horribly,
					// and either crash or don't register a keypress.
					KillTimer(hW, 0x80);
					hWC = GetDlgItem(hW, disabled);
					conf.keys[padn][disabled-IDC_L2] = key;
					Button_SetText(hWC, GetKeyLabel(padn, disabled - IDC_L2).c_str());
					EnableWindow(hWC, TRUE);
					disabled = 0;
					return TRUE;
				}
			}
			return TRUE;

		case WM_COMMAND:
			for (i = IDC_L2; i <= IDC_LEFT; i += 2)
			{
				if (LOWORD(wParam) == i)
				{
					// A button was pressed
					if (disabled)//change selection
						EnableWindow(GetDlgItem(hW, disabled), TRUE);

					EnableWindow(GetDlgItem(hW, disabled = wParam), FALSE);

					SetTimer(hW, 0x80, 2500, NULL);

					return TRUE;
				}
			}

			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					KillTimer(hW, 0x80);
					EndDialog(hW, TRUE);
					return TRUE;

				case IDOK:
					KillTimer(hW, 0x80);
					if (IsDlgButtonChecked(hW, IDC_LOG))
						conf.log = 1;
					else
						conf.log = 0;
					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
			break;

		case WM_NOTIFY:
			switch (wParam)
			{
				case IDC_TABC:
					hWC = GetDlgItem(hW, IDC_TABC);
					padn = TabCtrl_GetCurSel(hWC);

					for (i = 0; i < PADKEYS; i++)
					{
						hWC = GetDlgItem(hW, IDC_EL3 + i);
						Button_SetText(hWC, GetKeyLabel(padn, i).c_str());
					}


					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK PADconfigure()
{
	INT_PTR ret;

	ret = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_DIALOG1),GetActiveWindow(),(DLGPROC)ConfigureDlgProc,1);
}

void CALLBACK PADabout()
{
	SysMessage("Author: zerofrog\nThanks to SSSPSXPad, TwinPAD, and PADwin plugins");
}

s32 CALLBACK PADtest()
{
	return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason,
                      LPVOID lpReserved)
{
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}
