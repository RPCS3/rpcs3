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
// Helper Macros
//------------------------------------------------------------------

#define createBlock(blockEndPtr) {									\
	block.pipelineState = pipelineState;							\
	block.x86ptrStart = x86ptrStart;								\
	block.x86ptrEnd = blockEndPtr;									\
	/*block.x86ptrBranch;*/											\
	if (!(pipelineState & 1)) {										\
		memcpy_fast(&block.pState, pState, sizeof(microRegInfo));	\
	}																\
}

#define branchCase(JMPcc)											\
	CMP16ItoM((uptr)mVU->branch, 0);								\
	ajmp = JMPcc((uptr)0);											\
	break

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

#define startLoop()			{ mVUdebug1(); mVUstall = 0; memset(&mVUregsTemp, 0, sizeof(mVUregsTemp)); }
#define calcCycles(reg, x)	{ reg = ((reg > x) ? (reg - x) : 0); }
#define incP()				{ mVU->p = (mVU->p+1) & 1; }
#define incQ()				{ mVU->q = (mVU->q+1) & 1; }
#define doUpperOp()			{ mVUopU<vuIndex, 1>(); mVUdivSet<vuIndex>(); }

//------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------

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
	int xStatus	= 8, xMac = 8, xClip = 8; // Flag Instances start at #0 on every block ((8&3) == 0)
	int pStatus	= 3, pMac = 3, pClip = 3;
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

	// Setup Last 4 instances of Status/Mac flags (needed for accurate block linking)
	iPC = endPC;
	for (int i = 3, j = 3, ii = 1, jj = 1; aCount > 0; ii++, jj++, aCount--) {
		if ((doStatus||isFSSET||doDivFlag) && (i >= 0)) { 
			for (; (ii > 0 && i >= 0); i--, ii--) { xStatus = (xStatus-1) & 3; bStatus[i] = xStatus; }
		}
		if (doMac && (j >= 0)) { 
			for (; (jj > 0 && j >= 0); j--, jj--) { xMac = (xMac-1) & 3; bMac[i] = xMac; }
		}
		incPC2(-2);
	}
}

#define getFlagReg1(x)	((x == 3) ? gprF3 : ((x == 2) ? gprF2 : ((x == 1) ? gprF1 : gprF0)))
#define getFlagReg2(x)	((x == bStatus[3]) ? gprESP : ((x == bStatus[2]) ? gprR : ((x == bStatus[1]) ? gprT2 : gprT1)))

// Recompiles Code for Proper Flags on Block Linkings
microVUt(void) mVUsetFlagsRec(int* bStatus, int* bMac) {

	PUSH32R(gprR);   // Backup gprR
	PUSH32R(gprESP); // Backup gprESP

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

	POP32R(gprESP); // Restore gprESP
	POP32R(gprR);   // Restore gprR
}

microVUt(void) mVUincCycles(int x) {
	microVU* mVU = mVUx;
	mVUcycles += x;
	for (int z = 31; z > 0; z--) {
		calcCycles(mVUregs.VF[z].x, x);
		calcCycles(mVUregs.VF[z].y, x);
		calcCycles(mVUregs.VF[z].z, x);
		calcCycles(mVUregs.VF[z].w, x);
	}
	for (int z = 16; z > 0; z--) {
		calcCycles(mVUregs.VI[z], x);
	}
	if (mVUregs.q) {
		if (mVUregs.q > 4) { calcCycles(mVUregs.q, x); if (mVUregs.q <= 4) { mVUinfo |= _doDivFlag; } }
		else { calcCycles(mVUregs.q, x); }
		if (!mVUregs.q) { incQ(); }
	}
	if (mVUregs.p) {
		calcCycles(mVUregs.p, x);
		if (!mVUregs.p) { incP(); }
	}
	calcCycles(mVUregs.r, x);
	calcCycles(mVUregs.xgkick, x);
}

microVUt(void) mVUsetCycles() {
	microVU* mVU = mVUx;
	incCycles(mVUstall);
	if (mVUregsTemp.VFreg[0] == mVUregsTemp.VFreg[1] && !mVUregsTemp.VFreg[0]) { // If upper Op && lower Op write to same VF reg
		mVUinfo |= (mVUregsTemp.r || mVUregsTemp.VI) ? _noWriteVF : _isNOP;		 // If lower Op doesn't modify anything else, then make it a NOP
		mVUregsTemp.VF[1].x = aMax(mVUregsTemp.VF[0].x, mVUregsTemp.VF[1].x);	 // Use max cycles from each vector
		mVUregsTemp.VF[1].y = aMax(mVUregsTemp.VF[0].y, mVUregsTemp.VF[1].y);
		mVUregsTemp.VF[1].z = aMax(mVUregsTemp.VF[0].z, mVUregsTemp.VF[1].z);
		mVUregsTemp.VF[1].w = aMax(mVUregsTemp.VF[0].w, mVUregsTemp.VF[1].w);
	}
	mVUregs.VF[mVUregsTemp.VFreg[0]].reg = mVUregsTemp.VF[0].reg;
	mVUregs.VF[mVUregsTemp.VFreg[1]].reg = mVUregsTemp.VF[1].reg;
	mVUregs.VI[mVUregsTemp.VIreg]		 = mVUregsTemp.VI;
	mVUregs.q							 = mVUregsTemp.q;
	mVUregs.p							 = mVUregsTemp.p;
	mVUregs.r							 = mVUregsTemp.r;
	mVUregs.xgkick						 = mVUregsTemp.xgkick;
}

microVUt(void) mVUdivSet() {
	microVU* mVU = mVUx;
	int flagReg1, flagReg2;
	getFlagReg(flagReg1, fsInstance);
	if (!doStatus) { getFlagReg(flagReg2, fpsInstance); MOV16RtoR(flagReg1, flagReg2); }
	AND16ItoR(flagReg1, 0xfcf);
	OR16MtoR (flagReg1, (uptr)&mVU->divFlag);
}

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

microVUt(void*) __fastcall mVUcompile(u32 startPC, uptr pState) {
	microVU* mVU = mVUx;
	u8* thisPtr = mVUcurProg.x86ptr;
	iPC = startPC / 4;
	
	// Searches for Existing Compiled Block (if found, then returns; else, compile)
	microBlock* pblock = mVUblock[iPC/2]->search((microRegInfo*)pState);
	if (pblock) { return pblock->x86ptrStart; }

	// First Pass
	setCode();
	mVUbranch	= 0;
	mVUstartPC	= iPC;
	mVUcount	= 0;
	mVUcycles	= 1; // Skips "M" phase, and starts counting cycles at "T" stage
	mVU->p		= 0; // All blocks start at p index #0
	mVU->q		= 0; // All blocks start at q index #0
	for (int branch = 0;; ) {
		startLoop();
		mVUopU<vuIndex, 0>();
		if (curI & _Ebit_)	  { branch = 1; }
		if (curI & _MDTbit_)  { branch = 2; }
		if (curI & _Ibit_)	  { incPC(1); mVUinfo |= _isNOP; }
		else				  { incPC(1); mVUopL<vuIndex, 0>(); }
		mVUsetCycles<vuIndex>();
		if (mVU->p)			  { mVUinfo |= _readP; }
		if (mVU->q)			  { mVUinfo |= _readQ; }
		else				  { mVUinfo |= _writeQ; }
		if		(branch >= 2) { mVUinfo |= _isEOB | ((branch == 3) ? _isBdelay : 0); if (mVUbranch) { Console::Error("microVU Warning: Branch in E-bit/Branch delay slot!"); mVUinfo |= _isNOP; } break; }
		else if (branch == 1) { branch = 2; }
		if		(mVUbranch)	  { branch = 3; mVUbranch = 0; mVUinfo |= _isBranch; }
		incPC(1);
		incCycles(1);
		mVUcount++;
	}

	// Sets Up Flag instances
	int bStatus[4]; int bMac[4];
	mVUsetFlags<vuIndex>(bStatus, bMac);
	
	// Second Pass
	iPC = mVUstartPC;
	setCode();
	for (bool x = 1; x; ) {
		if (isEOB)			{ x = 0; }
		if (isNOP)			{ doUpperOp(); if (curI & _Ibit_) { incPC(1); mVU->iReg = curI; } else { incPC(1); } }
		else if (!swapOps)	{ doUpperOp(); incPC(1); mVUopL<vuIndex, 1>(); }
		else				{ incPC(1); mVUopL<vuIndex, 1>(); incPC(-1); doUpperOp(); incPC(1); }

		if (!isBdelay) { incPC(1); }
		else {
			u32* ajmp;
			switch (mVUbranch) {
				case 3: branchCase(JZ32);  // IBEQ
				case 4: branchCase(JGE32); // IBGEZ
				case 5: branchCase(JG32);  // IBGTZ
				case 6: branchCase(JLE32); // IBLEQ
				case 7: branchCase(JL32);  // IBLTZ
				case 8: branchCase(JNZ32); // IBNEQ
				case 1: case 2: // B/BAL
					// ToDo: search for block
					// (remember about global variables and recursion!)
					mVUsetFlagsRec<vuIndex>(bStatus, bMac);
					ajmp = JMP32((uptr)0); 
					break;
				case 9: case 10: // JR/JALR
					
					mVUsetFlagsRec<vuIndex>(bStatus, bMac);

					PUSH32R(gprR); // Backup EDX
					MOV32MtoR(gprT2, (uptr)&mVU->branch);	// Get startPC (ECX first argument for __fastcall)
					AND32ItoR(gprT2, (vuIndex) ? 0x3ff8 : 0xff8);
					MOV32ItoR(gprR, (u32)&pblock->pState);	// Get pState (EDX second argument for __fastcall)

					//ToDo: Add block to block manager and use its address instead of pblock!

					if (!vuIndex) CALLFunc((uptr)mVUcompileVU0); //(u32 startPC, uptr pState)
					else		  CALLFunc((uptr)mVUcompileVU1);
					POP32R(gprR); // Restore
					JMPR(gprT1);  // Jump to rec-code address
					break;
			}
			//mVUcurProg.x86Ptr
			return thisPtr;
		}
	}

	// Do E-bit end stuff here
	incCycles(55); // Ensures Valid P/Q instances
	mVUcycles -= 55;
	if (mVU->q) { SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, 0xe5); }
	SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_Q], xmmPQ);
	SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, mVU->p ? 3 : 2);
	SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_P], xmmPQ);

	//MOV32ItoM((uptr)&mVU->p, mVU->p);
	//MOV32ItoM((uptr)&mVU->q, mVU->q);

	AND32ItoM((uptr)&microVU0.regs->VI[REG_VPU_STAT].UL, (vuIndex ? ~0x100 : ~0x001)); // VBS0/VBS1 flag
	AND32ItoM((uptr)&mVU->regs->vifRegs->stat, ~0x4); // Clear VU 'is busy' signal for vif
	MOV32ItoM((uptr)&mVU->regs->VI[REG_TPC], xPC);
	JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));
	return thisPtr;
}

void* __fastcall mVUcompileVU0(u32 startPC, uptr pState) { return mVUcompile<0>(startPC, pState); }
void* __fastcall mVUcompileVU1(u32 startPC, uptr pState) { return mVUcompile<1>(startPC, pState); }

#endif //PCSX2_MICROVU
