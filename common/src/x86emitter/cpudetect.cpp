/*  Cpudetection lib
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "cpudetect_internal.h"
#include "internal.h"

using namespace x86Emitter;

__aligned16 x86capabilities x86caps;

// Recompiled code buffer for SSE and MXCSR feature testing.
static __pagealigned u8 recSSE[__pagesize];
static __pagealigned u8 targetFXSAVE[512];

#ifdef __LINUX__
#	include <sys/time.h>
#	include <errno.h>
#endif

static const char* bool_to_char( bool testcond )
{
	return testcond ? "true" : "false";
}

// Warning!  We've had problems with the MXCSR detection code causing stack corruption in
// MSVC PGO builds.  The problem was fixed when I moved the MXCSR code to this function, and
// moved the recSSE[] array to a global static (it was local to cpudetectInit).  Commented
// here in case the nutty crash ever re-surfaces. >_<
void x86capabilities::SIMD_EstablishMXCSRmask()
{
	if( !hasStreamingSIMDExtensions ) return;

	MXCSR_Mask.bitmask = 0xFFBF;		// MMX/SSE default

	if( hasStreamingSIMD2Extensions )
	{
		// This is generally safe assumption, but FXSAVE is the "correct" way to
		// detect MXCSR masking features of the cpu, so we use it's result below
		// and override this.

		MXCSR_Mask.bitmask = 0xFFFF;	// SSE2 features added
	}

	if( !CanEmitShit() ) return;

	// the fxsave buffer must be 16-byte aligned to avoid GPF.  I just save it to an
	// unused portion of recSSE, since it has plenty of room to spare.

	HostSys::MemProtectStatic( recSSE, PageAccess_ReadWrite() );

	xSetPtr( recSSE );
	xFXSAVE( ptr[&targetFXSAVE] );
	xRET();

	HostSys::MemProtectStatic( recSSE, PageAccess_ExecOnly() );

	CallAddress( recSSE );

	u32 result = (u32&)targetFXSAVE[28];			// bytes 28->32 are the MXCSR_Mask.
	if( result != 0 )
		MXCSR_Mask.bitmask = result;
}

// Counts the number of cpu cycles executed over the requested number of PerformanceCounter
// ticks. Returns that exact count.
// For best results you should pick a period of time long enough to get a reading that won't
// be prone to rounding error; but short enough that it'll be highly unlikely to be interrupted
// by the operating system task switches.
s64 x86capabilities::_CPUSpeedHz( u64 time ) const
{
	u64 timeStart, timeStop;
	s64 startCycle, endCycle;

	if( ! hasTimeStampCounter )
		return 0;

	SingleCoreAffinity affinity_lock;

	// Align the cpu execution to a cpuTick boundary.

	do {
		timeStart = GetCPUTicks();
		startCycle = __rdtsc();
	} while( GetCPUTicks() == timeStart );

	do {
		timeStop = GetCPUTicks();
		endCycle = __rdtsc();
	} while( ( timeStop - timeStart ) < time );

	s64 cycleCount = endCycle - startCycle;
	s64 timeCount = timeStop - timeStart;
	s64 overrun = timeCount - time;
	if( !overrun ) return cycleCount;

	// interference could cause us to overshoot the target time, compensate:

	double cyclesPerTick = (double)cycleCount / (double)timeCount;
	double newCycleCount = (double)cycleCount - (cyclesPerTick * overrun);

	return (s64)newCycleCount;
}

wxString x86capabilities::GetTypeName() const
{
	switch( TypeID )
	{
		case 0:		return L"Standard OEM";
		case 1:		return L"Overdrive";
		case 2:		return L"Dual";
		case 3:		return L"Reserved";
		default:	return L"Unknown";
	}
}

void x86capabilities::CountCores()
{
	Identify();

	s32 regs[ 4 ];
	u32 cmds;

	LogicalCoresPerPhysicalCPU = 0;
	PhysicalCoresPerPhysicalCPU = 1;

	// detect multicore for Intel cpu

	__cpuid( regs, 0 );
	cmds = regs[ 0 ];
	
	if( cmds >= 0x00000001 )
		LogicalCoresPerPhysicalCPU = ( regs[1] >> 16 ) & 0xff;

	if ((cmds >= 0x00000004) && (VendorID == x86Vendor_Intel))
	{
		__cpuid( regs, 0x00000004 );
		PhysicalCoresPerPhysicalCPU += ( regs[0] >> 26) & 0x3f;
	}

	__cpuid( regs, 0x80000000 );
	cmds = regs[ 0 ];

	// detect multicore for AMD cpu

	if ((cmds >= 0x80000008) && (VendorID == x86Vendor_AMD) )
	{
		__cpuid( regs, 0x80000008 );
		PhysicalCoresPerPhysicalCPU += ( regs[2] ) & 0xff;
		
		// AMD note: they don't support hyperthreading, but they like to flag this true
		// anyway.  Let's force-unflag it until we come up with a better solution.
		// (note: seems to affect some Phenom II's only? -- Athlon X2's and PhenomI's do
		// not seem to do this) --air
		hasMultiThreading = 0;
	}

	if( !hasMultiThreading || LogicalCoresPerPhysicalCPU == 0 )
		LogicalCoresPerPhysicalCPU = 1;

	// This will assign values into LogicalCores and PhysicalCores
	CountLogicalCores();
}

static const char* tbl_x86vendors[] = 
{
	"GenuineIntel",
	"AuthenticAMD"
	"Unknown     ",
};

// Performs all _cpuid-related activity.  This fills *most* of the x86caps structure, except for
// the cpuSpeed and the mxcsr masks.  Those must be completed manually.
void x86capabilities::Identify()
{
	if( isIdentified ) return;
	isIdentified = true;

	s32 regs[ 4 ];
	u32 cmds;

	//AMD 64 STUFF
	u32 x86_64_8BITBRANDID;
	u32 x86_64_12BITBRANDID;

	memzero( VendorName );
	__cpuid( regs, 0 );

	cmds = regs[ 0 ];
	((u32*)VendorName)[ 0 ] = regs[ 1 ];
	((u32*)VendorName)[ 1 ] = regs[ 3 ];
	((u32*)VendorName)[ 2 ] = regs[ 2 ];

	// Determine Vendor Specifics!
	// It's really not recommended that we base much (if anything) on CPU vendor names,
	// however it's currently necessary in order to gain a (pseudo)reliable count of cores
	// and threads used by the CPU (AMD and Intel can't agree on how to make this info available).

	int& vid = (int&)VendorID;
	for( vid=0; vid<x86Vendor_Unknown; ++vid )
	{
		if( memcmp( VendorName, tbl_x86vendors[vid], 12 ) == 0 ) break;
	}

	if ( cmds >= 0x00000001 )
	{
		__cpuid( regs, 0x00000001 );

		StepID		=  regs[ 0 ]        & 0xf;
		Model		= (regs[ 0 ] >>  4) & 0xf;
		FamilyID	= (regs[ 0 ] >>  8) & 0xf;
		TypeID		= (regs[ 0 ] >> 12) & 0x3;
		x86_64_8BITBRANDID	=  regs[ 1 ] & 0xff;
		Flags		=  regs[ 3 ];
		Flags2		=  regs[ 2 ];
	}

	__cpuid( regs, 0x80000000 );
	cmds = regs[ 0 ];
	if ( cmds >= 0x80000001 )
	{
		__cpuid( regs, 0x80000001 );

		x86_64_12BITBRANDID = regs[1] & 0xfff;
		EFlags2 = regs[ 2 ];
		EFlags = regs[ 3 ];
	}

	memzero( FamilyName );
	__cpuid( (int*)FamilyName,		0x80000002);
	__cpuid( (int*)(FamilyName+16),	0x80000003);
	__cpuid( (int*)(FamilyName+32),	0x80000004);

	hasFloatingPointUnit						= ( Flags >>  0 ) & 1;
	hasVirtual8086ModeEnhancements				= ( Flags >>  1 ) & 1;
	hasDebuggingExtensions						= ( Flags >>  2 ) & 1;
	hasPageSizeExtensions						= ( Flags >>  3 ) & 1;
	hasTimeStampCounter							= ( Flags >>  4 ) & 1;
	hasModelSpecificRegisters					= ( Flags >>  5 ) & 1;
	hasPhysicalAddressExtension					= ( Flags >>  6 ) & 1;
	hasMachineCheckArchitecture					= ( Flags >>  7 ) & 1;
	hasCOMPXCHG8BInstruction					= ( Flags >>  8 ) & 1;
	hasAdvancedProgrammableInterruptController	= ( Flags >>  9 ) & 1;
	hasSEPFastSystemCall						= ( Flags >> 11 ) & 1;
	hasMemoryTypeRangeRegisters					= ( Flags >> 12 ) & 1;
	hasPTEGlobalFlag							= ( Flags >> 13 ) & 1;
	hasMachineCheckArchitecture					= ( Flags >> 14 ) & 1;
	hasConditionalMoveAndCompareInstructions	= ( Flags >> 15 ) & 1;
	hasFGPageAttributeTable						= ( Flags >> 16 ) & 1;
	has36bitPageSizeExtension					= ( Flags >> 17 ) & 1;
	hasProcessorSerialNumber					= ( Flags >> 18 ) & 1;
	hasCFLUSHInstruction						= ( Flags >> 19 ) & 1;
	hasDebugStore								= ( Flags >> 21 ) & 1;
	hasACPIThermalMonitorAndClockControl		= ( Flags >> 22 ) & 1;
	hasMultimediaExtensions						= ( Flags >> 23 ) & 1; //mmx
	hasFastStreamingSIMDExtensionsSaveRestore	= ( Flags >> 24 ) & 1;
	hasStreamingSIMDExtensions					= ( Flags >> 25 ) & 1; //sse
	hasStreamingSIMD2Extensions					= ( Flags >> 26 ) & 1; //sse2
	hasSelfSnoop								= ( Flags >> 27 ) & 1;
	hasMultiThreading							= ( Flags >> 28 ) & 1;
	hasThermalMonitor							= ( Flags >> 29 ) & 1;
	hasIntel64BitArchitecture					= ( Flags >> 30 ) & 1;

	// -------------------------------------------------
	// --> SSE3 / SSSE3 / SSE4.1 / SSE 4.2 detection <--
	// -------------------------------------------------

	hasStreamingSIMD3Extensions					= ( Flags2 >> 0 ) & 1; //sse3
	hasSupplementalStreamingSIMD3Extensions		= ( Flags2 >> 9 ) & 1; //ssse3
	hasStreamingSIMD4Extensions					= ( Flags2 >> 19 ) & 1; //sse4.1
	hasStreamingSIMD4Extensions2				= ( Flags2 >> 20 ) & 1; //sse4.2

	// Ones only for AMDs:
	hasMultimediaExtensionsExt					= ( EFlags >> 22 ) & 1; //mmx2
	hasAMD64BitArchitecture						= ( EFlags >> 29 ) & 1; //64bit cpu
	has3DNOWInstructionExtensionsExt			= ( EFlags >> 30 ) & 1; //3dnow+
	has3DNOWInstructionExtensions				= ( EFlags >> 31 ) & 1; //3dnow
	hasStreamingSIMD4ExtensionsA				= ( EFlags2 >> 6 ) & 1; //INSERTQ / EXTRQ / MOVNT

	isIdentified = true;
}

u32 x86capabilities::CalculateMHz() const
{
	InitCPUTicks();
	u64 span = GetTickFrequency();

	if( (span % 1000) < 400 )	// helps minimize rounding errors
		return (u32)( _CPUSpeedHz( span / 1000 ) / 1000 );
	else
		return (u32)( _CPUSpeedHz( span / 500 ) / 2000 );
}

// Special extended version of SIMD testning, which uses exceptions to double-check the presence
// of SSE2/3/4 instructions.  Useful if you don't trust cpuid (at least one report of an invalid
// cpuid has been reported on a Core2 Quad -- the user fixed it by clearing his CMOS).
//
// Results of CPU
void x86capabilities::SIMD_ExceptionTest()
{
	HostSys::MemProtectStatic( recSSE, PageAccess_ReadWrite() );

	//////////////////////////////////////////////////////////////////////////////////////////
	// SIMD Instruction Support Detection (Second Pass)
	//

	if( CanTestInstructionSets() )
	{
		xSetPtr( recSSE );
		xMOVDQU( ptr[ecx], xmm1 );
		xMOVSLDUP( xmm1, xmm0 );
		xMOVDQU( xmm1, ptr[ecx] );
		xRET();

		u8* funcSSSE3 = xGetPtr();
		xMOVDQU( ptr[ecx], xmm1 );
		xPABS.W( xmm1, xmm0 );
		xMOVDQU( xmm1, ptr[ecx] );
		xRET();

		u8* funcSSE41 = xGetPtr();
		xMOVDQU( ptr[ecx], xmm1 );
		xBLEND.VPD( xmm1, xmm0 );
		xMOVDQU( xmm1, ptr[ecx] );
		xRET();

		HostSys::MemProtectStatic( recSSE, PageAccess_ExecOnly() );

		bool sse3_result = _test_instruction( recSSE );  // sse3
		bool ssse3_result = _test_instruction( funcSSSE3 );
		bool sse41_result = _test_instruction( funcSSE41 );

		// Test for and log any irregularities here.
		// We take the instruction test result over cpuid since (in theory) it should be a
		// more reliable gauge of the cpu's actual ability.  But since a difference in bit
		// and actual ability may represent a cmos/bios problem, we report it to the user.

		if( sse3_result != !!hasStreamingSIMD3Extensions )
		{
			Console.Warning( "SSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!hasStreamingSIMD3Extensions ), bool_to_char( sse3_result ) );

			hasStreamingSIMD3Extensions = sse3_result;
		}

		if( ssse3_result != !!hasSupplementalStreamingSIMD3Extensions )
		{
			Console.Warning( "SSSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!hasSupplementalStreamingSIMD3Extensions ), bool_to_char( ssse3_result ) );

			hasSupplementalStreamingSIMD3Extensions = ssse3_result;
		}

		if( sse41_result != !!hasStreamingSIMD4Extensions )
		{
			Console.Warning( "SSE4 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!hasStreamingSIMD4Extensions ), bool_to_char( sse41_result ) );

			hasStreamingSIMD4Extensions = sse41_result;
		}

	}

	SIMD_EstablishMXCSRmask();
}

