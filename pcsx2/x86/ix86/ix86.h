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
/*
 * ix86 definitions v0.6.2
 *  Authors: linuzappz <linuzappz@pcsx.net>
 *           alexey silinov
 *           goldfinger
 *           shadow < shadow@pcsx2.net >
 *			 cottonvibes(@gmail.com)
 */

#pragma once
#include "PrecompiledHeader.h"
#include "PS2Etypes.h"       // Basic types header

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define emitterT template<int I/*Emitter Index*/> inline /*__forceinline*/

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0);
#define RexR(w, reg) if( w||(reg)>=8 ) assert(0);
#define RexB(w, base) if( w||(base)>=8 ) assert(0);
#define RexRB(w, reg, base) if( w||(reg) >= 8 || (base)>=8 ) assert(0);
#define RexRXB(w, reg, index, base) if( w||(reg) >= 8 || (index) >= 8 || (base) >= 8 ) assert(0);

#define XMMREGS 8
#define X86REGS 8

#define MMXREGS 8

#define SIB 4
#define SIBDISP 5
#define DISP32 5

// general types
typedef int x86IntRegType;
#define EAX 0
#define EBX 3
#define ECX 1
#define EDX 2
#define ESI 6
#define EDI 7
#define EBP 5
#define ESP 4

#define X86ARG1 EAX
#define X86ARG2 ECX
#define X86ARG3 EDX
#define X86ARG4 EBX

#define MM0 0
#define MM1 1
#define MM2 2
#define MM3 3
#define MM4 4
#define MM5 5
#define MM6 6
#define MM7 7

typedef int x86MMXRegType;

#define XMM0 0 
#define XMM1 1 
#define XMM2 2 
#define XMM3 3 
#define XMM4 4 
#define XMM5 5 
#define XMM6 6 
#define XMM7 7 
#define XMM8 8 
#define XMM9 9 
#define XMM10 10 
#define XMM11 11 
#define XMM12 12 
#define XMM13 13 
#define XMM14 14 
#define XMM15 15

typedef int x86SSERegType;

enum XMMSSEType
{
	XMMT_INT = 0, // integer (sse2 only)
	XMMT_FPS = 1, // floating point
	//XMMT_FPD = 3, // double
};

extern XMMSSEType g_xmmtypes[XMMREGS];

extern void cpudetectInit( void );//this is all that needs to be called and will fill up the below structs

typedef struct CAPABILITIES CAPABILITIES;
//cpu capabilities structure
struct CAPABILITIES {
   u32 hasFloatingPointUnit;
   u32 hasVirtual8086ModeEnhancements;
   u32 hasDebuggingExtensions;
   u32 hasPageSizeExtensions;
   u32 hasTimeStampCounter;
   u32 hasModelSpecificRegisters;
   u32 hasPhysicalAddressExtension;
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
   u32 hasMultiThreading;			// is TRUE for both mutli-core and Hyperthreaded CPUs.
   u32 hasThermalMonitor;
   u32 hasIntel64BitArchitecture;
   u32 hasStreamingSIMD3Extensions;
   u32 hasSupplementalStreamingSIMD3Extensions;
   u32 hasStreamingSIMD4Extensions;

   // AMD-specific CPU Features
   u32 hasMultimediaExtensionsExt;
   u32 hasAMD64BitArchitecture;
   u32 has3DNOWInstructionExtensionsExt;
   u32 has3DNOWInstructionExtensions;
};

extern CAPABILITIES cpucaps;

struct CPUINFO{
   
   u32 x86Family;	   // Processor Family
   u32 x86Model;	   // Processor Model
   u32 x86PType;	   // Processor Type
   u32 x86StepID;	   // Stepping ID
   u32 x86Flags;	   // Feature Flags
   u32 x86Flags2;	   // More Feature Flags
   u32 x86EFlags;	   // Extended Feature Flags

   u32 PhysicalCores;
   u32 LogicalCores;

   char x86ID[16];	   // Vendor ID  //the vendor creator (in %s)
   char x86Type[20];   //cpu type in char format //the cpu type (in %s)
   char x86Fam[50];    // family in char format //the original cpu name string (in %s)
   u32 cpuspeed;      // speed of cpu //this will give cpu speed (in %d)
};

extern CPUINFO cpuinfo;
//------------------------------------------------------------------

//------------------------------------------------------------------
// write functions
//------------------------------------------------------------------
extern u8  *x86Ptr[3];
extern u8  *j8Ptr[32];
extern u32 *j32Ptr[32];

emitterT void write8(u8 val ) {  
	*x86Ptr[I] = (u8)val; 
	x86Ptr[I]++; 
} 

emitterT void write16(u16 val ) { 
	*(u16*)x86Ptr[I] = (u16)val; 
	x86Ptr[I] += 2;  
} 

emitterT void write24(u32 val ) { 
	*(u8*)x86Ptr[I] = (u8)(val & 0xff); 
	x86Ptr[I]++;  
	*(u8*)x86Ptr[I] = (u8)((val >> 8) & 0xff); 
	x86Ptr[I]++;  
	*(u8*)x86Ptr[I] = (u8)((val >> 16) & 0xff); 
	x86Ptr[I]++;  
} 

emitterT void write32(u32 val ) { 
	*(u32*)x86Ptr[I] = val; 
	x86Ptr[I] += 4; 
} 

emitterT void write64( u64 val ){ 
	*(u64*)x86Ptr[I] = val; 
	x86Ptr[I] += 8; 
}
//------------------------------------------------------------------
/*
//------------------------------------------------------------------
// jump/align functions
//------------------------------------------------------------------
emitterT void ex86SetPtr( u8 *ptr );
emitterT void ex86SetJ8( u8 *j8 );
emitterT void ex86SetJ8A( u8 *j8 );
emitterT void ex86SetJ16( u16 *j16 );
emitterT void ex86SetJ16A( u16 *j16 );
emitterT void ex86SetJ32( u32 *j32 );
emitterT void ex86SetJ32A( u32 *j32 );
emitterT void ex86Align( int bytes );
emitterT void ex86AlignExecutable( int align );
//------------------------------------------------------------------

//------------------------------------------------------------------
// General Emitter Helper functions
//------------------------------------------------------------------
emitterT void WriteRmOffset(x86IntRegType to, int offset);
emitterT void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset);
emitterT void ModRM( int mod, int reg, int rm );
emitterT void SibSB( int ss, int index, int base );
emitterT void SET8R( int cc, int to );
emitterT void CMOV32RtoR( int cc, int to, int from );
emitterT void CMOV32MtoR( int cc, int to, uptr from );
emitterT u8*  J8Rel( int cc, int to );
emitterT u32* J32Rel( int cc, u32 to );
emitterT u64  GetCPUTick( void );
//------------------------------------------------------------------
*/
#define MMXONLY(code) code
#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))
extern void SysPrintf(const char *fmt, ...);

#include "ix86_macros.h"
#include "ix86.inl"
#include "ix86_3dnow.inl"
#include "ix86_fpu.inl"
#include "ix86_mmx.inl"
#include "ix86_sse.inl"
