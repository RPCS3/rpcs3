#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "GS.h"
#include "Win32.h"
#include "Rec.h"

HINSTANCE hInst=NULL;

void CALLBACK GSkeyEvent(keyEvent *ev) {
	switch (ev->event) {
		case KEYPRESS:
			switch (ev->key) {
				case VK_PRIOR:
					if (conf.fps) fpspos++; break;
				case VK_NEXT:
					if (conf.fps) fpspos--; break;
				case VK_END:
					if (conf.fps) fpspress = 1; break;
				case VK_DELETE:
					conf.fps = 1 - conf.fps;
					break;
			}
			break;
	}
}

#include "Win32/resource.h"

BOOL CALLBACK LoggingDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			if (conf.log)        CheckDlgButton(hW, IDC_LOG, TRUE);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, FALSE);
					return TRUE;
				case IDOK:
					if (IsDlgButtonChecked(hW, IDC_LOG))
						 conf.log = 1;
					else conf.log = 0;

					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

int CacheSizes[] = { 64, 128, 256, 0 };

DXmode wmodes[] = {
	{ 320, 240, 0 },
	{ 512, 384, 0 },
	{ 640, 480, 0 },
	{ 800, 600, 0 },
	{ 0, 0, 0 },
};

void OnInitDialog(HWND hW) {
	HWND hWC;
	char str[256];
	int i, j;
	int nmode;

	LoadConfig();
	if (recExist() == 0) {
		EnableWindow (GetDlgItem(hW, IDC_RECORD), TRUE);
		EnableWindow (GetDlgItem(hW, IDC_CODEC) , TRUE);
	} else {
		conf.record=0;
		CheckDlgButton(hW,IDC_RECORD,BST_UNCHECKED);
		EnableWindow (GetDlgItem(hW, IDC_RECORD), FALSE);
		EnableWindow (GetDlgItem(hW, IDC_CODEC),  FALSE);
	}
	for (i=0; ; i++) {
		if (codeclist[i] == NULL) break;
		ComboBox_AddString(GetDlgItem(hW, IDC_CODEC), codeclist[i]);
	}

	ComboBox_SetCurSel(GetDlgItem(hW, IDC_CODEC), conf.codec);
	nmode = DXgetModes();

	hWC = GetDlgItem(hW, IDC_FRES);
	for (i=0; i<nmode; i++) {
		sprintf(str, "%dx%d", modes[i].width, modes[i].height);
		ComboBox_AddString(hWC, str);
	}
	for (i=0; i<nmode; i++) {
		if (memcmp(&conf.fmode, &modes[i], sizeof(DXmode)) == 0) {
			ComboBox_SetCurSel(hWC, i); break;
		}
	}
	if (i == nmode) ComboBox_SetCurSel(hWC, 0);

	hWC = GetDlgItem(hW, IDC_WRES);
	for (i=0; ; i++) {
		if (wmodes[i].width == 0) break;
		sprintf(str, "%dx%d", wmodes[i].width, wmodes[i].height);
		ComboBox_AddString(hWC, str);
	}
	nmode = i;
	for (i=0; i<nmode; i++) {
		if (conf.wmode.width  == wmodes[i].width &&
			conf.wmode.height == wmodes[i].height) {
			ComboBox_SetCurSel(hWC, i); break;
		}
	}
	if (i == nmode) ComboBox_SetCurSel(hWC, 0);

	hWC = GetDlgItem(hW, IDC_CACHESIZE);
	for (i=0; ; i++) {
		if (CacheSizes[i] == 0) { j = i; break; }
		sprintf(str, "%d", CacheSizes[i]);
		ComboBox_AddString(hWC, str);
	}
	for (i=0; ; i++) {
		if (CacheSizes[i] == 0) break;
		if (conf.cachesize == CacheSizes[i]) {
			ComboBox_SetCurSel(hWC, i); break;
		}
	}
	if (i == j) ComboBox_SetCurSel(hWC, 0);

	hWC = GetDlgItem(hW, IDC_FILTERS);
	for (i=0; ; i++) {
		if (filterslist[i] == 0) break;
		ComboBox_AddString(hWC, filterslist[i]);
	}
	ComboBox_SetCurSel(hWC, conf.filter);

	if (conf.fullscreen) CheckDlgButton(hW, IDC_FULLSCREEN, TRUE);
	if (conf.fps)        CheckDlgButton(hW, IDC_FPSCOUNT, TRUE);
	if (conf.frameskip)  CheckDlgButton(hW, IDC_FRAMESKIP, TRUE);
	if (conf.record)     CheckDlgButton(hW, IDC_RECORD, TRUE);
	if (conf.cache)      CheckDlgButton(hW, IDC_CACHE, TRUE);
}

void OnOK(HWND hW) {
	HWND hWC;
	int i;

	hWC = GetDlgItem(hW, IDC_FRES);
	i = ComboBox_GetCurSel(hWC);
	memcpy(&conf.fmode, &modes[i], sizeof(DXmode));

	hWC = GetDlgItem(hW, IDC_WRES);
	i = ComboBox_GetCurSel(hWC);
	memcpy(&conf.wmode, &wmodes[i], sizeof(DXmode));

	if (IsDlgButtonChecked(hW, IDC_FULLSCREEN))
		 conf.fullscreen = 1;
	else conf.fullscreen = 0;

	if (IsDlgButtonChecked(hW, IDC_FPSCOUNT))
		 conf.fps = 1;
	else conf.fps = 0;

	if (IsDlgButtonChecked(hW, IDC_FRAMESKIP))
		 conf.frameskip = 1;
	else conf.frameskip = 0;

	if (IsDlgButtonChecked(hW, IDC_RECORD))
		 conf.record = 1;
	else conf.record = 0;

	if (IsDlgButtonChecked(hW, IDC_CACHE))
		 conf.cache = 1;
	else conf.cache = 0;

	conf.codec  = ComboBox_GetCurSel(GetDlgItem(hW, IDC_CODEC));
	conf.filter = ComboBox_GetCurSel(GetDlgItem(hW, IDC_FILTERS));

	hWC = GetDlgItem(hW, IDC_CACHESIZE);
	i = ComboBox_GetCurSel(hWC);
	conf.cachesize = CacheSizes[i];

	SaveConfig();
	EndDialog(hW, FALSE);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			OnInitDialog(hW);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					OnOK(hW);
					return TRUE;

				case IDC_LOGGING:
				    DialogBox(hInst,
              			      MAKEINTRESOURCE(IDD_LOGGING),
			                  hW,  
              			      (DLGPROC)LoggingDlgProc);
					return TRUE;
			}
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

void CALLBACK GSconfigure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_CONFIG),
              GetActiveWindow(),  
              (DLGPROC)ConfigureDlgProc);
}

s32 CALLBACK GStest() {
	return 0;
}

void CALLBACK GSabout() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT),
              GetActiveWindow(),  
              (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

static char *err = "Error Loading Symbol";
static int errval;

void *SysLoadLibrary(char *lib) {
	return LoadLibrary(lib);
}

void *SysLoadSym(void *lib, char *sym) {
	void *tmp = GetProcAddress((HINSTANCE)lib, sym);
	if (tmp == NULL) errval = 1;
	else errval = 0;
	return tmp;
}

char *SysLibError() {
	if (errval) { errval = 0; return err; }
	return NULL;
}

void SysCloseLibrary(void *lib) {
	FreeLibrary((HINSTANCE)lib);
}
