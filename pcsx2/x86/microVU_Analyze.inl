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
// Micro VU - Pass 1 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define aReg(x)	   mVUregs.VF[x]
#define bReg(x, y) mVUregsTemp.VFreg[y] = x; mVUregsTemp.VF[y]
#define aMax(x, y) ((x > y) ? x : y)
#define aMin(x, y) ((x < y) ? x : y)

// Read a VF reg
#define analyzeReg1(xReg, vfRead) {																\
	if (xReg) {																					\
		if (_X) { mVUstall = aMax(mVUstall, aReg(xReg).x); vfRead.reg = xReg; vfRead.x = 1; }	\
		if (_Y) { mVUstall = aMax(mVUstall, aReg(xReg).y); vfRead.reg = xReg; vfRead.y = 1; }	\
		if (_Z) { mVUstall = aMax(mVUstall, aReg(xReg).z); vfRead.reg = xReg; vfRead.z = 1; }	\
		if (_W) { mVUstall = aMax(mVUstall, aReg(xReg).w); vfRead.reg = xReg; vfRead.w = 1; }	\
	}																							\
}

// Write to a VF reg
#define analyzeReg2(xReg, vfWrite, isLowOp) {										\
	if (xReg) {																		\
		if (_X) { bReg(xReg, isLowOp).x = 4; vfWrite.reg = xReg; vfWrite.x = 4; }	\
		if (_Y) { bReg(xReg, isLowOp).y = 4; vfWrite.reg = xReg; vfWrite.y = 4; }	\
		if (_Z) { bReg(xReg, isLowOp).z = 4; vfWrite.reg = xReg; vfWrite.z = 4; }	\
		if (_W) { bReg(xReg, isLowOp).w = 4; vfWrite.reg = xReg; vfWrite.w = 4; }	\
	}																				\
}

// Read a VF reg (BC opcodes)
#define analyzeReg3(xReg, vfRead) {																	  \
	if (xReg) {																						  \
		if (_bc_x)		{ mVUstall = aMax(mVUstall, aReg(xReg).x); vfRead.reg = xReg; vfRead.x = 1; } \
		else if (_bc_y) { mVUstall = aMax(mVUstall, aReg(xReg).y); vfRead.reg = xReg; vfRead.y = 1; } \
		else if (_bc_z) { mVUstall = aMax(mVUstall, aReg(xReg).z); vfRead.reg = xReg; vfRead.z = 1; } \
		else			{ mVUstall = aMax(mVUstall, aReg(xReg).w); vfRead.reg = xReg; vfRead.w = 1; } \
	}																								  \
}

// For Clip Opcode
#define analyzeReg4(xReg, vfRead) {					\
	if (xReg) {										\
		mVUstall = aMax(mVUstall, aReg(xReg).w);	\
		vfRead.reg = xReg; vfRead.w = 1;			\
	}												\
}

// Read VF reg (FsF/FtF)
#define analyzeReg5(xReg, fxf, vfRead) {																\
	if (xReg) {																							\
		switch (fxf) {																					\
			case 0: mVUstall = aMax(mVUstall, aReg(xReg).x); vfRead.reg = xReg; vfRead.x = 1; break;	\
			case 1: mVUstall = aMax(mVUstall, aReg(xReg).y); vfRead.reg = xReg; vfRead.y = 1; break;	\
			case 2: mVUstall = aMax(mVUstall, aReg(xReg).z); vfRead.reg = xReg; vfRead.z = 1; break;	\
			case 3: mVUstall = aMax(mVUstall, aReg(xReg).w); vfRead.reg = xReg; vfRead.w = 1; break;	\
		}																								\
	}																									\
}

// Flips xyzw stalls to yzwx (MR32 Opcode)
#define analyzeReg6(xReg, vfRead) {																\
	if (xReg) {																					\
		if (_X) { mVUstall = aMax(mVUstall, aReg(xReg).y); vfRead.reg = xReg; vfRead.y = 1; }	\
		if (_Y) { mVUstall = aMax(mVUstall, aReg(xReg).z); vfRead.reg = xReg; vfRead.z = 1; }	\
		if (_Z) { mVUstall = aMax(mVUstall, aReg(xReg).w); vfRead.reg = xReg; vfRead.w = 1; }	\
		if (_W) { mVUstall = aMax(mVUstall, aReg(xReg).x); vfRead.reg = xReg; vfRead.x = 1; }	\
	}																							\
}

// Reading a VI reg
#define analyzeVIreg1(xReg, viRead) {				 \
	if (xReg) {										 \
		mVUstall = aMax(mVUstall, mVUregs.VI[xReg]); \
		viRead.reg = xReg; viRead.used = 1;			 \
	}												 \
}

// Writing to a VI reg
#define analyzeVIreg2(xReg, viWrite, aCycles) {	\
	if (xReg) {									\
		mVUconstReg[xReg].isValid = 0;			\
		mVUregsTemp.VIreg = xReg;				\
		mVUregsTemp.VI = aCycles;				\
		viWrite.reg = xReg;						\
		viWrite.used = aCycles;					\
	}											\
}

#define analyzeQreg(x)	  { mVUregsTemp.q = x; mVUstall = aMax(mVUstall, mVUregs.q); }
#define analyzePreg(x)	  { mVUregsTemp.p = x; mVUstall = aMax(mVUstall, ((mVUregs.p) ? (mVUregs.p - 1) : 0)); }
#define analyzeRreg()	  { mVUregsTemp.r = 1; }
#define analyzeXGkick1()  { mVUstall = aMax(mVUstall, mVUregs.xgkick); }
#define analyzeXGkick2(x) { mVUregsTemp.xgkick = x; }
#define setConstReg(x, v) { if (x) { mVUconstReg[x].isValid = 1; mVUconstReg[x].regValue = v; } }

//------------------------------------------------------------------
// FMAC1 - Normal FMAC Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeFMAC1(mV, int Fd, int Fs, int Ft) {
	sFLAG.doFlag = 1;
	analyzeReg1(Fs, mVUup.VF_read[0]);
	analyzeReg1(Ft, mVUup.VF_read[1]);
	analyzeReg2(Fd, mVUup.VF_write, 0);
}

//------------------------------------------------------------------
// FMAC2 - ABS/FTOI/ITOF Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeFMAC2(mV, int Fs, int Ft) {
	analyzeReg1(Fs, mVUup.VF_read[0]);
	analyzeReg2(Ft, mVUup.VF_write, 0);
}

//------------------------------------------------------------------
// FMAC3 - BC(xyzw) FMAC Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeFMAC3(mV, int Fd, int Fs, int Ft) {
	sFLAG.doFlag = 1;
	analyzeReg1(Fs, mVUup.VF_read[0]);
	analyzeReg3(Ft, mVUup.VF_read[1]);
	analyzeReg2(Fd, mVUup.VF_write, 0);
}

//------------------------------------------------------------------
// FMAC4 - Clip FMAC Opcode
//------------------------------------------------------------------

__fi void mVUanalyzeFMAC4(mV, int Fs, int Ft) {
	cFLAG.doFlag = 1;
	analyzeReg1(Fs, mVUup.VF_read[0]);
	analyzeReg4(Ft, mVUup.VF_read[1]);
}

//------------------------------------------------------------------
// IALU - IALU Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeIALU1(mV, int Id, int Is, int It) {
	if (!Id) { mVUlow.isNOP = 1; }
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	analyzeVIreg1(It, mVUlow.VI_read[1]);
	analyzeVIreg2(Id, mVUlow.VI_write, 1);
}

__fi void mVUanalyzeIALU2(mV, int Is, int It) {
	if (!It) { mVUlow.isNOP = 1; }
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	analyzeVIreg2(It, mVUlow.VI_write, 1);
}

__fi void mVUanalyzeIADDI(mV, int Is, int It, s16 imm) {
	mVUanalyzeIALU2(mVU, Is, It);
	if (!Is) { setConstReg(It, imm); }
}

//------------------------------------------------------------------
// MR32 - MR32 Opcode
//------------------------------------------------------------------

__fi void mVUanalyzeMR32(mV, int Fs, int Ft) {
	if (!Ft) { mVUlow.isNOP = 1; }
	analyzeReg6(Fs, mVUlow.VF_read[0]);
	analyzeReg2(Ft, mVUlow.VF_write, 1);
}

//------------------------------------------------------------------
// FDIV - DIV/SQRT/RSQRT Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeFDIV(mV, int Fs, int Fsf, int Ft, int Ftf, u8 xCycles) {
	analyzeReg5(Fs, Fsf, mVUlow.VF_read[0]);
	analyzeReg5(Ft, Ftf, mVUlow.VF_read[1]);
	analyzeQreg(xCycles);
}

//------------------------------------------------------------------
// EFU - EFU Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeEFU1(mV, int Fs, int Fsf, u8 xCycles) {
	analyzeReg5(Fs, Fsf, mVUlow.VF_read[0]);
	analyzePreg(xCycles);
}

__fi void mVUanalyzeEFU2(mV, int Fs, u8 xCycles) {
	analyzeReg1(Fs, mVUlow.VF_read[0]);
	analyzePreg(xCycles);
}

//------------------------------------------------------------------
// MFP - MFP Opcode
//------------------------------------------------------------------

__fi void mVUanalyzeMFP(mV, int Ft) {
	if (!Ft) { mVUlow.isNOP = 1; }
	analyzeReg2(Ft, mVUlow.VF_write, 1);
}

//------------------------------------------------------------------
// MOVE - MOVE Opcode
//------------------------------------------------------------------

__fi void mVUanalyzeMOVE(mV, int Fs, int Ft) {
	if (!Ft || (Ft == Fs)) { mVUlow.isNOP = 1; }
	analyzeReg1(Fs, mVUlow.VF_read[0]);
	analyzeReg2(Ft, mVUlow.VF_write, 1);
}

//------------------------------------------------------------------
// LQx - LQ/LQD/LQI Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeLQ(mV, int Ft, int Is, bool writeIs) {
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	analyzeReg2  (Ft, mVUlow.VF_write, 1);
	if (!Ft)	 { if (writeIs && Is) { mVUlow.noWriteVF = 1; } else { mVUlow.isNOP = 1; } }
	if (writeIs) { analyzeVIreg2(Is, mVUlow.VI_write, 1); }
}

//------------------------------------------------------------------
// SQx - SQ/SQD/SQI Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeSQ(mV, int Fs, int It, bool writeIt) {
	analyzeReg1  (Fs, mVUlow.VF_read[0]);
	analyzeVIreg1(It, mVUlow.VI_read[0]);
	if (writeIt) { analyzeVIreg2(It, mVUlow.VI_write, 1); }
}

//------------------------------------------------------------------
// R*** - R Reg Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeR1(mV, int Fs, int Fsf) {
	analyzeReg5(Fs, Fsf, mVUlow.VF_read[0]);
	analyzeRreg();
}

__fi void mVUanalyzeR2(mV, int Ft, bool canBeNOP) {
	if (!Ft) { if (canBeNOP) { mVUlow.isNOP = 1; } else { mVUlow.noWriteVF = 1; } }
	analyzeReg2(Ft, mVUlow.VF_write, 1);
	analyzeRreg();
}

//------------------------------------------------------------------
// Sflag - Status Flag Opcodes
//------------------------------------------------------------------
__ri void flagSet(mV, bool setMacFlag) {
	int curPC = iPC;
	for (int i = mVUcount, j = 0; i > 0; i--, j++) {
		j += mVUstall;
		incPC2(-2);
		if (sFLAG.doFlag && (j >= 3)) { 
			if (setMacFlag) { mFLAG.doFlag = 1; }
			else { sFLAG.doNonSticky = 1; }
			break; 
		}
	}
	iPC = curPC;
}

__ri void mVUanalyzeSflag(mV, int It) {
	mVUlow.readFlags = 1;
	analyzeVIreg2(It, mVUlow.VI_write, 1);
	if (!It) { mVUlow.isNOP = 1; }
	else {
		mVUsFlagHack = 0; // Don't Optimize Out Status Flags for this block
		mVUinfo.swapOps = 1;
		flagSet(mVU, 0);
		if (mVUcount < 4) {
			if (!(mVUpBlock->pState.needExactMatch & 1)) // The only time this should happen is on the first program block
				DevCon.WriteLn(Color_Green, "microVU%d: pState's sFlag Info was expected to be set [%04x]", getIndex, xPC);
		}
	}
}

__ri void mVUanalyzeFSSET(mV) {
	mVUlow.isFSSET = 1;
	mVUlow.readFlags = 1;
}

//------------------------------------------------------------------
// Mflag - Mac Flag Opcodes
//------------------------------------------------------------------

__ri void mVUanalyzeMflag(mV, int Is, int It) {
	mVUlow.readFlags = 1;
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	analyzeVIreg2(It, mVUlow.VI_write, 1);
	if (!It) { mVUlow.isNOP = 1; }
	else {
		mVUinfo.swapOps = 1;
		flagSet(mVU, 1);
		if (mVUcount < 4) { 
			if (!(mVUpBlock->pState.needExactMatch & 2)) // The only time this should happen is on the first program block
				DevCon.WriteLn(Color_Green, "microVU%d: pState's mFlag Info was expected to be set [%04x]", getIndex, xPC);
		}
	}
}

//------------------------------------------------------------------
// Cflag - Clip Flag Opcodes
//------------------------------------------------------------------

__fi void mVUanalyzeCflag(mV, int It) {
	mVUinfo.swapOps = 1;
	mVUlow.readFlags = 1;
	if (mVUcount < 4) { 
		if (!(mVUpBlock->pState.needExactMatch & 4)) // The only time this should happen is on the first program block
			DevCon.WriteLn(Color_Green, "microVU%d: pState's cFlag Info was expected to be set [%04x]", getIndex, xPC);
	}
	analyzeVIreg2(It, mVUlow.VI_write, 1);
}

//------------------------------------------------------------------
// XGkick
//------------------------------------------------------------------

__fi void mVUanalyzeXGkick(mV, int Fs, int xCycles) {
	analyzeVIreg1(Fs, mVUlow.VI_read[0]);
	analyzeXGkick1();
	analyzeXGkick2(xCycles);
	// Note: Technically XGKICK should stall on the next instruction,
	// this code stalls on the same instruction. The only case where this
	// will be a problem with, is if you have very-specifically placed
	// FMxxx or FSxxx opcodes checking flags near this instruction AND
	// the XGKICK instruction stalls. No-game should be effected by 
	// this minor difference.
}

//------------------------------------------------------------------
// Branches - Branch Opcodes
//------------------------------------------------------------------

static void analyzeBranchVI(mV, int xReg, bool &infoVar) {
	if (!xReg) return;
	int i, j = 0;
	int iEnd = 4;
	int bPC  = iPC;
	incPC2(-2);
	for (i = 0; i < iEnd; i++) {
		if (i == mVUcount) {
			bool warn = 0;
			if (i == 1) warn = 1;
			if (mVUpBlock->pState.viBackUp == xReg) {
				if (i == 0) warn = 1;
				infoVar = 1;
				j = i; i++;
			}
			if (warn) DevCon.Warning("microVU%d: Warning Branch VI-Delay with small block (%d) [%04x]", getIndex, i, xPC);
			break; // if (warn), we don't have enough information to always guarantee the correct result.
		}
		if ((mVUlow.VI_write.reg == xReg) && mVUlow.VI_write.used) {
			if (mVUlow.readFlags) {
				if (i) DevCon.Warning("microVU%d: Warning Branch VI-Delay with Read Flags Set (%d) [%04x]", getIndex, i, xPC);
				break; // Not sure if on the above "if (i)" case, if we need to "continue" or if we should "break"
			}
			j = i;
		}
		elif (i == 0) break;
		incPC2(-2);
	}
	if (i) {
		if (!infoVar) {
			iPC = bPC;
			incPC2(-2*(j+1));
			mVUlow.backupVI = 1;
			infoVar = 1;
		}
		iPC = bPC;
		DevCon.WriteLn(Color_Green, "microVU%d: Branch VI-Delay (%d) [%04x]", getIndex, j+1, xPC);
	}
	else iPC = bPC;
}

/*
// Dead Code... the old version of analyzeBranchVI()
__fi void analyzeBranchVI(mV, int xReg, bool &infoVar) {
	if (!xReg) return;
	int i;
	int iEnd = aMin(5, (mVUcount+1));
	int bPC = iPC;
	incPC2(-2);
	for (i = 0; i < iEnd; i++) {
		if ((i == mVUcount) && (i < 5)) {
			if (mVUpBlock->pState.viBackUp == xReg) {
				infoVar = 1;
				i++;
			}
			break; 
		}
		if ((mVUlow.VI_write.reg == xReg) && mVUlow.VI_write.used) {
			if (mVUlow.readFlags || i == 5) break;
			if (i == 0) { incPC2(-2); continue;	}
			if (((mVUlow.VI_read[0].reg == xReg) && (mVUlow.VI_read[0].used))
			||	((mVUlow.VI_read[1].reg == xReg) && (mVUlow.VI_read[1].used)))
			{ incPC2(-2); continue; }
		}
		break;
	}
	if (i) {
		if (!infoVar) {
			incPC2(2);
			mVUlow.backupVI = 1;
			infoVar = 1;
		}
		iPC = bPC;
		DevCon.WriteLn( Color_Green, "microVU%d: Branch VI-Delay (%d) [%04x]", getIndex, i, xPC);
	}
	else iPC = bPC;
}
*/

// Branch in Branch Delay-Slots
__ri int mVUbranchCheck(mV) {
	if (!mVUcount) return 0;
	incPC(-2);
	if (mVUlow.branch) {
		mVUlow.badBranch  = 1;
		incPC(2);
		mVUlow.evilBranch = 1;
		mVUregs.blockType = 2;
		mVUregs.needExactMatch |= 7; // This might not be necessary, but w/e...
		DevCon.Warning("microVU%d Warning: Branch in Branch delay slot! [%04x]", mVU.index, xPC);
		return 1;
	}
	incPC(2);
	return 0;
}

__fi void mVUanalyzeCondBranch1(mV, int Is) {
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	if (!mVUstall && !mVUbranchCheck(mVU)) { 
		analyzeBranchVI(mVU, Is, mVUlow.memReadIs);
	}
}

__fi void mVUanalyzeCondBranch2(mV, int Is, int It) {
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	analyzeVIreg1(It, mVUlow.VI_read[1]);
	if (!mVUstall && !mVUbranchCheck(mVU)) {
		analyzeBranchVI(mVU, Is, mVUlow.memReadIs);
		analyzeBranchVI(mVU, It, mVUlow.memReadIt);
	}
}

__fi void mVUanalyzeNormBranch(mV, int It, bool isBAL) {
	mVUbranchCheck(mVU);
	if (isBAL) {
		analyzeVIreg2(It, mVUlow.VI_write, 1); 
		setConstReg(It, bSaveAddr);
	}
}

__ri void mVUanalyzeJump(mV, int Is, int It, bool isJALR) {
	mVUbranchCheck(mVU);
	mVUlow.branch = (isJALR) ? 10 : 9;
	if (mVUconstReg[Is].isValid && doConstProp) {
		mVUlow.constJump.isValid  = 1;
		mVUlow.constJump.regValue = mVUconstReg[Is].regValue;
		//DevCon.Status("microVU%d: Constant JR/JALR Address Optimization", mVU.index);
	}
	analyzeVIreg1(Is, mVUlow.VI_read[0]);
	if (isJALR) {
		analyzeVIreg2(It, mVUlow.VI_write, 1);
		setConstReg(It, bSaveAddr);
	}
}
