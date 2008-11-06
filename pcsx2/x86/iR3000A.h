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
#ifndef _R3000A_SUPERREC_
#define _R3000A_SUPERREC_

extern void __Log(char *fmt, ...);

// to be consistent with EE
#define PSX_HI XMMGPR_HI
#define PSX_LO XMMGPR_LO

extern uptr *psxRecLUT;

u8 _psxLoadWritesRs(u32 tempcode);
u8 _psxIsLoadStore(u32 tempcode);

void _psxFlushAllUnused();
int _psxFlushUnusedConstReg();
void _psxFlushCachedRegs();
void _psxFlushConstReg(int reg);
void _psxFlushConstRegs();

void _psxDeleteReg(int reg, int flush);
void _psxFlushCall(int flushtype);

void _psxOnWriteReg(int reg);
void PSX_CHECK_SAVE_REG(int reg);

extern u32 psxpc;			// recompiler pc
extern int psxbranch;		// set for branch

void psxSaveBranchState();
void psxLoadBranchState();

void psxSetBranchReg(u32 reg);
void psxSetBranchImm( u32 imm );
void psxRecompileNextInstruction(int delayslot);

typedef void (*R3000AFNPTR)();
typedef void (*R3000AFNPTR_INFO)(int info);

void psxRecClearMem(BASEBLOCK* p);

//
// non mmx/xmm version, slower
//
// rd = rs op rt
#define PSXRECOMPILE_CONSTCODE0(fn) \
void rpsx##fn(void) \
{ \
	psxRecompileCodeConst0(rpsx##fn##_const, rpsx##fn##_consts, rpsx##fn##_constt, rpsx##fn##_); \
} \

// rt = rs op imm16
#define PSXRECOMPILE_CONSTCODE1(fn) \
void rpsx##fn(void) \
{ \
	psxRecompileCodeConst1(rpsx##fn##_const, rpsx##fn##_); \
} \

// rd = rt op sa
#define PSXRECOMPILE_CONSTCODE2(fn) \
void rpsx##fn(void) \
{ \
	psxRecompileCodeConst2(rpsx##fn##_const, rpsx##fn##_); \
} \

// [lo,hi] = rt op rs
#define PSXRECOMPILE_CONSTCODE3(fn, LOHI) \
void rpsx##fn(void) \
{ \
	psxRecompileCodeConst3(rpsx##fn##_const, rpsx##fn##_consts, rpsx##fn##_constt, rpsx##fn##_, LOHI); \
} \

// rd = rs op rt
void psxRecompileCodeConst0(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode);
// rt = rs op imm16
void psxRecompileCodeConst1(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode);
// rd = rt op sa
void psxRecompileCodeConst2(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode);
// [lo,hi] = rt op rs
void psxRecompileCodeConst3(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode, int LOHI);

#endif
