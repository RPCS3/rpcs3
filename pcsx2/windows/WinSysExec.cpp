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

#include "Common.h"
//#include "PsxCommon.h"
#include "VUmicro.h"

#include "iR5900.h"

int UseGui = 1;
int nDisableSC = 0; // screensaver

SafeArray<u8>* g_RecoveryState = NULL;
SafeArray<u8>* g_gsRecoveryState = NULL;


bool g_ReturnToGui = false;			// set to exit the execution of the emulator and return control to the GUI
bool g_EmulationInProgress = false;	// Set TRUE if a game is actively running (set to false on reset)

// This instance is not modified by command line overrides so
// that command line plugins and stuff won't be saved into the
// user's conf file accidentally.
PcsxConfig winConfig;		// local storage of the configuration options.

HWND hStatusWnd = NULL;
AppData gApp;

const char* g_pRunGSState = NULL;


#define CmdSwitchIs( text ) ( stricmp( command, text ) == 0 )

// For issuing notices to both the status bar and the console at the same time.
// Single-line text only please!  Mutli-line msgs should be directed to the
// console directly, thanks.
void StatusBar_Notice( const std::string& text )
{
	// mirror output to the console!
	Console::Status( text.c_str() );

	// don't try this in GCC folks!
	SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text.c_str() );
}

// Sets the status bar message without mirroring the output to the console.
void StatusBar_SetMsg( const std::string& text )
{
	// don't try this in GCC folks!
	SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text.c_str() );
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
			UseGui = 0;
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

static void __fastcall KeyEvent(keyEvent* ev);

__forceinline void SysUpdate() {

    keyEvent* ev1 = PAD1keyEvent();
	keyEvent* ev2 = PAD2keyEvent();

	KeyEvent( (ev1 != NULL) ? ev1 : ev2);
}

static void TryRecoverFromGsState()
{
	if( g_gsRecoveryState != NULL )
	{
		s32 dummylen;

		memLoadingState eddie( *g_gsRecoveryState );
		eddie.FreezePlugin( "GS", gsSafeFreeze );
		eddie.Freeze( dummylen );		// reads the length value recorded earlier.
		eddie.gsFreeze();
	}
}

void ExecuteCpu()
{
	// Make sure any left-over recovery states are cleaned up.
	safe_delete( g_RecoveryState );

	// Just in case they weren't initialized earlier (no harm in calling this multiple times)
	if (OpenPlugins(NULL) == -1) return;

	// this needs to be called for every new game!
	// (note: sometimes launching games through bios will give a crc of 0)

	if( GSsetGameCRC != NULL )
		GSsetGameCRC(ElfCRC, g_ZeroGSOptions);

	TryRecoverFromGsState();

	safe_delete( g_gsRecoveryState );

	// ... and hide the window.  Ugly thing.

	ShowWindow( gApp.hWnd, SW_HIDE );

	g_EmulationInProgress = true;
	g_ReturnToGui = false;

	// Optimization: We hardcode two versions of the EE here -- one for recs and one for ints.
	// This is because recs are performance critical, and being able to inline them into the
	// function here helps a small bit (not much but every small bit counts!).

	timeBeginPeriod( 1 );

	if( CHECK_EEREC )
	{
		while( !g_ReturnToGui )
		{
			recExecute();
			SysUpdate();
		}
	}
	else
	{
		while( !g_ReturnToGui )
		{
			Cpu->Execute();
			SysUpdate();
		}
	}

	timeEndPeriod( 1 );

	ShowWindow( gApp.hWnd, SW_SHOW );
	SetForegroundWindow( gApp.hWnd );
}

// Runs and ELF image directly (ISO or ELF program or BIN)
// Used by Run::FromCD and such
void RunExecute( const char* elf_file, bool use_bios )
{
	SetThreadPriority(GetCurrentThread(), Config.ThPriority);
	SetPriorityClass(GetCurrentProcess(), Config.ThPriority == THREAD_PRIORITY_HIGHEST ? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
    nDisableSC = 1;

	try
	{
		cpuReset();
	}
	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert( ex.cMessage() );
		return;
	}

	if (OpenPlugins(g_TestRun.ptitle) == -1)
		return;

	if( elf_file == NULL )
	{
		if(g_RecoveryState != NULL)
		{
			try
			{
				memLoadingState( *g_RecoveryState ).FreezeAll();
			}
			catch( std::runtime_error& ex )
			{
				Msgbox::Alert(
					"Gamestate recovery failed.  Your game progress will be lost (sorry!)\n"
					"\nError: %s\n", params ex.what() );

				// Take the user back to the GUI...
				safe_delete( g_RecoveryState );
				ClosePlugins();
				return;
			}
		}
		else if( g_gsRecoveryState == NULL )
		{
			// Not recovering a state, so need to execute the bios and load the ELF information.
			// (note: gsRecoveries are done from ExecuteCpu)

			// if the elf_file is null we use the CDVD elf file.
			// But if the elf_file is an empty string then we boot the bios instead.

			char ename[g_MaxPath];
			ename[0] = 0;
			if( !use_bios )
				GetPS2ElfName( ename );

			loadElfFile( ename );
		}
	}
	else
	{
		// Custom ELF specified (not using CDVD).
		// Run the BIOS and load the ELF.

		loadElfFile( elf_file );
	}

	ExecuteCpu();
}

class RecoveryMemSavingState : public memSavingState, Sealed
{
public:
	virtual ~RecoveryMemSavingState() { }
	RecoveryMemSavingState() : memSavingState( *g_RecoveryState )
	{
	}

	void gsFreeze()
	{
		if( g_gsRecoveryState != NULL )
		{
			// just copy the data from src to dst.
			// the normal savestate doesn't expect a length prefix for internal structures,
			// so don't copy that part.
			const u32 pluginlen = *((u32*)g_gsRecoveryState->GetPtr());
			const u32 gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
			memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(pluginlen+8), gslen );
			m_idx += gslen;
		}
		else
			memSavingState::gsFreeze();
	}

	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
	{
		if( (freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL) )
		{
			// Gs data is already in memory, so just copy from src to dest:
			// length of the GS data is stored as the first u32, so use that to run the copy:
			const u32 len = *((u32*)g_gsRecoveryState->GetPtr());
			memcpy( m_memory.GetPtr(m_idx), g_gsRecoveryState->GetPtr(), len+4 );
			m_idx += len+4;
		}
		else
			memSavingState::FreezePlugin( name, freezer );
	}
};

class RecoveryZipSavingState : public gzSavingState, Sealed
{
public:
	virtual ~RecoveryZipSavingState() { }
	RecoveryZipSavingState( const string& filename ) : gzSavingState( filename )
	{
	}

	void gsFreeze()
	{
		if( g_gsRecoveryState != NULL )
		{
			// read data from the gsRecoveryState allocation instead of the GS, since the gs
			// info was invalidated when the plugin was shut down.

			// the normal savestate doesn't expect a length prefix for internal structures,
			// so don't copy that part.

			u32& pluginlen = *((u32*)g_gsRecoveryState->GetPtr(0));
			u32& gslen = *((u32*)g_gsRecoveryState->GetPtr(pluginlen+4));
			gzwrite( m_file, g_gsRecoveryState->GetPtr(pluginlen+4), gslen );
		}
		else
			gzSavingState::gsFreeze();
	}

	void FreezePlugin( const char* name, s32 (CALLBACK* freezer)(int mode, freezeData *data) )
	{
		if( (freezer == gsSafeFreeze) && (g_gsRecoveryState != NULL) )
		{
			// Gs data is already in memory, so just copy from there into the gzip file.
			// length of the GS data is stored as the first u32, so use that to run the copy:
			u32& len = *((u32*)g_gsRecoveryState->GetPtr());
			gzwrite( m_file, g_gsRecoveryState->GetPtr(), len+4 );
		}
		else
			gzSavingState::FreezePlugin( name, freezer );
	}
};

void States_Load( const string& file, int num )
{
	if( !Path::isFile( file ) )
	{
		Console::Notice( "Saveslot %d is empty.", params num );
		return;
	}

	try
	{
		char Text[g_MaxPath];
		gzLoadingState joe( file );		// this'll throw an StateLoadError_Recoverable.

		// Make sure the cpu and plugins are ready to be state-ified!
		cpuReset();
		OpenPlugins( NULL );

		joe.FreezeAll();

		if( num != -1 )
			sprintf (Text, _("*PCSX2*: Loaded State %d"), num);
		else
			sprintf (Text, _("*PCSX2*: Loaded State %s"), file);

		StatusBar_Notice( Text );

		if( GSsetGameCRC != NULL )
			GSsetGameCRC(ElfCRC, g_ZeroGSOptions);
	}
	catch( Exception::StateLoadError_Recoverable& ex)
	{
		if( num != -1 )
			Console::Notice( "Could not load savestate from slot %d.\n\n%s", params num, ex.cMessage() );
		else
			Console::Notice( "Could not load savestate file: %s.\n\n%s", params file, ex.cMessage() );

		// At this point the cpu hasn't been reset, so we can return
		// control to the user safely... (that's why we use a console notice instead of a popup)

		return;
	}
	catch( Exception::BaseException& ex )
	{
		// The emulation state is ruined.  Might as well give them a popup and start the gui.

		string message;

		if( num != -1 )
			ssprintf( message,
				"Encountered an error while loading savestate from slot %d.\n", num );
		else
			ssprintf( message,
				"Encountered an error while loading savestate from file: %s.\n", file );

		if( g_EmulationInProgress )
			message += "Since the savestate load was incomplete, the emulator has been reset.\n";

		message += "\nError: " + ex.Message();

		Msgbox::Alert( message.c_str() );
		SysClose();
		return;
	}

	// Start emulating!
	ExecuteCpu();
}

void States_Save( const string& file, int num )
{
	try
	{
		string text;
		RecoveryZipSavingState( file ).FreezeAll();
		if( num != -1 )
			ssprintf( text, _( "State saved to slot %d" ), num );
		else
			ssprintf( text, _( "State saved to file: %s" ), file );

		StatusBar_Notice( text );
	}
	catch( Exception::BaseException& ex )
	{
		string message;

		if( num != -1 )
			ssprintf( message, "An error occurred while trying to save to slot %d\n", num );
		else
			ssprintf( message, "An error occurred while trying to save to file: %s\n", file );

		message += "Your emulation state has not been saved!\n\nError: " + ex.Message();

		Console::Error( message.c_str() );
	}
}

void States_Load(int num)
{
	States_Load( SaveState::GetFilename( num ), num );
}

void States_Save(int num)
{
	if( g_RecoveryState != NULL )
	{
		// State is already saved into memory, and the emulator (and in-progress flag)
		// have likely been cleared out.  So save from the Recovery buffer instead of
		// doing a "standard" save:

		string text;
		SaveState::GetFilename( text, num );
		gzFile fileptr = gzopen( text.c_str(), "wb" );
		if( fileptr == NULL )
		{
			Msgbox::Alert( _("File permissions error while trying to save to slot %d"), params num );
			return;
		}
		gzwrite( fileptr, &g_SaveVersion, sizeof( u32 ) );
		gzwrite( fileptr, g_RecoveryState->GetPtr(), g_RecoveryState->GetSizeInBytes() );
		gzclose( fileptr );
		return;
	}

	if( !g_EmulationInProgress )
	{
		Msgbox::Alert( "You need to start a game first before you can save it's state." );
		return;
	}

	States_Save( SaveState::GetFilename( num ), num );
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

class JustGsSavingState : public memSavingState, Sealed
{
public:
	virtual ~JustGsSavingState() { }
	JustGsSavingState() : memSavingState( *g_gsRecoveryState )
	{
	}

	// This special override saves the gs info to m_idx+4, and then goes back and
	// writes in the length of data saved.
	void gsFreeze()
	{
		int oldmidx = m_idx;
		m_idx += 4;
		memSavingState::gsFreeze();
		if( IsSaving() )
		{
			s32& len = *((s32*)m_memory.GetPtr( oldmidx ));
			len = (m_idx - oldmidx)-4;
		}
	}
};

static int shiftkey = 0;
static void __fastcall KeyEvent(keyEvent* ev)
{
	if (ev == NULL) return;
	if (ev->evt == KEYRELEASE) {
		switch (ev->key) {
		case VK_SHIFT: shiftkey = 0; break;
		}
		GSkeyEvent(ev); return;
	}
	if (ev->evt != KEYPRESS)
        return;

    //some pad plugins don't give a key released event for shift, so this is needed
    //shiftkey = GetKeyState(VK_SHIFT)&0x8000;
    //Well SSXPad breaks with your code, thats why my code worked and your makes reg dumping impossible
	//So i suggest you fix the plugins that dont.
    
	switch (ev->key) {
		case VK_SHIFT: shiftkey = 1; break;

		case VK_F1: case VK_F2:  case VK_F3:  case VK_F4:
		case VK_F5: case VK_F6:  case VK_F7:  case VK_F8:
		case VK_F9: case VK_F10: case VK_F11: case VK_F12:
			try
			{
				ProcessFKeys(ev->key-VK_F1 + 1, shiftkey);
			}
			catch( Exception::CpuStateShutdown& )
			{
				// Woops!  Something was unrecoverable.  Bummer.
				// Let's give the user a RunGui!

				g_EmulationInProgress = false;
				g_ReturnToGui = true;
			}
		break;

		case VK_ESCAPE:
#ifdef PCSX2_DEVBUILD
			if( g_SaveGSStream >= 3 ) {
				// gs state
				g_SaveGSStream = 4;
				break;
			}
#endif

			g_ReturnToGui = true;
			if( CHECK_ESCAPE_HACK )
			{
				//PostMessage(GetForegroundWindow(), WM_CLOSE, 0, 0);
				DestroyWindow( gApp.hWnd );
			}
			else
			{
				if( !UseGui ) {
					// not using GUI and user just quit, so exit
					WinClose();
				}

				if( Config.closeGSonEsc )
				{
					safe_delete( g_gsRecoveryState );
					safe_delete( g_RecoveryState );
					g_gsRecoveryState = new SafeArray<u8>();
					JustGsSavingState eddie;
					eddie.FreezePlugin( "GS", gsSafeFreeze ) ;
					eddie.gsFreeze();
					PluginsResetGS();
				}

				ClosePlugins();
				nDisableSC = 0;
			}
			break;

		default:
			GSkeyEvent(ev);
			break;
	}
}

static bool sinit=false;


void SysRestorableReset()
{
	// already reset? and saved?
	if( !g_EmulationInProgress ) return;
	if( g_RecoveryState != NULL ) return;

	try
	{
		g_RecoveryState = new SafeArray<u8>( "Memory Savestate Recovery" );
		RecoveryMemSavingState().FreezeAll();
		safe_delete( g_gsRecoveryState );
		g_EmulationInProgress = false;
	}
	catch( Exception::RuntimeError& ex )
	{
		Msgbox::Alert(
			"Pcsx2 gamestate recovery failed. Some options may have been reverted to protect your game's state.\n"
			"Error: %s", params ex.cMessage() );
		safe_delete( g_RecoveryState );
	}
}

void SysReset()
{
	if (!sinit) return;

	// fixme - this code  sets the statusbar but never returns control to the window message pump
	// so the status bar won't receive the WM_PAINT messages needed to update itself anyway.
	// Oops! (air)

	StatusBar_Notice(_("Resetting..."));
	Console::SetTitle(_("Resetting..."));

	g_EmulationInProgress = false;
	safe_delete( g_RecoveryState );
	safe_delete( g_gsRecoveryState );
	ResetPlugins();

	ElfCRC = 0;

	// Note : No need to call cpuReset() here.  It gets called automatically before the
	// emulator resumes execution.

	StatusBar_Notice(_("Ready"));
	Console::SetTitle(_("*PCSX2* Emulation state is reset."));
}

bool SysInit()
{
	if( sinit ) return true;
	sinit = true;

	CreateDirectory(MEMCARDS_DIR, NULL);
	CreateDirectory(SSTATES_DIR, NULL);

	// Set the compression attribute on the Memcards folder.
	// Memcards generally compress very well via NTFS compression.
	
	NTFS_CompressFile( MEMCARDS_DIR );

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

// completely shuts down the emulator's cpu state, and unloads all plugins from memory.
void SysClose() {
	if (!sinit) return;
	cpuShutdown();
	ClosePlugins();
	ReleasePlugins();
	sinit=false;
}


static const char *err = N_("Error Loading Symbol");
static int errval;

void *SysLoadLibrary(const char *lib) {
	return LoadLibrary(lib);
}

void *SysLoadSym(void *lib, const char *sym) {
	void *tmp = GetProcAddress((HINSTANCE)lib, sym);
	if (tmp == NULL) errval = GetLastError();
	else errval = 0;
	return tmp;
}

const char *SysLibError() {
	if( errval ) 
	{ 
		static char perr[4096];

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),NULL,perr,4095,NULL);

		errval = 0;
		return _(perr); 
	}
	return NULL;
}

void SysCloseLibrary(void *lib) {
	FreeLibrary((HINSTANCE)lib);
}

void *SysMmap(uptr base, u32 size) {
	void *mem;

	mem = VirtualAlloc((void*)base, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	return mem;
}

void SysMunmap(uptr base, u32 size)
{
	if( base == NULL ) return;
	VirtualFree((void*)base, size, MEM_DECOMMIT);
	VirtualFree((void*)base, 0, MEM_RELEASE);
}
