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
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// mVUupdateFlags() - Updates status/mac flags
//------------------------------------------------------------------

#define AND_XYZW (_XYZW_SS ? (1) : (doMac ? (_X_Y_Z_W) : (flipMask[_X_Y_Z_W])))

microVUt(void) mVUupdateFlags(int reg, int regT1, int regT2, int xyzw) {
	microVU* mVU = mVUx;
	static u8 *pjmp, *pjmp2;
	static const int flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	//SysPrintf ("mVUupdateFlags\n");
	if( !(doFlags) ) return;

	if (!doMac) { regT1 = reg; }
	else SSE2_PSHUFD_XMM_to_XMM(regT1, reg, 0x1B); // Flip wzyx to xyzw
	if (doStatus) {
		SSE_PEXTRW_XMM_to_R32(gprT1, xmmF, fpsInstance); // Get Prev Status Flag
		AND16ItoR(gprT1, 0xff0); // Keep Sticky and D/I flags
	}

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM(regT2, regT2); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM(regT2, regT1); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(gprT3, regT2); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM(regT2, regT1);

	SSE_MOVMSKPS_XMM_to_R32(gprT2, regT2); // Move the sign bits of the t1reg

	AND16ItoR(gprT2, AND_XYZW );  // Grab "Is Signed" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		if (doMac)	  SHL16ItoR(gprT2, 4);
		if (doStatus) OR16ItoR(gprT1, 0x82); // SS, S flags
		if (_XYZW_SS) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
	x86SetJ8(pjmp);

	//-------------------------Check for Zero flags------------------------------

	AND16ItoR(gprT3, AND_XYZW );  // Grab "Is Zero" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		if (doMac)	  OR32RtoR(gprT2, gprT3);	
		if (doStatus) OR16ItoR(gprT1, 0x41); // ZS, Z flags		
	x86SetJ8(pjmp);

	//-------------------------Finally: Send the Flags to the Mac Flag Address------------------------------

	if (_XYZW_SS) x86SetJ8(pjmp2); // If we skipped the Zero Flag Checking, return here

	if (doMac)	  SSE_PINSRW_R32_to_XMM(xmmF, gprT2, fmInstance); // Set Mac Flag
	if (doStatus) SSE_PINSRW_R32_to_XMM(xmmF, gprT1, fsInstance); // Set Status Flag
}

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define mVU_FMAC1(operation) {									\
	microVU* mVU = mVUx;										\
	if (recPass == 0) {}										\
	else {														\
		int Fd, Fs, Ft;											\
		if (isNOP) return;										\
		mVUallocFMAC1a<vuIndex>(Fd, Fs, Ft);					\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W);		\
		mVUallocFMAC1b<vuIndex>(Fd);							\
	}															\
}

#define mVU_FMAC3(operation) {									\
	microVU* mVU = mVUx;										\
	if (recPass == 0) {}										\
	else {														\
		int Fd, Fs, Ft;											\
		if (isNOP) return;										\
		mVUallocFMAC3a<vuIndex>(Fd, Fs, Ft);					\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W);		\
		mVUallocFMAC3b<vuIndex>(Fd);							\
	}															\
}

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int Fs, Ft;
		if (isNOP) return;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);
		SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_absclip);
		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_ADD()	 { mVU_FMAC1(ADD); }
microVUf(void) mVU_ADDi(){}
microVUf(void) mVU_ADDq(){}
microVUq(void) mVU_ADDxyzw() { mVU_FMAC3(ADD); }
microVUf(void) mVU_ADDx()	 { mVU_ADDxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_ADDy()	 { mVU_ADDxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_ADDz()	 { mVU_ADDxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_ADDw()	 { mVU_ADDxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_ADDA(){}
microVUf(void) mVU_ADDAi(){}
microVUf(void) mVU_ADDAq(){}
microVUf(void) mVU_ADDAx(){}
microVUf(void) mVU_ADDAy(){}
microVUf(void) mVU_ADDAz(){}
microVUf(void) mVU_ADDAw(){}
microVUf(void) mVU_SUB()	 { mVU_FMAC1(SUB); }
microVUf(void) mVU_SUBi(){}
microVUf(void) mVU_SUBq(){}
microVUq(void) mVU_SUBxyzw() { mVU_FMAC3(SUB); }
microVUf(void) mVU_SUBx()	 { mVU_SUBxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_SUBy()	 { mVU_SUBxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_SUBz()	 { mVU_SUBxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_SUBw()	 { mVU_SUBxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_SUBA(){}
microVUf(void) mVU_SUBAi(){}
microVUf(void) mVU_SUBAq(){}
microVUf(void) mVU_SUBAx(){}
microVUf(void) mVU_SUBAy(){}
microVUf(void) mVU_SUBAz(){}
microVUf(void) mVU_SUBAw(){}
microVUf(void) mVU_MUL()	 { mVU_FMAC1(MUL); }
microVUf(void) mVU_MULi(){}
microVUf(void) mVU_MULq(){}
microVUq(void) mVU_MULxyzw() { mVU_FMAC3(MUL); }
microVUf(void) mVU_MULx()	 { mVU_MULxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MULy()	 { mVU_MULxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MULz()	 { mVU_MULxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MULw()	 { mVU_MULxyzw<vuIndex, recPass>(); }
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
microVUf(void) mVU_MAX()	 { mVU_FMAC1(MAX); }
microVUf(void) mVU_MAXi(){}
microVUq(void) mVU_MAXxyzw() { mVU_FMAC3(MAX); }
microVUf(void) mVU_MAXx()	 { mVU_MAXxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MAXy()	 { mVU_MAXxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MAXz()	 { mVU_MAXxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MAXw()	 { mVU_MAXxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MINI()	 { mVU_FMAC1(MIN); }
microVUf(void) mVU_MINIi(){}
microVUq(void) mVU_MINIxyzw(){ mVU_FMAC3(MIN); }
microVUf(void) mVU_MINIx()	 { mVU_MINIxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MINIy()	 { mVU_MINIxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MINIz()	 { mVU_MINIxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_MINIw()	 { mVU_MINIxyzw<vuIndex, recPass>(); }
microVUf(void) mVU_OPMULA(){}
microVUf(void) mVU_OPMSUB(){}
microVUf(void) mVU_NOP(){}
microVUq(void) mVU_FTOIx(uptr addr) {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int Fs, Ft;
		if (isNOP) return;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);

		// Note: For help understanding this algorithm see recVUMI_FTOI_Saturate()
		SSE_MOVAPS_XMM_to_XMM(xmmT1, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Fs, addr); }
		SSE2_CVTTPS2DQ_XMM_to_XMM(Fs, Fs);
		SSE2_PXOR_M128_to_XMM(xmmT1, (uptr)mVU_signbit);
		SSE2_PSRAD_I8_to_XMM(xmmT1, 31);
		SSE_MOVAPS_XMM_to_XMM(xmmFt, Fs);
		SSE2_PCMPEQD_M128_to_XMM(xmmFt, (uptr)mVU_signbit);
		SSE_ANDPS_XMM_to_XMM(xmmT1, xmmFt);
		SSE2_PADDD_XMM_to_XMM(Fs, xmmT1);

		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_FTOI0()	 { mVU_FTOIx<vuIndex, recPass>(0); }
microVUf(void) mVU_FTOI4()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_4); }
microVUf(void) mVU_FTOI12()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_12); }
microVUf(void) mVU_FTOI15()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_15); }
microVUq(void) mVU_ITOFx(uptr addr) {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int Fs, Ft;
		if (isNOP) return;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);

		SSE2_CVTDQ2PS_XMM_to_XMM(Ft, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Ft, addr); }
		//mVUclamp2(Ft, xmmT1, 15); // Clamp infinities (not sure if this is needed)
		
		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_ITOF0()	 { mVU_ITOFx<vuIndex, recPass>(0); }
microVUf(void) mVU_ITOF4()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_4); }
microVUf(void) mVU_ITOF12()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_12); }
microVUf(void) mVU_ITOF15()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_15); }
microVUf(void) mVU_CLIP(){}
#endif //PCSX2_MICROVU
