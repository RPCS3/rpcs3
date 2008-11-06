
#if defined (__WIN32__)

#include <windows.h>

#endif

#include <memory.h>
#include <string.h>
#include <stdio.h>

#include "ix86.h"

#if defined (__VCNET2005__)

   void __cpuid(int* CPUInfo, int InfoType);
   unsigned __int64 __rdtsc();

   #pragma intrinsic(__cpuid)
   #pragma intrinsic(__rdtsc)

#endif

u32 hasFloatingPointUnit;
u32 hasVirtual8086ModeEnhancements;
u32 hasDebuggingExtensions;
u32 hasPageSizeExtensions;
u32 hasTimeStampCounter;
u32 hasModelSpecificRegisters;
u32 hasPhysicalAddressExtension;
u32 hasMachineCheckArchitecture;
u32 hasCOMPXCHG8BInstruction;
u32 hasAdvancedProgrammableInterruptController;
u32 hasSEPFastSystemCall;
u32 hasMemoryTypeRangeRegisters;
u32 hasPTEGlobalFlag;
u32 hasMachineCheckArchitecture;
u32 hasConditionalMoveAndCompareInstructions;
u32 hasFGPageAttributeTable;
u32 has36bitPageSizeExtension;
u32 hasProcessorSerialNumber;
u32 hasCFLUSHInstruction;
u32 hasDebugStore;
u32 hasACPIThermalMonitorAndClockControl;
u32 hasMultimediaExtensions;
u32 hasFastStreamingSIMDExtensionsSaveRestore;
u32 hasStreamingSIMDExtensions;
u32 hasStreamingSIMD2Extensions;
u32 hasSelfSnoop;
u32 hasHyperThreading;
u32 hasThermalMonitor;
u32 hasIntel64BitArchitecture;
//that is only for AMDs
u32 hasMultimediaExtensionsExt;
u32 hasAMD64BitArchitecture;
u32 has3DNOWInstructionExtensionsExt;
u32 has3DNOWInstructionExtensions;

s8  x86ID[16];	   // Vendor ID
u32 x86Family;	   // Processor Family
u32 x86Model;	   // Processor Model
u32 x86PType;	   // Processor Type
u32 x86StepID;	   // Stepping ID
u32 x86Flags;	   // Feature Flags
u32 x86EFlags;	   // Extended Feature Flags
//AMD 64 STUFF
u32 x86_64_8BITBRANDID;
u32 x86_64_12BITBRANDID;
s8  x86Type[20];   //cpu type in char format
s8  x86Fam[50];    // family in char format
u32 cpuspeed;      // speed of cpu
int cputype;            // Cpu type

static s32 iCpuId( u32 cmd, u32 *regs ) 
{
   int flag;

#if defined (__VCNET2005__)

   __cpuid( regs, cmd );

   return 0;

#elif defined (__MSCW32__) && !defined(__x86_64__)
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

   __asm__ __volatile__ (
#ifdef __x86_64__
	"sub $0x18, %%rsp\n"
#endif
      "pushf\n"
      "pop %%eax\n"
      "mov %%eax, %%edx\n"
      "xor $0x200000, %%eax\n"
      "push %%eax\n"
      "popf\n"
      "pushf\n"
      "pop %%eax\n"
      "xor %%edx, %%eax\n"
      "mov %%eax, %0\n"
#ifdef __x86_64__
	"add $0x18, %%rsp\n"
#endif
      : "=r"(flag) :
   );
   
   if ( ! flag )
   {
      return -1;
   }

   __asm__ __volatile__ (
      "mov %4, %%eax\n"
      "cpuid\n"
      "mov %%eax, %0\n"
      "mov %%ebx, %1\n"
      "mov %%ecx, %2\n"
      "mov %%edx, %3\n"
      : "=m" (regs[0]), "=m" (regs[1]),
        "=m" (regs[2]), "=m" (regs[3])
      : "m"(cmd)
      : "eax", "ebx", "ecx", "edx"
   );

   return 0;
#endif
}

u64 GetCPUTick( void ) 
{
#if defined (__VCNET2005__)

   return __rdtsc();

#elif defined(__MSCW32__) && !defined(__x86_64__)

   __asm rdtsc;

#else

   u32 _a, _d;
	__asm__ __volatile__ ("rdtsc" : "=a"(_a), "=d"(_d));
	return (u64)_a | ((u64)_d << 32);

#endif
}

#if defined __LINUX__

#include <sys/time.h>
#include <errno.h>

u32 timeGetTime( void ) 
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif

s64 CPUSpeedHz( unsigned int time )
{
   s64 timeStart, 
            timeStop;
   s64 startTick, 
            endTick;
   s64 overhead;

   if( ! hasTimeStampCounter )
   {
      return 0; //check if function is supported
   }

	overhead = GetCPUTick() - GetCPUTick();
	
	timeStart = timeGetTime( );
	while( timeGetTime( ) == timeStart ) 
   {
      timeStart = timeGetTime( );
   }
	while ( 1 )
	{
		timeStop = timeGetTime( );
		if ( ( timeStop - timeStart ) > 1 )	
		{
			startTick = GetCPUTick( );
			break;
		}
	}

	timeStart = timeStop;
	while ( 1 )
	{
		timeStop = timeGetTime( );
		if ( ( timeStop - timeStart ) > time )	
		{
			endTick = GetCPUTick( );
			break;
		}
	}

	return (s64)( ( endTick - startTick ) + ( overhead ) );
}
u32 AMDspeed;
s8 AMDspeedString[10];
////////////////////////////////////////////////////
void x86Init( void ) 
{
   u32 regs[ 4 ];
   u32 cmds;

   memset( x86ID, 0, sizeof( x86ID ) );
   x86Family = 0;
   x86Model  = 0;
   x86PType  = 0;
   x86StepID = 0;
   x86Flags  = 0;
   x86EFlags = 0;
   
   if ( iCpuId( 0, regs ) == -1 ) return;

   cmds = regs[ 0 ];
   ((u32*)x86ID)[ 0 ] = regs[ 1 ];
   ((u32*)x86ID)[ 1 ] = regs[ 3 ];
   ((u32*)x86ID)[ 2 ] = regs[ 2 ];
   if ( cmds >= 0x00000001 ) 
   {
      if ( iCpuId( 0x00000001, regs ) != -1 )
      {
         x86StepID =  regs[ 0 ]        & 0xf;
         x86Model  = (regs[ 0 ] >>  4) & 0xf;
         x86Family = (regs[ 0 ] >>  8) & 0xf;
         x86PType  = (regs[ 0 ] >> 12) & 0x3;
         x86_64_8BITBRANDID = regs[1] & 0xff;
         x86Flags  =  regs[ 3 ];
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
            x86EFlags = regs[ 3 ];
            
         }
      }
   }
   switch(x86PType)
   {
      case 0:
         strcpy( x86Type, "Standard OEM");
         break;
      case 1:
         strcpy( x86Type, "Overdrive");
         break;
      case 2:
         strcpy( x86Type, "Dual");
         break;
      case 3:
         strcpy( x86Type, "Reserved");
         break;
      default:
         strcpy( x86Type, "Unknown");
         break;
   }
   if ( x86ID[ 0 ] == 'G' ){ cputype=0;}//trick lines but if you know a way better ;p
   if ( x86ID[ 0 ] == 'A' ){ cputype=1;}
   
   if ( cputype == 0 ) //intel cpu
   {
      if( ( x86Family >= 7 ) && ( x86Family < 15 ) )
      {
         strcpy( x86Fam, "Intel P6 family (Not PIV and Higher then PPro" );
      }
      else
      {
         switch( x86Family )
         {     
            // Start at 486 because if it's below 486 there is no cpuid instruction
            case 4:
               strcpy( x86Fam, "Intel 486" );
               break;
            case 5:     
               switch( x86Model )
               {
               case 4:
               case 8:     // 0.25 µm
                  strcpy( x86Fam, "Intel Pentium (MMX)");
                  break;
               default:
                  strcpy( x86Fam, "Intel Pentium" );
               }
               break;
            case 6:     
               switch( x86Model )
               {
               case 0:     // Pentium pro (P6 A-Step)
               case 1:     // Pentium pro
                  strcpy( x86Fam, "Intel Pentium Pro" );
                  break;

               case 2:     // 66 MHz FSB
               case 5:     // Xeon/Celeron (0.25 µm)
               case 6:     // Internal L2 cache
                  strcpy( x86Fam, "Intel Pentium II" );
                  break;

               case 7:     // Xeon external L2 cache
               case 8:     // Xeon/Celeron with 256 KB on-die L2 cache
               case 10:    // Xeon/Celeron with 1 or 2 MB on-die L2 cache
               case 11:    // Xeon/Celeron with Tualatin core, on-die cache
                  strcpy( x86Fam, "Intel Pentium III" );
                  break;

               default:
                  strcpy( x86Fam, "Intel Pentium Pro (Unknown)" );
               }
               break;
            case 15:
               switch( x86Model )
               {
               case 0:     // Willamette (A-Step)
               case 1:     // Willamette 
                  strcpy( x86Fam, "Willamette Intel Pentium IV" );
                  break;
               case 2:     // Northwood 
                  strcpy( x86Fam, "Northwood Intel Pentium IV" );
                  break;

               default:
                  strcpy( x86Fam, "Intel Pentium IV (Unknown)" );
                  break;
               }
               break;
            default:
               strcpy( x86Fam, "Unknown Intel CPU" );
         }
      }
   }
   else if ( cputype == 1 ) //AMD cpu
   {
      if( x86Family >= 7 )
      {
		  if((x86_64_12BITBRANDID !=0) || (x86_64_8BITBRANDID !=0))
		  {
		    if(x86_64_8BITBRANDID == 0 )
		    {
               switch((x86_64_12BITBRANDID >>6)& 0x3f)
			   {
			    case 4:
				 strcpy(x86Fam,"AMD Athlon(tm) 64 Processor");
                 AMDspeed = 22 + (x86_64_12BITBRANDID & 0x1f);
				 //AMDspeedString = strtol(AMDspeed, (char**)NULL,10);
				 sprintf(AMDspeedString," %d",AMDspeed);
				 strcat(AMDspeedString,"00+");
				 strcat(x86Fam,AMDspeedString);
				 break;
			    case 12: 
				 strcpy(x86Fam,"AMD Opteron(tm) Processor");
				 break;
			    default:
				   strcpy(x86Fam,"Unknown AMD 64 proccesor");
				   
			    }
		     }
		     else //8bit brand id is non zero
		     {
                strcpy(x86Fam,"Unsupported yet AMD64 cpu");
		     }
		  }
		  else
		  {		 
			  strcpy( x86Fam, "AMD K7+" );
		  }
      }
      else
      {
         switch ( x86Family )
         {
            case 4:
               switch( x86Model )
               {
               case 14: 
               case 15:       // Write-back enhanced
                  strcpy( x86Fam, "AMD 5x86" );
                  break;

               case 3:        // DX2
               case 7:        // Write-back enhanced DX2
               case 8:        // DX4
               case 9:        // Write-back enhanced DX4
                  strcpy( x86Fam, "AMD 486" );
                  break;

               default:
                  strcpy( x86Fam, "AMD Unknown" );

               }
               break;

            case 5:     
               switch( x86Model)
               {
               case 0:     // SSA 5 (75, 90 and 100 Mhz)
               case 1:     // 5k86 (PR 120 and 133 MHz)
               case 2:     // 5k86 (PR 166 MHz)
               case 3:     // K5 5k86 (PR 200 MHz)
                  strcpy( x86Fam, "AMD K5" );
                  break;

               case 6:     
               case 7:     // (0.25 µm)
               case 8:     // K6-2
               case 9:     // K6-III
               case 14:    // K6-2+ / K6-III+
                  strcpy( x86Fam, "AMD K6" );
                  break;

               default:
                  strcpy( x86Fam, "AMD Unknown" );
               }
               break;
            case 6:     
               strcpy( x86Fam, "AMD K7" );
               break;
            default:
               strcpy( x86Fam, "Unknown AMD CPU" ); 
         }
      }
   }
   //capabilities
   hasFloatingPointUnit                         = ( x86Flags >>  0 ) & 1;
   hasVirtual8086ModeEnhancements               = ( x86Flags >>  1 ) & 1;
   hasDebuggingExtensions                       = ( x86Flags >>  2 ) & 1;
   hasPageSizeExtensions                        = ( x86Flags >>  3 ) & 1;
   hasTimeStampCounter                          = ( x86Flags >>  4 ) & 1;
   hasModelSpecificRegisters                    = ( x86Flags >>  5 ) & 1;
   hasPhysicalAddressExtension                  = ( x86Flags >>  6 ) & 1;
   hasMachineCheckArchitecture                  = ( x86Flags >>  7 ) & 1;
   hasCOMPXCHG8BInstruction                     = ( x86Flags >>  8 ) & 1;
   hasAdvancedProgrammableInterruptController   = ( x86Flags >>  9 ) & 1;
   hasSEPFastSystemCall                         = ( x86Flags >> 11 ) & 1;
   hasMemoryTypeRangeRegisters                  = ( x86Flags >> 12 ) & 1;
   hasPTEGlobalFlag                             = ( x86Flags >> 13 ) & 1;
   hasMachineCheckArchitecture                  = ( x86Flags >> 14 ) & 1;
   hasConditionalMoveAndCompareInstructions     = ( x86Flags >> 15 ) & 1;
   hasFGPageAttributeTable                      = ( x86Flags >> 16 ) & 1;
   has36bitPageSizeExtension                    = ( x86Flags >> 17 ) & 1;
   hasProcessorSerialNumber                     = ( x86Flags >> 18 ) & 1;
   hasCFLUSHInstruction                         = ( x86Flags >> 19 ) & 1;
   hasDebugStore                                = ( x86Flags >> 21 ) & 1;
   hasACPIThermalMonitorAndClockControl         = ( x86Flags >> 22 ) & 1;
   hasMultimediaExtensions                      = ( x86Flags >> 23 ) & 1; //mmx
   hasFastStreamingSIMDExtensionsSaveRestore    = ( x86Flags >> 24 ) & 1;
   hasStreamingSIMDExtensions                   = ( x86Flags >> 25 ) & 1; //sse
   hasStreamingSIMD2Extensions                  = ( x86Flags >> 26 ) & 1; //sse2
   hasSelfSnoop                                 = ( x86Flags >> 27 ) & 1;
   hasHyperThreading                            = ( x86Flags >> 28 ) & 1;
   hasThermalMonitor                            = ( x86Flags >> 29 ) & 1;
   hasIntel64BitArchitecture                    = ( x86Flags >> 30 ) & 1;
    //that is only for AMDs
   hasMultimediaExtensionsExt                   = ( x86EFlags >> 22 ) & 1; //mmx2
   hasAMD64BitArchitecture                      = ( x86EFlags >> 29 ) & 1; //64bit cpu
   has3DNOWInstructionExtensionsExt             = ( x86EFlags >> 30 ) & 1; //3dnow+
   has3DNOWInstructionExtensions                = ( x86EFlags >> 31 ) & 1; //3dnow

	cpuspeed = (u32 )(CPUSpeedHz( 1000 ) / 1000000);
}
