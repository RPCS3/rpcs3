/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

//------------------------------------------------------------------
// DIV/SQRT/RSQRT
//------------------------------------------------------------------

// Test if Vector is +/- Zero
static __fi void testZero(const xmm& xmmReg, const xmm& xmmTemp, const x32& gprTemp)
{
	xXOR.PS(xmmTemp, xmmTemp);
	xCMPEQ.SS(xmmTemp, xmmReg);
	if (!x86caps.hasStreamingSIMD4Extensions) {
		xMOVMSKPS(gprTemp, xmmTemp);
		xTEST(gprTemp, 1);
	}
	else xPTEST(xmmTemp, xmmTemp);
}

// Test if Vector is Negative (Set Flags and Makes Positive)
static __fi void testNeg(mV, const xmm& xmmReg, const x32& gprTemp)
{
	xMOVMSKPS(gprTemp, xmmReg);
	xTEST(gprTemp, 1);
	xForwardJZ8 skip;
		xMOV(ptr32[&mVU.divFlag], divI);
		xAND.PS(xmmReg, ptr128[mVUglob.absclip]);
	skip.SetTarget();
}

mVUop(mVU_DIV) {
	pass1 { mVUanalyzeFDIV(mVU, _Fs_, _Fsf_, _Ft_, _Ftf_, 7); }
	pass2 {
		xmm Ft;
		if (_Ftf_) Ft = mVU.regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));
		else	   Ft = mVU.regAlloc->allocReg(_Ft_);
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		const xmm& t1 = mVU.regAlloc->allocReg();

		testZero(Ft, t1, gprT1); // Test if Ft is zero
		xForwardJZ8 cjmp; // Skip if not zero

			testZero(Fs, t1, gprT1); // Test if Fs is zero
			xForwardJZ8 ajmp;
				xMOV(ptr32[&mVU.divFlag], divI); // Set invalid flag (0/0)
				xForwardJump8 bjmp;
			ajmp.SetTarget();
				xMOV(ptr32[&mVU.divFlag], divD); // Zero divide (only when not 0/0)
			bjmp.SetTarget();

			xXOR.PS(Fs, Ft);
			xAND.PS(Fs, ptr128[mVUglob.signbit]);
			xOR.PS (Fs, ptr128[mVUglob.maxvals]); // If division by zero, then xmmFs = +/- fmax

			xForwardJump8 djmp;
		cjmp.SetTarget();
			xMOV(ptr32[&mVU.divFlag], 0); // Clear I/D flags
			SSE_DIVSS(mVU, Fs, Ft);
			mVUclamp1(Fs, t1, 8, 1);
		djmp.SetTarget();

		writeQreg(Fs, mVUinfo.writeQ);

		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(Ft);
		mVU.regAlloc->clearNeeded(t1);
		mVU.profiler.EmitOp(opDIV);
	}
	pass3 { mVUlog("DIV Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

mVUop(mVU_SQRT) {
	pass1 { mVUanalyzeFDIV(mVU, 0, 0, _Ft_, _Ftf_, 7); }
	pass2 {
		const xmm& Ft = mVU.regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));

		xMOV(ptr32[&mVU.divFlag], 0); // Clear I/D flags
		testNeg(mVU, Ft, gprT1); // Check for negative sqrt

		if (CHECK_VU_OVERFLOW) xMIN.SS(Ft, ptr32[mVUglob.maxvals]); // Clamp infinities (only need to do positive clamp since xmmFt is positive)
		xSQRT.SS(Ft, Ft);
		writeQreg(Ft, mVUinfo.writeQ);

		mVU.regAlloc->clearNeeded(Ft);
		mVU.profiler.EmitOp(opSQRT);
	}
	pass3 { mVUlog("SQRT Q, vf%02d%s", _Ft_, _Ftf_String); }
}

mVUop(mVU_RSQRT) {
	pass1 { mVUanalyzeFDIV(mVU, _Fs_, _Fsf_, _Ft_, _Ftf_, 13); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		const xmm& Ft = mVU.regAlloc->allocReg(_Ft_, 0, (1 << (3 - _Ftf_)));
		const xmm& t1 = mVU.regAlloc->allocReg();

		xMOV(ptr32[&mVU.divFlag], 0); // Clear I/D flags
		testNeg(mVU, Ft, gprT1); // Check for negative sqrt

		xSQRT.SS(Ft, Ft);
		testZero(Ft, t1, gprT1); // Test if Ft is zero
		xForwardJZ8 ajmp; // Skip if not zero

			testZero(Fs, t1, gprT1); // Test if Fs is zero
			xForwardJZ8 bjmp; // Skip if none are
				xMOV(ptr32[&mVU.divFlag], divI); // Set invalid flag (0/0)
				xForwardJump8 cjmp;
			bjmp.SetTarget();
				xMOV(ptr32[&mVU.divFlag], divD); // Zero divide flag (only when not 0/0)
			cjmp.SetTarget();

			xAND.PS(Fs, ptr128[mVUglob.signbit]);
			xOR.PS (Fs, ptr128[mVUglob.maxvals]); // xmmFs = +/-Max

			xForwardJump8 djmp;
		ajmp.SetTarget();
			SSE_DIVSS(mVU, Fs, Ft);
			mVUclamp1(Fs, t1, 8, 1);
		djmp.SetTarget();

		writeQreg(Fs, mVUinfo.writeQ);

		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(Ft);
		mVU.regAlloc->clearNeeded(t1);
		mVU.profiler.EmitOp(opRSQRT);
	}
	pass3 { mVUlog("RSQRT Q, vf%02d%s, vf%02d%s", _Fs_, _Fsf_String, _Ft_, _Ftf_String); }
}

//------------------------------------------------------------------
// EATAN/EEXP/ELENG/ERCPR/ERLENG/ERSADD/ERSQRT/ESADD/ESIN/ESQRT/ESUM
//------------------------------------------------------------------

#define EATANhelper(addr) {				\
	SSE_MULSS(mVU, t2, Fs);				\
	SSE_MULSS(mVU, t2, Fs);				\
	xMOVAPS       (t1, t2);				\
	xMUL.SS       (t1, ptr32[addr]);	\
	SSE_ADDSS(mVU, PQ, t1);				\
}

// ToDo: Can Be Optimized Further? (takes approximately (~115 cycles + mem access time) on a c2d)
static __fi void mVU_EATAN_(mV, const xmm& PQ, const xmm& Fs, const xmm& t1, const xmm& t2) {
	xMOVSS(PQ, Fs);
	xMUL.SS(PQ, ptr32[mVUglob.T1]);
	xMOVAPS(t2, Fs);
	EATANhelper(mVUglob.T2);
	EATANhelper(mVUglob.T3);
	EATANhelper(mVUglob.T4);
	EATANhelper(mVUglob.T5);
	EATANhelper(mVUglob.T6);
	EATANhelper(mVUglob.T7);
	EATANhelper(mVUglob.T8);
	xADD.SS(PQ, ptr32[mVUglob.Pi4]);
	xPSHUF.D(PQ, PQ, mVUinfo.writeP ? 0x27 : 0xC6);
}

mVUop(mVU_EATAN) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 54); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		const xmm& t1 = mVU.regAlloc->allocReg();
		const xmm& t2 = mVU.regAlloc->allocReg();
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS (xmmPQ, Fs);
		xSUB.SS(Fs,    ptr32[mVUglob.one]);
		xADD.SS(xmmPQ, ptr32[mVUglob.one]);
		SSE_DIVSS(mVU, Fs, xmmPQ);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.regAlloc->clearNeeded(t2);
		mVU.profiler.EmitOp(opEATAN);
	}
	pass3 { mVUlog("EATAN P"); }
}

mVUop(mVU_EATANxy) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 54); }
	pass2 {
		const xmm& t1 = mVU.regAlloc->allocReg(_Fs_, 0, 0xf);
		const xmm& Fs = mVU.regAlloc->allocReg();
		const xmm& t2 = mVU.regAlloc->allocReg();
		xPSHUF.D(Fs, t1, 0x01);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS  (xmmPQ, Fs);
		SSE_SUBSS (mVU, Fs, t1); // y-x, not y-1? ><
		SSE_ADDSS (mVU, t1, xmmPQ);
		SSE_DIVSS (mVU, Fs, t1);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.regAlloc->clearNeeded(t2);
		mVU.profiler.EmitOp(opEATANxy);
	}
	pass3 { mVUlog("EATANxy P"); }
}

mVUop(mVU_EATANxz) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 54); }
	pass2 {
		const xmm& t1 = mVU.regAlloc->allocReg(_Fs_, 0, 0xf);
		const xmm& Fs = mVU.regAlloc->allocReg();
		const xmm& t2 = mVU.regAlloc->allocReg();
		xPSHUF.D(Fs, t1, 0x02);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS  (xmmPQ, Fs);
		SSE_SUBSS (mVU, Fs, t1);
		SSE_ADDSS (mVU, t1, xmmPQ);
		SSE_DIVSS (mVU, Fs, t1);
		mVU_EATAN_(mVU, xmmPQ, Fs, t1, t2);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.regAlloc->clearNeeded(t2);
		mVU.profiler.EmitOp(opEATANxz);
	}
	pass3 { mVUlog("EATANxz P"); }
}

#define eexpHelper(addr) {				\
	SSE_MULSS(mVU, t2, Fs);				\
	xMOVAPS       (t1, t2);				\
	xMUL.SS       (t1, ptr32[addr]);	\
	SSE_ADDSS(mVU, xmmPQ, t1);			\
}

mVUop(mVU_EEXP) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 44); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		const xmm& t1 = mVU.regAlloc->allocReg();
		const xmm& t2 = mVU.regAlloc->allocReg();
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS  (xmmPQ, Fs);
		xMUL.SS (xmmPQ, ptr32[mVUglob.E1]);
		xADD.SS (xmmPQ, ptr32[mVUglob.one]);
		xMOVAPS       (t1, Fs);
		SSE_MULSS(mVU, t1, Fs);
		xMOVAPS       (t2, t1);
		xMUL.SS       (t1, ptr32[mVUglob.E2]);
		SSE_ADDSS(mVU, xmmPQ, t1);
		eexpHelper(&mVUglob.E3);
		eexpHelper(&mVUglob.E4);
		eexpHelper(&mVUglob.E5);
		SSE_MULSS(mVU, t2, Fs);
		xMUL.SS       (t2, ptr32[mVUglob.E6]);
		SSE_ADDSS(mVU, xmmPQ, t2);
		SSE_MULSS(mVU, xmmPQ, xmmPQ);
		SSE_MULSS(mVU, xmmPQ, xmmPQ);
		xMOVSSZX      (t2, ptr32[mVUglob.one]);
		SSE_DIVSS(mVU, t2, xmmPQ);
		xMOVSS        (xmmPQ, t2);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.regAlloc->clearNeeded(t2);
		mVU.profiler.EmitOp(opEEXP);
	}
	pass3 { mVUlog("EEXP P"); }
}

// sumXYZ(): PQ.x = x ^ 2 + y ^ 2 + z ^ 2
static __fi void mVU_sumXYZ(mV, const xmm& PQ, const xmm& Fs) {
	if (x86caps.hasStreamingSIMD4Extensions) {
		xDP.PS(Fs, Fs, 0x71);
		xMOVSS(PQ, Fs);
	}
	else {
		SSE_MULPS(mVU, Fs, Fs);       // wzyx ^ 2
		xMOVSS        (PQ, Fs);		  // x ^ 2
		xPSHUF.D      (Fs, Fs, 0xe1); // wzyx -> wzxy
		SSE_ADDSS(mVU, PQ, Fs);       // x ^ 2 + y ^ 2
		xPSHUF.D      (Fs, Fs, 0xd2); // wzxy -> wxyz
		SSE_ADDSS(mVU, PQ, Fs);       // x ^ 2 + y ^ 2 + z ^ 2
	}
}

mVUop(mVU_ELENG) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 18); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(mVU, xmmPQ, Fs);
		xSQRT.SS       (xmmPQ, xmmPQ);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opELENG);
	}
	pass3 { mVUlog("ELENG P"); }
}

mVUop(mVU_ERCPR) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 12); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS        (xmmPQ, Fs);
		xMOVSSZX      (Fs, ptr32[mVUglob.one]);
		SSE_DIVSS(mVU, Fs, xmmPQ);
		xMOVSS        (xmmPQ, Fs);
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opERCPR);
	}
	pass3 { mVUlog("ERCPR P"); }
}

mVUop(mVU_ERLENG) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 24); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(mVU, xmmPQ, Fs);
		xSQRT.SS       (xmmPQ, xmmPQ);
		xMOVSSZX       (Fs, ptr32[mVUglob.one]);
		SSE_DIVSS (mVU, Fs, xmmPQ);
		xMOVSS         (xmmPQ, Fs);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opERLENG);
	}
	pass3 { mVUlog("ERLENG P"); }
}

mVUop(mVU_ERSADD) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 18); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(mVU, xmmPQ, Fs);
		xMOVSSZX       (Fs, ptr32[mVUglob.one]);
		SSE_DIVSS (mVU, Fs, xmmPQ);
		xMOVSS         (xmmPQ, Fs);
		xPSHUF.D       (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opERSADD);
	}
	pass3 { mVUlog("ERSADD P"); }
}

mVUop(mVU_ERSQRT) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 18); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xAND.PS       (Fs, ptr128[mVUglob.absclip]);
		xSQRT.SS      (xmmPQ, Fs);
		xMOVSSZX      (Fs, ptr32[mVUglob.one]);
		SSE_DIVSS(mVU, Fs, xmmPQ);
		xMOVSS        (xmmPQ, Fs);
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opERSQRT);
	}
	pass3 { mVUlog("ERSQRT P"); }
}

mVUop(mVU_ESADD) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 11); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		mVU_sumXYZ(mVU, xmmPQ, Fs);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opESADD);
	}
	pass3 { mVUlog("ESADD P"); }
}

mVUop(mVU_ESIN) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 29); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		const xmm& t1 = mVU.regAlloc->allocReg();
		const xmm& t2 = mVU.regAlloc->allocReg();
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xMOVSS        (xmmPQ, Fs); // pq = X
		SSE_MULSS(mVU, Fs, Fs);    // fs = X^2
		xMOVAPS       (t1, Fs);    // t1 = X^2
		SSE_MULSS(mVU, Fs, xmmPQ); // fs = X^3
		xMOVAPS       (t2, Fs);    // t2 = X^3
		xMUL.SS       (Fs, ptr32[mVUglob.S2]); // fs = s2 * X^3
		SSE_ADDSS(mVU, xmmPQ, Fs); // pq = X + s2 * X^3

		SSE_MULSS(mVU, t2, t1);    // t2 = X^3 * X^2
		xMOVAPS       (Fs, t2);    // fs = X^5
		xMUL.SS       (Fs, ptr32[mVUglob.S3]); // ps = s3 * X^5
		SSE_ADDSS(mVU, xmmPQ, Fs); // pq = X + s2 * X^3 + s3 * X^5

		SSE_MULSS(mVU, t2, t1);    // t2 = X^5 * X^2
		xMOVAPS       (Fs, t2);    // fs = X^7
		xMUL.SS       (Fs, ptr32[mVUglob.S4]); // fs = s4 * X^7
		SSE_ADDSS(mVU, xmmPQ, Fs); // pq = X + s2 * X^3 + s3 * X^5 + s4 * X^7

		SSE_MULSS(mVU, t2, t1);    // t2 = X^7 * X^2
		xMUL.SS       (t2, ptr32[mVUglob.S5]); // t2 = s5 * X^9
		SSE_ADDSS(mVU, xmmPQ, t2); // pq = X + s2 * X^3 + s3 * X^5 + s4 * X^7 + s5 * X^9
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.regAlloc->clearNeeded(t2);
		mVU.profiler.EmitOp(opESIN);
	}
	pass3 { mVUlog("ESIN P"); }
}

mVUop(mVU_ESQRT) {
	pass1 { mVUanalyzeEFU1(mVU, _Fs_, _Fsf_, 12); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xAND.PS (Fs, ptr128[mVUglob.absclip]);
		xSQRT.SS(xmmPQ, Fs);
		xPSHUF.D(xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opESQRT);
	}
	pass3 { mVUlog("ESQRT P"); }
}

mVUop(mVU_ESUM) {
	pass1 { mVUanalyzeEFU2(mVU, _Fs_, 12); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		const xmm& t1 = mVU.regAlloc->allocReg();
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip xmmPQ to get Valid P instance
		xPSHUF.D      (t1, Fs, 0x1b);
		SSE_ADDPS(mVU, Fs, t1);
		xPSHUF.D      (t1, Fs, 0x01);
		SSE_ADDSS(mVU, Fs, t1);
		xMOVSS        (xmmPQ, Fs);
		xPSHUF.D      (xmmPQ, xmmPQ, mVUinfo.writeP ? 0x27 : 0xC6); // Flip back
		mVU.regAlloc->clearNeeded(Fs);
		mVU.regAlloc->clearNeeded(t1);
		mVU.profiler.EmitOp(opESUM);
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
		xAND(gprT1, _Imm24_);
		xADD(gprT1, 0xffffff);
		xSHR(gprT1, 24);
		mVUallocVIb(mVU, gprT1, 1);
		mVU.profiler.EmitOp(opFCAND);
	}
	pass3 { mVUlog("FCAND vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCEQ) {
	pass1 { mVUanalyzeCflag(mVU, 1); }
	pass2 {
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		xXOR(gprT1, _Imm24_);
		xSUB(gprT1, 1);
		xSHR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, 1);
		mVU.profiler.EmitOp(opFCEQ);
	}
	pass3 { mVUlog("FCEQ vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCGET) {
	pass1 { mVUanalyzeCflag(mVU, _It_); }
	pass2 {
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		xAND(gprT1, 0xfff);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFCGET);
	}
	pass3 { mVUlog("FCGET vi%02d", _Ft_);	   }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCOR) {
	pass1 { mVUanalyzeCflag(mVU, 1); }
	pass2 {
		mVUallocCFLAGa(mVU, gprT1, cFLAG.read);
		xOR(gprT1, _Imm24_);
		xADD(gprT1, 1);  // If 24 1's will make 25th bit 1, else 0
		xSHR(gprT1, 24); // Get the 25th bit (also clears the rest of the garbage in the reg)
		mVUallocVIb(mVU, gprT1, 1);
		mVU.profiler.EmitOp(opFCOR);
	}
	pass3 { mVUlog("FCOR vi01, $%x", _Imm24_); }
	pass4 { mVUregs.needExactMatch |= 4; }
}

mVUop(mVU_FCSET) {
	pass1 { cFLAG.doFlag = 1; }
	pass2 {
		xMOV(gprT1, _Imm24_);
		mVUallocCFLAGb(mVU, gprT1, cFLAG.write);
		mVU.profiler.EmitOp(opFCSET);
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
		xAND(gprT1b, gprT2b);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFMAND);
	}
	pass3 { mVUlog("FMAND vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2; }
}

mVUop(mVU_FMEQ) {
	pass1 { mVUanalyzeMflag(mVU, _Is_, _It_); }
	pass2 {
		mVUallocMFLAGa(mVU, gprT1, mFLAG.read);
		mVUallocVIa(mVU, gprT2, _Is_);
		xXOR(gprT1, gprT2);
		xSUB(gprT1, 1);
		xSHR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFMEQ);
	}
	pass3 { mVUlog("FMEQ vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2; }
}

mVUop(mVU_FMOR) {
	pass1 { mVUanalyzeMflag(mVU, _Is_, _It_); }
	pass2 {
		mVUallocMFLAGa(mVU, gprT1, mFLAG.read);
		mVUallocVIa(mVU, gprT2, _Is_);
		xOR(gprT1b, gprT2b);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFMOR);
	}
	pass3 { mVUlog("FMOR vi%02d, vi%02d", _Ft_, _Fs_); }
	pass4 { mVUregs.needExactMatch |= 2; }
}

//------------------------------------------------------------------
// FSAND/FSEQ/FSOR/FSSET
//------------------------------------------------------------------

mVUop(mVU_FSAND) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 {
		if (_Imm12_ & 0x0c30) DevCon.WriteLn(Color_Green, "mVU_FSAND: Checking I/D/IS/DS Flags");
		if (_Imm12_ & 0x030c) DevCon.WriteLn(Color_Green, "mVU_FSAND: Checking U/O/US/OS Flags");
		mVUallocSFLAGc(gprT1, gprT2, sFLAG.read);
		xAND(gprT1, _Imm12_);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFSAND);
	}
	pass3 { mVUlog("FSAND vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSOR) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 {
		mVUallocSFLAGc(gprT1, gprT2, sFLAG.read);
		xOR(gprT1, _Imm12_);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFSOR);
	}
	pass3 { mVUlog("FSOR vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSEQ) {
	pass1 { mVUanalyzeSflag(mVU, _It_); }
	pass2 {
		int imm = 0;
		if (_Imm12_ & 0x0c30) DevCon.WriteLn(Color_Green, "mVU_FSEQ: Checking I/D/IS/DS Flags");
		if (_Imm12_ & 0x030c) DevCon.WriteLn(Color_Green, "mVU_FSEQ: Checking U/O/US/OS Flags");
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
		setBitFSEQ(gprT1, 0x0f00); // Z  bit
		setBitFSEQ(gprT1, 0xf000); // S  bit
		setBitFSEQ(gprT1, 0x000f); // ZS bit
		setBitFSEQ(gprT1, 0x00f0); // SS bit
		xXOR(gprT1, imm);
		xSUB(gprT1, 1);
		xSHR(gprT1, 31);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opFSEQ);
	}
	pass3 { mVUlog("FSEQ vi%02d, $%x", _Ft_, _Imm12_); }
	pass4 { mVUregs.needExactMatch |= 1; }
}

mVUop(mVU_FSSET) {
	pass1 { mVUanalyzeFSSET(mVU); }
	pass2 {
		int imm = 0;
		if (_Imm12_ & 0x0040) imm |= 0x000000f; // ZS
		if (_Imm12_ & 0x0080) imm |= 0x00000f0; // SS
		if (_Imm12_ & 0x0100) imm |= 0x0400000; // US
		if (_Imm12_ & 0x0200) imm |= 0x0800000; // OS
		if (_Imm12_ & 0x0400) imm |= 0x1000000; // IS
		if (_Imm12_ & 0x0800) imm |= 0x2000000; // DS
		if (!(sFLAG.doFlag || mVUinfo.doDivFlag)) {
			mVUallocSFLAGa(getFlagReg(sFLAG.write), sFLAG.lastWrite); // Get Prev Status Flag
		}
		xAND(getFlagReg(sFLAG.write), 0xfff00); // Keep Non-Sticky Bits
		if (imm) xOR(getFlagReg(sFLAG.write), imm);
		mVU.profiler.EmitOp(opFSSET);
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
			xADD(gprT1b, gprT2b);
		}
		else xADD(gprT1b, gprT1b);
		mVUallocVIb(mVU, gprT1, _Id_);
		mVU.profiler.EmitOp(opIADD);
	}
	pass3 { mVUlog("IADD vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_IADDI) {
	pass1 { mVUanalyzeIADDI(mVU, _Is_, _It_, _Imm5_); }
	pass2 {
		mVUallocVIa(mVU, gprT1, _Is_);
		xADD(gprT1b, _Imm5_);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opIADDI);
	}
	pass3 { mVUlog("IADDI vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm5_); }
}

mVUop(mVU_IADDIU) {
	pass1 { mVUanalyzeIADDI(mVU, _Is_, _It_, _Imm15_); }
	pass2 {
		mVUallocVIa(mVU, gprT1, _Is_);
		xADD(gprT1b, _Imm15_);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opIADDIU);
	}
	pass3 { mVUlog("IADDIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

mVUop(mVU_IAND) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 {
		mVUallocVIa(mVU, gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT2, _It_);
			xAND(gprT1, gprT2);
		}
		mVUallocVIb(mVU, gprT1, _Id_);
		mVU.profiler.EmitOp(opIAND);
	}
	pass3 { mVUlog("IAND vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_IOR) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 {
		mVUallocVIa(mVU, gprT1, _Is_);
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT2, _It_);
			xOR(gprT1, gprT2);
		}
		mVUallocVIb(mVU, gprT1, _Id_);
		mVU.profiler.EmitOp(opIOR);
	}
	pass3 { mVUlog("IOR vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_ISUB) {
	pass1 { mVUanalyzeIALU1(mVU, _Id_, _Is_, _It_); }
	pass2 {
		if (_It_ != _Is_) {
			mVUallocVIa(mVU, gprT1, _Is_);
			mVUallocVIa(mVU, gprT2, _It_);
			xSUB(gprT1b, gprT2b);
			mVUallocVIb(mVU, gprT1, _Id_);
		}
		else {
			xXOR(gprT1, gprT1);
			mVUallocVIb(mVU, gprT1, _Id_);
		}
		mVU.profiler.EmitOp(opISUB);
	}
	pass3 { mVUlog("ISUB vi%02d, vi%02d, vi%02d", _Fd_, _Fs_, _Ft_); }
}

mVUop(mVU_ISUBIU) {
	pass1 { mVUanalyzeIALU2(mVU, _Is_, _It_); }
	pass2 {
		mVUallocVIa(mVU, gprT1, _Is_);
		xSUB(gprT1b, _Imm15_);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opISUBIU);
	}
	pass3 { mVUlog("ISUBIU vi%02d, vi%02d, %d", _Ft_, _Fs_, _Imm15_); }
}

//------------------------------------------------------------------
// MFIR/MFP/MOVE/MR32/MTIR
//------------------------------------------------------------------

mVUop(mVU_MFIR) {
	pass1 {
		if (!_Ft_) { mVUlow.isNOP = 1; }
		analyzeVIreg1(mVU, _Is_, mVUlow.VI_read[0]);
		analyzeReg2  (mVU, _Ft_, mVUlow.VF_write, 1);
	}
	pass2 {
		const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		mVUallocVIa(mVU, gprT1, _Is_, true);
		xMOVDZX(Ft, gprT1);
		if (!_XYZW_SS) { mVUunpack_xyzw(Ft, Ft, 0); }
		mVU.regAlloc->clearNeeded(Ft);
		mVU.profiler.EmitOp(opMFIR);
	}
	pass3 { mVUlog("MFIR.%s vf%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MFP) {
	pass1 { mVUanalyzeMFP(mVU, _Ft_); }
	pass2 {
		const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		getPreg(mVU, Ft);
		mVU.regAlloc->clearNeeded(Ft);
		mVU.profiler.EmitOp(opMFP);
	}
	pass3 { mVUlog("MFP.%s vf%02d, P", _XYZW_String, _Ft_); }
}

mVUop(mVU_MOVE) {
	pass1 { mVUanalyzeMOVE(mVU, _Fs_, _Ft_); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, _Ft_, _X_Y_Z_W);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opMOVE);
	}
	pass3 { mVUlog("MOVE.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MR32) {
	pass1 { mVUanalyzeMR32(mVU, _Fs_, _Ft_); }
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_);
		const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		if (_XYZW_SS) mVUunpack_xyzw(Ft, Fs, (_X ? 1 : (_Y ? 2 : (_Z ? 3 : 0))));
		else		  xPSHUF.D(Ft, Fs, 0x39);
		mVU.regAlloc->clearNeeded(Ft);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opMR32);
	}
	pass3 { mVUlog("MR32.%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
}

mVUop(mVU_MTIR) {
	pass1 {
		if (!_It_) mVUlow.isNOP = 1;
		analyzeReg5  (mVU, _Fs_, _Fsf_, mVUlow.VF_read[0]);
		analyzeVIreg2(mVU, _It_, mVUlow.VI_write, 1);
	}
	pass2 {
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
		xMOVD(gprT1, Fs);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opMTIR);
	}
	pass3 { mVUlog("MTIR vi%02d, vf%02d%s", _Ft_, _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// ILW/ILWR
//------------------------------------------------------------------

mVUop(mVU_ILW) {
	pass1 {
		if (!_It_) mVUlow.isNOP = 1;
		analyzeVIreg1(mVU, _Is_, mVUlow.VI_read[0]);
		analyzeVIreg2(mVU, _It_, mVUlow.VI_write, 4);
	}
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem + offsetSS);
		if (_Is_) {
			mVUallocVIa(mVU, gprT2, _Is_);
			xADD(gprT2, _Imm11_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		else
			ptr += getVUmem(_Imm11_);
		xMOVZX(gprT1, ptr16[ptr]);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opILW);
	}
	pass3 { mVUlog("ILW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

mVUop(mVU_ILWR) {
	pass1 {
		if (!_It_) mVUlow.isNOP = 1;
		analyzeVIreg1(mVU, _Is_, mVUlow.VI_read[0]);
		analyzeVIreg2(mVU, _It_, mVUlow.VI_write, 4);
	}
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem + offsetSS);
		if (_Is_) {
			mVUallocVIa(mVU, gprT2, _Is_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		xMOVZX(gprT1, ptr16[ptr]);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opILWR);
	}
	pass3 { mVUlog("ILWR.%s vi%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// ISW/ISWR
//------------------------------------------------------------------

mVUop(mVU_ISW) {
	pass1 {
		analyzeVIreg1(mVU, _Is_, mVUlow.VI_read[0]);
		analyzeVIreg1(mVU, _It_, mVUlow.VI_read[1]);
	}
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_Is_) {
			mVUallocVIa(mVU, gprT2, _Is_);
			xADD(gprT2, _Imm11_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		else
			ptr += getVUmem(_Imm11_);
		mVUallocVIa(mVU, gprT1, _It_);
		if (_X) xMOV(ptr32[ptr], gprT1);
		if (_Y) xMOV(ptr32[ptr+4], gprT1);
		if (_Z) xMOV(ptr32[ptr+8], gprT1);
		if (_W) xMOV(ptr32[ptr+12], gprT1);
		mVU.profiler.EmitOp(opISW);
	}
	pass3 { mVUlog("ISW.%s vi%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_);  }
}

mVUop(mVU_ISWR) {
	pass1 {
		analyzeVIreg1(mVU, _Is_, mVUlow.VI_read[0]);
		analyzeVIreg1(mVU, _It_, mVUlow.VI_read[1]); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_Is_) {
			mVUallocVIa(mVU, gprT2, _Is_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		mVUallocVIa(mVU, gprT1, _It_);
		if (_X) xMOV(ptr32[ptr], gprT1);
		if (_Y) xMOV(ptr32[ptr+4], gprT1);
		if (_Z) xMOV(ptr32[ptr+8], gprT1);
		if (_W) xMOV(ptr32[ptr+12], gprT1);
		mVU.profiler.EmitOp(opISWR);
	}
	pass3 { mVUlog("ISWR.%s vi%02d, vi%02d", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// LQ/LQD/LQI
//------------------------------------------------------------------

mVUop(mVU_LQ) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 0); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_Is_) {
			mVUallocVIa(mVU, gprT2, _Is_);
			xADD(gprT2, _Imm11_);
			mVUaddrFix(mVU, gprT2);
			ptr += gprT2;
		}
		else
			ptr += getVUmem(_Imm11_);
		const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		mVUloadReg(Ft, ptr, _X_Y_Z_W);
		mVU.regAlloc->clearNeeded(Ft);
		mVU.profiler.EmitOp(opLQ);
	}
	pass3 { mVUlog("LQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Ft_, _Fs_, _Imm11_); }
}

mVUop(mVU_LQD) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 1); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_Is_ || isVU0) { // Access VU1 regs mem-map in !_Is_ case
			mVUallocVIa(mVU, gprT2, _Is_);
			xSUB(gprT2b, 1);
			if (_Is_) mVUallocVIb(mVU, gprT2, _Is_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		else ptr += (0xffff & (mVU.microMemSize-8));
		if (!mVUlow.noWriteVF) {
			const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
			mVUloadReg(Ft, ptr, _X_Y_Z_W);
			mVU.regAlloc->clearNeeded(Ft);
		}
		mVU.profiler.EmitOp(opLQD);
	}
	pass3 { mVUlog("LQD.%s vf%02d, --vi%02d", _XYZW_String, _Ft_, _Is_); }
}

mVUop(mVU_LQI) {
	pass1 { mVUanalyzeLQ(mVU, _Ft_, _Is_, 1); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_Is_) {
			mVUallocVIa(mVU, gprT1, _Is_);
			xMOV(gprT2, gprT1);
			xADD(gprT1b, 1);
			mVUallocVIb(mVU, gprT1, _Is_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		if (!mVUlow.noWriteVF) {
			const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
			mVUloadReg(Ft, ptr, _X_Y_Z_W);
			mVU.regAlloc->clearNeeded(Ft);
		}
		mVU.profiler.EmitOp(opLQI);
	}
	pass3 { mVUlog("LQI.%s vf%02d, vi%02d++", _XYZW_String, _Ft_, _Fs_); }
}

//------------------------------------------------------------------
// SQ/SQD/SQI
//------------------------------------------------------------------

mVUop(mVU_SQ) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 0); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_It_) {
			mVUallocVIa(mVU, gprT2, _It_);
			xADD(gprT2, _Imm11_);
			mVUaddrFix(mVU, gprT2);
			ptr += gprT2;
		}
		else
			ptr += getVUmem(_Imm11_);
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		mVUsaveReg(Fs, ptr, _X_Y_Z_W, 1);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opSQ);
	}
	pass3 { mVUlog("SQ.%s vf%02d, vi%02d + %d", _XYZW_String, _Fs_, _Ft_, _Imm11_); }
}

mVUop(mVU_SQD) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 1); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_It_ || isVU0) {// Access VU1 regs mem-map in !_It_ case
			mVUallocVIa(mVU, gprT2, _It_);
			xSUB(gprT2b, 1);
			if (_It_) mVUallocVIb(mVU, gprT2, _It_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		else ptr += (0xffff & (mVU.microMemSize-8));
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		mVUsaveReg(Fs, ptr, _X_Y_Z_W, 1);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opSQD);
	}
	pass3 { mVUlog("SQD.%s vf%02d, --vi%02d", _XYZW_String, _Fs_, _Ft_); }
}

mVUop(mVU_SQI) {
	pass1 { mVUanalyzeSQ(mVU, _Fs_, _It_, 1); }
	pass2 {
		xAddressVoid ptr(mVU.regs().Mem);
		if (_It_) {
			mVUallocVIa(mVU, gprT1, _It_);
			xMOV(gprT2, gprT1);
			xADD(gprT1b, 1);
			mVUallocVIb(mVU, gprT1, _It_);
			mVUaddrFix (mVU, gprT2);
			ptr += gprT2;
		}
		const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		mVUsaveReg(Fs, ptr, _X_Y_Z_W, 1);
		mVU.regAlloc->clearNeeded(Fs);
		mVU.profiler.EmitOp(opSQI);
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
			const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
			xMOVD(gprT1, Fs);
			xAND(gprT1, 0x007fffff);
			xOR (gprT1, 0x3f800000);
			xMOV(ptr32[Rmem], gprT1);
			mVU.regAlloc->clearNeeded(Fs);
		}
		else xMOV(ptr32[Rmem], 0x3f800000);
		mVU.profiler.EmitOp(opRINIT);
	}
	pass3 { mVUlog("RINIT R, vf%02d%s", _Fs_, _Fsf_String); }
}

static __fi void mVU_RGET_(mV, const x32& Rreg) {
	if (!mVUlow.noWriteVF) {
		const xmm& Ft = mVU.regAlloc->allocReg(-1, _Ft_, _X_Y_Z_W);
		xMOVDZX(Ft, Rreg);
		if (!_XYZW_SS) mVUunpack_xyzw(Ft, Ft, 0);
		mVU.regAlloc->clearNeeded(Ft);
	}
}

mVUop(mVU_RGET) {
	pass1 { mVUanalyzeR2(mVU, _Ft_, 1); }
	pass2 {
		xMOV(gprT1, ptr32[Rmem]);
		mVU_RGET_(mVU, gprT1);
		mVU.profiler.EmitOp(opRGET);
	}
	pass3 { mVUlog("RGET.%s vf%02d, R", _XYZW_String, _Ft_); }
}

mVUop(mVU_RNEXT) {
	pass1 { mVUanalyzeR2(mVU, _Ft_, 0); }
	pass2 {
		// algorithm from www.project-fao.org
		xMOV(gprT3, ptr32[Rmem]);
		xMOV(gprT1, gprT3);
		xSHR(gprT1, 4);
		xAND(gprT1, 1);

		xMOV(gprT2, gprT3);
		xSHR(gprT2, 22);
		xAND(gprT2, 1);

		xSHL(gprT3, 1);
		xXOR(gprT1, gprT2);
		xXOR(gprT3, gprT1);
		xAND(gprT3, 0x007fffff);
		xOR (gprT3, 0x3f800000);
		xMOV(ptr32[Rmem], gprT3);
		mVU_RGET_(mVU,  gprT3);
		mVU.profiler.EmitOp(opRNEXT);
	}
	pass3 { mVUlog("RNEXT.%s vf%02d, R", _XYZW_String, _Ft_); }
}

mVUop(mVU_RXOR) {
	pass1 { mVUanalyzeR1(mVU, _Fs_, _Fsf_); }
	pass2 {
		if (_Fs_ || (_Fsf_ == 3)) {
			const xmm& Fs = mVU.regAlloc->allocReg(_Fs_, 0, (1 << (3 - _Fsf_)));
			xMOVD(gprT1, Fs);
			xAND(gprT1, 0x7fffff);
			xXOR(ptr32[Rmem],  gprT1);
			mVU.regAlloc->clearNeeded(Fs);
		}
		mVU.profiler.EmitOp(opRXOR);
	}
	pass3 { mVUlog("RXOR R, vf%02d%s", _Fs_, _Fsf_String); }
}

//------------------------------------------------------------------
// WaitP/WaitQ
//------------------------------------------------------------------

mVUop(mVU_WAITP) {
	pass1 { mVUstall = max(mVUstall, (u8)((mVUregs.p) ? (mVUregs.p - 1) : 0)); }
	pass2 { mVU.profiler.EmitOp(opWAITP); }
	pass3 { mVUlog("WAITP"); }
}

mVUop(mVU_WAITQ) {
	pass1 { mVUstall = max(mVUstall, mVUregs.q); }
	pass2 { mVU.profiler.EmitOp(opWAITQ); }
	pass3 { mVUlog("WAITQ"); }
}

//------------------------------------------------------------------
// XTOP/XITOP
//------------------------------------------------------------------

mVUop(mVU_XTOP) {
	pass1 {
		if (!_It_) mVUlow.isNOP = 1;
		analyzeVIreg2(mVU, _It_, mVUlow.VI_write, 1);
	}
	pass2 {
		xMOVZX(gprT1, ptr16[&mVU.getVifRegs().top]);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opXTOP);
	}
	pass3 { mVUlog("XTOP vi%02d", _Ft_); }
}

mVUop(mVU_XITOP) {
	pass1 {
		if (!_It_) mVUlow.isNOP = 1;
		analyzeVIreg2(mVU, _It_, mVUlow.VI_write, 1); }
	pass2 {
		xMOVZX(gprT1, ptr16[&mVU.getVifRegs().itop]);
		xAND  (gprT1, isVU1 ? 0x3ff : 0xff);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opXITOP);
	}
	pass3 { mVUlog("XITOP vi%02d", _Ft_); }
}

//------------------------------------------------------------------
// XGkick
//------------------------------------------------------------------

void __fastcall mVU_XGKICK_(u32 addr) {
	addr = (addr & 0x3ff) * 16;
	u32 diff = 0x4000 - addr;
	u32 size = gifUnit.GetGSPacketSize(GIF_PATH_1, vuRegs[1].Mem, addr);

	if (size > diff) {
		//DevCon.WriteLn(Color_Green, "microVU1: XGkick Wrap!");
		gifUnit.gifPath[GIF_PATH_1].CopyGSPacketData(  &vuRegs[1].Mem[addr],  diff,true);
		gifUnit.TransferGSPacketData(GIF_TRANS_XGKICK, &vuRegs[1].Mem[0],size-diff,true);
	}
	else {
		gifUnit.TransferGSPacketData(GIF_TRANS_XGKICK, &vuRegs[1].Mem[addr], size, true);
	}
}

static __fi void mVU_XGKICK_DELAY(mV, bool memVI) {
	mVUbackupRegs(mVU);
#if 0 // XGkick Break - ToDo: Change "SomeGifPathValue" to w/e needs to be tested
	xTEST (ptr32[&SomeGifPathValue], 1); // If '1', breaks execution
	xMOV  (ptr32[&mVU.resumePtrXG], (uptr)xGetPtr() + 10 + 6);
	xJcc32(Jcc_NotZero, (uptr)mVU.exitFunctXG - ((uptr)xGetPtr()+6));
#endif
	if (memVI)	xMOV(gprT2, ptr32[&mVU.VIxgkick]);
	else		mVUallocVIa(mVU, gprT2, _Is_);
	xCALL(mVU_XGKICK_);
	mVUrestoreRegs(mVU);
}

mVUop(mVU_XGKICK) {
	pass1 { mVUanalyzeXGkick(mVU, _Is_, mVU_XGKICK_CYCLES); }
	pass2 {
		if (!mVU_XGKICK_CYCLES)	{ mVU_XGKICK_DELAY(mVU, 0); return; }
		elif (mVUinfo.doXGKICK)	{ mVU_XGKICK_DELAY(mVU, 1); mVUinfo.doXGKICK = 0; }
		mVUallocVIa(mVU, gprT1, _Is_);
		xMOV(ptr32[&mVU.VIxgkick], gprT1);
		mVU.profiler.EmitOp(opXGKICK);
	}
	pass3 { mVUlog("XGKICK vi%02d", _Fs_); }
}

//------------------------------------------------------------------
// Branches/Jumps
//------------------------------------------------------------------

void setBranchA(mP, int x, int _x_) {
	pass1 {
		if (_Imm11_ == 1 && !_x_) {
			DevCon.WriteLn(Color_Green, "microVU%d: Branch Optimization", mVU.index);
			mVUlow.isNOP = 1;
			return;
		}
		mVUbranch	  = x;
		mVUlow.branch = x;
	}
	pass2 { if (_Imm11_ == 1 && !_x_) { return; } mVUbranch = x; }
	pass3 { mVUbranch = x; }
	pass4 { if (_Imm11_ == 1 && !_x_) { return; } mVUbranch = x; }
}

void condEvilBranch(mV, int JMPcc) {
	if (mVUlow.badBranch) {
		xMOV(ptr32[&mVU.branch], gprT1);
		xMOV(ptr32[&mVU.badBranch], branchAddrN);
		xCMP(gprT1b, 0);
		xForwardJump8 cJMP((JccComparisonType)JMPcc);
			incPC(6); // Branch Not Taken Addr + 8
			xMOV(ptr32[&mVU.badBranch], xPC);
			incPC(-6);
		cJMP.SetTarget();
		return;
	}
	xMOV(ptr32[&mVU.evilBranch], branchAddr);
	xCMP(gprT1b, 0);
	xForwardJump8 cJMP((JccComparisonType)JMPcc);
		xMOV(gprT1, ptr32[&mVU.badBranch]); // Branch Not Taken
		xMOV(ptr32[&mVU.evilBranch], gprT1);
	cJMP.SetTarget();
}

mVUop(mVU_B) {
	setBranchA(mX, 1, 0);
	pass1 { mVUanalyzeNormBranch(mVU, 0, 0); }
	pass2 {
		if (mVUlow.badBranch)  { xMOV(ptr32[&mVU.badBranch],  branchAddrN); }
		if (mVUlow.evilBranch) { xMOV(ptr32[&mVU.evilBranch], branchAddr); }
		mVU.profiler.EmitOp(opB);
	}
	pass3 { mVUlog("B [<a href=\"#addr%04x\">%04x</a>]", branchAddr, branchAddr); }
}

mVUop(mVU_BAL) {
	setBranchA(mX, 2, _It_);
	pass1 { mVUanalyzeNormBranch(mVU, _It_, 1); }
	pass2 {
		xMOV(gprT1, bSaveAddr);
		mVUallocVIb(mVU, gprT1, _It_);
		if (mVUlow.badBranch)  { xMOV(ptr32[&mVU.badBranch],  branchAddrN); }
		if (mVUlow.evilBranch) { xMOV(ptr32[&mVU.evilBranch], branchAddr); }
		mVU.profiler.EmitOp(opBAL);
	}
	pass3 { mVUlog("BAL vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, branchAddr, branchAddr); }
}

mVUop(mVU_IBEQ) {
	setBranchA(mX, 3, 0);
	pass1 { mVUanalyzeCondBranch2(mVU, _Is_, _It_); }
	pass2 {
		if (mVUlow.memReadIs) xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else mVUallocVIa(mVU, gprT1, _Is_);
		
		if (mVUlow.memReadIt) xXOR(gprT1, ptr32[&mVU.VIbackup]);
		else { mVUallocVIa(mVU, gprT2, _It_); xXOR(gprT1, gprT2); }

		if (!(isBadOrEvil))	xMOV(ptr32[&mVU.branch], gprT1);
		else				condEvilBranch(mVU, Jcc_Equal);
		mVU.profiler.EmitOp(opIBEQ);
	}
	pass3 { mVUlog("IBEQ vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBGEZ) {
	setBranchA(mX, 4, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		xMOV(ptr32[&mVU.branch], gprT1);
		else					condEvilBranch(mVU, Jcc_GreaterOrEqual);
		mVU.profiler.EmitOp(opIBGEZ);
	}
	pass3 { mVUlog("IBGEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBGTZ) {
	setBranchA(mX, 5, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		xMOV(ptr32[&mVU.branch], gprT1);
		else					condEvilBranch(mVU, Jcc_Greater);
		mVU.profiler.EmitOp(opIBGTZ);
	}
	pass3 { mVUlog("IBGTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBLEZ) {
	setBranchA(mX, 6, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {
		if (mVUlow.memReadIs)	xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		xMOV(ptr32[&mVU.branch], gprT1);
		else					condEvilBranch(mVU, Jcc_LessOrEqual);
		mVU.profiler.EmitOp(opIBLEZ);
	}
	pass3 { mVUlog("IBLEZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBLTZ) {
	setBranchA(mX, 7, 0);
	pass1 { mVUanalyzeCondBranch1(mVU, _Is_); }
	pass2 {	
		if (mVUlow.memReadIs)	xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else					mVUallocVIa(mVU, gprT1, _Is_);
		if (!(isBadOrEvil))		xMOV(ptr32[&mVU.branch], gprT1);
		else					condEvilBranch(mVU, Jcc_Less);
		mVU.profiler.EmitOp(opIBLTZ);
	}
	pass3 { mVUlog("IBLTZ vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Fs_, branchAddr, branchAddr); }
}

mVUop(mVU_IBNE) {
	setBranchA(mX, 8, 0);
	pass1 { mVUanalyzeCondBranch2(mVU, _Is_, _It_); }
	pass2 {
		if (mVUlow.memReadIs) xMOV(gprT1, ptr32[&mVU.VIbackup]);
		else mVUallocVIa(mVU, gprT1, _Is_);
		
		if (mVUlow.memReadIt) xXOR(gprT1, ptr32[&mVU.VIbackup]);
		else { mVUallocVIa(mVU, gprT2, _It_); xXOR(gprT1, gprT2); }
		
		if (!(isBadOrEvil))	xMOV(ptr32[&mVU.branch], gprT1);
		else				condEvilBranch(mVU, Jcc_NotEqual);
		mVU.profiler.EmitOp(opIBNE);
	}
	pass3 { mVUlog("IBNE vi%02d, vi%02d [<a href=\"#addr%04x\">%04x</a>]", _Ft_, _Fs_, branchAddr, branchAddr); }
}

void normJumpPass2(mV) {
	if (!mVUlow.constJump.isValid || mVUlow.evilBranch) {
		mVUallocVIa(mVU, gprT1, _Is_);
		xSHL(gprT1, 3);
		xAND(gprT1, mVU.microMemSize - 8);
		if (!mVUlow.evilBranch) xMOV(ptr32[&mVU.branch],	 gprT1);
		else					xMOV(ptr32[&mVU.evilBranch], gprT1);
		if (mVUlow.badBranch) {
			xADD(gprT1, 8);
			xAND(gprT1, mVU.microMemSize - 8);
			xMOV(ptr32[&mVU.badBranch],  gprT1);
		}
	}
}

mVUop(mVU_JR) {
	mVUbranch = 9;
	pass1 { mVUanalyzeJump(mVU, _Is_, 0, 0); }
	pass2 { normJumpPass2(mVU); mVU.profiler.EmitOp(opJR); }
	pass3 { mVUlog("JR [vi%02d]", _Fs_); }
}

mVUop(mVU_JALR) {
	mVUbranch = 10;
	pass1 { mVUanalyzeJump(mVU, _Is_, _It_, 1); }
	pass2 {
		normJumpPass2(mVU);
		xMOV(gprT1, bSaveAddr);
		mVUallocVIb(mVU, gprT1, _It_);
		mVU.profiler.EmitOp(opJALR);
	}
	pass3 { mVUlog("JALR vi%02d, [vi%02d]", _Ft_, _Fs_); }
}
