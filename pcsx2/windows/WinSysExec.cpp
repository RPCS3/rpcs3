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

#include "Win32.h"
#include <winnt.h>

#include "Common.h"
#include "VUmicro.h"

#include "iR5900.h"

static bool sinit = false;
bool UseGui = true;
bool nDisableSC = false; // screensaver
bool renderswitch = 0;


// This instance is not modified by command line overrides so
// that command line plugins and stuff won't be saved into the
// user's conf file accidentally.
PcsxConfig winConfig;		// local storage of the configuration options.

HWND hStatusWnd = NULL;
AppData gApp;

const char* g_pRunGSState = NULL;

#define CmdSwitchIs( text ) ( stricmp( command, text ) == 0 )

int SysPageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	const _EXCEPTION_RECORD& ExceptionRecord = *eps->ExceptionRecord;
	
	if (ExceptionRecord.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// get bad virtual address
	u32 offset = (u8*)ExceptionRecord.ExceptionInformation[1]-psM;

	if (offset>=Ps2MemSize::Base)
		return EXCEPTION_CONTINUE_SEARCH;

	mmap_ClearCpuBlock( offset );

	return EXCEPTION_CONTINUE_EXECUTION;
}


// returns 1 if the user requested help (show help and exit)
// returns zero on success.
// returns -1 on failure (bad command line argument)
int ParseCommandLine( int tokenCount, TCHAR *const *const tokens )
{
	int tidx = 0;
	g_TestRun.efile = 0;

	while( tidx < tokenCount )
	{
		const TCHAR* command = tokens[tidx++];

		if( command[0] != '-' )
		{
			g_TestRun.ptitle = command;
            printf("opening file %s\n", command);
			continue;
		}

		// jump past the '-' switch char, and skip if this is a dud switch:
		command++;
		if( command[0] == 0 ) continue;

		if( CmdSwitchIs( "help" ) )
		{
			return -1;
		}
        else if( CmdSwitchIs( "nogui" ) ) {
			UseGui = false;
		}
        else if( CmdSwitchIs( "highpriority" ) ) {
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		}		
#ifdef PCSX2_DEVBUILD
			else if( CmdSwitchIs( "jpg" ) ) {
				g_TestRun.jpgcapture = 1;
			}
#endif
		else
		{
			const TCHAR* param;
			if( tidx >= tokenCount ) break;

			// None of the parameter-less toggles flagged.
			// Check for switches that require one or more parameters here:

			param = tokens[tidx++];

			if( CmdSwitchIs( "cfg" ) ) {
				g_CustomConfigFile = param;
			}

			else if( CmdSwitchIs( "efile" ) ) {
				g_TestRun.efile = !!atoi( param );
			}
			else if( CmdSwitchIs( "loadgs" ) ) {
				g_pRunGSState = param;
			}

			// Options to configure plugins:

			else if( CmdSwitchIs( "gs" ) ) {
				g_TestRun.pgsdll = param;
			}
			else if( CmdSwitchIs( "cdvd" ) ) {
				g_TestRun.pcdvddll = param;
			}
			else if( CmdSwitchIs( "spu" ) ) {
				g_TestRun.pspudll = param;
			}
			else if( CmdSwitchIs( "pads" ) ) {
				g_TestRun.ppad1dll = param;
				g_TestRun.ppad2dll = param;
			}
			else if( CmdSwitchIs( "pad1" ) ) {
				g_TestRun.ppad1dll = param;
			}
			else if( CmdSwitchIs( "pad2" ) ) {
				g_TestRun.ppad2dll = param;
			}
			else if( CmdSwitchIs( "dev9" ) ) {
				g_TestRun.pdev9dll = param;
			}

#ifdef PCSX2_DEVBUILD
			else if( CmdSwitchIs( "image" ) ) {
				g_TestRun.pimagename = param;
			}
			else if( CmdSwitchIs( "log" ) ) {
				g_TestRun.plogname = param;
			}
			else if( CmdSwitchIs( "logopt" ) ) {
				if( param[0] == '0' && param[1] == 'x' ) param += 2;
				sscanf(param, "%x", &varLog);
			}
			else if( CmdSwitchIs( "frame" ) ) {
				g_TestRun.frame = atoi( param );
			}
			else if( CmdSwitchIs( "numimages" ) ) {
				g_TestRun.numimages = atoi( param );
			}
#endif
		}
	}
	return 0;
}

void SysPrintf(const char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list,fmt);
	vsprintf_s(msg,fmt,list);
	msg[511] = '\0';
	va_end(list);

	Console::Write( msg );
}

void OnStates_LoadOther()
{
	OPENFILENAME ofn;
	char szFileName[g_MaxPath];
	char szFileTitle[g_MaxPath];
	char szFilter[g_MaxPath];

	memzero_obj( szFileName );
	memzero_obj( szFileTitle );

	strcpy(szFilter, _("PCSX2 State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= g_MaxPath;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= g_MaxPath;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "EXE";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) 
		States_Load( szFileName );
} 

void OnStates_SaveOther()
{
	OPENFILENAME ofn;
	char szFileName[g_MaxPath];
	char szFileTitle[g_MaxPath];
	char szFilter[g_MaxPath];

	memzero_obj( szFileName );
	memzero_obj( szFileTitle );

	strcpy(szFilter, _("PCSX2 State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= g_MaxPath;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= g_MaxPath;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "EXE";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn))
		States_Save( szFileName );
}

bool HostGuiInit()
{
	if( sinit ) return true;
	sinit = true;

	CreateDirectory(MEMCARDS_DIR, NULL);
	CreateDirectory(SSTATES_DIR, NULL);
	CreateDirectory(SNAPSHOTS_DIR, NULL);

	// Set the compression attribute on the Memcards folder.
	// Memcards generally compress very well via NTFS compression.
	
	NTFS_CompressFile( MEMCARDS_DIR, Config.McdEnableNTFS );

	if( IsDevBuild && emuLog == NULL && g_TestRun.plogname != NULL )
		emuLog = fopen(g_TestRun.plogname, "w");

	if( emuLog == NULL )
		emuLog = fopen(LOGS_DIR "\\emuLog.txt","w");

	PCSX2_MEM_PROTECT_BEGIN();
	SysDetect();
	if( !SysAllocateMem() )
		return false;	// critical memory allocation failure;

	SysAllocateDynarecs();
	PCSX2_MEM_PROTECT_END();

	return true;
}


static const char *err = N_("Error Loading Symbol");
static int errval;

namespace HostSys
{
	void *LoadLibrary(const char *lib)
	{
		return LoadLibraryA( lib );
	}

	void *LoadSym(void *lib, const char *sym)
	{
		void *tmp = GetProcAddress((HINSTANCE)lib, sym);
		if (tmp == NULL) errval = GetLastError();
		else errval = 0;
		return tmp;
	}

	const char *LibError()
	{
		if( errval ) 
		{ 
			static char perr[4096];

			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),NULL,perr,4095,NULL);

			errval = 0;
			return _(perr); 
		}
		return NULL;
	}

	void CloseLibrary(void *lib)
	{
		FreeLibrary((HINSTANCE)lib);
	}

	void *Mmap(uptr base, u32 size)
	{
		return VirtualAlloc((void*)base, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}

	void Munmap(uptr base, u32 size)
	{
		if( base == NULL ) return;
		VirtualFree((void*)base, size, MEM_DECOMMIT);
		VirtualFree((void*)base, 0, MEM_RELEASE);
	}

	void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution )
	{
		DWORD winmode = 0;

		switch( mode )
		{
			case Protect_NoAccess:
				winmode = ( allowExecution ) ? PAGE_EXECUTE : PAGE_NOACCESS;
			break;
			
			case Protect_ReadOnly:
				winmode = ( allowExecution ) ? PAGE_EXECUTE_READ : PAGE_READONLY;
			break;
			
			case Protect_ReadWrite:
				winmode = ( allowExecution ) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			break;
		}

		DWORD OldProtect;	// enjoy my uselessness, yo!
		VirtualProtect( baseaddr, size, winmode, &OldProtect );
	}
}

namespace HostGui
{
	// Sets the status bar message without mirroring the output to the console.
	void SetStatusMsg( const string& text )
	{
		// don't try this in GCC folks!
		SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text.c_str() );
	}

	void Notice( const string& text )
	{
		// mirror output to the console!
		Console::Status( text.c_str() );
		SetStatusMsg( text );
	}

	void ResetMenuSlots()
	{
		for( int i=0; i<5; i++ )
		{
			EnableMenuItem( GetSubMenu(gApp.hMenu, 0),
				ID_FILE_STATES_LOAD_SLOT1+i,
				States_isSlotUsed(i) ? MF_ENABLED : MF_GRAYED
			);
		}
	}

	// Because C++ lacks the finally clause >_<
	static void _executeCpuFinally()
	{
		timeEndPeriod( 1 );
		ShowWindow( gApp.hWnd, SW_SHOW );
		SetForegroundWindow( gApp.hWnd );
		nDisableSC = false;
	}

	void BeginExecution()
	{
		nDisableSC = true;

		// ... and hide the window.  Ugly thing.
		ShowWindow( gApp.hWnd, SW_HIDE );
		timeBeginPeriod( 1 );		// improves multithreaded responsiveness

		try
		{
			SysExecute();
		}
		catch( Exception::BaseException& )
		{
			_executeCpuFinally();
			throw;
		}

		_executeCpuFinally();
	}

	void __fastcall KeyEvent( keyEvent* ev )
	{
		static int shiftkey = 0;

		if (ev == NULL) return;
		if (ev->evt == KEYRELEASE)
		{
			switch (ev->key)
			{
				case VK_SHIFT: shiftkey = 0; break;
			}
			GSkeyEvent(ev); return;
		}

		if (ev->evt != KEYPRESS) return;

		switch (ev->key)
		{
			case VK_SHIFT: shiftkey = 1; break;

			case VK_F1: case VK_F2:  case VK_F3:  case VK_F4:
			case VK_F5: case VK_F6:  case VK_F7:  case VK_F8:
			case VK_F9: case VK_F10: case VK_F12: 
				try
				{
					ProcessFKeys(ev->key-VK_F1 + 1, shiftkey);
				}
				catch( Exception::CpuStateShutdown& )
				{
					// Woops!  Something was unrecoverable (like state load).  Bummer.
					// Let's give the user a RunGui!

					g_EmulationInProgress = false;
					SysEndExecution();
				}
			break;
			
			case VK_F11: //gsdx "on the fly" renderer switching 
				if (!renderswitch) {
					StateRecovery::MakeGsOnly();
					g_EmulationInProgress = false;
					ClosePlugins( 1 );
					renderswitch = true;	//go to dx9 sw
					StateRecovery::Recover();
					HostGui::BeginExecution(); //also sets g_EmulationInProgress to true later
				}
				else {
					StateRecovery::MakeGsOnly();
					g_EmulationInProgress = false;
					ClosePlugins( 1 );
					renderswitch = false;	//return to default renderer
					StateRecovery::Recover();
					HostGui::BeginExecution(); //also sets g_EmulationInProgress to true later
				}
				break;
			case VK_TAB:
				CycleFrameLimit(0);
				break;

			case VK_ESCAPE:
#ifdef PCSX2_DEVBUILD
				if( g_SaveGSStream >= 3 ) {
					// gs state
					g_SaveGSStream = 4;
					break;
				}
#endif

				if( CHECK_ESCAPE_HACK )
				{
					g_EmulationInProgress = false;
					DestroyWindow( gApp.hWnd );
				}
				else
				{
					if( !UseGui ) {
						// not using GUI and user just quit, so exit
						WinClose();
					}
					
					nDisableSC = 0;
				}

				SysEndExecution();
			break;

			default:
				GSkeyEvent(ev);
			break;
		}
	}
}