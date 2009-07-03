/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2003  Pcsx2 Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#include "VUmicro.h"

extern u32 vudump;

#define VU0_MEMSIZE 0x1000
#define VU1_MEMSIZE 0x4000

void recResetVU0();
void recExecuteVU0Block();
void recClearVU0( u32 Addr, u32 Size );

void recVU1Init();
void recVU1Shutdown();
void recResetVU1();
void recExecuteVU1Block();
void recClearVU1( u32 Addr, u32 Size );


u32 GetVIAddr(VURegs * VU, int reg, int read, int info); // returns the correct VI addr
void recUpdateFlags(VURegs * VU, int reg, int info);

void _recvuTestPipes(VURegs * VU);
void _recvuFlushFDIV(VURegs * VU);
void _recvuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn);

#define VUOP_READ 2
#define VUOP_WRITE 4

// save on mem
struct _vuopinfo {
	int cycle;
	int cycles;
	u8 statusflag;
	u8 macflag;
	u8 clipflag;
	u8 dummy;
	u8 q;
	u8 p;
	u16 pqinst; // bit of instruction specifying index (srec only)
};

void SuperVUAnalyzeOp(VURegs *VU, _vuopinfo *info, _VURegsNum* pCodeRegs);
int eeVURecompileCode(VURegs *VU, _VURegsNum* regs); // allocates all the necessary regs and returns the indices
void VU1XGKICK_MTGSTransfer(u32 *pMem, u32 addr); // used for MTGS in XGKICK

extern int vucycle;
typedef void (*vFloat)(int regd, int regTemp);
extern vFloat vFloats1[16];
extern vFloat vFloats1_useEAX[16];
extern vFloat vFloats2[16];
extern vFloat vFloats4[16];
extern vFloat vFloats4_useEAX[16];
PCSX2_ALIGNED16_EXTERN(const float s_fones[8]);
PCSX2_ALIGNED16_EXTERN(const u32 s_mask[4]);
PCSX2_ALIGNED16_EXTERN(const u32 s_expmask[4]);
PCSX2_ALIGNED16_EXTERN(const u32 g_minvals[4]);
PCSX2_ALIGNED16_EXTERN(const u32 g_maxvals[4]);
PCSX2_ALIGNED16_EXTERN(const u32 const_clip[8]);

u32 GetVIAddr(VURegs * VU, int reg, int read, int info);
int _vuGetTempXMMreg(int info);
void vuFloat(int info, int regd, int XYZW);
void vuFloat_useEAX(int regd, int regTemp, int XYZW);
void vuFloat2(int regd, int regTemp, int XYZW);
void vuFloat3(uptr x86ptr);
void vuFloat4(int regd, int regTemp, int XYZW);
void vuFloat4_useEAX(int regd, int regTemp, int XYZW);
void vuFloat5(int regd, int regTemp, int XYZW);
void vuFloat5_useEAX(int regd, int regTemp, int XYZW);
void _vuFlipRegSS(VURegs * VU, int reg);
void _vuFlipRegSS_xyzw(int reg, int xyzw);
void _vuMoveSS(VURegs * VU, int dstreg, int srcreg);
void _unpackVF_xyzw(int dstreg, int srcreg, int xyzw);
void _unpackVFSS_xyzw(int dstreg, int srcreg, int xyzw);
void VU_MERGE_REGS_CUSTOM(int dest, int src, int xyzw);
void VU_MERGE_REGS_SAFE(int dest, int src, int xyzw);
#define VU_MERGE_REGS(dest, src) { \
	VU_MERGE_REGS_CUSTOM(dest, src, _X_Y_Z_W); \
}

// use for allocating vi regs
#define ALLOCTEMPX86(mode) _allocX86reg(-1, X86TYPE_TEMP, 0, ((info&PROCESS_VU_SUPER)?0:MODE_NOFRAME)|mode)
#define ALLOCVI(vi, mode) _allocX86reg(-1, X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), vi, ((info&PROCESS_VU_SUPER)?0:MODE_NOFRAME)|mode)
#define ADD_VI_NEEDED(vi) _addNeededX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), vi);

#define SWAP(x, y) *(u32*)&y ^= *(u32*)&x ^= *(u32*)&y ^= *(u32*)&x;

/*****************************************
   VU Micromode Upper instructions
*****************************************/

void recVUMI_ABS(VURegs *vuRegs, int info);
void recVUMI_ADD(VURegs *vuRegs, int info);
void recVUMI_ADDi(VURegs *vuRegs, int info);
void recVUMI_ADDq(VURegs *vuRegs, int info);
void recVUMI_ADDx(VURegs *vuRegs, int info);
void recVUMI_ADDy(VURegs *vuRegs, int info);
void recVUMI_ADDz(VURegs *vuRegs, int info);
void recVUMI_ADDw(VURegs *vuRegs, int info);
void recVUMI_ADDA(VURegs *vuRegs, int info);
void recVUMI_ADDAi(VURegs *vuRegs, int info);
void recVUMI_ADDAq(VURegs *vuRegs, int info);
void recVUMI_ADDAx(VURegs *vuRegs, int info);
void recVUMI_ADDAy(VURegs *vuRegs, int info);
void recVUMI_ADDAz(VURegs *vuRegs, int info);
void recVUMI_ADDAw(VURegs *vuRegs, int info);
void recVUMI_SUB(VURegs *vuRegs, int info);
void recVUMI_SUBi(VURegs *vuRegs, int info);
void recVUMI_SUBq(VURegs *vuRegs, int info);
void recVUMI_SUBx(VURegs *vuRegs, int info);
void recVUMI_SUBy(VURegs *vuRegs, int info);
void recVUMI_SUBz(VURegs *vuRegs, int info);
void recVUMI_SUBw(VURegs *vuRegs, int info);
void recVUMI_SUBA(VURegs *vuRegs, int info);
void recVUMI_SUBAi(VURegs *vuRegs, int info);
void recVUMI_SUBAq(VURegs *vuRegs, int info);
void recVUMI_SUBAx(VURegs *vuRegs, int info);
void recVUMI_SUBAy(VURegs *vuRegs, int info);
void recVUMI_SUBAz(VURegs *vuRegs, int info);
void recVUMI_SUBAw(VURegs *vuRegs, int info);
void recVUMI_MUL(VURegs *vuRegs, int info);
void recVUMI_MULi(VURegs *vuRegs, int info);
void recVUMI_MULq(VURegs *vuRegs, int info);
void recVUMI_MULx(VURegs *vuRegs, int info);
void recVUMI_MULy(VURegs *vuRegs, int info);
void recVUMI_MULz(VURegs *vuRegs, int info);
void recVUMI_MULw(VURegs *vuRegs, int info);
void recVUMI_MULA(VURegs *vuRegs, int info);
void recVUMI_MULAi(VURegs *vuRegs, int info);
void recVUMI_MULAq(VURegs *vuRegs, int info);
void recVUMI_MULAx(VURegs *vuRegs, int info);
void recVUMI_MULAy(VURegs *vuRegs, int info);
void recVUMI_MULAz(VURegs *vuRegs, int info);
void recVUMI_MULAw(VURegs *vuRegs, int info);
void recVUMI_MADD(VURegs *vuRegs, int info);
void recVUMI_MADDi(VURegs *vuRegs, int info);
void recVUMI_MADDq(VURegs *vuRegs, int info);
void recVUMI_MADDx(VURegs *vuRegs, int info);
void recVUMI_MADDy(VURegs *vuRegs, int info);
void recVUMI_MADDz(VURegs *vuRegs, int info);
void recVUMI_MADDw(VURegs *vuRegs, int info);
void recVUMI_MADDA(VURegs *vuRegs, int info);
void recVUMI_MADDAi(VURegs *vuRegs, int info);
void recVUMI_MADDAq(VURegs *vuRegs, int info);
void recVUMI_MADDAx(VURegs *vuRegs, int info);
void recVUMI_MADDAy(VURegs *vuRegs, int info);
void recVUMI_MADDAz(VURegs *vuRegs, int info);
void recVUMI_MADDAw(VURegs *vuRegs, int info);
void recVUMI_MSUB(VURegs *vuRegs, int info);
void recVUMI_MSUBi(VURegs *vuRegs, int info);
void recVUMI_MSUBq(VURegs *vuRegs, int info);
void recVUMI_MSUBx(VURegs *vuRegs, int info);
void recVUMI_MSUBy(VURegs *vuRegs, int info);
void recVUMI_MSUBz(VURegs *vuRegs, int info);
void recVUMI_MSUBw(VURegs *vuRegs, int info);
void recVUMI_MSUBA(VURegs *vuRegs, int info);
void recVUMI_MSUBAi(VURegs *vuRegs, int info);
void recVUMI_MSUBAq(VURegs *vuRegs, int info);
void recVUMI_MSUBAx(VURegs *vuRegs, int info);
void recVUMI_MSUBAy(VURegs *vuRegs, int info);
void recVUMI_MSUBAz(VURegs *vuRegs, int info);
void recVUMI_MSUBAw(VURegs *vuRegs, int info);
void recVUMI_MAX(VURegs *vuRegs, int info);
void recVUMI_MAXi(VURegs *vuRegs, int info);
void recVUMI_MAXx(VURegs *vuRegs, int info);
void recVUMI_MAXy(VURegs *vuRegs, int info);
void recVUMI_MAXz(VURegs *vuRegs, int info);
void recVUMI_MAXw(VURegs *vuRegs, int info);
void recVUMI_MINI(VURegs *vuRegs, int info);
void recVUMI_MINIi(VURegs *vuRegs, int info);
void recVUMI_MINIx(VURegs *vuRegs, int info);
void recVUMI_MINIy(VURegs *vuRegs, int info);
void recVUMI_MINIz(VURegs *vuRegs, int info);
void recVUMI_MINIw(VURegs *vuRegs, int info);
void recVUMI_OPMULA(VURegs *vuRegs, int info);
void recVUMI_OPMSUB(VURegs *vuRegs, int info);
void recVUMI_NOP(VURegs *vuRegs, int info);
void recVUMI_FTOI0(VURegs *vuRegs, int info);
void recVUMI_FTOI4(VURegs *vuRegs, int info);
void recVUMI_FTOI12(VURegs *vuRegs, int info);
void recVUMI_FTOI15(VURegs *vuRegs, int info);
void recVUMI_ITOF0(VURegs *vuRegs, int info);
void recVUMI_ITOF4(VURegs *vuRegs, int info);
void recVUMI_ITOF12(VURegs *vuRegs, int info);
void recVUMI_ITOF15(VURegs *vuRegs, int info);
void recVUMI_CLIP(VURegs *vuRegs, int info);

/*****************************************
   VU Micromode Lower instructions
*****************************************/

void recVUMI_DIV(VURegs *vuRegs, int info);
void recVUMI_SQRT(VURegs *vuRegs, int info);
void recVUMI_RSQRT(VURegs *vuRegs, int info);
void recVUMI_IADD(VURegs *vuRegs, int info);
void recVUMI_IADDI(VURegs *vuRegs, int info);
void recVUMI_IADDIU(VURegs *vuRegs, int info);
void recVUMI_IAND(VURegs *vuRegs, int info);
void recVUMI_IOR(VURegs *vuRegs, int info);
void recVUMI_ISUB(VURegs *vuRegs, int info);
void recVUMI_ISUBIU(VURegs *vuRegs, int info);
void recVUMI_MOVE(VURegs *vuRegs, int info);
void recVUMI_MFIR(VURegs *vuRegs, int info);
void recVUMI_MTIR(VURegs *vuRegs, int info);
void recVUMI_MR32(VURegs *vuRegs, int info);
void recVUMI_LQ(VURegs *vuRegs, int info);
void recVUMI_LQD(VURegs *vuRegs, int info);
void recVUMI_LQI(VURegs *vuRegs, int info);
void recVUMI_SQ(VURegs *vuRegs, int info);
void recVUMI_SQD(VURegs *vuRegs, int info);
void recVUMI_SQI(VURegs *vuRegs, int info);
void recVUMI_ILW(VURegs *vuRegs, int info);
void recVUMI_ISW(VURegs *vuRegs, int info);
void recVUMI_ILWR(VURegs *vuRegs, int info);
void recVUMI_ISWR(VURegs *vuRegs, int info);
void recVUMI_LOI(VURegs *vuRegs, int info);
void recVUMI_RINIT(VURegs *vuRegs, int info);
void recVUMI_RGET(VURegs *vuRegs, int info);
void recVUMI_RNEXT(VURegs *vuRegs, int info);
void recVUMI_RXOR(VURegs *vuRegs, int info);
void recVUMI_WAITQ(VURegs *vuRegs, int info);
void recVUMI_FSAND(VURegs *vuRegs, int info);
void recVUMI_FSEQ(VURegs *vuRegs, int info);
void recVUMI_FSOR(VURegs *vuRegs, int info);
void recVUMI_FSSET(VURegs *vuRegs, int info);
void recVUMI_FMAND(VURegs *vuRegs, int info);
void recVUMI_FMEQ(VURegs *vuRegs, int info);
void recVUMI_FMOR(VURegs *vuRegs, int info);
void recVUMI_FCAND(VURegs *vuRegs, int info);
void recVUMI_FCEQ(VURegs *vuRegs, int info);
void recVUMI_FCOR(VURegs *vuRegs, int info);
void recVUMI_FCSET(VURegs *vuRegs, int info);
void recVUMI_FCGET(VURegs *vuRegs, int info);
void recVUMI_IBEQ(VURegs *vuRegs, int info);
void recVUMI_IBGEZ(VURegs *vuRegs, int info);
void recVUMI_IBGTZ(VURegs *vuRegs, int info);
void recVUMI_IBLTZ(VURegs *vuRegs, int info);
void recVUMI_IBLEZ(VURegs *vuRegs, int info);
void recVUMI_IBNE(VURegs *vuRegs, int info);
void recVUMI_B(VURegs *vuRegs, int info);
void recVUMI_BAL(VURegs *vuRegs, int info);
void recVUMI_JR(VURegs *vuRegs, int info);
void recVUMI_JALR(VURegs *vuRegs, int info);
void recVUMI_MFP(VURegs *vuRegs, int info);
void recVUMI_WAITP(VURegs *vuRegs, int info);
void recVUMI_ESADD(VURegs *vuRegs, int info);
void recVUMI_ERSADD(VURegs *vuRegs, int info);
void recVUMI_ELENG(VURegs *vuRegs, int info);
void recVUMI_ERLENG(VURegs *vuRegs, int info);
void recVUMI_EATANxy(VURegs *vuRegs, int info);
void recVUMI_EATANxz(VURegs *vuRegs, int info);
void recVUMI_ESUM(VURegs *vuRegs, int info);
void recVUMI_ERCPR(VURegs *vuRegs, int info);
void recVUMI_ESQRT(VURegs *vuRegs, int info);
void recVUMI_ERSQRT(VURegs *vuRegs, int info);
void recVUMI_ESIN(VURegs *vuRegs, int info); 
void recVUMI_EATAN(VURegs *vuRegs, int info);
void recVUMI_EEXP(VURegs *vuRegs, int info);
void recVUMI_XGKICK(VURegs *vuRegs, int info);
void recVUMI_XTOP(VURegs *vuRegs, int info);
void recVUMI_XITOP(VURegs *vuRegs, int info);
void recVUMI_XTOP( VURegs *VU , int info);

