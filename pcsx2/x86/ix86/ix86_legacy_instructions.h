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

#pragma once

//------------------------------------------------------------------
// legacy write functions (depreciated, use x86Emitter::EmitPtrCache instead)
//------------------------------------------------------------------
#define emitterT __forceinline

using x86Emitter::write8;
using x86Emitter::write16;
using x86Emitter::write24;
using x86Emitter::write32;
using x86Emitter::write64;

//------------------------------------------------------------------
// legacy jump/align functions
//------------------------------------------------------------------
extern void x86SetPtr( u8 *ptr );
extern void x86SetJ8( u8 *j8 );
extern void x86SetJ8A( u8 *j8 );
extern void x86SetJ16( u16 *j16 );
extern void x86SetJ16A( u16 *j16 );
extern void x86SetJ32( u32 *j32 );
extern void x86SetJ32A( u32 *j32 );
extern void x86Align( int bytes );
extern void x86AlignExecutable( int align );
//------------------------------------------------------------------

extern void CLC( void );
extern void NOP( void );

////////////////////////////////////
// mov instructions               //
////////////////////////////////////

// mov r32 to r32 
extern void MOV32RtoR( x86IntRegType to, x86IntRegType from );
// mov r32 to m32 
extern void MOV32RtoM( uptr to, x86IntRegType from );
// mov m32 to r32 
extern void MOV32MtoR( x86IntRegType to, uptr from );
// mov [r32] to r32 
extern void MOV32RmtoR( x86IntRegType to, x86IntRegType from, int offset=0 );
// mov [r32][r32<<scale] to r32 
extern void MOV32RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
// mov [imm32(from2) + r32(from1)<<scale] to r32 
extern void MOV32RmSOffsettoR( x86IntRegType to, x86IntRegType from1, int from2, int scale=0 );
// mov r32 to [r32][r32*scale]
extern void MOV32RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
// mov imm32 to r32 
extern void MOV32ItoR( x86IntRegType to, u32 from );
// mov imm32 to m32 
extern void MOV32ItoM( uptr to, u32 from );
// mov imm32 to [r32+off]
extern void MOV32ItoRm( x86IntRegType to, u32 from, int offset=0);
// mov r32 to [r32+off]
extern void MOV32RtoRm( x86IntRegType to, x86IntRegType from, int offset=0);

// mov r16 to r16 
extern void MOV16RtoR( x86IntRegType to, x86IntRegType from ) ;
// mov r16 to m16 
extern void MOV16RtoM( uptr to, x86IntRegType from );
// mov m16 to r16 
extern void MOV16MtoR( x86IntRegType to, uptr from );
// mov [r32] to r16
extern void MOV16RmtoR( x86IntRegType to, x86IntRegType from ) ;
extern void MOV16RmtoR( x86IntRegType to, x86IntRegType from, int offset=0 );
// mov [imm32(from2) + r32(from1)<<scale] to r16 
extern void MOV16RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale=0 );
// mov imm16 to m16 
extern void MOV16ItoM( uptr to, u16 from );
/* mov r16 to [r32][r32*scale] */
extern void MOV16RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale=0);
// mov imm16 to r16
extern void MOV16ItoR( x86IntRegType to, u16 from );
// mov imm16 to [r16+off]
extern void MOV16ItoRm( x86IntRegType to, u16 from, u32 offset);
// mov r16 to [r16+off]
extern void MOV16RtoRm( x86IntRegType to, x86IntRegType from, int offset=0);

// mov r8 to m8 
extern void MOV8RtoM( uptr to, x86IntRegType from );
// mov m8 to r8 
extern void MOV8MtoR( x86IntRegType to, uptr from );
// mov [r32] to r8
extern void MOV8RmtoR(x86IntRegType to, x86IntRegType from);
extern void MOV8RmtoR(x86IntRegType to, x86IntRegType from, int offset=0);
extern void MOV8RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale=0 );
// mov imm8 to m8 
extern void MOV8ItoM( uptr to, u8 from );
// mov imm8 to r8
extern void MOV8ItoR( x86IntRegType to, u8 from );
// mov imm8 to [r8+off]
extern void MOV8ItoRm( x86IntRegType to, u8 from, int offset=0);
// mov r8 to [r8+off]
extern void MOV8RtoRm( x86IntRegType to, x86IntRegType from, int offset=0);

// movsx r8 to r32 
extern void MOVSX32R8toR( x86IntRegType to, x86IntRegType from );
extern void MOVSX32Rm8toR( x86IntRegType to, x86IntRegType from, int offset=0 );
// movsx m8 to r32 
extern void MOVSX32M8toR( x86IntRegType to, u32 from );
// movsx r16 to r32 
extern void MOVSX32R16toR( x86IntRegType to, x86IntRegType from );
extern void MOVSX32Rm16toR( x86IntRegType to, x86IntRegType from, int offset=0 );
// movsx m16 to r32 
extern void MOVSX32M16toR( x86IntRegType to, u32 from );

// movzx r8 to r32 
extern void MOVZX32R8toR( x86IntRegType to, x86IntRegType from );
extern void MOVZX32Rm8toR( x86IntRegType to, x86IntRegType from, int offset=0 );
// movzx m8 to r32 
extern void MOVZX32M8toR( x86IntRegType to, u32 from );
// movzx r16 to r32 
extern void MOVZX32R16toR( x86IntRegType to, x86IntRegType from );
extern void MOVZX32Rm16toR( x86IntRegType to, x86IntRegType from, int offset=0 );
// movzx m16 to r32 
extern void MOVZX32M16toR( x86IntRegType to, u32 from );

// cmovbe r32 to r32 
extern void CMOVBE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovbe m32 to r32
extern void CMOVBE32MtoR( x86IntRegType to, uptr from );
// cmovb r32 to r32 
extern void CMOVB32RtoR( x86IntRegType to, x86IntRegType from );
// cmovb m32 to r32
extern void CMOVB32MtoR( x86IntRegType to, uptr from );
// cmovae r32 to r32 
extern void CMOVAE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovae m32 to r32
extern void CMOVAE32MtoR( x86IntRegType to, uptr from );
// cmova r32 to r32 
extern void CMOVA32RtoR( x86IntRegType to, x86IntRegType from );
// cmova m32 to r32
extern void CMOVA32MtoR( x86IntRegType to, uptr from );

// cmovo r32 to r32 
extern void CMOVO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovo m32 to r32
extern void CMOVO32MtoR( x86IntRegType to, uptr from );
// cmovp r32 to r32 
extern void CMOVP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovp m32 to r32
extern void CMOVP32MtoR( x86IntRegType to, uptr from );
// cmovs r32 to r32 
extern void CMOVS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovs m32 to r32
extern void CMOVS32MtoR( x86IntRegType to, uptr from );
// cmovno r32 to r32 
extern void CMOVNO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovno m32 to r32
extern void CMOVNO32MtoR( x86IntRegType to, uptr from );
// cmovnp r32 to r32 
extern void CMOVNP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovnp m32 to r32
extern void CMOVNP32MtoR( x86IntRegType to, uptr from );
// cmovns r32 to r32 
extern void CMOVNS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovns m32 to r32
extern void CMOVNS32MtoR( x86IntRegType to, uptr from );

// cmovne r32 to r32 
extern void CMOVNE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovne m32 to r32
extern void CMOVNE32MtoR( x86IntRegType to, uptr from );
// cmove r32 to r32
extern void CMOVE32RtoR( x86IntRegType to, x86IntRegType from );
// cmove m32 to r32
extern void CMOVE32MtoR( x86IntRegType to, uptr from );
// cmovg r32 to r32
extern void CMOVG32RtoR( x86IntRegType to, x86IntRegType from );
// cmovg m32 to r32
extern void CMOVG32MtoR( x86IntRegType to, uptr from );
// cmovge r32 to r32
extern void CMOVGE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovge m32 to r32
extern void CMOVGE32MtoR( x86IntRegType to, uptr from );
// cmovl r32 to r32
extern void CMOVL32RtoR( x86IntRegType to, x86IntRegType from );
// cmovl m32 to r32
extern void CMOVL32MtoR( x86IntRegType to, uptr from );
// cmovle r32 to r32
extern void CMOVLE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovle m32 to r32
extern void CMOVLE32MtoR( x86IntRegType to, uptr from );

////////////////////////////////////
// arithmetic instructions        //
////////////////////////////////////

// add imm32 to EAX
extern void ADD32ItoEAX( u32 from );
// add imm32 to r32 
extern void ADD32ItoR( x86IntRegType to, u32 from );
// add imm32 to m32 
extern void ADD32ItoM( uptr to, u32 from );
// add imm32 to [r32+off]
extern void ADD32ItoRm( x86IntRegType to, u32 from, int offset=0);
// add r32 to r32 
extern void ADD32RtoR( x86IntRegType to, x86IntRegType from );
// add r32 to m32 
extern void ADD32RtoM( uptr to, x86IntRegType from );
// add m32 to r32 
extern void ADD32MtoR( x86IntRegType to, uptr from );

// add r16 to r16 
extern void ADD16RtoR( x86IntRegType to , x86IntRegType from );
// add imm16 to r16 
extern void ADD16ItoR( x86IntRegType to, u16 imm );
// add imm16 to m16 
extern void ADD16ItoM( uptr to, u16 imm );
// add r16 to m16 
extern void ADD16RtoM( uptr to, x86IntRegType from );
// add m16 to r16 
extern void ADD16MtoR( x86IntRegType to, uptr from );

// add m8 to r8
extern void ADD8MtoR( x86IntRegType to, uptr from );

// adc imm32 to r32 
extern void ADC32ItoR( x86IntRegType to, u32 from );
// adc imm32 to m32 
extern void ADC32ItoM( uptr to, u32 from );
// adc r32 to r32 
extern void ADC32RtoR( x86IntRegType to, x86IntRegType from );
// adc m32 to r32 
extern void ADC32MtoR( x86IntRegType to, uptr from );
// adc r32 to m32 
extern void ADC32RtoM( uptr to, x86IntRegType from );

// inc r32 
extern void INC32R( x86IntRegType to );
// inc m32 
extern void INC32M( u32 to );
// inc r16 
extern void INC16R( x86IntRegType to );
// inc m16 
extern void INC16M( u32 to );

// sub imm32 to r32 
extern void SUB32ItoR( x86IntRegType to, u32 from );
// sub imm32 to m32
extern void SUB32ItoM( uptr to, u32 from ) ;
// sub r32 to r32 
extern void SUB32RtoR( x86IntRegType to, x86IntRegType from );
// sub m32 to r32 
extern void SUB32MtoR( x86IntRegType to, uptr from ) ;
// sub r32 to m32 
extern void SUB32RtoM( uptr to, x86IntRegType from );
// sub r16 to r16 
extern void SUB16RtoR( x86IntRegType to, x86IntRegType from );
// sub imm16 to r16 
extern void SUB16ItoR( x86IntRegType to, u16 from );
// sub imm16 to m16
extern void SUB16ItoM( uptr to, u16 from ) ;
// sub m16 to r16 
extern void SUB16MtoR( x86IntRegType to, uptr from );

// sbb imm32 to r32 
extern void SBB32ItoR( x86IntRegType to, u32 from );
// sbb imm32 to m32 
extern void SBB32ItoM( uptr to, u32 from );
// sbb r32 to r32 
extern void SBB32RtoR( x86IntRegType to, x86IntRegType from );
// sbb m32 to r32 
extern void SBB32MtoR( x86IntRegType to, uptr from );
// sbb r32 to m32 
extern void SBB32RtoM( uptr to, x86IntRegType from );

// dec r32 
extern void DEC32R( x86IntRegType to );
// dec m32 
extern void DEC32M( u32 to );
// dec r16 
extern void DEC16R( x86IntRegType to );
// dec m16 
extern void DEC16M( u32 to );

// mul eax by r32 to edx:eax 
extern void MUL32R( x86IntRegType from );
// mul eax by m32 to edx:eax 
extern void MUL32M( u32 from );

// imul eax by r32 to edx:eax 
extern void IMUL32R( x86IntRegType from );
// imul eax by m32 to edx:eax 
extern void IMUL32M( u32 from );
// imul r32 by r32 to r32 
extern void IMUL32RtoR( x86IntRegType to, x86IntRegType from );

// div eax by r32 to edx:eax 
extern void DIV32R( x86IntRegType from );
// div eax by m32 to edx:eax 
extern void DIV32M( u32 from );

// idiv eax by r32 to edx:eax 
extern void IDIV32R( x86IntRegType from );
// idiv eax by m32 to edx:eax 
extern void IDIV32M( u32 from );

////////////////////////////////////
// shifting instructions          //
////////////////////////////////////

// shl imm8 to r32 
extern void SHL32ItoR( x86IntRegType to, u8 from );
/* shl imm8 to m32 */
extern void SHL32ItoM( uptr to, u8 from );
// shl cl to r32 
extern void SHL32CLtoR( x86IntRegType to );

// shl imm8 to r16
extern void SHL16ItoR( x86IntRegType to, u8 from );
// shl imm8 to r8
extern void SHL8ItoR( x86IntRegType to, u8 from );

// shr imm8 to r32 
extern void SHR32ItoR( x86IntRegType to, u8 from );
/* shr imm8 to m32 */
extern void SHR32ItoM( uptr to, u8 from );
// shr cl to r32 
extern void SHR32CLtoR( x86IntRegType to );

// shr imm8 to r16
extern void SHR16ItoR( x86IntRegType to, u8 from );

// shr imm8 to r8
extern void SHR8ItoR( x86IntRegType to, u8 from );

// sar imm8 to r32 
extern void SAR32ItoR( x86IntRegType to, u8 from );
// sar imm8 to m32 
extern void SAR32ItoM( uptr to, u8 from );
// sar cl to r32 
extern void SAR32CLtoR( x86IntRegType to );

// sar imm8 to r16
extern void SAR16ItoR( x86IntRegType to, u8 from );

// ror imm8 to r32 (rotate right)
extern void ROR32ItoR( x86IntRegType to, u8 from );

extern void RCR32ItoR( x86IntRegType to, u8 from );
extern void RCR32ItoM( uptr to, u8 from );
// shld imm8 to r32
extern void SHLD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift );
// shrd imm8 to r32
extern void SHRD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift );

// sal imm8 to r32 
#define SAL32ItoR SHL32ItoR
// sal cl to r32 
#define SAL32CLtoR SHL32CLtoR

// logical instructions

// or imm32 to r32 
extern void OR32ItoR( x86IntRegType to, u32 from );
// or imm32 to m32 
extern void OR32ItoM( uptr to, u32 from );
// or r32 to r32 
extern void OR32RtoR( x86IntRegType to, x86IntRegType from );
// or r32 to m32 
extern void OR32RtoM( uptr to, x86IntRegType from );
// or m32 to r32 
extern void OR32MtoR( x86IntRegType to, uptr from );
// or r16 to r16
extern void OR16RtoR( x86IntRegType to, x86IntRegType from );
// or imm16 to r16
extern void OR16ItoR( x86IntRegType to, u16 from );
// or imm16 to m16
extern void OR16ItoM( uptr to, u16 from );
// or m16 to r16 
extern void OR16MtoR( x86IntRegType to, uptr from );
// or r16 to m16 
extern void OR16RtoM( uptr to, x86IntRegType from );

// or r8 to r8
extern void OR8RtoR( x86IntRegType to, x86IntRegType from );
// or r8 to m8
extern void OR8RtoM( uptr to, x86IntRegType from );
// or imm8 to m8
extern void OR8ItoM( uptr to, u8 from );
// or m8 to r8
extern void OR8MtoR( x86IntRegType to, uptr from );

// xor imm32 to r32 
extern void XOR32ItoR( x86IntRegType to, u32 from );
// xor imm32 to m32 
extern void XOR32ItoM( uptr to, u32 from );
// xor r32 to r32 
extern void XOR32RtoR( x86IntRegType to, x86IntRegType from );
// xor r16 to r16 
extern void XOR16RtoR( x86IntRegType to, x86IntRegType from );
// xor r32 to m32 
extern void XOR32RtoM( uptr to, x86IntRegType from );
// xor m32 to r32 
extern void XOR32MtoR( x86IntRegType to, uptr from );
// xor r16 to m16
extern void XOR16RtoM( uptr to, x86IntRegType from );
// xor imm16 to r16
extern void XOR16ItoR( x86IntRegType to, u16 from );

// and imm32 to r32 
extern void AND32ItoR( x86IntRegType to, u32 from );
// and sign ext imm8 to r32 
extern void AND32I8toR( x86IntRegType to, u8 from );
// and imm32 to m32 
extern void AND32ItoM( uptr to, u32 from );
// and sign ext imm8 to m32 
extern void AND32I8toM( uptr to, u8 from );
// and r32 to r32 
extern void AND32RtoR( x86IntRegType to, x86IntRegType from );
// and r32 to m32 
extern void AND32RtoM( uptr to, x86IntRegType from );
// and m32 to r32 
extern void AND32MtoR( x86IntRegType to, uptr from );
// and r16 to r16
extern void AND16RtoR( x86IntRegType to, x86IntRegType from );
// and imm16 to r16 
extern void AND16ItoR( x86IntRegType to, u16 from );
// and imm16 to m16
extern void AND16ItoM( uptr to, u16 from );
// and r16 to m16
extern void AND16RtoM( uptr to, x86IntRegType from );
// and m16 to r16 
extern void AND16MtoR( x86IntRegType to, uptr from );
// and imm8 to r8 
extern void AND8ItoR( x86IntRegType to, u8 from );
// and imm8 to m32
extern void AND8ItoM( uptr to, u8 from );
// and r8 to m8
extern void AND8RtoM( uptr to, x86IntRegType from );
// and m8 to r8 
extern void AND8MtoR( x86IntRegType to, uptr from );
// and r8 to r8
extern void AND8RtoR( x86IntRegType to, x86IntRegType from );

// not r32 
extern void NOT32R( x86IntRegType from );
// not m32 
extern void NOT32M( u32 from );
// neg r32 
extern void NEG32R( x86IntRegType from );
// neg m32 
extern void NEG32M( u32 from );
// neg r16 
extern void NEG16R( x86IntRegType from );

////////////////////////////////////
// jump instructions              //
////////////////////////////////////

// jmp rel8 
extern u8*  JMP8( u8 to );

// jmp rel32 
extern u32* JMP32( uptr to );
// jmp r32 (r64 if __x86_64__)
extern void JMPR( x86IntRegType to );
// jmp m32 
extern void JMP32M( uptr to );

// jp rel8 
extern u8*  JP8( u8 to );
// jnp rel8 
extern u8*  JNP8( u8 to );
// je rel8 
extern u8*  JE8( u8 to );
// jz rel8 
extern u8*  JZ8( u8 to );
// jg rel8 
extern u8*  JG8( u8 to );
// jge rel8 
extern u8*  JGE8( u8 to );
// js rel8 
extern u8*  JS8( u8 to );
// jns rel8 
extern u8*  JNS8( u8 to );
// jl rel8 
extern u8*  JL8( u8 to );
// ja rel8 
extern u8*  JA8( u8 to );
// jae rel8 
extern u8*  JAE8( u8 to );
// jb rel8 
extern u8*  JB8( u8 to );
// jbe rel8 
extern u8*  JBE8( u8 to );
// jle rel8 
extern u8*  JLE8( u8 to );
// jne rel8 
extern u8*  JNE8( u8 to );
// jnz rel8 
extern u8*  JNZ8( u8 to );
// jng rel8 
extern u8*  JNG8( u8 to );
// jnge rel8 
extern u8*  JNGE8( u8 to );
// jnl rel8 
extern u8*  JNL8( u8 to );
// jnle rel8 
extern u8*  JNLE8( u8 to );
// jo rel8 
extern u8*  JO8( u8 to );
// jno rel8 
extern u8*  JNO8( u8 to );

/*
// jb rel16
extern u16*  JA16( u16 to );
// jb rel16
extern u16*  JB16( u16 to );
// je rel16
extern u16*  JE16( u16 to );
// jz rel16
extern u16*  JZ16( u16 to );
*/

// jns rel32 
extern u32* JNS32( u32 to );
// js rel32 
extern u32* JS32( u32 to );

// jb rel32 
extern u32* JB32( u32 to );
// je rel32 
extern u32* JE32( u32 to );
// jz rel32 
extern u32* JZ32( u32 to );
// jg rel32 
extern u32* JG32( u32 to );
// jge rel32 
extern u32* JGE32( u32 to );
// jl rel32 
extern u32* JL32( u32 to );
// jle rel32 
extern u32* JLE32( u32 to );
// jae rel32 
extern u32* JAE32( u32 to );
// jne rel32 
extern u32* JNE32( u32 to );
// jnz rel32 
extern u32* JNZ32( u32 to );
// jng rel32 
extern u32* JNG32( u32 to );
// jnge rel32 
extern u32* JNGE32( u32 to );
// jnl rel32 
extern u32* JNL32( u32 to );
// jnle rel32 
extern u32* JNLE32( u32 to );
// jo rel32 
extern u32* JO32( u32 to );
// jno rel32 
extern u32* JNO32( u32 to );
// js rel32
extern u32*  JS32( u32 to );

// call func 
extern void CALLFunc( uptr func); // based on CALL32
// call r32 
extern void CALL32R( x86IntRegType to );
// call m32 
extern void CALL32M( u32 to );

////////////////////////////////////
// misc instructions              //
////////////////////////////////////

// cmp imm32 to r32 
extern void CMP32ItoR( x86IntRegType to, u32 from );
// cmp imm32 to m32 
extern void CMP32ItoM( uptr to, u32 from );
// cmp r32 to r32 
extern void CMP32RtoR( x86IntRegType to, x86IntRegType from );
// cmp m32 to r32 
extern void CMP32MtoR( x86IntRegType to, uptr from );

// cmp imm16 to r16 
extern void CMP16ItoR( x86IntRegType to, u16 from );
// cmp imm16 to m16 
extern void CMP16ItoM( uptr to, u16 from );
// cmp r16 to r16 
extern void CMP16RtoR( x86IntRegType to, x86IntRegType from );
// cmp m16 to r16 
extern void CMP16MtoR( x86IntRegType to, uptr from );

// cmp imm8 to r8
extern void CMP8ItoR( x86IntRegType to, u8 from );
// cmp m8 to r8
extern void CMP8MtoR( x86IntRegType to, uptr from );

// test imm32 to r32 
extern void TEST32ItoR( x86IntRegType to, u32 from );
// test imm32 to m32 
extern void TEST32ItoM( uptr to, u32 from );
// test r32 to r32 
extern void TEST32RtoR( x86IntRegType to, x86IntRegType from );
// test imm32 to [r32]
extern void TEST32ItoRm( x86IntRegType to, u32 from );
// test imm16 to r16
extern void TEST16ItoR( x86IntRegType to, u16 from );
// test r16 to r16
extern void TEST16RtoR( x86IntRegType to, x86IntRegType from );
// test r8 to r8
extern void TEST8RtoR( x86IntRegType to, x86IntRegType from );
// test imm8 to r8
extern void TEST8ItoR( x86IntRegType to, u8 from );
// test imm8 to r8
extern void TEST8ItoM( uptr to, u8 from );

// sets r8 
extern void SETS8R( x86IntRegType to );
// setl r8 
extern void SETL8R( x86IntRegType to );
// setge r8 
extern void SETGE8R( x86IntRegType to );
// setge r8 
extern void SETG8R( x86IntRegType to );
// seta r8 
extern void SETA8R( x86IntRegType to );
// setae r8 
extern void SETAE8R( x86IntRegType to );
// setb r8 
extern void SETB8R( x86IntRegType to );
// setnz r8 
extern void SETNZ8R( x86IntRegType to );
// setz r8 
extern void SETZ8R( x86IntRegType to );
// sete r8 
extern void SETE8R( x86IntRegType to );

// push imm32
extern void PUSH32I( u32 from );

// push r32 
extern void PUSH32R( x86IntRegType from );
// push m32 
extern void PUSH32M( u32 from );
// push imm32 
extern void PUSH32I( u32 from );
// pop r32 
extern void POP32R( x86IntRegType from );
// pushad 
extern void PUSHA32( void );
// popad 
extern void POPA32( void );

extern void PUSHR(x86IntRegType from);
extern void POPR(x86IntRegType from);

// pushfd 
extern void PUSHFD( void );
// popfd 
extern void POPFD( void );
// ret 
extern void RET( void );
// ret (2-byte code used for misprediction)
extern void RET2( void );

extern void CBW();
extern void CWDE();
// cwd 
extern void CWD( void );
// cdq 
extern void CDQ( void );
// cdqe
extern void CDQE( void );

extern void LAHF();
extern void SAHF();

extern void BT32ItoR( x86IntRegType to, u8 from );
extern void BTR32ItoR( x86IntRegType to, u8 from );
extern void BSRRtoR(x86IntRegType to, x86IntRegType from);
extern void BSWAP32R( x86IntRegType to );

// to = from + offset
extern void LEA16RtoR(x86IntRegType to, x86IntRegType from, s16 offset=0);
extern void LEA32RtoR(x86IntRegType to, x86IntRegType from, s32 offset=0);

// to = from0 + from1
extern void LEA16RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1);
extern void LEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1);

// to = from << scale (max is 3)
extern void LEA16RStoR(x86IntRegType to, x86IntRegType from, u32 scale);
extern void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale);

//******************
// FPU instructions 
//******************

// fild m32 to fpu reg stack 
extern void FILD32( u32 from );
// fistp m32 from fpu reg stack 
extern void FISTP32( u32 from );
// fld m32 to fpu reg stack 
extern void FLD32( u32 from );
// fld st(i)
extern void FLD(int st);
// fld1 (push +1.0f on the stack)
extern void FLD1();
// fld1 (push log_2 e on the stack)
extern void FLDL2E();
// fst m32 from fpu reg stack 
extern void FST32( u32 to );
// fstp m32 from fpu reg stack 
extern void FSTP32( u32 to );
// fstp st(i)
extern void FSTP(int st);

// fldcw fpu control word from m16 
extern void FLDCW( u32 from );
// fstcw fpu control word to m16 
extern void FNSTCW( u32 to );
extern void FXAM();
extern void FDECSTP();
// frndint
extern void FRNDINT();
extern void FXCH(int st);
extern void F2XM1();
extern void FSCALE();

// fadd ST(src) to fpu reg stack ST(0) 
extern void FADD32Rto0( x86IntRegType src );
// fadd ST(0) to fpu reg stack ST(src) 
extern void FADD320toR( x86IntRegType src );
// fsub ST(src) to fpu reg stack ST(0) 
extern void FSUB32Rto0( x86IntRegType src );
// fsub ST(0) to fpu reg stack ST(src) 
extern void FSUB320toR( x86IntRegType src );
// fsubp -> subtract ST(0) from ST(1), store in ST(1) and POP stack 
extern void FSUBP( void );
// fmul ST(src) to fpu reg stack ST(0) 
extern void FMUL32Rto0( x86IntRegType src );
// fmul ST(0) to fpu reg stack ST(src) 
extern void FMUL320toR( x86IntRegType src );
// fdiv ST(src) to fpu reg stack ST(0) 
extern void FDIV32Rto0( x86IntRegType src );
// fdiv ST(0) to fpu reg stack ST(src) 
extern void FDIV320toR( x86IntRegType src );
// fdiv ST(0) to fpu reg stack ST(src), pop stack, store in ST(src)
extern void FDIV320toRP( x86IntRegType src );

// fadd m32 to fpu reg stack 
extern void FADD32( u32 from );
// fsub m32 to fpu reg stack 
extern void FSUB32( u32 from );
// fmul m32 to fpu reg stack 
extern void FMUL32( u32 from );
// fdiv m32 to fpu reg stack 
extern void FDIV32( u32 from );
// fcomi st, st( i) 
extern void FCOMI( x86IntRegType src );
// fcomip st, st( i) 
extern void FCOMIP( x86IntRegType src );
// fucomi st, st( i) 
extern void FUCOMI( x86IntRegType src );
// fucomip st, st( i) 
extern void FUCOMIP( x86IntRegType src );
// fcom m32 to fpu reg stack 
extern void FCOM32( u32 from );
// fabs fpu reg stack 
extern void FABS( void );
// fsqrt fpu reg stack 
extern void FSQRT( void );
// ftan fpu reg stack 
extern void FPATAN( void );
// fsin fpu reg stack 
extern void FSIN( void );
// fchs fpu reg stack 
extern void FCHS( void );

// fcmovb fpu reg to fpu reg stack 
extern void FCMOVB32( x86IntRegType from );
// fcmove fpu reg to fpu reg stack 
extern void FCMOVE32( x86IntRegType from );
// fcmovbe fpu reg to fpu reg stack 
extern void FCMOVBE32( x86IntRegType from );
// fcmovu fpu reg to fpu reg stack 
extern void FCMOVU32( x86IntRegType from );
// fcmovnb fpu reg to fpu reg stack 
extern void FCMOVNB32( x86IntRegType from );
// fcmovne fpu reg to fpu reg stack 
extern void FCMOVNE32( x86IntRegType from );
// fcmovnbe fpu reg to fpu reg stack 
extern void FCMOVNBE32( x86IntRegType from );
// fcmovnu fpu reg to fpu reg stack 
extern void FCMOVNU32( x86IntRegType from );
extern void FCOMP32( u32 from );
extern void FNSTSWtoAX( void );

#define MMXONLY(code) code

//******************
// MMX instructions 
//******************

// r64 = mm

// movq m64 to r64 
extern void MOVQMtoR( x86MMXRegType to, uptr from );
// movq r64 to m64 
extern void MOVQRtoM( uptr to, x86MMXRegType from );

// pand r64 to r64 
extern void PANDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pand m64 to r64 ;
extern void PANDMtoR( x86MMXRegType to, uptr from );
// pandn r64 to r64 
extern void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pandn r64 to r64 
extern void PANDNMtoR( x86MMXRegType to, uptr from );
// por r64 to r64 
extern void PORRtoR( x86MMXRegType to, x86MMXRegType from );
// por m64 to r64 
extern void PORMtoR( x86MMXRegType to, uptr from );
// pxor r64 to r64 
extern void PXORRtoR( x86MMXRegType to, x86MMXRegType from );
// pxor m64 to r64 
extern void PXORMtoR( x86MMXRegType to, uptr from );

// psllq r64 to r64 
extern void PSLLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psllq m64 to r64 
extern void PSLLQMtoR( x86MMXRegType to, uptr from );
// psllq imm8 to r64 
extern void PSLLQItoR( x86MMXRegType to, u8 from );
// psrlq r64 to r64 
extern void PSRLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psrlq m64 to r64 
extern void PSRLQMtoR( x86MMXRegType to, uptr from );
// psrlq imm8 to r64 
extern void PSRLQItoR( x86MMXRegType to, u8 from );

// paddusb r64 to r64 
extern void PADDUSBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusb m64 to r64 
extern void PADDUSBMtoR( x86MMXRegType to, uptr from );
// paddusw r64 to r64 
extern void PADDUSWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusw m64 to r64 
extern void PADDUSWMtoR( x86MMXRegType to, uptr from );

// paddb r64 to r64 
extern void PADDBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddb m64 to r64 
extern void PADDBMtoR( x86MMXRegType to, uptr from );
// paddw r64 to r64 
extern void PADDWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddw m64 to r64 
extern void PADDWMtoR( x86MMXRegType to, uptr from );
// paddd r64 to r64 
extern void PADDDRtoR( x86MMXRegType to, x86MMXRegType from );
// paddd m64 to r64 
extern void PADDDMtoR( x86MMXRegType to, uptr from );
extern void PADDSBRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PADDSWRtoR( x86MMXRegType to, x86MMXRegType from );

// paddq m64 to r64 (sse2 only?)
extern void PADDQMtoR( x86MMXRegType to, uptr from );
// paddq r64 to r64 (sse2 only?)
extern void PADDQRtoR( x86MMXRegType to, x86MMXRegType from );

extern void PSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ); 
extern void PSUBSWRtoR( x86MMXRegType to, x86MMXRegType from );

extern void PSUBBRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PSUBWRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PSUBDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PSUBDMtoR( x86MMXRegType to, uptr from );

// psubq m64 to r64 (sse2 only?)
extern void PSUBQMtoR( x86MMXRegType to, uptr from );
// psubq r64 to r64 (sse2 only?)
extern void PSUBQRtoR( x86MMXRegType to, x86MMXRegType from );

// pmuludq m64 to r64 (sse2 only?)
extern void PMULUDQMtoR( x86MMXRegType to, uptr from );
// pmuludq r64 to r64 (sse2 only?)
extern void PMULUDQRtoR( x86MMXRegType to, x86MMXRegType from );

extern void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPEQDMtoR( x86MMXRegType to, uptr from );
extern void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PCMPGTDMtoR( x86MMXRegType to, uptr from );
extern void PSRLWItoR( x86MMXRegType to, u8 from );
extern void PSRLDItoR( x86MMXRegType to, u8 from );
extern void PSRLDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PSLLWItoR( x86MMXRegType to, u8 from );
extern void PSLLDItoR( x86MMXRegType to, u8 from );
extern void PSLLDRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PSRAWItoR( x86MMXRegType to, u8 from );
extern void PSRADItoR( x86MMXRegType to, u8 from );
extern void PSRADRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PUNPCKLDQMtoR( x86MMXRegType to, uptr from );
extern void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from );
extern void PUNPCKHDQMtoR( x86MMXRegType to, uptr from );
extern void MOVQRtoR( x86MMXRegType to, x86MMXRegType from );
extern void MOVQRmtoR( x86MMXRegType to, x86IntRegType from, int offset=0 );
extern void MOVQRtoRm( x86IntRegType  to, x86MMXRegType from, int offset=0 );
extern void MOVDMtoMMX( x86MMXRegType to, uptr from );
extern void MOVDMMXtoM( uptr to, x86MMXRegType from );
extern void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from );
extern void MOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from, int offset=0 );
extern void MOVD32MMXtoR( x86IntRegType to, x86MMXRegType from );
extern void MOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from, int offset=0 );
extern void PINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 );
extern void PSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8);
extern void PSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8);
extern void MASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from);

// emms 
extern void EMMS( void );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word 64bits
//**********************************************************************************
extern void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from);
extern void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from);

extern void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from);

//*********************
// SSE   instructions *
//*********************
extern void SSE_MOVAPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MOVAPS_XMM_to_M128( uptr to, x86SSERegType from );
extern void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE_MOVUPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MOVUPS_XMM_to_M128( uptr to, x86SSERegType from );

extern void SSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from );
extern void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from );
extern void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MOVSS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset=0 );

extern void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from );
extern void SSE_MOVLPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVLPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset=0 );

extern void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from );       
extern void SSE_MOVHPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVHPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset=0 );

extern void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from );
extern void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVLPSRtoRm( x86SSERegType to, x86IntRegType from );
extern void SSE_MOVLPSRtoRm( x86SSERegType to, x86IntRegType from, int offset=0 );

extern void SSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
extern void SSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
extern void SSE_MOVAPSRtoRm( x86IntRegType to, x86SSERegType from, int offset=0 );
extern void SSE_MOVAPSRmtoR( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
extern void SSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale=0 );
extern void SSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from );
extern void SSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from );

extern void SSE_MOVUPSRmtoR( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE_MOVUPSRtoRm( x86SSERegType to, x86IntRegType from, int offset=0 );

extern void SSE2_MOVDQARtoRm( x86IntRegType to, x86SSERegType from, int offset=0 );
extern void SSE2_MOVDQARmtoR( x86SSERegType to, x86IntRegType from, int offset=0 );

extern void SSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_RCPPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_RCPSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_RCPSS_M32_to_XMM( x86SSERegType to, uptr from );

extern void SSE_ORPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_XORPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_ANDPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_ANDNPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_ADDPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_ADDSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_SUBPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_SUBSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MULPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MULSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPEQSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPLTSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPLESS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNESS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNLESS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPORDSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE_UCOMISS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_UCOMISS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
extern void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
extern void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from );
extern void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from );
extern void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from );

extern void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from);
extern void SSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from);
extern void SSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from);
extern void SSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from);

extern void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE2_MAXPD_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MAXPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MAXPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MAXSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_MINPD_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MINPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MINPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_MINSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_RSQRTPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_RSQRTSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_SQRTPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_SQRTSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
extern void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );
extern void SSE_SHUFPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 );
extern void SSE_CMPEQPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPLTPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPLEPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNEPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_CMPORDPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_DIVPS_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE_DIVSS_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
// VectorPath
extern void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
extern void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );

extern void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
extern void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );
extern void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
extern void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );

extern void SSE2_SHUFPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
extern void SSE2_SHUFPD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 );

extern void SSE_STMXCSR( uptr from );
extern void SSE_LDMXCSR( uptr from );


//*********************
//  SSE 2 Instructions*
//*********************

extern void SSE2_CVTSS2SD_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_CVTSS2SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_CVTSD2SS_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_CVTSD2SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE2_MOVDQA_M128_to_XMM(x86SSERegType to, uptr from); 
extern void SSE2_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from); 
extern void SSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from); 

extern void SSE2_MOVDQU_M128_to_XMM(x86SSERegType to, uptr from); 
extern void SSE2_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from); 
extern void SSE2_MOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from); 

extern void SSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSRLW_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSRLD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSRLQ_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSRAW_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSRAD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSLLW_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSLLD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PSLLQ_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
extern void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PMAXSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PMAXUB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PMINSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PMINUB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDSB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBSB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PSUBUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBUSB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PSUBUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBUSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PAND_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PANDN_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PXOR_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDW_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDUSB_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDUSW_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDB_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDD_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PADDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PADDQ_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PMADDWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);

extern void SSE2_ANDPD_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_ANDPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_UCOMISD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_UCOMISD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_SQRTSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_SQRTSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_MAXPD_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MAXPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_MAXSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MAXSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE2_XORPD_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_XORPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_ADDSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_ADDSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_SUBSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_SUBSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE2_MULSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MULSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_DIVSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_DIVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_MINSD_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MINSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
extern void SSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, uptr from);

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
extern void SSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PUNPCKHQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PUNPCKHQDQ_M128_to_XMM(x86SSERegType to, uptr from);

// mult by half words
extern void SSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PMULLW_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PMULHW_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, uptr from);


//**********************************************************************************/
//PMOVMSKB: Create 16bit mask from signs of 8bit integers
//**********************************************************************************
extern void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from);

extern void SSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from);
extern void SSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from);

//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
extern void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 );
extern void SSE_PINSRW_R32_to_XMM(x86SSERegType from, x86IntRegType to, u8 imm8 );


//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
extern void SSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBB_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBW_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBD_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PSUBQ_M128_to_XMM(x86SSERegType to, uptr from );
///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
extern void SSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, uptr from );

//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
extern void SSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from );
extern void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
extern void SSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from );
extern void SSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset=0 );

extern void SSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

extern void SSE2_MOVQ_XMM_to_R( x86IntRegType to, x86SSERegType from );
extern void SSE2_MOVQ_R_to_XMM( x86SSERegType to, x86IntRegType from );
extern void SSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from );
extern void SSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from );

//**********************************************************************************/
//MOVD: Move Qword(64bit) to/from MMX/XMM reg                                      *
//**********************************************************************************
extern void SSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from);
extern void SSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from);


//**********************************************************************************/
//POR : SSE Bitwise OR                                                             *
//**********************************************************************************
extern void SSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSE2_POR_M128_to_XMM( x86SSERegType to, uptr from );

extern void SSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from);

extern void SSE3_MOVSLDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE3_MOVSLDUP_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE3_MOVSHDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE3_MOVSHDUP_M128_to_XMM(x86SSERegType to, uptr from);

// SSSE3

extern void SSSE3_PABSB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSSE3_PABSW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSSE3_PABSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSSE3_PALIGNR_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);

// SSE4.1

#ifndef _MM_MK_INSERTPS_NDX
#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))
#endif

extern void SSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);
extern void SSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8);
extern void SSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);
extern void SSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8);
extern void SSE4_BLENDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8);
extern void SSE4_BLENDVPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_BLENDVPS_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE4_PMOVSXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_PINSRD_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8);
extern void SSE4_PMAXSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_PMINSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_PMAXUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_PMINUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
extern void SSE4_PMAXSD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE4_PMINSD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE4_PMAXUD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE4_PMINUD_M128_to_XMM(x86SSERegType to, uptr from);
extern void SSE4_PMULDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);

//*********************
// 3DNOW instructions * 
//*********************
extern void FEMMS( void );
extern void PFCMPEQMtoR( x86IntRegType to, uptr from );
extern void PFCMPGTMtoR( x86IntRegType to, uptr from );
extern void PFCMPGEMtoR( x86IntRegType to, uptr from );
extern void PFADDMtoR( x86IntRegType to, uptr from );
extern void PFADDRtoR( x86IntRegType to, x86IntRegType from );
extern void PFSUBMtoR( x86IntRegType to, uptr from );
extern void PFSUBRtoR( x86IntRegType to, x86IntRegType from );
extern void PFMULMtoR( x86IntRegType to, uptr from );
extern void PFMULRtoR( x86IntRegType to, x86IntRegType from );
extern void PFRCPMtoR( x86IntRegType to, uptr from );
extern void PFRCPRtoR( x86IntRegType to, x86IntRegType from );
extern void PFRCPIT1RtoR( x86IntRegType to, x86IntRegType from );
extern void PFRCPIT2RtoR( x86IntRegType to, x86IntRegType from );
extern void PFRSQRTRtoR( x86IntRegType to, x86IntRegType from );
extern void PFRSQIT1RtoR( x86IntRegType to, x86IntRegType from );
extern void PF2IDMtoR( x86IntRegType to, uptr from );
extern void PI2FDMtoR( x86IntRegType to, uptr from );
extern void PI2FDRtoR( x86IntRegType to, x86IntRegType from );
extern void PFMAXMtoR( x86IntRegType to, uptr from );
extern void PFMAXRtoR( x86IntRegType to, x86IntRegType from );
extern void PFMINMtoR( x86IntRegType to, uptr from );
extern void PFMINRtoR( x86IntRegType to, x86IntRegType from );

