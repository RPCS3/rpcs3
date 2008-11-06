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

#define WINVER 0x0500

#if _WIN32_WINNT < 0x0501
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include <ntsecapi.h>

#include <assert.h>

#include "Common.h"
#include "PsxCommon.h"
#include "win32.h"
#include "resource.h"
#include "debugger.h"
#include "rdebugger.h"
#include "AboutDlg.h"
#include "McdsDlg.h"

#include "VU.h"
#include "iCore.h"
#include "iVUzerorec.h"

#include "cheats/cheats.h"

#include "../Paths.h"

#define COMPILEDATE         __DATE__

static int efile;
char filename[256];
extern int g_SaveGSStream;

static int AccBreak = 0;
int needReset = 1;
unsigned int langsMax;
typedef struct {
	char lang[256];
} _langs;
_langs *langs = NULL;

int UseGui = 1;
int nDisableSC = 0; // screensaver
int firstRun=1;
int RunExe = 0;
void OpenConsole() {
	COORD csize;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
	SMALL_RECT srect;

	if (gApp.hConsole) return;
	AllocConsole();
	SetConsoleTitle(_("Ps2 Output"));
	csize.X = 100;
	csize.Y = 1024;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), csize);

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo);
	srect = csbiInfo.srWindow;
	srect.Right = srect.Left + 99;
	srect.Bottom = srect.Top + 64;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &srect);
	gApp.hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void CloseConsole() {
	if (gApp.hConsole == NULL) return;
	FreeConsole(); gApp.hConsole = NULL;
}
void strcatz(char *dst, char *src) {
	int len = strlen(dst) + 1;
	strcpy(dst + len, src);
}

//2002-09-20 (Florin)
BOOL APIENTRY CmdlineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);//forward def
//-------------------

extern int g_ZeroGSOptions;
void RunExecute(int run) {
	SetThreadPriority(GetCurrentThread(), Config.ThPriority);
	SetPriorityClass(GetCurrentProcess(), Config.ThPriority == THREAD_PRIORITY_HIGHEST ? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
    nDisableSC = 1;

	if (needReset == 1) {
		SysReset();
	}

    if( UseGui )
        AccBreak = 1;

	DestroyWindow(gApp.hWnd);
	gApp.hWnd = NULL;

	if (OpenPlugins(g_TestRun.ptitle) == -1) {
		CreateMainWindow(SW_SHOWNORMAL);
		return;
	}

	if (needReset == 1) {
		if(RunExe == 0)cpuExecuteBios();
		//if (efile == 2)
		if(!efile)efile=GetPS2ElfName(filename);
		//if (efile)
		loadElfFile(filename);
		
		RunExe = 0;
		efile=0;
		needReset = 0;
	}

	// this needs to be called for every new game! (note: sometimes launching games through bios will give a crc of 0)
	if( GSsetGameCRC != NULL )
		GSsetGameCRC(ElfCRC, g_ZeroGSOptions);

	if (run) Cpu->Execute();
}

int Slots[5] = { -1, -1, -1, -1, -1 };

void ResetMenuSlots() {
	int i;

	for (i=0; i<5; i++) {
		if (Slots[i] == -1)
			EnableMenuItem(GetSubMenu(gApp.hMenu, 0), ID_FILE_STATES_LOAD_SLOT1+i, MF_GRAYED);
		else 
			EnableMenuItem(GetSubMenu(gApp.hMenu, 0), ID_FILE_STATES_LOAD_SLOT1+i, MF_ENABLED);
	}
}

void UpdateMenuSlots() {
	char str[256];
	int i;

	for (i=0; i<5; i++) {
		sprintf (str, "sstates\\%8.8X.%3.3d", ElfCRC, i);
		Slots[i] = CheckState(str);
	}
}

void States_Load(int num) {
	char Text[256];
	int ret;

	efile = 0;
	RunExecute(0);

	sprintf (Text, "sstates\\%8.8X.%3.3d", ElfCRC, num);
	ret = LoadState(Text);
	if (ret == 0)
		 sprintf (Text, _("*PCSX2*: Loaded State %d"), num+1);
	else sprintf (Text, _("*PCSX2*: Error Loading State %d"), num+1);
	StatusSet(Text);

	Cpu->Execute();
}

void States_Save(int num) {
	char Text[256];
	int ret;


	sprintf (Text, "sstates\\%8.8X.%3.3d", ElfCRC, num); 
	ret = SaveState(Text);
	if (ret == 0)
		 sprintf(Text, _("*PCSX2*: Saving State %d"), num+1);
	else sprintf(Text, _("*PCSX2*: Error Saving State %d"), num+1);
	StatusSet(Text);

	RunExecute(1);
}

void OnStates_LoadOther() {
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[256];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	strcpy(szFilter, _("PCSX2 State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "EXE";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		char Text[256];
		int ret;

		efile = 2;
		RunExecute(0);

		ret = LoadState(szFileName);

		if (ret == 0)
			 sprintf(Text, _("*PCSX2*: Saving State %s"), szFileName);
		else sprintf(Text, _("*PCSX2*: Error Saving State %s"), szFileName);
		StatusSet(Text);

		Cpu->Execute();
	}
} 

void OnStates_Save1() { States_Save(0); } 
void OnStates_Save2() { States_Save(1); } 
void OnStates_Save3() { States_Save(2); } 
void OnStates_Save4() { States_Save(3); } 
void OnStates_Save5() { States_Save(4); } 

char* g_pRunGSState = NULL;

void OnStates_SaveOther() {
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[256];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	strcpy(szFilter, _("PCSX2 State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "EXE";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		char Text[256];
		int ret;

		ret = SaveState(szFileName);
		if (ret == 0)
			 sprintf(Text, _("*PCSX2*: Loaded State %s"), szFileName);
		else sprintf(Text, _("*PCSX2*: Error Loading State %s"), szFileName);
		StatusSet(Text);

		RunExecute(1);
	}
}

TESTRUNARGS g_TestRun;

static int ParseCommandLine(char* pcmd)
{
	const char* pdelim = " \t\r\n";
	char* token = strtok(pcmd, pdelim);

	g_TestRun.efile = 0;

	while(token != NULL) {

		if( _stricmp(token, "-help") == 0) {
            const char* phelpmsg = 
                "pcsx2 [options] [file]\n\n"
                "-cfg [file] {configuration file}\n"
                "-efile [efile] {0 - reset, 1 - runcd (default), 2 - loadelf}\n"
                "-help {display this help file}\n"
                "-nogui {Don't use gui when launching}\n"
                "-loadgs [file} {Loads a gsstate}\n"
                "\n"
#ifdef PCSX2_DEVBUILD
                "Testing Options: \n"
                "\t-frame [frame] {game will run up to this frame before exiting}\n"
				"\t-image [name] {path and base name of image (do not include the .ext)}\n"
                "\t-jpg {save images to jpg format}\n"
				"\t-log [name] {log path to save log file in}\n"
				"\t-logopt [hex] {log options in hex (see debug.h) }\n"
				"\t-numimages [num] {after hitting frame, this many images will be captures every 20 frames}\n"
                "\t-test {Triggers testing mode (only for dev builds)}\n"
                "\n"
#endif

                "Load Plugins:\n"
                "\t-cdvd [dllpath] {specify the dll load path of the CDVD plugin}\n"
                "\t-gs [dllpath] {specify the dll load path of the GS plugin}\n"
                "-pad [tsxcal] {specify to hold down on the triangle, square, circle, x, start, select buttons}\n"
                "\t-spu [dllpath] {specify the dll load path of the SPU2 plugin}\n"
                "\n";

            printf("%s", phelpmsg);
			MessageBox(NULL,phelpmsg,"Help", MB_OK);
			return -1;
		}
        else if( _stricmp(token, "-nogui") == 0 ) {
			UseGui = 0;
		}
#ifdef PCSX2_DEVBUILD
        else if( _stricmp(token, "-image") == 0 ) {
			token = strtok(NULL, pdelim);
			g_TestRun.pimagename = token;
		}
		else if( _stricmp(token, "-log") == 0 ) {
			token = strtok(NULL, pdelim);
			g_TestRun.plogname = token;
		}
		else if( _stricmp(token, "-logopt") == 0 ) {
			token = strtok(NULL, pdelim);
			if( token != NULL ) {
				if( token[0] == '0' && token[1] == 'x' ) token += 2;
				sscanf(token, "%x", &varLog);
			}
		}
        else if( _stricmp(token, "-frame") == 0 ) {
			token = strtok(NULL, pdelim);
			if( token != NULL ) {
				g_TestRun.frame = atoi(token);
			}
		}
		else if( _stricmp(token, "-numimages") == 0 ) {
			token = strtok(NULL, pdelim);
			if( token != NULL ) {
				g_TestRun.numimages = atoi(token);
			}
		}
        else if( _stricmp(token, "-jpg") == 0 ) {
			g_TestRun.jpgcapture = 1;
		}
#endif
		else if( _stricmp(token, "-pad") == 0 ) {
			token = strtok(NULL, pdelim);
			printf("-pad ignored\n");
		}
		else if( _stricmp(token, "-efile") == 0 ) {
			token = strtok(NULL, pdelim);
			if( token != NULL ) {
				g_TestRun.efile = atoi(token);
			}
		}
		else if( _stricmp(token, "-gs") == 0 ) {
			token = strtok(NULL, pdelim);
			g_TestRun.pgsdll = token;
		}
		else if( _stricmp(token, "-cdvd") == 0 ) {
			token = strtok(NULL, pdelim);
			g_TestRun.pcdvddll = token;
		}
		else if( _stricmp(token, "-spu") == 0 ) {
			token = strtok(NULL, pdelim);
			g_TestRun.pspudll = token;
		}
		else if( _stricmp(token, "-loadgs") == 0 ) {
			token = strtok(NULL, pdelim);
			g_pRunGSState = token;
		}
		else {
            g_TestRun.ptitle = token;
            printf("opening file %s\n", token);
		}

		if( token == NULL ) {
			printf("invalid args\n");
			return -1;
		}

		token = strtok(NULL, pdelim);
	}

	return 0;
}

extern void LoadPatch(char *crc);

BOOL SysLoggedSetLockPagesPrivilege ( HANDLE hProcess, BOOL bEnable);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	char *lang;
	int i;
	
#ifdef PCSX2_VIRTUAL_MEM
	LPVOID lpMemReserved;
#endif

	InitCommonControls();
	pInstance=hInstance;
	FirstShow=true;

#ifdef PCSX2_VIRTUAL_MEM

	if( !SysLoggedSetLockPagesPrivilege( GetCurrentProcess(), TRUE ) )
		return -1;

	lpMemReserved = VirtualAlloc(PS2MEM_BASE, 0x40000000, MEM_RESERVE, PAGE_NOACCESS);

	if( lpMemReserved == NULL || lpMemReserved!= PS2MEM_BASE ) {
		char str[255];
		sprintf(str, "Cannot allocate mem addresses %x-%x, err: %d", PS2MEM_BASE, PS2MEM_BASE+0x40000000, GetLastError());
		MessageBox(NULL, str, "SysError", MB_OK);
		return -1;
	}

	__try 
	{

#endif

	gApp.hInstance = hInstance;
	gApp.hMenu = NULL;
	gApp.hWnd = NULL;
	gApp.hConsole = NULL;

#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, "Langs\\");
	textdomain(PACKAGE);
#endif

	if (LoadConfig() == -1) {
		memset(&Config, 0, sizeof(Config));
		//strcpy(Config.Bios, "HLE");
		strcpy(Config.BiosDir,    "Bios\\");
		strcpy(Config.PluginsDir, "Plugins\\");
		Config.Patch = 1;
        Config.Options = PCSX2_EEREC|PCSX2_VU0REC|PCSX2_VU1REC|PCSX2_COP2REC;

		SysMessage(_("Pcsx2 needs to be configured"));
		Pcsx2Configure(NULL);

		return 0;
	}
	if (Config.Lang[0] == 0) {
		strcpy(Config.Lang, "en_US");
	}

	langs = (_langs*)malloc(sizeof(_langs));
	strcpy(langs[0].lang, "en_US");
	InitLanguages(); i=1;
	while ((lang = GetLanguageNext()) != NULL) {
		langs = (_langs*)realloc(langs, sizeof(_langs)*(i+1));
		strcpy(langs[i].lang, lang);
		i++;
	}
	CloseLanguages();
	langsMax = i;

	if (Config.PsxOut) OpenConsole();

	memset(&g_TestRun, 0, sizeof(g_TestRun));

	if( lpCmdLine == NULL || *lpCmdLine == 0 )
		SysPrintf("-help to see arguments\n");
	else if( ParseCommandLine(lpCmdLine) == -1 ) {
		return 2;
	}

	if( g_TestRun.pgsdll )
		_snprintf(Config.GS, sizeof(Config.GS), "%s", g_TestRun.pgsdll);
	if( g_TestRun.pcdvddll )
		_snprintf(Config.CDVD, sizeof(Config.CDVD), "%s", g_TestRun.pcdvddll);
	if( g_TestRun.pspudll )
		_snprintf(Config.SPU2, sizeof(Config.SPU2), "%s", g_TestRun.pspudll);

	if (SysInit() == -1) return 1;

#ifdef PCSX2_DEVBUILD
    if( g_TestRun.enabled || g_TestRun.ptitle != NULL ) {
		// run without ui
        UseGui = 0;
		_snprintf(filename, sizeof(filename), "%s", g_TestRun.ptitle);
		needReset = 1;
		efile = g_TestRun.efile;
		RunExecute(1);
		SysClose();
		return 0; // success!
	}
#endif

#ifdef PCSX2_DEVBUILD
	if( g_pRunGSState ) {
		LoadGSState(g_pRunGSState);
		SysClose();
		return 0;
	}
#endif

	CreateMainWindow(nCmdShow);

    if( Config.PsxOut ) {
	    // output the help commands
	    SysPrintf("\tF1 - save state\n");
	    SysPrintf("\t(Shift +) F2 - cycle states\n");
	    SysPrintf("\tF3 - load state\n");

#ifdef PCSX2_DEVBUILD
	    SysPrintf("\tF10 - dump performance counters\n");
	    SysPrintf("\tF11 - save GS state\n");
	    SysPrintf("\tF12 - dump hardware registers\n");
#endif
    }

	LoadPatch("default");

//    needReset = 1;
//	efile = 0;
//	RunExecute(1);

	RunGui();

#ifdef PCSX2_VIRTUAL_MEM

	}
	__except(SysPageFaultExceptionFilter(GetExceptionInformation()))
	{
	}

	VirtualFree(PS2MEM_BASE, 0, MEM_RELEASE);
#endif

	return 0;
}

void RunGui() {
    MSG msg;

    for (;;) {
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(10);
	}
}

static int shiftkey = 0;
void CALLBACK KeyEvent(keyEvent* ev)
{
	
	if (ev == NULL) return;
	if (ev->event == KEYRELEASE) {
		switch (ev->key) {
		case VK_SHIFT: shiftkey = 0; break;
		}
		GSkeyEvent(ev); return;
	}
	if (ev->event != KEYPRESS)
        return;

    //some pad plugins don't give a key released event for shift, so this is needed
    //shiftkey = GetKeyState(VK_SHIFT)&0x8000;
    //Well SSXPad breaks with your code, thats why my code worked and your makes reg dumping impossible
	//So i suggest you fix the plugins that dont.
    
	switch (ev->key) {
		case VK_SHIFT: shiftkey = 1; break;
		case VK_F1: ProcessFKeys(1, shiftkey); break;
		case VK_F2: ProcessFKeys(2, shiftkey); break;
        case VK_F3: ProcessFKeys(3, shiftkey); break;
        case VK_F4: ProcessFKeys(4, shiftkey); break;
        case VK_F5: ProcessFKeys(5, shiftkey); break;
        case VK_F6: ProcessFKeys(6, shiftkey); break;
        case VK_F7: ProcessFKeys(7, shiftkey); break;
        case VK_F8: ProcessFKeys(8, shiftkey); break;
        case VK_F9: ProcessFKeys(9, shiftkey); break;
        case VK_F10: ProcessFKeys(10, shiftkey); break;
        case VK_F11: ProcessFKeys(11, shiftkey); break;
        case VK_F12: ProcessFKeys(12, shiftkey); break;

		case VK_ESCAPE:
#ifdef PCSX2_DEVBUILD
			if( g_SaveGSStream >= 3 ) {
				// gs state
				g_SaveGSStream = 4;
				break;
			}
#endif

			ClosePlugins();

            if( !UseGui ) {
                // not using GUI and user just quit, so exit
                exit(0);
            }

			CreateMainWindow(SW_SHOWNORMAL);
			RunGui();
            nDisableSC = 0;
			break;
		default:
			GSkeyEvent(ev);
			break;
	}
}

#ifdef PCSX2_DEVBUILD

BOOL APIENTRY LogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int i;
    switch (message) {
        case WM_INITDIALOG:
			for (i=0; i<32; i++)
				if (varLog & (1<<i))
					CheckDlgButton(hDlg, IDC_LOG+i, TRUE);
			if (Log) CheckDlgButton(hDlg, IDC_LOGS, TRUE);

            return TRUE;

        case WM_COMMAND:

            if (LOWORD(wParam) == IDOK) {
				for (i=0; i<32; i++) {
	 			    int ret = Button_GetCheck(GetDlgItem(hDlg, IDC_LOG+i));
					if (ret) varLog|= 1<<i;
					else varLog&=~(1<<i);
				}
	 			if (Button_GetCheck(GetDlgItem(hDlg, IDC_LOGS)))
					 Log = 1;
				else Log = 0;

				SaveConfig();              

                EndDialog(hDlg, TRUE);
            } 
            return TRUE;
    }

    return FALSE;
}

#endif

BOOL APIENTRY AdvancedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	//char str[256];

    switch (message) {
        case WM_INITDIALOG:
			//CheckDlgButton(hDlg, IDC_REGCACHING, CHECK_REGCACHING ? TRUE : FALSE);
			//if (Config.SafeCnts) CheckDlgButton(hDlg, IDC_SAFECOUNTERS, TRUE);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
				//Config.Options &= ~PCSX2_REGCACHING;
				//Config.Options |= IsDlgButtonChecked(hDlg, IDC_REGCACHING) ? PCSX2_REGCACHING : 0;
			
				SaveConfig();              

                EndDialog(hDlg, TRUE);
            } else
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
            } else
            if (LOWORD(wParam) == IDC_ADVRESET) {
				CheckDlgButton(hDlg, IDC_REGCACHING, FALSE);
				CheckDlgButton(hDlg, IDC_SAFECOUNTERS, FALSE);
				CheckDlgButton(hDlg, IDC_SPU2HACK, FALSE);
            } else
            return TRUE;
    }

    return FALSE;
}

BOOL APIENTRY HacksProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	//char str[256];

    switch (message) {
        case WM_INITDIALOG:
			if(Config.Hacks & 1) CheckDlgButton(hDlg, IDC_SYNCHACK, TRUE);
			if(Config.Hacks & 2) CheckDlgButton(hDlg, IDC_ABSHACK, TRUE);
			if(Config.Hacks & 4) CheckDlgButton(hDlg, IDC_SOUNDHACK, TRUE);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
				Config.Hacks = 0;
				Config.Hacks |= IsDlgButtonChecked(hDlg, IDC_SYNCHACK) ? 1 : 0;
				Config.Hacks |= IsDlgButtonChecked(hDlg, IDC_ABSHACK) ? 2 : 0;
				Config.Hacks |= IsDlgButtonChecked(hDlg, IDC_SOUNDHACK) ? 4 : 0;
			
				SaveConfig();              

                EndDialog(hDlg, TRUE);
            } else
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
            } else
            return TRUE;
    }

    return FALSE;
}

HBITMAP hbitmap_background;//the background image

LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int remoteDebugBios=0;

	switch (msg) {
        case WM_CREATE:
	        return TRUE;

		case WM_PAINT:
	    {
			BITMAP bm;
			PAINTSTRUCT ps;

   	        HDC hdc = BeginPaint(gApp.hWnd, &ps);

   	        HDC hdcMem = CreateCompatibleDC(hdc);
   	        HBITMAP hbmOld = SelectObject(hdcMem, hbitmap_background);

   	        GetObject(hbitmap_background, sizeof(bm), &bm);
//			BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
						ps.rcPaint.right-ps.rcPaint.left+1,
						ps.rcPaint.bottom-ps.rcPaint.top+1,
						hdcMem, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            EndPaint(gApp.hWnd, &ps);
    	 }
		 return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{
			case ID_HACKS:
				 DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_HACKS), hWnd, (DLGPROC)HacksProc);
				 return TRUE;
			case ID_CHEAT_FINDER_SHOW:
				ShowFinder(pInstance,hWnd);
				return TRUE;

			case ID_CHEAT_BROWSER_SHOW:
				ShowCheats(pInstance,hWnd);
				return TRUE;

			case ID_FILE_EXIT:
				SysClose();
				PostQuitMessage(0);
				exit(0);
				return TRUE;

			case ID_FILEOPEN:
				if (Open_File_Proc(filename) == FALSE) return TRUE;

				needReset = 1;
				efile = 1;
				RunExecute(1);
				return TRUE;

			case ID_RUN_EXECUTE:
				if(needReset == 1) RunExe = 1;
				efile = 0;
				RunExecute(1);
				return TRUE;

			case ID_FILE_RUNCD:
				needReset = 1;
				efile = 0;
				RunExecute(1);
                
				return TRUE;

			case ID_RUN_RESET:
				ResetPlugins();
				needReset = 1;
				efile = 0;
				return TRUE;

			//2002-09-20 (Florin)
			case ID_RUN_CMDLINE:
				DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CMDLINE), hWnd, (DLGPROC)CmdlineProc);
				return TRUE;
			//-------------------
           	case ID_PATCHBROWSER:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_PATCHBROWSER), hWnd, (DLGPROC)PatchBDlgProc);
				return TRUE;
			case ID_CONFIG_CONFIGURE:
				Pcsx2Configure(hWnd);
				ReleasePlugins();
				LoadPlugins();
				return TRUE;

			case ID_CONFIG_GRAPHICS:
				if (GSconfigure) GSconfigure();
				return TRUE;
         

			case ID_CONFIG_CONTROLLERS:
				if (PAD1configure) PAD1configure();
				if (PAD2configure) {
					if (strcmp(Config.PAD1, Config.PAD2))PAD2configure();
				}
				return TRUE;

			case ID_CONFIG_SOUND:
				if (SPU2configure) SPU2configure();
				return TRUE;

			case ID_CONFIG_CDVDROM:
				if (CDVDconfigure) CDVDconfigure();
				return TRUE;

			case ID_CONFIG_DEV9:
				if (DEV9configure) DEV9configure();
				return TRUE;

			case ID_CONFIG_USB:
				if (USBconfigure) USBconfigure();
				return TRUE;
  
			case ID_CONFIG_FW:
				if (FWconfigure) FWconfigure();
				return TRUE;

			 case ID_FILE_STATES_LOAD_SLOT1: States_Load(0); return TRUE;
			 case ID_FILE_STATES_LOAD_SLOT2: States_Load(1); return TRUE;
			 case ID_FILE_STATES_LOAD_SLOT3: States_Load(2); return TRUE;
			 case ID_FILE_STATES_LOAD_SLOT4: States_Load(3); return TRUE;
			 case ID_FILE_STATES_LOAD_SLOT5: States_Load(4); return TRUE;
			 case ID_FILE_STATES_LOAD_OTHER: OnStates_LoadOther(); return TRUE;

			 case ID_FILE_STATES_SAVE_SLOT1: States_Save(0); return TRUE;
			 case ID_FILE_STATES_SAVE_SLOT2: States_Save(1); return TRUE;
			 case ID_FILE_STATES_SAVE_SLOT3: States_Save(2); return TRUE;
			 case ID_FILE_STATES_SAVE_SLOT4: States_Save(3); return TRUE;
			 case ID_FILE_STATES_SAVE_SLOT5: States_Save(4); return TRUE;
			 case ID_FILE_STATES_SAVE_OTHER: OnStates_SaveOther(); return TRUE;

			case ID_CONFIG_CPU:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CPUDLG), hWnd, (DLGPROC)CpuDlgProc);
				return TRUE;

			case ID_CONFIG_ADVANCED:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_ADVANCED), hWnd, (DLGPROC)AdvancedProc);
				return TRUE;

#ifdef PCSX2_DEVBUILD
			case ID_DEBUG_ENTERDEBUGGER:
				RunExecute(0);
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_DEBUG), NULL, (DLGPROC)DebuggerProc);
                
				CreateMainWindow(SW_SHOWNORMAL);
				RunGui();
                return TRUE;

			case ID_DEBUG_REMOTEDEBUGGING:
				//read debugging params
				if (Config.Options & PCSX2_EEREC){
					MessageBox(hWnd, _("Nah, you have to be in\nInterpreter Mode to debug"), 0, 0);
					return FALSE;
				} else {
					remoteDebugBios=DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RDEBUGPARAMS), NULL, (DLGPROC)RemoteDebuggerParamsProc);
					if (remoteDebugBios){
						RunExecute(0);

						DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RDEBUG), NULL, (DLGPROC)RemoteDebuggerProc);
						CreateMainWindow(SW_SHOWNORMAL);
						RunGui();
					}
				}
                return TRUE;

			case ID_DEBUG_MEMORY_DUMP:
			    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MEMORY), hWnd, (DLGPROC)MemoryProc);
				return TRUE;

			case ID_DEBUG_LOGGING:
			    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_LOGGING), hWnd, (DLGPROC)LogProc);
				return TRUE;
#endif

			case ID_HELP_ABOUT:
				DialogBox(gApp.hInstance, MAKEINTRESOURCE(ABOUT_DIALOG), hWnd, (DLGPROC)AboutDlgProc);
				return TRUE;

			case ID_HELP_HELP:
				//system("help\\index.html");
				system("compat_list\\compat_list.html");
				return TRUE;

			case ID_CONFIG_MEMCARDS:
				DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MCDCONF), hWnd, (DLGPROC)ConfigureMcdsDlgProc);
				SaveConfig();
				return TRUE;
			case ID_PROCESSLOW: 
               Config.ThPriority = THREAD_PRIORITY_LOWEST;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_UNCHECKED);
                return TRUE;
			case ID_PROCESSNORMAL:
                Config.ThPriority = THREAD_PRIORITY_NORMAL;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_UNCHECKED);
                return TRUE;
			case ID_PROCESSHIGH:
                Config.ThPriority = THREAD_PRIORITY_HIGHEST;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_UNCHECKED);
                return TRUE;
			case ID_CONSOLE:
				if(Config.PsxOut)
				{
                   CheckMenuItem(gApp.hMenu,ID_CONSOLE,MF_UNCHECKED);
				   CloseConsole();
				   Config.PsxOut=0;
				   SaveConfig();
				}
				else
				{
                   CheckMenuItem(gApp.hMenu,ID_CONSOLE,MF_CHECKED);
				   OpenConsole();
				   Config.PsxOut=1;
				   SaveConfig();
				}
				return TRUE;
			case ID_PATCHES:
				if(Config.Patch)
				{
                  CheckMenuItem(gApp.hMenu,ID_PATCHES,MF_UNCHECKED);
				  Config.Patch=0;
				  SaveConfig();
				}
				else
				{
                  CheckMenuItem(gApp.hMenu,ID_PATCHES,MF_CHECKED);
				  Config.Patch=1;
				  SaveConfig();
				}
                return TRUE;
			default:
				if (LOWORD(wParam) >= ID_LANGS && LOWORD(wParam) <= (ID_LANGS + langsMax)) {
					AccBreak = 1;
					DestroyWindow(gApp.hWnd);
					ChangeLanguage(langs[LOWORD(wParam) - ID_LANGS].lang);
					CreateMainWindow(SW_NORMAL);
					return TRUE;
				}
			}
			break;
		case WM_DESTROY:
			if (!AccBreak) {
				SysClose();
                DeleteObject(hbitmap_background);
				PostQuitMessage(0);
				exit(0);
			} else AccBreak = 0;
		    return TRUE;

        case WM_SYSCOMMAND:
            if( nDisableSC && (wParam== SC_SCREENSAVE || wParam == SC_MONITORPOWER) ) {
               return FALSE;
            }
            else
                return DefWindowProc(hWnd, msg, wParam, lParam);
            break;
        
		case WM_QUIT:
			if (Config.PsxOut) CloseConsole();
			exit(0);
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return FALSE;
}

#define _ADDSUBMENU(menu, menun, string) \
	submenu[menun] = CreatePopupMenu(); \
	AppendMenu(menu, MF_STRING | MF_POPUP, (UINT)submenu[menun], string);

#define ADDSUBMENU(menun, string) \
	_ADDSUBMENU(gApp.hMenu, menun, string);

#define ADDSUBMENUS(submn, menun, string) \
	submenu[menun] = CreatePopupMenu(); \
	InsertMenu(submenu[submn], 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)submenu[menun], string);

#define ADDMENUITEM(menun, string, id) \
	item.fType = MFT_STRING; \
	item.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID; \
	item.fState = MFS_ENABLED; \
	item.wID = id; \
	sprintf(buf, string); \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

#define ADDMENUITEMC(menun, string, id) \
	item.fType = MFT_STRING; \
	item.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID; \
	item.fState = MFS_ENABLED | MFS_CHECKED; \
	item.wID = id; \
	sprintf(buf, string); \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

#define ADDSEPARATOR(menun) \
	item.fMask = MIIM_TYPE; \
	item.fType = MFT_SEPARATOR; \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

void CreateMainMenu() {
	MENUITEMINFO item;
	HMENU submenu[5];
	char buf[256];
	int i;

	item.cbSize = sizeof(MENUITEMINFO);
	item.dwTypeData = buf;
	item.cch = 256;

	gApp.hMenu = CreateMenu();

	//submenu = CreatePopupMenu();
	//AppendMenu(gApp.hMenu, MF_STRING | MF_POPUP, (UINT)submenu, _("&File"));
    ADDSUBMENU(0, _("&File"));
	ADDMENUITEM(0, _("E&xit"), ID_FILE_EXIT);
	ADDSEPARATOR(0);
	ADDSUBMENUS(0, 1, _("&States"));
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("&Open ELF File"), ID_FILEOPEN);
	ADDMENUITEM(0, _("&Run CD/DVD"), ID_FILE_RUNCD);
	ADDSUBMENUS(1, 3, _("&Save"));
	ADDSUBMENUS(1, 2, _("&Load"));
	ADDMENUITEM(2, _("&Other..."), ID_FILE_STATES_LOAD_OTHER);
	ADDMENUITEM(2, _("Slot &5"), ID_FILE_STATES_LOAD_SLOT5);
	ADDMENUITEM(2, _("Slot &4"), ID_FILE_STATES_LOAD_SLOT4);
	ADDMENUITEM(2, _("Slot &3"), ID_FILE_STATES_LOAD_SLOT3);
	ADDMENUITEM(2, _("Slot &2"), ID_FILE_STATES_LOAD_SLOT2);
	ADDMENUITEM(2, _("Slot &1"), ID_FILE_STATES_LOAD_SLOT1);
	ADDMENUITEM(3, _("&Other..."), ID_FILE_STATES_SAVE_OTHER);
	ADDMENUITEM(3, _("Slot &5"), ID_FILE_STATES_SAVE_SLOT5);
	ADDMENUITEM(3, _("Slot &4"), ID_FILE_STATES_SAVE_SLOT4);
	ADDMENUITEM(3, _("Slot &3"), ID_FILE_STATES_SAVE_SLOT3);
	ADDMENUITEM(3, _("Slot &2"), ID_FILE_STATES_SAVE_SLOT2);
	ADDMENUITEM(3, _("Slot &1"), ID_FILE_STATES_SAVE_SLOT1);

    ADDSUBMENU(0, _("&Run"));

    ADDSUBMENUS(0, 1, _("&Process Priority"));
	ADDMENUITEM(1, _("&Low"), ID_PROCESSLOW );
	ADDMENUITEM(1, _("High"), ID_PROCESSHIGH);
	ADDMENUITEM(1, _("Normal"), ID_PROCESSNORMAL);
	ADDMENUITEM(0,_("&Arguments"), ID_RUN_CMDLINE);
	ADDMENUITEM(0,_("Re&set"), ID_RUN_RESET);
	ADDMENUITEM(0,_("E&xecute"), ID_RUN_EXECUTE);

	ADDSUBMENU(0,_("&Config"));
#ifdef PCSX2_DEVBUILD
//	ADDMENUITEM(0,_("&Advanced"), ID_CONFIG_ADVANCED);
#endif
	ADDMENUITEM(0,_("Speed &Hacks"), ID_HACKS);
	ADDMENUITEM(0,_("&Patches"), ID_PATCHBROWSER);
	ADDMENUITEM(0,_("C&pu"), ID_CONFIG_CPU);
	ADDMENUITEM(0,_("&Memcards"), ID_CONFIG_MEMCARDS);
	ADDSEPARATOR(0);
	ADDMENUITEM(0,_("Fire&Wire"), ID_CONFIG_FW);
	ADDMENUITEM(0,_("U&SB"), ID_CONFIG_USB);
	ADDMENUITEM(0,_("D&ev9"), ID_CONFIG_DEV9);
	ADDMENUITEM(0,_("C&dvdrom"), ID_CONFIG_CDVDROM);
	ADDMENUITEM(0,_("&Sound"), ID_CONFIG_SOUND);
	ADDMENUITEM(0,_("C&ontrollers"), ID_CONFIG_CONTROLLERS);
	ADDMENUITEM(0,_("&Graphics"), ID_CONFIG_GRAPHICS);
	ADDSEPARATOR(0);
	ADDMENUITEM(0,_("&Configure"), ID_CONFIG_CONFIGURE);

    ADDSUBMENU(0,_("&Language"));

	for (i=langsMax-1; i>=0; i--) {
		if (!strcmp(Config.Lang, langs[i].lang)) {
			ADDMENUITEMC(0,ParseLang(langs[i].lang), ID_LANGS + i);
		} else {
			ADDMENUITEM(0,ParseLang(langs[i].lang), ID_LANGS + i);
		}
	}

#ifdef PCSX2_DEVBUILD
	ADDSUBMENU(0, _("&Debug"));
	ADDMENUITEM(0,_("&Logging"), ID_DEBUG_LOGGING);
	ADDMENUITEM(0,_("Memory Dump"), ID_DEBUG_MEMORY_DUMP);
	ADDMENUITEM(0,_("&Remote Debugging"), ID_DEBUG_REMOTEDEBUGGING);
	ADDMENUITEM(0,_("Enter &Debugger..."), ID_DEBUG_ENTERDEBUGGER);
#endif

	ADDSUBMENU(0, _("&Misc"));
	ADDMENUITEM(0,_("Enable &Patches"), ID_PATCHES);
	ADDMENUITEM(0,_("Enable &Console"), ID_CONSOLE); 
	ADDSEPARATOR(0);
	ADDMENUITEM(0,_("Patch &Finder..."), ID_CHEAT_FINDER_SHOW); 
	ADDMENUITEM(0,_("Patch &Browser..."), ID_CHEAT_BROWSER_SHOW); 


    ADDSUBMENU(0, _("&Help"));
	ADDMENUITEM(0,_("&Compatibility List..."), ID_HELP_HELP);
	ADDMENUITEM(0,_("&About..."), ID_HELP_ABOUT);

#ifndef PCSX2_DEVBUILD
	EnableMenuItem(GetSubMenu(gApp.hMenu, 4), ID_DEBUG_LOGGING, MF_GRAYED);
#endif
}

void CreateMainWindow(int nCmdShow) {
	WNDCLASS wc;
	HWND hWnd;
	char buf[256];
	char COMPILER[20]="";
	BITMAP bm;
	RECT rect;
	int w, h;

#ifdef _MSC_VER
	sprintf(COMPILER, "(VC%d)", (_MSC_VER+100)/200);//hacky:) works for VC6 & VC.NET
#elif __BORLANDC__
	sprintf(COMPILER, "(BC)");
#endif
	/* Load Background Bitmap from the ressource */ 
	hbitmap_background = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(SPLASH_LOGO));

	wc.lpszClassName = "PCSX2 Main";
	wc.lpfnWndProc = MainWndProc;
	wc.style = 0;
	wc.hInstance = gApp.hInstance;
	wc.hIcon = LoadIcon(gApp.hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_MENUTEXT);
	wc.lpszMenuName = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	RegisterClass(&wc);
	GetObject(hbitmap_background, sizeof(bm), &bm);

	{
#ifdef PCSX2_VIRTUAL_MEM
		const char* pvm = "VM";
#else
		const char* pvm = "non VM";
#endif

#ifdef PCSX2_DEVBUILD
		sprintf(buf, _("PCSX2 %s Watermoose - %s Compile Date - %s %s"), PCSX2_VERSION, pvm, COMPILEDATE, COMPILER);
#else
		sprintf(buf, _("PCSX2 %s Watermoose - %s"), PCSX2_VERSION, pvm);
#endif
	}

	hWnd = CreateWindow("PCSX2 Main",
						buf, WS_OVERLAPPED | WS_SYSMENU,
						20, 20, 320, 240, NULL, NULL,
						gApp.hInstance, NULL);

	gApp.hWnd = hWnd;
    ResetMenuSlots();
	CreateMainMenu();
   
	SetMenu(gApp.hWnd, gApp.hMenu);
    if(Config.ThPriority==THREAD_PRIORITY_NORMAL) CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_CHECKED);
	if(Config.ThPriority==THREAD_PRIORITY_HIGHEST) CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_CHECKED);
	if(Config.ThPriority==THREAD_PRIORITY_LOWEST)  CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_CHECKED);
	if(Config.PsxOut) CheckMenuItem(gApp.hMenu,ID_CONSOLE,MF_CHECKED);
	if(Config.Patch)  CheckMenuItem(gApp.hMenu,ID_PATCHES,MF_CHECKED);
	hStatusWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "", hWnd, 100);
    sprintf(buf, "PCSX2 %s", PCSX2_VERSION);
	StatusSet(buf);

	w = bm.bmWidth; h = bm.bmHeight;
	GetWindowRect(hStatusWnd, &rect);
	h+= rect.bottom - rect.top;
	GetMenuItemRect(hWnd, gApp.hMenu, 0, &rect);
	h+= rect.bottom - rect.top;
	MoveWindow(hWnd, 20, 20, w, h, TRUE);

	DestroyWindow(hStatusWnd);
	hStatusWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "", hWnd, 100);
	sprintf(buf, "F1 - save, F2 - next state, Shift+F2 - prev state, F3 - load, F8 - snapshot", PCSX2_VERSION);
	StatusSet(buf);
	ShowWindow(hWnd, nCmdShow);
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}

BOOL Open_File_Proc(char * filename) {
	OPENFILENAME ofn;
	char szFileName[ 256 ];
	char szFileTitle[ 256 ];
	char * filter = "ELF Files (*.ELF)\0*.ELF\0ALL Files (*.*)\0*.*\0";

	memset( &szFileName, 0, sizeof( szFileName ) );
	memset( &szFileTitle, 0, sizeof( szFileTitle ) );

	ofn.lStructSize			= sizeof( OPENFILENAME );
	ofn.hwndOwner			= gApp.hWnd;
	ofn.lpstrFilter			= filter;
	ofn.lpstrCustomFilter   = NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= szFileName;
	ofn.nMaxFile			= 256;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrFileTitle		= szFileTitle;
	ofn.nMaxFileTitle		= 256;
	ofn.lpstrTitle			= NULL;
	ofn.lpstrDefExt			= "ELF";
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	 
	if (GetOpenFileName(&ofn)) {
		struct stat buf;

		if (stat(szFileName, &buf) != 0) {
			return FALSE;
		}

		strcpy(filename, szFileName);
		return TRUE;             
	}

	return FALSE;
}

//2002-09-20 (Florin)
BOOL APIENTRY CmdlineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
			SetWindowText(hDlg, _("Program arguments"));

			Button_SetText(GetDlgItem(hDlg, IDOK), _("OK"));
			Button_SetText(GetDlgItem(hDlg, IDCANCEL), _("Cancel"));
			Static_SetText(GetDlgItem(hDlg, IDC_TEXT), _("Fill in the command line arguments for opened program:"));
			Static_SetText(GetDlgItem(hDlg, IDC_TIP), _("Tip: If you don't know what to write\nleave it blank"));

            SetDlgItemText(hDlg, IDC_CMDLINE, args);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
				char tmp[256];

				GetDlgItemText(hDlg, IDC_CMDLINE, tmp, 256);

				ZeroMemory(args, 256);
				strcpy(args, tmp);
				args[255]=0;
                
                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

WIN32_FIND_DATA lFindData;
HANDLE lFind;
int lFirst;

void InitLanguages() {
	lFind = FindFirstFile("Langs\\*", &lFindData);
	lFirst = 1;
}

char *GetLanguageNext() {
	for (;;) {
		if (!strcmp(lFindData.cFileName, ".")) {
			if (FindNextFile(lFind, &lFindData) == FALSE)
				return NULL;
			continue;
		}
		if (!strcmp(lFindData.cFileName, "..")) {
			if (FindNextFile(lFind, &lFindData) == FALSE)
				return NULL;
			continue;
		}
		break;
	}
	if (lFirst == 0) {
		if (FindNextFile(lFind, &lFindData) == FALSE)
			return NULL;
	} else lFirst = 0;
	if (lFind==INVALID_HANDLE_VALUE) return NULL;

	return lFindData.cFileName;
}

void CloseLanguages() {
	if (lFind!=INVALID_HANDLE_VALUE) FindClose(lFind);
}

void ChangeLanguage(char *lang) {
	strcpy(Config.Lang, lang);
	SaveConfig();
	LoadConfig();
}

//-------------------

static int sinit=0;

int SysInit() {
	CreateDirectory(MEMCARDS_DIR, NULL);
	CreateDirectory(SSTATES_DIR, NULL);
#ifdef EMU_LOG
	CreateDirectory(LOGS_DIR, NULL);

#ifdef PCSX2_DEVBUILD
	if( g_TestRun.plogname != NULL )
		emuLog = fopen(g_TestRun.plogname, "w");
	if( emuLog == NULL )
		emuLog = fopen(LOGS_DIR "\\emuLog.txt","w");
#endif

	if( emuLog != NULL )
		setvbuf(emuLog, NULL,  _IONBF, 0);

#endif
	if (cpuInit() == -1) return -1;

	while (LoadPlugins() == -1) {
		if (Pcsx2Configure(NULL) == FALSE) {
			exit(1);
		}
	}

	sinit=1;

	return 0;
}

void SysReset() {
	if (sinit == 0) return;
	StatusSet(_("Resetting..."));
	cpuReset();
	StatusSet(_("Ready"));
}


void SysClose() {
	if (sinit == 0) return;
	cpuShutdown();
	ReleasePlugins();
	sinit=0;
}

int concolors[] = {
	0,
	FOREGROUND_RED,
	FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_RED,
	FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
};

void SysPrintf(char *fmt, ...) {
	va_list list;
	char msg[512];
	char *ptr;
	DWORD tmp;
	int len, s;
	int i, j;

	va_start(list,fmt);
	_vsnprintf(msg,511,fmt,list);
	msg[511] = '\0';
	va_end(list);

    if (Config.PsxOut == 0) {
#ifdef EMU_LOG
#ifndef LOG_STDOUT
        if (emuLog != NULL && !(varLog & 0x80000000)) {
            fprintf(emuLog, "%s", msg);
        }
#endif
#endif
        return;
    }

	ptr = msg; len = strlen(msg);
	for (i=0, j=0; i<len; i++, j++) {
		if (ptr[j] == '\033') {
			ptr[j] = 0;
			WriteConsole(gApp.hConsole, ptr, (DWORD)strlen(ptr), &tmp, 0);
#ifdef EMU_LOG
#ifndef LOG_STDOUT
			if (emuLog != NULL && !(varLog & 0x80000000)) {
				fprintf(emuLog, "%s", ptr);
			}
#endif
#endif

			if (ptr[j+2] == '0') {
				SetConsoleTextAttribute(gApp.hConsole, 7);
				s = 4;
			} else {
				SetConsoleTextAttribute(gApp.hConsole, 7);
				s = 5;
			}
			ptr+= j+s;
			j = 0;
		}
	}

#ifdef EMU_LOG
#ifndef LOG_STDOUT
	if (emuLog != NULL && !(varLog & 0x80000000))
		fprintf(emuLog, "%s", ptr);
#endif
#endif
	WriteConsole(gApp.hConsole, ptr, (DWORD)strlen(ptr), &tmp, 0);
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	_vsnprintf(tmp,511,fmt,list);
	tmp[511] = '\0';
	va_end(list);
	MessageBox(0, tmp, _("Pcsx2 Msg"), 0);
}

void SysUpdate() {

    KeyEvent(PAD1keyEvent()); //Only need 1 as its used for windows keys only
	KeyEvent(PAD2keyEvent());
}

void SysRunGui() {
	RunGui();
}

static char *err = N_("Error Loading Symbol");
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
	if (errval) { errval = 0; return _(err); }
	return NULL;
}

void SysCloseLibrary(void *lib) {
	FreeLibrary((HINSTANCE)lib);
}

void *SysMmap(uptr base, u32 size) {
	void *mem;

	mem = VirtualAlloc((void*)base, size, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	mem = VirtualAlloc((void*)mem,  size, MEM_COMMIT , PAGE_EXECUTE_READWRITE);
	return mem;
}

void SysMunmap(uptr base, u32 size) {
	VirtualFree((void*)base, size, MEM_DECOMMIT);
	VirtualFree((void*)base, 0, MEM_RELEASE);
}

#ifdef PCSX2_VIRTUAL_MEM

// virtual memory/privileges
#include "ntsecapi.h"

static wchar_t s_szUserName[255];

LRESULT WINAPI UserNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowPos(hDlg, HWND_TOPMOST, 200, 100, 0, 0, SWP_NOSIZE);
			return TRUE;

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
				{
					wchar_t str[255];
					GetWindowTextW(GetDlgItem(hDlg, IDC_USER_NAME), str, 255);
					swprintf(s_szUserName, 255, L"%S", str);
					EndDialog(hDlg, TRUE );
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(hDlg, FALSE );
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL InitLsaString(
  PLSA_UNICODE_STRING pLsaString,
  LPCWSTR pwszString
)
{
  DWORD dwLen = 0;

  if (NULL == pLsaString)
      return FALSE;

  if (NULL != pwszString) 
  {
      dwLen = wcslen(pwszString);
      if (dwLen > 0x7ffe)   // String is too large
          return FALSE;
  }

  // Store the string.
  pLsaString->Buffer = (WCHAR *)pwszString;
  pLsaString->Length =  (USHORT)dwLen * sizeof(WCHAR);
  pLsaString->MaximumLength= (USHORT)(dwLen+1) * sizeof(WCHAR);

  return TRUE;
}

PLSA_TRANSLATED_SID2 GetSIDInformation (LPWSTR AccountName,LSA_HANDLE PolicyHandle)
{
  LSA_UNICODE_STRING lucName;
  PLSA_TRANSLATED_SID2 ltsTranslatedSID;
  PLSA_REFERENCED_DOMAIN_LIST lrdlDomainList;
  //LSA_TRUST_INFORMATION myDomain;
  NTSTATUS ntsResult;
  PWCHAR DomainString = NULL;

  // Initialize an LSA_UNICODE_STRING with the name.
  if (!InitLsaString(&lucName, AccountName))
  {
         wprintf(L"Failed InitLsaString\n");
         return NULL;
  }

  ntsResult = LsaLookupNames2(
     PolicyHandle,     // handle to a Policy object
	 0,
     1,                // number of names to look up
     &lucName,         // pointer to an array of names
     &lrdlDomainList,  // receives domain information
     &ltsTranslatedSID // receives relative SIDs
  );
  if (0 != ntsResult) 
  {
    wprintf(L"Failed LsaLookupNames - %lu \n",
      LsaNtStatusToWinError(ntsResult));
    return NULL;
  }

  // Get the domain the account resides in.
//  myDomain = lrdlDomainList->Domains[ltsTranslatedSID->DomainIndex];
//  DomainString = (PWCHAR) LocalAlloc(LPTR, myDomain.Name.Length + 1);
//  wcsncpy(DomainString, myDomain.Name.Buffer, myDomain.Name.Length);

  // Display the relative Id. 
//  wprintf(L"Relative Id is %lu in domain %ws.\n",
//    ltsTranslatedSID->RelativeId,
//    DomainString);

  LsaFreeMemory(lrdlDomainList);

  return ltsTranslatedSID;
}

BOOL AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle, BOOL bAdd)
{
  LSA_UNICODE_STRING lucPrivilege;
  NTSTATUS ntsResult;

  // Create an LSA_UNICODE_STRING for the privilege name(s).
  if (!InitLsaString(&lucPrivilege, L"SeLockMemoryPrivilege"))
  {
         wprintf(L"Failed InitLsaString\n");
         return FALSE;
  }

  if( bAdd ) {
    ntsResult = LsaAddAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID.
        &lucPrivilege, // The privilege(s).
        1              // Number of privileges.
    );
  }
  else {
      ntsResult = LsaRemoveAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID
        FALSE,
        &lucPrivilege, // The privilege(s).
        1              // Number of privileges.
    );
  }
      
  if (ntsResult == 0) 
  {
    wprintf(L"Privilege added.\n");
  }
  else
  {
	  int err = LsaNtStatusToWinError(ntsResult);
	  char str[255];
		_snprintf(str, 255, "Privilege was not added - %lu \n", LsaNtStatusToWinError(ntsResult));
		MessageBox(NULL, str, "Privilege error", MB_OK);
	return FALSE;
  }

  return TRUE;
} 

#define TARGET_SYSTEM_NAME L"mysystem"
LSA_HANDLE GetPolicyHandle()
{
  LSA_OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR SystemName[] = TARGET_SYSTEM_NAME;
  USHORT SystemNameLength;
  LSA_UNICODE_STRING lusSystemName;
  NTSTATUS ntsResult;
  LSA_HANDLE lsahPolicyHandle;

  // Object attributes are reserved, so initialize to zeroes.
  ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

  //Initialize an LSA_UNICODE_STRING to the server name.
  SystemNameLength = wcslen(SystemName);
  lusSystemName.Buffer = SystemName;
  lusSystemName.Length = SystemNameLength * sizeof(WCHAR);
  lusSystemName.MaximumLength = (SystemNameLength+1) * sizeof(WCHAR);

  // Get a handle to the Policy object.
  ntsResult = LsaOpenPolicy(
        NULL,    //Name of the target system.
        &ObjectAttributes, //Object attributes.
        POLICY_ALL_ACCESS, //Desired access permissions.
        &lsahPolicyHandle  //Receives the policy handle.
    );

  if (ntsResult != 0)
  {
    // An error occurred. Display it as a win32 error code.
    wprintf(L"OpenPolicy returned %lu\n",
      LsaNtStatusToWinError(ntsResult));
    return NULL;
  } 
  return lsahPolicyHandle;
}


/*****************************************************************
   LoggedSetLockPagesPrivilege: a function to obtain, if possible, or
   release the privilege of locking physical pages.

   Inputs:

       HANDLE hProcess: Handle for the process for which the
       privilege is needed

       BOOL bEnable: Enable (TRUE) or disable?

   Return value: TRUE indicates success, FALSE failure.

*****************************************************************/
BOOL SysLoggedSetLockPagesPrivilege ( HANDLE hProcess, BOOL bEnable)
{
	struct {
	u32 Count;
	LUID_AND_ATTRIBUTES Privilege [1];
	} Info;

	HANDLE Token;
	BOOL Result;

	// Open the token.

	Result = OpenProcessToken ( hProcess,
								TOKEN_ADJUST_PRIVILEGES,
								& Token);

	if( Result != TRUE ) {
	SysPrintf( "Cannot open process token.\n" );
	return FALSE;
	}

	// Enable or disable?

	Info.Count = 1;
	if( bEnable ) 
	{
	Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	} 
	else 
	{
	Info.Privilege[0].Attributes = SE_PRIVILEGE_REMOVED;
	}

	// Get the LUID.
	Result = LookupPrivilegeValue ( NULL,
									SE_LOCK_MEMORY_NAME,
									&(Info.Privilege[0].Luid));

	if( Result != TRUE ) 
	{
	SysPrintf( "Cannot get privilege value for %s.\n", SE_LOCK_MEMORY_NAME );
	return FALSE;
	}

	// Adjust the privilege.

	Result = AdjustTokenPrivileges ( Token, FALSE,
									(PTOKEN_PRIVILEGES) &Info,
									0, NULL, NULL);

	// Check the result.
	if( Result != TRUE ) 
	{
		SysPrintf ("Cannot adjust token privileges, error %u.\n", GetLastError() );
		return FALSE;
	} 
	else 
	{
		if( GetLastError() != ERROR_SUCCESS ) 
		{

			BOOL bSuc = FALSE;
			LSA_HANDLE policy;
			PLSA_TRANSLATED_SID2 ltsTranslatedSID;

//			if( !DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_USERNAME), gApp.hWnd, (DLGPROC)UserNameProc) )
//				return FALSE;
            DWORD len = sizeof(s_szUserName);
            GetUserNameW(s_szUserName, &len);

			policy = GetPolicyHandle();

			if( policy != NULL ) {

				ltsTranslatedSID = GetSIDInformation(s_szUserName, policy);

				if( ltsTranslatedSID != NULL ) {
					bSuc = AddPrivileges(ltsTranslatedSID->Sid, policy, bEnable);
					LsaFreeMemory(ltsTranslatedSID);
				}

				LsaClose(policy);
			}

			if( bSuc ) {
				// Get the LUID.
				LookupPrivilegeValue ( NULL, SE_LOCK_MEMORY_NAME, &(Info.Privilege[0].Luid));

				bSuc = AdjustTokenPrivileges ( Token, FALSE, (PTOKEN_PRIVILEGES) &Info, 0, NULL, NULL);
			}

			if( bSuc ) {
				if( MessageBox(NULL, "PCSX2 just changed your SE_LOCK_MEMORY privilege in order to gain access to physical memory.\n"
								"Log off/on and run pcsx2 again. Do you want to log off?\n",
								"Privilege changed query", MB_YESNO) == IDYES ) {
					ExitWindows(EWX_LOGOFF, 0);
				}
			}
			else {
				MessageBox(NULL, "Failed adding SE_LOCK_MEMORY privilege, please check the local policy.\n"
					"Go to security settings->Local Policies->User Rights. There should be a \"Lock pages in memory\".\n"
					"Add your user to that and log off/on. This enables pcsx2 to run at real-time by allocating physical memory.\n"
					"Also can try Control Panel->Local Security Policy->... (this does not work on Windows XP Home)\n"
					"(zerofrog)\n", "Virtual Memory Access Denied", MB_OK);
				return FALSE;
			}
		}
	}

	CloseHandle( Token );

	return TRUE;
}

static u32 s_dwPageSize = 0;
int SysPhysicalAlloc(u32 size, PSMEMORYBLOCK* pblock)
{
//#ifdef WIN32_FILE_MAPPING
//	assert(0);
//#endif
	ULONG_PTR NumberOfPagesInitial; // initial number of pages requested
	int PFNArraySize;               // memory to request for PFN array
	BOOL bResult;

	assert( pblock != NULL );
	memset(pblock, 0, sizeof(PSMEMORYBLOCK));

	if( s_dwPageSize == 0 ) {
		SYSTEM_INFO sSysInfo;           // useful system information
		GetSystemInfo(&sSysInfo);  // fill the system information structure
		s_dwPageSize = sSysInfo.dwPageSize;

		if( s_dwPageSize != 0x1000 ) {
			SysMessage("Error! OS page size must be 4Kb!\n"
				"If for some reason the OS cannot have 4Kb pages, then run the TLB build.");
			return -1;
		}
	}

	// Calculate the number of pages of memory to request.
	pblock->NumberPages = (size+s_dwPageSize-1)/s_dwPageSize;
	PFNArraySize = pblock->NumberPages * sizeof (ULONG_PTR);

	pblock->aPFNs = (ULONG_PTR *) HeapAlloc (GetProcessHeap (), 0, PFNArraySize);

	if (pblock->aPFNs == NULL) {
		SysPrintf("Failed to allocate on heap.\n");
		goto eCleanupAndExit;
	}

	// Allocate the physical memory.
	NumberOfPagesInitial = pblock->NumberPages;
	bResult = AllocateUserPhysicalPages( GetCurrentProcess(), &pblock->NumberPages, pblock->aPFNs );

	if( bResult != TRUE ) 
	{
		SysPrintf("Cannot allocate physical pages, error %u.\n", GetLastError() );
		goto eCleanupAndExit;
	}

	if( NumberOfPagesInitial != pblock->NumberPages ) 
	{
		SysPrintf("Allocated only %p of %p pages.\n", pblock->NumberPages, NumberOfPagesInitial );
		goto eCleanupAndExit;
	}

	pblock->aVFNs = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, PFNArraySize);

	return 0;

eCleanupAndExit:
	SysPhysicalFree(pblock);
	return -1;
}

void SysPhysicalFree(PSMEMORYBLOCK* pblock)
{
	assert( pblock != NULL );

	// Free the physical pages.
	FreeUserPhysicalPages( GetCurrentProcess(), &pblock->NumberPages, pblock->aPFNs );

	if( pblock->aPFNs != NULL ) HeapFree(GetProcessHeap(), 0, pblock->aPFNs);
	if( pblock->aVFNs != NULL ) HeapFree(GetProcessHeap(), 0, pblock->aVFNs);
	memset(pblock, 0, sizeof(PSMEMORYBLOCK));
}

int SysVirtualPhyAlloc(void* base, u32 size, PSMEMORYBLOCK* pblock)
{
	BOOL bResult;
	int i;

	LPVOID lpMemReserved = VirtualAlloc( base, size, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE );
	if( lpMemReserved == NULL || base != lpMemReserved )
	{
		SysPrintf("Cannot reserve memory at 0x%8.8x(%x), error: %d.\n", base, lpMemReserved, GetLastError());
		goto eCleanupAndExit;
	}

	// Map the physical memory into the window.  
	bResult = MapUserPhysicalPages( base, pblock->NumberPages, pblock->aPFNs );

	for(i = 0; i < pblock->NumberPages; ++i)
		pblock->aVFNs[i] = (ULONG_PTR)base + 0x1000*i;

	if( bResult != TRUE ) 
	{
		SysPrintf("MapUserPhysicalPages failed to map, error %u.\n", GetLastError() );
		goto eCleanupAndExit;
	}

	return 0;

eCleanupAndExit:
	SysVirtualFree(base, size);
	return -1;
}

void SysVirtualFree(void* lpMemReserved, u32 size)
{
	// unmap   
	if( MapUserPhysicalPages( lpMemReserved, (size+s_dwPageSize-1)/s_dwPageSize, NULL ) != TRUE ) 
	{
		SysPrintf("MapUserPhysicalPages failed to unmap, error %u.\n", GetLastError() );
		return;
	}

	// Free virtual memory.
	VirtualFree( lpMemReserved, 0, MEM_RELEASE );
}

int SysMapUserPhysicalPages(void* Addr, uptr NumPages, uptr* pfn, int pageoffset)
{
	BOOL bResult = MapUserPhysicalPages(Addr, NumPages, pfn+pageoffset);

#ifdef _DEBUG
	//if( !bResult )
		//__Log("Failed to map user pages: 0x%x:0x%x, error = %d\n", Addr, NumPages, GetLastError());
#endif

	return bResult;
}

#else

#endif
