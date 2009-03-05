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

#define _bc_	 (mVU->code & 0x03)
#define _bc_x	((mVU->code & 0x03) == 0)
#define _bc_y	((mVU->code & 0x03) == 1)
#define _bc_z	((mVU->code & 0x03) == 2)
#define _bc_w	((mVU->code & 0x03) == 3)

#define _Fsf_ ((mVU->code >> 21) & 0x03)
#define _Ftf_ ((mVU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(mVU->code & 0x400 ? 0xfffffc00 | (mVU->code & 0x3ff) : mVU->code & 0x3ff)
#define _UImm11_	(s32)(mVU->code & 0x7ff)

#define xmmT1	0 // Temp Reg
#define xmmFs	1 // Holds the Value of Fs (writes back result Fd)
#define xmmFt	2 // Holds the Value of Ft
#define xmmACC1	3 // Holds the Value of ACC
#define xmmACC2	4 // Holds the Backup Value of ACC
#define xmmPQ	5 // Holds the Value and Backup Values of P and Q regs
#define xmmVI	6 // Holds VI regs 8, 9, 10, 11, 12, 13, 14, and 15
#define xmmF	7 // Holds 4 instances of the status and mac flags (macflagX4::statusflagX4)

#define gprT1	0 // Temp Reg
#define gprT2	1 // Temp Reg
#define gprT3	2 // Temp Reg
#define gprVI7	3 // VI 7
#define gprESP	4 // Don't use?
#define gprVI5	5 // VI 6::5
#define gprVI3	6 // VI 4::3
#define gprVI1	7 // VI 2::1

// Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define microVUt(aType) template<int vuIndex> __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex, int recPass> aType
#define microVUq(aType) template<int vuIndex, int recPass>  __forceinline aType

#define mVUallocInfo mVU->prog.prog[mVU->prog.cur].allocInfo

#define isNOP		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<0))
#define getFd		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<1))
#define getFs		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<2))
#define getFt		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<3))
#define setFd		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<7))
#define doFlags		(mVUallocInfo.info[mVUallocInfo.curPC] & (3<<8))
#define doMac		(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<8))
#define doStatus	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<9))
#define fmInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<10)) + 4)
#define fsInstance	((mVUallocInfo.info[mVUallocInfo.curPC] & (3<<12)) + 0)
#define fpmInstance	((((u8)(mVUallocInfo.info[mVUallocInfo.curPC] & (3<<10)) - 1) & 0x3) + 4)
#define fpsInstance	((((u8)(mVUallocInfo.info[mVUallocInfo.curPC] & (3<<10)) - 1) & 0x3) + 0)

#include "microVU_Misc.inl"
