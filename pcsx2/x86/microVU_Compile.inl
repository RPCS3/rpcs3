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
// Helper Macros
//------------------------------------------------------------------

#define branchCase(JMPcond) branchCaseFunct(mVU, bBlock, xStatus, xMac, xClip, xCycles, ajmp, JMPcond); break

#define branchWarning() {																						\
	if (mVUbranch) {																							\
		Console::Error("microVU%d Warning: Branch in E-bit/Branch delay slot! [%04x]", params mVU->index, xPC);	\
		mVUlow.isNOP = 1;																						\
	}																											\
}

#define startLoop() {																	\
	if (curI & _Mbit_)	{ Console::Status("microVU%d: M-bit set!", params getIndex); }	\
	if (curI & _Dbit_)	{ DevCon::Status ("microVU%d: D-bit set!", params getIndex); }	\
	if (curI & _Tbit_)	{ DevCon::Status ("microVU%d: T-bit set!", params getIndex); }	\
	memset(&mVUinfo,	 0, sizeof(mVUinfo));											\
	memset(&mVUregsTemp, 0, sizeof(mVUregsTemp));										\
}

#define calcCycles(reg, x)	{ reg = ((reg > x) ? (reg - x) : 0); }
#define optimizeReg(rState) { rState = (rState==1) ? 0 : rState; }
#define tCycles(dest, src)	{ dest = aMax(dest, src); }
#define incP()				{ mVU->p = (mVU->p+1) & 1; }
#define incQ()				{ mVU->q = (mVU->q+1) & 1; }
#define doUpperOp()			{ mVUopU(mVU, 1); mVUdivSet(mVU); }
#define doLowerOp()			{ incPC(-1); mVUopL(mVU, 1); incPC(1); }
#define blockCreate(addr)	{ if (!mVUblocks[addr]) mVUblocks[addr] = new microBlockManager(); }

//------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------

microVUt(void) doSwapOp(mV) { 
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {
		DevCon::Status("microVU%d: Backing Up VF Reg [%04x]", params getIndex, xPC);
		int t1 = mVU->regAlloc->allocReg(mVUlow.VF_write.reg);
		int t2 = mVU->regAlloc->allocReg();
		SSE_MOVAPS_XMM_to_XMM(t2, t1);
		mVU->regAlloc->clearNeeded(t1);
		mVUopL(mVU, 1);
		t1 = mVU->regAlloc->allocReg(mVUlow.VF_write.reg, mVUlow.VF_write.reg, 0xf, 0);
		SSE_XORPS_XMM_to_XMM(t2, t1);
		SSE_XORPS_XMM_to_XMM(t1, t2);
		SSE_XORPS_XMM_to_XMM(t2, t1);
		mVU->regAlloc->clearNeeded(t1);
		incPC(1); 
		doUpperOp();
		t1 = mVU->regAlloc->allocReg(-1, mVUlow.VF_write.reg, 0xf);
		SSE_MOVAPS_XMM_to_XMM(t1, t2);
		mVU->regAlloc->clearNeeded(t1);
		mVU->regAlloc->clearNeeded(t2);
	}
	else { mVUopL(mVU, 1); incPC(1); doUpperOp(); }
}

microVUt(void) doIbit(mV) { 
	if (mVUup.iBit) { 
		incPC(-1);
		if (CHECK_VU_OVERFLOW && ((curI & 0x7fffffff) >= 0x7f800000)) {
			Console::Status("microVU%d: Clamping I Reg", params mVU->index);
			int tempI = (0x80000000 & curI) | 0x7f7fffff; // Clamp I Reg
			MOV32ItoM((uptr)&mVU->regs->VI[REG_I].UL, tempI); 
		}
		else MOV32ItoM((uptr)&mVU->regs->VI[REG_I].UL, curI); 
		incPC(1);
	} 
}

// Used by mVUsetupRange
microVUt(void) mVUcheckIsSame(mV) {

	if (mVU->prog.isSame == -1) {
		mVU->prog.isSame = !memcmp_mmx((u8*)mVUcurProg.data, mVU->regs->Micro, mVU->microMemSize);
	}
	if (mVU->prog.isSame == 0) {
		if (!isVU1)	mVUcacheProg<0>(mVU->prog.cur);
		else		mVUcacheProg<1>(mVU->prog.cur);
		mVU->prog.isSame = 1;
	}
}

// Sets up microProgram PC ranges based on whats been recompiled
microVUt(void) mVUsetupRange(mV, s32 pc, bool isStartPC) {

	if (isStartPC || !(mVUrange[1] == -1)) {
		for (int i = 0; i <= mVUcurProg.ranges.total; i++) {
			if ((pc >= mVUcurProg.ranges.range[i][0])
			&&	(pc <= mVUcurProg.ranges.range[i][1])) { return; }
		}
	}

	mVUcheckIsSame(mVU);

	if (isStartPC) {
		if (mVUcurProg.ranges.total < mVUcurProg.ranges.max) {
			mVUcurProg.ranges.total++;
			mVUrange[0] = pc;
		}
		else {
			mVUcurProg.ranges.total = 0;
			mVUrange[0] = 0;
			mVUrange[1] = mVU->microMemSize - 8;
			DevCon::Status("microVU%d: Prog Range List Full", params mVU->index);
		}
	}
	else {
		if (mVUrange[0] <= pc) {
			mVUrange[1] = pc;
			bool mergedRange = 0;
			for (int i = 0; i <= (mVUcurProg.ranges.total-1); i++) {
				int rStart = (mVUrange[0] < 8) ? 0 : (mVUrange[0] - 8);
				int rEnd   = pc;
				if((mVUcurProg.ranges.range[i][1] >= rStart)
				&& (mVUcurProg.ranges.range[i][1] <= rEnd)){
					mVUcurProg.ranges.range[i][1] = pc;
					mergedRange = 1;
					//DevCon::Status("microVU%d: Prog Range Merging", params mVU->index);
				}
			}
			if (mergedRange) {
				mVUrange[0] = -1;
				mVUrange[1] = -1;
				mVUcurProg.ranges.total--;
			}
		}
		else {
			DevCon::Status("microVU%d: Prog Range Wrap [%04x] [%d]", params mVU->index, mVUrange[0], mVUrange[1]);
			mVUrange[1] = mVU->microMemSize - 8;
			if (mVUcurProg.ranges.total < mVUcurProg.ranges.max) {
				mVUcurProg.ranges.total++;
				mVUrange[0] = 0;
				mVUrange[1] = pc;
			}
			else {
				mVUcurProg.ranges.total = 0;
				mVUrange[0] = 0;
				mVUrange[1] = mVU->microMemSize - 8;
				DevCon::Status("microVU%d: Prog Range List Full", params mVU->index);
			}
		}
	}
}

// Optimizes the End Pipeline State Removing Unnecessary Info
microVUt(void) mVUoptimizePipeState(mV) {
	for (int i = 0; i < 32; i++) {
		optimizeReg(mVUregs.VF[i].x);
		optimizeReg(mVUregs.VF[i].y);
		optimizeReg(mVUregs.VF[i].z);
		optimizeReg(mVUregs.VF[i].w);
	}
	for (int i = 0; i < 16; i++) {
		optimizeReg(mVUregs.VI[i]);
	}
	mVUregs.r = 0;
}

// Recompiles Code for Proper Flags and Q/P regs on Block Linkings
microVUt(void) mVUsetupBranch(mV, int* xStatus, int* xMac, int* xClip, int xCycles) {
	mVUprint("mVUsetupBranch");

	// Flush Allocated Regs
	mVU->regAlloc->flushAll();

	// Shuffle Flag Instances
	mVUsetupFlags(mVU, xStatus, xMac, xClip, xCycles);

	// Shuffle P/Q regs since every block starts at instance #0
	if (mVU->p || mVU->q) { SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, shufflePQ); }
}

microVUt(void) mVUincCycles(mV, int x) {
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
		if (mVUregs.q > 4) { calcCycles(mVUregs.q, x); if (mVUregs.q <= 4) { mVUinfo.doDivFlag = 1; } }
		else { calcCycles(mVUregs.q, x); }
		if (!mVUregs.q) { incQ(); }
	}
	if (mVUregs.p) {
		calcCycles(mVUregs.p, x);
		if (!mVUregs.p || (mVUregs.p && mVUregsTemp.p)) { incP(); }
	}
	if (mVUregs.xgkick) {
		calcCycles(mVUregs.xgkick, x);
		if (!mVUregs.xgkick) { mVUinfo.doXGKICK = 1; }
	}
	calcCycles(mVUregs.r, x);
}

#define cmpVFregs(VFreg1, VFreg2, xVar) {	\
	if (VFreg1.reg == VFreg2.reg) {			\
		if ((VFreg1.x && VFreg2.x)			\
		||	(VFreg1.y && VFreg2.y)			\
		||	(VFreg1.z && VFreg2.z)			\
		||	(VFreg1.w && VFreg2.w))			\
		{ xVar = 1; }						\
	}										\
}

microVUt(void) mVUsetCycles(mV) {
	mVUincCycles(mVU, mVUstall);
	// If upper Op && lower Op write to same VF reg:
	if ((mVUregsTemp.VFreg[0] == mVUregsTemp.VFreg[1]) && mVUregsTemp.VFreg[0]) {
		if (mVUregsTemp.r || mVUregsTemp.VI) mVUlow.noWriteVF = 1; 
		else mVUlow.isNOP = 1; // If lower Op doesn't modify anything else, then make it a NOP
	}
	// If lower op reads a VF reg that upper Op writes to:
	if ((mVUlow.VF_read[0].reg || mVUlow.VF_read[1].reg) && mVUup.VF_write.reg) {
		cmpVFregs(mVUup.VF_write, mVUlow.VF_read[0], mVUinfo.swapOps);
		cmpVFregs(mVUup.VF_write, mVUlow.VF_read[1], mVUinfo.swapOps);
	}
	// If above case is true, and upper op reads a VF reg that lower Op Writes to:
	if (mVUinfo.swapOps && ((mVUup.VF_read[0].reg || mVUup.VF_read[1].reg) && mVUlow.VF_write.reg)) {
		cmpVFregs(mVUlow.VF_write, mVUup.VF_read[0], mVUinfo.backupVF);
		cmpVFregs(mVUlow.VF_write, mVUup.VF_read[1], mVUinfo.backupVF);
	}

	tCycles(mVUregs.VF[mVUregsTemp.VFreg[0]].x, mVUregsTemp.VF[0].x);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[0]].y, mVUregsTemp.VF[0].y);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[0]].z, mVUregsTemp.VF[0].z);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[0]].w, mVUregsTemp.VF[0].w);

	tCycles(mVUregs.VF[mVUregsTemp.VFreg[1]].x, mVUregsTemp.VF[1].x);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[1]].y, mVUregsTemp.VF[1].y);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[1]].z, mVUregsTemp.VF[1].z);
	tCycles(mVUregs.VF[mVUregsTemp.VFreg[1]].w, mVUregsTemp.VF[1].w);

	tCycles(mVUregs.VI[mVUregsTemp.VIreg],	mVUregsTemp.VI);
	tCycles(mVUregs.q,						mVUregsTemp.q);
	tCycles(mVUregs.p,						mVUregsTemp.p);
	tCycles(mVUregs.r,						mVUregsTemp.r);
	tCycles(mVUregs.xgkick,					mVUregsTemp.xgkick);
}

#define sI ((mVUpBlock->pState.needExactMatch & 0x000f) ? 0 : ((mVUpBlock->pState.flags >> 0) & 3))
#define cI ((mVUpBlock->pState.needExactMatch & 0x0f00) ? 0 : ((mVUpBlock->pState.flags >> 2) & 3))

microVUt(void) mVUendProgram(mV, int isEbit, int* xStatus, int* xMac, int* xClip) {

	int fStatus = (isEbit) ? findFlagInst(xStatus, 0x7fffffff) : sI;
	int fMac	= (isEbit) ? findFlagInst(xMac,	   0x7fffffff) : 0;
	int fClip	= (isEbit) ? findFlagInst(xClip,   0x7fffffff) : cI;
	int qInst	= 0;
	int pInst	= 0;
	mVU->regAlloc->flushAll();

	if (isEbit) {
		mVUprint("mVUcompile ebit");
		memset(&mVUinfo, 0, sizeof(mVUinfo));
		mVUincCycles(mVU, 100); // Ensures Valid P/Q instances (And sets all cycle data to 0)
		mVUcycles -= 100;
		qInst = mVU->q;
		pInst = mVU->p;
		if (mVUinfo.doDivFlag) {
			sFLAG.doFlag = 1;
			sFLAG.write  = fStatus;
			mVUdivSet(mVU);
		}
		if (mVUinfo.doXGKICK) { mVU_XGKICK_DELAY(mVU, 1); }
	}

	// Save P/Q Regs
	if (qInst) { SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, 0xe5); }
	SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_Q].UL, xmmPQ);
	if (isVU1) {
		SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, pInst ? 3 : 2);
		SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_P].UL, xmmPQ);
	}

	// Save Flag Instances
	mVUallocSFLAGc(gprT1, gprT2, fStatus);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_STATUS_FLAG].UL,	gprT1);
	mVUallocMFLAGa(mVU, gprT1, fMac);
	mVUallocCFLAGa(mVU, gprT2, fClip);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_MAC_FLAG].UL,	gprT1);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_CLIP_FLAG].UL,	gprT2);

	if (isEbit || isVU1) { // Clear 'is busy' Flags
		AND32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, (isVU1 ? ~0x100 : ~0x001)); // VBS0/VBS1 flag
		AND32ItoM((uptr)&mVU->regs->vifRegs->stat, ~0x4); // Clear VU 'is busy' signal for vif
	}

	if (isEbit != 2) { // Save PC, and Jump to Exit Point
		MOV32ItoM((uptr)&mVU->regs->VI[REG_TPC].UL, xPC);
		JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));
	}
}

void branchCaseFunct(mV, microBlock* &bBlock, int* xStatus, int* xMac, int* xClip, int &xCycles, s32* &ajmp, int JMPcc) {
	using namespace x86Emitter;
	mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);
	xCMP(ptr16[&mVU->branch], 0);
	if (mVUup.eBit) { // Conditional Branch With E-Bit Set
		mVUendProgram(mVU, 2, xStatus, xMac, xClip);
		xForwardJump8 eJMP((JccComparisonType)JMPcc);
			incPC(1); // Set PC to First instruction of Non-Taken Side
			xMOV(ptr32[&mVU->regs->VI[REG_TPC].UL], xPC);
			xJMP(mVU->exitFunct);
		eJMP.SetTarget();
		incPC(-4); // Go Back to Branch Opcode to get branchAddr
		iPC = branchAddr/4;
		xMOV(ptr32[&mVU->regs->VI[REG_TPC].UL], xPC);
		xJMP(mVU->exitFunct);
	}
	else { // Normal Conditional Branch
		incPC2(1); // Check if Branch Non-Taken Side has already been recompiled
		blockCreate(iPC/2);
		bBlock = mVUblocks[iPC/2]->search((microRegInfo*)&mVUregs);
		incPC2(-1);
		if (bBlock)	{ xJcc( xInvertCond((JccComparisonType)JMPcc), bBlock->x86ptrStart ); }
		else		{ ajmp = xJcc32((JccComparisonType)JMPcc); }
	}
}

void __fastcall mVUwarning0(u32 PC) { Console::Error("microVU0 Warning: Exiting from Possible Infinite Loop [%04x]", params PC); }
void __fastcall mVUwarning1(u32 PC) { Console::Error("microVU1 Warning: Exiting from Possible Infinite Loop [%04x]", params PC); }
void __fastcall mVUprintPC1(u32 PC) { Console::Write("Block PC [%04x] ", params PC); }
void __fastcall mVUprintPC2(u32 PC) { Console::Write("[%04x]\n", params PC); }

microVUt(void) mVUtestCycles(mV) {
	iPC = mVUstartPC;
	mVUdebugNOW(0);
	SUB32ItoM((uptr)&mVU->cycles, mVUcycles);
	u32* jmp32 = JG32(0);
		MOV32ItoR(gprT2, xPC);
		if (isVU1)  CALLFunc((uptr)mVUwarning1);
		//else		CALLFunc((uptr)mVUwarning0); // VU0 is allowed early exit for COP2 Interlock Simulation
		MOV32ItoR(gprR, Roffset); // Restore gprR
		mVUendProgram(mVU, 0, NULL, NULL, NULL);
	x86SetJ32(jmp32);
}

microVUt(void) mVUinitConstValues(mV) {
	for (int i = 0; i < 16; i++) {
		mVUconstReg[i].isValid	= 0;
		mVUconstReg[i].regValue	= 0;
	}
	mVUconstReg[15].isValid  = mVUregs.vi15 >> 31;
	mVUconstReg[15].regValue = mVUconstReg[15].isValid ? (mVUregs.vi15&0xffff) : 0;
}

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

microVUr(void*) mVUcompile(microVU* mVU, u32 startPC, uptr pState) {
	
	using namespace x86Emitter;
	microBlock*	pBlock	 = NULL;
	u8*			thisPtr  = x86Ptr;
	const u32	endCount = (mVU->microMemSize / 8) - 1;

	// Setup Program Bounds/Range
	mVUsetupRange(mVU, startPC, 1);
	
	// Reset regAlloc
	mVU->regAlloc->reset();

	// First Pass
	iPC = startPC / 4;
	setCode();
	mVUbranch	= 0;
	mVUstartPC	= iPC;
	mVUcount	= 0;
	mVUcycles	= 0; // Skips "M" phase, and starts counting cycles at "T" stage
	mVU->p		= 0; // All blocks start at p index #0
	mVU->q		= 0; // All blocks start at q index #0
	memcpy_fast(&mVUregs, (microRegInfo*)pState, sizeof(microRegInfo)); // Loads up Pipeline State Info
	mVUblock.x86ptrStart = thisPtr;
	pBlock			= mVUblocks[startPC/8]->add(&mVUblock); // Add this block to block manager
	mVUpBlock		= pBlock;
	mVUregs.flags	= 0;
	mVUflagInfo		= 0;
	mVUsFlagHack	= CHECK_VU_FLAGHACK;

	mVUinitConstValues(mVU);

	for (int branch = 0; mVUcount < endCount; mVUcount++) {
		incPC(1);
		startLoop();
		mVUincCycles(mVU, 1);
		mVUopU(mVU, 0);
		if (curI & _Ebit_)	  { branch = 1; mVUup.eBit = 1; }
		if (curI & _DTbit_)	  { branch = 4; }
		if (curI & _Mbit_)	  { mVUup.mBit = 1; }
		if (curI & _Ibit_)	  { mVUlow.isNOP = 1; mVUup.iBit = 1; }
		else				  { incPC(-1); mVUopL(mVU, 0); incPC(1); }
		mVUsetCycles(mVU);
		mVUinfo.readQ  =  mVU->q;
		mVUinfo.writeQ = !mVU->q;
		mVUinfo.readP  =  mVU->p;
		mVUinfo.writeP = !mVU->p;
		if		(branch >= 2) { mVUinfo.isEOB = 1; if (branch == 3) { mVUinfo.isBdelay = 1; } mVUcount++; branchWarning(); break; }
		else if (branch == 1) { branch = 2; }
		if		(mVUbranch)   { mVUsetFlagInfo(mVU); branch = 3; mVUbranch = 0; }
		incPC(1);
	}

	// Sets Up Flag instances
	int xStatus[4], xMac[4], xClip[4];
	int xCycles = mVUsetFlags(mVU, xStatus, xMac, xClip);
	mVUtestCycles(mVU);

	// Fix up vi15 const info for propagation through blocks
	mVUregs.vi15 = (mVUconstReg[15].isValid && !CHECK_VU_CONSTHACK) ? ((1<<31) | (mVUconstReg[15].regValue&0xffff)) : 0;

	// Optimize the End Pipeline State for nicer Block Linking
	mVUoptimizePipeState(mVU);

	// Second Pass
	iPC = mVUstartPC;
	setCode();
	mVUbranch = 0;
	uint x;
	for (x = 0; x < endCount; x++) {
		if (mVUinfo.isEOB)			{ x = 0xffff; }
		if (mVUup.mBit)				{ OR32ItoM((uptr)&mVU->regs->flags, VUFLAG_MFLAGSET); }
		if (mVUlow.isNOP)			{ incPC(1); doUpperOp(); doIbit(mVU); }
		else if (!mVUinfo.swapOps)	{ incPC(1); doUpperOp(); doLowerOp(); }
		else						{ doSwapOp(mVU); }
		if (mVUinfo.doXGKICK)		{ mVU_XGKICK_DELAY(mVU, 1); }
		if (!doRegAlloc)			{ mVU->regAlloc->flushAll(); }

		if (!mVUinfo.isBdelay) { incPC(1); }
		else {
			microBlock* bBlock = NULL;
			s32* ajmp = 0;
			mVUsetupRange(mVU, xPC, 0);
			mVUdebugNOW(1);

			switch (mVUbranch) {
				case 3: branchCase(Jcc_Equal);			// IBEQ
				case 4: branchCase(Jcc_GreaterOrEqual); // IBGEZ
				case 5: branchCase(Jcc_Greater);		// IBGTZ
				case 6: branchCase(Jcc_LessOrEqual);	// IBLEQ
				case 7: branchCase(Jcc_Less);			// IBLTZ
				case 8: branchCase(Jcc_NotEqual);		// IBNEQ
				case 1: case 2: // B/BAL

					mVUprint("mVUcompile B/BAL");
					incPC(-3); // Go back to branch opcode (to get branch imm addr)

					if (mVUup.eBit) { iPC = branchAddr/4; mVUendProgram(mVU, 1, xStatus, xMac, xClip); } // E-bit Branch
					mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);

					// Check if branch-block has already been compiled
					blockCreate(branchAddr/8);
					pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
					if (pBlock)	{ xJMP(pBlock->x86ptrStart); }
					else		{ mVUcompile(mVU, branchAddr, (uptr)&mVUregs); }
					return thisPtr;
				case 9: case 10: // JR/JALR

					mVUprint("mVUcompile JR/JALR");
					incPC(-3); // Go back to jump opcode

					if (mVUlow.constJump.isValid) {
						if (mVUup.eBit) { // E-bit Jump
							iPC = (mVUlow.constJump.regValue*2)&(mVU->progSize-1);
							mVUendProgram(mVU, 1, xStatus, xMac, xClip);
						}
						else {
							int jumpAddr = (mVUlow.constJump.regValue*8)&(mVU->microMemSize-8);
							mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);
							// Check if jump-to-block has already been compiled
							blockCreate(jumpAddr/8);
							pBlock = mVUblocks[jumpAddr/8]->search((microRegInfo*)&mVUregs);
							if (pBlock)	{ xJMP(pBlock->x86ptrStart); }
							else		{ mVUcompile(mVU, jumpAddr, (uptr)&mVUregs); }
						}
						return thisPtr;
					}

					if (mVUup.eBit) { // E-bit Jump
						mVUendProgram(mVU, 2, xStatus, xMac, xClip);
						MOV32MtoR(gprT1, (uptr)&mVU->branch);
						MOV32RtoM((uptr)&mVU->regs->VI[REG_TPC].UL, gprT1);
						xJMP(mVU->exitFunct);
						return thisPtr;
					}

					memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
					mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);

					mVUbackupRegs(mVU);
					MOV32MtoR(gprT2, (uptr)&mVU->branch);	  // Get startPC (ECX first argument for __fastcall)
					MOV32ItoR(gprR, (u32)&pBlock->pStateEnd); // Get pState (EDX second argument for __fastcall)

					if (!mVU->index) xCALL(mVUcompileJIT<0>); //(u32 startPC, uptr pState)
					else			 xCALL(mVUcompileJIT<1>);
					mVUrestoreRegs(mVU);
					JMPR(gprT1);  // Jump to rec-code address
					return thisPtr;
			}
			// Conditional Branches
			mVUprint("mVUcompile conditional branch");
			if (mVUup.eBit) return thisPtr; // Handled in Branch Case
			if (bBlock) {  // Branch non-taken has already been compiled
				incPC(-3); // Go back to branch opcode (to get branch imm addr)

				// Check if branch-block has already been compiled
				blockCreate(branchAddr/8);
				pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
				if (pBlock)	{ xJMP( pBlock->x86ptrStart ); }
				else		{ mVUblockFetch(mVU, branchAddr, (uptr)&mVUregs); }
			}
			else {
				uptr jumpAddr;
				u32 bPC = iPC; // mVUcompile can modify iPC and mVUregs so back them up
				memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
	
				incPC2(1);  // Get PC for branch not-taken
				mVUcompile(mVU, xPC, (uptr)&mVUregs);

				iPC = bPC;
				incPC(-3); // Go back to branch opcode (to get branch imm addr)
				jumpAddr = (uptr)mVUblockFetch(mVU, branchAddr, (uptr)&pBlock->pStateEnd);
				*ajmp = (jumpAddr - ((uptr)ajmp + 4));
			}
			return thisPtr;
		}
	}
	if (x == endCount) { Console::Error("microVU%d: Possible infinite compiling loop!", params mVU->index); }

	// E-bit End
	mVUsetupRange(mVU, xPC-8, 0);
	mVUendProgram(mVU, 1, xStatus, xMac, xClip);
	return thisPtr;
}

 // Search for Existing Compiled Block (if found, return x86ptr; else, compile and return x86ptr)
microVUt(void*) mVUblockFetch(microVU* mVU, u32 startPC, uptr pState) {

	using namespace x86Emitter;
	if (startPC > mVU->microMemSize-8) { DevCon::Error("microVU%d: invalid startPC [%04x]", params mVU->index, startPC); }
	startPC &= mVU->microMemSize-8;
	
	blockCreate(startPC/8);
	microBlock* pBlock = mVUblocks[startPC/8]->search((microRegInfo*)pState);
	if (pBlock) { return pBlock->x86ptrStart; }
	else		{ return mVUcompile(mVU, startPC, pState); }
}

// mVUcompileJIT() - Called By JR/JALR during execution
microVUx(void*) __fastcall mVUcompileJIT(u32 startPC, uptr pState) {
	return mVUblockFetch(mVUx, startPC, pState);
}

