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

#ifndef __R3000A_H__
#define __R3000A_H__

#include <stdio.h>

union GPRRegs {
	struct {
		u32 r0, at, v0, v1, a0, a1, a2, a3,
			t0, t1, t2, t3, t4, t5, t6, t7,
			s0, s1, s2, s3, s4, s5, s6, s7,
			t8, t9, k0, k1, gp, sp, s8, ra, hi, lo; // hi needs to be at index 32! don't change
	} n;
	u32 r[34]; /* Lo, Hi in r[33] and r[32] */
};

union CP0Regs {
	struct {
		u32 Index,     Random,    EntryLo0,  EntryLo1,
			Context,   PageMask,  Wired,     Reserved0,
			BadVAddr,  Count,     EntryHi,   Compare,
			Status,    Cause,     EPC,       PRid,
			Config,    LLAddr,    WatchLO,   WatchHI,
			XContext,  Reserved1, Reserved2, Reserved3,
			Reserved4, Reserved5, ECC,       CacheErr,
			TagLo,     TagHi,     ErrorEPC,  Reserved6;
	} n;
	u32 r[32];
};

struct SVector2D {
	short x, y;
};

struct SVector2Dz {
	short z, pad;
};

struct SVector3D {
	short x, y, z, pad;
};

struct LVector3D {
	short x, y, z, pad;
};

struct CBGR {
	unsigned char r, g, b, c;
};

struct SMatrix3D {
	short m11, m12, m13, m21, m22, m23, m31, m32, m33, pad;
};

union CP2Data {
	struct {
		SVector3D     v0, v1, v2;
		CBGR          rgb;
		s32          otz;
		s32          ir0, ir1, ir2, ir3;
		SVector2D     sxy0, sxy1, sxy2, sxyp;
		SVector2Dz    sz0, sz1, sz2, sz3;
		CBGR          rgb0, rgb1, rgb2;
		s32          reserved;
		s32          mac0, mac1, mac2, mac3;
		u32 irgb, orgb;
		s32          lzcs, lzcr;
	} n;
	u32 r[32];
};

union CP2Ctrl {
	struct {
		SMatrix3D rMatrix;
		s32      trX, trY, trZ;
		SMatrix3D lMatrix;
		s32      rbk, gbk, bbk;
		SMatrix3D cMatrix;
		s32      rfc, gfc, bfc;
		s32      ofx, ofy;
		s32      h;
		s32      dqa, dqb;
		s32      zsf3, zsf4;
		s32      flag;
	} n;
	u32 r[32];
};

struct psxRegisters {
	GPRRegs GPR;		/* General Purpose Registers */
	CP0Regs CP0;		/* Coprocessor0 Registers */
	CP2Data CP2D; 		/* Cop2 data registers */
	CP2Ctrl CP2C; 		/* Cop2 control registers */
	u32 pc;				/* Program counter */
	u32 code;			/* The instruction */
	u32 cycle;
	u32 interrupt;
	u32 sCycle[64];		// start cycle for signaled ints
	s32 eCycle[64];		// cycle delta for signaled ints (sCycle + eCycle == branch cycle)
	u32 _msflag[32];
	u32 _smflag[32];
};

PCSX2_ALIGNED16_EXTERN(psxRegisters psxRegs);

extern u32 g_psxNextBranchCycle;
extern s32 psxBreak;		// used when the IOP execution is broken and control returned to the EE
extern s32 psxCycleEE;		// tracks IOP's current sych status with the EE

#ifndef _PC_

#define _i32(x) (s32)x
#define _u32(x) x

#define _i16(x) (short)x
#define _u16(x) (unsigned short)x

#define _i8(x) (char)x
#define _u8(x) (unsigned char)x

/**** R3000A Instruction Macros ****/
#define _PC_       psxRegs.pc       // The next PC to be executed

#define _Funct_  ((psxRegs.code      ) & 0x3F)  // The funct part of the instruction register 
#define _Rd_     ((psxRegs.code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Rt_     ((psxRegs.code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Rs_     ((psxRegs.code >> 21) & 0x1F)  // The rs part of the instruction register 
#define _Sa_     ((psxRegs.code >>  6) & 0x1F)  // The sa part of the instruction register
#define _Im_     ((unsigned short)psxRegs.code) // The immediate part of the instruction register
#define _Target_ (psxRegs.code & 0x03ffffff)    // The target part of the instruction register

#define _Imm_	((short)psxRegs.code) // sign-extended immediate
#define _ImmU_	(psxRegs.code&0xffff) // zero-extended immediate

#define _rRs_   psxRegs.GPR.r[_Rs_]   // Rs register
#define _rRt_   psxRegs.GPR.r[_Rt_]   // Rt register
#define _rRd_   psxRegs.GPR.r[_Rd_]   // Rd register
#define _rSa_   psxRegs.GPR.r[_Sa_]   // Sa register
#define _rFs_   psxRegs.CP0.r[_Rd_]   // Fs register

#define _c2dRs_ psxRegs.CP2D.r[_Rs_]  // Rs cop2 data register
#define _c2dRt_ psxRegs.CP2D.r[_Rt_]  // Rt cop2 data register
#define _c2dRd_ psxRegs.CP2D.r[_Rd_]  // Rd cop2 data register
#define _c2dSa_ psxRegs.CP2D.r[_Sa_]  // Sa cop2 data register

#define _rHi_   psxRegs.GPR.n.hi   // The HI register
#define _rLo_   psxRegs.GPR.n.lo   // The LO register

#define _JumpTarget_    ((_Target_ << 2) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
#define _BranchTarget_  (((s32)(s16)_Imm_ * 4) + _PC_)                 // Calculates the target during a branch instruction
//#define _JumpTarget_    ((_Target_ * 4) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
//#define _BranchTarget_  ((short)_Im_ * 4 + _PC_)                 // Calculates the target during a branch instruction

#define _SetLink(x)     psxRegs.GPR.r[x] = _PC_ + 4;       // Sets the return address in the link register

extern s32 EEsCycle;
extern u32 EEoCycle;

#endif

extern s32 psxNextCounter;
extern u32 psxNextsCounter;
extern bool iopBranchAction;
extern bool iopEventTestIsActive;

// Branching status used when throwing exceptions.
extern bool iopIsDelaySlot;

////////////////////////////////////////////////////////////////////
// R3000A  Public Interface / API

struct R3000Acpu {
	void (*Allocate)();
	void (*Reset)();
	void (*Execute)();
	s32 (*ExecuteBlock)( s32 eeCycles );		// executes the given number of EE cycles.
	void (*Clear)(u32 Addr, u32 Size);
	void (*Shutdown)();
};

extern R3000Acpu *psxCpu;
extern R3000Acpu psxInt;
extern R3000Acpu psxRec;

void psxReset();
void psxShutdown();
void psxException(u32 code, u32 step);
void psxBranchTest();
void psxExecuteBios();
void psxMemReset();

// Subsets
extern void (*psxBSC[64])();
extern void (*psxSPC[64])();
extern void (*psxREG[32])();
extern void (*psxCP0[32])();
extern void (*psxCP2[64])();
extern void (*psxCP2BSC[32])();

#endif /* __R3000A_H__ */
