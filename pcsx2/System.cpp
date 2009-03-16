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
#include "iVUzerorec.h"		// for SuperVUReset

#include "R5900Exceptions.h"

using namespace std;
using namespace Console;

// disable all session overrides by default...
SessionOverrideFlags g_Session = {false};

bool sysInitialized = false;

namespace Exception
{
	BaseException::~BaseException() throw() {}
}


// I can't believe I had to make my own version of trim.  C++'s STL is totally whack.
// And I still had to fix it too.  I found three samples of trim online and *all* three
// were buggy.  People really need to learn to code before they start posting trim
// functions in their blogs.  (air)
static void trim( string& line )
{
   if ( line.empty() )
      return;

   int string_size = line.length();
   int beginning_of_string = 0;
   int end_of_string = string_size - 1;
   
   bool encountered_characters = false;
   
   // find the start of characters in the string
   while ( (beginning_of_string < string_size) && (!encountered_characters) )
   {
      if ( (line[ beginning_of_string ] != ' ') && (line[ beginning_of_string ] != '\t') )
         encountered_characters = true;
      else
         ++beginning_of_string;
   }

   // test if no characters were found in the string
   if ( beginning_of_string == string_size )
      return;
   
   encountered_characters = false;

   // find the character in the string
   while ( (end_of_string > beginning_of_string) && (!encountered_characters) )
   {
      // if a space or tab was found then ignore it
      if ( (line[ end_of_string ] != ' ') && (line[ end_of_string ] != '\t') )
         encountered_characters = true;
      else
         --end_of_string;
   }   
   
   // return the original string with all whitespace removed from its beginning and end
   // + 1 at the end to add the space for the string delimiter
   //line.substr( beginning_of_string, end_of_string - beginning_of_string + 1 );
   line.erase( end_of_string+1, string_size );
   line.erase( 0, beginning_of_string );
}


//////////////////////////////////////////////////////////////////////////////////////////
// This function should be called once during program execution.
void SysDetect()
{
	if( sysInitialized ) return;
	sysInitialized = true;

	Notice("PCSX2 " PCSX2_VERSION " - compiled on " __DATE__ );
	Notice("Savestate version: %x", params g_SaveVersion);

	// fixme: This line is here for the purpose of creating external ASM code.  Yah. >_<
	DevCon::Notice( "EE pc offset: 0x%x, IOP pc offset: 0x%x", params (u32)&cpuRegs.pc - (u32)&cpuRegs, (u32)&psxRegs.pc - (u32)&psxRegs );

	cpudetectInit();

	string family( cpuinfo.x86Fam );
	trim( family );

	SetColor( Console::Color_White );

	WriteLn( "x86Init:" );
	WriteLn(
		"\tCPU vendor name =  %s\n"
		"\tFamilyID  =  %x\n"
		"\tx86Family =  %s\n"
		"\tCPU speed =  %d.%03d Ghz\n"
		"\tCores     =  %d physical [%d logical]\n"
		"\tx86PType  =  %s\n"
		"\tx86Flags  =  %8.8x %8.8x\n"
		"\tx86EFlags =  %8.8x\n", params
			cpuinfo.x86ID, cpuinfo.x86StepID, family.c_str(), 
			cpuinfo.cpuspeed / 1000, cpuinfo.cpuspeed%1000,
			cpuinfo.PhysicalCores, cpuinfo.LogicalCores,
			cpuinfo.x86Type, cpuinfo.x86Flags, cpuinfo.x86Flags2,
			cpuinfo.x86EFlags
	);

	WriteLn( "Features:" );
	WriteLn(
		"\t%sDetected MMX\n"
		"\t%sDetected SSE\n"
		"\t%sDetected SSE2\n"
		"\t%sDetected SSE3\n"
		"\t%sDetected SSSE3\n"
		"\t%sDetected SSE4.1\n", params
			cpucaps.hasMultimediaExtensions     ? "" : "Not ",
			cpucaps.hasStreamingSIMDExtensions  ? "" : "Not ",
			cpucaps.hasStreamingSIMD2Extensions ? "" : "Not ",
			cpucaps.hasStreamingSIMD3Extensions ? "" : "Not ",
			cpucaps.hasSupplementalStreamingSIMD3Extensions ? "" : "Not ",
			cpucaps.hasStreamingSIMD4Extensions ? "" : "Not "
	);

	if ( cpuinfo.x86ID[0] == 'A' ) //AMD cpu
	{
		WriteLn( " Extended AMD Features:" );
		WriteLn(
			"\t%sDetected MMX2\n"
			"\t%sDetected 3DNOW\n"
			"\t%sDetected 3DNOW2\n", params
			cpucaps.hasMultimediaExtensionsExt       ? "" : "Not ",
			cpucaps.has3DNOWInstructionExtensions    ? "" : "Not ",
			cpucaps.has3DNOWInstructionExtensionsExt ? "" : "Not "
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
		memAlloc();
		psxMemAlloc();
		vuMicroMemAlloc();
	}
	catch( Exception::OutOfMemory& ex )
	{
		// Failures on the core initialization of memory is bad, since it means the emulator is
		// completely non-functional.  If the failure is in the VM build then we can try running
		// the VTLB build instead.  If it's the VTLB build then ... ouch.

		// VTLB build must fail outright...
		Msgbox::Alert( "Failed to allocate memory needed to run pcsx2.\n\nError: %s", params ex.cMessage() );
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
	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert(
			"The EE/IOP recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe EE/IOP interpreter will be used instead (slow!).", params
			ex.cMessage()
		);

		g_Session.ForceDisableEErec = true;

		recCpu.Shutdown();
		psxRec.Shutdown();
	}

	try
	{
		VU0micro::recAlloc();
	}
	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert(
			"The VU0 recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe VU0 interpreter will be used for this session (may slow down some games).", params
			ex.cMessage()
		);

		g_Session.ForceDisableVU0rec = true;
		VU0micro::recShutdown();
	}

	try
	{
		VU1micro::recAlloc();
	}
	catch( Exception::BaseException& ex )
	{
		Msgbox::Alert(
			"The VU1 recompiler failed to initialize with the following error:\n\n"
			"%s"
			"\n\nThe VU1 interpreter will be used for this session (will slow down most games).", params 
			ex.cMessage()
		);

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

	// make sure the VU1 doesn't have lingering "skip" enabled.
	vu1MicroDisableSkip();
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
		Console::Error( ex.cMessage() );
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
void SysPrepareExecution( const char* elf_file, bool use_bios )
{
	if( !g_EmulationInProgress )
	{
		try
		{
			cpuReset();
		}
		catch( Exception::BaseException& ex )
		{
			Msgbox::Alert( ex.cMessage() );
			return;
		}

		if (OpenPlugins(NULL) == -1)
			return;

		if( elf_file == NULL )
		{
			if( !StateRecovery::HasState() )
			{
				// Not recovering a state, so need to execute the bios and load the ELF information.
				// (note: gsRecoveries are done from ExecuteCpu)

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

	HostGui::Notice(_("Resetting..."));
	Console::SetTitle(_("Resetting..."));

	g_EmulationInProgress = false;
	StateRecovery::Clear();

	cpuShutdown();
	ShutdownPlugins();

	ElfCRC = 0;

	// Note : No need to call cpuReset() here.  It gets called automatically before the
	// emulator resumes execution.

	HostGui::Notice(_("Ready"));
	Console::SetTitle(_("*PCSX2* Emulation state is reset."));
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

void *SysLoadLibrary(const char *lib) { return HostSys::LoadLibrary( lib ); }
void *SysLoadSym(void *lib, const char *sym) { return HostSys::LoadSym( lib, sym ); }
const char *SysLibError() { return HostSys::LibError(); }
void SysCloseLibrary(void *lib) { HostSys::CloseLibrary( lib ); }
