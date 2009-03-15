/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2-Playground Team
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
// Global Variables
//------------------------------------------------------------------

PCSX2_ALIGNED16_EXTERN(const u32 mVU_absclip[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_signbit[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_minvals[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_maxvals[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T1[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T2[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T3[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T4[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T5[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T6[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T7[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_T8[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_Pi4[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_S2[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_S3[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_S4[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_S5[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E1[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E2[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E3[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E4[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E5[4]);
PCSX2_ALIGNED16_EXTERN(const u32 mVU_E6[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_FTOI_4[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_FTOI_12[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_FTOI_15[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_ITOF_4[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_ITOF_12[4]);
PCSX2_ALIGNED16_EXTERN(const float mVU_ITOF_15[4]);

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ ((mVU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((mVU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((mVU->code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X ((mVU->code>>24) & 0x1)
#define _Y ((mVU->code>>23) & 0x1)
#define _Z ((mVU->code>>22) & 0x1)
#define _W ((mVU->code>>21) & 0x1)

#define _XYZW_SS (_X+_Y+_Z+_W==1)

#define _X_Y_Z_W  (((mVU->code >> 21 ) & 0xF ))
#define _xyzw_ACC ((_XYZW_SS && !_X) ? 15 : _X_Y_Z_W)

#define _bc_	 (mVU->code & 0x03)
#define _bc_x	((mVU->code & 0x03) == 0)
#define _bc_y	((mVU->code & 0x03) == 1)
#define _bc_z	((mVU->code & 0x03) == 2)
#define _bc_w	((mVU->code & 0x03) == 3)

#define _Fsf_	((mVU->code >> 21) & 0x03)
#define _Ftf_	((mVU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(mVU->code & 0x400 ? 0xfffffc00 | (mVU->code & 0x3ff) : mVU->code & 0x3ff)
#define _UImm11_	(s32)(mVU->code & 0x7ff)
#define _Imm12_		(((mVU->code >> 21 ) & 0x1) << 11) | (mVU->code & 0x7ff)
#define _Imm5_		(((mVU->code & 0x400) ? 0xfff0 : 0) | ((mVU->code >> 6) & 0xf))
#define _Imm15_		(((mVU->code >> 10) & 0x7800) | (mVU->code & 0x7ff))
#define _Imm24_		(u32)(mVU->code & 0xffffff)

#define getVUmem(x)	(((vuIndex == 1) ? (x & 0x3ff) : ((x >= 0x400) ? (x & 0x43f) : (x & 0xff))) * 16)
#define offsetSS	((_X) ? (0) : ((_Y) ? (4) : ((_Z) ? 8: 12)))

#define xmmT1	0 // Temp Reg
#define xmmFs	1 // Holds the Value of Fs (writes back result Fd)
#define xmmFt	2 // Holds the Value of Ft
#define xmmACC0	3 // Holds ACC Instance #0
#define xmmACC1	4 // Holds ACC Instance #1
#define xmmACC2	5 // Holds ACC Instance #2
#define xmmACC3	6 // Holds ACC Instance #3
#define xmmPQ	7 // Holds the Value and Backup Values of P and Q regs

#define mmxVI1	0 // Holds VI 1
#define mmxVI2	1 // Holds VI 2
#define mmxVI3	2 // Holds VI 3
#define mmxVI4	3 // Holds VI 4
#define mmxVI5	4 // Holds VI 5
#define mmxVI6	5 // Holds VI 6
#define mmxVI7	6 // Holds VI 7
#define mmxVI8	7 // Holds VI 8

#define gprT1	0 // Temp Reg
#define gprT2	1 // Temp Reg
#define gprR	2 // R Reg
#define gprF0	3 // MAC Flag::Status Flag 0
#define gprESP	4 // Don't use?
#define gprF1	5 // MAC Flag::Status Flag 1
#define gprF2	6 // MAC Flag::Status Flag 2
#define gprF3	7 // MAC Flag::Status Flag 3

// Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define microVUt(aType) template<int vuIndex> __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex, int recPass> aType
#define microVUq(aType) template<int vuIndex, int recPass>  __forceinline aType

#define mVUallocInfo mVU->prog.prog[mVU->prog.cur].allocInfo

#define isNOP		 (mVUallocInfo.info[mVUallocInfo.curPC] & (1<<0))
#define writeACC	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<1)) >> 1)
#define prevACC		(((u8)((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<1)) >> 1) - 1) & 0x3)
#define readACC		((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<3)) >> 3)
#define writeQ		((mVUallocInfo.info[mVUallocInfo.curPC] & (1<<5)) >> 5)
#define readQ		((mVUallocInfo.info[mVUallocInfo.curPC] & (1<<6)) >> 6)
#define writeP		((mVUallocInfo.info[mVUallocInfo.curPC] & (1<<7)) >> 7)
#define readP		((mVUallocInfo.info[mVUallocInfo.curPC] & (1<<7)) >> 7) // same as write
#define doFlags		 (mVUallocInfo.info[mVUallocInfo.curPC] & (3<<8))
#define doMac		 (mVUallocInfo.info[mVUallocInfo.curPC] & (1<<8))
#define doStatus	 (mVUallocInfo.info[mVUallocInfo.curPC] & (1<<9))
#define fmInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<10)) >> 10)
#define fsInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<12)) >> 12)
#define fpmInstance	(((u8)((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<10)) >> 10) - 1) & 0x3)
#define fpsInstance	(((u8)((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<12)) >> 12) - 1) & 0x3)
#define fvmInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<14)) >> 14)
#define fvsInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<16)) >> 16)
#define fvcInstance	1//((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<14)) >> 14)
#define fcInstance	1//((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<14)) >> 14)
//#define getFs		 (mVUallocInfo.info[mVUallocInfo.curPC] & (1<<13))
//#define getFt		 (mVUallocInfo.info[mVUallocInfo.curPC] & (1<<14))

#define isMMX(_VIreg_)	(_VIreg_ >= 1 && _VIreg_ <=9)
#define mmVI(_VIreg_)	(_VIreg_ - 1)

#include "microVU_Misc.inl"
