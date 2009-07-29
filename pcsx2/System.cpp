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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "HostGui.h"

#include "VUmicro.h"
#include "iR5900.h"
#include "R3000A.h"
#include "IopMem.h"
#include "sVU_zerorec.h"		// for SuperVUReset

#include "R5900Exceptions.h"

#include "CDVD/CDVD.h"

using namespace std;

// disable all session overrides by default...
SessionOverrideFlags g_Session = {false};

bool sysInitialized = false;


// -----------------------------------------------------------------------
// This function should be called once during program execution.
//
void SysDetect()
{
	using namespace Console;

	if( sysInitialized ) return;
	sysInitialized = true;

	Notice("PCSX2 " PCSX2_VERSION " - compiled on " __DATE__ );
	Notice("Savestate version: %x", params g_SaveVersion);

	cpudetectInit();

	SetColor( Color_Black );

	WriteLn( "x86Init:" );
	WriteLn( wxsFormat(
		L"\tCPU vendor name  =  %s\n"
		L"\tFamilyID         =  %x\n"
		L"\tx86Family        =  %s\n"
		L"\tCPU speed        =  %d.%03d Ghz\n"
		L"\tCores            =  %d physical [%d logical]\n"
		L"\tx86PType         =  %s\n"
		L"\tx86Flags         =  %8.8x %8.8x\n"
		L"\tx86EFlags        =  %8.8x\n",
			wxString::FromAscii( x86caps.VendorName ).c_str(), x86caps.StepID,
			wxString::FromAscii( x86caps.FamilyName ).Trim().Trim(false).c_str(), 
			x86caps.Speed / 1000, x86caps.Speed%1000,
			x86caps.PhysicalCores, x86caps.LogicalCores,
			wxString::FromAscii( x86caps.TypeName ).c_str(),
			x86caps.Flags, x86caps.Flags2,
			x86caps.EFlags
	) );

	WriteLn( "Features:" );
	WriteLn(
		"\t%sDetected MMX\n"
		"\t%sDetected SSE\n"
		"\t%sDetected SSE2\n"
		"\t%sDetected SSE3\n"
		"\t%sDetected SSSE3\n"
		"\t%sDetected SSE4.1\n"
		"\t%sDetected SSE4.2\n", params
			x86caps.hasMultimediaExtensions     ? "" : "Not ",
			x86caps.hasStreamingSIMDExtensions  ? "" : "Not ",
			x86caps.hasStreamingSIMD2Extensions ? "" : "Not ",
			x86caps.hasStreamingSIMD3Extensions ? "" : "Not ",
			x86caps.hasSupplementalStreamingSIMD3Extensions ? "" : "Not ",
			x86caps.hasStreamingSIMD4Extensions  ? "" : "Not ",
			x86caps.hasStreamingSIMD4Extensions2 ? "" : "Not "
	);

	if ( x86caps.VendorName[0] == 'A' ) //AMD cpu
	{
		WriteLn( " Extended AMD Features:" );
		WriteLn(
			"\t%sDetected MMX2\n"
			"\t%sDetected 3DNOW\n"
			"\t%sDetected 3DNOW2\n"
			"\t%sDetected SSE4a\n", params
			x86caps.hasMultimediaExtensionsExt       ? "" : "Not ",
			x86caps.has3DNOWInstructionExtensions    ? "" : "Not ",
			x86caps.has3DNOWInstructionExtensionsExt ? "" : "Not ",
			x86caps.hasStreamingSIMD4ExtensionsA     ? "" : "Not "
		);
	}

	Console::ClearColor();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Allocates memory for all PS2 systems.
bool SysAllocateMem()
{
	// Allocate PS2 system ram space (required by interpreters and recompilers both)

	try
	{
		vtlb_Core_Alloc();
		memAlloc();
		psxMemAlloc();
		vuMicroMemAlloc();
	}
	catch( Exception::OutOfMemory& )
	{
		// TODO : Should this error be handled here or allowed to be handled by the main
		// exception handler?

		// Failures on the core initialization of memory is bad, since it means the emulator is
		// completely non-functional.
		
		//Msgbox::Alert( "Failed to allocate memory needed to run pcsx2.\n\nError: %s", params ex.cMessage() );
		SysShutdownMem();
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Allocates memory for all recompilers, and force-disables any recs that fail to initialize.
// This should be done asap, since the recompilers tend to demand a lot of system resources,
// and prefer to have those resources at specific address ranges.  The sooner memory is
// allocated, the better.
//
// Returns FALSE on *critical* failure (GUI should issue a msg and exit).
void SysAllocateDynarecs()
{
	// Attempt to initialize the recompilers.
	// Most users want to use recs anyway, and if they are using interpreters I don't think the
	// extra few megs of allocation is going to be an issue.

	try
	{
		// R5900 and R3000a must be rec-enabled together for now so if either fails they both fail.
		recCpu.Allocate();
		psxRec.Allocate();
	}
	catch( Exception::BaseException& )
	{
		// TODO : Fix this message.  It should respond according to the user's
		// currently configured recompiler.interpreter options, for example.
		
		/*Msgbox::Alert(
			"The EE/IOP recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe EE/IOP interpreter will be used instead (slow!).", params
			ex.cMessage()
		);*/

		g_Session.ForceDisableEErec = true;

		recCpu.Shutdown();
		psxRec.Shutdown();
	}

	try
	{
		VU0micro::recAlloc();
	}
	catch( Exception::BaseException& )
	{

		// TODO : Fix this message.  It should respond according to the user's
		// currently configured recompiler.interpreter options, for example.
/*
		Msgbox::Alert(
			"The VU0 recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe VU0 interpreter will be used for this session (may slow down some games).", params
			ex.cMessage()
		);
*/

		g_Session.ForceDisableVU0rec = true;
		VU0micro::recShutdown();
	}

	try
	{
		VU1micro::recAlloc();
	}
	catch( Exception::BaseException& )
	{

		// TODO : Fix this message.  It should respond according to the user's
		// currently configured recompiler.interpreter options, for example.
/*
		Msgbox::Alert(
			"The VU1 recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe VU1 interpreter will be used for this session (will slow down most games).", params 
			ex.cMessage()
		);
*/

		g_Session.ForceDisableVU1rec = true;
		VU1micro::recShutdown();
	}

	// If both VUrecs failed, then make sure the SuperVU is totally closed out:
	if( !CHECK_VU0REC && !CHECK_VU1REC)
		SuperVUDestroy( -1 );

}

//////////////////////////////////////////////////////////////////////////////////////////
// This should be called last thing before Pcsx2 exits.
void SysShutdownMem()
{
	cpuShutdown();

	vuMicroMemShutdown();
	psxMemShutdown();
	memShutdown();
	vtlb_Core_Shutdown();
}

//////////////////////////////////////////////////////////////////////////////////////////
// This should generally be called right before calling SysShutdownMem(), although you can optionally
// use it in conjunction with SysAllocDynarecs to allocate/free the dynarec resources on the fly (as
// risky as it might be, since dynarecs could very well fail on the second attempt).
void SysShutdownDynarecs()
{
	// Special SuperVU "complete" terminator.
	SuperVUDestroy( -1 );

	psxRec.Shutdown();
	recCpu.Shutdown();
}


bool g_ReturnToGui = false;			// set to exit the execution of the emulator and return control to the GUI
bool g_EmulationInProgress = false;	// Set TRUE if a game is actively running (set to false on reset)

//////////////////////////////////////////////////////////////////////////////////////////
// Resets all PS2 cpu execution caches, which does not affect that actual PS2 state/condition.
// This can be called at any time outside the context of a Cpu->Execute() block without
// bad things happening (recompilers will slow down for a brief moment since rec code blocks
// are dumped).
// Use this method to reset the recs when important global pointers like the MTGS are re-assigned.
void SysClearExecutionCache()
{
	if( CHECK_EEREC )
	{
		Cpu = &recCpu;
		psxCpu = &psxRec;
	}
	else
	{
		Cpu = &intCpu;
		psxCpu = &psxInt;
	}

	Cpu->Reset();
	psxCpu->Reset();

	vuMicroCpuReset();
}

__forceinline void SysUpdate()
{
#ifdef __LINUX__
	// Doing things the other way results in no keys functioning under Linux!
	HostGui::KeyEvent(PAD1keyEvent());
	HostGui::KeyEvent(PAD2keyEvent());
#else
	keyEvent* ev1 = PAD1keyEvent();
	keyEvent* ev2 = PAD2keyEvent();

	HostGui::KeyEvent( (ev1 != NULL) ? ev1 : ev2);
#endif
}

void SysExecute()
{
	g_EmulationInProgress = true;
	g_ReturnToGui = false;

	// Optimization: We hardcode two versions of the EE here -- one for recs and one for ints.
	// This is because recs are performance critical, and being able to inline them into the
	// function here helps a small bit (not much but every small bit counts!).

	try
	{
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
	}
	catch( R5900Exception::BaseExcept& ex )
	{
		Console::Error( ex.LogMessage() );
		Console::Error( fmt_string( "(EE) PC: 0x%8.8x  \tCycle: 0x%8.8x", ex.cpuState.pc, ex.cpuState.cycle ).c_str() );
	}
}

// Function provided to escape the emulation state, by shutting down plugins and saving
// the GS state.  The execution state is effectively preserved, and can be resumed with a
// call to SysExecute.
void SysEndExecution()
{
	if( Config.closeGSonEsc )
		StateRecovery::MakeGsOnly();

	ClosePlugins( Config.closeGSonEsc );
	g_ReturnToGui = true;
}

// Runs an ELF image directly (ISO or ELF program or BIN)
// Used by Run::FromCD, and Run->Execute when no active emulation state is present.
// elf_file - if NULL, the CDVD plugin is queried for the ELF file.
// use_bios - forces the game to boot through the PS2 bios, instead of bypassing it.
void SysPrepareExecution( const wxString& elf_file, bool use_bios )
{
	// solve a little crash
	if(CDVD.init == NULL)
		CDVD = CDVD_plugin;

	if( !g_EmulationInProgress )
	{
		try
		{
			cpuReset();
		}
		catch( Exception::BaseException& ex )
		{
			Msgbox::Alert( ex.DisplayMessage() );
			return;
		}

		g_Startup.BootMode = (elf_file) ? BootMode_Elf : BootMode_Normal;

		if (OpenPlugins(NULL) == -1) { 
			return; 
		}

		if( elf_file.IsEmpty() )
		{
			if( !StateRecovery::HasState() )
			{
				// Not recovering a state, so need to execute the bios and load the ELF information.
				// (note: gsRecoveries are done from ExecuteCpu)

				wxString ename;
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
	}

	StateRecovery::Recover();
	HostGui::BeginExecution();
}

void SysRestorableReset()
{
	if( !g_EmulationInProgress ) return;
	StateRecovery::MakeFull();
}

void SysReset()
{
	// fixme - this code  sets the statusbar but never returns control to the window message pump
	// so the status bar won't receive the WM_PAINT messages needed to update itself anyway.
	// Oops! (air)

	HostGui::Notice( _("Resetting...") );
	Console::SetTitle( _("Resetting...") );

	g_EmulationInProgress = false;
	StateRecovery::Clear();

	cpuShutdown();
	ShutdownPlugins();

	ElfCRC = 0;

	// Note : No need to call cpuReset() here.  It gets called automatically before the
	// emulator resumes execution.

	HostGui::Notice( _("Ready") );
	Console::SetTitle( _("Emulation state is reset.") );
}

u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller)
{
	u8 *Mem = (u8*)HostSys::Mmap( base, size );

	if( (Mem == NULL) || (bounds != 0 && (((uptr)Mem + size) > bounds)) )
	{
		DevCon::Notice( "First try failed allocating %s at address 0x%x", params caller, base );

		// memory allocation *must* have the top bit clear, so let's try again
		// with NULL (let the OS pick something for us).

		SafeSysMunmap( Mem, size );

		Mem = (u8*)HostSys::Mmap( NULL, size );
		if( bounds != 0 && (((uptr)Mem + size) > bounds) )
		{
			DevCon::Error( "Fatal Error:\n\tSecond try failed allocating %s, block ptr 0x%x does not meet required criteria.", params caller, Mem );
			SafeSysMunmap( Mem, size );

			// returns NULL, caller should throw an exception.
		}
	}
	return Mem;
}

// Ensures existence of necessary folders, and performs error handling if the
// folders fail to create.
static void InitFolderStructure()
{

}

// Returns FALSE if the core/recompiler memory allocations failed.
bool SysInit()
{
	PCSX2_MEM_PROTECT_BEGIN();
	SysDetect();
	if( !SysAllocateMem() )
		return false;	// critical memory allocation failure;

	SysAllocateDynarecs();
	PCSX2_MEM_PROTECT_END();

	return true;
}
