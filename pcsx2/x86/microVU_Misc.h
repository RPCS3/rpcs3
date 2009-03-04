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

#define _X_Y_Z_W  (((mVU->code >> 21 ) & 0xF ) )

#define _Fsf_ ((mVU->code >> 21) & 0x03)
#define _Ftf_ ((mVU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(mVU->code & 0x400 ? 0xfffffc00 | (mVU->code & 0x3ff) : mVU->code & 0x3ff)
#define _UImm11_	(s32)(mVU->code & 0x7ff)

#define xmmT1	0 // XMM0 // Temp Reg
#define xmmFd	1 // XMM1 // Holds the Value of Fd
#define xmmFs	2 // XMM2 // Holds the Value of Fs
#define xmmFt	3 // XMM3 // Holds the Value of Ft
#define xmmACC1	4 // XMM4 // Holds the Value of ACC
#define xmmACC2	5 // XMM5 // Holds the Backup Value of ACC
#define xmmPQ	6 // XMM6 // Holds the Value and Backup Values of P and Q regs
#define xmmF	7 // XMM7 // Holds 4 instances of the status and mac flags (macflagX4::statusflagX4)

// Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define microVUt(aType) template<int vuIndex> __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex, int recPass> aType

#define mVUallocInfo mVU->prog.prog[mVU->prog.cur].allocInfo

#define isNOP	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<0))
#define getFd	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<1))
#define getFs	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<2))
#define getFt	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<3))
#define setFd	(mVUallocInfo.info[mVUallocInfo.curPC] & (1<<7))
#define doFlags	(mVUallocInfo.info[mVUallocInfo.curPC] & (3<<8))

#include "microVU_Misc.inl"
