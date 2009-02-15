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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "VUmicro.h"
#include "Threading.h"

#include "iR5900.h"
#include "iR3000A.h"
#include "IopMem.h"
#include "iVUzerorec.h"		// for SuperVUReset


#include "x86/ix86/ix86.h"

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
		"\t%sDetected SSE4.1\n", params
			cpucaps.hasMultimediaExtensions     ? "" : "Not ",
			cpucaps.hasStreamingSIMDExtensions  ? "" : "Not ",
			cpucaps.hasStreamingSIMD2Extensions ? "" : "Not ",
			cpucaps.hasStreamingSIMD3Extensions ? "" : "Not ",
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


// Allocates memory for all recompilers, and force-disables any recs that fail to initialize.
// This should be done asap, since the recompilers tend to demand a lot of system resources, and prefer
// to have those resources at specific address ranges.  The sooner memory is allocated, the better.
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

// This should be called last thing before Pcsx2 exits.
void SysShutdownMem()
{
	cpuShutdown();

	vuMicroMemShutdown();
	psxMemShutdown();
	memShutdown();
}

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

// Resets all PS2 cpu execution states, which does not affect that actual PS2 state/condition.
// This can be called at any time outside the context of a Cpu->Execute() block without
// bad things happening (recompilers will slow down for a brief moment since rec code blocks
// are dumped).
// Use this method to reset the recs when important global pointers like the MTGS are re-assigned.
void SysResetExecutionState()
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

u8 *SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller)
{
	u8 *Mem = (u8*)SysMmap( base, size );

	if( (Mem == NULL) || (bounds != 0 && (((uptr)Mem + size) > bounds)) )
	{
		DevCon::Notice( "First try failed allocating %s at address 0x%x", params caller, base );

		// memory allocation *must* have the top bit clear, so let's try again
		// with NULL (let the OS pick something for us).

		SafeSysMunmap( Mem, size );

		Mem = (u8*)SysMmap( NULL, size );
		if( bounds != 0 && (((uptr)Mem + size) > bounds) )
		{
			DevCon::Error( "Fatal Error:\n\tSecond try failed allocating %s, block ptr 0x%x does not meet required criteria.", params caller, Mem );
			SafeSysMunmap( Mem, size );

			// returns NULL, caller should throw an exception.
		}
	}
	return Mem;
}

