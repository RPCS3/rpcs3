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

// ToDo: Fix this properly.
#define flagSetMacro(xFlag, pFlag, xF, yF, zF) {	\
	yF += (mVUstall > 3) ? 3 : mVUstall;			\
	if (yF > zF) {									\
		pFlag += (yF-zF);							\
		if (pFlag >= xFlag) pFlag = (xFlag-1);		\
		zF++;										\
		xF = (yF-zF);								\
		zF = yF;									\
		yF -= xF;									\
	}												\
	yF++;											\
}

// Sets FDIV Flags at the proper time
microVUt(void) mVUdivSet() {
	microVU* mVU = mVUx;
	int flagReg1, flagReg2;
	if (doDivFlag) {
		getFlagReg(flagReg1, fsInstance);
		if (!doStatus) { getFlagReg(flagReg2, fpsInstance); MOV16RtoR(flagReg1, flagReg2); }
		AND16ItoR(flagReg1, 0x0fcf);
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
}

// Note: Flag handling is 'very' complex, it requires full knowledge of how microVU recs work, so don't touch!
microVUt(void) mVUsetFlags(int* bStatus, int* bMac) {
	microVU* mVU = mVUx;

	// Ensure last ~4+ instructions update mac flags
	int endPC  = iPC;
	u32 aCount = 1; // Amount of instructions needed to get 4 valid status/mac flag instances
	for (int i = mVUcount, iX = 0; i > 0; i--, aCount++) {
		if (doStatus) { mVUinfo |= _doMac; iX++; if ((iX >= 4) || (aCount > 4)) { break; } }
		incPC2(-2);
	}

	// Status/Mac Flags Setup Code
	int xStatus	= 8, xMac = 8; // Flag Instances start at #0 on every block ((8&3) == 0)
	int pStatus	= 3, pMac = 3;
	int xClip = mVUregs.clip + 8, pClip = mVUregs.clip + 7; // Clip Instance starts from where it left off
	int xS = 0, yS = 1, zS = 0;
	int xM = 0, yM = 1, zM = 0;
	int xC = 0, yC = 1, zC = 0;
	u32 xCount	= mVUcount; // Backup count
	iPC			= mVUstartPC;
	for (mVUcount = 0; mVUcount < xCount; mVUcount++) {
		if (((xCount - mVUcount) > aCount) && isFSSET) mVUstatusFlagOp<vuIndex>(); // Don't Optimize out on the last ~4+ instructions

		flagSetMacro(xStatus, pStatus, xS, yS, zS); // Handles _fvsinstances
		flagSetMacro(xMac,	  pMac,	   xM, yM, zM); // Handles _fvminstances
		flagSetMacro(xClip,	  pClip,   xC, yC, zC); // Handles _fvcinstances

		mVUinfo |= (xStatus&3)	<< 12; // _fsInstance
		mVUinfo |= (xMac&3)		<< 10; // _fmInstance
		mVUinfo |= (xClip&3)	<< 14; // _fcInstance

		mVUinfo |= (pStatus&3)	<< 18; // _fvsInstance
		mVUinfo |= (pMac&3)		<< 16; // _fvmInstance
		mVUinfo |= (pClip&3)	<< 20; // _fvcInstance

		if (doStatus||isFSSET||doDivFlag) { xStatus = (xStatus+1); }
		if (doMac)						  { xMac	= (xMac+1); }
		if (doClip)						  { xClip	= (xClip+1); }
		incPC2(2);
	}
	mVUcount = xCount; // Restore count
	mVUregs.clip = xClip&3; // Note: Clip timing isn't cycle-accurate between block linking; but hopefully doesn't matter

	// Temp Hack-fix until flag-algorithm rewrite
	for (int i = 0; i < 4; i++) {
		bStatus[i] = 0;
		bMac[i]    = 0;
	}

	// Setup Last 4 instances of Status/Mac flags (needed for accurate block linking)
	iPC = endPC;
	for (int i = 3, j = 3, ii = 1, jj = 1; aCount > 0; ii++, jj++, aCount--) {
		if ((doStatus||isFSSET||doDivFlag) && (i >= 0)) { 
			for (; (ii > 0 && i >= 0); i--, ii--) { xStatus = (xStatus-1) & 3; bStatus[i] = xStatus; }
		}
		if (doMac && (j >= 0)) { 
			for (; (jj > 0 && j >= 0); j--, jj--) { xMac = (xMac-1) & 3; bMac[j] = xMac; }
		}
		incPC2(-2);
	}
	iPC = endPC;
}

#define getFlagReg1(x)	((x == 3) ? gprF3 : ((x == 2) ? gprF2 : ((x == 1) ? gprF1 : gprF0)))
#define getFlagReg2(x)	((x == bStatus[3]) ? gprESP : ((x == bStatus[2]) ? gprR : ((x == bStatus[1]) ? gprT2 : gprT1)))

// Recompiles Code for Proper Flags on Block Linkings
microVUt(void) mVUsetupFlags(int* bStatus, int* bMac) {
	microVU* mVU = mVUx;

	PUSH32R(gprR);   // Backup gprR
	MOV32RtoM((uptr)&mVU->espBackup, gprESP);

	MOV32RtoR(gprT1,  getFlagReg1(bStatus[0])); 
	MOV32RtoR(gprT2,  getFlagReg1(bStatus[1]));
	MOV32RtoR(gprR,   getFlagReg1(bStatus[2]));
	MOV32RtoR(gprESP, getFlagReg1(bStatus[3]));

	MOV32RtoR(gprF0, gprT1);
	MOV32RtoR(gprF1, gprT2); 
	MOV32RtoR(gprF2, gprR); 
	MOV32RtoR(gprF3, gprESP);

	AND32ItoR(gprT1,  0xffff0000);
	AND32ItoR(gprT2,  0xffff0000);
	AND32ItoR(gprR,   0xffff0000);
	AND32ItoR(gprESP, 0xffff0000);

	AND32ItoR(gprF0,  0x0000ffff);
	AND32ItoR(gprF1,  0x0000ffff);
	AND32ItoR(gprF2,  0x0000ffff);
	AND32ItoR(gprF3,  0x0000ffff);

	OR32RtoR(gprF0, getFlagReg2(bMac[0])); 
	OR32RtoR(gprF1, getFlagReg2(bMac[1]));
	OR32RtoR(gprF2, getFlagReg2(bMac[2]));
	OR32RtoR(gprF3, getFlagReg2(bMac[3]));

	MOV32MtoR(gprESP, (uptr)&mVU->espBackup);
	POP32R(gprR);   // Restore gprR
}

#endif //PCSX2_MICROVU