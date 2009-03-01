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

#ifdef __LINUX__
#include "ix86/ix86.h"
#endif

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ ((code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X ((code>>24) & 0x1)
#define _Y ((code>>23) & 0x1)
#define _Z ((code>>22) & 0x1)
#define _W ((code>>21) & 0x1)

#define _XYZW_SS (_X+_Y+_Z+_W==1)

#define _X_Y_Z_W  (((code >> 21 ) & 0xF ) )

#define _Fsf_ ((code >> 21) & 0x03)
#define _Ftf_ ((code >> 23) & 0x03)

#define _Imm11_ 	(s32)(code & 0x400 ? 0xfffffc00 | (code & 0x3ff) : code & 0x3ff)
#define _UImm11_	(s32)(code & 0x7ff)
/*
#define VU_VFx_ADDR(x)  (uptr)&VU->VF[x].UL[0]
#define VU_VFy_ADDR(x)  (uptr)&VU->VF[x].UL[1]
#define VU_VFz_ADDR(x)  (uptr)&VU->VF[x].UL[2]
#define VU_VFw_ADDR(x)  (uptr)&VU->VF[x].UL[3]

#define VU_REGR_ADDR    (uptr)&VU->VI[REG_R]
#define VU_REGQ_ADDR    (uptr)&VU->VI[REG_Q]
#define VU_REGMAC_ADDR  (uptr)&VU->VI[REG_MAC_FLAG]

#define VU_VI_ADDR(x, read) GetVIAddr(VU, x, read, info)

#define VU_ACCx_ADDR    (uptr)&VU->ACC.UL[0]
#define VU_ACCy_ADDR    (uptr)&VU->ACC.UL[1]
#define VU_ACCz_ADDR    (uptr)&VU->ACC.UL[2]
#define VU_ACCw_ADDR    (uptr)&VU->ACC.UL[3]
*/

#define xmmT1	0 // XMM0
#define xmmFd	1 // XMM1
#define xmmFs	2 // XMM2
#define xmmFt	3 // XMM3
#define xmmACC1	4 // XMM4
#define xmmACC2	5 // XMM5
#define xmmPQ	6 // XMM6
#define xmmZ	7 // XMM7

// Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define microVUt(aType) template<int vuIndex> __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex, int recPass> aType

microVUx(void) mVUsaveReg(u32 code, int reg, u32 offset);
