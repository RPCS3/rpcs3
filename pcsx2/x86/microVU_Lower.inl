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

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

//------------------------------------------------------------------
// DIV/SQRT/RSQRT
//------------------------------------------------------------------

// Test if Vector is +/- Zero
#define testZero(xmmReg, xmmTemp, gprTemp) {				\
	SSE_XORPS_XMM_to_XMM(xmmTemp, xmmTemp);					\
	SSE_CMPEQSS_XMM_to_XMM(xmmTemp, xmmReg);				\
	if (!x86caps.hasStreamingSIMD4Extensions) {				\
		SSE_MOVMSKPS_XMM_to_R32(gprTemp, xmmTemp);			\
		TEST32ItoR(gprTemp, 1);								\
	}														\
	else SSE4_PTEST_XMM_to_XMM(xmmTemp, xmmTemp);			\
}

// Test if Vector is Negative (Set Flags and Makes Positive)
#define testNeg(xmmReg, gprTemp, aJump) {					\
	SSE_MOVMSKPS_XMM_to_R32(gprTemp, xmmReg);				\
	TEST32ItoR(gprTemp, 1);									\
	aJump = JZ8(0);											\
		MOV32ItoM((uptr)&mVU->divFlag, divI);				\
		SSE_ANDPS_M128_to_XMM(xmmReg, (uptr)mVU_absclip);	\
	x86SetJ8(aJump);										\
}

mVUop(mVU_DIV) {
	pass1 { mVUanalyzeFDIV(mVU, _Fs_, _Fsf_, _Ft_, _Ftf_, 7); }
	pass2 { 
		u8 *ajmp, *bjmp, *cjmp, *djmp;
		int Ft;
		if (_Ftf_) Ft = mVU->regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));
		else	   Ft = mVU->regAlloc->allocReg(_Ft_);
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		int t1 = mVU->regAlloc->allocReg();

		testZero(Ft, t1, gprT1); // Test if Ft is zero
		cjmp = JZ8(0); // Skip if not zero

			testZero(Fs, t1, gprT1); // Test if Fs is zero
			ajmp = JZ8(0);
				MOV32ItoM((uptr)&mVU->divFlag, divI); // Set invalid flag (0/0)
				bjmp = JMP8(0);
			x86SetJ8(ajmp);
				MOV32ItoM((uptr)&mVU->divFlag, divD); // Zero divide (only when not 0/0)
			x86SetJ8(bjmp);

			SSE_XORPS_XMM_to_XMM (Fs, Ft);
			SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_signbit);
			SSE_ORPS_M128_to_XMM (Fs, (uptr)mVU_maxvals); // If division by zero, then xmmFs = +/- fmax

			djmp = JMP8(0);
		x86SetJ8(cjmp);
			MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
			SSE_DIVSS_XMM_to_XMM(Fs, Ft);
			mVUclamp1(Fs, t1, 8);
		x86SetJ8(djmp);

		writeQreg(Fs, mVUinfo.writeQ);

		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(t1);
	}
	pass3 { mVUlog("DIV Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

mVUop(mVU_SQRT) {
	pass1 { mVUanalyzeFDIV(mVU, 0, 0, _Ft_, _Ftf_, 7); }
	pass2 { 
		u8 *ajmp;
		int Ft = mVU->regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));

		MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
		testNeg(Ft, gprT1, ajmp); // Check for negative sqrt

		if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(Ft, (uptr)mVU_maxvals); // Clamp infinities (only need to do positive clamp since xmmFt is positive)
		SSE_SQRTSS_XMM_to_XMM(Ft, Ft);
		writeQreg(Ft, mVUinfo.writeQ);

		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVUlog("SQRT Q, vf%02d%s", _Ft_, _Ftf_String); }
}

mVUop(mVU_RSQRT) {
	pass1 { mVUanalyzeFDIV(mVU, _Fs_, _Fsf_, _Ft_, _Ftf_, 13); }
	pass2 { 
		u8 *ajmp, *bjmp, *cjmp, *djmp;
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		int Ft = mVU->regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));
		int t1 = mVU->regAlloc->allocReg();

		MOV32ItoM((uptr)&mVU->divFlag, 0); // Clear I/D flags
		testNeg(Ft, gprT1, ajmp); // Check for negative sqrt

		SSE_SQRTSS_XMM_to_XMM(Ft, Ft);
		testZero(Ft, t1, gprT1); // Test if Ft is zero
		ajmp = JZ8(0); // Skip if not zero

			testZero(Fs, t1, gprT1); // Test if Fs is zero
			bjmp = JZ8(0); // Skip if none are
				MOV32ItoM((uptr)&mVU->divFlag, divI); // Set invalid flag (0/0)
				cjmp = JMP8(0);
			x86SetJ8(bjmp);
				MOV32ItoM((uptr)&mVU->divFlag, divD); // Zero divide flag (only when not 0/0)
			x86SetJ8(cjmp);

			SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_signbit);
			SSE_ORPS_M128_to_XMM (Fs, (uptr)mVU_maxvals); // xmmFs = +/-Max

			djmp = JMP8(0);
		x86SetJ8(ajmp);
			SSE_DIVSS_XMM_to_XMM(Fs, Ft);
			mVUclamp1(Fs, t1, 8);
		x86SetJ8(djmp);

		writeQreg(Fs, mVUinfo.writeQ);

		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(t1);
	}
	pass3 { mVUlog("RSQRT Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

//------------------------------------------------------------------
// EATAN/EEXP/ELENG/ERCPR/ERLENG/ERSADD/ERSQRT/ESADD/ESIN/ESQRT/ESUM
//------------------------------------------------------------------

#define EATANhelper(addr) {					\
	SSE_MULSS_XMM_to_XMM (t2, Fs);			\
	SSE_MULSS_XMM_to_XMM (t2, Fs);			\
	SSE_MOVAPS_XMM_to_XMM(t1, t2);			\
	SSE_MULSS_M32_to_XMM (t1, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM (PQ, t1);			\
}

// ToDo: Can Be Optimized Further? (takes approximately (~115 cycles + mem access time) on a c2d)
microVUt(void) mVU_EATAN_(mV, int PQ, int Fs, int t1, int t2) {
	SSE_MOVSS_XMM_to_XMM (PQ, Fs);
	SSE_MULSS_M32_to_XMM (PQ, (uptr)mVU_T1);
	SSE_MOVAPS_XMM_to_XMM(t2, Fs);
	EATANhelper(mVU_T2);
	EATANhelper(mVU_T3);
	EATANhelper(mVU_T4);
	EATANhelper(mVU_T5);
	EATANhelper(mVU_T6);
	EATANhelper(mVU_T7);
	EATANhelper(mVU_T8);
	SSE_ADDSS_M32_to_XMM  (PQ, (uptr)mVU_Pi4);
	SSE2_PSHUFD_XMM_to_XMM(PQ, PQ, mVUinfo.writeP ? 0x27 : 0xC6);
}

mVUop(mVU_EATAN) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 54); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		int t1 = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_SUBSS_M32_to_XMM  (Fs,    (uptr)mVU_one);
		SSE_ADDSS_M32_to_XMM  (xmmPQ, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (Fs, xmmPQ);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog("EATAN P"); }
}

mVUop(mVU_EATANxy) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 54); }
	pass2 { 
		int t1 = mVU->regAlloc->allocReg(_Fs_, 0, 0xf);
		int Fs = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(Fs, t1, 0x01);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_SUBSS_XMM_to_XMM  (Fs, t1); // y-x, not y-1? ><
		SSE_ADDSS_XMM_to_XMM  (t1, xmmPQ);
		SSE_DIVSS_XMM_to_XMM  (Fs, t1);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog("EATANxy P"); }
}

mVUop(mVU_EATANxz) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 54); }
	pass2 { 
		int t1 = mVU->regAlloc->allocReg(_Fs_, 0, 0xf);
		int Fs = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(Fs, t1, 0x02);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_SUBSS_XMM_to_XMM  (Fs, t1);
		SSE_ADDSS_XMM_to_XMM  (t1, xmmPQ);
		SSE_DIVSS_XMM_to_XMM  (Fs, t1);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog("EATANxz P"); }
}

#define eexpHelper(addr) {					\
	SSE_MULSS_XMM_to_XMM (t2, Fs);			\
	SSE_MOVAPS_XMM_to_XMM(t1, t2);			\
	SSE_MULSS_M32_to_XMM (t1, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM (xmmPQ, t1);		\
}

mVUop(mVU_EEXP) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 44); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		int t1 = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_MULSS_M32_to_XMM  (xmmPQ, (uptr)mVU_E1);
		SSE_ADDSS_M32_to_XMM  (xmmPQ, (uptr)mVU_one);
		SSE_MOVAPS_XMM_to_XMM (t1, Fs);
		SSE_MULSS_XMM_to_XMM  (t1, Fs);
		SSE_MOVAPS_XMM_to_XMM (t2, t1);
		SSE_MULSS_M32_to_XMM  (t1, (uptr)mVU_E2);
		SSE_ADDSS_XMM_to_XMM  (xmmPQ, t1);
		eexpHelper(mVU_E3);
		eexpHelper(mVU_E4);
		eexpHelper(mVU_E5);
		SSE_MULSS_XMM_to_XMM  (t2, Fs);
		SSE_MULSS_M32_to_XMM  (t2, (uptr)mVU_E6);
		SSE_ADDSS_XMM_to_XMM  (xmmPQ, t2);
		SSE_MULSS_XMM_to_XMM  (xmmPQ, xmmPQ);
		SSE_MULSS_XMM_to_XMM  (xmmPQ, xmmPQ);
		SSE_MOVSS_M32_to_XMM  (t2, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (t2, xmmPQ);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, t2);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog("EEXP P"); }
}

// sumXYZ(): PQ.x = x ^ 2 + y ^ 2 + z ^ 2
microVUt(void) mVU_sumXYZ(int PQ, int Fs) {
	if( x86caps.hasStreamingSIMD4Extensions ) {
		SSE4_DPPS_XMM_to_XMM(Fs, Fs, 0x71);
		SSE_MOVSS_XMM_to_XMM(PQ, Fs);
	}
	else {
		SSE_MULPS_XMM_to_XMM  (Fs, Fs);		  // wzyx ^ 2
		SSE_MOVSS_XMM_to_XMM  (PQ, Fs);		  // x ^ 2
		SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xe1); // wzyx -> wzxy
		SSE_ADDSS_XMM_to_XMM  (PQ, Fs);		  // x ^ 2 + y ^ 2
		SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xD2); // wzxy -> wxyz
		SSE_ADDSS_XMM_to_XMM  (PQ, Fs);		  // x ^ 2 + y ^ 2 + z ^ 2
	}
}

mVUop(mVU_ELENG) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 18); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(xmmPQ, Fs);
		SSE_SQRTSS_XMM_to_XMM (xmmPQ, xmmPQ);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ELENG P"); }
}

mVUop(mVU_ERCPR) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 12); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_MOVSS_M32_to_XMM  (Fs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (Fs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ERCPR P"); }
}

mVUop(mVU_ERLENG) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 24); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(xmmPQ, Fs);
		SSE_SQRTSS_XMM_to_XMM (xmmPQ, xmmPQ);
		SSE_MOVSS_M32_to_XMM  (Fs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (Fs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ERLENG P"); }
}

mVUop(mVU_ERSADD) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 18); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(xmmPQ, Fs);
		SSE_MOVSS_M32_to_XMM  (Fs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (Fs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ERSADD P"); }
}

mVUop(mVU_ERSQRT) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 18); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_ANDPS_M128_to_XMM (Fs, (uptr)mVU_absclip);
		SSE_SQRTSS_XMM_to_XMM (xmmPQ, Fs);
		SSE_MOVSS_M32_to_XMM  (Fs, (uptr)mVU_one);
		SSE_DIVSS_XMM_to_XMM  (Fs, xmmPQ);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ERSQRT P"); }
}

mVUop(mVU_ESADD) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 11); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ESADD P"); }
}

#define esinHelper(addr) {					\
	SSE_MULSS_XMM_to_XMM (t2, t1);			\
	SSE_MOVAPS_XMM_to_XMM(Fs, t2);			\
	SSE_MULSS_M32_to_XMM (Fs, (uptr)addr);	\
	SSE_ADDSS_XMM_to_XMM (xmmPQ, Fs);		\
}

mVUop(mVU_ESIN) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 29); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		int t1 = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE_MOVAPS_XMM_to_XMM (t1, Fs);
		SSE_MULSS_XMM_to_XMM  (Fs, t1);
		SSE_MOVAPS_XMM_to_XMM (t2, Fs);
		SSE_MULSS_XMM_to_XMM  (Fs, t1);
		SSE_MOVAPS_XMM_to_XMM (t1, Fs);
		SSE_MULSS_M32_to_XMM  (Fs, (uptr)mVU_S2);
		SSE_ADDSS_XMM_to_XMM  (xmmPQ, Fs);
		esinHelper(mVU_S3);
		esinHelper(mVU_S4);
		SSE_MULSS_XMM_to_XMM  (t2, t1);
		SSE_MULSS_M32_to_XMM  (t2, (uptr)mVU_S5);
		SSE_ADDSS_XMM_to_XMM  (xmmPQ, t2);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog("ESIN P"); }
} 

mVUop(mVU_ESQRT) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 12); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE_ANDPS_M128_to_XMM (Fs, (uptr)mVU_absclip);
		SSE_SQRTSS_XMM_to_XMM (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ESQRT P"); }
}

mVUop(mVU_ESUM) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 12); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		int t1 = mVU->regAlloc->allocReg();
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		SSE2_PSHUFD_XMM_to_XMM(t1, Fs, 0x1b);
		SSE_ADDPS_XMM_to_XMM  (Fs, t1);
		SSE2_PSHUFD_XMM_to_XMM(t1, Fs, 0x01);
		SSE_ADDSS_XMM_to_XMM  (Fs, t1);
		SSE_MOVSS_XMM_to_XMM  (xmmPQ, Fs);
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
	}
	pass3 { mVUlog("ESUM P"); }
}

//------------------------------------------------------------------
// FCAND/FCEQ/FCGET/FCOR/FCSET
//------------------------------------------------------------------

mVUop(mVU_FCAND) {
	pass1 { mVUanalyzeCflag(mVU, 1); }
	pass2 { 
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		AND32ItoR(gprT1, _Imm24_);
		ADD32ItoR(gprT1, 0xffffff);
		SHR32ItoR(gprT1, 24);
		mVUallocVIb(mVU, gprT1, 1);
	}
	pass3 { mVUlog("FCAND vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCEQ) {
	pass1 { mVUanalyzeCflag(mVU, 1); }
	pass2 { 
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		XOR32ItoR(gprT1, _Imm24_);
		SUB32ItoR(gprT1, 1);
		SHR32ItoR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, 1);
	}
	pass3 { mVUlog("FCEQ vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCGET) {
	pass1 { mVUanalyzeCflag(mVU, _It_); }
	pass2 { 
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		AND32ItoR(gprT1, 0xfff);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FCGET vi%02d", _Ft_);	   }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCOR) {
	pass1 { mVUanalyzeCflag(mVU, 1); }
	pass2 { 
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		OR32ItoR(gprT1, _Imm24_);
		ADD32ItoR(gprT1, 1);  // If 24 1's will make 25th bit 1, else 0
		SHR32ItoR(gprT1, 24); // Get the 25th bit (also clears the rest of the garbage in the reg)
		mVUallocVIb(mVU, gprT1, 1);
	}
	pass3 { mVUlog("FCOR vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCSET) {
	pass1 { cFLAG.doFlag = 1; }
	pass2 { 
		MOV32ItoR(gprT1, _Imm24_);
		mVUallocCFLAGb(mVU, gprT1, cFLAG.write);
	}
	pass3 { mVUlog("FCSET $%x", _Imm24_); }
}

//------------------------------------------------------------------
// FMAND/FMEQ/FMOR
//------------------------------------------------------------------

mVUop(mVU_FMAND) {
	pass1 { mVUanalyzeMflag(mVU, _Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa(mVU, gprT1, mFLAG.read);
		mVUallocVIa(mVU, gprT2, _Is_);
		AND16RtoR(gprT1, gprT2);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FMAND vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2;   }
}

mVUop(mVU_FMEQ) {
	pass1 { mVUanalyzeMflag(mVU, _Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa(mVU, gprT1, mFLAG.read);
		mVUallocVIa(mVU, gprT2, _Is_);
		XOR32RtoR(gprT1, gprT2);
		SUB32ItoR(gprT1, 1);
		SHR32ItoR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FMEQ vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2;  }
}

mVUop(mVU_FMOR) {
	pass1 { mVUanalyzeMflag(mVU, _Is_, _It_); }
	pass2 { 
		mVUallocMFLAGa(mVU, gprT1, mFLAG.read);
		mVUallocVIa(mVU, gprT2, _Is_);
		OR16RtoR(gprT1, gprT2);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FMOR vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2;  }
}

//------------------------------------------------------------------
// FSAND/FSEQ/FSOR/FSSET
//------------------------------------------------------------------

mVUop(mVU_FSAND) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 { 
		mVUallocSFLAGc(gprT1, gprT2, sFLAG.read);
		AND32ItoR(gprT1, _Imm12_);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FSAND vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSOR) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 { 
		mVUallocSFLAGc(gprT1, gprT2, sFLAG.read);
		OR32ItoR(gprT1, _Imm12_);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FSOR vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSEQ) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 { 
		u8 *pjmp;
		int imm = 0;
		if (_Imm12_ & 0x0001) imm |= 0x0000f00; // Z
		if (_Imm12_ & 0x0002) imm |= 0x000f000; // S
		if (_Imm12_ & 0x0004) imm |= 0x0010000; // U
		if (_Imm12_ & 0x0008) imm |= 0x0020000; // O
		if (_Imm12_ & 0x0010) imm |= 0x0040000; // I
		if (_Imm12_ & 0x0020) imm |= 0x0080000; // D
		if (_Imm12_ & 0x0040) imm |= 0x000000f; // ZS
		if (_Imm12_ & 0x0080) imm |= 0x00000f0; // SS
		if (_Imm12_ & 0x0100) imm |= 0x0400000; // US
		if (_Imm12_ & 0x0200) imm |= 0x0800000; // OS
		if (_Imm12_ & 0x0400) imm |= 0x1000000; // IS
		if (_Imm12_ & 0x0800) imm |= 0x2000000; // DS

		mVUallocSFLAGa(gprT1, sFLAG.read);
		setBitFSEQ(0x0f00); // Z  bit
		setBitFSEQ(0xf000); // S  bit
		setBitFSEQ(0x000f); // ZS bit
		setBitFSEQ(0x00f0); // SS bit
		XOR32ItoR(gprT1, imm);
		SUB32ItoR(gprT1, 1);
		SHR32ItoR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("FSEQ vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSSET) {
	pass1 { mVUanalyzeFSSET(mVU); }
	pass2 { 
		int sReg, imm = 0;
		if (_Imm12_ & 0x0040) imm |= 0x000000f; // ZS
		if (_Imm12_ & 0x0080) imm |= 0x00000f0; // SS
		if (_Imm12_ & 0x0100) imm |= 0x0400000; // US
		if (_Imm12_ & 0x0200) imm |= 0x0800000; // OS
		if (_Imm12_ & 0x0400) imm |= 0x1000000; // IS
		if (_Imm12_ & 0x0800) imm |= 0x2000000; // DS
		getFlagReg(sReg, sFLAG.write);
		if (!(sFLAG.doFlag || mVUinfo.doDivFlag)) { 
			mVUallocSFLAGa(sReg, sFLAG.lastWrite); // Get Prev Status Flag
		}
		AND32ItoR(sReg, 0xfff00); // Keep Non-Sticky Bits
		if (imm) OR32ItoR(sReg, imm);
	}
	pass3 { mVUlog("FSSET $%x", _Imm12_); }
}

//------------------------------------------------------------------
// IADD/IADDI/IADDIU/IAND/IOR/ISUB/ISUBIU
//------------------------------------------------------------------

mVUop(mVU_IADD) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT2, _It_);
			ADD16RtoR(gprT1, gprT2);
		}
		else ADD16RtoR(gprT1, gprT1);
		mVUallocVIb(mVU, gprT1, _Id_);
	}
	pass3 { mVUlog("IADD vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_IADDI) {
	pass1 { mVUanalyzeIADDI(mVU, _Is_, _It_, _Imm5_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		ADD16ItoR(gprT1, _Imm5_);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("IADDI vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm5_); }
}

mVUop(mVU_IADDIU) {
	pass1 { mVUanalyzeIADDI(mVU, _Is_, _It_, _Imm15_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		ADD16ItoR(gprT1, _Imm15_);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("IADDIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

mVUop(mVU_IAND) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT2, _It_);
			AND32RtoR(gprT1, gprT2);
		}
		mVUallocVIb(mVU, gprT1, _Id_);
	}
	pass3 { mVUlog("IAND vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_IOR) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT2, _It_);
			OR32RtoR(gprT1, gprT2);
		}
		mVUallocVIb(mVU, gprT1, _Id_);
	}
	pass3 { mVUlog("IOR vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_ISUB) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 { 
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT1, _Is_);
			mVUallocVIa(mVU, gprT2, _It_);
			SUB16RtoR(gprT1, gprT2);
			mVUallocVIb(mVU, gprT1, _Id_);
		}
		else { 
			XOR32RtoR(gprT1, gprT1);
			mVUallocVIb(mVU, gprT1, _Id_);
		}
	}
	pass3 { mVUlog("ISUB vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_ISUBIU) {
	pass1 { mVUanalyzeIALU2(mVU, _Is_, _It_); }
	pass2 { 
		mVUallocVIa(mVU, gprT1, _Is_);
		SUB16ItoR(gprT1, _Imm15_);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("ISUBIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

//------------------------------------------------------------------
// MFIR/MFP/MOVE/MR32/MTIR
//------------------------------------------------------------------

mVUop(mVU_MFIR) {
	pass1 { if (!_Ft_) { mVUlow.isNOP = 1; } analyzeVIreg1(_Is_, mVUlow.VI_read[0]); analyzeReg2(_Ft_, mVUlow.VF_write, 1); }
	pass2 { 
		int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		mVUallocVIa(mVU, gprT1, _Is_);
		MOVSX32R16toR(gprT1, gprT1);
		SSE2_MOVD_R_to_XMM(Ft, gprT1);
		if (!_XYZW_SS) { mVUunpack_xyzw(Ft, Ft, 0); }
		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVUlog("MFIR.%s vf%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MFP) {
	pass1 { mVUanalyzeMFP(mVU, _Ft_); }
	pass2 { 
		int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		getPreg(mVU, Ft);
		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVUlog("MFP.%s vf%02d, P", _XYZW_String, _Ft_); }
}

mVUop(mVU_MOVE) {
	pass1 { mVUanalyzeMOVE(mVU, _Fs_, _Ft_); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, _Ft_, _X_Y_Z_W);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("MOVE.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MR32) {
	pass1 { mVUanalyzeMR32(mVU, _Fs_, _Ft_); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_);
		int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		if (_XYZW_SS) mVUunpack_xyzw(Ft, Fs, (_X ? 1 : (_Y ? 2 : (_Z ? 3 : 0))));
		else		  SSE2_PSHUFD_XMM_to_XMM(Ft, Fs, 0x39);
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("MR32.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MTIR) {
	pass1 { if (!_It_) { mVUlow.isNOP = 1; } analyzeReg5(_Fs_, _Fsf_, mVUlow.VF_read[0]); analyzeVIreg2(_It_, mVUlow.VI_write, 1); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		SSE2_MOVD_XMM_to_R(gprT1, Fs);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("MTIR vi%02d, vf%02d%s", _Ft_, _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// ILW/ILWR
//------------------------------------------------------------------

mVUop(mVU_ILW) {
	pass1 { if (!_It_) { mVUlow.isNOP = 1; } analyzeVIreg1(_Is_, mVUlow.VI_read[0]); analyzeVIreg2(_It_, mVUlow.VI_write, 4);  }
	pass2 { 
		if (!_Is_) {
			MOVZX32M16toR(gprT1, (uptr)mVU->regs->Mem + getVUmem(_Imm11_) + offsetSS);
			mVUallocVIb(mVU, gprT1, _It_);
		}
		else {
			mVUallocVIa(mVU, gprT1, _Is_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix(mVU, gprT1);
			MOVZX32Rm16toR(gprT1, gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb(mVU, gprT1, _It_);
		}
	}
	pass3 { mVUlog("ILW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

mVUop(mVU_ILWR) {
	pass1 { if (!_It_) { mVUlow.isNOP = 1; } analyzeVIreg1(_Is_, mVUlow.VI_read[0]); analyzeVIreg2(_It_, mVUlow.VI_write, 4); }
	pass2 { 
		if (!_Is_) {
			MOVZX32M16toR(gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb(mVU, gprT1, _It_);
		}
		else {
			mVUallocVIa(mVU, gprT1, _Is_);
			mVUaddrFix(mVU, gprT1);
			MOVZX32Rm16toR(gprT1, gprT1, (uptr)mVU->regs->Mem + offsetSS);
			mVUallocVIb(mVU, gprT1, _It_);
		}
	}
	pass3 { mVUlog("ILWR.%s vi%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// ISW/ISWR
//------------------------------------------------------------------

mVUop(mVU_ISW) {
	pass1 { analyzeVIreg1(_Is_, mVUlow.VI_read[0]); analyzeVIreg1(_It_, mVUlow.VI_read[1]); }
	pass2 { 
		if (!_Is_) {
			int imm = getVUmem(_Imm11_);
			mVUallocVIa(mVU, gprT1, _It_);
			if (_X) MOV32RtoM((uptr)mVU->regs->Mem + imm,		gprT1);
			if (_Y) MOV32RtoM((uptr)mVU->regs->Mem + imm + 4,	gprT1);
			if (_Z) MOV32RtoM((uptr)mVU->regs->Mem + imm + 8,	gprT1);
			if (_W) MOV32RtoM((uptr)mVU->regs->Mem + imm + 12,	gprT1);
		}
		else {
			mVUallocVIa(mVU, gprT1, _Is_);
			mVUallocVIa(mVU, gprT2, _It_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix(mVU, gprT1);
			if (_X) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem);
			if (_Y) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+4);
			if (_Z) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+8);
			if (_W) MOV32RtoRm(gprT1, gprT2, (uptr)mVU->regs->Mem+12);
		}
	}
	pass3 { mVUlog("ISW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_);  }
}

mVUop(mVU_ISWR) {
	pass1 { analyzeVIreg1(_Is_, mVUlow.VI_read[0]); analyzeVIreg1(_It_, mVUlow.VI_read[1]); }
	pass2 { 
		if (!_Is_) {
			mVUallocVIa(mVU, gprT1, _It_);
			if (_X) MOV32RtoM((uptr)mVU->regs->Mem,	   gprT1);
			if (_Y) MOV32RtoM((uptr)mVU->regs->Mem+4,  gprT1);
			if (_Z) MOV32RtoM((uptr)mVU->regs->Mem+8,  gprT1);
			if (_W) MOV32RtoM((uptr)mVU->regs->Mem+12, gprT1);
		}
		else {
			mVUallocVIa(mVU, gprT1, _Is_);
			mVUallocVIa(mVU, gprT2, _It_);
			mVUaddrFix (mVU, gprT1);
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

mVUop(mVU_LQ) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 0); }
	pass2 { 
		int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		if (_Is_) {
			mVUallocVIa(mVU, gprT1, _Is_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix(mVU, gprT1);
			mVUloadReg2(Ft, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
		}
		else mVUloadReg(Ft, (uptr)mVU->regs->Mem + getVUmem(_Imm11_), _X_Y_Z_W);
		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVUlog("LQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

mVUop(mVU_LQD) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 1); }
	pass2 { 
		if (_Is_) {
			mVUallocVIa(mVU, gprT1, _Is_);
			SUB16ItoR(gprT1, 1);
			mVUallocVIb(mVU, gprT1, _Is_);
			if (!mVUlow.noWriteVF) {
				int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
				mVUaddrFix(mVU, gprT1);
				mVUloadReg2(Ft, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
				mVU->regAlloc->clearNeeded(Ft);
			}
		}
		else if (!mVUlow.noWriteVF) {
			int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
			mVUloadReg(Ft, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			mVU->regAlloc->clearNeeded(Ft);
		}
	}
	pass3 { mVUlog("LQD.%s vf%02d, --vi%02d", _XYZW_String, _Ft_, _Is_); }
}

mVUop(mVU_LQI) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 1); }
	pass2 { 
		if (_Is_) {
			mVUallocVIa(mVU, (!mVUlow.noWriteVF) ? gprT1 : gprT2, _Is_);
			if (!mVUlow.noWriteVF) {
				int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
				MOV32RtoR(gprT2, gprT1);
				mVUaddrFix(mVU, gprT1);
				mVUloadReg2(Ft, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
				mVU->regAlloc->clearNeeded(Ft);
			}
			ADD16ItoR(gprT2, 1);
			mVUallocVIb(mVU, gprT2, _Is_);
		}
		else if (!mVUlow.noWriteVF) {
			int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
			mVUloadReg(Ft, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			mVU->regAlloc->clearNeeded(Ft);
		}
	}
	pass3 { mVUlog("LQI.%s vf%02d, vi%02d++", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// SQ/SQD/SQI
//------------------------------------------------------------------

mVUop(mVU_SQ) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 0); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		if (_It_) {
			mVUallocVIa(mVU, gprT1, _It_);
			ADD32ItoR(gprT1, _Imm11_);
			mVUaddrFix(mVU, gprT1);
			mVUsaveReg2(Fs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);		
		}
		else mVUsaveReg(Fs, (uptr)mVU->regs->Mem + getVUmem(_Imm11_), _X_Y_Z_W, 1);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("SQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Fs_, _Ft_, _Imm11_); }
}

mVUop(mVU_SQD) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 1); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		if (_It_) {
			mVUallocVIa(mVU, gprT1, _It_);
			SUB16ItoR(gprT1, 1);
			mVUallocVIb(mVU, gprT1, _It_);
			mVUaddrFix(mVU, gprT1);
			mVUsaveReg2(Fs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
		}
		else mVUsaveReg(Fs, (uptr)mVU->regs->Mem, _X_Y_Z_W, 1);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("SQD.%s vf%02d, --vi%02d", _XYZW_String, _Fs_, _Ft_); }
}

mVUop(mVU_SQI) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 1); }
	pass2 { 
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		if (_It_) {
			mVUallocVIa(mVU, gprT1, _It_);
			MOV32RtoR(gprT2, gprT1);
			mVUaddrFix(mVU, gprT1);
			mVUsaveReg2(Fs, gprT1, (uptr)mVU->regs->Mem, _X_Y_Z_W);
			ADD16ItoR(gprT2, 1);
			mVUallocVIb(mVU, gprT2, _It_);	
		}
		else mVUsaveReg(Fs, (uptr)mVU->regs->Mem, _X_Y_Z_W, 1);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("SQI.%s vf%02d, vi%02d++", _XYZW_String, _Fs_, _Ft_); }
}

//------------------------------------------------------------------
// RINIT/RGET/RNEXT/RXOR
//------------------------------------------------------------------

mVUop(mVU_RINIT) {
	pass1 { mVUanalyzeR1(mVU, _Fs_, _Fsf_); }
	pass2 { 
		if (_Fs_ || (_Fsf_ == 3)) {
			int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
			SSE2_MOVD_XMM_to_R(gprT1, Fs);
			AND32ItoR(gprT1, 0x007fffff);
			OR32ItoR (gprT1, 0x3f800000);
			MOV32RtoM(Rmem, gprT1);
			mVU->regAlloc->clearNeeded(Fs);
		}
		else MOV32ItoM(Rmem, 0x3f800000);
	}
	pass3 { mVUlog("RINIT R, vf%02d%s", _Fs_, _Fsf_String); }
}

microVUt(void) mVU_RGET_(mV, int Rreg) {
	if (!mVUlow.noWriteVF) {
		int Ft = mVU->regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		SSE2_MOVD_R_to_XMM(Ft, Rreg);
		if (!_XYZW_SS) mVUunpack_xyzw(Ft, Ft, 0);
		mVU->regAlloc->clearNeeded(Ft);
	}
}

mVUop(mVU_RGET) {
	pass1 { mVUanalyzeR2(mVU, _Ft_, 1); }
	pass2 { MOV32MtoR(gprT1, Rmem); mVU_RGET_(mVU, gprT1); }
	pass3 { mVUlog("RGET.%s vf%02d, R", _XYZW_String, _Ft_); }
}

mVUop(mVU_RNEXT) {
	pass1 { mVUanalyzeR2(mVU, _Ft_, 0); }
	pass2 { 
		// algorithm from www.project-fao.org
		MOV32MtoR(gprR, Rmem);
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
		MOV32RtoM(Rmem, gprR);
		mVU_RGET_(mVU, gprR);
		MOV32ItoR(gprR, Roffset); // Restore gprR
	}
	pass3 { mVUlog("RNEXT.%s vf%02d, R", _XYZW_String, _Ft_); }
}

mVUop(mVU_RXOR) {
	pass1 { mVUanalyzeR1(mVU, _Fs_, _Fsf_); }
	pass2 { 
		if (_Fs_ || (_Fsf_ == 3)) {
			int Fs = mVU->regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
			SSE2_MOVD_XMM_to_R(gprT1, Fs);
			AND32ItoR(gprT1, 0x7fffff);
			XOR32RtoM(Rmem,  gprT1);
			mVU->regAlloc->clearNeeded(Fs);
		}
	}
	pass3 { mVUlog("RXOR R, vf%02d%s", _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// WaitP/WaitQ
//------------------------------------------------------------------

mVUop(mVU_WAITP) {
	pass1 { mVUstall = aMax(mVUstall, ((mVUregs.p) ? (mVUregs.p - 1) : 0)); }
	pass3 { mVUlog("WAITP"); }
}

mVUop(mVU_WAITQ) {
	pass1 { mVUstall = aMax(mVUstall, mVUregs.q); }
	pass3 { mVUlog("WAITQ"); }
}

//------------------------------------------------------------------
// XTOP/XITOP
//------------------------------------------------------------------

mVUop(mVU_XTOP) {
	pass1 { if (!_It_) { mVUlow.isNOP = 1; } analyzeVIreg2(_It_, mVUlow.VI_write, 1); }
	pass2 { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->vifRegs->top);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("XTOP vi%02d", _Ft_); }
}

mVUop(mVU_XITOP) {
	pass1 { if (!_It_) { mVUlow.isNOP = 1; } analyzeVIreg2(_It_, mVUlow.VI_write, 1); }
	pass2 { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->vifRegs->itop);
		AND32ItoR(gprT1, isVU1 ? 0x3ff : 0xff);
		mVUallocVIb(mVU, gprT1, _It_);
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
/*	if((size << 4) > (0x4000-(addr&0x3fff))) {
		//DevCon::Notice("addr + Size = 0x%x, transferring %x then doing %x", params (addr&0x3fff) + (size << 4), (0x4000-(addr&0x3fff)) >> 4, size - ((0x4000-(addr&0x3fff)) >> 4));
		memcpy_aligned(pDest, microVU1.regs->Mem + addr, 0x4000-(addr&0x3fff));
		size -= (0x4000-(addr&0x3fff)) >> 4;
		//DevCon::Notice("Size left %x", params size);
		pDest += 0x4000-(addr&0x3fff);
		memcpy_aligned(pDest, microVU1.regs->Mem, size<<4);
	}
	else { */
		memcpy_aligned(pDest, microVU1.regs->Mem + addr, size<<4);
//	}
	mtgsThread->SendDataPacket();
}

microVUt(void) mVU_XGKICK_DELAY(mV, bool memVI) {
	mVUbackupRegs(mVU);
	if (memVI)		MOV32MtoR(gprT2, (uptr)&mVU->VIxgkick);
	else			mVUallocVIa(mVU, gprT2, _Is_);

	CALLFunc((uptr)mVU_XGKICK_);
	mVUrestoreRegs(mVU);
}

mVUop(mVU_XGKICK) {
	pass1 { mVUanalyzeXGkick(mVU, _Is_, mVU_XGKICK_CYCLES); }
	pass2 {
		if (!mVU_XGKICK_CYCLES)		{ mVU_XGKICK_DELAY(mVU, 0); return; }
		else if (mVUinfo.doXGKICK)	{ mVU_XGKICK_DELAY(mVU, 1); mVUinfo.doXGKICK = 0; }
		mVUallocVIa(mVU, gprT1, _Is_); 
		MOV32RtoM((uptr)&mVU->VIxgkick, gprT1);
	}
	pass3 { mVUlog("XGKICK vi%02d", _Fs_); }
}

//------------------------------------------------------------------
// Branches/Jumps
//------------------------------------------------------------------

void setBranchA(mP, int x, int _x_) {
	pass1 { 
		if (_Imm11_ == 1 && !_x_) { mVUlow.isNOP = 1; return; } 
		mVUbranch	  = x; 
		mVUlow.branch = x;
	}
	pass2 { if (_Imm11_ == 1 && !_x_) { return; } mVUbranch = x; }
	pass3 { mVUbranch = x; }
	pass4 { if (_Imm11_ == 1 && !_x_) { return; } mVUbranch = x; }
}

void condEvilBranch(mV, int JMPcc) {
	using namespace x86Emitter;
	if (mVUlow.badBranch) {
		xMOV(ptr32[&mVU->branch], eax);
		xMOV(ptr32[&mVU->badBranch], branchAddrN);
		xCMP(ax, 0);
		xForwardJump8 cJMP((JccComparisonType)JMPcc);
			incPC(4); // Branch Not Taken
			xMOV(ptr32[&mVU->badBranch], xPC);
			incPC(-4);
		cJMP.SetTarget();
		return;
	}
	xMOV(ptr32[&mVU->evilBranch], branchAddr);
	xCMP(ax, 0);
	xForwardJump8 cJMP((JccComparisonType)JMPcc);
		xMOV(eax, ptr32[&mVU->badBranch]); // Branch Not Taken
		xMOV(ptr32[&mVU->evilBranch], eax);
	cJMP.SetTarget();
}

mVUop(mVU_B) {
	setBranchA(mX, 1, 0);
	pass1 { mVUanalyzeNormBranch(mVU, 0, 0); }
	pass2 { 
		if (mVUlow.badBranch)  { MOV32ItoM((uptr)&mVU->badBranch,  branchAddrN); }
		if (mVUlow.evilBranch) { MOV32ItoM((uptr)&mVU->evilBranch, branchAddr); } 
	}
	pass3 { mVUlog("B [<a href=\"#addr%04x\">%04x</a>]", branchAddr, branchAddr); }
}

mVUop(mVU_BAL) {
	setBranchA(mX, 2, _It_);
	pass1 { mVUanalyzeNormBranch(mVU, _It_, 1); }
	pass2 {
		MOV32ItoR(gprT1, bSaveAddr);
		mVUallocVIb(mVU, gprT1, _It_);
		if (mVUlow.badBranch)  { MOV32ItoM((uptr)&mVU->badBranch,  branchAddrN); }
		if (mVUlow.evilBranch) { MOV32ItoM((uptr)&mVU->evilBranch, branchAddr); } 
	}
	pass3 { mVUlog("BAL vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, branchAddr, branchAddr); }
}

mVUop(mVU_IBEQ) {
	using namespace x86Emitter;
	setBranchA(mX, 3, 0);
	pass1 { mVUanalyzeCondBranch2(mVU, _Is_, _It_); }
	pass2 {
		if (mVUlow.memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else mVUallocVIa(mVU, gprT1, _Is_);
		
		if (mVUlow.memReadIt) XOR32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else { mVUallocVIa(mVU, gprT2, _It_); XOR32RtoR(gprT1, gprT2); }

		if (!(isBadOrEvil))	MOV32RtoM((uptr)&mVU->branch, gprT1);
		else				condEvilBranch(mVU, Jcc_Equal);
	}
	pass3 { mVUlog("IBEQ vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBGEZ) {
	using namespace x86Emitter;
	setBranchA(mX, 4, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		MOV32RtoM((uptr)&mVU->branch, gprT1);
		else					condEvilBranch(mVU, Jcc_GreaterOrEqual);
	}
	pass3 { mVUlog("IBGEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBGTZ) {
	using namespace x86Emitter;
	setBranchA(mX, 5, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		MOV32RtoM((uptr)&mVU->branch, gprT1);
		else					condEvilBranch(mVU, Jcc_Greater);
	}
	pass3 { mVUlog("IBGTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBLEZ) {
	using namespace x86Emitter;
	setBranchA(mX, 6, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		MOV32RtoM((uptr)&mVU->branch, gprT1);
		else					condEvilBranch(mVU, Jcc_LessOrEqual);
	}
	pass3 { mVUlog("IBLEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBLTZ) {
	using namespace x86Emitter;
	setBranchA(mX, 7, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {	
		if (mVUlow.memReadIs)	MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		MOV32RtoM((uptr)&mVU->branch, gprT1);
		else					condEvilBranch(mVU, Jcc_Less);
	}
	pass3 { mVUlog("IBLTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBNE) {
	using namespace x86Emitter;
	setBranchA(mX, 8, 0);
	pass1 { mVUanalyzeCondBranch2(mVU, _Is_, _It_); }
	pass2 {
		if (mVUlow.memReadIs) MOV32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else mVUallocVIa(mVU, gprT1, _Is_);
		
		if (mVUlow.memReadIt) XOR32MtoR(gprT1, (uptr)&mVU->VIbackup);
		else { mVUallocVIa(mVU, gprT2, _It_); XOR32RtoR(gprT1, gprT2); }
		
		if (!(isBadOrEvil))	MOV32RtoM((uptr)&mVU->branch, gprT1);
		else				condEvilBranch(mVU, Jcc_NotEqual);
	}
	pass3 { mVUlog("IBNE vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

void normJumpPass2(mV) {
	if (!mVUlow.constJump.isValid || mVUlow.evilBranch) {
		mVUallocVIa(mVU, gprT1, _Is_);
		SHL32ItoR(gprT1, 3);
		AND32ItoR(gprT1, mVU->microMemSize - 8);
		MOV32RtoM((uptr)&mVU->branch, gprT1);
		if (!mVUlow.evilBranch) MOV32RtoM((uptr)&mVU->branch,	  gprT1);
		else					MOV32RtoM((uptr)&mVU->evilBranch, gprT1);
		if (mVUlow.badBranch) {
			ADD32ItoR(gprT1, 8);
			AND32ItoR(gprT1, mVU->microMemSize - 8);
			MOV32RtoM((uptr)&mVU->badBranch,  gprT1);
		}
	}
}

mVUop(mVU_JR) {
	mVUbranch = 9;
	pass1 { mVUanalyzeJump(mVU, _Is_, 0, 0); }
	pass2 { normJumpPass2(mVU); }
	pass3 { mVUlog("JR [vi%02d]", _Fs_); }
}

mVUop(mVU_JALR) {
	mVUbranch = 10;
	pass1 { mVUanalyzeJump(mVU, _Is_, _It_, 1); }
	pass2 {
		normJumpPass2(mVU);
		MOV32ItoR(gprT1, bSaveAddr);
		mVUallocVIb(mVU, gprT1, _It_);
	}
	pass3 { mVUlog("JALR vi%02d, [vi%02d]", _Ft_, _Fs_); }
}

