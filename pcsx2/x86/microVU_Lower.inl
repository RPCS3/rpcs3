/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2 Team
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

//------------------------------------------------------------------
// DIV/SQRT/RSQRT
//------------------------------------------------------------------

#define testZero(xmmReg, xmmTemp, gprTemp) {										\
	SSE_XORPS_XMM_to_XMM(xmmTemp, xmmTemp);		/* Clear xmmTemp (make it 0) */		\
	SSE_CMPEQPS_XMM_to_XMM(xmmTemp, xmmReg);	/* Set all F's if zero */			\
	SSE_MOVMSKPS_XMM_to_R32(gprTemp, xmmTemp);	/* Move the sign bits */			\
	TEST32ItoR(gprTemp, 1);						/* Test "Is Zero" bit */			\
}

#define testNeg(xmmReg, gprTemp, aJump) {											\
	SSE_MOVMSKPS_XMM_to_R32(gprTemp, xmmReg);										\
	TEST32ItoR(gprTemp, 1);								  /* Check sign bit */		\
	aJump = JZ8(0);										  /* Skip if positive */	\
		MOV32ItoM((uptr)&mVU->divFlag, 0x410);			  /* Set Invalid Flags */	\
		SSE_ANDPS_M128_to_XMM(xmmReg, (uptr)mVU_absclip); /* Abs(xmmReg) */			\
	x86SetJ8(aJump);																\
}

microVUf(void) mVU_DIV() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFDIV<vuIndex>(_Fs_, _Fsf_, _Ft_, _Ftf_, 7); }
	pass2 { 
		u8 *ajmp, *bjmp, *cjmp, *djmp;
		getReg5(xmmFs, _Fs_, _Fsf_);
		getReg5(xmmFt, _Ft_, _Ftf_);

		testZero(xmmFt, xmmT1, gprT1); // Test if Ft is zero
		cjmp = JZ8(0); // Skip if not zero

			testZero(xmmFs, xmmT1, gprT1); // Test if Fs is zero
			ajmp = JZ8(0);
				MOV32ItoM((uptr)&mVU->divFlag, 0x410); // Set invalid flag (0/0)
				bjmp = JMP8(0);
			x86SetJ8(ajmp);
				MOV32ItoM((uptr)&mVU->divFlag, 0x820); // Zero divide (only when not 0/0)
			x86SetJ8(bjmp);

			SSE_XORPS_XMM_to_XMM(xmmFs, xmmFt);
			SSE_ANDPS_M128_to_XMM(xmmFs, (uptr)mVU_signbit);
			SSE_ORPS_XMM_to_XMM(xmmFs, xmmMax); // If division by zero, then xmmFs = +/- fmax

			djmp = JMP8(0);
		x86SetJ8(cjmp);
			MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
			SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);
			mVUclamp1<vuIndex>(xmmFs, xmmFt, 8);
		x86SetJ8(djmp);

		mVUunpack_xyzw<vuIndex>(xmmFs, xmmFs, 0);
		mVUmergeRegs(xmmPQ, xmmFs, writeQ ? 4 : 8);
	}
	pass3 { mVUlog("DIV Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

microVUf(void) mVU_SQRT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFDIV<vuIndex>(0, 0, _Ft_, _Ftf_, 7); }
	pass2 { 
		u8 *ajmp;
		getReg5(xmmFt, _Ft_, _Ftf_);

		MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
		testNeg(xmmFt, gprT1, ajmp); // Check for negative sqrt

		if (CHECK_VU_OVERFLOW) SSE_MINSS_XMM_to_XMM(xmmFt, xmmMax); // Clamp infinities (only need to do positive clamp since xmmFt is positive)
		SSE_SQRTSS_XMM_to_XMM(xmmFt, xmmFt);
		mVUunpack_xyzw<vuIndex>(xmmFt, xmmFt, 0);
		mVUmergeRegs(xmmPQ, xmmFt, writeQ ? 4 : 8);
	}
	pass3 { mVUlog("SQRT Q, vf%02d%s", _Ft_, _Ftf_String); }
}

microVUf(void) mVU_RSQRT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFDIV<vuIndex>(_Fs_, _Fsf_, _Ft_, _Ftf_, 13); }
	pass2 { 
		u8 *ajmp, *bjmp, *cjmp, *djmp;
		getReg5(xmmFs, _Fs_, _Fsf_);
		getReg5(xmmFt, _Ft_, _Ftf_);

		MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
		testNeg(xmmFt, gprT1, ajmp); // Check for negative sqrt

		SSE_SQRTSS_XMM_to_XMM(xmmFt, xmmFt);
		testZero(xmmFt, xmmT1, gprT1); // Test if Ft is zero
		ajmp = JZ8(0); // Skip if not zero

			testZero(xmmFs, xmmT1, gprT1); // Test if Fs is zero
			bjmp = JZ8(0); // Skip if none are
				MOV32ItoM((uptr)&mVU->divFlag, 0x410); // Set invalid flag (0/0)
				cjmp = JMP8(0);
			x86SetJ8(bjmp);
				MOV32ItoM((uptr)&mVU->divFlag, 0x820); // Zero divide flag (only when not 0/0)
			x86SetJ8(cjmp);

			SSE_ANDPS_M128_to_XMM(xmmFs, (uptr)mVU_signbit);
			SSE_ORPS_XMM_to_XMM(xmmFs, xmmMax); // xmmFs = +/-Max

			djmp = JMP8(0);
		x86SetJ8(ajmp);
			SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);
			mVUclamp1<vuIndex>(xmmFs, xmmFt, 8);
		x86SetJ8(djmp);

		mVUunpack_xyzw<vuIndex>(xmmFs, xmmFs, 0);
		mVUmergeRegs(xmmPQ, xmmFs, writeQ ? 4 : 8);
	}
	pass3 { mVUlog("RSQRT Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

//------------------------------------------------------------------
// EATAN/EEXP/ELENG/ERCPR/ERLENG/ERSADD/ERSQRT/ESADD/ESIN/ESQRT/ESUM
//------------------------------------------------------------------

#define EATANhelper(addr) {						\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);			\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);			\
	SSE_MOVAPS_XMM_to_XMM(xmmFt, xmmT1);		\
	SSE_MULSS_M32_to_XMM(xmmFt, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFt);			\
}

microVUt(void) mVU_EATAN_() {
	microVU* mVU = mVUx;

	// ToDo: Can Be Optimized Further? (takes approximately (~115 cycles + mem access time) on a c2d)
	SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
	SSE_MULSS_M32_to_XMM(xmmPQ, (uptr)mVU_T1);
	SSE_MOVAPS_XMM_to_XMM(xmmT1, xmmFs);

	EATANhelper(mVU_T2);
	EATANhelper(mVU_T3);
	EATANhelper(mVU_T4);
	EATANhelper(mVU_T5);
	EATANhelper(mVU_T6);
	EATANhelper(mVU_T7);
	EATANhelper(mVU_T8);

	SSE_ADDSS_M32_to_XMM(xmmPQ, (uptr)mVU_Pi4);
	SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6);
}

microVUf(void) mVU_EATAN() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU1<vuIndex>(_Fs_, _Fsf_, 54); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_ADDSS_M32_to_XMM(xmmPQ, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);

		mVU_EATAN_<vuIndex>();
	}
	pass3 { mVUlog("EATAN P"); }
}

microVUf(void) mVU_EATANxy() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 54); }
	pass2 { 
		getReg6(xmmFt, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmFs, xmmFt, 0x01);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_XMM_to_XMM(xmmFs, xmmFt); // y-x, not y-1? ><
		SSE_ADDSS_XMM_to_XMM(xmmFt, xmmPQ);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);

		mVU_EATAN_<vuIndex>();
	}
	pass3 { mVUlog("EATANxy P"); }
}

microVUf(void) mVU_EATANxz() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 54); }
	pass2 { 
		getReg6(xmmFt, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmFs, xmmFt, 0x02);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance

		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_SUBSS_XMM_to_XMM(xmmFs, xmmFt);
		SSE_ADDSS_XMM_to_XMM(xmmFt, xmmPQ);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmFt);

		mVU_EATAN_<vuIndex>();
	}
	pass3 { mVUlog("EATANxz P"); }
}

#define eexpHelper(addr) {						\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);			\
	SSE_MOVAPS_XMM_to_XMM(xmmFt, xmmT1);		\
	SSE_MULSS_M32_to_XMM(xmmFt, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFt);			\
}

microVUf(void) mVU_EEXP() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU1<vuIndex>(_Fs_, _Fsf_, 44); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_MULSS_M32_to_XMM(xmmPQ, (uptr)mVU_E1);
		SSE_ADDSS_M32_to_XMM(xmmPQ, (uptr)mVU_one);
		
		SSE_MOVAPS_XMM_to_XMM(xmmFt, xmmFs);
		SSE_MULSS_XMM_to_XMM(xmmFt, xmmFs);
		SSE_MOVAPS_XMM_to_XMM(xmmT1, xmmFt);
		SSE_MULSS_M32_to_XMM(xmmFt, (uptr)mVU_E2);
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFt);

		eexpHelper(mVU_E3);
		eexpHelper(mVU_E4);
		eexpHelper(mVU_E5);
		
		SSE_MULSS_XMM_to_XMM(xmmT1, xmmFs);
		SSE_MULSS_M32_to_XMM(xmmT1, (uptr)mVU_E6);
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmT1);
		SSE_MULSS_XMM_to_XMM(xmmPQ, xmmPQ);
		SSE_MULSS_XMM_to_XMM(xmmPQ, xmmPQ);
		SSE_MOVSS_M32_to_XMM(xmmT1, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmT1, xmmPQ);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmT1);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("EEXP P"); }
}

microVUt(void) mVU_sumXYZ() { 
	// regd.x =  x ^ 2 + y ^ 2 + z ^ 2
	if( cpucaps.hasStreamingSIMD4Extensions ) {
		SSE4_DPPS_XMM_to_XMM(xmmFs, xmmFs, 0x71);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
	}
	else {
		SSE_MULPS_XMM_to_XMM(xmmFs, xmmFs); // wzyx ^ 2
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmFs, xmmFs, 0xe1); // wzyx -> wzxy
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFs); // x ^ 2 + y ^ 2
		SSE2_PSHUFD_XMM_to_XMM(xmmFs, xmmFs, 0xD2); // wzxy -> wxyz
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFs); // x ^ 2 + y ^ 2 + z ^ 2
	}
}

microVUf(void) mVU_ELENG() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 18); }
	pass2 { 
		getReg6(xmmFs, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ<vuIndex>();
		SSE_SQRTSS_XMM_to_XMM(xmmPQ, xmmPQ);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ELENG P"); }
}

microVUf(void) mVU_ERCPR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU1<vuIndex>(_Fs_, _Fsf_, 12); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_MOVSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ERCPR P"); }
}

microVUf(void) mVU_ERLENG() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 24); }
	pass2 { 
		getReg6(xmmFs, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ<vuIndex>();
		SSE_SQRTSS_XMM_to_XMM(xmmPQ, xmmPQ);
		SSE_MOVSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ERLENG P"); }
}

microVUf(void) mVU_ERSADD() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 18); }
	pass2 { 
		getReg6(xmmFs, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ<vuIndex>();
		//SSE_RCPSS_XMM_to_XMM(xmmPQ, xmmPQ); // Lower Precision is bad?
		SSE_MOVSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ERSADD P"); }
}

microVUf(void) mVU_ERSQRT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU1<vuIndex>(_Fs_, _Fsf_, 18); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_SQRTSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE_MOVSS_M32_to_XMM(xmmFs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM(xmmFs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ERSQRT P"); }
}

microVUf(void) mVU_ESADD() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 11); }
	pass2 { 
		getReg6(xmmFs, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ<vuIndex>();
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ESADD P"); }
}

#define esinHelper(addr) {						\
	SSE_MULSS_XMM_to_XMM(xmmT1, xmmFt);			\
	SSE_MOVAPS_XMM_to_XMM(xmmFs, xmmT1);		\
	SSE_MULSS_M32_to_XMM(xmmFs, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFs);			\
}

microVUf(void) mVU_ESIN() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 29); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		//SSE_MULSS_M32_to_XMM(xmmPQ, (uptr)mVU_one); // Multiplying by 1 is redundant?
		SSE_MOVAPS_XMM_to_XMM(xmmFt, xmmFs);
		SSE_MULSS_XMM_to_XMM(xmmFs, xmmFt);
		SSE_MOVAPS_XMM_to_XMM(xmmT1, xmmFs);
		SSE_MULSS_XMM_to_XMM(xmmFs, xmmFt);
		SSE_MOVAPS_XMM_to_XMM(xmmFt, xmmFs);
		SSE_MULSS_M32_to_XMM(xmmFs, (uptr)mVU_S2);
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmFs);

		esinHelper(mVU_S3);
		esinHelper(mVU_S4);
		
		SSE_MULSS_XMM_to_XMM(xmmT1, xmmFt);
		SSE_MULSS_M32_to_XMM(xmmT1, (uptr)mVU_S5);
		SSE_ADDSS_XMM_to_XMM(xmmPQ, xmmT1);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ESIN P"); }
} 

microVUf(void) mVU_ESQRT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU1<vuIndex>(_Fs_, _Fsf_, 12); }
	pass2 { 
		getReg5(xmmFs, _Fs_, _Fsf_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_SQRTSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ESQRT P"); }
}

microVUf(void) mVU_ESUM() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeEFU2<vuIndex>(_Fs_, 12); }
	pass2 { 
		getReg6(xmmFs, _Fs_);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE2_PSHUFD_XMM_to_XMM(xmmFt, xmmFs, 0x1b);
		SSE_ADDPS_XMM_to_XMM(xmmFs, xmmFt);
		SSE2_PSHUFD_XMM_to_XMM(xmmFt, xmmFs, 0x01);
		SSE_ADDSS_XMM_to_XMM(xmmFs, xmmFt);
		SSE_MOVSS_XMM_to_XMM(xmmPQ, xmmFs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, writeP ? 0x27 : 0xC6); // Flip back
	}
	pass3 { mVUlog("ESUM P"); }
}

//------------------------------------------------------------------
// FCAND/FCEQ/FCGET/FCOR/FCSET
//------------------------------------------------------------------

microVUf(void) mVU_FCAND() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeCflag<vuIndex>(); }
	pass2 { 
		mVUallocCFLAGa<vuIndex>(gprT1, fvcInstance);
		AND32ItoR(gprT1, _Imm24_);
		ADD32ItoR(gprT1, 0xffffff);
		SHR32ItoR(gprT1, 24);
		mVUallocVIb<vuIndex>(gprT1, 1);
	}
	pass3 { mVUlog("FCAND vi01, $%x", _Imm24_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 8); }
}

microVUf(void) mVU_FCEQ() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeCflag<vuIndex>(); }
	pass2 { 
		mVUallocCFLAGa<vuIndex>(gprT1, fvcInstance);
		XOR32ItoR(gprT1, _Imm24_);
		SUB32ItoR(gprT1, 1);
		SHR32ItoR(gprT1, 31);
		mVUallocVIb<vuIndex>(gprT1, 1);
	}
	pass3 { mVUlog("FCEQ vi01, $%x", _Imm24_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 8); }
}

microVUf(void) mVU_FCGET() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeCflag<vuIndex>(); }
	pass2 { 
		mVUallocCFLAGa<vuIndex>(gprT1, fvcInstance);
		AND32ItoR(gprT1, 0xfff);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FCGET vi%02d", _Ft_);	   }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 8); }
}

microVUf(void) mVU_FCOR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeCflag<vuIndex>(); }
	pass2 { 
		mVUallocCFLAGa<vuIndex>(gprT1, fvcInstance);
		OR32ItoR(gprT1, _Imm24_);
		ADD32ItoR(gprT1, 1);  // If 24 1's will make 25th bit 1, else 0
		SHR32ItoR(gprT1, 24); // Get the 25th bit (also clears the rest of the garbage in the reg)
		mVUallocVIb<vuIndex>(gprT1, 1);
	}
	pass3 { mVUlog("FCOR vi01, $%x", _Imm24_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 8); }
}

microVUf(void) mVU_FCSET() {
	microVU* mVU = mVUx;
	pass1 { mVUinfo |= _doClip; }
	pass2 { 
		MOV32ItoR(gprT1, _Imm24_);
		mVUallocCFLAGb<vuIndex>(gprT1, fcInstance);
	}
	pass3 { mVUlog("FCSET $%x", _Imm24_); }
}

//------------------------------------------------------------------
// FMAND/FMEQ/FMOR
//------------------------------------------------------------------

microVUf(void) mVU_FMAND() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMflag<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Is_);
		AND16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FMAND vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 4);   }
}

microVUf(void) mVU_FMEQ() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMflag<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Is_);
		XOR32RtoR(gprT1, gprT2);
		SUB32ItoR(gprT1, 1);
		SHR32ItoR(gprT1, 31);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FMEQ vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 4);  }
}

microVUf(void) mVU_FMOR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMflag<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Is_);
		OR16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FMOR vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 4);  }
}

//------------------------------------------------------------------
// FSAND/FSEQ/FSOR/FSSET
//------------------------------------------------------------------

microVUf(void) mVU_FSAND() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSflag<vuIndex>(_It_); }
	pass2 { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		AND16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FSAND vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 0); mVUsFlagHack = 0; }
}

microVUf(void) mVU_FSEQ() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSflag<vuIndex>(_It_); }
	pass2 { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		XOR16ItoR(gprT1, _Imm12_);
		SUB16ItoR(gprT1, 1);
		SHR16ItoR(gprT1, 15);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FSEQ vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 0); mVUsFlagHack = 0; }
}

microVUf(void) mVU_FSOR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSflag<vuIndex>(_It_); }
	pass2 { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		OR16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("FSOR vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUflagInfo |= 0xf << (/*mVUcount +*/ 0); mVUsFlagHack = 0; }
}

microVUf(void) mVU_FSSET() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFSSET<vuIndex>(); }
	pass2 { 
		int flagReg1, flagReg2;
		getFlagReg(flagReg1, fsInstance);
		if (!(doStatus||doDivFlag)) { getFlagReg(flagReg2, fpsInstance); MOV32RtoR(flagReg1, flagReg2); } // Get status result from last status setting instruction	
		AND16ItoR(flagReg1, 0x03f); // Remember not to modify upper 16 bits because of mac flag 
		OR16ItoR (flagReg1, (_Imm12_ & 0xfc0));
	}
	pass3 { mVUlog("FSSET $%x", _Imm12_); }
	pass4 { mVUsFlagHack = 0; }
}

//------------------------------------------------------------------
// IADD/IADDI/IADDIU/IAND/IOR/ISUB/ISUBIU
//------------------------------------------------------------------

microVUf(void) mVU_IADD() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU1<vuIndex>(_Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa<vuIndex>(gprT2, _It_);
			ADD16RtoR(gprT1, gprT2);
		}
		else ADD16RtoR(gprT1, gprT1);
		mVUallocVIb<vuIndex>(gprT1, _Id_);
	}
	pass3 { mVUlog("IADD vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

microVUf(void) mVU_IADDI() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU2<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		ADD16ItoR(gprT1, _Imm5_);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("IADDI vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm5_); }
}

microVUf(void) mVU_IADDIU() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU2<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		ADD16ItoR(gprT1, _Imm15_);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("IADDIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

microVUf(void) mVU_IAND() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU1<vuIndex>(_Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa<vuIndex>(gprT2, _It_);
			AND32RtoR(gprT1, gprT2);
		}
		mVUallocVIb<vuIndex>(gprT1, _Id_);
	}
	pass3 { mVUlog("IAND vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

microVUf(void) mVU_IOR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU1<vuIndex>(_Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa<vuIndex>(gprT2, _It_);
			OR32RtoR(gprT1, gprT2);
		}
		mVUallocVIb<vuIndex>(gprT1, _Id_);
	}
	pass3 { mVUlog("IOR vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

microVUf(void) mVU_ISUB() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU1<vuIndex>(_Id_, _Is_, _It_); }
	pass2 { 
		if (_It_ != _Is_) {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			mVUallocVIa<vuIndex>(gprT2, _It_);
			SUB16RtoR(gprT1, gprT2);
			mVUallocVIb<vuIndex>(gprT1, _Id_);
		}
		else if (!isMMX(_Id_)) { 
			XOR32RtoR(gprT1, gprT1);
			mVUallocVIb<vuIndex>(gprT1, _Id_);
		}
		else { PXORRtoR(mmVI(_Id_), mmVI(_Id_)); }
	}
	pass3 { mVUlog("ISUB vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

microVUf(void) mVU_ISUBIU() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeIALU2<vuIndex>(_Is_, _It_); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		SUB16ItoR(gprT1, _Imm15_);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("ISUBIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

//------------------------------------------------------------------
// MFIR/MFP/MOVE/MR32/MTIR
//------------------------------------------------------------------

microVUf(void) mVU_MFIR() {
	microVU* mVU = mVUx;
	pass1 { if (!_Ft_) { mVUinfo |= _isNOP; } analyzeVIreg1(_Is_); analyzeReg2(_Ft_, 1); }
	pass2 { 
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		MOVSX32R16toR(gprT1, gprT1);
		SSE2_MOVD_R_to_XMM(xmmT1, gprT1);
		if (!_XYZW_SS) { mVUunpack_xyzw<vuIndex>(xmmT1, xmmT1, 0); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
	}
	pass3 { mVUlog("MFIR.%s vf%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

microVUf(void) mVU_MFP() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMFP<vuIndex>(_Ft_); }
	pass2 { 
		getPreg(xmmFt);
		mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
	}
	pass3 { mVUlog("MFP.%s vf%02d, P", _XYZW_String, _Ft_); }
}

microVUf(void) mVU_MOVE() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMOVE<vuIndex>(_Fs_, _Ft_); }
	pass2 { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], _X_Y_Z_W);
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
	}
	pass3 { mVUlog("MOVE.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

microVUf(void) mVU_MR32() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeMR32<vuIndex>(_Fs_, _Ft_); }
	pass2 { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], (_X_Y_Z_W == 8) ? 4 : 15);
		if (_X_Y_Z_W != 8) { SSE2_PSHUFD_XMM_to_XMM(xmmT1, xmmT1, 0x39); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 0);
	}
	pass3 { mVUlog("MR32.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

microVUf(void) mVU_MTIR() {
	microVU* mVU = mVUx;
	pass1 { if (!_It_) { mVUinfo |= _isNOP; } analyzeReg5(_Fs_, _Fsf_); analyzeVIreg2(_It_, 1); }
	pass2 { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->VF[_Fs_].UL[_Fsf_]);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("MTIR vi%02d, vf%02d%s", _Ft_, _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// ILW/ILWR
//------------------------------------------------------------------

microVUf(void) mVU_ILW() {
	microVU* mVU = mVUx;
	pass1 { if (!_It_) { mVUinfo |= _isNOP; } analyzeVIreg1(_Is_); analyzeVIreg2(_It_, 4);  }
	pass2 { 
		if (!_Is_) {
			MOVZX32M16toR(gprT1, (uptr)mVU->regs->Mem + getVUmem(_Imm11_) + offsetSS);
			mVUallocVIb<vuIndex>(gprT1, _It_);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix<vuIndex>(gprT1);
			MOVZX32Rm16toR(gprT1, gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb<vuIndex>(gprT1, _It_);
		}
	}
	pass3 { mVUlog("ILW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

microVUf(void) mVU_ILWR() {
	microVU* mVU = mVUx;
	pass1 { if (!_It_) { mVUinfo |= _isNOP; } analyzeVIreg1(_Is_); analyzeVIreg2(_It_, 4); }
	pass2 { 
		if (!_Is_) {
			MOVZX32M16toR(gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb<vuIndex>(gprT1, _It_);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			mVUaddrFix<vuIndex>(gprT1);
			MOVZX32Rm16toR(gprT1, gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb<vuIndex>(gprT1, _It_);
		}
	}
	pass3 { mVUlog("ILWR.%s vi%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// ISW/ISWR
//------------------------------------------------------------------

microVUf(void) mVU_ISW() {
	microVU* mVU = mVUx;
	pass1 { analyzeVIreg1(_Is_); analyzeVIreg1(_It_); }
	pass2 { 
		if (!_Is_) {
			int imm = getVUmem(_Imm11_);
			mVUallocVIa<vuIndex>(gprT1, _It_);
			if (_X) MOV32RtoM((uptr)mVU->regs->Mem + imm,		gprT1);
			if (_Y) MOV32RtoM((uptr)mVU->regs->Mem + imm + 4,	gprT1);
			if (_Z) MOV32RtoM((uptr)mVU->regs->Mem + imm + 8,	gprT1);
			if (_W) MOV32RtoM((uptr)mVU->regs->Mem + imm + 12,	gprT1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			mVUallocVIa<vuIndex>(gprT2, _It_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix<vuIndex>(gprT1);
			if (_X) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem);
			if (_Y) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+4);
			if (_Z) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+8);
			if (_W) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+12);
		}
	}
	pass3 { mVUlog("ISW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_);  }
}

microVUf(void) mVU_ISWR() {
	microVU* mVU = mVUx;
	pass1 { analyzeVIreg1(_Is_); analyzeVIreg1(_It_); }
	pass2 { 
		if (!_Is_) {
			mVUallocVIa<vuIndex>(gprT1, _It_);
			if (_X) MOV32RtoM((uptr)mVU->regs->Mem,	   gprT1);
			if (_Y) MOV32RtoM((uptr)mVU->regs->Mem+4,  gprT1);
			if (_Z) MOV32RtoM((uptr)mVU->regs->Mem+8,  gprT1);
			if (_W) MOV32RtoM((uptr)mVU->regs->Mem+12, gprT1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			mVUallocVIa<vuIndex>(gprT2, _It_);
			mVUaddrFix<vuIndex>(gprT1);
			if (_X) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem);
			if (_Y) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+4);
			if (_Z) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+8);
			if (_W) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+12);
		}
	}
	pass3 { mVUlog("ISWR.%s vi%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// LQ/LQD/LQI
//------------------------------------------------------------------

microVUf(void) mVU_LQ() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeLQ<vuIndex>(_Ft_, _Is_, 0); }
	pass2 { 
		if (!_Is_) {
			mVUloadReg<vuIndex>(xmmFt, (uptr)mVU->regs->Mem + getVUmem(_Imm11_), _X_Y_Z_W);
			mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix<vuIndex>(gprT1);
			mVUloadReg2<vuIndex>(xmmFt, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
		}
	}
	pass3 { mVUlog("LQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

microVUf(void) mVU_LQD() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeLQ<vuIndex>(_Ft_, _Is_, 1); }
	pass2 { 
		if (!_Is_ && !noWriteVF) {
			mVUloadReg<vuIndex>(xmmFt, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _Is_);
			SUB16ItoR(gprT1, 1);
			mVUallocVIb<vuIndex>(gprT1, _Is_);
			if (!noWriteVF) {
				mVUaddrFix<vuIndex>(gprT1);
				mVUloadReg2<vuIndex>(xmmFt, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
				mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
			}
		}
	}
	pass3 { mVUlog("LQD.%s vf%02d, --vi%02d", _XYZW_String, _Ft_, _Is_); }
}

microVUf(void) mVU_LQI() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeLQ<vuIndex>(_Ft_, _Is_, 1); }
	pass2 { 
		if (!_Is_ && !noWriteVF) {
			mVUloadReg<vuIndex>(xmmFt, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>((!noWriteVF) ? gprT1 : gprT2, _Is_);
			if (!noWriteVF) {
				MOV32RtoR(gprT2, gprT1);
				mVUaddrFix<vuIndex>(gprT1);
				mVUloadReg2<vuIndex>(xmmFt, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
				mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
			}
			ADD16ItoR(gprT2, 1);
			mVUallocVIb<vuIndex>(gprT2, _Is_);
		}
	}
	pass3 { mVUlog("LQI.%s vf%02d, vi%02d++", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// SQ/SQD/SQI
//------------------------------------------------------------------

microVUf(void) mVU_SQ() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSQ<vuIndex>(_Fs_, _It_, 0); }
	pass2 { 
		if (!_It_) {
			getReg7(xmmFs, _Fs_);
			mVUsaveReg<vuIndex>(xmmFs, (uptr)mVU->regs->Mem + getVUmem(_Imm11_), _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _It_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix<vuIndex>(gprT1);
			getReg7(xmmFs, _Fs_);
			mVUsaveReg2<vuIndex>(xmmFs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
		}
	}
	pass3 { mVUlog("SQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Fs_, _Ft_, _Imm11_); }
}

microVUf(void) mVU_SQD() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSQ<vuIndex>(_Fs_, _It_, 1); }
	pass2 { 
		if (!_It_) {
			getReg7(xmmFs, _Fs_);
			mVUsaveReg<vuIndex>(xmmFs, (uptr)mVU->regs->Mem, _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _It_);
			SUB16ItoR(gprT1, 1);
			mVUallocVIb<vuIndex>(gprT1, _It_);
			mVUaddrFix<vuIndex>(gprT1);
			getReg7(xmmFs, _Fs_);
			mVUsaveReg2<vuIndex>(xmmFs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
		}
	}
	pass3 { mVUlog("SQD.%s vf%02d, --vi%02d", _XYZW_String, _Fs_, _Ft_); }
}

microVUf(void) mVU_SQI() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeSQ<vuIndex>(_Fs_, _It_, 1); }
	pass2 { 
		if (!_It_) {
			getReg7(xmmFs, _Fs_);
			mVUsaveReg<vuIndex>(xmmFs, (uptr)mVU->regs->Mem, _X_Y_Z_W, 1);
		}
		else {
			mVUallocVIa<vuIndex>(gprT1, _It_);
			MOV32RtoR(gprT2, gprT1);
			mVUaddrFix<vuIndex>(gprT1);
			getReg7(xmmFs, _Fs_);
			mVUsaveReg2<vuIndex>(xmmFs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			ADD16ItoR(gprT2, 1);
			mVUallocVIb<vuIndex>(gprT2, _It_);
		}
	}
	pass3 { mVUlog("SQI.%s vf%02d, vi%02d++", _XYZW_String, _Fs_, _Ft_); }
}

//------------------------------------------------------------------
// RINIT/RGET/RNEXT/RXOR
//------------------------------------------------------------------

microVUf(void) mVU_RINIT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeR1<vuIndex>(_Fs_, _Fsf_); }
	pass2 { 
		if (_Fs_ || (_Fsf_ == 3)) {
			getReg8(gprR, _Fs_, _Fsf_);
			AND32ItoR(gprR, 0x007fffff);
			OR32ItoR (gprR, 0x3f800000);
		}
		else MOV32ItoR(gprR, 0x3f800000);
	}
	pass3 { mVUlog("RINIT R, vf%02d%s", _Fs_, _Fsf_String); }
}

microVUt(void) mVU_RGET_() {
	microVU* mVU = mVUx;
	if (!noWriteVF) {
		if (_X) MOV32RtoM((uptr)&mVU->regs->VF[_Ft_].UL[0], gprR);
		if (_Y) MOV32RtoM((uptr)&mVU->regs->VF[_Ft_].UL[1], gprR);
		if (_Z) MOV32RtoM((uptr)&mVU->regs->VF[_Ft_].UL[2], gprR);
		if (_W) MOV32RtoM((uptr)&mVU->regs->VF[_Ft_].UL[3], gprR);
	}
}

microVUf(void) mVU_RGET() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeR2<vuIndex>(_Ft_, 1); }
	pass2 { mVU_RGET_<vuIndex>(); }
	pass3 { mVUlog("RGET.%s vf%02d, R", _XYZW_String, _Ft_); }
}

microVUf(void) mVU_RNEXT() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeR2<vuIndex>(_Ft_, 0); }
	pass2 { 
		// algorithm from www.project-fao.org
		MOV32RtoR(gprT1, gprR);
		SHR32ItoR(gprT1, 4);
		AND32ItoR(gprT1, 1);

		MOV32RtoR(gprT2, gprR);
		SHR32ItoR(gprT2, 22);
		AND32ItoR(gprT2, 1);

		SHL32ItoR(gprR, 1);
		XOR32RtoR(gprT1, gprT2);
		XOR32RtoR(gprR,  gprT1);
		AND32ItoR(gprR, 0x007fffff);
		OR32ItoR (gprR, 0x3f800000);
		mVU_RGET_<vuIndex>(); 
	}
	pass3 { mVUlog("RNEXT.%s vf%02d, R", _XYZW_String, _Ft_); }
}

microVUf(void) mVU_RXOR() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeR1<vuIndex>(_Fs_, _Fsf_); }
	pass2 { 
		if (_Fs_ || (_Fsf_ == 3)) {
			getReg8(gprT1, _Fs_, _Fsf_);
			AND32ItoR(gprT1, 0x7fffff);
			XOR32RtoR(gprR,  gprT1);
		}
	}
	pass3 { mVUlog("RXOR R, vf%02d%s", _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// WaitP/WaitQ
//------------------------------------------------------------------

microVUf(void) mVU_WAITP() {
	microVU* mVU = mVUx;
	pass1 { mVUstall = aMax(mVUstall, ((mVUregs.p) ? (mVUregs.p - 1) : 0)); }
	pass3 { mVUlog("WAITP"); }
}

microVUf(void) mVU_WAITQ() {
	microVU* mVU = mVUx;
	pass1 { mVUstall = aMax(mVUstall, mVUregs.q); }
	pass3 { mVUlog("WAITQ"); }
}

//------------------------------------------------------------------
// XTOP/XITOP
//------------------------------------------------------------------

microVUf(void) mVU_XTOP() {
	microVU* mVU = mVUx;
	pass1 { if (!_It_) { mVUinfo |= _isNOP; } analyzeVIreg2(_It_, 1); }
	pass2 { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->vifRegs->top);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("XTOP vi%02d", _Ft_); }
}

microVUf(void) mVU_XITOP() {
	microVU* mVU = mVUx;
	pass1 { if (!_It_) { mVUinfo |= _isNOP; } analyzeVIreg2(_It_, 1); }
	pass2 { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->vifRegs->itop);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("XITOP vi%02d", _Ft_); }
}

//------------------------------------------------------------------
// XGkick
//------------------------------------------------------------------

void __fastcall mVU_XGKICK_(u32 addr) {
	addr = (addr<<4) & 0x3fff; // Multiply addr by 16 to get real address
	u32 *data = (u32*)(microVU1.regs->Mem + addr);
	u32  size = mtgsThread->PrepDataPacket(GIF_PATH_1, data, (0x4000-addr) >> 4);
	u8 *pDest = mtgsThread->GetDataPacketPtr();
	memcpy_aligned(pDest, microVU1.regs->Mem + addr, size<<4);
	mtgsThread->SendDataPacket();
}

void __fastcall mVU_XGKICK__(u32 addr) {
	GSGIFTRANSFER1((u32*)microVU1.regs->Mem, ((addr<<4)&0x3fff));
}

microVUf(void) mVU_XGKICK() {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeXGkick<vuIndex>(_Is_, 4); }
	pass2 {
		mVUprint("XGkick");
		mVUallocVIa<vuIndex>(gprT2, _Is_); // gprT2 = ECX for __fastcall
		mVUbackupRegs<vuIndex>();
		if (mtgsThread != NULL)	CALLFunc((uptr)mVU_XGKICK_);
		else					CALLFunc((uptr)mVU_XGKICK__);
		mVUrestoreRegs<vuIndex>();
	}
	pass3 { mVUlog("XGKICK vi%02d", _Fs_); }
}

//------------------------------------------------------------------
// Branches/Jumps
//------------------------------------------------------------------

#define setBranchA(x, _x_) {															\
	pass1 { if (_Imm11_ == 1 && !_x_) { mVUinfo |= _isNOP; return; } mVUbranch = x; }	\
	pass2 { mVUbranch = x; }															\
}

microVUf(void) mVU_B() {
	microVU* mVU = mVUx;
	setBranchA(1, 0);
	pass3 { mVUlog("B [<a href=\"#addr%04x\">%04x</a>]", branchAddr, branchAddr); }
}

microVUf(void) mVU_BAL() {
	microVU* mVU = mVUx;
	setBranchA(2, _It_);
	pass1 { analyzeVIreg2(_It_, 1); }
	pass2 {
		MOV32ItoR(gprT1, bSaveAddr);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("BAL vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBEQ() {
	microVU* mVU = mVUx;
	setBranchA(3, 0);
	pass1 { mVUanalyzeBranch2<vuIndex>(_Is_, _It_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		if (memReadIt) XOR32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else { mVUallocVIa<vuIndex>(gprT2, _It_); XOR32RtoR(gprT1, gprT2); }
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBEQ vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBGEZ() {
	microVU* mVU = mVUx;
	setBranchA(4, 0);
	pass1 { mVUanalyzeBranch1<vuIndex>(_Is_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBGEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBGTZ() {
	microVU* mVU = mVUx;
	setBranchA(5, 0);
	pass1 { mVUanalyzeBranch1<vuIndex>(_Is_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBGTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBLEZ() {
	microVU* mVU = mVUx;
	setBranchA(6, 0);
	pass1 { mVUanalyzeBranch1<vuIndex>(_Is_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBLEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBLTZ() {
	microVU* mVU = mVUx;
	setBranchA(7, 0);
	pass1 { mVUanalyzeBranch1<vuIndex>(_Is_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBLTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_IBNE() {
	microVU* mVU = mVUx;
	setBranchA(8, 0);
	pass1 { mVUanalyzeBranch2<vuIndex>(_Is_, _It_); }
	pass2 {
		if (memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else mVUallocVIa<vuIndex>(gprT1, _Is_);
		if (memReadIt) XOR32MtoR(gprT1, (uptr)&mVU->VIbackup[0]);
		else { mVUallocVIa<vuIndex>(gprT2, _It_); XOR32RtoR(gprT1, gprT2); }
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("IBNE vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

microVUf(void) mVU_JR() {
	microVU* mVU = mVUx;
	mVUbranch = 9;
	pass1 { analyzeVIreg1(_Is_); }
	pass2 {
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		SHL32ItoR(gprT1, 3);
		AND32ItoR(gprT1, vuIndex ? 0x3ff8 : 0xff8);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
	}
	pass3 { mVUlog("JR [vi%02d]", _Fs_); }
}

microVUf(void) mVU_JALR() {
	microVU* mVU = mVUx;
	mVUbranch = 10;
	pass1 { analyzeVIreg1(_Is_); analyzeVIreg2(_It_, 1); }
	pass2 {
		mVUallocVIa<vuIndex>(gprT1, _Is_);
		SHL32ItoR(gprT1, 3);
		AND32ItoR(gprT1, vuIndex ? 0x3ff8 : 0xff8);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
		MOV32ItoR(gprT1, bSaveAddr);
		mVUallocVIb<vuIndex>(gprT1, _It_);
	}
	pass3 { mVUlog("JALR vi%02d, [vi%02d]", _Ft_, _Fs_); }
}

#endif //PCSX2_MICROVU
