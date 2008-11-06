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

#ifndef __IVUMICRO_H__
#define __IVUMICRO_H__

#define VU0_MEMSIZE 0x1000
#define VU1_MEMSIZE 0x4000

#define RECOMPILE_VUMI_ABS
#define RECOMPILE_VUMI_SUB
#define RECOMPILE_VUMI_SUBA
#define RECOMPILE_VUMI_MADD
#define RECOMPILE_VUMI_MADDA
#define RECOMPILE_VUMI_MSUB
#define RECOMPILE_VUMI_MSUBA

#define RECOMPILE_VUMI_ADD
#define RECOMPILE_VUMI_ADDA
#define RECOMPILE_VUMI_MUL
#define RECOMPILE_VUMI_MULA
#define RECOMPILE_VUMI_MAX
#define RECOMPILE_VUMI_MINI
#define RECOMPILE_VUMI_FTOI

#define RECOMPILE_VUMI_MATH
#define RECOMPILE_VUMI_MISC
#define RECOMPILE_VUMI_E
#define RECOMPILE_VUMI_X
#define RECOMPILE_VUMI_RANDOM
#define RECOMPILE_VUMI_FLAG
#define RECOMPILE_VUMI_BRANCH
#define RECOMPILE_VUMI_ARITHMETIC
#define RECOMPILE_VUMI_LOADSTORE

#ifdef __x86_64__
#undef RECOMPILE_VUMI_X
#endif

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
typedef struct {
	int cycle;
	int cycles;
	u8 statusflag;
	u8 macflag;
	u8 clipflag;
	u8 dummy;
	u8 q;
	u8 p;
	u16 pqinst; // bit of instruction specifying index (srec only)
} _vuopinfo;
extern _vuopinfo *cinfo;

void SuperVUAnalyzeOp(VURegs *VU, _vuopinfo *info, _VURegsNum* pCodeRegs);

// allocates all the necessary regs and returns the indices
int eeVURecompileCode(VURegs *VU, _VURegsNum* regs);

void VU1XGKICK_MTGSTransfer(u32 *pMem, u32 addr); // used for MTGS in XGKICK

extern _VURegsNum* g_VUregs;

#define SWAP(x, y) *(u32*)&y ^= *(u32*)&x ^= *(u32*)&y ^= *(u32*)&x;
#define VUREC_INFO eeVURecompileCode(VU, g_VUregs)

extern int vucycle;
extern int vucycleold;


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

#endif /* __IVUMICRO_H__ */
