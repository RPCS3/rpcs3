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

#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "resource.h"
#include "../zeropad.h"

#include <pthread.h>
#include <string>

using namespace std;

HINSTANCE hInst = NULL;
static pthread_spinlock_t s_mutexStatus;
static u32 s_keyPress[2], s_keyRelease[2];
extern u16 status[2];

extern string s_strIniPath;
LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC GSwndProc = NULL;
HWND GShwnd = NULL;

extern keyEvent event;


void SaveConfig()
{
	char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
	int i, j;

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if (!szTemp) return;
	strcpy(szTemp, "\\inis\\zeropad.ini");

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			sprintf(szProf, "%d_%d", j, i);
			sprintf(szValue, "%d", conf.keys[j][i]);
			WritePrivateProfileString("Interface", szProf, szValue, szIniFile);
		}
	}

	sprintf(szValue, "%u", conf.log);
	WritePrivateProfileString("Interface", "Logging", szValue, szIniFile);
}

void LoadConfig()
{
	FILE *fp;
	char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
	int i, j;

	memset(&conf, 0, sizeof(conf));
#ifdef _WIN32
	conf.keys[0][0] = 'W';                      // L2
	conf.keys[0][1] = 'O';                      // R2
	conf.keys[0][2] = 'A';                      // L1
	conf.keys[0][3] = ';';                      // R1
	conf.keys[0][4] = 'I';                      // TRIANGLE
	conf.keys[0][5] = 'L';                      // CIRCLE
	conf.keys[0][6] = 'K';                      // CROSS
	conf.keys[0][7] = 'J';                      // SQUARE
	conf.keys[0][8] = 'V';      // SELECT
	conf.keys[0][11] = 'N';     // START
	conf.keys[0][12] = 'E';     // UP
	conf.keys[0][13] = 'F';     // RIGHT
	conf.keys[0][14] = 'D';     // DOWN
	conf.keys[0][15] = 'S';     // LEFT
#endif
	conf.log = 0;

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if (!szTemp) return ;
	strcpy(szTemp, "\\inis\\zeropad.ini");
	fp = fopen("inis\\zeropad.ini", "rt");//check if usbnull.ini really exists
	if (!fp)
	{
		CreateDirectory("inis", NULL);
		SaveConfig();//save and return
		return ;
	}
	fclose(fp);

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < PADKEYS; i++)
		{
			sprintf(szProf, "%d_%d", j, i);
			GetPrivateProfileString("Interface", szProf, NULL, szValue, 20, szIniFile);
			conf.keys[j][i] = strtoul(szValue, NULL, 10);
		}
	}

	GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
	conf.log = strtoul(szValue, NULL, 10);
}

void SysMessage(char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);
	MessageBox(0, tmp, "PADwinKeyb Msg", 0);
}

s32  _PADopen(void *pDsp)
{
	memset(&event, 0, sizeof(event));
	LoadConfig();
	pthread_spin_init(&s_mutexStatus, PTHREAD_PROCESS_PRIVATE);
	s_keyPress[0] = s_keyPress[1] = 0;
	s_keyRelease[0] = s_keyRelease[1] = 0;

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
	pthread_spin_destroy(&s_mutexStatus);
}

void _PADupdate(int pad)
{
	pthread_spin_lock(&s_mutexStatus);
	status[pad] |= s_keyRelease[pad];
	status[pad] &= ~s_keyPress[pad];
	s_keyRelease[pad] = 0;
	s_keyPress[pad] = 0;
	pthread_spin_unlock(&s_mutexStatus);
}

void CALLBACK PADupdate(int pad)
{
}

LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i, pad, keyPress[2] = {0}, keyRelease[2] = {0};
	static bool lbutton = false, rbutton = false;

	switch (msg)
	{
		case WM_KEYDOWN:
			if (lParam & 0x40000000)
				return TRUE;

			for (pad = 0; pad < 2; ++pad)
			{
				for (i = 0; i < PADKEYS; i++)
				{
					if (wParam == conf.keys[pad][i])
					{
						keyPress[pad] |= (1 << i);
						keyRelease[pad] &= ~(1 << i);
						break;
					}
				}
			}

			event.evt = KEYPRESS;
			event.key = wParam;
			break;

		case WM_KEYUP:
			for (pad = 0; pad < 2; ++pad)
			{
				for (i = 0; i < PADKEYS; i++)
				{
					if (wParam == conf.keys[pad][i])
					{
						keyPress[pad] &= ~(1 << i);
						keyRelease[pad] |= (1 << i);
						break;
					}
				}
			}


			event.evt = KEYRELEASE;
			event.key = wParam;
			break;

		case WM_LBUTTONDOWN:
			lbutton = true;
			break;

		case WM_LBUTTONUP:
			g_lanalog[0].x = 0x80;
			g_lanalog[0].y = 0x80;
			g_lanalog[1].x = 0x80;
			g_lanalog[1].y = 0x80;
			lbutton = false;
			break;

		case WM_RBUTTONDOWN:
			rbutton = true;
			break;

		case WM_RBUTTONUP:
			g_ranalog[0].x = 0x80;
			g_ranalog[0].y = 0x80;
			g_ranalog[1].x = 0x80;
			g_ranalog[1].y = 0x80;
			rbutton = false;
			break;

		case WM_MOUSEMOVE:
			if (lbutton)
			{
				g_lanalog[0].x = LOWORD(lParam) & 254;
				g_lanalog[0].y = HIWORD(lParam) & 254;
				g_lanalog[1].x = LOWORD(lParam) & 254;
				g_lanalog[1].y = HIWORD(lParam) & 254;
			}
			if (rbutton)
			{
				g_ranalog[0].x = LOWORD(lParam) & 254;
				g_ranalog[0].y = HIWORD(lParam) & 254;
				g_ranalog[1].x = LOWORD(lParam) & 254;
				g_ranalog[1].y = HIWORD(lParam) & 254;
			}
			break;

		case WM_DESTROY:
		case WM_QUIT:
			event.evt = KEYPRESS;
			event.key = VK_ESCAPE;
			return GSwndProc(hWnd, msg, wParam, lParam);

		default:
			return GSwndProc(hWnd, msg, wParam, lParam);
	}

	pthread_spin_lock(&s_mutexStatus);
	for (pad = 0; pad < 2; ++pad)
	{
		s_keyPress[pad] |= keyPress[pad];
		s_keyPress[pad] &= ~keyRelease[pad];
		s_keyRelease[pad] |= keyRelease[pad];
		s_keyRelease[pad] &= ~keyPress[pad];
	}
	pthread_spin_unlock(&s_mutexStatus);

	return TRUE;
}

string GetKeyLabel(const int pad, const int index)
{
	const int key = conf.keys[pad][index];
	char buff[16] = "NONE)";
	if (key < 0x100)
	{
		if (key == 0)
			strcpy(buff, "NONE");
		else
		{
			if (key >= 0x60 && key <= 0x69)
			{
				sprintf(buff, "NumPad %c", '0' + key - 0x60);
			}
			else sprintf(buff, "%c", key);
		}
	}
	else if (key >= 0x1000 && key < 0x2000)
	{
		sprintf(buff, "J%d_%d", (key & 0xfff) / 0x100, (key & 0xff) + 1);
	}
	else if (key >= 0x2000 && key < 0x3000)
	{
		static const char name[][4] = { "MIN", "MAX" };
		const int axis = (key & 0xff);
		sprintf(buff, "J%d_AXIS%d_%s", (key & 0xfff) / 0x100, axis / 2, name[axis % 2]);
		if (index >= 17 && index <= 20)
			buff[strlen(buff) -4] = '\0';
	}
	else if (key >= 0x3000 && key < 0x4000)
	{
		static const char name[][7] = { "FOWARD", "RIGHT", "BACK", "LEFT" };
		const int pov = (key & 0xff);
		sprintf(buff, "J%d_POV%d_%s", (key & 0xfff) / 0x100, pov / 4, name[pov % 4]);
	}

	return buff;
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWC;
	TCITEM tcI;
	int i, key, numkeys;
	u8* pkeyboard;
	static int disabled = 0;
	static int padn = 0;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			LoadConfig();
			padn = 0;
			if (conf.log)        CheckDlgButton(hW, IDC_LOG, TRUE);

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
				key = 0;
				//pkeyboard = SDL_GetKeyState(&numkeys);
				for (int i = 0; i < numkeys; ++i)
				{
					if (pkeyboard[i])
					{
						key = i;
						break;
					}
				}

				if (key == 0)
				{
					// check joystick
				}

				if (key != 0)
				{
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
					if (disabled)//change selection
						EnableWindow(GetDlgItem(hW, disabled), TRUE);

					EnableWindow(GetDlgItem(hW, disabled = wParam), FALSE);

					SetTimer(hW, 0x80, 250, NULL);

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
					else conf.log = 0;
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
	DialogBox(hInst,
	          MAKEINTRESOURCE(IDD_DIALOG1),
	          GetActiveWindow(),
	          (DLGPROC)ConfigureDlgProc);
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
