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
*  You should have meived a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

void mVU_ABS(VURegs *vuRegs, int info);
void mVU_ADD(VURegs *vuRegs, int info);
void mVU_ADDi(VURegs *vuRegs, int info);
void mVU_ADDq(VURegs *vuRegs, int info);
void mVU_ADDx(VURegs *vuRegs, int info);
void mVU_ADDy(VURegs *vuRegs, int info);
void mVU_ADDz(VURegs *vuRegs, int info);
void mVU_ADDw(VURegs *vuRegs, int info);
void mVU_ADDA(VURegs *vuRegs, int info);
void mVU_ADDAi(VURegs *vuRegs, int info);
void mVU_ADDAq(VURegs *vuRegs, int info);
void mVU_ADDAx(VURegs *vuRegs, int info);
void mVU_ADDAy(VURegs *vuRegs, int info);
void mVU_ADDAz(VURegs *vuRegs, int info);
void mVU_ADDAw(VURegs *vuRegs, int info);
void mVU_SUB(VURegs *vuRegs, int info);
void mVU_SUBi(VURegs *vuRegs, int info);
void mVU_SUBq(VURegs *vuRegs, int info);
void mVU_SUBx(VURegs *vuRegs, int info);
void mVU_SUBy(VURegs *vuRegs, int info);
void mVU_SUBz(VURegs *vuRegs, int info);
void mVU_SUBw(VURegs *vuRegs, int info);
void mVU_SUBA(VURegs *vuRegs, int info);
void mVU_SUBAi(VURegs *vuRegs, int info);
void mVU_SUBAq(VURegs *vuRegs, int info);
void mVU_SUBAx(VURegs *vuRegs, int info);
void mVU_SUBAy(VURegs *vuRegs, int info);
void mVU_SUBAz(VURegs *vuRegs, int info);
void mVU_SUBAw(VURegs *vuRegs, int info);
void mVU_MUL(VURegs *vuRegs, int info);
void mVU_MULi(VURegs *vuRegs, int info);
void mVU_MULq(VURegs *vuRegs, int info);
void mVU_MULx(VURegs *vuRegs, int info);
void mVU_MULy(VURegs *vuRegs, int info);
void mVU_MULz(VURegs *vuRegs, int info);
void mVU_MULw(VURegs *vuRegs, int info);
void mVU_MULA(VURegs *vuRegs, int info);
void mVU_MULAi(VURegs *vuRegs, int info);
void mVU_MULAq(VURegs *vuRegs, int info);
void mVU_MULAx(VURegs *vuRegs, int info);
void mVU_MULAy(VURegs *vuRegs, int info);
void mVU_MULAz(VURegs *vuRegs, int info);
void mVU_MULAw(VURegs *vuRegs, int info);
void mVU_MADD(VURegs *vuRegs, int info);
void mVU_MADDi(VURegs *vuRegs, int info);
void mVU_MADDq(VURegs *vuRegs, int info);
void mVU_MADDx(VURegs *vuRegs, int info);
void mVU_MADDy(VURegs *vuRegs, int info);
void mVU_MADDz(VURegs *vuRegs, int info);
void mVU_MADDw(VURegs *vuRegs, int info);
void mVU_MADDA(VURegs *vuRegs, int info);
void mVU_MADDAi(VURegs *vuRegs, int info);
void mVU_MADDAq(VURegs *vuRegs, int info);
void mVU_MADDAx(VURegs *vuRegs, int info);
void mVU_MADDAy(VURegs *vuRegs, int info);
void mVU_MADDAz(VURegs *vuRegs, int info);
void mVU_MADDAw(VURegs *vuRegs, int info);
void mVU_MSUB(VURegs *vuRegs, int info);
void mVU_MSUBi(VURegs *vuRegs, int info);
void mVU_MSUBq(VURegs *vuRegs, int info);
void mVU_MSUBx(VURegs *vuRegs, int info);
void mVU_MSUBy(VURegs *vuRegs, int info);
void mVU_MSUBz(VURegs *vuRegs, int info);
void mVU_MSUBw(VURegs *vuRegs, int info);
void mVU_MSUBA(VURegs *vuRegs, int info);
void mVU_MSUBAi(VURegs *vuRegs, int info);
void mVU_MSUBAq(VURegs *vuRegs, int info);
void mVU_MSUBAx(VURegs *vuRegs, int info);
void mVU_MSUBAy(VURegs *vuRegs, int info);
void mVU_MSUBAz(VURegs *vuRegs, int info);
void mVU_MSUBAw(VURegs *vuRegs, int info);
void mVU_MAX(VURegs *vuRegs, int info);
void mVU_MAXi(VURegs *vuRegs, int info);
void mVU_MAXx(VURegs *vuRegs, int info);
void mVU_MAXy(VURegs *vuRegs, int info);
void mVU_MAXz(VURegs *vuRegs, int info);
void mVU_MAXw(VURegs *vuRegs, int info);
void mVU_MINI(VURegs *vuRegs, int info);
void mVU_MINIi(VURegs *vuRegs, int info);
void mVU_MINIx(VURegs *vuRegs, int info);
void mVU_MINIy(VURegs *vuRegs, int info);
void mVU_MINIz(VURegs *vuRegs, int info);
void mVU_MINIw(VURegs *vuRegs, int info);
void mVU_OPMULA(VURegs *vuRegs, int info);
void mVU_OPMSUB(VURegs *vuRegs, int info);
void mVU_NOP(VURegs *vuRegs, int info);
void mVU_FTOI0(VURegs *vuRegs, int info);
void mVU_FTOI4(VURegs *vuRegs, int info);
void mVU_FTOI12(VURegs *vuRegs, int info);
void mVU_FTOI15(VURegs *vuRegs, int info);
void mVU_ITOF0(VURegs *vuRegs, int info);
void mVU_ITOF4(VURegs *vuRegs, int info);
void mVU_ITOF12(VURegs *vuRegs, int info);
void mVU_ITOF15(VURegs *vuRegs, int info);
void mVU_CLIP(VURegs *vuRegs, int info);

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

void mVU_DIV(VURegs *vuRegs, int info);
void mVU_SQRT(VURegs *vuRegs, int info);
void mVU_RSQRT(VURegs *vuRegs, int info);
void mVU_IADD(VURegs *vuRegs, int info);
void mVU_IADDI(VURegs *vuRegs, int info);
void mVU_IADDIU(VURegs *vuRegs, int info);
void mVU_IAND(VURegs *vuRegs, int info);
void mVU_IOR(VURegs *vuRegs, int info);
void mVU_ISUB(VURegs *vuRegs, int info);
void mVU_ISUBIU(VURegs *vuRegs, int info);
void mVU_MOVE(VURegs *vuRegs, int info);
void mVU_MFIR(VURegs *vuRegs, int info);
void mVU_MTIR(VURegs *vuRegs, int info);
void mVU_MR32(VURegs *vuRegs, int info);
void mVU_LQ(VURegs *vuRegs, int info);
void mVU_LQD(VURegs *vuRegs, int info);
void mVU_LQI(VURegs *vuRegs, int info);
void mVU_SQ(VURegs *vuRegs, int info);
void mVU_SQD(VURegs *vuRegs, int info);
void mVU_SQI(VURegs *vuRegs, int info);
void mVU_ILW(VURegs *vuRegs, int info);
void mVU_ISW(VURegs *vuRegs, int info);
void mVU_ILWR(VURegs *vuRegs, int info);
void mVU_ISWR(VURegs *vuRegs, int info);
void mVU_LOI(VURegs *vuRegs, int info);
void mVU_RINIT(VURegs *vuRegs, int info);
void mVU_RGET(VURegs *vuRegs, int info);
void mVU_RNEXT(VURegs *vuRegs, int info);
void mVU_RXOR(VURegs *vuRegs, int info);
void mVU_WAITQ(VURegs *vuRegs, int info);
void mVU_FSAND(VURegs *vuRegs, int info);
void mVU_FSEQ(VURegs *vuRegs, int info);
void mVU_FSOR(VURegs *vuRegs, int info);
void mVU_FSSET(VURegs *vuRegs, int info);
void mVU_FMAND(VURegs *vuRegs, int info);
void mVU_FMEQ(VURegs *vuRegs, int info);
void mVU_FMOR(VURegs *vuRegs, int info);
void mVU_FCAND(VURegs *vuRegs, int info);
void mVU_FCEQ(VURegs *vuRegs, int info);
void mVU_FCOR(VURegs *vuRegs, int info);
void mVU_FCSET(VURegs *vuRegs, int info);
void mVU_FCGET(VURegs *vuRegs, int info);
void mVU_IBEQ(VURegs *vuRegs, int info);
void mVU_IBGEZ(VURegs *vuRegs, int info);
void mVU_IBGTZ(VURegs *vuRegs, int info);
void mVU_IBLTZ(VURegs *vuRegs, int info);
void mVU_IBLEZ(VURegs *vuRegs, int info);
void mVU_IBNE(VURegs *vuRegs, int info);
void mVU_B(VURegs *vuRegs, int info);
void mVU_BAL(VURegs *vuRegs, int info);
void mVU_JR(VURegs *vuRegs, int info);
void mVU_JALR(VURegs *vuRegs, int info);
void mVU_MFP(VURegs *vuRegs, int info);
void mVU_WAITP(VURegs *vuRegs, int info);
void mVU_ESADD(VURegs *vuRegs, int info);
void mVU_ERSADD(VURegs *vuRegs, int info);
void mVU_ELENG(VURegs *vuRegs, int info);
void mVU_ERLENG(VURegs *vuRegs, int info);
void mVU_EATANxy(VURegs *vuRegs, int info);
void mVU_EATANxz(VURegs *vuRegs, int info);
void mVU_ESUM(VURegs *vuRegs, int info);
void mVU_ERCPR(VURegs *vuRegs, int info);
void mVU_ESQRT(VURegs *vuRegs, int info);
void mVU_ERSQRT(VURegs *vuRegs, int info);
void mVU_ESIN(VURegs *vuRegs, int info); 
void mVU_EATAN(VURegs *vuRegs, int info);
void mVU_EEXP(VURegs *vuRegs, int info);
void mVU_XGKICK(VURegs *vuRegs, int info);
void mVU_XTOP(VURegs *vuRegs, int info);
void mVU_XITOP(VURegs *vuRegs, int info);
void mVU_XTOP( VURegs *VU , int info);
