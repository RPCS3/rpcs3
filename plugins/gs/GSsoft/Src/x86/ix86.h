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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
/*
 * ix86 definitions v0.6.2
 *  Authors: linuzappz <linuzappz@pcsx.net>
 *           alexey silinov
 *           goldfinger
 *           shadow < shadow@pcsx2.net >
 */

#ifndef __IX86_H__
#define __IX86_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "PS2Etypes.h"       // Basic types header

#ifdef __x86_64__
#define XMMREGS 16
#define X86REGS 16
#else
#define XMMREGS 8
#define X86REGS 8
#endif

#define MMXREGS 8

#define SIB 4
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

#ifdef __x86_64__
#define RAX 0
#define RBX 3
#define RCX 1
#define RDX 2
#define RSI 6
#define RDI 7
#define RBP 5
#define RSP 4
#define R8  8 
#define R9  9 
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

#define X86_TEMP RAX // don't allocate anything

#ifdef _MSC_VER
extern x86IntRegType g_x86savedregs[8];
extern x86IntRegType g_x86tempregs[6];
#else
extern x86IntRegType g_x86savedregs[6];
extern x86IntRegType g_x86tempregs[8];
#endif

extern x86IntRegType g_x86allregs[14]; // all registers that can be used by the recs
extern x86IntRegType g_x868bitregs[11];
extern x86IntRegType g_x86non8bitregs[3];

#ifdef _MSC_VER
#define X86ARG1 RCX
#define X86ARG2 RDX
#define X86ARG3 R8
#define X86ARG4 R9
#else
#define X86ARG1 RDI
#define X86ARG2 RSI
#define X86ARG3 RDX
#define X86ARG4 RCX
#endif

#else

#define X86ARG1 EAX
#define X86ARG2 ECX
#define X86ARG3 EDX
#define X86ARG4 EBX

#endif // __x86_64__

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

typedef enum
{
	XMMT_INT = 0, // integer (sse2 only)
	XMMT_FPS = 1, // floating point
	//XMMT_FPD = 3, // double
} XMMSSEType;

extern XMMSSEType g_xmmtypes[XMMREGS];

void cpudetectInit( void );//this is all that needs to be called and will fill up the below structs

//cpu capabilities structure
typedef struct {
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
   u32 hasHyperThreading;
   u32 hasThermalMonitor;
   u32 hasIntel64BitArchitecture;
   u32 hasStreamingSIMD3Extensions;
   u32 hasStreamingSIMD4Extensions;
   //that is only for AMDs
   u32 hasMultimediaExtensionsExt;
   u32 hasAMD64BitArchitecture;
   u32 has3DNOWInstructionExtensionsExt;
   u32 has3DNOWInstructionExtensions;
} CAPABILITIES;

extern CAPABILITIES cpucaps;

typedef struct {
   
   u32 x86Family;	   // Processor Family
   u32 x86Model;	   // Processor Model
   u32 x86PType;	   // Processor Type
   u32 x86StepID;	   // Stepping ID
   u32 x86Flags;	   // Feature Flags
   u32 x86Flags2;	   // More Feature Flags
   u32 x86EFlags;	   // Extended Feature Flags
   //all the above returns hex values
   s8  x86ID[16];	   // Vendor ID  //the vendor creator (in %s)
   s8  x86Type[20];   //cpu type in char format //the cpu type (in %s)
   s8  x86Fam[50];    // family in char format //the original cpu name string (in %s)
   u32 cpuspeed;      // speed of cpu //this will give cpu speed (in %d)
} CPUINFO;

extern CPUINFO cpuinfo;

extern s8  *x86Ptr;
extern u8  *j8Ptr[32];
extern u32 *j32Ptr[32];


#ifdef __x86_64__
#define X86_64ASSERT() assert(0)
#define MEMADDR(addr, oplen)	((addr) - ((u64)x86Ptr + ((u64)oplen)))
#else
#define X86_64ASSERT()
#define MEMADDR(addr, oplen)	(addr)
#endif

#ifdef __x86_64__
#define Rex( w, r, x, b ) write8( 0x40 | ((w) << 3) | ((r) << 2) | ((x) << 1) | (b) );
#define RexR(w, reg) if( w||(reg)>=8 ) { Rex(w, (reg)>=8, 0, 0); }
#define RexB(w, base) if( w||(base)>=8 ) { Rex(w, 0, 0, (base)>=8); }
#define RexRB(w, reg, base) if( w || (reg) >= 8 || (base)>=8 ) { Rex(w, (reg)>=8, 0, (base)>=8); }
#define RexRXB(w, reg, index, base) if( w||(reg) >= 8 || (index) >= 8 || (base) >= 8 ) { \
        Rex(w, (reg)>=8, (index)>=8, (base)>=8);                           \
    }
#else
#define Rex(w,r,x,b) assert(0);
#define RexR(w, reg) if( w||(reg)>=8 ) assert(0);
#define RexB(w, base) if( w||(base)>=8 ) assert(0);
#define RexRB(w, reg, base) if( w||(reg) >= 8 || (base)>=8 ) assert(0);
#define RexRXB(w, reg, index, base) if( w||(reg) >= 8 || (index) >= 8 || (base) >= 8 ) assert(0);
#endif

void write8( int val );
void write16( int val );
void write32( u32 val );
void write64( u64 val );


void x86SetPtr( char *ptr );
void x86Shutdown( void );

void x86SetJ8( u8 *j8 );
void x86SetJ8A( u8 *j8 );
void x86SetJ16( u16 *j16 );
void x86SetJ16A( u16 *j16 );
void x86SetJ32( u32 *j32 );
void x86SetJ32A( u32 *j32 );

void x86Align( int bytes );
u64 GetCPUTick( void );

// General Helper functions
void ModRM( int mod, int rm, int reg );
void SibSB( int ss, int rm, int index );
void SET8R( int cc, int to );
u8* J8Rel( int cc, int to );
u32* J32Rel( int cc, u32 to );
void CMOV32RtoR( int cc, int to, int from );
void CMOV32MtoR( int cc, int to, uptr from );

//******************
// IX86 intructions 
//******************

//
// * scale values:
// *  0 - *1
// *  1 - *2
// *  2 - *4
// *  3 - *8
// 

void STC( void );
void CLC( void );

////////////////////////////////////
// mov instructions               //
////////////////////////////////////

// mov r64 to r64 
void MOV64RtoR( x86IntRegType to, x86IntRegType from );
// mov r64 to m64 
void MOV64RtoM( uptr to, x86IntRegType from );
// mov m64 to r64 
void MOV64MtoR( x86IntRegType to, uptr from );
// mov sign ext imm32 to m64 
void MOV64I32toM( uptr to, u32 from );
// mov sign ext imm32 to r64
void MOV64I32toR( x86IntRegType to, s32 from);
// mov imm64 to r64
void MOV64ItoR( x86IntRegType to, u64 from);
// mov imm64 to [r64+off]
void MOV64ItoRmOffset( x86IntRegType to, u32 from, int offset);
// mov [r64+offset] to r64
void MOV64RmOffsettoR( x86IntRegType to, x86IntRegType from, int offset );
// mov [r64][r64*scale] to r64
void MOV64RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale);
// mov r64 to [r64+offset]
void MOV64RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset );
// mov r64 to [r64][r64*scale]
void MOV64RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale);

// mov r32 to r32 
void MOV32RtoR( x86IntRegType to, x86IntRegType from );
// mov r32 to m32 
void MOV32RtoM( uptr to, x86IntRegType from );
// mov m32 to r32 
void MOV32MtoR( x86IntRegType to, uptr from );
// mov [r32] to r32 
void MOV32RmtoR( x86IntRegType to, x86IntRegType from );
void MOV32RmtoROffset( x86IntRegType to, x86IntRegType from, int offset );
// mov [r32][r32<<scale] to r32 
void MOV32RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale );
// mov [imm32(from2) + r32(from1)<<scale] to r32 
void MOV32RmSOffsettoR( x86IntRegType to, x86IntRegType from1, int from2, int scale );
// mov r32 to [r32] 
void MOV32RtoRm( x86IntRegType to, x86IntRegType from );
// mov r32 to [r32][r32*scale]
void MOV32RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale );
// mov imm32 to r32 
void MOV32ItoR( x86IntRegType to, u32 from );
// mov imm32 to m32 
void MOV32ItoM( uptr to, u32 from );
// mov imm32 to [r32+off]
void MOV32ItoRmOffset( x86IntRegType to, u32 from, int offset);
// mov r32 to [r32+off]
void MOV32RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset);

// mov r16 to m16 
void MOV16RtoM( uptr to, x86IntRegType from );
// mov m16 to r16 
void MOV16MtoR( x86IntRegType to, uptr from );
// mov [r32] to r16
void MOV16RmtoR( x86IntRegType to, x86IntRegType from ) ;
void MOV16RmtoROffset( x86IntRegType to, x86IntRegType from, int offset );
// mov [imm32(from2) + r32(from1)<<scale] to r16 
void MOV16RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale );
// mov r16 to [r32]
void MOV16RtoRm(x86IntRegType to, x86IntRegType from);
// mov imm16 to m16 
void MOV16ItoM( uptr to, u16 from );
/* mov r16 to [r32][r32*scale] */
void MOV16RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale);
// mov imm16 to r16
void MOV16ItoR( x86IntRegType to, u16 from );
// mov imm16 to [r16+off]
void MOV16ItoRmOffset( x86IntRegType to, u16 from, u32 offset);
// mov r16 to [r16+off]
void MOV16RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset);

// mov r8 to m8 
void MOV8RtoM( uptr to, x86IntRegType from );
// mov m8 to r8 
void MOV8MtoR( x86IntRegType to, uptr from );
// mov [r32] to r8
void MOV8RmtoR(x86IntRegType to, x86IntRegType from);
void MOV8RmtoROffset(x86IntRegType to, x86IntRegType from, int offset);
// mov r8 to [r32]
void MOV8RtoRm(x86IntRegType to, x86IntRegType from);
// mov imm8 to m8 
void MOV8ItoM( uptr to, u8 from );
// mov imm8 to r8
void MOV8ItoR( x86IntRegType to, u8 from );
// mov imm8 to [r8+off]
void MOV8ItoRmOffset( x86IntRegType to, u8 from, int offset);
// mov r8 to [r8+off]
void MOV8RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset);

// movsx r8 to r32 
void MOVSX32R8toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm8toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm8toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movsx m8 to r32 
void MOVSX32M8toR( x86IntRegType to, u32 from );
// movsx r16 to r32 
void MOVSX32R16toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm16toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm16toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movsx m16 to r32 
void MOVSX32M16toR( x86IntRegType to, u32 from );

// movzx r8 to r32 
void MOVZX32R8toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm8toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm8toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movzx m8 to r32 
void MOVZX32M8toR( x86IntRegType to, u32 from );
// movzx r16 to r32 
void MOVZX32R16toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm16toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm16toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movzx m16 to r32 
void MOVZX32M16toR( x86IntRegType to, u32 from );

#ifdef __x86_64__
void MOVZX64R8toR( x86IntRegType to, x86IntRegType from );
void MOVZX64Rm8toR( x86IntRegType to, x86IntRegType from );
void MOVZX64Rm8toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movzx m8 to r64
void MOVZX64M8toR( x86IntRegType to, u32 from );
// movzx r16 to r64 
void MOVZX64R16toR( x86IntRegType to, x86IntRegType from );
void MOVZX64Rm16toR( x86IntRegType to, x86IntRegType from );
void MOVZX64Rm16toROffset( x86IntRegType to, x86IntRegType from, int offset );
// movzx m16 to r64
void MOVZX64M16toR( x86IntRegType to, u32 from );
#endif

// cmovbe r32 to r32 
void CMOVBE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovbe m32 to r32
void CMOVBE32MtoR( x86IntRegType to, uptr from );
// cmovb r32 to r32 
void CMOVB32RtoR( x86IntRegType to, x86IntRegType from );
// cmovb m32 to r32
void CMOVB32MtoR( x86IntRegType to, uptr from );
// cmovae r32 to r32 
void CMOVAE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovae m32 to r32
void CMOVAE32MtoR( x86IntRegType to, uptr from );
// cmova r32 to r32 
void CMOVA32RtoR( x86IntRegType to, x86IntRegType from );
// cmova m32 to r32
void CMOVA32MtoR( x86IntRegType to, uptr from );

// cmovo r32 to r32 
void CMOVO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovo m32 to r32
void CMOVO32MtoR( x86IntRegType to, uptr from );
// cmovp r32 to r32 
void CMOVP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovp m32 to r32
void CMOVP32MtoR( x86IntRegType to, uptr from );
// cmovs r32 to r32 
void CMOVS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovs m32 to r32
void CMOVS32MtoR( x86IntRegType to, uptr from );
// cmovno r32 to r32 
void CMOVNO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovno m32 to r32
void CMOVNO32MtoR( x86IntRegType to, uptr from );
// cmovnp r32 to r32 
void CMOVNP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovnp m32 to r32
void CMOVNP32MtoR( x86IntRegType to, uptr from );
// cmovns r32 to r32 
void CMOVNS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovns m32 to r32
void CMOVNS32MtoR( x86IntRegType to, uptr from );

// cmovne r32 to r32 
void CMOVNE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovne m32 to r32
void CMOVNE32MtoR( x86IntRegType to, uptr from );
// cmove r32 to r32
void CMOVE32RtoR( x86IntRegType to, x86IntRegType from );
// cmove m32 to r32
void CMOVE32MtoR( x86IntRegType to, uptr from );
// cmovg r32 to r32
void CMOVG32RtoR( x86IntRegType to, x86IntRegType from );
// cmovg m32 to r32
void CMOVG32MtoR( x86IntRegType to, uptr from );
// cmovge r32 to r32
void CMOVGE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovge m32 to r32
void CMOVGE32MtoR( x86IntRegType to, uptr from );
// cmovl r32 to r32
void CMOVL32RtoR( x86IntRegType to, x86IntRegType from );
// cmovl m32 to r32
void CMOVL32MtoR( x86IntRegType to, uptr from );
// cmovle r32 to r32
void CMOVLE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovle m32 to r32
void CMOVLE32MtoR( x86IntRegType to, uptr from );

////////////////////////////////////
// arithmetic instructions        //
////////////////////////////////////

// add imm32 to r64 
void ADD64ItoR( x86IntRegType to, u32 from );
// add m64 to r64 
void ADD64MtoR( x86IntRegType to, uptr from );

// add imm32 to r32 
void ADD32ItoR( x86IntRegType to, u32 from );
// add imm32 to m32 
void ADD32ItoM( uptr to, u32 from );
// add imm32 to [r32+off]
void ADD32ItoRmOffset( x86IntRegType to, u32 from, int offset);
// add r32 to r32 
void ADD32RtoR( x86IntRegType to, x86IntRegType from );
// add r32 to m32 
void ADD32RtoM( uptr to, x86IntRegType from );
// add m32 to r32 
void ADD32MtoR( x86IntRegType to, uptr from );

// add r16 to r16 
void ADD16RtoR( x86IntRegType to , x86IntRegType from );
// add imm16 to r16 
void ADD16ItoR( x86IntRegType to, u16 from );
// add imm16 to m16 
void ADD16ItoM( uptr to, u16 from );
// add r16 to m16 
void ADD16RtoM( uptr to, x86IntRegType from );
// add m16 to r16 
void ADD16MtoR( x86IntRegType to, uptr from );

// add m8 to r8
void ADD8MtoR( x86IntRegType to, uptr from );

// adc imm32 to r32 
void ADC32ItoR( x86IntRegType to, u32 from );
// adc imm32 to m32 
void ADC32ItoM( uptr to, u32 from );
// adc r32 to r32 
void ADC32RtoR( x86IntRegType to, x86IntRegType from );
// adc m32 to r32 
void ADC32MtoR( x86IntRegType to, uptr from );
// adc r32 to m32 
void ADC32RtoM( uptr to, x86IntRegType from );

// inc r32 
void INC32R( x86IntRegType to );
// inc m32 
void INC32M( u32 to );
// inc r16 
void INC16R( x86IntRegType to );
// inc m16 
void INC16M( u32 to );

// sub m64 to r64 
void SUB64MtoR( x86IntRegType to, uptr from );

// sub imm32 to r32 
void SUB32ItoR( x86IntRegType to, u32 from );
// sub imm32 to m32
void SUB32ItoM( uptr to, u32 from ) ;
// sub r32 to r32 
void SUB32RtoR( x86IntRegType to, x86IntRegType from );
// sub m32 to r32 
void SUB32MtoR( x86IntRegType to, uptr from ) ;
// sub r32 to m32 
void SUB32RtoM( uptr to, x86IntRegType from );
// sub r16 to r16 
void SUB16RtoR( x86IntRegType to, u16 from );
// sub imm16 to r16 
void SUB16ItoR( x86IntRegType to, u16 from );
// sub imm16 to m16
void SUB16ItoM( uptr to, u16 from ) ;
// sub m16 to r16 
void SUB16MtoR( x86IntRegType to, uptr from );

// sbb r64 to r64 
void SBB64RtoR( x86IntRegType to, x86IntRegType from );

// sbb imm32 to r32 
void SBB32ItoR( x86IntRegType to, u32 from );
// sbb imm32 to m32 
void SBB32ItoM( uptr to, u32 from );
// sbb r32 to r32 
void SBB32RtoR( x86IntRegType to, x86IntRegType from );
// sbb m32 to r32 
void SBB32MtoR( x86IntRegType to, uptr from );
// sbb r32 to m32 
void SBB32RtoM( uptr to, x86IntRegType from );

// dec r32 
void DEC32R( x86IntRegType to );
// dec m32 
void DEC32M( u32 to );
// dec r16 
void DEC16R( x86IntRegType to );
// dec m16 
void DEC16M( u32 to );

// mul eax by r32 to edx:eax 
void MUL32R( x86IntRegType from );
// mul eax by m32 to edx:eax 
void MUL32M( u32 from );

// imul eax by r32 to edx:eax 
void IMUL32R( x86IntRegType from );
// imul eax by m32 to edx:eax 
void IMUL32M( u32 from );
// imul r32 by r32 to r32 
void IMUL32RtoR( x86IntRegType to, x86IntRegType from );

// div eax by r32 to edx:eax 
void DIV32R( x86IntRegType from );
// div eax by m32 to edx:eax 
void DIV32M( u32 from );

// idiv eax by r32 to edx:eax 
void IDIV32R( x86IntRegType from );
// idiv eax by m32 to edx:eax 
void IDIV32M( u32 from );

////////////////////////////////////
// shifting instructions          //
////////////////////////////////////

// shl imm8 to r64 
void SHL64ItoR( x86IntRegType to, u8 from );
// shl cl to r64
void SHL64CLtoR( x86IntRegType to );
// shr imm8 to r64 
void SHR64ItoR( x86IntRegType to, u8 from );
// shr cl to r64
void SHR64CLtoR( x86IntRegType to );
// sar imm8 to r64 
void SAR64ItoR( x86IntRegType to, u8 from );
// sar cl to r64
void SAR64CLtoR( x86IntRegType to );

// shl imm8 to r32 
void SHL32ItoR( x86IntRegType to, u8 from );
/* shl imm8 to m32 */
void SHL32ItoM( uptr to, u8 from );
// shl cl to r32 
void SHL32CLtoR( x86IntRegType to );

// shl imm8 to r16
void SHL16ItoR( x86IntRegType to, u8 from );
// shl imm8 to r8
void SHL8ItoR( x86IntRegType to, u8 from );

// shr imm8 to r32 
void SHR32ItoR( x86IntRegType to, u8 from );
/* shr imm8 to m32 */
void SHR32ItoM( uptr to, u8 from );
// shr cl to r32 
void SHR32CLtoR( x86IntRegType to );

// shr imm8 to r8
void SHR8ItoR( x86IntRegType to, u8 from );

// sar imm8 to r32 
void SAR32ItoR( x86IntRegType to, u8 from );
// sar imm8 to m32 
void SAR32ItoM( uptr to, u8 from );
// sar cl to r32 
void SAR32CLtoR( x86IntRegType to );

// sar imm8 to r16
void SAR16ItoR( x86IntRegType to, u8 from );

// ror imm8 to r32 (rotate right)
void ROR32ItoR( x86IntRegType to,u8 from );

void RCR32ItoR( x86IntRegType to,u8 from );
// shld imm8 to r32
void SHLD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift );
// shrd imm8 to r32
void SHRD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift );

// sal imm8 to r32 
#define SAL32ItoR SHL32ItoR
// sal cl to r32 
#define SAL32CLtoR SHL32CLtoR

// logical instructions

// or imm32 to r64 
void OR64ItoR( x86IntRegType to, u32 from );
// or m64 to r64 
void OR64MtoR( x86IntRegType to, uptr from );
// or r64 to r64
void OR64RtoR( x86IntRegType to, x86IntRegType from );
// or r32 to m64
void OR64RtoM( uptr to, x86IntRegType from );

// or imm32 to r32 
void OR32ItoR( x86IntRegType to, u32 from );
// or imm32 to m32 
void OR32ItoM( uptr to, u32 from );
// or r32 to r32 
void OR32RtoR( x86IntRegType to, x86IntRegType from );
// or r32 to m32 
void OR32RtoM( uptr to, x86IntRegType from );
// or m32 to r32 
void OR32MtoR( x86IntRegType to, uptr from );
// or r16 to r16
void OR16RtoR( x86IntRegType to, x86IntRegType from );
// or imm16 to r16
void OR16ItoR( x86IntRegType to, u16 from );
// or imm16 to m16
void OR16ItoM( uptr to, u16 from );
// or m16 to r16 
void OR16MtoR( x86IntRegType to, uptr from );
// or r16 to m16 
void OR16RtoM( uptr to, x86IntRegType from );

// or r8 to r8
void OR8RtoR( x86IntRegType to, x86IntRegType from );
// or r8 to m8
void OR8RtoM( uptr to, x86IntRegType from );
// or imm8 to m8
void OR8ItoM( uptr to, u8 from );
// or m8 to r8
void OR8MtoR( x86IntRegType to, uptr from );

// xor imm32 to r64 
void XOR64ItoR( x86IntRegType to, u32 from );
// xor r64 to r64 
void XOR64RtoR( x86IntRegType to, x86IntRegType from );
// xor m64 to r64
void XOR64MtoR( x86IntRegType to, uptr from );
// xor r64 to r64
void XOR64RtoR( x86IntRegType to, x86IntRegType from );
// xor r64 to m64
void XOR64RtoM( uptr to, x86IntRegType from );
// xor imm32 to r32 
void XOR32ItoR( x86IntRegType to, u32 from );
// xor imm32 to m32 
void XOR32ItoM( uptr to, u32 from );
// xor r32 to r32 
void XOR32RtoR( x86IntRegType to, x86IntRegType from );
// xor r16 to r16 
void XOR16RtoR( x86IntRegType to, x86IntRegType from );
// xor r32 to m32 
void XOR32RtoM( uptr to, x86IntRegType from );
// xor m32 to r32 
void XOR32MtoR( x86IntRegType to, uptr from );
// xor r16 to m16
void XOR16RtoM( uptr to, x86IntRegType from );
// xor imm16 to r16
void XOR16ItoR( x86IntRegType to, u16 from );

// and imm32 to r64 
void AND64I32toR( x86IntRegType to, u32 from );
// and m64 to r64
void AND64MtoR( x86IntRegType to, uptr from );
// and r64 to m64
void AND64RtoM( uptr to, x86IntRegType from );
// and r64 to r64
void AND64RtoR( x86IntRegType to, x86IntRegType from );
// and imm32 to m64
void AND64I32toM( uptr to, u32 from );

// and imm32 to r32 
void AND32ItoR( x86IntRegType to, u32 from );
// and sign ext imm8 to r32 
void AND32I8toR( x86IntRegType to, u8 from );
// and imm32 to m32 
void AND32ItoM( uptr to, u32 from );
// and sign ext imm8 to m32 
void AND32I8toM( uptr to, u8 from );
// and r32 to r32 
void AND32RtoR( x86IntRegType to, x86IntRegType from );
// and r32 to m32 
void AND32RtoM( uptr to, x86IntRegType from );
// and m32 to r32 
void AND32MtoR( x86IntRegType to, uptr from );
// and r16 to r16
void AND16RtoR( x86IntRegType to, x86IntRegType from );
// and imm16 to r16 
void AND16ItoR( x86IntRegType to, u16 from );
// and imm16 to m16
void AND16ItoM( uptr to, u16 from );
// and r16 to m16
void AND16RtoM( uptr to, x86IntRegType from );
// and m16 to r16 
void AND16MtoR( x86IntRegType to, uptr from );
// and imm8 to r8 
void AND8ItoR( x86IntRegType to, u8 from );
// and imm8 to m32
void AND8ItoM( uptr to, u8 from );
// and r8 to m8
void AND8RtoM( uptr to, x86IntRegType from );
// and m8 to r8 
void AND8MtoR( x86IntRegType to, uptr from );
// and r8 to r8
void AND8RtoR( x86IntRegType to, x86IntRegType from );

// not r64 
void NOT64R( x86IntRegType from );
// not r32 
void NOT32R( x86IntRegType from );
// not m32 
void NOT32M( u32 from );
// neg r64 
void NEG64R( x86IntRegType from );
// neg r32 
void NEG32R( x86IntRegType from );
// neg m32 
void NEG32M( u32 from );
// neg r16 
void NEG16R( x86IntRegType from );

////////////////////////////////////
// jump instructions              //
////////////////////////////////////

// jmp rel8 
u8*  JMP8( u8 to );

// jmp rel32 
u32* JMP32( uptr to );
// jmp r32 (r64 if __x86_64__)
void JMPR( x86IntRegType to );
// jmp m32 
void JMP32M( uptr to );

// jp rel8 
u8*  JP8( u8 to );
// jnp rel8 
u8*  JNP8( u8 to );
// je rel8 
u8*  JE8( u8 to );
// jz rel8 
u8*  JZ8( u8 to );
// jg rel8 
u8*  JG8( u8 to );
// jge rel8 
u8*  JGE8( u8 to );
// js rel8 
u8*  JS8( u8 to );
// jns rel8 
u8*  JNS8( u8 to );
// jl rel8 
u8*  JL8( u8 to );
// ja rel8 
u8*  JA8( u8 to );
// jae rel8 
u8*  JAE8( u8 to );
// jb rel8 
u8*  JB8( u8 to );
// jbe rel8 
u8*  JBE8( u8 to );
// jle rel8 
u8*  JLE8( u8 to );
// jne rel8 
u8*  JNE8( u8 to );
// jnz rel8 
u8*  JNZ8( u8 to );
// jng rel8 
u8*  JNG8( u8 to );
// jnge rel8 
u8*  JNGE8( u8 to );
// jnl rel8 
u8*  JNL8( u8 to );
// jnle rel8 
u8*  JNLE8( u8 to );
// jo rel8 
u8*  JO8( u8 to );
// jno rel8 
u8*  JNO8( u8 to );

// jb rel8 
u16*  JB16( u16 to );

// jb rel32 
u32* JB32( u32 to );
// je rel32 
u32* JE32( u32 to );
// jz rel32 
u32* JZ32( u32 to );
// jg rel32 
u32* JG32( u32 to );
// jge rel32 
u32* JGE32( u32 to );
// jl rel32 
u32* JL32( u32 to );
// jle rel32 
u32* JLE32( u32 to );
// jae rel32 
u32* JAE32( u32 to );
// jne rel32 
u32* JNE32( u32 to );
// jnz rel32 
u32* JNZ32( u32 to );
// jng rel32 
u32* JNG32( u32 to );
// jnge rel32 
u32* JNGE32( u32 to );
// jnl rel32 
u32* JNL32( u32 to );
// jnle rel32 
u32* JNLE32( u32 to );
// jo rel32 
u32* JO32( u32 to );
// jno rel32 
u32* JNO32( u32 to );
// js rel32
u32*  JS32( u32 to );

// call func 
void CALLFunc( uptr func); // based on CALL32
// call rel32 
void CALL32( u32 to );
// call r32 
void CALL32R( x86IntRegType to );
// call m32 
void CALL32M( u32 to );

////////////////////////////////////
// misc instructions              //
////////////////////////////////////

// cmp imm32 to r64 
void CMP64I32toR( x86IntRegType to, u32 from );
// cmp m64 to r64 
void CMP64MtoR( x86IntRegType to, uptr from );
// cmp r64 to r64 
void CMP64RtoR( x86IntRegType to, x86IntRegType from );

// cmp imm32 to r32 
void CMP32ItoR( x86IntRegType to, u32 from );
// cmp imm32 to m32 
void CMP32ItoM( uptr to, u32 from );
// cmp r32 to r32 
void CMP32RtoR( x86IntRegType to, x86IntRegType from );
// cmp m32 to r32 
void CMP32MtoR( x86IntRegType to, uptr from );
// cmp imm32 to [r32]
void CMP32I8toRm( x86IntRegType to, u8 from);
// cmp imm32 to [r32+off]
void CMP32I8toRmOffset8( x86IntRegType to, u8 from, u8 off);
// cmp imm8 to [r32]
void CMP32I8toM( uptr to, u8 from);

// cmp imm16 to r16 
void CMP16ItoR( x86IntRegType to, u16 from );
// cmp imm16 to m16 
void CMP16ItoM( uptr to, u16 from );
// cmp r16 to r16 
void CMP16RtoR( x86IntRegType to, x86IntRegType from );
// cmp m16 to r16 
void CMP16MtoR( x86IntRegType to, uptr from );

// cmp imm8 to r8
void CMP8ItoR( x86IntRegType to, u8 from );
// cmp m8 to r8
void CMP8MtoR( x86IntRegType to, uptr from );

// test imm32 to r32 
void TEST32ItoR( x86IntRegType to, u32 from );
// test imm32 to m32 
void TEST32ItoM( uptr to, u32 from );
// test r32 to r32 
void TEST32RtoR( x86IntRegType to, x86IntRegType from );
// test imm32 to [r32]
void TEST32ItoRm( x86IntRegType to, u32 from );
// test imm16 to r16
void TEST16ItoR( x86IntRegType to, u16 from );
// test r16 to r16
void TEST16RtoR( x86IntRegType to, x86IntRegType from );
// test imm8 to r8
void TEST8ItoR( x86IntRegType to, u8 from );
// test imm8 to r8
void TEST8ItoM( uptr to, u8 from );

// sets r8 
void SETS8R( x86IntRegType to );
// setl r8 
void SETL8R( x86IntRegType to );
// setge r8 
void SETGE8R( x86IntRegType to );
// setge r8 
void SETG8R( x86IntRegType to );
// seta r8 
void SETA8R( x86IntRegType to );
// setae r8 
void SETAE8R( x86IntRegType to );
// setb r8 
void SETB8R( x86IntRegType to );
// setnz r8 
void SETNZ8R( x86IntRegType to );
// setz r8 
void SETZ8R( x86IntRegType to );
// sete r8 
void SETE8R( x86IntRegType to );

// push imm32
void PUSH32I( u32 from );

#ifdef __x86_64__
// push r64
void PUSH64R( x86IntRegType from );
// push m64 
void PUSH64M( uptr from );
// pop r32 
void POP64R( x86IntRegType from );
#else
// push r32 
void PUSH32R( x86IntRegType from );
// push m32 
void PUSH32M( u32 from );
// push imm32 
void PUSH32I( u32 from );
// pop r32 
void POP32R( x86IntRegType from );
// pushad 
void PUSHA32( void );
// popad 
void POPA32( void );
#endif

void PUSHR(x86IntRegType from);
void POPR(x86IntRegType from);

// pushfd 
void PUSHFD( void );
// popfd 
void POPFD( void );
// ret 
void RET( void );
// ret (2-byte code used for misprediction)
void RET2( void );

void CBW();
void CWDE();
// cwd 
void CWD( void );
// cdq 
void CDQ( void );
// cdqe
void CDQE( void );

void LAHF();
void SAHF();

void BT32ItoR( x86IntRegType to, x86IntRegType from );
void BSRRtoR(x86IntRegType to, x86IntRegType from);
void BSWAP32R( x86IntRegType to );

// to = from + offset
void LEA16RtoR(x86IntRegType to, x86IntRegType from, u16 offset);
void LEA32RtoR(x86IntRegType to, x86IntRegType from, u32 offset);

// to = from0 + from1
void LEA16RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1);
void LEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1);

// to = from << scale (max is 3)
void LEA16RStoR(x86IntRegType to, x86IntRegType from, u32 scale);
void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale);

//******************
// FPU instructions 
//******************

// fild m32 to fpu reg stack 
void FILD32( u32 from );
// fistp m32 from fpu reg stack 
void FISTP32( u32 from );
// fld m32 to fpu reg stack 
void FLD32( u32 from );
// fld st(i)
void FLD(int st);
// fld1 (push +1.0f on the stack)
void FLD1();
// fld1 (push log_2 e on the stack)
void FLDL2E();
// fst m32 from fpu reg stack 
void FST32( u32 to );
// fstp m32 from fpu reg stack 
void FSTP32( u32 to );
// fstp st(i)
void FSTP(int st);

// fldcw fpu control word from m16 
void FLDCW( u32 from );
// fstcw fpu control word to m16 
void FNSTCW( u32 to );
void FXAM();
void FDECSTP();
// frndint
void FRNDINT();
void FXCH(int st);
void F2XM1();
void FSCALE();

// fadd ST(src) to fpu reg stack ST(0) 
void FADD32Rto0( x86IntRegType src );
// fadd ST(0) to fpu reg stack ST(src) 
void FADD320toR( x86IntRegType src );
// fsub ST(src) to fpu reg stack ST(0) 
void FSUB32Rto0( x86IntRegType src );
// fsub ST(0) to fpu reg stack ST(src) 
void FSUB320toR( x86IntRegType src );
// fsubp -> subtract ST(0) from ST(1), store in ST(1) and POP stack 
void FSUBP( void );
// fmul ST(src) to fpu reg stack ST(0) 
void FMUL32Rto0( x86IntRegType src );
// fmul ST(0) to fpu reg stack ST(src) 
void FMUL320toR( x86IntRegType src );
// fdiv ST(src) to fpu reg stack ST(0) 
void FDIV32Rto0( x86IntRegType src );
// fdiv ST(0) to fpu reg stack ST(src) 
void FDIV320toR( x86IntRegType src );
// fdiv ST(0) to fpu reg stack ST(src), pop stack, store in ST(src)
void FDIV320toRP( x86IntRegType src );

// fadd m32 to fpu reg stack 
void FADD32( u32 from );
// fsub m32 to fpu reg stack 
void FSUB32( u32 from );
// fmul m32 to fpu reg stack 
void FMUL32( u32 from );
// fdiv m32 to fpu reg stack 
void FDIV32( u32 from );
// fcomi st, st( i) 
void FCOMI( x86IntRegType src );
// fcomip st, st( i) 
void FCOMIP( x86IntRegType src );
// fucomi st, st( i) 
void FUCOMI( x86IntRegType src );
// fucomip st, st( i) 
void FUCOMIP( x86IntRegType src );
// fcom m32 to fpu reg stack 
void FCOM32( u32 from );
// fabs fpu reg stack 
void FABS( void );
// fsqrt fpu reg stack 
void FSQRT( void );
// ftan fpu reg stack 
void FPATAN( void );
// fsin fpu reg stack 
void FSIN( void );
// fchs fpu reg stack 
void FCHS( void );

// fcmovb fpu reg to fpu reg stack 
void FCMOVB32( x86IntRegType from );
// fcmove fpu reg to fpu reg stack 
void FCMOVE32( x86IntRegType from );
// fcmovbe fpu reg to fpu reg stack 
void FCMOVBE32( x86IntRegType from );
// fcmovu fpu reg to fpu reg stack 
void FCMOVU32( x86IntRegType from );
// fcmovnb fpu reg to fpu reg stack 
void FCMOVNB32( x86IntRegType from );
// fcmovne fpu reg to fpu reg stack 
void FCMOVNE32( x86IntRegType from );
// fcmovnbe fpu reg to fpu reg stack 
void FCMOVNBE32( x86IntRegType from );
// fcmovnu fpu reg to fpu reg stack 
void FCMOVNU32( x86IntRegType from );
void FCOMP32( u32 from );
void FNSTSWtoAX( void );

// probably a little extreme here, but x86-64 should NOT use MMX
#ifdef __x86_64__

#define MMXONLY(code)

#else

#define MMXONLY(code) code

//******************
// MMX instructions 
//******************

// r64 = mm

// movq m64 to r64 
void MOVQMtoR( x86MMXRegType to, uptr from );
// movq r64 to m64 
void MOVQRtoM( uptr to, x86MMXRegType from );

// pand r64 to r64 
void PANDRtoR( x86MMXRegType to, x86MMXRegType from );
void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pand m64 to r64 ;
void PANDMtoR( x86MMXRegType to, uptr from );
// pandn r64 to r64 
void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pandn r64 to r64 
void PANDNMtoR( x86MMXRegType to, uptr from );
// por r64 to r64 
void PORRtoR( x86MMXRegType to, x86MMXRegType from );
// por m64 to r64 
void PORMtoR( x86MMXRegType to, uptr from );
// pxor r64 to r64 
void PXORRtoR( x86MMXRegType to, x86MMXRegType from );
// pxor m64 to r64 
void PXORMtoR( x86MMXRegType to, uptr from );

// psllq r64 to r64 
void PSLLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psllq m64 to r64 
void PSLLQMtoR( x86MMXRegType to, uptr from );
// psllq imm8 to r64 
void PSLLQItoR( x86MMXRegType to, u8 from );
// psrlq r64 to r64 
void PSRLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psrlq m64 to r64 
void PSRLQMtoR( x86MMXRegType to, uptr from );
// psrlq imm8 to r64 
void PSRLQItoR( x86MMXRegType to, u8 from );

// paddusb r64 to r64 
void PADDUSBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusb m64 to r64 
void PADDUSBMtoR( x86MMXRegType to, uptr from );
// paddusw r64 to r64 
void PADDUSWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusw m64 to r64 
void PADDUSWMtoR( x86MMXRegType to, uptr from );

// paddb r64 to r64 
void PADDBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddb m64 to r64 
void PADDBMtoR( x86MMXRegType to, uptr from );
// paddw r64 to r64 
void PADDWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddw m64 to r64 
void PADDWMtoR( x86MMXRegType to, uptr from );
// paddd r64 to r64 
void PADDDRtoR( x86MMXRegType to, x86MMXRegType from );
// paddd m64 to r64 
void PADDDMtoR( x86MMXRegType to, uptr from );
void PADDSBRtoR( x86MMXRegType to, x86MMXRegType from );
void PADDSWRtoR( x86MMXRegType to, x86MMXRegType from );

// paddq m64 to r64 (sse2 only?)
void PADDQMtoR( x86MMXRegType to, uptr from );
// paddq r64 to r64 (sse2 only?)
void PADDQRtoR( x86MMXRegType to, x86MMXRegType from );

void PSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ); 
void PSUBSWRtoR( x86MMXRegType to, x86MMXRegType from );

void PSUBBRtoR( x86MMXRegType to, x86MMXRegType from );
void PSUBWRtoR( x86MMXRegType to, x86MMXRegType from );
void PSUBDRtoR( x86MMXRegType to, x86MMXRegType from );
void PSUBDMtoR( x86MMXRegType to, uptr from );

// psubq m64 to r64 (sse2 only?)
void PSUBQMtoR( x86MMXRegType to, uptr from );
// psubq r64 to r64 (sse2 only?)
void PSUBQRtoR( x86MMXRegType to, x86MMXRegType from );

// pmuludq m64 to r64 (sse2 only?)
void PMULUDQMtoR( x86MMXRegType to, uptr from );
// pmuludq r64 to r64 (sse2 only?)
void PMULUDQRtoR( x86MMXRegType to, x86MMXRegType from );

void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQDMtoR( x86MMXRegType to, uptr from );
void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTDMtoR( x86MMXRegType to, uptr from );
void PSRLWItoR( x86MMXRegType to, u8 from );
void PSRLDItoR( x86MMXRegType to, u8 from );
void PSRLDRtoR( x86MMXRegType to, x86MMXRegType from );
void PSLLWItoR( x86MMXRegType to, u8 from );
void PSLLDItoR( x86MMXRegType to, u8 from );
void PSLLDRtoR( x86MMXRegType to, x86MMXRegType from );
void PSRAWItoR( x86MMXRegType to, u8 from );
void PSRADItoR( x86MMXRegType to, u8 from );
void PSRADRtoR( x86MMXRegType to, x86MMXRegType from );
void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from );
void PUNPCKLDQMtoR( x86MMXRegType to, uptr from );
void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from );
void PUNPCKHDQMtoR( x86MMXRegType to, uptr from );
void MOVQ64ItoR( x86MMXRegType reg, u64 i ); //Prototype.Todo add all consts to end of block.not after jr $+8
void MOVQRtoR( x86MMXRegType to, x86MMXRegType from );
void MOVQRmtoROffset( x86MMXRegType to, x86IntRegType from, u32 offset );
void MOVQRtoRmOffset( x86IntRegType  to, x86MMXRegType from, u32 offset );
void MOVDMtoMMX( x86MMXRegType to, uptr from );
void MOVDMMXtoM( uptr to, x86MMXRegType from );
void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from );
void MOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from );
void MOVD32RmOffsettoMMX( x86MMXRegType to, x86IntRegType from, u32 offset );
void MOVD32MMXtoR( x86IntRegType to, x86MMXRegType from );
void MOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from );
void MOVD32MMXtoRmOffset( x86IntRegType to, x86MMXRegType from, u32 offset );
void PINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 );
void PSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8);
void PSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8);
void MASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from);

// emms 
void EMMS( void );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word 64bits
//**********************************************************************************
void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from);
void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from);

void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from);

void SSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from);
void SSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from);

#endif // !__x86_64__

//*********************
// SSE   instructions *
//*********************
void SSE_MOVAPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_MOVAPS_XMM_to_M128( uptr to, x86SSERegType from );
void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MOVUPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_MOVUPS_XMM_to_M128( uptr to, x86SSERegType from );

void SSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from );
void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from );
void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MOVSS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVSS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset );

void SSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from );
void SSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from );

void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from );
void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from );
void SSE_MOVLPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVLPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset );

void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from );
void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from );       
void SSE_MOVHPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVHPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset );

void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from );
void SSE_MOVLPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVLPSRtoRm( x86SSERegType to, x86IntRegType from );
void SSE_MOVLPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset );

void SSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVAPSRtoRmOffset( x86IntRegType to, x86SSERegType from, int offset );
void SSE_MOVAPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from );
void SSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from );

void SSE_MOVUPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset );
void SSE_MOVUPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset );

void SSE2_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset );
void SSE2_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset );

void SSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RCPPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_RCPSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RCPSS_M32_to_XMM( x86SSERegType to, uptr from );

void SSE_ORPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_XORPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ANDPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ANDNPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ADDPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ADDSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SUBPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SUBSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MULPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MULSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPEQSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLTSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLESS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNESS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLESS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPORDSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_UCOMISS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_UCOMISS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

#ifndef __x86_64__
void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from );
void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from );
void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from );
#endif
void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from );
void SSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from);
void SSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from);
void SSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from);
void SSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from);

void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MAXPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MAXSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MINPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MINSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RSQRTPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RSQRTSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SQRTPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SQRTSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );
void SSE_SHUFPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 );
void SSE_CMPEQPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLTPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLEPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNEPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPORDPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_DIVPS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_DIVSS_M32_to_XMM( x86SSERegType to, uptr from );
void SSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
// VectorPath
void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );

void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );
void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );

void SSE_STMXCSR( uptr from );
void SSE_LDMXCSR( uptr from );


//*********************
//  SSE 2 Instructions*
//*********************
void SSE2_MOVDQA_M128_to_XMM(x86SSERegType to, uptr from); 
void SSE2_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from); 
void SSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from); 

void SSE2_MOVDQU_M128_to_XMM(x86SSERegType to, uptr from); 
void SSE2_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from); 
void SSE2_MOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from); 

void SSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLW_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLD_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLQ_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRAW_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRAD_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLW_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLD_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLQ_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMAXSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMAXUB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMINSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMINUB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBSB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PSUBUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBUSB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PSUBUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBUSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PAND_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PANDN_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PXOR_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDW_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDUSB_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDUSW_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDB_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDD_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PADDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDQ_M128_to_XMM(x86SSERegType to, uptr from );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
void SSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, uptr from);

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
void SSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PUNPCKHQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHQDQ_M128_to_XMM(x86SSERegType to, uptr from);

// mult by half words
void SSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULLW_M128_to_XMM(x86SSERegType to, uptr from);
void SSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULHW_M128_to_XMM(x86SSERegType to, uptr from);

void SSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, uptr from);


//**********************************************************************************/
//PMOVMSKB: Create 16bit mask from signs of 8bit integers
//**********************************************************************************
void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from);

void SSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from);
void SSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from);

//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 );
void SSE_PINSRW_R32_to_XMM(x86SSERegType from, x86IntRegType to, u8 imm8 );


//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
void SSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBB_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBW_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBD_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBQ_M128_to_XMM(x86SSERegType to, uptr from );
///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
void SSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, uptr from );
void SSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, uptr from );
//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
void SSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from );
void SSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from );
void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from );
void SSE2_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
void SSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from );
void SSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from );
void SSE2_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset );

#ifdef __x86_64__
void SSE2_MOVQ_XMM_to_R( x86IntRegType to, x86SSERegType from );
void SSE2_MOVQ_R_to_XMM( x86SSERegType to, x86IntRegType from );
#endif

//**********************************************************************************/
//POR : SSE Bitwise OR                                                             *
//**********************************************************************************
void SSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_POR_M128_to_XMM( x86SSERegType to, uptr from );

void SSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from);

void SSE3_MOVSLDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE3_MOVSLDUP_M128_to_XMM(x86SSERegType to, uptr from);
void SSE3_MOVSHDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE3_MOVSHDUP_M128_to_XMM(x86SSERegType to, uptr from);

// SSE4.1

#ifndef _MM_MK_INSERTPS_NDX
#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))
#endif

void SSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);
void SSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8);
void SSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);
void SSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8);
void SSE4_BLENDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);

//*********************
// SSE-X - uses both SSE,SSE2 code and tries to keep consistensies between the data
// Uses g_xmmtypes to infer the correct type.
//*********************
void SSEX_MOVDQA_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from );
void SSEX_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSEX_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset );
void SSEX_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset );

void SSEX_MOVDQU_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from );
void SSEX_MOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSEX_MOVD_M32_to_XMM( x86SSERegType to, uptr from );
void SSEX_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
void SSEX_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from );
void SSEX_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSEX_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset );

void SSEX_POR_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSEX_PXOR_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSEX_PAND_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSEX_PANDN_M128_to_XMM( x86SSERegType to, uptr from );
void SSEX_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSEX_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSEX_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from);
void SSEX_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSEX_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from);

void SSEX_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

//*********************
// 3DNOW instructions * 
//*********************
void FEMMS( void );
void PFCMPEQMtoR( x86IntRegType to, uptr from );
void PFCMPGTMtoR( x86IntRegType to, uptr from );
void PFCMPGEMtoR( x86IntRegType to, uptr from );
void PFADDMtoR( x86IntRegType to, uptr from );
void PFADDRtoR( x86IntRegType to, x86IntRegType from );
void PFSUBMtoR( x86IntRegType to, uptr from );
void PFSUBRtoR( x86IntRegType to, x86IntRegType from );
void PFMULMtoR( x86IntRegType to, uptr from );
void PFMULRtoR( x86IntRegType to, x86IntRegType from );
void PFRCPMtoR( x86IntRegType to, uptr from );
void PFRCPRtoR( x86IntRegType to, x86IntRegType from );
void PFRCPIT1RtoR( x86IntRegType to, x86IntRegType from );
void PFRCPIT2RtoR( x86IntRegType to, x86IntRegType from );
void PFRSQRTRtoR( x86IntRegType to, x86IntRegType from );
void PFRSQIT1RtoR( x86IntRegType to, x86IntRegType from );
void PF2IDMtoR( x86IntRegType to, uptr from );
void PF2IDRtoR( x86IntRegType to, x86IntRegType from );
void PI2FDMtoR( x86IntRegType to, uptr from );
void PI2FDRtoR( x86IntRegType to, x86IntRegType from );
void PFMAXMtoR( x86IntRegType to, uptr from );
void PFMAXRtoR( x86IntRegType to, x86IntRegType from );
void PFMINMtoR( x86IntRegType to, uptr from );
void PFMINRtoR( x86IntRegType to, x86IntRegType from );

void SSE2EMU_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from);
void SSE2EMU_MOVQ_M64_to_XMM( x86SSERegType to, uptr from);
void SSE2EMU_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from);
void SSE2EMU_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset );
void SSE2EMU_MOVD_XMM_to_RmOffset(x86IntRegType to, x86SSERegType from, int offset );

#ifndef __x86_64__
void SSE2EMU_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from);
void SSE2EMU_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from);
#endif

/* SSE2 emulated functions for SSE CPU's by kekko*/

void SSE2EMU_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2EMU_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from );
void SSE2EMU_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2EMU_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from );
void SSE2EMU_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
void SSE2EMU_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from );

////////////////////////////////////////////////////
#ifdef _DEBUG
#define WRITECHECK() CheckX86Ptr()
#else
#define WRITECHECK()
#endif

#define write8(val )  {  \
	*(u8*)x86Ptr = (u8)val; \
	x86Ptr++; \
} \

#define write16(val ) \
{ \
	*(u16*)x86Ptr = (u16)val; \
	x86Ptr += 2;  \
} \

#define write24(val ) \
{ \
	*(u8*)x86Ptr = (u8)(val & 0xff); \
	x86Ptr++;  \
	*(u8*)x86Ptr = (u8)((val >> 8) & 0xff); \
	x86Ptr++;  \
	*(u8*)x86Ptr = (u8)((val >> 16) & 0xff); \
	x86Ptr++;  \
} \

#define write32( val ) \
{ \
	*(u32*)x86Ptr = val; \
	x86Ptr += 4; \
} \

#ifdef __cplusplus
}
#endif

#endif // __IX86_H__
