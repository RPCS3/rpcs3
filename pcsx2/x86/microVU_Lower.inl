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
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

microVUf(void) mVU_DIV() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		//u8 *pjmp;, *pjmp1;
		u32 *ajmp32, *bjmp32;

		getReg5(xmmFs, _Fs_, _Fsf_);
		getReg5(xmmFt, _Ft_, _Ftf_);

		//AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags

		// FT can be zero here! so we need to check if its zero and set the correct flag.
		SSE_XORPS_XMM_to_XMM(xmmT1, xmmT1); // Clear xmmT1
		SSE_CMPEQPS_XMM_to_XMM(xmmT1, xmmFt); // Set all F's if each vector is zero
		SSE_MOVMSKPS_XMM_to_R32(gprT1, xmmT1); // Move the sign bits of the previous calculation

		AND32ItoR(gprT1, 1);  // Grab "Is Zero" bits from the previous calculation
		ajmp32 = JZ32(0); // Skip if none are
			//SSE_XORPS_XMM_to_XMM(xmmT1, xmmT1); // Clear xmmT1
			//SSE_CMPEQPS_XMM_to_XMM(xmmT1, xmmFs); // Set all F's if each vector is zero
			//SSE_MOVMSKPS_XMM_to_R32(gprT1, xmmT1); // Move the sign bits of the previous calculation

			//AND32ItoR(gprT1, 1);  // Grab "Is Zero" bits from the previous calculation
			//pjmp = JZ8(0);
			//	OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410 ); // Set invalid flag (0/0)
			//	pjmp1 = JMP8(0);
			//x86SetJ8(pjmp);
			//	OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820 ); // Zero divide (only when not 0/0)
			//x86SetJ8(pjmp1);

			SSE_XORPS_XMM_to_XMM(xmmFs, xmmFt);
			SSE_ANDPS_M128_to_XMM(xmmFs, (uptr)mVU_signbit);
			SSE_ORPS_M128_to_XMM(xmmFs, (uptr)mVU_maxvals); // If division by zero, then xmmFs = +/- fmax

			bjmp32 = JMP32(0);
		x86SetJ32(ajmp32);

		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);
		mVUclamp1<vuIndex>(xmmFs, xmmFt, 8);

		x86SetJ32(bjmp32);

		mVUunpack_xyzw<vuIndex>(xmmFs, xmmFs, 0);
		mVUmergeRegs<vuIndex>(xmmPQ, xmmFs, writeQ ? 4 : 8);
	}
}
microVUf(void) mVU_SQRT() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		//u8* pjmp;
		getReg5(xmmFt, _Ft_, _Ftf_);

		//AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags
		/* Check for negative sqrt */
		//SSE_MOVMSKPS_XMM_to_R32(gprT1, xmmFt);
		//AND32ItoR(gprT1, 1);  //Check sign
		//pjmp = JZ8(0); //Skip if none are
		//	OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); // Invalid Flag - Negative number sqrt
		//x86SetJ8(pjmp);

		SSE_ANDPS_M128_to_XMM(xmmFt, (uptr)mVU_absclip); // Do a cardinal sqrt
		if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(xmmFt, (uptr)mVU_maxvals); // Clamp infinities (only need to do positive clamp since xmmFt is positive)
		SSE_SQRTSS_XMM_to_XMM(xmmFt, xmmFt);
		mVUunpack_xyzw<vuIndex>(xmmFt, xmmFt, 0);
		mVUmergeRegs<vuIndex>(xmmPQ, xmmFt, writeQ ? 4 : 8);
	}
}
microVUf(void) mVU_RSQRT() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		u8 *ajmp8, *bjmp8;

		getReg5(xmmFs, _Fs_, _Fsf_);
		getReg5(xmmFt, _Ft_, _Ftf_);

		//AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags
		/* Check for negative divide */
		//SSE_MOVMSKPS_XMM_to_R32(gprT1, xmmT1);
		//AND32ItoR(gprT1, 1);  //Check sign
		//ajmp8 = JZ8(0); //Skip if none are
		//	OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); // Invalid Flag - Negative number sqrt
		//x86SetJ8(ajmp8);

		SSE_ANDPS_M128_to_XMM(xmmFt, (uptr)mVU_absclip); // Do a cardinal sqrt
		SSE_SQRTSS_XMM_to_XMM(xmmFt, xmmFt);

		// Ft can still be zero here! so we need to check if its zero and set the correct flag.
		SSE_XORPS_XMM_to_XMM(xmmT1, xmmT1); // Clear t1reg
		SSE_CMPEQSS_XMM_to_XMM(xmmT1, xmmFt); // Set all F's if each vector is zero

		SSE_MOVMSKPS_XMM_to_R32(gprT1, xmmT1); // Move the sign bits of the previous calculation

		AND32ItoR(gprT1, 1);  // Grab "Is Zero" bits from the previous calculation
		ajmp8 = JZ8(0); // Skip if none are
			//OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); // Zero divide flag
			SSE_ANDPS_M128_to_XMM(xmmFs, (uptr)mVU_signbit);
			SSE_ORPS_M128_to_XMM(xmmFs, (uptr)mVU_maxvals); // EEREC_TEMP = +/-Max
			bjmp8 = JMP8(0);
		x86SetJ8(ajmp8);
			SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);
			mVUclamp1<vuIndex>(xmmFs, xmmFt, 8);
		x86SetJ8(bjmp8);

		mVUunpack_xyzw<vuIndex>(xmmFs, xmmFs, 0);
		mVUmergeRegs<vuIndex>(xmmPQ, xmmFs, writeQ ? 4 : 8);
	}
}

#define EATANhelper(addr) {						\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);			\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);			\
	SSE_MOVSS_XMM_to_XMM(xmmFt, xmmT1);			\
	SSE_MULSS_M32_to_XMM(xmmFt, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFt);			\
}
microVUt(void) mVU_EATAN_() {
	microVU* mVU = mVUx;

	// ToDo: Can Be Optimized Further? (takes approximately (~115 cycles + mem access time) on a c2d)
	SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
	SSE_MULSS_M32_to_XMM(xmmPQ, (uptr)mVU_T1);
	SSE_MOVSS_XMM_to_XMM(xmmT1, xmmFs);

	EATANhelper(mVU_T2);
	EATANhelper(mVU_T3);
	EATANhelper(mVU_T4);
	EATANhelper(mVU_T5);
	EATANhelper(mVU_T6);
	EATANhelper(mVU_T7);
	EATANhelper(mVU_T8);

	SSE_ADDSS_M32_to_XMM(xmmPQ, (uptr)mVU_Pi4);
	SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6);
}
microVUf(void) mVU_EATAN() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		// ToDo: Can Be Optimized Further? (takes approximately (~125 cycles + mem access time) on a c2d)
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_ADDSS_M32_to_XMM(xmmPQ, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);

		mVU_EATAN_<vuIndex>();
	}
}
microVUf(void) mVU_EATANxy() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		getReg5(xmmFs, _Fs_, 1);
		getReg5(xmmFt, _Fs_, 0);
		SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_ADDSS_XMM_to_XMM(xmmFt, xmmPQ);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);

		mVU_EATAN_<vuIndex>();
	}
}
microVUf(void) mVU_EATANxz() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		getReg5(xmmFs, _Fs_, 2);
		getReg5(xmmFt, _Fs_, 0);
		SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_XMM_to_XMM(xmmFs, xmmFt);
		SSE_ADDSS_XMM_to_XMM(xmmFt, xmmPQ);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);

		mVU_EATAN_<vuIndex>();
	}
}
microVUf(void) mVU_EEXP() {}
microVUf(void) mVU_ELENG() {}
microVUf(void) mVU_ERCPR() {}
microVUf(void) mVU_ERLENG() {}
microVUf(void) mVU_ERSADD() {}
microVUf(void) mVU_ERSQRT() {}
microVUf(void) mVU_ESADD() {}
microVUf(void) mVU_ESIN() {} 
microVUf(void) mVU_ESQRT() {}
microVUf(void) mVU_ESUM() {}

microVUf(void) mVU_FCAND() {}
microVUf(void) mVU_FCEQ() {}
microVUf(void) mVU_FCOR() {}
microVUf(void) mVU_FCSET() {}
microVUf(void) mVU_FCGET() {}

microVUf(void) mVU_FMAND() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		AND16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FMEQ() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		CMP16RtoR(gprT1, gprT2);
		SETE8R(gprT1);
		AND16ItoR(gprT1, 0x1);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FMOR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		OR16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}

microVUf(void) mVU_FSAND() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		AND16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSEQ() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		CMP16ItoR(gprT1, _Imm12_);
		SETE8R(gprT1);
		AND16ItoR(gprT1, 0x1);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSOR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		OR16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSSET() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int flagReg;
		getFlagReg(flagReg, fsInstance);
		MOV16ItoR(gprT1, (_Imm12_ & 0xfc0));
	}
}

microVUf(void) mVU_IADD() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		mVUallocVIa<vuIndex>(gprT2, _Ft_);
		ADD16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Fd_);
	}
}
microVUf(void) mVU_IADDI() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		ADD16ItoR(gprT1, _Imm5_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_IADDIU() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		ADD16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_IAND() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		mVUallocVIa<vuIndex>(gprT2, _Ft_);
		AND32RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Fd_);
	}
}
microVUf(void) mVU_IOR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		mVUallocVIa<vuIndex>(gprT2, _Ft_);
		OR32RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Fd_);
	}
}
microVUf(void) mVU_ISUB() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		mVUallocVIa<vuIndex>(gprT2, _Ft_);
		SUB16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Fd_);
	}
}
microVUf(void) mVU_ISUBIU() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		SUB16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}

microVUf(void) mVU_B() {}
microVUf(void) mVU_BAL() {}
microVUf(void) mVU_IBEQ() {}
microVUf(void) mVU_IBGEZ() {}
microVUf(void) mVU_IBGTZ() {}
microVUf(void) mVU_IBLTZ() {}
microVUf(void) mVU_IBLEZ() {}
microVUf(void) mVU_IBNE() {}
microVUf(void) mVU_JR() {}
microVUf(void) mVU_JALR() {}

microVUf(void) mVU_ILW() {}
microVUf(void) mVU_ISW() {}
microVUf(void) mVU_ILWR() {}
microVUf(void) mVU_ISWR() {}

microVUf(void) mVU_MOVE() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], _X_Y_Z_W);
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MFIR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		MOVSX32R16toR(gprT1, gprT1);
		SSE2_MOVD_R_to_XMM(xmmT1, gprT1);
		if (!_XYZW_SS) { mVUunpack_xyzw<vuIndex>(xmmT1, xmmT1, 0); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MFP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		getPreg(xmmFt);
		mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MTIR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->VF[_Fs_].UL[_Fsf_]);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_MR32() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], (_X_Y_Z_W == 8) ? 4 : 15);
		if (_X_Y_Z_W != 8) { SSE_SHUFPS_XMM_to_XMM(xmmT1, xmmT1, 0x39); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}

microVUf(void) mVU_LQ() {}
microVUf(void) mVU_LQD() {}
microVUf(void) mVU_LQI() {}
microVUf(void) mVU_SQ() {}
microVUf(void) mVU_SQD() {}
microVUf(void) mVU_SQI() {}
//microVUf(void) mVU_LOI() {}

microVUf(void) mVU_RINIT() {}
microVUf(void) mVU_RGET() {}
microVUf(void) mVU_RNEXT() {}
microVUf(void) mVU_RXOR() {}

microVUf(void) mVU_WAITP() {}
microVUf(void) mVU_WAITQ() {}

microVUf(void) mVU_XGKICK() {}
microVUf(void) mVU_XTOP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR( gprT1, (uptr)&mVU->regs->vifRegs->top);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_XITOP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR( gprT1, (uptr)&mVU->regs->vifRegs->itop );
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
#endif //PCSX2_MICROVU
