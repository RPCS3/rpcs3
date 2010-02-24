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
// Helper Macros
//------------------------------------------------------------------

#define calcCycles(reg, x)	{ reg = ((reg > x) ? (reg - x) : 0); }
#define optimizeReg(rState) { rState = (rState==1) ? 0 : rState; }
#define tCycles(dest, src)	{ dest = aMax(dest, src); }
#define incP()				{ mVU->p = (mVU->p+1) & 1; }
#define incQ()				{ mVU->q = (mVU->q+1) & 1; }
#define doUpperOp()			{ mVUopU(mVU, 1); mVUdivSet(mVU); }
#define doLowerOp()			{ incPC(-1); mVUopL(mVU, 1); incPC(1); }

//------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------

// Used by mVUsetupRange
_f void mVUcheckIsSame(mV) {

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
_f void mVUsetupRange(mV, s32 pc, bool isStartPC) {

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
			DevCon.WriteLn( Color_StrongBlack, "microVU%d: Prog Range List Full", mVU->index);
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
					//DevCon.Status("microVU%d: Prog Range Merging", mVU->index);
				}
			}
			if (mergedRange) {
				mVUrange[0] = -1;
				mVUrange[1] = -1;
				mVUcurProg.ranges.total--;
			}
		}
		else {
			DevCon.WriteLn("microVU%d: Prog Range Wrap [%04x] [%d]", mVU->index, mVUrange[0], mVUrange[1]);
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
				DevCon.WriteLn( Color_StrongBlack, "microVU%d: Prog Range List Full", mVU->index);
			}
		}
	}
}

_f void startLoop(mV) {
	if (curI & _Mbit_)	{ Console.WriteLn(Color_Green, "microVU%d: M-bit set!", getIndex); }
	if (curI & _Dbit_)	{ DevCon.WriteLn (Color_Green, "microVU%d: D-bit set!", getIndex); }
	if (curI & _Tbit_)	{ DevCon.WriteLn (Color_Green, "microVU%d: T-bit set!", getIndex); }
	memzero(mVUinfo);
	memzero(mVUregsTemp);
}

_f void doIbit(mV) { 
	if (mVUup.iBit) { 
		incPC(-1);
		u32 tempI;
		mVU->regAlloc->clearRegVF(33);

		if (CHECK_VU_OVERFLOW && ((curI & 0x7fffffff) >= 0x7f800000)) {
			Console.WriteLn(Color_Green,"microVU%d: Clamping I Reg", mVU->index);
			tempI = (0x80000000 & curI) | 0x7f7fffff; // Clamp I Reg
		}
		else tempI = curI;
		
		MOV32ItoM((uptr)&mVU->regs->VI[REG_I].UL, tempI);
		incPC(1);
	} 
}

_f void doSwapOp(mV) { 
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {
		DevCon.WriteLn(Color_Green, "microVU%d: Backing Up VF Reg [%04x]", getIndex, xPC);
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

_f void branchWarning(mV) {
	incPC(-2);
	if (mVUup.eBit && mVUbranch) {
		incPC(2);
		Console.Error("microVU%d Warning: Branch in E-bit delay slot! [%04x]", mVU->index, xPC);
		mVUlow.isNOP = 1;
	}
	else incPC(2);
	if (mVUinfo.isBdelay) { // Check if VI Reg Written to on Branch Delay Slot Instruction
		if (mVUlow.VI_write.reg && mVUlow.VI_write.used && !mVUlow.readFlags) {
			mVUlow.backupVI = 1;
			mVUregs.viBackUp = mVUlow.VI_write.reg;
		}
	}
}

_f void eBitPass1(mV, int& branch) {
	if (mVUregs.blockType != 1) {
		branch = 1; 
		mVUup.eBit = 1;
	}
}

_f void eBitWarning(mV) {
	if (mVUpBlock->pState.blockType == 1) Console.Error("microVU%d Warning: Branch, E-bit, Branch! [%04x]",  mVU->index, xPC);
	if (mVUpBlock->pState.blockType == 2) Console.Error("microVU%d Warning: Branch, Branch, Branch! [%04x]", mVU->index, xPC);
	incPC(2);
	if (curI & _Ebit_) {
		DevCon.Warning("microVU%d: E-bit in Branch delay slot! [%04x]", mVU->index, xPC);
		mVUregs.blockType = 1;
	}
	incPC(-2);
}

// Optimizes the End Pipeline State Removing Unnecessary Info
_f void mVUoptimizePipeState(mV) {
	for (int i = 0; i < 32; i++) {
		optimizeReg(mVUregs.VF[i].x);
		optimizeReg(mVUregs.VF[i].y);
		optimizeReg(mVUregs.VF[i].z);
		optimizeReg(mVUregs.VF[i].w);
	}
	for (int i = 0; i < 16; i++) {
		optimizeReg(mVUregs.VI[i]);
	}
	if (mVUregs.q) { optimizeReg(mVUregs.q); if (!mVUregs.q) { incQ(); } }
	if (mVUregs.p) { optimizeReg(mVUregs.p); if (!mVUregs.p) { incP(); } }
	mVUregs.r = 0; // There are no stalls on the R-reg, so its Safe to discard info
}

_f void mVUincCycles(mV, int x) {
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
		else			   { calcCycles(mVUregs.q, x); }
		if (!mVUregs.q) { incQ(); }
	}
	if (mVUregs.p) {
		calcCycles(mVUregs.p, x);
		if (!mVUregs.p || mVUregsTemp.p) { incP(); }
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

_f void mVUsetCycles(mV) {
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

void __fastcall mVUwarning0(mV)		{ Console.Error("microVU0 Warning: Exiting from Possible Infinite Loop [%04x] [%x]", xPC, mVU->prog.cur); }
void __fastcall mVUwarning1(mV)		{ Console.Error("microVU1 Warning: Exiting from Possible Infinite Loop [%04x] [%x]", xPC, mVU->prog.cur); }
void __fastcall mVUprintPC1(u32 PC) { Console.Write("Block PC [%04x] ", PC); }
void __fastcall mVUprintPC2(u32 PC) { Console.Write("[%04x]\n", PC); }

// vu0 is allowed to exit early, so are dev builds (for inf loops)
_f bool doEarlyExit(microVU* mVU) {
	return IsDevBuild || !isVU1;
}

// Saves Pipeline State for resuming from early exits
_f void mVUsavePipelineState(microVU* mVU) {
	u32* lpS = (u32*)&mVU->prog.lpState.vi15;
	for (int i = 0; i < (sizeof(microRegInfo)-4)/4; i++, lpS++) {
		MOV32ItoM((uptr)lpS, lpS[0]);
	}
}

_f void mVUtestCycles(microVU* mVU) {
	//u32* vu0jmp;
	iPC = mVUstartPC;
	mVUdebugNOW(0);
	if (doEarlyExit(mVU)) {
		CMP32ItoM((uptr)&mVU->cycles, 0);
		u32* jmp32 = JG32(0);
		//if (!isVU1) { TEST32ItoM((uptr)&mVU->regs->flags, VUFLAG_MFLAGSET); vu0jmp = JZ32(0); }
			MOV32ItoR(gprT2, (uptr)mVU);
			if (isVU1)  CALLFunc((uptr)mVUwarning1);
			//else		CALLFunc((uptr)mVUwarning0); // VU0 is allowed early exit for COP2 Interlock Simulation
			mVUsavePipelineState(mVU);
			mVUendProgram(mVU, NULL, 0);
		//if (!isVU1) x86SetJ32(vu0jmp);
		x86SetJ32(jmp32);
	}
	SUB32ItoM((uptr)&mVU->cycles, mVUcycles);
}

// Initialize VI Constants (vi15 propagates through blocks)
_f void mVUinitConstValues(microVU* mVU) {
	for (int i = 0; i < 16; i++) {
		mVUconstReg[i].isValid	= 0;
		mVUconstReg[i].regValue	= 0;
	}
	mVUconstReg[15].isValid  = mVUregs.vi15 >> 31;
	mVUconstReg[15].regValue = mVUconstReg[15].isValid ? (mVUregs.vi15&0xffff) : 0;
}

// Initialize Variables
_f void mVUinitFirstPass(microVU* mVU, uptr pState, u8* thisPtr) {
	mVUstartPC				= iPC;	// Block Start PC
	mVUbranch				= 0;	// Branch Type
	mVUcount				= 0;	// Number of instructions ran
	mVUcycles				= 0;	// Skips "M" phase, and starts counting cycles at "T" stage
	mVU->p					= 0;	// All blocks start at p index #0
	mVU->q					= 0;	// All blocks start at q index #0
	if ((uptr)&mVUregs != pState) {	// Loads up Pipeline State Info
		memcpy_const((u8*)&mVUregs, (u8*)pState, sizeof(microRegInfo));
	}
	if (doEarlyExit(mVU) && ((uptr)&mVU->prog.lpState != pState)) {
		memcpy_const((u8*)&mVU->prog.lpState, (u8*)pState, sizeof(microRegInfo));
	}
	mVUblock.x86ptrStart	= thisPtr;
	mVUpBlock				= mVUblocks[mVUstartPC/2]->add(&mVUblock); // Add this block to block manager
	mVUregs.blockType		= 0;
	mVUregs.viBackUp		= 0;
	mVUregs.flags			= 0;
	mVUregs.needExactMatch	= 0;
	mVUsFlagHack			= CHECK_VU_FLAGHACK;
	mVUinitConstValues(mVU);
}

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

_r void* mVUcompile(microVU* mVU, u32 startPC, uptr pState) {
	
	microFlagCycles mFC;
	u8*				thisPtr  = x86Ptr;
	const u32		endCount = (((microRegInfo*)pState)->blockType) ? 1 : (mVU->microMemSize / 8);

	// First Pass
	iPC = startPC / 4;
	mVUsetupRange(mVU, startPC, 1);	// Setup Program Bounds/Range
	mVU->regAlloc->reset();			// Reset regAlloc
	mVUinitFirstPass(mVU, pState, thisPtr);
	for (int branch = 0; mVUcount < endCount; mVUcount++) {
		incPC(1);
		startLoop(mVU);
		mVUincCycles(mVU, 1);
		mVUopU(mVU, 0);
		if (curI & _Ebit_)	  { eBitPass1(mVU, branch); }
		if (curI & _DTbit_)	  { branch = 4; }
		if (curI & _Mbit_)	  { mVUup.mBit = 1; }
		if (curI & _Ibit_)	  { mVUlow.isNOP = 1; mVUup.iBit = 1; }
		else				  { incPC(-1); mVUopL(mVU, 0); incPC(1); }
		mVUsetCycles(mVU);
		mVUinfo.readQ  =  mVU->q;
		mVUinfo.writeQ = !mVU->q;
		mVUinfo.readP  =  mVU->p;
		mVUinfo.writeP = !mVU->p;
		if		(branch >= 2) { mVUinfo.isEOB = 1; if (branch == 3) { mVUinfo.isBdelay = 1; } mVUcount++; branchWarning(mVU); break; }
		else if (branch == 1) { branch = 2; }
		if		(mVUbranch)   { mVUsetFlagInfo(mVU); eBitWarning(mVU); branch = 3; mVUbranch = 0; }
		incPC(1);
	}

	// Fix up vi15 const info for propagation through blocks
	mVUregs.vi15 = (mVUconstReg[15].isValid && CHECK_VU_CONSTPROP) ? ((1<<31) | (mVUconstReg[15].regValue&0xffff)) : 0;
	
	mVUsetFlags(mVU, mFC);	   // Sets Up Flag instances
	mVUoptimizePipeState(mVU); // Optimize the End Pipeline State for nicer Block Linking
	mVUtestCycles(mVU);		   // Update VU Cycles and Exit Early if Necessary

	// Second Pass
	iPC = mVUstartPC;
	setCode();
	mVUbranch = 0;
	u32 x = 0;
	for (; x < endCount; x++) {
		if (mVUinfo.isEOB)			{ x = 0xffff; }
		if (mVUup.mBit)				{ OR32ItoM((uptr)&mVU->regs->flags, VUFLAG_MFLAGSET); }
		if (mVUlow.isNOP)			{ incPC(1); doUpperOp(); doIbit(mVU); }
		else if (!mVUinfo.swapOps)	{ incPC(1); doUpperOp(); doLowerOp(); }
		else						{ doSwapOp(mVU); }
		if (mVUinfo.doXGKICK)		{ mVU_XGKICK_DELAY(mVU, 1); }
		if (!doRegAlloc)			{ mVU->regAlloc->flushAll(); }
		if (isEvilBlock)			{ mVUsetupRange(mVU, xPC, 0); normJumpCompile(mVU, mFC, 1); return thisPtr; }
		else if (!mVUinfo.isBdelay)	{ incPC(1); }
		else {
			mVUsetupRange(mVU, xPC, 0);
			mVUdebugNOW(1);
			incPC(-3); // Go back to branch opcode
			switch (mVUlow.branch) {
				case 1: case 2:  normBranch(mVU, mFC);			  return thisPtr; // B/BAL
				case 9: case 10: normJump  (mVU, mFC);			  return thisPtr; // JR/JALR
				case 3: condBranch(mVU, mFC, Jcc_Equal);		  return thisPtr; // IBEQ
				case 4: condBranch(mVU, mFC, Jcc_GreaterOrEqual); return thisPtr; // IBGEZ
				case 5: condBranch(mVU, mFC, Jcc_Greater);		  return thisPtr; // IBGTZ
				case 6: condBranch(mVU, mFC, Jcc_LessOrEqual);	  return thisPtr; // IBLEQ
				case 7: condBranch(mVU, mFC, Jcc_Less);			  return thisPtr; // IBLTZ
				case 8: condBranch(mVU, mFC, Jcc_NotEqual);		  return thisPtr; // IBNEQ
			}
		}
	}
	if ((x == endCount) && (x!=1)) { Console.Error("microVU%d: Possible infinite compiling loop!", mVU->index); }

	// E-bit End
	mVUsetupRange(mVU, xPC-8, 0);
	mVUendProgram(mVU, &mFC, 1);
	return thisPtr;
}

 // Search for Existing Compiled Block (if found, return x86ptr; else, compile and return x86ptr)
_f void* mVUblockFetch(microVU* mVU, u32 startPC, uptr pState) {

	if (startPC > mVU->microMemSize-8) { DevCon.Error("microVU%d: invalid startPC [%04x]", mVU->index, startPC); }
	startPC &= mVU->microMemSize-8;
	
	blockCreate(startPC/8);
	microBlock* pBlock = mVUblocks[startPC/8]->search((microRegInfo*)pState);
	if (pBlock) { return pBlock->x86ptrStart; }
	else		{ return mVUcompile(mVU, startPC, pState); }
}

// mVUcompileJIT() - Called By JR/JALR during execution
_mVUt void* __fastcall mVUcompileJIT(u32 startPC, uptr pState) {
	return mVUblockFetch(mVUx, startPC, pState);
}
