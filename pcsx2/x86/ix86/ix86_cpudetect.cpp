/*  Cpudetection lib
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

#include "PrecompiledHeader.h"

#include "System.h"
#include "ix86_legacy_internal.h"

#include "RedtapeWindows.h"

using namespace x86Emitter;

#if defined (_MSC_VER) && _MSC_VER >= 1400

	extern "C"
	{
		void __cpuid(int* CPUInfo, int InfoType);
		unsigned __int64 __rdtsc();
#ifndef __INTEL_COMPILER
#		pragma intrinsic(__cpuid)
#		pragma intrinsic(__rdtsc)
#endif
	}

#endif

CAPABILITIES cpucaps;
CPUINFO cpuinfo;

#define cpuid(cmd,a,b,c,d) \
  __asm__ __volatile__("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
		: "=a" (a), "=r" (b), "=c" (c), "=d" (d)  : "0" (cmd), "c" (0))

extern s32 iCpuId( u32 cmd, u32 *regs )
{
   int flag=1;

#if defined (_MSC_VER) && _MSC_VER >= 1400

   __asm
   {
      xor ecx, ecx; /* ecx should be zero for CPUID(4) */
   }
   __cpuid( (int*)regs, cmd );

   return 0;

#elif defined (_MSC_VER)
   __asm
   {
      push ebx;
      push edi;

      pushfd;
      pop eax;
      mov edx, eax;
      xor eax, 1 << 21;
      push eax;
      popfd;
      pushfd;
      pop eax;
      xor eax, edx;
      mov flag, eax;
   }
   if ( ! flag )
   {
      return -1;
   }

   __asm
   {
      mov eax, cmd;
      xor ecx, ecx; /* ecx should be zero for CPUID(4) */
      cpuid;
      mov edi, [regs]
      mov [edi], eax;
      mov [edi+4], ebx;
      mov [edi+8], ecx;
      mov [edi+12], edx;

      pop edi;
      pop ebx;
   }
   return 0;

#else

   // GCC Assembly Code -->

   // see if we can use cpuid
   __asm__ __volatile__ (
      "sub $0x18, %%esp\n"
      "pushf\n"
      "pop %%eax\n"
      "mov %%eax, %%edx\n"
      "xor $0x200000, %%eax\n"
      "push %%eax\n"
      "popf\n"
      "pushf\n"
      "pop %%eax\n"
      "xor %%edx, %%eax\n"
      "mov %%eax, %[flag]\n"
	  "add $0x18, %%esp\n"
      "cmpl $0x0,%%eax\n"
      "jne 1f\n"
      "mov $0xffffffff, %%eax\n"
      "leave\n"
      "ret\n"
      "1:\n"
      : [flag]"=r"(flag) :
   );

   cpuid(cmd, regs[0], regs[1], regs[2], regs[3]);
   return 0;

#endif // _MSC_VER
}

u64 GetCPUTick( void )
{
#if defined (_MSC_VER) && _MSC_VER >= 1400

   return __rdtsc();

#elif defined(_WIN32)

   __asm rdtsc;

#else

   u32 _a, _d;
	__asm__ __volatile__ ("rdtsc" : "=a"(_a), "=d"(_d));
	return (u64)_a | ((u64)_d << 32);

#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// Note: This function doesn't support GCC/Linux.  Looking online it seems the only 
// way to simulate the Micrsoft SEH model is to use unix signals, and the 'sigaction'
// function specifically.  Maybe a project for a linux developer at a later date. :)
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

#if defined __LINUX__

#include <sys/time.h>
#include <errno.h>

#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
s64 CPUSpeedHz( int time )
{
   int timeStart, 
            timeStop;
   s64 startTick, 
            endTick;

   if( ! cpucaps.hasTimeStampCounter )
   {
      return 0; //check if function is supported
   }
	
	// Align the cpu execution to a timeGetTime boundary.
	// Without this the result could be skewed by up to several milliseconds.

	do { timeStart = GetCPUTicks();
	} while( timeGetTime( ) == timeStart );

	do
	{
		timeStop = GetCPUTicks( );
		startTick = GetCPUTick( );
	} while( ( timeStop - timeStart ) == 0 );

	timeStart = timeStop;
	do
	{
		timeStop = GetCPUTicks();
		endTick = GetCPUTick();
	}
	while( ( timeStop - timeStart ) < time );

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
	
   memzero_obj( cpuinfo.x86ID );
   cpuinfo.x86Family = 0;
   cpuinfo.x86Model  = 0;
   cpuinfo.x86PType  = 0;
   cpuinfo.x86StepID = 0;
   cpuinfo.x86Flags  = 0;
   cpuinfo.x86EFlags = 0;
   
   if ( iCpuId( 0, regs ) == -1 ) return;

   cmds = regs[ 0 ];
   ((u32*)cpuinfo.x86ID)[ 0 ] = regs[ 1 ];
   ((u32*)cpuinfo.x86ID)[ 1 ] = regs[ 3 ];
   ((u32*)cpuinfo.x86ID)[ 2 ] = regs[ 2 ];
   
   // Hack - prevents reg[2] & reg[3] from being optimized out of existance!
   num = sprintf(str, "\tx86Flags  =  %8.8x %8.8x\n", regs[3], regs[2]);
   
   u32 LogicalCoresPerPhysicalCPU = 0;
   u32 PhysicalCoresPerPhysicalCPU = 1;

   if ( cmds >= 0x00000001 ) 
   {
      if ( iCpuId( 0x00000001, regs ) != -1 )
      {
         cpuinfo.x86StepID =  regs[ 0 ]        & 0xf;
         cpuinfo.x86Model  = (regs[ 0 ] >>  4) & 0xf;
         cpuinfo.x86Family = (regs[ 0 ] >>  8) & 0xf;
         cpuinfo.x86PType  = (regs[ 0 ] >> 12) & 0x3;
		 LogicalCoresPerPhysicalCPU = ( regs[1] >> 16 ) & 0xff;
         x86_64_8BITBRANDID = regs[1] & 0xff;
         cpuinfo.x86Flags  =  regs[ 3 ];
         cpuinfo.x86Flags2 =  regs[ 2 ];
      }
   }
   /* detect multicore for intel cpu */
   if ((cmds >= 0x00000004) && !strcmp("GenuineIntel",cpuinfo.x86ID))
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
			cpuinfo.x86EFlags2 = regs[ 2 ];
            cpuinfo.x86EFlags = regs[ 3 ];
            
         }
      }
      /* detect multicore for amd cpu */
      if ((cmds >= 0x80000008) && !strcmp("AuthenticAMD",cpuinfo.x86ID))
      {
         if ( iCpuId( 0x80000008, regs ) != -1 )
         {
            PhysicalCoresPerPhysicalCPU += ( regs[2] ) & 0xff;
         }
      }
   }
   
	switch(cpuinfo.x86PType)
	{
		case 0:
			strcpy( cpuinfo.x86Type, "Standard OEM");
		break;
		case 1:
			strcpy( cpuinfo.x86Type, "Overdrive");
		break;
		case 2:
			strcpy( cpuinfo.x86Type, "Dual");
		break;
		case 3:
			strcpy( cpuinfo.x86Type, "Reserved");
		break;
		default:
			strcpy( cpuinfo.x86Type, "Unknown");
		break;
	}
	if ( cpuinfo.x86ID[ 0 ] == 'G' ){ cputype=0;}//trick lines but if you know a way better ;p
	if ( cpuinfo.x86ID[ 0 ] == 'A' ){ cputype=1;}
   
	memzero_obj( cpuinfo.x86Fam );
	iCpuId( 0x80000002, (u32*)cpuinfo.x86Fam);
	iCpuId( 0x80000003, (u32*)(cpuinfo.x86Fam+16));
	iCpuId( 0x80000004, (u32*)(cpuinfo.x86Fam+32));

	//capabilities
	cpucaps.hasFloatingPointUnit                         = ( cpuinfo.x86Flags >>  0 ) & 1;
	cpucaps.hasVirtual8086ModeEnhancements               = ( cpuinfo.x86Flags >>  1 ) & 1;
	cpucaps.hasDebuggingExtensions                       = ( cpuinfo.x86Flags >>  2 ) & 1;
	cpucaps.hasPageSizeExtensions                        = ( cpuinfo.x86Flags >>  3 ) & 1;
	cpucaps.hasTimeStampCounter                          = ( cpuinfo.x86Flags >>  4 ) & 1;
	cpucaps.hasModelSpecificRegisters                    = ( cpuinfo.x86Flags >>  5 ) & 1;
	cpucaps.hasPhysicalAddressExtension                  = ( cpuinfo.x86Flags >>  6 ) & 1;
	cpucaps.hasMachineCheckArchitecture                  = ( cpuinfo.x86Flags >>  7 ) & 1;
	cpucaps.hasCOMPXCHG8BInstruction                     = ( cpuinfo.x86Flags >>  8 ) & 1;
	cpucaps.hasAdvancedProgrammableInterruptController   = ( cpuinfo.x86Flags >>  9 ) & 1;
	cpucaps.hasSEPFastSystemCall                         = ( cpuinfo.x86Flags >> 11 ) & 1;
	cpucaps.hasMemoryTypeRangeRegisters                  = ( cpuinfo.x86Flags >> 12 ) & 1;
	cpucaps.hasPTEGlobalFlag                             = ( cpuinfo.x86Flags >> 13 ) & 1;
	cpucaps.hasMachineCheckArchitecture                  = ( cpuinfo.x86Flags >> 14 ) & 1;
	cpucaps.hasConditionalMoveAndCompareInstructions     = ( cpuinfo.x86Flags >> 15 ) & 1;
	cpucaps.hasFGPageAttributeTable                      = ( cpuinfo.x86Flags >> 16 ) & 1;
	cpucaps.has36bitPageSizeExtension                    = ( cpuinfo.x86Flags >> 17 ) & 1;
	cpucaps.hasProcessorSerialNumber                     = ( cpuinfo.x86Flags >> 18 ) & 1;
	cpucaps.hasCFLUSHInstruction                         = ( cpuinfo.x86Flags >> 19 ) & 1;
	cpucaps.hasDebugStore                                = ( cpuinfo.x86Flags >> 21 ) & 1;
	cpucaps.hasACPIThermalMonitorAndClockControl         = ( cpuinfo.x86Flags >> 22 ) & 1;
	cpucaps.hasMultimediaExtensions                      = ( cpuinfo.x86Flags >> 23 ) & 1; //mmx
	cpucaps.hasFastStreamingSIMDExtensionsSaveRestore    = ( cpuinfo.x86Flags >> 24 ) & 1;
	cpucaps.hasStreamingSIMDExtensions                   = ( cpuinfo.x86Flags >> 25 ) & 1; //sse
	cpucaps.hasStreamingSIMD2Extensions                  = ( cpuinfo.x86Flags >> 26 ) & 1; //sse2
	cpucaps.hasSelfSnoop                                 = ( cpuinfo.x86Flags >> 27 ) & 1;
	cpucaps.hasMultiThreading                            = ( cpuinfo.x86Flags >> 28 ) & 1;
	cpucaps.hasThermalMonitor                            = ( cpuinfo.x86Flags >> 29 ) & 1;
	cpucaps.hasIntel64BitArchitecture                    = ( cpuinfo.x86Flags >> 30 ) & 1;
	
	//that is only for AMDs
	cpucaps.hasMultimediaExtensionsExt                   = ( cpuinfo.x86EFlags >> 22 ) & 1; //mmx2
	cpucaps.hasAMD64BitArchitecture                      = ( cpuinfo.x86EFlags >> 29 ) & 1; //64bit cpu
	cpucaps.has3DNOWInstructionExtensionsExt             = ( cpuinfo.x86EFlags >> 30 ) & 1; //3dnow+
	cpucaps.has3DNOWInstructionExtensions                = ( cpuinfo.x86EFlags >> 31 ) & 1; //3dnow   
	cpucaps.hasStreamingSIMD4ExtensionsA                 = ( cpuinfo.x86EFlags2 >> 6 ) & 1; //INSERTQ / EXTRQ / MOVNT

	InitCPUTicks();
	uint span = GetTickFrequency();

	if( (span % 1000) < 400 )	// helps minimize rounding errors
		cpuinfo.cpuspeed = (u32)( CPUSpeedHz( span / 1000 ) / 1000 );
	else
		cpuinfo.cpuspeed = (u32)( CPUSpeedHz( span / 500 ) / 2000 );

	// --> SSE3 / SSSE3 / SSE4.1 / SSE 4.2 detection <--

	cpucaps.hasStreamingSIMD3Extensions  = ( cpuinfo.x86Flags2 >> 0 ) & 1; //sse3
	cpucaps.hasSupplementalStreamingSIMD3Extensions = ( cpuinfo.x86Flags2 >> 9 ) & 1; //ssse3  
	cpucaps.hasStreamingSIMD4Extensions  = ( cpuinfo.x86Flags2 >> 19 ) & 1; //sse4.1   
	cpucaps.hasStreamingSIMD4Extensions2 = ( cpuinfo.x86Flags2 >> 20 ) & 1; //sse4.2

	// Can the SSE3 / SSE4.1 bits be trusted?  Using an instruction test is a very "complete"
	// approach to ensuring the bit is accurate, and at least one reported case of a Q9550 not
	// having SSE 4.1 set but still supporting it properly is fixed by this --air

	#ifdef _MSC_VER
	u8* recSSE = (u8*)HostSys::Mmap( NULL, 0x1000 );
	if( recSSE != NULL )
	{
		xSetPtr( recSSE );
		xMOVSLDUP( xmm1, xmm0 );
		RET();

		u8* funcSSSE3 = xGetPtr();
		xPABS.W( xmm0, xmm1 );
		RET();

		u8* funcSSE41 = xGetPtr();
		xBLEND.VPD( xmm1, xmm0 );
		RET();
		
		bool sse3_result = _test_instruction( recSSE );  // sse3
		bool ssse3_result = _test_instruction( funcSSSE3 );
		bool sse41_result = _test_instruction( funcSSE41 );

		HostSys::Munmap( recSSE, 0x1000 );

		// Test for and log any irregularities here.
		// We take the instruction test result over cpuid since (in theory) it should be a
		// more reliable gauge of the cpu's actual ability.

		if( sse3_result != !!cpucaps.hasStreamingSIMD3Extensions )
		{
			Console::Notice( "SSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				params bool_to_char( !!cpucaps.hasStreamingSIMD3Extensions ), bool_to_char( sse3_result ) );
				
			cpucaps.hasStreamingSIMD3Extensions = sse3_result;
		}

		if( ssse3_result != !!cpucaps.hasSupplementalStreamingSIMD3Extensions )
		{
			Console::Notice( "SSSE3 Detection Inconsistency: cpuid=%s, test_result=%s",
				params bool_to_char( !!cpucaps.hasSupplementalStreamingSIMD3Extensions ), bool_to_char( ssse3_result ) );

			cpucaps.hasSupplementalStreamingSIMD3Extensions = ssse3_result;
		}

		if( sse41_result != !!cpucaps.hasStreamingSIMD4Extensions )
		{
			Console::Notice( "SSE4 Detection Inconsistency: cpuid=%s, test_result=%s",
				params bool_to_char( !!cpucaps.hasStreamingSIMD4Extensions ), bool_to_char( sse41_result ) );

			cpucaps.hasStreamingSIMD4Extensions = sse41_result;
		}

	}
	else
		Console::Notice(
			"Notice: Could not allocate memory for SSE3/4 detection.\n"
			"\tRelying on CPUID results. [this is not an error]"
		);
	#endif

	//////////////////////////////////////
	//  Core Counting!

	if( !cpucaps.hasMultiThreading || LogicalCoresPerPhysicalCPU == 0 )
		LogicalCoresPerPhysicalCPU = 1;

	// This will assign values into cpuinfo.LogicalCores and PhysicalCores
	Threading::CountLogicalCores( LogicalCoresPerPhysicalCPU, PhysicalCoresPerPhysicalCPU );
}

