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

// Sets FDIV Flags at the proper time
microVUt(void) mVUdivSet(mV) {
	int flagReg1, flagReg2;
	if (mVUinfo.doDivFlag) {
		getFlagReg(flagReg1, sFLAG.write);
		if (!sFLAG.doFlag) { getFlagReg(flagReg2, sFLAG.lastWrite); MOV32RtoR(flagReg1, flagReg2); }
		AND32ItoR(flagReg1, 0xfff3ffff);
		OR32MtoR (flagReg1, (uptr)&mVU->divFlag);
	}
}

// Optimizes out unneeded status flag updates
microVUt(void) mVUstatusFlagOp(mV) {
	int curPC = iPC;
	int i = mVUcount;
	bool runLoop = 1;
	if (sFLAG.doFlag) { sFLAG.doNonSticky = 1; }
	else {
		for (; i > 0; i--) {
			incPC2(-2);
			if (sFLAG.doNonSticky) { runLoop = 0; break; }
			else if (sFLAG.doFlag) { sFLAG.doNonSticky = 1; break; }
		}
	}
	if (runLoop) {
		for (; i > 0; i--) {
			incPC2(-2);
			if (sFLAG.doNonSticky) break;
			sFLAG.doFlag = 0;
		}
	}
	iPC = curPC;
	DevCon::Status("microVU%d: FSSET Optimization", params getIndex);
}

int findFlagInst(int* fFlag, int cycles) {
	int j = 0, jValue = -1;
	for (int i = 0; i < 4; i++) {
		if ((fFlag[i] <= cycles) && (fFlag[i] > jValue)) { j = i; jValue = fFlag[i]; }
	}
	return j;
}

// Setup Last 4 instances of Status/Mac/Clip flags (needed for accurate block linking)
int sortFlag(int* fFlag, int* bFlag, int cycles) {
	int lFlag = -5;
	int x = 0;
	for (int i = 0; i < 4; i++) {
		bFlag[i] = findFlagInst(fFlag, cycles);
		if (lFlag != bFlag[i]) { x++; }
		lFlag = bFlag[i];
		cycles++;
	}
	return x; // Returns the number of Valid Flag Instances
}

#define sFlagCond (sFLAG.doFlag || mVUlow.isFSSET || mVUinfo.doDivFlag)
#define sHackCond (mVUsFlagHack && !sFLAG.doNonSticky)

// Note: Flag handling is 'very' complex, it requires full knowledge of how microVU recs work, so don't touch!
microVUt(void) mVUsetFlags(mV, microFlagCycles& mFC) {

	int endPC  = iPC;
	u32 aCount = 1; // Amount of instructions needed to get valid mac flag instances for block linking

	// Ensure last ~4+ instructions update mac/status flags (if next block's first 4 instructions will read them)
	for (int i = mVUcount; i > 0; i--, aCount++) {
		if (sFLAG.doFlag) { 
			if (__Mac)	  { mFLAG.doFlag = 1; } 
			if (__Status) { sFLAG.doNonSticky = 1; } 		
			if (aCount >= 4) { break; } 
		}
		incPC2(-2);
	}

	// Status/Mac Flags Setup Code
	int xS = 0, xM = 0, xC = 0;
	for (int i = 0; i < 4; i++) {
		mFC.xStatus[i] = i;
		mFC.xMac   [i] = i;
		mFC.xClip  [i] = i;
	}

	if (!(mVUpBlock->pState.needExactMatch & 1)) {
		xS = (mVUpBlock->pState.flags >> 0) & 3;
		mFC.xStatus[0] = -1; mFC.xStatus[1] = -1;
		mFC.xStatus[2] = -1; mFC.xStatus[3] = -1;
		mFC.xStatus[(xS-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 4)) {
		xC = (mVUpBlock->pState.flags >> 2) & 3;
		mFC.xClip[0] = -1; mFC.xClip[1] = -1;
		mFC.xClip[2] = -1; mFC.xClip[3] = -1;
		mFC.xClip[(xC-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 2)) {
		mFC.xMac[0] = -1; mFC.xMac[1] = -1;
		mFC.xMac[2] = -1; mFC.xMac[3] = -1;
	}

	mFC.cycles	= 0;
	u32 xCount	= mVUcount; // Backup count
	iPC			= mVUstartPC;
	for (mVUcount = 0; mVUcount < xCount; mVUcount++) {
		if (mVUlow.isFSSET) {
			if (__Status) { // Don't Optimize out on the last ~4+ instructions
				if ((xCount - mVUcount) > aCount) { mVUstatusFlagOp(mVU); }
			}
			else mVUstatusFlagOp(mVU);
		}
		mFC.cycles += mVUstall;

		sFLAG.read = findFlagInst(mFC.xStatus, mFC.cycles);
		mFLAG.read = findFlagInst(mFC.xMac,	   mFC.cycles);
		cFLAG.read = findFlagInst(mFC.xClip,   mFC.cycles);
		
		sFLAG.write = xS;
		mFLAG.write = xM;
		cFLAG.write = xC;

		sFLAG.lastWrite = (xS-1) & 3;
		mFLAG.lastWrite = (xM-1) & 3;
		cFLAG.lastWrite = (xC-1) & 3;

		if (sHackCond)	  { sFLAG.doFlag = 0; }
		if (sFlagCond)	  { mFC.xStatus[xS] = mFC.cycles + 4; xS = (xS+1) & 3; }
		if (mFLAG.doFlag) { mFC.xMac   [xM] = mFC.cycles + 4; xM = (xM+1) & 3; }
		if (cFLAG.doFlag) { mFC.xClip  [xC] = mFC.cycles + 4; xC = (xC+1) & 3; }

		mFC.cycles++;
		incPC2(2);
	}

	mVUregs.flags = ((__Clip) ? 0 : (xC << 2)) | ((__Status) ? 0 : xS);
	iPC = endPC;
}

#define getFlagReg1(x)	((x == 3) ? gprF3 : ((x == 2) ? gprF2 : ((x == 1) ? gprF1 : gprF0)))
#define getFlagReg2(x)	((bStatus[0] == x) ? getFlagReg1(x) : gprT1)
#define getFlagReg3(x)	((gFlag == x) ? gprT1 : getFlagReg1(x))
#define getFlagReg4(x)	((gFlag == x) ? gprT1 : gprT2)
#define shuffleMac		((bMac [3]<<6)|(bMac [2]<<4)|(bMac [1]<<2)|bMac [0])
#define shuffleClip		((bClip[3]<<6)|(bClip[2]<<4)|(bClip[1]<<2)|bClip[0])

// Recompiles Code for Proper Flags on Block Linkings
microVUt(void) mVUsetupFlags(mV, microFlagCycles& mFC) {

	if (__Status) {
		int bStatus[4];
		int sortRegs = sortFlag(mFC.xStatus, bStatus, mFC.cycles);
		// DevCon::Status("sortRegs = %d", params sortRegs);
		// Note: Emitter will optimize out mov(reg1, reg1) cases...
		if (sortRegs == 1) {
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg1(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg1(bStatus[2]));
			MOV32RtoR(gprF3,  getFlagReg1(bStatus[3]));
		}
		else if (sortRegs == 2) {
			MOV32RtoR(gprT1,  getFlagReg1(bStatus[3])); 
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg2(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg2(bStatus[2]));
			MOV32RtoR(gprF3,  gprT1);
		}
		else if (sortRegs == 3) {
			int gFlag = (bStatus[0] == bStatus[1]) ? bStatus[2] : bStatus[1];
			MOV32RtoR(gprT1,  getFlagReg1(gFlag)); 
			MOV32RtoR(gprT2,  getFlagReg1(bStatus[3]));
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg3(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg4(bStatus[2]));
			MOV32RtoR(gprF3,  gprT2);
		}
		else {
			MOV32RtoR(gprT1,  getFlagReg1(bStatus[0])); 
			MOV32RtoR(gprT2,  getFlagReg1(bStatus[1]));
			MOV32RtoR(gprR,   getFlagReg1(bStatus[2]));
			MOV32RtoR(gprF3,  getFlagReg1(bStatus[3]));
			MOV32RtoR(gprF0,  gprT1);
			MOV32RtoR(gprF1,  gprT2); 
			MOV32RtoR(gprF2,  gprR); 
			MOV32ItoR(gprR, Roffset); // Restore gprR
		}
	}

	if (__Mac) {
		int bMac[4];
		sortFlag(mFC.xMac, bMac, mFC.cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->macFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, shuffleMac);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->macFlag, xmmT1);
	}

	if (__Clip) {
		int bClip[4];
		sortFlag(mFC.xClip, bClip, mFC.cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT2, (uptr)mVU->clipFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT2, xmmT2, shuffleClip);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->clipFlag, xmmT2);
	}
}

#define shortBranch()											\
	if ((branch == 3) || (branch == 4)) {						\
		mVUflagPass(mVU, aBranchAddr, (xCount - (mVUcount+1)));	\
		if (branch == 3) { mVUcount = 4; break; }				\
	}															\
	else break

// Scan through instructions and check if flags are read (FSxxx, FMxxx, FCxxx opcodes)
void mVUflagPass(mV, u32 startPC, u32 xCount) {

	int oldPC	  = iPC;
	int oldCount  = mVUcount;
	int oldBranch = mVUbranch;
	int aBranchAddr;
	iPC		  = startPC / 4;
	mVUcount  = 0;
	mVUbranch = 0;
	for (int branch = 0; mVUcount < xCount; mVUcount=(mVUregs.needExactMatch&8)?(mVUcount+1):mVUcount) {
		incPC(1);
		mVUopU(mVU, 3);
		if (  curI & _Ebit_  )	{ branch = 1; }
		if (  curI & _DTbit_ )	{ branch = 6; }
		if (!(curI & _Ibit_) )	{ incPC(-1); mVUopL(mVU, 3); incPC(1); }
		if		(branch >= 2)	{ shortBranch(); }
		else if (branch == 1)	{ branch = 2; }
		if		(mVUbranch)		{ branch = ((mVUbranch>8)?(5):((mVUbranch<3)?3:4)); aBranchAddr = branchAddr; mVUbranch = 0; }
		incPC(1);
	}
	if (mVUcount < 4) { mVUregs.needExactMatch |= 0x7; }
	iPC		  = oldPC;
	mVUcount  = oldCount;
	mVUbranch = oldBranch;
	setCode();
}

#define branchType1 if		(mVUbranch <= 2)	// B/BAL
#define branchType2 else if (mVUbranch >= 9)	// JR/JALR
#define branchType3 else						// Conditional Branch

// Checks if the first 4 instructions of a block will read flags
microVUt(void) mVUsetFlagInfo(mV) {
	branchType1 { incPC(-1); mVUflagPass(mVU, branchAddr, 4); incPC(1); }
	branchType2 { 
		if (!mVUlow.constJump.isValid || CHECK_VU_CONSTHACK) { mVUregs.needExactMatch |= 0x7; } 
		else { mVUflagPass(mVU, (mVUlow.constJump.regValue*8)&(mVU->microMemSize-8), 4); }
	}
	branchType3 {
		incPC(-1);
		mVUflagPass(mVU, branchAddr, 4);
		int backupFlagInfo = mVUregs.needExactMatch;
		mVUregs.needExactMatch = 0;
		incPC(4); // Branch Not Taken
		mVUflagPass(mVU, xPC, 4);
		incPC(-3);
		mVUregs.needExactMatch |= backupFlagInfo;
	}
	mVUregs.needExactMatch &= 0x7;
}

