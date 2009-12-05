/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
// mVUupdateFlags() - Updates status/mac flags
//------------------------------------------------------------------

#define AND_XYZW			((_XYZW_SS && modXYZW) ? (1) : (mFLAG.doFlag ? (_X_Y_Z_W) : (flipMask[_X_Y_Z_W])))
#define ADD_XYZW			((_XYZW_SS && modXYZW) ? (_X ? 3 : (_Y ? 2 : (_Z ? 1 : 0))) : 0)
#define SHIFT_XYZW(gprReg)	{ if (_XYZW_SS && modXYZW && !_W) { SHL32ItoR(gprReg, ADD_XYZW); } }

// Note: If modXYZW is true, then it adjusts XYZW for Single Scalar operations
microVUt(void) mVUupdateFlags(mV, int reg, int regT1 = -1, int regT2 = -1, bool modXYZW = 1) {
	int sReg, mReg = gprT1, regT1b = 0, regT2b = 0;
	//int xyzw = _X_Y_Z_W;	// unused local, still needed? -- air
	static const u16 flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	//SysPrintf("Status = %d; Mac = %d\n", sFLAG.doFlag, mFLAG.doFlag);
	if (!sFLAG.doFlag && !mFLAG.doFlag) { return; }
	if ((mFLAG.doFlag && !(_XYZW_SS && modXYZW))) {
		if (regT2 < 0) { regT2 = mVU->regAlloc->allocReg(); regT2b = 1; }
		SSE2_PSHUFD_XMM_to_XMM(regT2, reg, 0x1B); // Flip wzyx to xyzw
	}
	else regT2 = reg;
	if (sFLAG.doFlag) {
		getFlagReg(sReg, sFLAG.write); // Set sReg to valid GPR by Cur Flag Instance
		mVUallocSFLAGa(sReg, sFLAG.lastWrite); // Get Prev Status Flag
		if (sFLAG.doNonSticky) AND32ItoR(sReg, 0xfffc00ff); // Clear O,U,S,Z flags
	}
	if (regT1 < 0) { regT1 = mVU->regAlloc->allocReg(); regT1b = 1; }

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM   (regT1, regT1); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM (regT1, regT2); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(gprT2, regT1); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM  (regT1, regT2); // Used for Sign Flag Calculation
	SSE_MOVMSKPS_XMM_to_R32(mReg,  regT1); // Move the Sign Bits of the t1reg

	AND32ItoR(mReg, AND_XYZW);	// Grab "Is Signed" bits from the previous calculation
	SHL32ItoR(mReg, 4 + ADD_XYZW);

	//-------------------------Check for Zero flags------------------------------

	AND32ItoR(gprT2, AND_XYZW);	// Grab "Is Zero" bits from the previous calculation
	if (mFLAG.doFlag) { SHIFT_XYZW(gprT2); }
	OR32RtoR(mReg, gprT2);

	//-------------------------Write back flags------------------------------

	if (mFLAG.doFlag) mVUallocMFLAGb(mVU, mReg, mFLAG.write); // Set Mac Flag
	if (sFLAG.doFlag) {
		OR32RtoR (sReg, mReg);
		if (sFLAG.doNonSticky) {
			SHL32ItoR(mReg, 8);
			OR32RtoR (sReg, mReg);
		}
	}
	if (regT1b) mVU->regAlloc->clearNeeded(regT1);
	if (regT2b) mVU->regAlloc->clearNeeded(regT2);
}

//------------------------------------------------------------------
// Helper Macros and Functions
//------------------------------------------------------------------

static void (*SSE_PS[]) (microVU*, int, int, int, int) = {
	SSE_ADDPS, // 0
	SSE_SUBPS, // 1
	SSE_MULPS, // 2
	SSE_MAXPS, // 3
	SSE_MINPS, // 4
	SSE_ADD2PS // 5
};

static void (*SSE_SS[]) (microVU*, int, int, int, int) = {
	SSE_ADDSS, // 0
	SSE_SUBSS, // 1
	SSE_MULSS, // 2
	SSE_MAXSS, // 3
	SSE_MINSS, // 4
	SSE_ADD2SS // 5
};

enum clampModes {
	cFt  = 0x01, // Clamp Ft / I-reg / Q-reg
	cFs  = 0x02, // Clamp Fs
	cACC = 0x04, // Clamp ACC
};

// Prints Opcode to MicroProgram Logs
void mVU_printOP(microVU* mVU, int opCase, const char* opName, bool isACC) {
	mVUlog(opName);
	opCase1 { if (isACC) { mVUlogACC(); } else { mVUlogFd(); } mVUlogFt(); }
	opCase2 { if (isACC) { mVUlogACC(); } else { mVUlogFd(); } mVUlogBC(); }
	opCase3 { if (isACC) { mVUlogACC(); } else { mVUlogFd(); } mVUlogI();  }
	opCase4 { if (isACC) { mVUlogACC(); } else { mVUlogFd(); } mVUlogQ();  }
}

// Sets Up Pass1 Info for Normal, BC, I, and Q Cases
void setupPass1(microVU* mVU, int opCase, bool isACC, bool noFlagUpdate) {
	opCase1 { mVUanalyzeFMAC1(mVU, ((isACC) ? 0 : _Fd_), _Fs_, _Ft_); }
	opCase2 { mVUanalyzeFMAC3(mVU, ((isACC) ? 0 : _Fd_), _Fs_, _Ft_); }
	opCase3 { mVUanalyzeFMAC1(mVU, ((isACC) ? 0 : _Fd_), _Fs_, 0); }
	opCase4 { mVUanalyzeFMAC1(mVU, ((isACC) ? 0 : _Fd_), _Fs_, 0); }
	if (noFlagUpdate) { sFLAG.doFlag = 0; }
}

// Safer to force 0 as the result for X minus X than to do actual subtraction
bool doSafeSub(microVU* mVU, int opCase, int opType, bool isACC) {
	opCase1 {
		if ((opType == 1) && (_Ft_ == _Fs_)) {
			int Fs = mVU->regAlloc->allocReg(-1, isACC ? 32 : _Fd_, _X_Y_Z_W);
			SSE2_PXOR_XMM_to_XMM(Fs, Fs); // Set to Positive 0
			mVUupdateFlags(mVU, Fs, -1);
			mVU->regAlloc->clearNeeded(Fs);
			return 1;
		}
	}
	return 0;
}

// Sets Up Ft Reg for Normal, BC, I, and Q Cases
void setupFtReg(microVU* mVU, int& Ft, int& tempFt, int opCase) {
	opCase1 {
		if (_XYZW_SS2)   { Ft = mVU->regAlloc->allocReg(_Ft_, 0, _X_Y_Z_W);	tempFt = Ft; }
		else if (clampE) { Ft = mVU->regAlloc->allocReg(_Ft_, 0, 0xf);		tempFt = Ft; }
		else			 { Ft = mVU->regAlloc->allocReg(_Ft_);				tempFt = -1; }
	}
	opCase2 {
		tempFt = mVU->regAlloc->allocReg(_Ft_);
		Ft	   = mVU->regAlloc->allocReg();
		mVUunpack_xyzw(Ft, tempFt, _bc_);
		mVU->regAlloc->clearNeeded(tempFt);
		tempFt = Ft;
	}
	opCase3 { Ft = mVU->regAlloc->allocReg(33, 0, _X_Y_Z_W); tempFt = Ft; }
	opCase4 {
		if (!clampE && _XYZW_SS && !mVUinfo.readQ) { Ft = xmmPQ; tempFt = -1; }
		else { Ft = mVU->regAlloc->allocReg(); tempFt = Ft; getQreg(Ft, mVUinfo.readQ); }
	}
}

// Normal FMAC Opcodes
void mVU_FMACa(microVU* mVU, int recPass, int opCase, int opType, bool isACC, const char* opName, int clampType) {
	pass1 { setupPass1(mVU, opCase, isACC, ((opType == 3) || (opType == 4))); }
	pass2 {
		if (doSafeSub(mVU, opCase, opType, isACC)) return;
		
		int Fs, Ft, ACC, tempFt;
		setupFtReg(mVU, Ft, tempFt, opCase);

		if (isACC) {
			Fs  = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
			ACC = mVU->regAlloc->allocReg((_X_Y_Z_W == 0xf) ? -1 : 32, 32, 0xf, 0);
			if (_XYZW_SS2) SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W));
		}
		else { Fs = mVU->regAlloc->allocReg(_Fs_, _Fd_, _X_Y_Z_W); }

		if (clampType & cFt) mVUclamp2(mVU, Ft, -1, _X_Y_Z_W);
		if (clampType & cFs) mVUclamp2(mVU, Fs, -1, _X_Y_Z_W);

		if (_XYZW_SS) SSE_SS[opType](mVU, Fs, Ft, -1, -1);
		else		  SSE_PS[opType](mVU, Fs, Ft, -1, -1);

		if (isACC) {
			if (_XYZW_SS) SSE_MOVSS_XMM_to_XMM(ACC, Fs);
			else		  mVUmergeRegs(ACC, Fs, _X_Y_Z_W);
			mVUupdateFlags(mVU, ACC, Fs, tempFt);
			if (_XYZW_SS2) SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W));
			mVU->regAlloc->clearNeeded(ACC);
		}
		else mVUupdateFlags(mVU, Fs, tempFt);

		mVU->regAlloc->clearNeeded(Fs); // Always Clear Written Reg First
		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVU_printOP(mVU, opCase, opName, isACC); }
	pass4 { if ((opType != 3) && (opType != 4)) mVUregs.needExactMatch |= 8; }
}

// MADDA/MSUBA Opcodes
void mVU_FMACb(microVU* mVU, int recPass, int opCase, int opType, const char* opName, int clampType) {
	pass1 { setupPass1(mVU, opCase, 1, 0); }
	pass2 {
		int Fs, Ft, ACC, tempFt;
		setupFtReg(mVU, Ft, tempFt, opCase);

		Fs  = mVU->regAlloc->allocReg(_Fs_, 0, _X_Y_Z_W);
		ACC = mVU->regAlloc->allocReg(32, 32, 0xf, 0);

		if (_XYZW_SS2) { SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W)); }

		if (clampType & cFt) mVUclamp2(mVU, Ft, -1, _X_Y_Z_W);
		if (clampType & cFs) mVUclamp2(mVU, Fs, -1, _X_Y_Z_W);

		if (_XYZW_SS) SSE_SS[2](mVU, Fs, Ft, -1, -1);
		else		  SSE_PS[2](mVU, Fs, Ft, -1, -1);

		if (_XYZW_SS || _X_Y_Z_W == 0xf) {
			if (_XYZW_SS) SSE_SS[opType](mVU, ACC, Fs, tempFt, -1);
			else		  SSE_PS[opType](mVU, ACC, Fs, tempFt, -1);
			mVUupdateFlags(mVU, ACC, Fs, tempFt);
			if (_XYZW_SS && _X_Y_Z_W != 8) SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W));
		}
		else {
			int tempACC = mVU->regAlloc->allocReg();
			SSE_MOVAPS_XMM_to_XMM(tempACC, ACC);
			SSE_PS[opType](mVU, tempACC, Fs, tempFt, -1);
			mVUmergeRegs(ACC, tempACC, _X_Y_Z_W);
			mVUupdateFlags(mVU, ACC, Fs, tempFt);
			mVU->regAlloc->clearNeeded(tempACC);
		}

		mVU->regAlloc->clearNeeded(ACC);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(Ft);
	}
	pass3 { mVU_printOP(mVU, opCase, opName, 1); }
	pass4 { mVUregs.needExactMatch |= 8; }
}

// MADD Opcodes
void mVU_FMACc(microVU* mVU, int recPass, int opCase, const char* opName, int clampType) {
	pass1 { setupPass1(mVU, opCase, 0, 0); }
	pass2 {
		int Fs, Ft, ACC, tempFt;
		setupFtReg(mVU, Ft, tempFt, opCase);

		ACC = mVU->regAlloc->allocReg(32);
		Fs  = mVU->regAlloc->allocReg(_Fs_, _Fd_, _X_Y_Z_W);

		if (_XYZW_SS2) { SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W)); }

		if (clampType & cFt)  mVUclamp2(mVU, Ft,  -1, _X_Y_Z_W);
		if (clampType & cFs)  mVUclamp2(mVU, Fs,  -1, _X_Y_Z_W);
		if (clampType & cACC) mVUclamp2(mVU, ACC, -1, _X_Y_Z_W);

		if (_XYZW_SS) { SSE_SS[2](mVU, Fs, Ft, -1, -1); SSE_SS[0](mVU, Fs, ACC, tempFt, -1); }
		else		  { SSE_PS[2](mVU, Fs, Ft, -1, -1); SSE_PS[0](mVU, Fs, ACC, tempFt, -1); }

		if (_XYZW_SS2) { SSE2_PSHUFD_XMM_to_XMM(ACC, ACC, shuffleSS(_X_Y_Z_W)); }

		mVUupdateFlags(mVU, Fs, tempFt);

		mVU->regAlloc->clearNeeded(Fs); // Always Clear Written Reg First
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(ACC);
	}
	pass3 { mVU_printOP(mVU, opCase, opName, 0); }
	pass4 { mVUregs.needExactMatch |= 8; }
}

// MSUB Opcodes
void mVU_FMACd(microVU* mVU, int recPass, int opCase, const char* opName, int clampType) {
	pass1 { setupPass1(mVU, opCase, 0, 0); }
	pass2 {
		int Fs, Ft, Fd, tempFt;
		setupFtReg(mVU, Ft, tempFt, opCase);

		Fs = mVU->regAlloc->allocReg(_Fs_,  0, _X_Y_Z_W);
		Fd = mVU->regAlloc->allocReg(32, _Fd_, _X_Y_Z_W);

		if (clampType & cFt)  mVUclamp2(mVU, Ft, -1, _X_Y_Z_W);
		if (clampType & cFs)  mVUclamp2(mVU, Fs, -1, _X_Y_Z_W);
		if (clampType & cACC) mVUclamp2(mVU, Fd, -1, _X_Y_Z_W);

		if (_XYZW_SS) { SSE_SS[2](mVU, Fs, Ft, -1, -1); SSE_SS[1](mVU, Fd, Fs, tempFt, -1); }
		else		  { SSE_PS[2](mVU, Fs, Ft, -1, -1); SSE_PS[1](mVU, Fd, Fs, tempFt, -1); }

		mVUupdateFlags(mVU, Fd, Fs, tempFt);

		mVU->regAlloc->clearNeeded(Fd); // Always Clear Written Reg First
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVU_printOP(mVU, opCase, opName, 0); }
	pass4 { mVUregs.needExactMatch |= 8; }
}

// ABS Opcode
mVUop(mVU_ABS) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 {
		if (!_Ft_) return;
		int Fs = mVU->regAlloc->allocReg(_Fs_, _Ft_, _X_Y_Z_W, !((_Fs_ == _Ft_) && (_X_Y_Z_W == 0xf)));
		SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVUglob.absclip);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("ABS"); mVUlogFtFs(); }
}

// OPMULA Opcode
mVUop(mVU_OPMULA) {
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, _Ft_); }
	pass2 {
		int Ft = mVU->regAlloc->allocReg(_Ft_,  0, _X_Y_Z_W);
		int Fs = mVU->regAlloc->allocReg(_Fs_, 32, _X_Y_Z_W);

		SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
		SSE2_PSHUFD_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
		SSE_MULPS(mVU, Fs, Ft);
		mVU->regAlloc->clearNeeded(Ft);
		mVUupdateFlags(mVU, Fs);
		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog("OPMULA"); mVUlogACC(); mVUlogFt(); }
	pass4 { mVUregs.needExactMatch |= 8; }
}

// OPMSUB Opcode
mVUop(mVU_OPMSUB) {
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, _Ft_); }
	pass2 {
		int Ft  = mVU->regAlloc->allocReg(_Ft_, 0, 0xf);
		int Fs  = mVU->regAlloc->allocReg(_Fs_, 0, 0xf);
		int ACC = mVU->regAlloc->allocReg(32, _Fd_, _X_Y_Z_W);

		SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
		SSE2_PSHUFD_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
		SSE_MULPS(mVU, Fs,  Ft);
		SSE_SUBPS(mVU, ACC, Fs);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(Ft);
		mVUupdateFlags(mVU, ACC);
		mVU->regAlloc->clearNeeded(ACC);

	}
	pass3 { mVUlog("OPMSUB"); mVUlogFd(); mVUlogFt(); }
	pass4 { mVUregs.needExactMatch |= 8; }
}

// FTOI0/FTIO4/FTIO12/FTIO15 Opcodes
static void mVU_FTOIx(mP, uptr addr, const char* opName) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 {
		if (!_Ft_) return;
		int Fs = mVU->regAlloc->allocReg(_Fs_, _Ft_, _X_Y_Z_W, !((_Fs_ == _Ft_) && (_X_Y_Z_W == 0xf)));
		int t1 = mVU->regAlloc->allocReg();
		int t2 = mVU->regAlloc->allocReg();

		// Note: For help understanding this algorithm see recVUMI_FTOI_Saturate()
		SSE_MOVAPS_XMM_to_XMM(t1, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Fs, addr); }
		SSE2_CVTTPS2DQ_XMM_to_XMM(Fs, Fs);
		SSE2_PXOR_M128_to_XMM(t1, (uptr)mVUglob.signbit);
		SSE2_PSRAD_I8_to_XMM (t1, 31);
		SSE_MOVAPS_XMM_to_XMM(t2, Fs);
		SSE2_PCMPEQD_M128_to_XMM(t2, (uptr)mVUglob.signbit);
		SSE_ANDPS_XMM_to_XMM (t1, t2);
		SSE2_PADDD_XMM_to_XMM(Fs, t1);

		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	pass3 { mVUlog(opName); mVUlogFtFs(); }
}

// ITOF0/ITOF4/ITOF12/ITOF15 Opcodes
static void mVU_ITOFx(mP, uptr addr, const char* opName) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 {
		if (!_Ft_) return;
		int Fs = mVU->regAlloc->allocReg(_Fs_, _Ft_, _X_Y_Z_W, !((_Fs_ == _Ft_) && (_X_Y_Z_W == 0xf)));

		SSE2_CVTDQ2PS_XMM_to_XMM(Fs, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Fs, addr); }
		//mVUclamp2(Fs, xmmT1, 15); // Clamp (not sure if this is needed)

		mVU->regAlloc->clearNeeded(Fs);
	}
	pass3 { mVUlog(opName); mVUlogFtFs(); }
}

// Clip Opcode
mVUop(mVU_CLIP) {
	pass1 { mVUanalyzeFMAC4(mVU, _Fs_, _Ft_); }
	pass2 {
		int Fs = mVU->regAlloc->allocReg(_Fs_, 0, 0xf);
		int Ft = mVU->regAlloc->allocReg(_Ft_, 0, 0x1);
		int t1 = mVU->regAlloc->allocReg();

		mVUunpack_xyzw(Ft, Ft, 0);
		mVUallocCFLAGa(mVU, gprT1, cFLAG.lastWrite);
		SHL32ItoR(gprT1, 6);

		SSE_ANDPS_M128_to_XMM(Ft, (uptr)mVUglob.absclip);
		SSE_MOVAPS_XMM_to_XMM(t1, Ft);
		SSE_ORPS_M128_to_XMM(t1, (uptr)mVUglob.signbit);

		SSE_CMPNLEPS_XMM_to_XMM(t1, Fs); // -w, -z, -y, -x
		SSE_CMPLTPS_XMM_to_XMM(Ft, Fs);  // +w, +z, +y, +x

		SSE_MOVAPS_XMM_to_XMM(Fs, Ft);	 // Fs = +w, +z, +y, +x
		SSE_UNPCKLPS_XMM_to_XMM(Ft, t1); // Ft = -y,+y,-x,+x
		SSE_UNPCKHPS_XMM_to_XMM(Fs, t1); // Fs = -w,+w,-z,+z

		SSE_MOVMSKPS_XMM_to_R32(gprT2, Fs); // -w,+w,-z,+z
		AND32ItoR(gprT2, 0x3);
		SHL32ItoR(gprT2, 4);
		OR32RtoR (gprT1, gprT2);

		SSE_MOVMSKPS_XMM_to_R32(gprT2, Ft); // -y,+y,-x,+x
		AND32ItoR(gprT2, 0xf);
		OR32RtoR (gprT1, gprT2);
		AND32ItoR(gprT1, 0xffffff);

		mVUallocCFLAGb(mVU, gprT1, cFLAG.write);
		mVU->regAlloc->clearNeeded(Fs);
		mVU->regAlloc->clearNeeded(Ft);
		mVU->regAlloc->clearNeeded(t1);
	}
	pass3 { mVUlog("CLIP"); mVUlogCLIP(); }
}

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

mVUop(mVU_ADD)		{ mVU_FMACa(mVU, recPass, 1, 0, 0,		"ADD",    0);  }
mVUop(mVU_ADDi)		{ mVU_FMACa(mVU, recPass, 3, 5, 0,		"ADDi",   0);  }
mVUop(mVU_ADDq)		{ mVU_FMACa(mVU, recPass, 4, 0, 0,		"ADDq",   0);  }
mVUop(mVU_ADDx)		{ mVU_FMACa(mVU, recPass, 2, 0, 0,		"ADDx",   0);  }
mVUop(mVU_ADDy)		{ mVU_FMACa(mVU, recPass, 2, 0, 0,		"ADDy",   0);  }
mVUop(mVU_ADDz)		{ mVU_FMACa(mVU, recPass, 2, 0, 0,		"ADDz",   0);  }
mVUop(mVU_ADDw)		{ mVU_FMACa(mVU, recPass, 2, 0, 0,		"ADDw",   0);  }
mVUop(mVU_ADDA)		{ mVU_FMACa(mVU, recPass, 1, 0, 1,		"ADDA",   0);  }
mVUop(mVU_ADDAi)	{ mVU_FMACa(mVU, recPass, 3, 0, 1,		"ADDAi",  0);  }
mVUop(mVU_ADDAq)	{ mVU_FMACa(mVU, recPass, 4, 0, 1,		"ADDAq",  0);  }
mVUop(mVU_ADDAx)	{ mVU_FMACa(mVU, recPass, 2, 0, 1,		"ADDAx",  0);  }
mVUop(mVU_ADDAy)	{ mVU_FMACa(mVU, recPass, 2, 0, 1,		"ADDAy",  0);  }
mVUop(mVU_ADDAz)	{ mVU_FMACa(mVU, recPass, 2, 0, 1,		"ADDAz",  0);  }
mVUop(mVU_ADDAw)	{ mVU_FMACa(mVU, recPass, 2, 0, 1,		"ADDAw",  0);  }
mVUop(mVU_SUB)		{ mVU_FMACa(mVU, recPass, 1, 1, 0,		"SUB",  (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBi)		{ mVU_FMACa(mVU, recPass, 3, 1, 0,		"SUBi", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBq)		{ mVU_FMACa(mVU, recPass, 4, 1, 0,		"SUBq", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBx)		{ mVU_FMACa(mVU, recPass, 2, 1, 0,		"SUBx", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBy)		{ mVU_FMACa(mVU, recPass, 2, 1, 0,		"SUBy", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBz)		{ mVU_FMACa(mVU, recPass, 2, 1, 0,		"SUBz", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBw)		{ mVU_FMACa(mVU, recPass, 2, 1, 0,		"SUBw", (_XYZW_PS)?(cFs|cFt):0);   } // Clamp (Kingdom Hearts I (VU0))
mVUop(mVU_SUBA)		{ mVU_FMACa(mVU, recPass, 1, 1, 1,		"SUBA",   0);  }
mVUop(mVU_SUBAi)	{ mVU_FMACa(mVU, recPass, 3, 1, 1,		"SUBAi",  0);  }
mVUop(mVU_SUBAq)	{ mVU_FMACa(mVU, recPass, 4, 1, 1,		"SUBAq",  0);  }
mVUop(mVU_SUBAx)	{ mVU_FMACa(mVU, recPass, 2, 1, 1,		"SUBAx",  0);  }
mVUop(mVU_SUBAy)	{ mVU_FMACa(mVU, recPass, 2, 1, 1,		"SUBAy",  0);  }
mVUop(mVU_SUBAz)	{ mVU_FMACa(mVU, recPass, 2, 1, 1,		"SUBAz",  0);  }
mVUop(mVU_SUBAw)	{ mVU_FMACa(mVU, recPass, 2, 1, 1,		"SUBAw",  0);  }
mVUop(mVU_MUL)		{ mVU_FMACa(mVU, recPass, 1, 2, 0,		"MUL",  (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULi)		{ mVU_FMACa(mVU, recPass, 3, 2, 0,		"MULi", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULq)		{ mVU_FMACa(mVU, recPass, 4, 2, 0,		"MULq", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULx)		{ mVU_FMACa(mVU, recPass, 2, 2, 0,		"MULx", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (vu0))
mVUop(mVU_MULy)		{ mVU_FMACa(mVU, recPass, 2, 2, 0,		"MULy", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULz)		{ mVU_FMACa(mVU, recPass, 2, 2, 0,		"MULz", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULw)		{ mVU_FMACa(mVU, recPass, 2, 2, 0,		"MULw", (_XYZW_PS)?(cFs|cFt):cFs); } // Clamp (TOTA, DoM, Ice Age (VU0))
mVUop(mVU_MULA)		{ mVU_FMACa(mVU, recPass, 1, 2, 1,		"MULA",   0);  }
mVUop(mVU_MULAi)	{ mVU_FMACa(mVU, recPass, 3, 2, 1,		"MULAi",  0);  }
mVUop(mVU_MULAq)	{ mVU_FMACa(mVU, recPass, 4, 2, 1,		"MULAq",  0);  }
mVUop(mVU_MULAx)	{ mVU_FMACa(mVU, recPass, 2, 2, 1,		"MULAx",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MULAy)	{ mVU_FMACa(mVU, recPass, 2, 2, 1,		"MULAy",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MULAz)	{ mVU_FMACa(mVU, recPass, 2, 2, 1,		"MULAz",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MULAw)	{ mVU_FMACa(mVU, recPass, 2, 2, 1,		"MULAw",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADD)		{ mVU_FMACc(mVU, recPass, 1,			"MADD",   0);  }
mVUop(mVU_MADDi)	{ mVU_FMACc(mVU, recPass, 3,			"MADDi",  0);  }
mVUop(mVU_MADDq)	{ mVU_FMACc(mVU, recPass, 4,			"MADDq",  0);  }
mVUop(mVU_MADDx)	{ mVU_FMACc(mVU, recPass, 2,			"MADDx",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDy)	{ mVU_FMACc(mVU, recPass, 2,			"MADDy",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDz)	{ mVU_FMACc(mVU, recPass, 2,			"MADDz",  cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDw)	{ mVU_FMACc(mVU, recPass, 2,			"MADDw", (isCOP2)?(cACC|cFt|cFs):cFs);} // Clamp (ICO (COP2), TOTA, DoM)
mVUop(mVU_MADDA)	{ mVU_FMACb(mVU, recPass, 1, 0,			"MADDA",  0);  }
mVUop(mVU_MADDAi)	{ mVU_FMACb(mVU, recPass, 3, 0,			"MADDAi", 0);  }
mVUop(mVU_MADDAq)	{ mVU_FMACb(mVU, recPass, 4, 0,			"MADDAq", 0);  }
mVUop(mVU_MADDAx)	{ mVU_FMACb(mVU, recPass, 2, 0,			"MADDAx", cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDAy)	{ mVU_FMACb(mVU, recPass, 2, 0,			"MADDAy", cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDAz)	{ mVU_FMACb(mVU, recPass, 2, 0,			"MADDAz", cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MADDAw)	{ mVU_FMACb(mVU, recPass, 2, 0,			"MADDAw", cFs);} // Clamp (TOTA, DoM, ...)
mVUop(mVU_MSUB)		{ mVU_FMACd(mVU, recPass, 1,			"MSUB",   0);  }
mVUop(mVU_MSUBi)	{ mVU_FMACd(mVU, recPass, 3,			"MSUBi",  0);  }
mVUop(mVU_MSUBq)	{ mVU_FMACd(mVU, recPass, 4,			"MSUBq",  0);  }
mVUop(mVU_MSUBx)	{ mVU_FMACd(mVU, recPass, 2,			"MSUBx",  0);  }
mVUop(mVU_MSUBy)	{ mVU_FMACd(mVU, recPass, 2,			"MSUBy",  0);  }
mVUop(mVU_MSUBz)	{ mVU_FMACd(mVU, recPass, 2,			"MSUBz",  0);  }
mVUop(mVU_MSUBw)	{ mVU_FMACd(mVU, recPass, 2,			"MSUBw",  0);  }
mVUop(mVU_MSUBA)	{ mVU_FMACb(mVU, recPass, 1, 1,			"MSUBA",  0);  }
mVUop(mVU_MSUBAi)	{ mVU_FMACb(mVU, recPass, 3, 1,			"MSUBAi", 0);  }
mVUop(mVU_MSUBAq)	{ mVU_FMACb(mVU, recPass, 4, 1,			"MSUBAq", 0);  }
mVUop(mVU_MSUBAx)	{ mVU_FMACb(mVU, recPass, 2, 1,			"MSUBAx", 0);  }
mVUop(mVU_MSUBAy)	{ mVU_FMACb(mVU, recPass, 2, 1,			"MSUBAy", 0);  }
mVUop(mVU_MSUBAz)	{ mVU_FMACb(mVU, recPass, 2, 1,			"MSUBAz", 0);  }
mVUop(mVU_MSUBAw)	{ mVU_FMACb(mVU, recPass, 2, 1,			"MSUBAw", 0);  }
mVUop(mVU_MAX)		{ mVU_FMACa(mVU, recPass, 1, 3, 0,		"MAX",    0);  }
mVUop(mVU_MAXi)		{ mVU_FMACa(mVU, recPass, 3, 3, 0,		"MAXi",   0);  }
mVUop(mVU_MAXx)		{ mVU_FMACa(mVU, recPass, 2, 3, 0,		"MAXx",   0);  }
mVUop(mVU_MAXy)		{ mVU_FMACa(mVU, recPass, 2, 3, 0,		"MAXy",   0);  }
mVUop(mVU_MAXz)		{ mVU_FMACa(mVU, recPass, 2, 3, 0,		"MAXz",   0);  }
mVUop(mVU_MAXw)		{ mVU_FMACa(mVU, recPass, 2, 3, 0,		"MAXw",   0);  }
mVUop(mVU_MINI)		{ mVU_FMACa(mVU, recPass, 1, 4, 0,		"MINI",   0);  }
mVUop(mVU_MINIi)	{ mVU_FMACa(mVU, recPass, 3, 4, 0,		"MINIi",  0);  }
mVUop(mVU_MINIx)	{ mVU_FMACa(mVU, recPass, 2, 4, 0,		"MINIx",  0);  }
mVUop(mVU_MINIy)	{ mVU_FMACa(mVU, recPass, 2, 4, 0,		"MINIy",  0);  }
mVUop(mVU_MINIz)	{ mVU_FMACa(mVU, recPass, 2, 4, 0,		"MINIz",  0);  }
mVUop(mVU_MINIw)	{ mVU_FMACa(mVU, recPass, 2, 4, 0,		"MINIw",  0);  }
mVUop(mVU_FTOI0)	{ mVU_FTOIx(mX, (uptr)0,				"FTOI0");      }
mVUop(mVU_FTOI4)	{ mVU_FTOIx(mX, (uptr)mVUglob.FTOI_4,	"FTOI4");      }
mVUop(mVU_FTOI12)	{ mVU_FTOIx(mX, (uptr)mVUglob.FTOI_12,	"FTOI12");     }
mVUop(mVU_FTOI15)	{ mVU_FTOIx(mX, (uptr)mVUglob.FTOI_15,	"FTOI15");     }
mVUop(mVU_ITOF0)	{ mVU_ITOFx(mX, (uptr)0,				"ITOF0");      }
mVUop(mVU_ITOF4)	{ mVU_ITOFx(mX, (uptr)mVUglob.ITOF_4,	"ITOF4");      }
mVUop(mVU_ITOF12)	{ mVU_ITOFx(mX, (uptr)mVUglob.ITOF_12,	"ITOF12");     }
mVUop(mVU_ITOF15)	{ mVU_ITOFx(mX, (uptr)mVUglob.ITOF_15,	"ITOF15");     }
mVUop(mVU_NOP)		{ pass3 { mVUlog("NOP"); } }
