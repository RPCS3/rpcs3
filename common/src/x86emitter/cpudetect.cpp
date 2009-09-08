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
#include "internal.h"
#include "tools.h"
#include "Utilities/RedtapeWindows.h"
#include "Utilities/Threading.h"

using namespace x86Emitter;

PCSX2_ALIGNED16( x86CPU_INFO x86caps );

static s32 iCpuId( u32 cmd, u32 *regs )
{
#ifdef _MSC_VER
	__asm
	{
		xor ecx, ecx;		// ecx should be zero for CPUID(4)
	}
#else
	__asm__ ( "xor %ecx, %ecx" );
#endif

   __cpuid( (int*)regs, cmd );
   return 0;
}

static u64 GetRdtsc( void )
{
   return __rdtsc();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Note: This function doesn't support GCC/Linux.  Looking online it seems the only
// way to simulate the Microsoft SEH model is to use unix signals, and the 'sigaction'
// function specifically.  Maybe a project for a linux developer at a later date. :)
//
#ifdef _MSC_VER
static bool _test_instruction( void* pfnCall )
{
	__try {
        ((void (*)())pfnCall)();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
	return true;
}

static char* bool_to_char( bool testcond )
{
	return testcond ? "true" : "false";
}

#endif

#ifdef __LINUX__
#	include <sys/time.h>
#	include <errno.h>
#endif

#ifdef _WINDOWS_
static HANDLE s_threadId = NULL;
static DWORD s_oldmask = ERROR_INVALID_PARAMETER;
#endif

static void SetSingleAffinity()
{
#ifdef _WINDOWS_
	// Assign a single CPU thread affinity to ensure rdtsc() accuracy.
	// (rdtsc for each CPU/core can differ, causing skewed results)

	DWORD_PTR availProcCpus, availSysCpus;
	if( !GetProcessAffinityMask( GetCurrentProcess(), &availProcCpus, &availSysCpus ) ) return;

	int i;
	for( i=0; i<32; ++i )
	{
		if( availProcCpus & (1<<i) ) break;
	}

	s_threadId = GetCurrentThread();
	s_oldmask = SetThreadAffinityMask( s_threadId, (1UL<<i) );

	if( s_oldmask == ERROR_INVALID_PARAMETER )
	{
		Console::Notice(
			"CpuDetect: SetThreadAffinityMask failed...\n"
			"\tSystem Affinity : 0x%08x"
			"\tProcess Affinity: 0x%08x"
			"\tAttempted Thread Affinity CPU: i",
			availProcCpus, availSysCpus, i
		);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//
static s64 CPUSpeedHz( u64 time )
{
	u64 timeStart, timeStop;
	s64 startTick, endTick;

	if( ! x86caps.hasTimeStampCounter )
		return 0; //check if function is supported

	SetSingleAffinity();

	// Align the cpu execution to a cpuTick boundary.

	do { timeStart = GetCPUTicks();
	} while( GetCPUTicks() == timeStart );

	do
	{
		timeStop = GetCPUTicks( );
		startTick = GetRdtsc( );
	} while( ( timeStop - timeStart ) == 0 );

	timeStart = timeStop;
	do
	{
		timeStop = GetCPUTicks();
		endTick = GetRdtsc();
	}
	while( ( timeStop - timeStart ) < time );

#ifdef _WINDOWS_
	if( s_oldmask != ERROR_INVALID_PARAMETER )
		SetThreadAffinityMask( s_threadId, s_oldmask );
#endif

	return (s64)( endTick - startTick );
}

////////////////////////////////////////////////////
int arr[] = {
	0x65746e49, 0x2952286c, 0x726f4320, 0x4d542865,
	0x51203229,0x20646175,0x20555043,0x20202020 ,
	0x20202020,0x20402020,0x36362e32,0x7a4847
};

void cpudetectInit()
{
   u32 regs[ 4 ];
   u32 cmds;
   int cputype=0;            // Cpu type
   //AMD 64 STUFF
   u32 x86_64_8BITBRANDID;
   u32 x86_64_12BITBRANDID;
   int num;
   char str[50];

   memzero_obj( x86caps.VendorName );
   x86caps.FamilyID = 0;
   x86caps.Model  = 0;
   x86caps.TypeID  = 0;
   x86caps.StepID = 0;
   x86caps.Flags  = 0;
   x86caps.EFlags = 0;

   if ( iCpuId( 0, regs ) == -1 ) return;

   cmds = regs[ 0 ];
   ((u32*)x86caps.VendorName)[ 0 ] = regs[ 1 ];
   ((u32*)x86caps.VendorName)[ 1 ] = regs[ 3 ];
   ((u32*)x86caps.VendorName)[ 2 ] = regs[ 2 ];

   // Hack - prevents reg[2] & reg[3] from being optimized out of existance!
   num = sprintf(str, "\tx86Flags  =  %8.8x %8.8x\n", regs[3], regs[2]);

   u32 LogicalCoresPerPhysicalCPU = 0;
   u32 PhysicalCoresPerPhysicalCPU = 1;

   if ( cmds >= 0x00000001 )
   {
      if ( iCpuId( 0x00000001, regs ) != -1 )
      {
         x86caps.StepID =  regs[ 0 ]        & 0xf;
         x86caps.Model  = (regs[ 0 ] >>  4) & 0xf;
         x86caps.FamilyID = (regs[ 0 ] >>  8) & 0xf;
         x86caps.TypeID  = (regs[ 0 ] >> 12) & 0x3;
		 LogicalCoresPerPhysicalCPU = ( regs[1] >> 16 ) & 0xff;
         x86_64_8BITBRANDID = regs[ 1 ] & 0xff;
         x86caps.Flags  =  regs[ 3 ];
         x86caps.Flags2 =  regs[ 2 ];
      }
   }
   /* detect multicore for intel cpu */
   if ((cmds >= 0x00000004) && !strcmp("GenuineIntel",x86caps.VendorName))
   {
      if ( iCpuId( 0x00000004, regs ) != -1 )
      {
         PhysicalCoresPerPhysicalCPU += ( regs[0] >> 26) & 0x3f;
      }
   }

   if ( iCpuId( 0x80000000, regs ) != -1 )
   {
      cmds = regs[ 0 ];
      if ( cmds >= 0x80000001 )
      {
		 if ( iCpuId( 0x80000001, regs ) != -1 )
         {
			x86_64_12BITBRANDID = regs[1] & 0xfff;
			x86caps.EFlags2 = regs[ 2 ];
            x86caps.EFlags = regs[ 3 ];

         }
      }
      /* detect multicore for amd cpu */
      if ((cmds >= 0x80000008) && !strcmp("AuthenticAMD",x86caps.VendorName))
      {
         if ( iCpuId( 0x80000008, regs ) != -1 )
         {
            PhysicalCoresPerPhysicalCPU += ( regs[2] ) & 0xff;
         }
      }
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
	if ( x86caps.VendorName[ 0 ] == 'G' ){ cputype=0;}//trick lines but if you know a way better ;p
	if ( x86caps.VendorName[ 0 ] == 'A' ){ cputype=1;}

	memzero_obj( x86caps.FamilyName );
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

	//////////////////////////////////////////////////////////////////////////////////////////
	// SIMD Instruction Support Detection
	//
	// Can the SSE3 / SSE4.1 bits be trusted?  Using an instruction test is a very "complete"
	// approach to ensuring the instruction set is supported, and at least one reported case
	// of a Q9550 not having it's SSE 4.1 bit set but still supporting it properly is fixed
	// by this --air
	//  (note: the user who reported the case later fixed the problem by doing a CMOS reset)
	//
	// Linux support note: Linux/GCC doesn't have SEH-style exceptions which allow handling of
	// CPU-level exceptions (__try/__except in msvc) so this code is disabled on GCC, and
	// detection relies on the CPUID bits alone.

	#ifdef _MSC_VER
	u8* recSSE = (u8*)HostSys::Mmap( NULL, 0x1000 );
	if( recSSE != NULL )
	{
		xSetPtr( recSSE );
		xMOVSLDUP( xmm1, xmm0 );
		xRET();

		u8* funcSSSE3 = xGetPtr();
		xPABS.W( xmm0, xmm1 );
		xRET();

		u8* funcSSE41 = xGetPtr();
		xBLEND.VPD( xmm1, xmm0 );
		xRET();

		bool sse3_result = _test_instruction( recSSE );  // sse3
		bool ssse3_result = _test_instruction( funcSSSE3 );
		bool sse41_result = _test_instruction( funcSSE41 );

		HostSys::Munmap( recSSE, 0x1000 );

		// Test for and log any irregularities here.
		// We take the instruction test result over cpuid since (in theory) it should be a
		// more reliable gauge of the cpu's actual ability.  But since a difference in bit
		// and actual ability may represent a cmos/bios problem, we report it to the user.

		if( sse3_result != !!x86caps.hasStreamingSIMD3Extensions )
		{
			Console::Notice( "SSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasStreamingSIMD3Extensions ), bool_to_char( sse3_result ) );

			x86caps.hasStreamingSIMD3Extensions = sse3_result;
		}

		if( ssse3_result != !!x86caps.hasSupplementalStreamingSIMD3Extensions )
		{
			Console::Notice( "SSSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasSupplementalStreamingSIMD3Extensions ), bool_to_char( ssse3_result ) );

			x86caps.hasSupplementalStreamingSIMD3Extensions = ssse3_result;
		}

		if( sse41_result != !!x86caps.hasStreamingSIMD4Extensions )
		{
			Console::Notice( "SSE4 Detection Inconsistency: cpuid=%s, test_result=%s",
				bool_to_char( !!x86caps.hasStreamingSIMD4Extensions ), bool_to_char( sse41_result ) );

			x86caps.hasStreamingSIMD4Extensions = sse41_result;
		}

	}
	else
	{
		Console::Notice(
			"Notice: Could not allocate memory for SSE3/4 detection.\n"
			"\tRelying on CPUID results. [this is not an error]"
		);
	}
	#endif

	////////////////////////////////////////////////////////////////////////////////////////////
	//  Core Counting!

	if( !x86caps.hasMultiThreading || LogicalCoresPerPhysicalCPU == 0 )
		LogicalCoresPerPhysicalCPU = 1;

	// This will assign values into x86caps.LogicalCores and PhysicalCores
	Threading::CountLogicalCores( LogicalCoresPerPhysicalCPU, PhysicalCoresPerPhysicalCPU );
}

