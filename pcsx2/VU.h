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
#include "Vif.h"

#define REG_STATUS_FLAG	16
#define REG_MAC_FLAG	17
#define REG_CLIP_FLAG	18
#define REG_ACC_FLAG	19 // dummy flag that indicates that VFACC is written/read (nothing to do with VI[19])
#define REG_R			20
#define REG_I			21
#define REG_Q			22
#define REG_P           23 // only exists in micromode 
#define REG_VF0_FLAG	24 // dummy flag that indicates VF0 is read (nothing to do with VI[24])
#define REG_TPC			26
#define REG_CMSAR0		27
#define REG_FBRST		28
#define REG_VPU_STAT	29
#define REG_CMSAR1		31

//interpreter hacks, WIP
//#define INT_VUSTALLHACK //some games work without those, big speedup
//#define INT_VUDOUBLEHACK

enum VUStatus {
	VU_Ready = 0,
	VU_Run   = 1,
	VU_Stop  = 2,
};

union VECTOR {
	struct {
		float x,y,z,w;
	} f;
	struct {
		u32 x,y,z,w;
	} i;

	float F[4];
	
	u64 UD[2];      //128 bits
	s64 SD[2];
	u32 UL[4];
	s32 SL[4];
	u16 US[8];
	s16 SS[8];
	u8  UC[16];
	s8  SC[16];
};

struct REG_VI {
	union {
		float F;
		s32   SL;
		u32	  UL;
		s16   SS[2];
		u16   US[2];
		s8    SC[4];
		u8    UC[4];
	};
	u32 padding[3]; // needs padding to make them 128bit; VU0 maps VU1's VI regs as 128bits to addr 0x4xx0 in
					// VU0 mem, with only lower 16 bits valid, and the upper 112bits are hardwired to 0 (cottonvibes)
};

#define VUFLAG_BREAKONMFLAG		0x00000001
#define VUFLAG_MFLAGSET			0x00000002

struct fdivPipe {
	int enable;
	REG_VI reg;
	u32 sCycle;
	u32 Cycle;
	u32 statusflag;
};

struct efuPipe {
	int enable;
	REG_VI reg;
	u32 sCycle;
	u32 Cycle;
};

struct fmacPipe {
	int enable;
	int reg;
	int xyzw;
	u32 sCycle;
	u32 Cycle;
	u32 macflag;
	u32 statusflag;
	u32 clipflag;
};

struct ialuPipe {
	int enable;
	int reg;
	u32 sCycle;
	u32 Cycle;
};

struct VURegs {
	VECTOR	VF[32]; // VF and VI need to be first in this struct for proper mapping
	REG_VI	VI[32]; // needs to be 128bit x 32 (cottonvibes)
	VECTOR ACC;
	REG_VI q;
	REG_VI p;

	u32 macflag;
	u32 statusflag;
	u32 clipflag;

	u32 cycle;
	u32 flags;

	void (*vuExec)(VURegs*);
	VIFregisters *vifRegs;

	u8 *Mem;
	u8 *Micro;

	u32 code;
	u32 maxmem;
	u32 maxmicro;

	u16 branch;
	u16 ebit;
	u32 branchpc;

	fmacPipe fmac[8];
	fdivPipe fdiv;
	efuPipe efu;
	ialuPipe ialu[8];

	VURegs() :
		Mem( NULL )
	,	Micro( NULL )
	{
	}
};

#define VUPIPE_NONE		0
#define VUPIPE_FMAC		1
#define VUPIPE_FDIV		2
#define VUPIPE_EFU		3
#define VUPIPE_IALU		4
#define VUPIPE_BRANCH	5
#define VUPIPE_XGKICK	6

#define VUREG_READ		0x1
#define VUREG_WRITE		0x2

struct _VURegsNum {
	u8 pipe; // if 0xff, COP2
	u8 VFwrite;
	u8 VFwxyzw;
	u8 VFr0xyzw;
	u8 VFr1xyzw;
	u8 VFread0;
	u8 VFread1;
	u32 VIwrite;
	u32 VIread;
	int cycles;
};

extern VURegs* g_pVU1;
PCSX2_ALIGNED16_EXTERN(VURegs VU0);

#define VU1 (*g_pVU1)


#ifdef _WIN32
extern __forceinline u32* GET_VU_MEM(VURegs* VU, u32 addr)
#else
static __forceinline u32* GET_VU_MEM(VURegs* VU, u32 addr)
#endif
{
	if( VU == g_pVU1 ) return (u32*)(VU1.Mem+(addr&0x3fff));
	
	if( addr >= 0x4000 ) return (u32*)(VU0.Mem+(addr&0x43f0)); // get VF and VI regs (they're mapped to 0x4xx0 in VU0 mem!)
	
	return (u32*)(VU0.Mem+(addr&0x0fff)); // for addr 0x0000 to 0x4000 just wrap around
}
