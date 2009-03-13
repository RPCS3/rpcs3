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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "PS2Etypes.h"
#include "Pcsx2Config.h"
#include "Exceptions.h"
#include "Paths.h"
#include "MemcpyFast.h"
#include "SafeArray.h"
#include "Misc.h"


enum PageProtectionMode
{
	Protect_NoAccess = 0,
	Protect_ReadOnly,
	Protect_ReadWrite
};

//////////////////////////////////////////////////////////////////////////////////////////
// HostSys - Namespace housing general system-level implementations relating to loading
// plugins and allocating memory.  For now, these functions are all accessed via Sys*
// versions defined in System.h/cpp.
namespace HostSys
{
	// Damn windows.h namespace pollution!!
	#undef LoadLibrary

	extern void *LoadLibrary(const char *lib);	// Loads Library
	extern void *LoadSym(void *lib, const char *sym);	// Loads Symbol from Library
	extern const char *LibError();				// Gets previous error loading symbols
	extern void CloseLibrary(void *lib);		// Closes Library

	// Maps a block of memory for use as a recompiled code buffer.
	// The allocated block has code execution privileges.
	// Returns NULL on allocation failure.
	extern void *Mmap(uptr base, u32 size);

	// Unmaps a block allocated by SysMmap
	extern void Munmap(uptr base, u32 size);

	extern void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution=false );
	
	static __forceinline void Munmap( void* base, u32 size )
	{
		Munmap( (uptr)base, size );
	}
}


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
extern void SysPrepareExecution( const char* elf_file, bool use_bios=false );

// initiates high-speed execution of the emulation state.  This function is currently
// designed to be run from an event loop, but will eventually be re-tooled with threading
// in mindunder the new GUI (assuming Linux approves!), which will make life easier *and*
// improve overall performance too!
extern void SysExecute();


// Library Helpers for HostSys functions, left in for now for convenience.

extern void *SysLoadLibrary(const char *lib);	// Loads Library
extern void *SysLoadSym(void *lib, const char *sym);	// Loads Symbol from Library
extern const char *SysLibError();				// Gets previous error loading symbols
extern void SysCloseLibrary(void *lib);		// Closes Library

// Maps a block of memory for use as a recompiled code buffer, and ensures that the 
// allocation is below a certain memory address (specified in "bounds" parameter).
// The allocated block has code execution privileges.
// Returns NULL on allocation failure.
extern u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller="Unnamed");

extern void vSyncDebugStuff( uint frame );

// Writes text to the console.
// *DEPRECIATED* Use Console namespace methods instead.
void SysPrintf(const char *fmt, ...);	// *DEPRECIATED* 

// Console Namespace -- Replacements for SysPrintf.
// SysPrintf is depreciated -- We should phase these in over time.
namespace Console
{
	enum Colors
	{
		Color_Black = 0,
		Color_Red,
		Color_Green,
		Color_Yellow,
		Color_Blue,
		Color_Magenta,
		Color_Cyan,
		Color_White
	};

	// va_args version of WriteLn, mostly for internal use only.
	extern void __fastcall _WriteLn( Colors color, const char* fmt, va_list args );

	extern void Open();
	extern void Close();
	extern void SetTitle( const std::string& title );

	// Changes the active console color.
	// This color will be unset by calls to colored text methods
	// such as ErrorMsg and Notice.
	extern void __fastcall SetColor( Colors color );

	// Restores the console color to default (usually low-intensity white on Win32)
	extern void ClearColor();

	// The following Write functions return bool so that we can use macros to exclude
	// them from different build types.  The return values are always zero.

	// Writes a newline to the console.
	extern bool __fastcall Newline();

	// Writes an unformatted string of text to the console (fast!)
	// No newline is appended.
	extern bool __fastcall Write( const char* fmt );

	// Writes an unformatted string of text to the console (fast!)
	// A newline is automatically appended.
	extern bool __fastcall WriteLn( const char* fmt );

	// Writes an unformatted string of text to the console (fast!)
	// A newline is automatically appended, and the console color reset to default.
	extern bool __fastcall WriteLn( Colors color, const char* fmt );

	// Writes a line of colored text to the console, with automatic newline appendage.
	// The console color is reset to default when the operation is complete.
	extern bool WriteLn( Colors color, const char* fmt, VARG_PARAM dummy, ... );

	// Writes a line of colored text to the console (no newline).
	// The console color is reset to default when the operation is complete.
	extern bool Write( Colors color, const char* fmt, VARG_PARAM dummy, ... );
	extern bool Write( Colors color, const char* fmt );

	// Writes a formatted message to the console (no newline)
	extern bool Write( const char* fmt, VARG_PARAM dummy, ... );

	// Writes a formatted message to the console, with appended newline.
	extern bool WriteLn( const char* fmt, VARG_PARAM dummy, ... );

	// Displays a message in the console with red emphasis.
	// Newline is automatically appended.
	extern bool Error( const char* fmt, VARG_PARAM dummy, ... );
	extern bool Error( const char* fmt );

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	extern bool Notice( const char* fmt, VARG_PARAM dummy, ... );
	extern bool Notice( const char* fmt );

	// Displays a message in the console with yellow emphasis.
	// Newline is automatically appended.
	extern bool Status( const char* fmt, VARG_PARAM dummy, ... );
	extern bool Status( const char* fmt );
}

// Different types of message boxes that the emulator can employ from the friendly confines
// of it's blissful unawareness of whatever GUI it runs under. :)  All message boxes exhibit
// blocking behavior -- they prompt the user for action and only return after the user has
// responded to the prompt.
namespace Msgbox
{
	// Pops up an alert Dialog Box with a singular "OK" button.
	// Always returns false.  Replacement for SysMessage.
	extern bool Alert( const char* fmt, VARG_PARAM dummy, ... );
	extern bool Alert( const char* fmt );

	// Pops up a dialog box with Ok/Cancel buttons.  Returns the result of the inquiry,
	// true if OK, false if cancel.
	extern bool OkCancel( const char* fmt, VARG_PARAM dummy, ... );
}

using Console::Color_Red;
using Console::Color_Green;
using Console::Color_Blue;
using Console::Color_Magenta;
using Console::Color_Cyan;
using Console::Color_Yellow;
using Console::Color_White;

//////////////////////////////////////////////////////////////
// Dev / Debug conditionals --
//   Consts for using if() statements instead of uglier #ifdef macros.
//   Abbreviated macros for dev/debug only consoles and msgboxes.

#ifdef PCSX2_DEVBUILD

#	define DevCon Console
#	define DevMsg MsgBox
	static const bool IsDevBuild = true;

#else

#	define DevCon 0&&Console
#	define DevMsg 
	static const bool IsDevBuild = false;

#endif

#ifdef _DEBUG

#	define DbgCon Console
	static const bool IsDebugBuild = true;

#else

#	define DbgCon 0&&Console
	static const bool IsDebugBuild = false;

#endif

#endif /* __SYSTEM_H__ */
