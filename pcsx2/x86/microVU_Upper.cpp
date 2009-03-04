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
#include "PrecompiledHeader.h"
#include "microVU.h"
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// mVUupdateFlags() - Updates status/mac flags
//------------------------------------------------------------------

microVUt(void) mVUupdateFlags(int reg, int regT1, int regT2, int xyzw) {
	microVU* mVU = mVUx;
	static u8 *pjmp, *pjmp2;
	static u32 *pjmp32;
	static u32 macaddr, stataddr, prevstataddr;
	static int x86macflag, x86statflag, x86temp;

	//SysPrintf ("mVUupdateFlags\n");
	if( !(doFlags) ) return;

	//macaddr = VU_VI_ADDR(REG_MAC_FLAG, 0);
	//stataddr = VU_VI_ADDR(REG_STATUS_FLAG, 0); // write address
	//prevstataddr = VU_VI_ADDR(REG_STATUS_FLAG, 2); // previous address


	SSE2_PSHUFD_XMM_to_XMM(regT1, reg, 0x1B); // Flip wzyx to xyzw 
	MOV32MtoR(x86statflag, prevstataddr); // Load the previous status in to x86statflag
	AND16ItoR(x86statflag, 0xff0); // Keep Sticky and D/I flags

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM(regT2, regT2); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM(regT2, regT1); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(EAX, regT2); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM(regT2, regT1);

	SSE_MOVMSKPS_XMM_to_R32(x86macflag, regT2); // Move the sign bits of the t1reg

	AND16ItoR(x86macflag, _X_Y_Z_W );  // Grab "Is Signed" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		OR16ItoR(x86statflag, 0x82); // SS, S flags
		SHL16ItoR(x86macflag, 4);
		if (_XYZW_SS) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
	x86SetJ8(pjmp);

	//-------------------------Check for Zero flags------------------------------

	AND16ItoR(EAX, _X_Y_Z_W );  // Grab "Is Zero" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		OR16ItoR(x86statflag, 0x41); // ZS, Z flags
		OR32RtoR(x86macflag, EAX);
	x86SetJ8(pjmp);

	//-------------------------Finally: Send the Flags to the Mac Flag Address------------------------------

	if (_XYZW_SS) x86SetJ8(pjmp2); // If we skipped the Zero Flag Checking, return here

	MOV16RtoM(macaddr, x86macflag);
	MOV16RtoM(stataddr, x86statflag);
}

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define mVU_FMAC1(operation) {								\
	if (isNOP) return;										\
	int Fd, Fs, Ft;											\
	mVUallocFMAC1a<vuIndex>(Fd, Fs, Ft, 1);					\
	if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
	else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
	mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W);		\
	mVUallocFMAC1b<vuIndex>(Fd);							\
}

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS(){}
microVUf(void) mVU_ADD() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { mVU_FMAC1(ADD); }
}
microVUf(void) mVU_ADDi(){}
microVUf(void) mVU_ADDq(){}
microVUf(void) mVU_ADDx(){}
microVUf(void) mVU_ADDy(){}
microVUf(void) mVU_ADDz(){}
microVUf(void) mVU_ADDw(){}
microVUf(void) mVU_ADDA(){}
microVUf(void) mVU_ADDAi(){}
microVUf(void) mVU_ADDAq(){}
microVUf(void) mVU_ADDAx(){}
microVUf(void) mVU_ADDAy(){}
microVUf(void) mVU_ADDAz(){}
microVUf(void) mVU_ADDAw(){}
microVUf(void) mVU_SUB(){
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { mVU_FMAC1(SUB); }
}
microVUf(void) mVU_SUBi(){}
microVUf(void) mVU_SUBq(){}
microVUf(void) mVU_SUBx(){}
microVUf(void) mVU_SUBy(){}
microVUf(void) mVU_SUBz(){}
microVUf(void) mVU_SUBw(){}
microVUf(void) mVU_SUBA(){}
microVUf(void) mVU_SUBAi(){}
microVUf(void) mVU_SUBAq(){}
microVUf(void) mVU_SUBAx(){}
microVUf(void) mVU_SUBAy(){}
microVUf(void) mVU_SUBAz(){}
microVUf(void) mVU_SUBAw(){}
microVUf(void) mVU_MUL(){
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { mVU_FMAC1(MUL); }
}
microVUf(void) mVU_MULi(){}
microVUf(void) mVU_MULq(){}
microVUf(void) mVU_MULx(){}
microVUf(void) mVU_MULy(){}
microVUf(void) mVU_MULz(){}
microVUf(void) mVU_MULw(){}
microVUf(void) mVU_MULA(){}
microVUf(void) mVU_MULAi(){}
microVUf(void) mVU_MULAq(){}
microVUf(void) mVU_MULAx(){}
microVUf(void) mVU_MULAy(){}
microVUf(void) mVU_MULAz(){}
microVUf(void) mVU_MULAw(){}
microVUf(void) mVU_MADD(){}
microVUf(void) mVU_MADDi(){}
microVUf(void) mVU_MADDq(){}
microVUf(void) mVU_MADDx(){}
microVUf(void) mVU_MADDy(){}
microVUf(void) mVU_MADDz(){}
microVUf(void) mVU_MADDw(){}
microVUf(void) mVU_MADDA(){}
microVUf(void) mVU_MADDAi(){}
microVUf(void) mVU_MADDAq(){}
microVUf(void) mVU_MADDAx(){}
microVUf(void) mVU_MADDAy(){}
microVUf(void) mVU_MADDAz(){}
microVUf(void) mVU_MADDAw(){}
microVUf(void) mVU_MSUB(){}
microVUf(void) mVU_MSUBi(){}
microVUf(void) mVU_MSUBq(){}
microVUf(void) mVU_MSUBx(){}
microVUf(void) mVU_MSUBy(){}
microVUf(void) mVU_MSUBz(){}
microVUf(void) mVU_MSUBw(){}
microVUf(void) mVU_MSUBA(){}
microVUf(void) mVU_MSUBAi(){}
microVUf(void) mVU_MSUBAq(){}
microVUf(void) mVU_MSUBAx(){}
microVUf(void) mVU_MSUBAy(){}
microVUf(void) mVU_MSUBAz(){}
microVUf(void) mVU_MSUBAw(){}
microVUf(void) mVU_MAX(){}
microVUf(void) mVU_MAXi(){}
microVUf(void) mVU_MAXx(){}
microVUf(void) mVU_MAXy(){}
microVUf(void) mVU_MAXz(){}
microVUf(void) mVU_MAXw(){}
microVUf(void) mVU_MINI(){}
microVUf(void) mVU_MINIi(){}
microVUf(void) mVU_MINIx(){}
microVUf(void) mVU_MINIy(){}
microVUf(void) mVU_MINIz(){}
microVUf(void) mVU_MINIw(){}
microVUf(void) mVU_OPMULA(){}
microVUf(void) mVU_OPMSUB(){}
microVUf(void) mVU_NOP(){}
microVUf(void) mVU_FTOI0(){}
microVUf(void) mVU_FTOI4(){}
microVUf(void) mVU_FTOI12(){}
microVUf(void) mVU_FTOI15(){}
microVUf(void) mVU_ITOF0(){}
microVUf(void) mVU_ITOF4(){}
microVUf(void) mVU_ITOF12(){}
microVUf(void) mVU_ITOF15(){}
microVUf(void) mVU_CLIP(){}
#endif //PCSX2_MICROVU
