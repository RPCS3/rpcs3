/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"		// to use threading stuff, include the Threading namespace in your file.
#include "Misc.h"
#include "CDVD/CDVDaccess.h"

static const int PCSX2_VersionHi	= 0;
static const int PCSX2_VersionMid	= 9;
static const int PCSX2_VersionLo	= 7;


class CoreEmuThread;

extern bool SysInit();
extern void SysDetect();				// Detects cpu type and fills cpuInfo structs.
extern void SysReset();					// Resets the various PS2 cpus, sub-systems, and recompilers.

extern void SysExecute( CoreEmuThread* newThread );
extern void SysExecute( CoreEmuThread* newThread, CDVD_SourceType cdvdsrc );
extern void SysEndExecution();

extern void SysSuspend();
extern void SysResume();

extern bool SysAllocateMem();			// allocates memory for all PS2 systems; returns FALSe on critical error.
extern void SysAllocateDynarecs();		// allocates memory for all dynarecs, and force-disables any failures.
extern void SysShutdownDynarecs();
extern void SysShutdownMem();
extern void SysShutdown();

extern void SysLoadState( const wxString& file );
extern void SysRestorableReset();		// Saves the current emulation state prior to spu reset.
extern void SysClearExecutionCache();	// clears recompiled execution caches!


extern u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller="Unnamed");
extern void vSyncDebugStuff( uint frame );

extern CoreEmuThread*	g_EmuThread;

//////////////////////////////////////////////////////////////////////////////////////////
//
#ifdef __LINUX__

#	include <signal.h>

	extern void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * );
	extern void __fastcall InstallLinuxExceptionHandler();
	extern void __fastcall ReleaseLinuxExceptionHandler();
	static void __unused NTFS_CompressFile( const wxString& file, bool compressStatus=true ) {}

#	define PCSX2_MEM_PROTECT_BEGIN()	InstallLinuxExceptionHandler()
#	define PCSX2_MEM_PROTECT_END()		ReleaseLinuxExceptionHandler()

#elif defined( _WIN32 )

	extern int SysPageFaultExceptionFilter(EXCEPTION_POINTERS* eps);
	extern void NTFS_CompressFile( const wxString& file, bool compressStatus=true );

#	define PCSX2_MEM_PROTECT_BEGIN()	__try {
#	define PCSX2_MEM_PROTECT_END()		} __except(SysPageFaultExceptionFilter(GetExceptionInformation())) {}

#else
#	error PCSX2 - Unsupported operating system platform.
#endif

class pxMessageBoxEvent;

//////////////////////////////////////////////////////////////////////////////////////////
// Different types of message boxes that the emulator can employ from the friendly confines
// of it's blissful unawareness of whatever GUI it runs under. :)  All message boxes exhibit
// blocking behavior -- they prompt the user for action and only return after the user has
// responded to the prompt.
//
namespace Msgbox
{
	extern void OnEvent( pxMessageBoxEvent& evt );

	extern bool Alert( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=wxICON_EXCLAMATION );
	extern bool OkCancel( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=0 );
	extern bool YesNo( const wxString& text, const wxString& caption=L"PCSX2 Message", int icon=wxICON_QUESTION );

	extern int  Assertion( const wxString& text, const wxString& stacktrace );
	extern void Except( const Exception::BaseException& src );
}

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEVT_MSGBOX, -1 )
	DECLARE_EVENT_TYPE( pxEVT_CallStackBox, -1 )
END_DECLARE_EVENT_TYPES()
