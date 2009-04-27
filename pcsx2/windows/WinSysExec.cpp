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

// temporary hack to keep this code compiling.
//static const char* _( const char* src ) { return src; }


static bool sinit = false;
bool nDisableSC = false; // screensaver

// This instance is not modified by command line overrides so
// that command line plugins and stuff won't be saved into the
// user's conf file accidentally.
PcsxConfig winConfig;		// local storage of the configuration options.

HWND hStatusWnd = NULL;
AppData gApp;

const char* g_pRunGSState = NULL;

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

static void CreateDirs()
{
	Path::CreateDirectory(MEMCARDS_DIR);
	Path::CreateDirectory(SSTATES_DIR);
	Path::CreateDirectory(SNAPSHOTS_DIR);
}

/*
bool HostGuiInit()
{
	if( sinit ) return true;
	sinit = true;

	CreateDirs();

	// Set the compression attribute on the Memcards folder.
	// Memcards generally compress very well via NTFS compression.
	
	NTFS_CompressFile( MEMCARDS_DIR, Config.McdEnableNTFS );

#ifdef OLD_TESTBUILD_STUFF
	if( IsDevBuild && emuLog == NULL && g_TestRun.plogname != NULL )
		emuLog = fopen(g_TestRun.plogname, "w");
#endif
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
*/

namespace HostSys
{
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
