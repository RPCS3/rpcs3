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

#pragma once

#include "Paths.h"
#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"		// to use threading stuff, include the Threading namespace in your file.
#include "Misc.h"

extern bool SysInit();
extern void SysDetect();				// Detects cpu type and fills cpuInfo structs.
extern void SysReset();					// Resets the various PS2 cpus, sub-systems, and recompilers.
extern void SysUpdate();				// Called on VBlank (to update i.e. pads)

extern bool SysAllocateMem();			// allocates memory for all PS2 systems; returns FALSe on critical error.
extern void SysAllocateDynarecs();		// allocates memory for all dynarecs, and force-disables any failures.
extern void SysShutdownDynarecs();
extern void SysShutdownMem();

extern void SysRestorableReset();		// Saves the current emulation state prior to spu reset.
extern void SysClearExecutionCache();	// clears recompiled execution caches!
extern void SysEndExecution();		// terminates plugins, saves GS state (if enabled), and signals emulation loop to end.
extern void SysPrepareExecution( const wxString& elf_file, bool use_bios=false );

// initiates high-speed execution of the emulation state.  This function is currently
// designed to be run from an event loop, but will eventually be re-tooled with threading
// in mindunder the new GUI (assuming Linux approves!), which will make life easier *and*
// improve overall performance too!
extern void SysExecute();


// Maps a block of memory for use as a recompiled code buffer, and ensures that the
// allocation is below a certain memory address (specified in "bounds" parameter).
// The allocated block has code execution privileges.
// Returns NULL on allocation failure.
extern u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller="Unnamed");
extern void vSyncDebugStuff( uint frame );

#ifdef __LINUX__

	extern void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * );
	extern void __fastcall InstallLinuxExceptionHandler();
	extern void __fastcall ReleaseLinuxExceptionHandler();
	static void NTFS_CompressFile( const wxString& file, bool compressStatus=true ) {}

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

//////////////////////////////////////////////////////////////////////////////////////////
// Different types of message boxes that the emulator can employ from the friendly confines
// of it's blissful unawareness of whatever GUI it runs under. :)  All message boxes exhibit
// blocking behavior -- they prompt the user for action and only return after the user has
// responded to the prompt.
//
namespace Msgbox
{
	// Pops up an alert Dialog Box with a singular "OK" button.
	// Always returns false.  Replacement for SysMessage.
	extern bool Alert( const wxString& text );

	// Pops up a dialog box with Ok/Cancel buttons.  Returns the result of the inquiry,
	// true if OK, false if cancel.
	extern bool OkCancel( const wxString& text );
}

