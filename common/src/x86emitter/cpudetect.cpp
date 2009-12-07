/*  Cpudetection lib
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

#include "PrecompiledHeader.h"
#include "cpudetect_internal.h"
#include "internal.h"

using namespace x86Emitter;

__aligned16 x86CPU_INFO x86caps;

#ifdef __LINUX__
#	include <sys/time.h>
#	include <errno.h>
#endif

static const char* bool_to_char( bool testcond )
{
	return testcond ? "true" : "false";
}

//////////////////////////////////////////////////////////////////////////////////////////
//
static s64 CPUSpeedHz( u64 time )
{
	u64 timeStart, timeStop;
	s64 startTick, endTick;

	if( ! x86caps.hasTimeStampCounter )
		return 0;

	SingleCoreAffinity affinity_lock;

	// Align the cpu execution to a cpuTick boundary.

	do { timeStart = GetCPUTicks();
	} while( GetCPUTicks() == timeStart );

	do
	{
		timeStop = GetCPUTicks();
		startTick = __rdtsc();
	} while( ( timeStop - timeStart ) == 0 );

	timeStart = timeStop;
	do
	{
		timeStop = GetCPUTicks();
		endTick = __rdtsc();
	}
	while( ( timeStop - timeStart ) < time );

	return (s64)( endTick - startTick );
}

////////////////////////////////////////////////////
void cpudetectInit()
{
	u32 regs[ 4 ];
	u32 cmds;
	//AMD 64 STUFF
	u32 x86_64_8BITBRANDID;
	u32 x86_64_12BITBRANDID;

	memzero( x86caps.VendorName );
	x86caps.FamilyID	= 0;
	x86caps.Model	= 0;
	x86caps.TypeID	= 0;
	x86caps.StepID	= 0;
	x86caps.Flags	= 0;
	x86caps.EFlags	= 0;

	//memzero( regs );
	iCpuId( 0, regs );

	cmds = regs[ 0 ];
	((u32*)x86caps.VendorName)[ 0 ] = regs[ 1 ];
	((u32*)x86caps.VendorName)[ 1 ] = regs[ 3 ];
	((u32*)x86caps.VendorName)[ 2 ] = regs[ 2 ];

	u32 LogicalCoresPerPhysicalCPU = 0;
	u32 PhysicalCoresPerPhysicalCPU = 1;

	if ( cmds >= 0x00000001 )
	{
		iCpuId( 0x00000001, regs );

		x86caps.StepID		=  regs[ 0 ]        & 0xf;
		x86caps.Model		= (regs[ 0 ] >>  4) & 0xf;
		x86caps.FamilyID	= (regs[ 0 ] >>  8) & 0xf;
		x86caps.TypeID		= (regs[ 0 ] >> 12) & 0x3;
		x86_64_8BITBRANDID	=  regs[ 1 ] & 0xff;
		x86caps.Flags		=  regs[ 3 ];
		x86caps.Flags2		=  regs[ 2 ];

		LogicalCoresPerPhysicalCPU = ( regs[1] >> 16 ) & 0xff;
	}

	// detect multicore for Intel cpu

	if ((cmds >= 0x00000004) && !strcmp("GenuineIntel",x86caps.VendorName))
	{
		iCpuId( 0x00000004, regs );
		PhysicalCoresPerPhysicalCPU += ( regs[0] >> 26) & 0x3f;
	}

	iCpuId( 0x80000000, regs );
	cmds = regs[ 0 ];
	if ( cmds >= 0x80000001 )
	{
		iCpuId( 0x80000001, regs );

		x86_64_12BITBRANDID = regs[1] & 0xfff;
		x86caps.EFlags2 = regs[ 2 ];
		x86caps.EFlags = regs[ 3 ];
	}
	  
	// detect multicore for AMD cpu

	if ((cmds >= 0x80000008) && !strcmp("AuthenticAMD",x86caps.VendorName))
	{
		iCpuId( 0x80000008, regs );
		PhysicalCoresPerPhysicalCPU += ( regs[2] ) & 0xff;
	}

	switch(x86caps.TypeID)
	{
		case 0:
			strcpy( x86caps.TypeName, "Standard OEM");
		break;
		case 1:
			strcpy( x86caps.TypeName, "Overdrive");
		break;
		case 2:
			strcpy( x86caps.TypeName, "Dual");
		break;
		case 3:
			strcpy( x86caps.TypeName, "Reserved");
		break;
		default:
			strcpy( x86caps.TypeName, "Unknown");
		break;
	}

	#if 0
	// vendor identification, currently unneeded.
	// It's really not recommended that we base much (if anything) on CPU vendor names.
	// But the code is left in as an ifdef, for possible future reference.

	int cputype=0;            // Cpu type
	static const char* Vendor_Intel	= "GenuineIntel";
	static const char* Vendor_AMD	= "AuthenticAMD";

	if( memcmp( x86caps.VendorName, Vendor_Intel, 12 ) == 0 ) { cputype = 0; } else
	if( memcmp( x86caps.VendorName, Vendor_AMD, 12 ) == 0 ) { cputype = 1; }

	if ( x86caps.VendorName[ 0 ] == 'G' ) { cputype = 0; }
	if ( x86caps.VendorName[ 0 ] == 'A' ) { cputype = 1; }
	#endif

	memzero( x86caps.FamilyName );
	iCpuId( 0x80000002, (u32*)x86caps.FamilyName);
	iCpuId( 0x80000003, (u32*)(x86caps.FamilyName+16));
	iCpuId( 0x80000004, (u32*)(x86caps.FamilyName+32));

	//capabilities
	x86caps.hasFloatingPointUnit                         = ( x86caps.Flags >>  0 ) & 1;
	x86caps.hasVirtual8086ModeEnhancements               = ( x86caps.Flags >>  1 ) & 1;
	x86caps.hasDebuggingExtensions                       = ( x86caps.Flags >>  2 ) & 1;
	x86caps.hasPageSizeExtensions                        = ( x86caps.Flags >>  3 ) & 1;
	x86caps.hasTimeStampCounter                          = ( x86caps.Flags >>  4 ) & 1;
	x86caps.hasModelSpecificRegisters                    = ( x86caps.Flags >>  5 ) & 1;
	x86caps.hasPhysicalAddressExtension                  = ( x86caps.Flags >>  6 ) & 1;
	x86caps.hasMachineCheckArchitecture                  = ( x86caps.Flags >>  7 ) & 1;
	x86caps.hasCOMPXCHG8BInstruction                     = ( x86caps.Flags >>  8 ) & 1;
	x86caps.hasAdvancedProgrammableInterruptController   = ( x86caps.Flags >>  9 ) & 1;
	x86caps.hasSEPFastSystemCall                         = ( x86caps.Flags >> 11 ) & 1;
	x86caps.hasMemoryTypeRangeRegisters                  = ( x86caps.Flags >> 12 ) & 1;
	x86caps.hasPTEGlobalFlag                             = ( x86caps.Flags >> 13 ) & 1;
	x86caps.hasMachineCheckArchitecture                  = ( x86caps.Flags >> 14 ) & 1;
	x86caps.hasConditionalMoveAndCompareInstructions     = ( x86caps.Flags >> 15 ) & 1;
	x86caps.hasFGPageAttributeTable                      = ( x86caps.Flags >> 16 ) & 1;
	x86caps.has36bitPageSizeExtension                    = ( x86caps.Flags >> 17 ) & 1;
	x86caps.hasProcessorSerialNumber                     = ( x86caps.Flags >> 18 ) & 1;
	x86caps.hasCFLUSHInstruction                         = ( x86caps.Flags >> 19 ) & 1;
	x86caps.hasDebugStore                                = ( x86caps.Flags >> 21 ) & 1;
	x86caps.hasACPIThermalMonitorAndClockControl         = ( x86caps.Flags >> 22 ) & 1;
	x86caps.hasMultimediaExtensions                      = ( x86caps.Flags >> 23 ) & 1; //mmx
	x86caps.hasFastStreamingSIMDExtensionsSaveRestore    = ( x86caps.Flags >> 24 ) & 1;
	x86caps.hasStreamingSIMDExtensions                   = ( x86caps.Flags >> 25 ) & 1; //sse
	x86caps.hasStreamingSIMD2Extensions                  = ( x86caps.Flags >> 26 ) & 1; //sse2
	x86caps.hasSelfSnoop                                 = ( x86caps.Flags >> 27 ) & 1;
	x86caps.hasMultiThreading                            = ( x86caps.Flags >> 28 ) & 1;
	x86caps.hasThermalMonitor                            = ( x86caps.Flags >> 29 ) & 1;
	x86caps.hasIntel64BitArchitecture                    = ( x86caps.Flags >> 30 ) & 1;

	//that is only for AMDs
	x86caps.hasMultimediaExtensionsExt                   = ( x86caps.EFlags >> 22 ) & 1; //mmx2
	x86caps.hasAMD64BitArchitecture                      = ( x86caps.EFlags >> 29 ) & 1; //64bit cpu
	x86caps.has3DNOWInstructionExtensionsExt             = ( x86caps.EFlags >> 30 ) & 1; //3dnow+
	x86caps.has3DNOWInstructionExtensions                = ( x86caps.EFlags >> 31 ) & 1; //3dnow
	x86caps.hasStreamingSIMD4ExtensionsA                 = ( x86caps.EFlags2 >> 6 ) & 1; //INSERTQ / EXTRQ / MOVNT

	InitCPUTicks();
	u64 span = GetTickFrequency();

	if( (span % 1000) < 400 )	// helps minimize rounding errors
		x86caps.Speed = (u32)( CPUSpeedHz( span / 1000 ) / 1000 );
	else
		x86caps.Speed = (u32)( CPUSpeedHz( span / 500 ) / 2000 );

	// --> SSE3 / SSSE3 / SSE4.1 / SSE 4.2 detection <--

	x86caps.hasStreamingSIMD3Extensions  = ( x86caps.Flags2 >> 0 ) & 1; //sse3
	x86caps.hasSupplementalStreamingSIMD3Extensions = ( x86caps.Flags2 >> 9 ) & 1; //ssse3
	x86caps.hasStreamingSIMD4Extensions  = ( x86caps.Flags2 >> 19 ) & 1; //sse4.1
	x86caps.hasStreamingSIMD4Extensions2 = ( x86caps.Flags2 >> 20 ) & 1; //sse4.2

	static __pagealigned u8 recSSE[__pagesize];
	HostSys::MemProtectStatic( recSSE, Protect_ReadWrite, true );
	
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

		bool sse3_result = _test_instruction( recSSE );  // sse3
		bool ssse3_result = _test_instruction( funcSSSE3 );
		bool sse41_result = _test_instruction( funcSSE41 );

		// Test for and log any irregularities here.
		// We take the instruction test result over cpuid since (in theory) it should be a
		// more reliable gauge of the cpu's actual ability.  But since a difference in bit
		// and actual ability may represent a cmos/bios problem, we report it to the user.

		if( sse3_result != !!x86caps.hasStreamingSIMD3Extensions )
		{
			Console.Warning( "SSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasStreamingSIMD3Extensions ), bool_to_char( sse3_result ) );

			x86caps.hasStreamingSIMD3Extensions = sse3_result;
		}

		if( ssse3_result != !!x86caps.hasSupplementalStreamingSIMD3Extensions )
		{
			Console.Warning( "SSSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasSupplementalStreamingSIMD3Extensions ), bool_to_char( ssse3_result ) );

			x86caps.hasSupplementalStreamingSIMD3Extensions = ssse3_result;
		}

		if( sse41_result != !!x86caps.hasStreamingSIMD4Extensions )
		{
			Console.Warning( "SSE4 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasStreamingSIMD4Extensions ), bool_to_char( sse41_result ) );

			x86caps.hasStreamingSIMD4Extensions = sse41_result;
		}

	}
	else
	{
		Console.Warning(
			"Notice: Could not allocate memory for SSE3/4 detection.\n"
			"\tRelying on CPUID results. [this is not an error]"
		);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	// Establish MXCSR Mask...
	
	MXCSR_Mask.bitmask = 0xFFBF;
	if( x86caps.hasFastStreamingSIMDExtensionsSaveRestore )
	{
		// the fxsave buffer should be 16-byte aligned.  I just save it to an unused portion of
		// recSSE, since it has plenty of room to spare.

		xSetPtr( recSSE );
		xFXSAVE( recSSE + 1024 );
		xRET();

		CallAddress( recSSE );

		u32 result = (u32&)recSSE[1024+28];			// bytes 28->32 are the MXCSR_Mask.
		if( result != 0 )
			MXCSR_Mask.bitmask = result;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//  Core Counting!

	if( !x86caps.hasMultiThreading || LogicalCoresPerPhysicalCPU == 0 )
		LogicalCoresPerPhysicalCPU = 1;

	// This will assign values into x86caps.LogicalCores and PhysicalCores
	CountLogicalCores( LogicalCoresPerPhysicalCPU, PhysicalCoresPerPhysicalCPU );
}

