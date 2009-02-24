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

#include <winnt.h>
#include <commctrl.h>
#include <direct.h>

#include <ntsecapi.h>

#include "Common.h"
#include "debugger.h"
#include "rdebugger.h"
#include "AboutDlg.h"
#include "McdsDlg.h"

#include "Patch.h"
#include "cheats/cheats.h"

#include "Paths.h"
#include "SamplProf.h"

#include "implement.h"		// pthreads-win32 defines for startup/shutdown

unsigned int langsMax;
static bool m_RestartGui = false;	// used to signal a GUI restart after DestroyWindow()
static HBITMAP hbitmap_background = NULL;


struct _langs {
	TCHAR lang[256];
};

_langs *langs = NULL;

void strcatz(char *dst, char *src)
{
	int len = strlen(dst) + 1;
	strcpy(dst + len, src);
}

//2002-09-20 (Florin)
BOOL APIENTRY CmdlineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);//forward def
//-------------------

TESTRUNARGS g_TestRun;

static const char* phelpmsg = 
    "pcsx2 [options] [file]\n\n"
    "-cfg [file] {configuration file}\n"
    "-efile [efile] {0 - reset, 1 - runcd (default), 2 - loadelf}\n"
    "-help {display this help file}\n"
    "-nogui {Don't use gui when launching}\n"
	"-loadgs [file] {Loads a gsstate}\n"
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

    "Plugin Overrides (specified dlls will be used in place of configured dlls):\n"
    "\t-cdvd [dllpath] {specifies an override for the CDVD plugin}\n"
    "\t-gs [dllpath] {specifies an override for the GS plugin}\n"
    "\t-spu [dllpath] {specifies an override for the SPU2 plugin}\n"
	"\t-pads [dllpath] {specifies an override for *both* pad plugins}\n"
	"\t-pad1 [dllpath] {specifies an override for the PAD1 plugin only}\n"
	"\t-pad2 [dllpath] {specifies an override for the PAD2 plugin only}\n"
	"\t-dev9 [dllpath] {specifies an override for the DEV9 plugin}\n"
    "\n";

/// This code is courtesy of http://alter.org.ua/en/docs/win/args/
static PTCHAR* _CommandLineToArgv( const TCHAR *CmdLine, int* _argc )
{
    PTCHAR* argv;
    PTCHAR  _argv;
    ULONG   len;
    ULONG   argc;
    TCHAR   a;
    ULONG   i, j;

    BOOLEAN  in_QM;
    BOOLEAN  in_TEXT;
    BOOLEAN  in_SPACE;

	len = _tcslen( CmdLine );
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (PTCHAR*)GlobalAlloc(GMEM_FIXED,
        i + (len+2)*sizeof(a));

    _argv = (PTCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while( a = CmdLine[i] ) {
        if(in_QM) {
            if(a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case '\"':
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if(in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

void WinClose()
{
	SysClose();

	// Don't check Config.Profiler here -- the Profiler will know if it's running or not.
	ProfilerTerm();

	ReleasePlugins();
	Console::Close();

	pthread_win32_process_detach_np();

	SysShutdownDynarecs();
	SysShutdownMem();
}

BOOL SysLoggedSetLockPagesPrivilege ( HANDLE hProcess, BOOL bEnable);

// Returns TRUE if the test run mode was activated (game was run and has been exited)
static bool TestRunMode()
{
	if( IsDevBuild && (g_TestRun.enabled || g_TestRun.ptitle != NULL) )
	{
		// run without ui
		UseGui = 0;
		PCSX2_MEM_PROTECT_BEGIN();
		RunExecute( g_TestRun.efile ? g_TestRun.ptitle : NULL );
		PCSX2_MEM_PROTECT_END();
		return true;
	}
	return false;
}

static void _doPluginOverride( const char* name, const char* src, char (&dest)[g_MaxPath] )
{
	if( src == NULL ) return;

	_tcscpy_s( dest, g_TestRun.pgsdll );
	Console::Notice( "* %s plugin override: \n\t%s\n", params name, Config.GS );
}

void WinRun()
{
	// Load the command line overrides for plugins.
	// Back up the user's preferences in winConfig.

	memcpy( &winConfig, &Config, sizeof( PcsxConfig ) );

	_doPluginOverride( "GS", g_TestRun.pgsdll, Config.GS );
	_doPluginOverride( "CDVD", g_TestRun.pcdvddll, Config.CDVD );
	_doPluginOverride( "SPU2", g_TestRun.pspudll, Config.SPU2 );
	_doPluginOverride( "PAD1", g_TestRun.ppad1dll, Config.PAD1 );
	_doPluginOverride( "PAD2", g_TestRun.ppad2dll, Config.PAD2 );
	_doPluginOverride( "DEV9", g_TestRun.pdev9dll, Config.DEV9 );


#ifndef _DEBUG
	if( Config.Profiler )
		ProfilerInit();
#endif

	InitCPUTicks();

	while (LoadPlugins() == -1)
	{
		if (Pcsx2Configure(NULL) == FALSE) return;
	}

	if( TestRunMode() ) return;

#ifdef PCSX2_DEVBUILD
	if( g_pRunGSState ) {
		LoadGSState(g_pRunGSState);
		return;
	}
#endif

	if( Config.PsxOut )
	{
		// output the help commands
		Console::SetColor( Console::Color_White );

		Console::WriteLn( "Hotkeys:" );

		Console::WriteLn(
			"\tF1  - save state\n"
			"\t(Shift +) F2 - cycle states\n"
			"\tF3  - load state"
			);

		DevCon::WriteLn(
			//"\tF10 - dump performance counters\n"
			"\tF11 - save GS state\n"
			//"\tF12 - dump hardware registers"
			);
		Console::ClearColor();
	}

	RunGui();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char *lang;
	int i;

	CreateDirectory(LOGS_DIR, NULL);

	InitCommonControls();
	pInstance=hInstance;
	FirstShow=true;			// this is used by cheats.cpp to search for stuff (broken?)

	pthread_win32_process_attach_np();

	gApp.hInstance = hInstance;
	gApp.hMenu = NULL;
	gApp.hWnd = NULL;

#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, "Langs\\");
	textdomain(PACKAGE);
#endif

	memzero_obj(g_TestRun);

	_getcwd( g_WorkingFolder, g_MaxPath );

	int argc;
	TCHAR *const *const argv = _CommandLineToArgv( lpCmdLine, &argc );

	if( argv == NULL )
	{
		Msgbox::Alert( "A fatal error occured while attempting to parse the command line.\n" );
		return 2;
	}

	switch( ParseCommandLine( argc, argv ) )
	{
		case 1:		// display help and exit:
			printf( "%s", phelpmsg );
			MessageBox( NULL, phelpmsg, "Pcsx2 Help", MB_OK);

		case -1:	// exit...
		return 0;
	}

	try
	{
		bool needsToConfig = !LoadConfig();

		// Enumerate available translations

		if (needsToConfig || Config.Lang[0] == 0) {
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

		// automatically and always open the console for first-time uses (no ini file)
		if(needsToConfig || Config.PsxOut )
			Console::Open();

		// Important!  Always allocate dynarecs before loading plugins, to give the dynarecs
		// the best possible chance of claiming ideal memory space!

		SysInit();

		if( needsToConfig )
		{
			// Prompt the user for a valid configuration before starting the program.
			Msgbox::Alert( _( "Pcsx2 needs to be configured." ) );
			Pcsx2Configure( NULL );
			LoadConfig();	// forces re-loading of language and stuff.
		}

		if( Config.PsxOut )
			Console::Open();
		else
			Console::Close();

		WinRun();
	}
	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert(
			"An unhandled or unrecoverable exception occurred, with the message:\n\n"
			"%s"
			"\n\nPcsx2 will now close.  More details may be available via the emuLog.txt file.", params
			ex.cMessage()
			);
	}
	catch( std::exception& ex )
	{
		Msgbox::Alert(
			"An unhandled or unrecoverable exception occurred, with the message:\n\n"
			"%s"
			"\n\nPcsx2 will now close.  More details may be available via the emuLog.txt file.", params
			ex.what()
			);
	}

	WinClose();

	return 0;
}

static std::string str_Default( "default" );

void RunGui()
{
    MSG msg;

	PCSX2_MEM_PROTECT_BEGIN();

	LoadPatch( str_Default );

	do 
	{
		CreateMainWindow();
		m_RestartGui = false;

		while( true )
		{
			if( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0 )
			{
				if( msg.message == WM_QUIT )
				{
					gApp.hWnd = NULL;
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			Sleep(10);
		}
	} while( m_RestartGui );

	PCSX2_MEM_PROTECT_END();
}

BOOL Open_File_Proc( std::string& outstr )
{
	OPENFILENAME ofn;
	char szFileName[ g_MaxPath ];
	char szFileTitle[ g_MaxPath ];
	char * filter = "ELF Files (*.ELF)\0*.ELF\0ALL Files (*.*)\0*.*\0";

	memzero_obj( szFileName );
	memzero_obj( szFileTitle );

	ofn.lStructSize			= sizeof( OPENFILENAME );
	ofn.hwndOwner			= gApp.hWnd;
	ofn.lpstrFilter			= filter;
	ofn.lpstrCustomFilter   = NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= szFileName;
	ofn.nMaxFile			= g_MaxPath;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrFileTitle		= szFileTitle;
	ofn.nMaxFileTitle		= g_MaxPath;
	ofn.lpstrTitle			= NULL;
	ofn.lpstrDefExt			= "ELF";
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;
	 
	if (GetOpenFileName(&ofn)) {
		struct stat buf;

		if (stat(szFileName, &buf) != 0) {
			return FALSE;
		}

		outstr = szFileName;
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

				strcpy_s(args, 256, tmp);
                
                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}


#ifdef PCSX2_DEVBUILD

BOOL APIENTRY LogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int i;
    switch (message) {
        case WM_INITDIALOG:
			for (i=0; i<32; i++)
				if (varLog & (1<<i))
					CheckDlgButton(hDlg, IDC_CPULOG+i, TRUE);

            return TRUE;

        case WM_COMMAND:

            if (LOWORD(wParam) == IDOK) {
				for (i=0; i<32; i++) {
	 			    int ret = Button_GetCheck(GetDlgItem(hDlg, IDC_CPULOG+i));
					if (ret) varLog|= 1<<i;
					else varLog&=~(1<<i);
				}

				SaveConfig();              

                EndDialog(hDlg, TRUE);
				return FALSE;
            } 
            return TRUE;
    }

    return FALSE;
}

#endif

BOOL APIENTRY GameFixes(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:
			if(Config.GameFixes & 0x1) CheckDlgButton(hDlg, IDC_GAMEFIX2, TRUE);//Tri-Ace fix
			if(Config.GameFixes & 0x4) CheckDlgButton(hDlg, IDC_GAMEFIX3, TRUE);//Tekken 5 fix
			if(Config.GameFixes & 0x8) CheckDlgButton(hDlg, IDC_GAMEFIX7, TRUE);//ICO fix
		return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
			{
				uint newfixes = 0;
				newfixes |= IsDlgButtonChecked(hDlg, IDC_GAMEFIX2) ? 0x1 : 0;
				newfixes |= IsDlgButtonChecked(hDlg, IDC_GAMEFIX3) ? 0x4 : 0;
				newfixes |= IsDlgButtonChecked(hDlg, IDC_GAMEFIX7) ? 0x8 : 0;
				
				EndDialog(hDlg, TRUE);

				if( newfixes != Config.GameFixes )
				{
					Config.GameFixes = newfixes;
					SysRestorableReset();
					SaveConfig();
				}
				return FALSE;
            } 
			else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
				return FALSE;
            }
		return TRUE;
    }

    return FALSE;
}

LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CREATE:
			return TRUE;

		case WM_PAINT:
		{
			BITMAP bm;
			PAINTSTRUCT ps;

			HDC hdc = BeginPaint(gApp.hWnd, &ps);

			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbitmap_background);

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
			case ID_GAMEFIXES:
				 DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_GAMEFIXES), hWnd, (DLGPROC)GameFixes);
				 break;

			case ID_HACKS:
				 DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_HACKS), hWnd, (DLGPROC)HacksProc);
				 break;

			case ID_ADVANCED_OPTIONS:
				 DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS), hWnd, (DLGPROC)AdvancedOptionsProc);
				 break;

			case ID_CHEAT_FINDER_SHOW:
				ShowFinder(pInstance,hWnd);
				break;

			case ID_CHEAT_BROWSER_SHOW:
				ShowCheats(pInstance,hWnd);
				break;

			case ID_FILE_EXIT:
				DestroyWindow( hWnd );
				// WM_DESTROY will do the shutdown work for us.
				break;

			case ID_FILEOPEN:
			{
				std::string outstr;
				if( Open_File_Proc( outstr ) )
					RunExecute( outstr.c_str() );
			}
			break;

			case ID_RUN_EXECUTE:
				if( g_EmulationInProgress )
					ExecuteCpu();
				else
					RunExecute( NULL, true );	// boots bios if no savestate is to be recovered
			break;

			case ID_FILE_RUNCD:
				SysReset();
				RunExecute( NULL );
			break;

			case ID_RUN_RESET:
				SysReset();
			break;

			//2002-09-20 (Florin)
			case ID_RUN_CMDLINE:
				if( IsDevBuild )
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CMDLINE), hWnd, (DLGPROC)CmdlineProc);
				break;
			//-------------------
           	case ID_PATCHBROWSER:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_PATCHBROWSER), hWnd, (DLGPROC)PatchBDlgProc);
				break;

			case ID_CONFIG_CONFIGURE:
				Pcsx2Configure(hWnd);

				// Configure may unload plugins if the user changes settings, so reload
				// them here.  If they weren't unloaded these functions do nothing.
				LoadPlugins();
				break;

			case ID_CONFIG_GRAPHICS:
				if (GSconfigure) GSconfigure();
				break;

			case ID_CONFIG_CONTROLLERS:
				if (PAD1configure) PAD1configure();
				if (PAD2configure) {
					if (strcmp(Config.PAD1, Config.PAD2))PAD2configure();
				}
				break;

			case ID_CONFIG_SOUND:
				if (SPU2configure) SPU2configure();
				break;

			case ID_CONFIG_CDVDROM:
				if (CDVDconfigure) CDVDconfigure();
				break;

			case ID_CONFIG_DEV9:
				if (DEV9configure) DEV9configure();
				break;

			case ID_CONFIG_USB:
				if (USBconfigure) USBconfigure();
				break;
  
			case ID_CONFIG_FW:
				if (FWconfigure) FWconfigure();
				break;

			case ID_FILE_STATES_LOAD_SLOT1: 
			case ID_FILE_STATES_LOAD_SLOT2: 
			case ID_FILE_STATES_LOAD_SLOT3: 
			case ID_FILE_STATES_LOAD_SLOT4: 
			case ID_FILE_STATES_LOAD_SLOT5:
				States_Load(LOWORD(wParam) - ID_FILE_STATES_LOAD_SLOT1);
			break;

			case ID_FILE_STATES_LOAD_OTHER:
				OnStates_LoadOther();
			break;

			case ID_FILE_STATES_SAVE_SLOT1: 
			case ID_FILE_STATES_SAVE_SLOT2: 
			case ID_FILE_STATES_SAVE_SLOT3: 
			case ID_FILE_STATES_SAVE_SLOT4: 
			case ID_FILE_STATES_SAVE_SLOT5: 
				States_Save(LOWORD(wParam) - ID_FILE_STATES_SAVE_SLOT1);
			break;

			case ID_FILE_STATES_SAVE_OTHER:
				OnStates_SaveOther();
			break;

			case ID_CONFIG_CPU:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CPUDLG), hWnd, (DLGPROC)CpuDlgProc);
				break;

#ifdef PCSX2_DEVBUILD
			case ID_DEBUG_ENTERDEBUGGER:
                DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_DEBUG), NULL, (DLGPROC)DebuggerProc);
                break;

			case ID_DEBUG_REMOTEDEBUGGING:
				//read debugging params
				if (Config.Options & PCSX2_EEREC){
					MessageBox(hWnd, _("Nah, you have to be in\nInterpreter Mode to debug"), 0, 0);
				} else 
				{
					int remoteDebugBios=0;
					remoteDebugBios=DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RDEBUGPARAMS), NULL, (DLGPROC)RemoteDebuggerParamsProc);
					if (remoteDebugBios)
					{
						cpuReset();
						SysResetExecutionState();
						cpuExecuteBios();

						DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RDEBUG), NULL, (DLGPROC)RemoteDebuggerProc);
						//CreateMainWindow(SW_SHOWNORMAL);
						//RunGui();
					}
				}
                break;

			case ID_DEBUG_MEMORY_DUMP:
			    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MEMORY), hWnd, (DLGPROC)MemoryProc);
				break;

			case ID_DEBUG_LOGGING:
			    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_LOGGING), hWnd, (DLGPROC)LogProc);
				break;
#endif

			case ID_HELP_ABOUT:
				DialogBox(gApp.hInstance, MAKEINTRESOURCE(ABOUT_DIALOG), hWnd, (DLGPROC)AboutDlgProc);
				break;

			case ID_HELP_HELP:
				ShellExecute( hWnd, "open", "http://www.pcsx2.net", NULL, NULL, SW_SHOWNORMAL );
				//system("compat_list\\compat_list.html");
				break;

			case ID_CONFIG_MEMCARDS:
				MemcardConfig::OpenDialog();
				break;

			case ID_PROCESSLOW: 
               Config.ThPriority = THREAD_PRIORITY_LOWEST;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_UNCHECKED);
                break;
                
			case ID_PROCESSNORMAL:
                Config.ThPriority = THREAD_PRIORITY_NORMAL;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_UNCHECKED);
                break;

			case ID_PROCESSHIGH:
                Config.ThPriority = THREAD_PRIORITY_HIGHEST;
                SaveConfig();
				CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_CHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_UNCHECKED);
                CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_UNCHECKED);
                break;

			case ID_CONSOLE:
				Config.PsxOut = !Config.PsxOut;
				if(Config.PsxOut)
				{
                   CheckMenuItem(gApp.hMenu,ID_CONSOLE,MF_CHECKED);
				   Console::Open();
				}
				else
				{
                   CheckMenuItem(gApp.hMenu, ID_CONSOLE, MF_UNCHECKED);
				   Console::Close();
				}
				SaveConfig();
				break;

			case ID_PATCHES:
				Config.Patch = !Config.Patch;
				CheckMenuItem(gApp.hMenu, ID_PATCHES, Config.Patch ? MF_CHECKED : MF_UNCHECKED);
				SaveConfig();
				break;

			case ID_CDVDPRINT:
				Config.cdvdPrint = !Config.cdvdPrint;
				CheckMenuItem(gApp.hMenu, ID_CDVDPRINT, Config.cdvdPrint ? MF_CHECKED : MF_UNCHECKED);
				SaveConfig();
				break;

			case ID_CLOSEGS:
				Config.closeGSonEsc = !Config.closeGSonEsc;
				CheckMenuItem(gApp.hMenu, ID_CLOSEGS, Config.closeGSonEsc ? MF_CHECKED : MF_UNCHECKED);
				SaveConfig();
				break;

#ifndef _DEBUG
			case ID_PROFILER:
				Config.Profiler = !Config.Profiler;
				if( Config.Profiler )
				{
					CheckMenuItem(gApp.hMenu,ID_PROFILER,MF_CHECKED);
					ProfilerInit();
				}
				else
				{
					CheckMenuItem(gApp.hMenu,ID_PROFILER,MF_UNCHECKED);
					ProfilerTerm();
				}
				SaveConfig();
				break;
#endif

			default:
				if (LOWORD(wParam) >= ID_LANGS && LOWORD(wParam) <= (ID_LANGS + langsMax))
				{
					m_RestartGui = true;
					DestroyWindow(gApp.hWnd);
					ChangeLanguage(langs[LOWORD(wParam) - ID_LANGS].lang);
					break;
				}
				return TRUE;
			}
		return FALSE;

		case WM_DESTROY:
			if( hbitmap_background != NULL )
			{
				DeleteObject( hbitmap_background );
				hbitmap_background = NULL;
			}
			gApp.hWnd = NULL;
		break;

		case WM_NCDESTROY:
			PostQuitMessage(0);
		break;


		// Explicit handling of WM_CLOSE.
		// This is Windows default behavior, but we handle it here sot hat we might add a
		// user confirmation prompt prior to exit (someday!)
		case WM_CLOSE:
			DestroyWindow( hWnd );
		return TRUE;

        case WM_SYSCOMMAND:
            if( nDisableSC && (wParam== SC_SCREENSAVE || wParam == SC_MONITORPOWER) ) {
               return FALSE;
            }
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
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

// fixme - this looks like the beginnings of a dynamic "list of valid saveslots"
// feature.  Too bad it's never called and CheckState was old/dead code.
/*void UpdateMenuSlots() {
	char str[g_MaxPath];
	int i;

	for (i=0; i<5; i++) {
		sprintf_s (str, g_MaxPath, "sstates\\%8.8X.%3.3d", ElfCRC, i);
		Slots[i] = CheckState(str);
	}
}*/


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
	ADDMENUITEM(2, _("Slot &4"), ID_FILE_STATES_LOAD_SLOT5);
	ADDMENUITEM(2, _("Slot &3"), ID_FILE_STATES_LOAD_SLOT4);
	ADDMENUITEM(2, _("Slot &2"), ID_FILE_STATES_LOAD_SLOT3);
	ADDMENUITEM(2, _("Slot &1"), ID_FILE_STATES_LOAD_SLOT2);
	ADDMENUITEM(2, _("Slot &0"), ID_FILE_STATES_LOAD_SLOT1);
	ADDMENUITEM(3, _("&Other..."), ID_FILE_STATES_SAVE_OTHER);
	ADDMENUITEM(3, _("Slot &4"), ID_FILE_STATES_SAVE_SLOT5);
	ADDMENUITEM(3, _("Slot &3"), ID_FILE_STATES_SAVE_SLOT4);
	ADDMENUITEM(3, _("Slot &2"), ID_FILE_STATES_SAVE_SLOT3);
	ADDMENUITEM(3, _("Slot &1"), ID_FILE_STATES_SAVE_SLOT2);
	ADDMENUITEM(3, _("Slot &0"), ID_FILE_STATES_SAVE_SLOT1);

    ADDSUBMENU(0, _("&Run"));

    ADDSUBMENUS(0, 1, _("&Process Priority"));
	ADDMENUITEM(1, _("&Low"), ID_PROCESSLOW );
	ADDMENUITEM(1, _("High"), ID_PROCESSHIGH);
	ADDMENUITEM(1, _("Normal"), ID_PROCESSNORMAL);
	if( IsDevBuild )
		ADDMENUITEM(0,_("&Arguments"), ID_RUN_CMDLINE);
	ADDMENUITEM(0,_("Re&set"), ID_RUN_RESET);
	ADDMENUITEM(0,_("E&xecute"), ID_RUN_EXECUTE);

	ADDSUBMENU(0,_("&Config"));
	ADDMENUITEM(0,_("Advanced"), ID_ADVANCED_OPTIONS);
	ADDMENUITEM(0,_("Speed &Hacks"), ID_HACKS);
	ADDMENUITEM(0,_("Gamefixes"), ID_GAMEFIXES);
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
	ADDMENUITEM(0,_("Print cdvd &Info"), ID_CDVDPRINT);
	ADDMENUITEM(0,_("Close GS Window on Esc"), ID_CLOSEGS);
	ADDSEPARATOR(0);
#ifndef _DEBUG
	ADDMENUITEM(0,_("Enable &Profiler"), ID_PROFILER);
#endif
	ADDMENUITEM(0,_("Enable &Patches"), ID_PATCHES);
	ADDMENUITEM(0,_("Enable &Console"), ID_CONSOLE); 
	ADDSEPARATOR(0);
	ADDMENUITEM(0,_("Patch &Finder..."), ID_CHEAT_FINDER_SHOW); 
	ADDMENUITEM(0,_("Patch &Browser..."), ID_CHEAT_BROWSER_SHOW); 


    ADDSUBMENU(0, _("&Help"));
	ADDMENUITEM(0,_("&Pcsx2 Website..."), ID_HELP_HELP);
	ADDMENUITEM(0,_("&About..."), ID_HELP_ABOUT);

	if( !IsDevBuild )
		EnableMenuItem(GetSubMenu(gApp.hMenu, 4), ID_DEBUG_LOGGING, MF_GRAYED);
}

void CreateMainWindow()
{
	WNDCLASS wc;
	HWND hWnd;
	char buf[256];
	char COMPILER[20]="";
	BITMAP bm;
	RECT rect;
	int w, h;

	g_ReturnToGui = true;

#ifdef _MSC_VER
	sprintf(COMPILER, "(VC%d)", (_MSC_VER+100)/200);//hacky:) works for VC6 & VC.NET
#elif __BORLANDC__
	sprintf(COMPILER, "(BC)");
#endif
	/* Load Background Bitmap from the ressource */
	if( hbitmap_background == NULL )
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

#ifdef PCSX2_DEVBUILD
	sprintf(buf, _("PCSX2 %s - Compile Date - %s %s"), PCSX2_VERSION, COMPILEDATE, COMPILER);
#else
	sprintf(buf, _("PCSX2 %s"), PCSX2_VERSION);
#endif

	hWnd = CreateWindow(
		"PCSX2 Main",
		buf, WS_OVERLAPPED | WS_SYSMENU,
		20, 20, 320, 240,
		NULL, NULL,
		gApp.hInstance, NULL
	);

	gApp.hWnd = hWnd;
    ResetMenuSlots();
	CreateMainMenu();
   
	SetMenu(gApp.hWnd, gApp.hMenu);
    if(Config.ThPriority==THREAD_PRIORITY_NORMAL) CheckMenuItem(gApp.hMenu,ID_PROCESSNORMAL,MF_CHECKED);
	if(Config.ThPriority==THREAD_PRIORITY_HIGHEST) CheckMenuItem(gApp.hMenu,ID_PROCESSHIGH,MF_CHECKED);
	if(Config.ThPriority==THREAD_PRIORITY_LOWEST)  CheckMenuItem(gApp.hMenu,ID_PROCESSLOW,MF_CHECKED);
	if(Config.PsxOut)	CheckMenuItem(gApp.hMenu,ID_CONSOLE,MF_CHECKED);
	if(Config.Patch)	CheckMenuItem(gApp.hMenu,ID_PATCHES,MF_CHECKED);
	if(Config.Profiler)	CheckMenuItem(gApp.hMenu,ID_PROFILER,MF_CHECKED);
	if(Config.cdvdPrint)CheckMenuItem(gApp.hMenu,ID_CDVDPRINT,MF_CHECKED);
	if(Config.closeGSonEsc)CheckMenuItem(gApp.hMenu,ID_CLOSEGS,MF_CHECKED);

	hStatusWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "", hWnd, 100);

	w = bm.bmWidth; h = bm.bmHeight;
	GetWindowRect(hStatusWnd, &rect);
	h+= rect.bottom - rect.top;
	GetMenuItemRect(hWnd, gApp.hMenu, 0, &rect);
	h+= rect.bottom - rect.top;
	MoveWindow(hWnd, 60, 60, w, h, TRUE);
	SendMessage( hStatusWnd, WM_SIZE, 0, 0 );

	StatusBar_SetMsg("F1 - save, F2 - next state, Shift+F2 - prev state, F3 - load, F8 - snapshot");

	ShowWindow(hWnd, true);
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
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
	strcpy_s(Config.Lang, lang);
	SaveConfig();
	LoadConfig();
}
