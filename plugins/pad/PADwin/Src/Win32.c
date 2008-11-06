/*  PADwin
 *  Copyright (C) 2002-2004  PADwin Team
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

#include "PAD.h"
//#define JOY_TEST//not fully working joystic code
HWND GShwnd;
HINSTANCE hInst=NULL;

WNDPROC GSwndProc;

int timer;
#ifdef JOY_TEST
int axis[4][7];//4 joys X (6 axis + 1 POV)
#endif
extern char *libraryName;

#define WM_MOUSEWHEEL                   0x020A
void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "PADwinKeyb Msg", 0);
}

int lbutton = 0;
int rbutton = 0;
int keydown = 0;
LRESULT WINAPI PADwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef JOY_TEST	
	int i; u32 j;
	JOYINFOEX ji={sizeof(JOYINFOEX), JOY_RETURNALL};
#endif
	switch (msg) {
#ifdef JOY_TEST
		case WM_TIMER:

			for (i=0; i<4; i++) {
				if (JOYERR_NOERROR == joyGetPosEx(i, &ji)) {
					for (j=0; j<32; j++) {
						if ((1<<j) & ji.dwButtons) {
							_KeyPressNE(0x80+0x20*i+j);
						} else {
							_KeyReleaseNE(0x80+0x20*i+j);
						}
					}

					for (j=0; j<6; j++){
						if (((u32*)&ji.dwXpos)[j]<0x4000 && axis[i][j]){
							_KeyPressNE(0x100+0x10*i+j*2+0);
						} else {
							_KeyReleaseNE(0x100+0x10*i+j*2+0);
						}
						if (((u32*)&ji.dwXpos)[j]>0xC000 && axis[i][j]){
							_KeyPressNE(0x100+0x10*i+j*2+1);
						} else {
							_KeyReleaseNE(0x100+0x10*i+j*2+1);
						}
					}

					if ((axis[i][6] && (ji.dwPOV>=    0) && (ji.dwPOV< 4500)) ||
						(axis[i][6] && (ji.dwPOV>=31500) && (ji.dwPOV<36000))) {
						_KeyPressNE(0x10c+0x10*i); //forward
					} else {
						_KeyReleaseNE(0x10c+0x10*i);
					}
					if (axis[i][6] && (ji.dwPOV>= 4500) && (ji.dwPOV<13500)) {
						_KeyPressNE((0x10e)+0x10*i); //right
					} else {
						_KeyReleaseNE((0x10e)+0x10*i);
					}
					if (axis[i][6] && (ji.dwPOV>=13500) && (ji.dwPOV<22500)) {
						_KeyPressNE(0x10d+0x10*i); //backward
					} else {
						_KeyReleaseNE(0x10d+0x10*i);
					}
					if (axis[i][6] && (ji.dwPOV>=22500) && (ji.dwPOV<31500)) {
						_KeyPressNE(0x10f+0x10*i); //left
					} else {
						_KeyReleaseNE(0x10f+0x10*i);
					}
				}
			}
			return TRUE;
#endif
		case WM_KEYDOWN:
			if (lParam & 0x40000000)
				return TRUE;
			_KeyPress(wParam);
			keydown = 1;
			return TRUE;

		case WM_KEYUP:
			_KeyRelease(wParam);
			keydown = 0;
			pressure = 5; // reset it
			return TRUE;

		case WM_MOUSEWHEEL:
			if(keydown)
			{
				pressure += ((short)HIWORD(wParam)/120)*5;
				if(pressure >= 100) pressure = 100;
				if(pressure <= 0) pressure = 0;
			}
			return TRUE;

		case WM_LBUTTONDBLCLK:
			lanalog.button = 1;
			return TRUE;

		case WM_LBUTTONDOWN:
			lbutton = 1;
			return TRUE;

		case WM_LBUTTONUP:
			lanalog.x = 0x80;
			lanalog.y = 0x80;
			lbutton = 0;
			return TRUE;

		case WM_RBUTTONDBLCLK:
			ranalog.button = 1;
			return TRUE;

		case WM_RBUTTONDOWN:
			rbutton = 1;
			return TRUE;

		case WM_RBUTTONUP:
			ranalog.x = 0x80;
			ranalog.y = 0x80;
			rbutton = 0;
			return TRUE;

		case WM_MOUSEMOVE:
			if(lbutton)
			{
				lanalog.x = LOWORD(lParam) & 254;
				lanalog.y = HIWORD(lParam) & 254;
			}
			if(rbutton)
			{
				ranalog.x = LOWORD(lParam) & 254;
				ranalog.y = HIWORD(lParam) & 254;
			}
			return TRUE;

		case WM_DESTROY:
		case WM_QUIT:
			KillTimer(hWnd, 0x80);
			// fake escape :)
			event.event = KEYPRESS;
			event.key = VK_ESCAPE;
//			exit(0);
		    return TRUE;

	    default:
			if (!timer){SetTimer(hWnd, 0x80, 50, NULL); timer=1;}
			return GSwndProc(hWnd, msg, wParam, lParam);
	}

	return FALSE;
}

int padopen=0;

s32  _PADopen(void *pDsp) {
#ifdef JOY_TEST
	JOYCAPS jc;
	int i, j;
#endif
	memset(&event, 0, sizeof(event));

	if (padopen == 1) return 0;
	LoadConfig();

	GShwnd = (HWND)*(long*)pDsp;

#ifdef PAD_LOG
	PAD_LOG("SubclassWindow\n");
#endif
	GSwndProc = SubclassWindow(GShwnd, PADwndProc);
#ifdef PAD_LOG
	PAD_LOG("SubclassWindowk\n");
#endif

	timer=0;
#ifdef JOY_TEST
	axis[0][0]=axis[0][1]=axis[1][0]=axis[1][1]=1;//all joys have X & Y
	for (i=JOYSTICKID1; i<=JOYSTICKID2; i++)
		if (JOYERR_NOERROR == joyGetDevCaps(i, &jc, sizeof(jc))){
			for (j=0; j<5; j++)
				if ((1<<j) & jc.wCaps)
					axis[i][j+2]=1;
	}
#endif
	padopen = 1;

	return 0;
}

void _PADclose() {
	SubclassWindow(GShwnd, GSwndProc);
	padopen = 0;
}


char keyNames[512][32] = {
	"NONE", "", "", "", "", "", "", "", /* 0x00 */
	"BACKSPACE", "TAB", "", "", "", "ENTER", "", "",
	"SHIFT", "CONTROL", "ALT", "", "", "", "", "", /* 0x10 */
	"", "", "", "", "", "", "", "",
	"SPACE", "PDUP", "PDDN", "END", "HOME", "LEFT", "UP", "RIGHT", /* 0x20 */
	"DOWN", "", "", "", "", "INSERT", "DELETE", "",
	"", "", "", "", "", "", "", "", /* 0x30 */
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", /* 0x40 */
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", /* 0x50 */
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", /* 0x60 */
	"", "", "*", "+", "", "-", ".", "/",
};

int padn=0;

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hWC;
	TCITEM tcI;
#ifdef JOY_TEST
	JOYINFOEX ji={sizeof(JOYINFOEX), JOY_RETURNALL | JOY_RETURNRAWDATA};
	JOYCAPS   jc;
	int j;
#endif
	char str[256];
	int i,key;
	static int disabled=0;

	switch(uMsg) {
		case WM_INITDIALOG:
			LoadConfig();
            if (conf.log)        CheckDlgButton(hW, IDC_LOG, TRUE);
			for (i='0'; i<='9'; i++) {
				sprintf(keyNames[i], "%c", i);
			}

			for (i='A'; i<='Z'; i++) {
				sprintf(keyNames[i], "%c", i);
			}

			for (i=0x60; i<=0x69; i++) {
				sprintf(keyNames[i], "NumPad %c", '0' + i - 0x60);
			}

			for (i=0x80; i<0x100; i++) {
				sprintf(keyNames[i], "J%d_%d", (i - 0x80) / 32, ((i - 0x80) % 32) + 1);
			}
			for (i=0; i<4; i++) {
				sprintf(keyNames[0x100+0x10*i], "J%d_LEFT", i);
				sprintf(keyNames[0x101+0x10*i], "J%d_RIGHT", i);
				sprintf(keyNames[0x102+0x10*i], "J%d_UP", i);
				sprintf(keyNames[0x103+0x10*i], "J%d_DOWN", i);
				sprintf(keyNames[0x104+0x10*i], "J%d_FWRD", i);
				sprintf(keyNames[0x105+0x10*i], "J%d_BACK", i);
				sprintf(keyNames[0x106+0x10*i], "J%d_RMIN", i);
				sprintf(keyNames[0x107+0x10*i], "J%d_RMAX", i);//ruder
				sprintf(keyNames[0x108+0x10*i], "J%d_UMIN", i);
				sprintf(keyNames[0x109+0x10*i], "J%d_UMAX", i);
				sprintf(keyNames[0x10a+0x10*i], "J%d_VMIN", i);
				sprintf(keyNames[0x10b+0x10*i], "J%d_VMAX", i);
				sprintf(keyNames[0x10c+0x10*i], "J%d_POVFWRD", i);
				sprintf(keyNames[0x10d+0x10*i], "J%d_POVBACK", i);
				sprintf(keyNames[(0x10e)+0x10*i], "J%d_POVRIGHT", i);
				sprintf(keyNames[0x10f+0x10*i], "J%d_POVLEFT", i);
			}

            for (i=0; i<16; i++) {
				if( i != 9 && i != 10) // We are handling them manually
				{
					hWC = GetDlgItem(hW, IDC_EL3 + i*2);
					strcpy(str, keyNames[conf.keys[0][i]]);

					Button_SetText(hWC, str);
				}
			}

			hWC = GetDlgItem(hW, IDC_TABC);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 1";

			TabCtrl_InsertItem(hWC, 0, &tcI);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 2";

			TabCtrl_InsertItem(hWC, 1, &tcI);
#ifdef JOY_TEST
			for (i=0; i<4; i++) {
				if (JOYERR_NOERROR == joyGetDevCaps(i, &jc, sizeof(jc))) {
					printf("joyGetDevCaps[%d]: %s;  %d\n", i, jc.szPname, jc.wNumButtons);
					axis[i][0] = axis[i][1] = 1; //all joys have X & Y
					for (j=0; j<5; j++) {
						if ((1<<j) & jc.wCaps) {
							axis[i][j+2]=1;
						}
					}
				}
			}
#endif
			
			return TRUE;

		case WM_TIMER:
			if (disabled){
				memset(str, 0, 256);
				GetKeyboardState(str);
				for (key=0; key<256; key++)
				{
					if (str[key] & 0x80) break;
					
				}
				if(key == 255) key = 256;
#ifdef JOY_TEST						
				for (i=0; i<4; i++) {
					if (JOYERR_NOERROR == joyGetPosEx(i, &ji)) {
						printf("dwButtons[%d] = %x\n", i, ji.dwButtons);
						printf("dwXpos[%d] = %x\n", i, ji.dwXpos);
						printf("dwYpos[%d] = %x\n", i, ji.dwYpos);
						printf("dwZpos[%d] = %x\n", i, ji.dwZpos);
						printf("dwRpos[%d] = %x\n", i, ji.dwRpos);
						printf("dwUpos[%d] = %x\n", i, ji.dwUpos);
						printf("dwVpos[%d] = %x\n", i, ji.dwVpos);
						printf("dwPOV[%d]  = %x\n", i, ji.dwPOV);
						if (ji.dwButtons){
							for (j=0; j<32; j++) {
								if ((1<<j) & ji.dwButtons)
									break;
							}
							key=0x80+0x20*i+j;
							break;
						}

						for (j=0; j<6; j++){
							if (((u32*)&ji.dwXpos)[j]<0x4000 && axis[i][j]){
								key=0x100+0x10*i+j*2+0;
								break;
							}
							if (((u32*)&ji.dwXpos)[j]>0xC000 && axis[i][j]){
								key=0x100+0x10*i+j*2+1;
								break;
							}
						}
						if (j!=6) break;

						if ((axis[i][6] && (ji.dwPOV>=    0) && (ji.dwPOV< 4500)) ||
							(axis[i][6] && (ji.dwPOV>=31500) && (ji.dwPOV<36000))){
							key=0x10c+0x10*i;	break;	}//forward
						if (axis[i][6] && (ji.dwPOV>= 4500) && (ji.dwPOV<13500)){
							key=(0x10e)+0x10*i;	break;	}//right
						if (axis[i][6] && (ji.dwPOV>=13500) && (ji.dwPOV<22500)){
							key=0x10d+0x10*i;	break;	}//backward
						if (axis[i][6] && (ji.dwPOV>=22500) && (ji.dwPOV<31500)){
							key=0x10f+0x10*i;	break;	}//left
					}
				}
#endif
                if ((key<256) && (key!=1/*mouse left*/) && (key!=2/*mouse right*/)){
					int i;
					KillTimer(hW, 0x80);
					for(i = IDC_L2; i <= IDC_LEFT; i+=2)
					{
						if(i == disabled)
						{
							HWND temp;
							temp = GetDlgItem(hW, disabled-1);
							hWC = GetDlgItem(hW, disabled);
							switch(disabled)
							{
							case IDC_L2:
								Button_SetText(temp, keyNames[conf.keys[padn][0] = key]);
								break;
							case IDC_R2:
								Button_SetText(temp, keyNames[conf.keys[padn][1] = key]);
								break;
							case IDC_L1:
								Button_SetText(temp, keyNames[conf.keys[padn][2] = key]);
								break;
							case IDC_R1:
								Button_SetText(temp, keyNames[conf.keys[padn][3] = key]);
								break;
							case IDC_CIRCLE:
								Button_SetText(temp, keyNames[conf.keys[padn][5] = key]);
								break;
							case IDC_TRI:
								Button_SetText(temp, keyNames[conf.keys[padn][4] = key]);
								break;
							case IDC_SQUARE:
								Button_SetText(temp, keyNames[conf.keys[padn][7] = key]);
								break;
							case IDC2_CROSS:
								Button_SetText(temp, keyNames[conf.keys[padn][6] = key]);
								break;
							case IDC_START:
								Button_SetText(temp, keyNames[conf.keys[padn][11] = key]);
								break;
							case IDC_SELECT:
								Button_SetText(temp, keyNames[conf.keys[padn][8] = key]);
								break;
							case IDC_UP:
								Button_SetText(temp, keyNames[conf.keys[padn][12] = key]);
								break;
							case IDC_DOWN:
								Button_SetText(temp, keyNames[conf.keys[padn][14] = key]);
								break;
							case IDC_LEFT:
								Button_SetText(temp, keyNames[conf.keys[padn][15] = key]);
								break;
							case IDC_RIGHT:
								Button_SetText(temp, keyNames[conf.keys[padn][13] = key]);
								break;
							}
							EnableWindow(hWC, TRUE);
							disabled=0;
							return TRUE;
						}
					}
					return TRUE;
				}
			}
			return TRUE;

		case WM_COMMAND:
			for(i = IDC_L2; i <= IDC_LEFT; i+=2)
			{
				if(LOWORD(wParam) == i)
				{
				  if (disabled)//change selection
					EnableWindow(GetDlgItem(hW, disabled), TRUE);

				  EnableWindow(GetDlgItem(hW, disabled=wParam), FALSE);
				
				  SetTimer(hW, 0x80, 250, NULL);
				
				  return TRUE;
				}
			}

			switch(LOWORD(wParam)) {
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
		case WM_NOTIFY:
			switch (wParam) {
				case IDC_TABC:
					hWC = GetDlgItem(hW, IDC_TABC);
					padn = TabCtrl_GetCurSel(hWC);
					
					for (i=0; i<16; i++) {
						if( i != 9 && i!=10)
						{
							hWC = GetDlgItem(hW, IDC_EL3 + i*2);
							strcpy(str, keyNames[conf.keys[padn][i]]);

							Button_SetText(hWC, str);
						}
					}

					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK PADconfigure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_DIALOG1),
              GetActiveWindow(),  
              (DLGPROC)ConfigureDlgProc);
}

void CALLBACK PADabout() {
	SysMessage("Made By Linuzappz - Redone by Asadr. 2004");
    /*DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT),
              GetActiveWindow(),  
              (DLGPROC)AboutDlgProc);*/
}

s32 CALLBACK PADtest() {
	return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

