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
microVUt(void) mVUdivSet() {
	microVU* mVU = mVUx;
	int flagReg1, flagReg2;
	if (doDivFlag) {
		getFlagReg(flagReg1, fsInstance);
		if (!doStatus) { getFlagReg(flagReg2, fpsInstance); MOV32RtoR(flagReg1, flagReg2); }
		AND32ItoR(flagReg1, 0x0fcf);
		OR32MtoR (flagReg1, (uptr)&mVU->divFlag);
	}
}

// Optimizes out unneeded status flag updates
microVUt(void) mVUstatusFlagOp() {
	microVU* mVU = mVUx;
	int curPC = iPC;
	int i = mVUcount;
	bool runLoop = 1;
	if (doStatus) { mVUinfo |= _isSflag; }
	else {
		for (; i > 0; i--) {
			incPC2(-2);
			if (isSflag)  { runLoop = 0; break; }
			if (doStatus) { mVUinfo |= _isSflag; break; }
		}
	}
	if (runLoop) {
		for (; i > 0; i--) {
			incPC2(-2);
			if (isSflag) break;
			mVUinfo &= ~_doStatus;
		}
	}
	iPC = curPC;
	DevCon::Status("microVU%d: FSSET Optimization", params vuIndex);
}

int findFlagInst(int* fFlag, int cycles) {
	int j = 0, jValue = -1;
	for (int i = 0; i < 4; i++) {
		if ((fFlag[i] <= cycles) && (fFlag[i] > jValue)) { j = i; jValue = fFlag[i]; }
	}
	return j;
}

// Setup Last 4 instances of Status/Mac/Clip flags (needed for accurate block linking)
void sortFlag(int* fFlag, int* bFlag, int cycles) {
	for (int i = 0; i < 4; i++) {
		bFlag[i] = findFlagInst(fFlag, cycles);
		cycles++;
	}
}

#define sFlagCond ((doStatus && !mVUsFlagHack) || isFSSET || doDivFlag)

// Note: Flag handling is 'very' complex, it requires full knowledge of how microVU recs work, so don't touch!
microVUt(int) mVUsetFlags(int* xStatus, int* xMac, int* xClip) {
	microVU* mVU = mVUx;

	int endPC  = iPC;
	u32 aCount = 1; // Amount of instructions needed to get valid mac flag instances for block linking

	// Ensure last ~4+ instructions update mac flags
	setCode();
	for (int i = mVUcount; i > 0; i--, aCount++) {
		if (doStatus) { if (__Mac) { mVUinfo |= _doMac; } if (aCount >= 4) { break; } }
		incPC(-2);
	}

	// Status/Mac Flags Setup Code
	int xS = 0, xM = 0, xC = 0;
	for (int i = 0; i < 4; i++) {
		xStatus[i] = i;
		xMac   [i] = i;
		xClip  [i] = i;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0x00f)) {
		xS = (mVUpBlock->pState.flags >> 0) & 3;
		xStatus[0] = -1; xStatus[1] = -1;
		xStatus[2] = -1; xStatus[3] = -1;
		xStatus[(xS-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0xf00)) {
		xC = (mVUpBlock->pState.flags >> 2) & 3;
		xClip[0] = -1; xClip[1] = -1;
		xClip[2] = -1; xClip[3] = -1;
		xClip[(xC-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0x0f0)) {
		xMac[0] = -1; xMac[1] = -1;
		xMac[2] = -1; xMac[3] = -1;
	}

	int cycles	= 0;
	u32 xCount	= mVUcount; // Backup count
	iPC			= mVUstartPC;
	for (mVUcount = 0; mVUcount < xCount; mVUcount++) {
		if (isFSSET) {
			if (__Status) { // Don't Optimize out on the last ~4+ instructions
				if ((xCount - mVUcount) > aCount) { mVUstatusFlagOp<vuIndex>(); }
			}
			else mVUstatusFlagOp<vuIndex>();
		}
		cycles += mVUstall;

		mVUinfo |= findFlagInst(xStatus, cycles) << 18; // _fvsInstance
		mVUinfo |= findFlagInst(xMac,	 cycles) << 16; // _fvmInstance
		mVUinfo |= findFlagInst(xClip,	 cycles) << 20; // _fvcInstance

		mVUinfo |= xS << 12; // _fsInstance
		mVUinfo |= xM << 10; // _fmInstance
		mVUinfo |= xC << 14; // _fcInstance

		if (sFlagCond)	{ xStatus[xS] = cycles + 4;  xS = (xS+1) & 3; }
		if (doMac)		{ xMac   [xM] = cycles + 4;  xM = (xM+1) & 3; }
		if (doClip)		{ xClip  [xC] = cycles + 4;  xC = (xC+1) & 3; }

		cycles++;
		incPC2(2);
	}

	mVUregs.flags = ((__Clip) ? 0 : (xC << 2)) | ((__Status) ? 0 : xS);
	return cycles;
}

#define getFlagReg1(x)	((x == 3) ? gprF3 : ((x == 2) ? gprF2 : ((x == 1) ? gprF1 : gprF0)))
#define shuffleMac		((bMac [3]<<6)|(bMac [2]<<4)|(bMac [1]<<2)|bMac [0])
#define shuffleClip		((bClip[3]<<6)|(bClip[2]<<4)|(bClip[1]<<2)|bClip[0])

// Recompiles Code for Proper Flags on Block Linkings
microVUt(void) mVUsetupFlags(int* xStatus, int* xMac, int* xClip, int cycles) {
	microVU* mVU = mVUx;

	if (__Status && !mVUflagHack) {
		int bStatus[4];
		sortFlag(xStatus, bStatus, cycles);
		MOV32RtoR(gprT1,  getFlagReg1(bStatus[0])); 
		MOV32RtoR(gprT2,  getFlagReg1(bStatus[1]));
		MOV32RtoR(gprR,   getFlagReg1(bStatus[2]));
		MOV32RtoR(gprF3,  getFlagReg1(bStatus[3]));
		MOV32RtoR(gprF0,  gprT1);
		MOV32RtoR(gprF1,  gprT2); 
		MOV32RtoR(gprF2,  gprR); 
		MOV32ItoR(gprR, Roffset); // Restore gprR
	}

	if (__Mac) {
		int bMac[4];
		sortFlag(xMac, bMac, cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->macFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, shuffleMac);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->macFlag, xmmT1);
	}

	if (__Clip) {
		int bClip[4];
		sortFlag(xClip, bClip, cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->clipFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, shuffleClip);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->clipFlag, xmmT1);
	}
}

#define shortBranch() {												\
	if (branch == 3) {												\
		mVUflagPass<vuIndex>(aBranchAddr, (xCount - (mVUcount+1)));	\
		mVUcount = 4;												\
	}																\
}

// Scan through instructions and check if flags are read (FSxxx, FMxxx, FCxxx opcodes)
microVUx(void) mVUflagPass(u32 startPC, u32 xCount) {

	microVU* mVU  = mVUx;
	int oldPC	  = iPC;
	int oldCount  = mVUcount;
	int oldBranch = mVUbranch;
	int aBranchAddr;
	iPC		  = startPC / 4;
	mVUcount  = 0;
	mVUbranch = 0;
	for (int branch = 0; mVUcount < xCount; mVUcount++) {
		incPC(1);
		if (  curI & _Ebit_   )	{ branch = 1; }
		if (  curI & _MDTbit_ )	{ branch = 4; }
		if (!(curI & _Ibit_)  )	{ incPC(-1); mVUopL<vuIndex>(3); incPC(1); }
		if		(branch >= 2)	{ shortBranch(); break; }
		else if (branch == 1)	{ branch = 2; }
		if		(mVUbranch)		{ branch = (mVUbranch >= 9) ? 5 : 3; aBranchAddr = branchAddr; mVUbranch = 0; }
		incPC(1);
	}
	if (mVUcount < 4) { mVUflagInfo |= 0xfff; }
	iPC		  = oldPC;
	mVUcount  = oldCount;
	mVUbranch = oldBranch;
	setCode();
}

#define branchType1 if		(mVUbranch <= 2)	// B/BAL
#define branchType2 else if (mVUbranch >= 9)	// JR/JALR
#define branchType3 else						// Conditional Branch

// Checks if the first 4 instructions of a block will read flags
microVUt(void) mVUsetFlagInfo() {
	microVU* mVU = mVUx;
	branchType1 { incPC(-1); mVUflagPass<vuIndex>(branchAddr, 4); incPC(1); }
	branchType2 { mVUflagInfo |= 0xfff; }
	branchType3 {
		incPC(-1); 
		mVUflagPass<vuIndex>(branchAddr, 4);
		int backupFlagInfo = mVUflagInfo;
		mVUflagInfo = 0;
		incPC(4); // Branch Not Taken
		mVUflagPass<vuIndex>(xPC, 4);
		incPC(-3);		
		mVUflagInfo |= backupFlagInfo;
	}
}

