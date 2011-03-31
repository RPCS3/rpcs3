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
// Messages Called at Execution Time...
//------------------------------------------------------------------

static void __fastcall mVUbadOp0(u32 prog, u32 pc)	{ Console.Error("microVU0 Warning: Exiting... Block started with illegal opcode. [%04x] [%x]", pc, prog); }
static void __fastcall mVUbadOp1(u32 prog, u32 pc)	{ Console.Error("microVU1 Warning: Exiting... Block started with illegal opcode. [%04x] [%x]", pc, prog); }
static void __fastcall mVUwarning0(u32 prog)		{ Console.Error("microVU0 Warning: Exiting from Possible Infinite Loop [%04x] [%x]", prog); }
static void __fastcall mVUwarning1(u32 prog)		{ Console.Error("microVU1 Warning: Exiting from Possible Infinite Loop [%04x] [%x]", prog); }
static void __fastcall mVUprintPC1(u32 pc)			{ Console.WriteLn("Block Start PC = 0x%04x", pc); }
static void __fastcall mVUprintPC2(u32 pc)			{ Console.WriteLn("Block End PC   = 0x%04x", pc); }

//------------------------------------------------------------------
// Program Range Checking and Setting up Ranges
//------------------------------------------------------------------

// Used by mVUsetupRange
__fi void mVUcheckIsSame(mV) {
	if (mVU.prog.isSame == -1) {
		mVU.prog.isSame = !memcmp_mmx((u8*)mVUcurProg.data, mVU.regs().Micro, mVU.microMemSize);
	}
	if (mVU.prog.isSame == 0) {
		mVUcacheProg(mVU, *mVU.prog.cur);
		mVU.prog.isSame = 1;
	}
}

// Sets up microProgram PC ranges based on whats been recompiled
void mVUsetupRange(microVU& mVU, s32 pc, bool isStartPC) {
	deque<microRange>*& ranges = mVUcurProg.ranges;
	pc &= mVU.microMemSize - 8;

	if (isStartPC) { // Check if startPC is already within a block we've recompiled
		deque<microRange>::const_iterator it(ranges->begin());
		for ( ; it != ranges->end(); ++it) {
			if ((pc >= it[0].start) && (pc <= it[0].end)) {
				if (it[0].start != it[0].end)
					return; // Last case makes sure its not a 1-opcode EvilBlock
			}
		}
	}
	elif (mVUrange.end != -1) return; // Above case was true

	mVUcheckIsSame(mVU);

	if (isStartPC) {
		microRange mRange = {pc, -1};
		ranges->push_front(mRange);
		return;
	}
	if (mVUrange.start <= pc) {
		mVUrange.end = pc;
		bool mergedRange = 0;
		s32  rStart = mVUrange.start;
		s32  rEnd   = mVUrange.end;
		deque<microRange>::iterator it(ranges->begin());
		for (++it; it != ranges->end(); ++it) {
			if((it[0].start >= rStart) && (it[0].start <= rEnd)) {
				it[0].end   = max(it[0].end, rEnd);
				mergedRange = 1;
			}
			elif((it[0].end >= rStart) && (it[0].end <= rEnd)) {
				it[0].start = min(it[0].start, rStart);
				mergedRange = 1;
			}
		}
		if (mergedRange) {
			//DevCon.WriteLn(Color_Green, "microVU%d: Prog Range Merging", mVU.index);
			ranges->erase(ranges->begin());
		}
	}
	else {
		DevCon.WriteLn(Color_Green, "microVU%d: Prog Range Wrap [%04x] [%d]", mVU.index, mVUrange.start, mVUrange.end);
		mVUrange.end = mVU.microMemSize;
		microRange mRange = {0, pc};
		ranges->push_front(mRange);
	}
}

//------------------------------------------------------------------
// Execute VU Opcode/Instruction (Upper and Lower)
//------------------------------------------------------------------

__ri void doUpperOp(mV) { mVUopU(mVU, 1); mVUdivSet(mVU); }
__ri void doLowerOp(mV) { incPC(-1); mVUopL(mVU, 1); incPC(1); }
__ri void flushRegs(mV) { if (!doRegAlloc) mVU.regAlloc->flushAll(); }

void doIbit(mV) { 
	if (mVUup.iBit) { 
		incPC(-1);
		u32 tempI;
		mVU.regAlloc->clearRegVF(33);

		if (CHECK_VU_OVERFLOW && ((curI & 0x7fffffff) >= 0x7f800000)) {
			DevCon.WriteLn(Color_Green,"microVU%d: Clamping I Reg", mVU.index);
			tempI = (0x80000000 & curI) | 0x7f7fffff; // Clamp I Reg
		}
		else tempI = curI;
		
		xMOV(ptr32[&mVU.getVI(REG_I)], tempI);
		incPC(1);
	} 
}

void doSwapOp(mV) { 
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {
		DevCon.WriteLn(Color_Green, "microVU%d: Backing Up VF Reg [%04x]", getIndex, xPC);

		// Allocate t1 first for better chance of reg-alloc
		const xmm& t1 = mVU.regAlloc->allocReg(mVUlow.VF_write.reg);
		const xmm& t2 = mVU.regAlloc->allocReg();
		xMOVAPS(t2, t1); // Backup VF reg
		mVU.regAlloc->clearNeeded(t1);

		mVUopL(mVU, 1);

		const xmm& t3 = mVU.regAlloc->allocReg(mVUlow.VF_write.reg, mVUlow.VF_write.reg, 0xf, 0);
		xXOR.PS(t2, t3); // Swap new and old values of the register
		xXOR.PS(t3, t2); // Uses xor swap trick...
		xXOR.PS(t2, t3);
		mVU.regAlloc->clearNeeded(t3);

		incPC(1); 
		doUpperOp(mVU);

		const xmm& t4 = mVU.regAlloc->allocReg(-1, mVUlow.VF_write.reg, 0xf);
		xMOVAPS(t4, t2);
		mVU.regAlloc->clearNeeded(t4);
		mVU.regAlloc->clearNeeded(t2);
	}
	else { mVUopL(mVU, 1); incPC(1); flushRegs(mVU); doUpperOp(mVU); }
}

void mVUexecuteInstruction(mV) {
	if   (mVUlow.isNOP)	   { incPC(1); doUpperOp(mVU); flushRegs(mVU); doIbit(mVU);    }
	elif(!mVUinfo.swapOps) { incPC(1); doUpperOp(mVU); flushRegs(mVU); doLowerOp(mVU); }
	else doSwapOp(mVU);
	flushRegs(mVU);
}

//------------------------------------------------------------------
// Warnings / Errors / Illegal Instructions
//------------------------------------------------------------------

// If 1st op in block is a bad opcode, then don't compile rest of block (Dawn of Mana Level 2)
__fi void mVUcheckBadOp(mV) {
	if (mVUinfo.isBadOp && mVUcount == 0) {
		mVUinfo.isEOB = true;
		Console.Warning("microVU Warning: First Instruction of block contains illegal opcode...");
	}
}

// Prints msg when exiting block early if 1st op was a bad opcode (Dawn of Mana Level 2)
__fi void handleBadOp(mV, int count) {
	if (mVUinfo.isBadOp && count == 0) {
		mVUbackupRegs(mVU, true);
		xMOV(gprT2, mVU.prog.cur->idx);
		xMOV(gprT3, xPC);
		if (!isVU1) xCALL(mVUbadOp0);
		else		xCALL(mVUbadOp1);
		mVUrestoreRegs(mVU, true);
	}
}

__ri void branchWarning(mV) {
	incPC(-2);
	if (mVUup.eBit && mVUbranch) {
		incPC(2);
		Console.Error("microVU%d Warning: Branch in E-bit delay slot! [%04x]", mVU.index, xPC);
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

__fi void eBitPass1(mV, int& branch) {
	if (mVUregs.blockType != 1) {
		branch = 1; 
		mVUup.eBit = 1;
	}
}

__ri void eBitWarning(mV) {
	if (mVUpBlock->pState.blockType == 1) Console.Error("microVU%d Warning: Branch, E-bit, Branch! [%04x]",  mVU.index, xPC);
	if (mVUpBlock->pState.blockType == 2) Console.Error("microVU%d Warning: Branch, Branch, Branch! [%04x]", mVU.index, xPC);
	incPC(2);
	if (curI & _Ebit_) {
		DevCon.Warning("microVU%d: E-bit in Branch delay slot! [%04x]", mVU.index, xPC);
		mVUregs.blockType = 1;
	}
	incPC(-2);
}

//------------------------------------------------------------------
// Cycles / Pipeline State / Early Exit from Execution
//------------------------------------------------------------------
__fi void optimizeReg(u8& rState)	 { rState = (rState==1) ? 0 : rState; }
__fi void calcCycles(u8& reg, u8 x)	 { reg = ((reg > x) ? (reg - x) : 0); }
__fi void tCycles(u8& dest, u8& src) { dest = max(dest, src); }
__fi void incP(mV)					 { mVU.p ^= 1; }
__fi void incQ(mV)					 { mVU.q ^= 1; }

// Optimizes the End Pipeline State Removing Unnecessary Info
// If the cycles remaining is just '1', we don't have to transfer it to the next block
// because mVU automatically decrements this number at the start of its loop,
// so essentially '1' will be the same as '0'...
void mVUoptimizePipeState(mV) {
	for (int i = 0; i < 32; i++) {
		optimizeReg(mVUregs.VF[i].x);
		optimizeReg(mVUregs.VF[i].y);
		optimizeReg(mVUregs.VF[i].z);
		optimizeReg(mVUregs.VF[i].w);
	}
	for (int i = 0; i < 16; i++) {
		optimizeReg(mVUregs.VI[i]);
	}
	if (mVUregs.q) { optimizeReg(mVUregs.q); if (!mVUregs.q) { incQ(mVU); } }
	if (mVUregs.p) { optimizeReg(mVUregs.p); if (!mVUregs.p) { incP(mVU); } }
	mVUregs.r = 0; // There are no stalls on the R-reg, so its Safe to discard info
}

void mVUincCycles(mV, int x) {
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
		if (!mVUregs.q)    { incQ(mVU); }
	}
	if (mVUregs.p) {
		calcCycles(mVUregs.p, x);
		if (!mVUregs.p || mVUregsTemp.p) { incP(mVU); }
	}
	if (mVUregs.xgkick) {
		calcCycles(mVUregs.xgkick, x);
		if (!mVUregs.xgkick) { mVUinfo.doXGKICK = 1; }
	}
	calcCycles(mVUregs.r, x);
}

// Helps check if upper/lower ops read/write to same regs...
void cmpVFregs(microVFreg& VFreg1, microVFreg& VFreg2, bool& xVar) {
	if (VFreg1.reg == VFreg2.reg) {
		if ((VFreg1.x && VFreg2.x) || (VFreg1.y && VFreg2.y)
		||	(VFreg1.z && VFreg2.z) || (VFreg1.w && VFreg2.w))
		{ xVar = 1; }
	}
}

void mVUsetCycles(mV) {
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

// Prints Start/End PC of blocks executed, for debugging...
void mVUdebugPrintBlocks(microVU& mVU, bool isEndPC) {
	if (mVUdebugNow) {
		mVUbackupRegs(mVU, true);
		xMOV(gprT2, xPC);
		if (isEndPC) xCALL(mVUprintPC2);
		else		 xCALL(mVUprintPC1);
		mVUrestoreRegs(mVU, true);
	}
}

// vu0 is allowed to exit early, so are dev builds (for inf loops)
__fi bool doEarlyExit(microVU& mVU) {
	return IsDevBuild || !isVU1;
}

// Saves Pipeline State for resuming from early exits
__fi void mVUsavePipelineState(microVU& mVU) {
	u32* lpS = (u32*)&mVU.prog.lpState;
	for (int i = 0; i < (sizeof(microRegInfo)-4)/4; i++, lpS++) {
		xMOV(ptr32[lpS], lpS[0]);
	}
}

// Test cycles to see if we need to exit-early...
void mVUtestCycles(microVU& mVU) {
	iPC = mVUstartPC;
	if (doEarlyExit(mVU)) {
		xCMP(ptr32[&mVU.cycles], 0);
		xForwardJG32 skip;
		if (isVU0) {
			// TEST32ItoM((uptr)&mVU.regs().flags, VUFLAG_MFLAGSET);
			// xFowardJZ32 vu0jmp;
			// mVUbackupRegs(mVU, true);
			// xMOV(gprT2, mVU.prog.cur->idx);
			// xCALL(mVUwarning0); // VU0 is allowed early exit for COP2 Interlock Simulation
			// mVUbackupRegs(mVU, true);
			mVUsavePipelineState(mVU);
			mVUendProgram(mVU, NULL, 0);
			// vu0jmp.SetTarget();
		}
		else {
			mVUbackupRegs(mVU, true);
			xMOV(gprT2, mVU.prog.cur->idx);
			xCALL(mVUwarning1);
			mVUbackupRegs(mVU, true);
			mVUsavePipelineState(mVU);
			mVUendProgram(mVU, NULL, 0);
		}
		skip.SetTarget();
	}
	xSUB(ptr32[&mVU.cycles], mVUcycles);
}

//------------------------------------------------------------------
// Initializing
//------------------------------------------------------------------

// This gets run at the start of every loop of mVU's first pass
__fi void startLoop(mV) {
	if (curI & _Mbit_)	{ DevCon.WriteLn (Color_Green, "microVU%d: M-bit set!", getIndex); }
	if (curI & _Dbit_)	{ DevCon.WriteLn (Color_Green, "microVU%d: D-bit set!", getIndex); }
	if (curI & _Tbit_)	{ DevCon.WriteLn (Color_Green, "microVU%d: T-bit set!", getIndex); }
	memzero(mVUinfo);
	memzero(mVUregsTemp);
}

// Initialize VI Constants (vi15 propagates through blocks)
__fi void mVUinitConstValues(microVU& mVU) {
	for (int i = 0; i < 16; i++) {
		mVUconstReg[i].isValid	= 0;
		mVUconstReg[i].regValue	= 0;
	}
	mVUconstReg[15].isValid  = mVUregs.vi15 >> 31;
	mVUconstReg[15].regValue = mVUconstReg[15].isValid ? (mVUregs.vi15&0xffff) : 0;
}

// Initialize Variables
__fi void mVUinitFirstPass(microVU& mVU, uptr pState, u8* thisPtr) {
	mVUstartPC				= iPC;	// Block Start PC
	mVUbranch				= 0;	// Branch Type
	mVUcount				= 0;	// Number of instructions ran
	mVUcycles				= 0;	// Skips "M" phase, and starts counting cycles at "T" stage
	mVU.p					= 0;	// All blocks start at p index #0
	mVU.q					= 0;	// All blocks start at q index #0
	if ((uptr)&mVUregs != pState) {	// Loads up Pipeline State Info
		memcpy_const((u8*)&mVUregs, (u8*)pState, sizeof(microRegInfo));
	}
	if (doEarlyExit(mVU) && ((uptr)&mVU.prog.lpState != pState)) {
		memcpy_const((u8*)&mVU.prog.lpState, (u8*)pState, sizeof(microRegInfo));
	}
	mVUblock.x86ptrStart	= thisPtr;
	mVUpBlock				= mVUblocks[mVUstartPC/2]->add(&mVUblock);  // Add this block to block manager
	mVUregs.needExactMatch	=(mVUregs.blockType || noFlagOpts) ? 7 : 0; // 1-Op blocks should just always set exactMatch (Sly Cooper)
	mVUregs.blockType		= 0;
	mVUregs.viBackUp		= 0;
	mVUregs.flags			= 0;
	mVUsFlagHack			= CHECK_VU_FLAGHACK;
	mVUinitConstValues(mVU);
}

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

void* mVUcompile(microVU& mVU, u32 startPC, uptr pState) {
	
	microFlagCycles mFC;
	u8*				thisPtr  = x86Ptr;
	const u32		endCount = (((microRegInfo*)pState)->blockType) ? 1 : (mVU.microMemSize / 8);

	// First Pass
	iPC = startPC / 4;
	mVUsetupRange(mVU, startPC, 1); // Setup Program Bounds/Range
	mVU.regAlloc->reset();	// Reset regAlloc
	mVUinitFirstPass(mVU, pState, thisPtr);
	for (int branch = 0; mVUcount < endCount; mVUcount++) {
		incPC(1);
		startLoop(mVU);
		mVUincCycles(mVU, 1);
		mVUopU(mVU, 0);
		mVUcheckBadOp(mVU);
		if (curI & _Ebit_)	  { eBitPass1(mVU, branch); }
		if (curI & _DTbit_)	  { branch = 4; }
		if (curI & _Mbit_)	  { mVUup.mBit = 1; }
		if (curI & _Ibit_)	  { mVUlow.isNOP = 1; mVUup.iBit = 1; }
		else				  { incPC(-1); mVUopL(mVU, 0); incPC(1); }
		mVUsetCycles(mVU);
		mVUinfo.readQ  =  mVU.q;
		mVUinfo.writeQ = !mVU.q;
		mVUinfo.readP  =  mVU.p;
		mVUinfo.writeP = !mVU.p;
		if		(branch >= 2) { mVUinfo.isEOB = 1; if (branch == 3) { mVUinfo.isBdelay = 1; } mVUcount++; branchWarning(mVU); break; }
		else if (branch == 1) { branch = 2; }
		if		(mVUbranch)   { mVUsetFlagInfo(mVU); eBitWarning(mVU); branch = 3; mVUbranch = 0; }
		incPC(1);
	}

	// Fix up vi15 const info for propagation through blocks
	mVUregs.vi15 = (mVUconstReg[15].isValid && doConstProp) ? ((1<<31) | (mVUconstReg[15].regValue&0xffff)) : 0;
	
	mVUsetFlags(mVU, mFC);	   // Sets Up Flag instances
	mVUoptimizePipeState(mVU); // Optimize the End Pipeline State for nicer Block Linking
	mVUdebugPrintBlocks(mVU,0);// Prints Start/End PC of blocks executed, for debugging...
	mVUtestCycles(mVU);		   // Update VU Cycles and Exit Early if Necessary

	// Second Pass
	iPC = mVUstartPC;
	setCode();
	mVUbranch = 0;
	u32 x = 0;
	for (; x < endCount; x++) {
		if (mVUinfo.isEOB)			{ handleBadOp(mVU, x); x = 0xffff; }
		if (mVUup.mBit)				{ xOR(ptr32[&mVU.regs().flags], VUFLAG_MFLAGSET); }
		mVUexecuteInstruction(mVU);
		if (mVUinfo.doXGKICK)		{ mVU_XGKICK_DELAY(mVU, 1); }
		if (isEvilBlock)			{ mVUsetupRange(mVU, xPC, 0); normJumpCompile(mVU, mFC, 1); return thisPtr; }
		else if (!mVUinfo.isBdelay)	{ incPC(1); }
		else {
			mVUsetupRange(mVU, xPC, 0);
			mVUdebugPrintBlocks(mVU,1);
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
	if ((x == endCount) && (x!=1)) { Console.Error("microVU%d: Possible infinite compiling loop!", mVU.index); }

	// E-bit End
	mVUsetupRange(mVU, xPC-8, 0);
	mVUendProgram(mVU, &mFC, 1);
	return thisPtr;
}

// Returns the entry point of the block (compiles it if not found)
__fi void* mVUentryGet(microVU& mVU, microBlockManager* block, u32 startPC, uptr pState) {
	microBlock* pBlock = block->search((microRegInfo*)pState);
	if (pBlock) return pBlock->x86ptrStart;
	else	    return mVUcompile(mVU, startPC, pState);
}

 // Search for Existing Compiled Block (if found, return x86ptr; else, compile and return x86ptr)
__fi void* mVUblockFetch(microVU& mVU, u32 startPC, uptr pState) {

	pxAssumeDev((startPC & 7) == 0,				pxsFmt("microVU%d: unaligned startPC=0x%04x", mVU.index, startPC) );
	pxAssumeDev( startPC < mVU.microMemSize-8,	pxsFmt("microVU%d: invalid startPC=0x%04x",   mVU.index, startPC) );
	startPC &= mVU.microMemSize-8;

	blockCreate(startPC/8);
	return mVUentryGet(mVU, mVUblocks[startPC/8], startPC, pState);
}

// mVUcompileJIT() - Called By JR/JALR during execution
_mVUt void* __fastcall mVUcompileJIT(u32 startPC, uptr ptr) {
	if (doJumpCaching) { // When doJumpCaching, ptr is a microBlock pointer
		microVU& mVU = mVUx;
		microBlock* pBlock = (microBlock*)ptr;
		microJumpCache& jc = pBlock->jumpCache[startPC/8];
		if (jc.prog && jc.prog == mVU.prog.quick[startPC/8].prog) return jc.x86ptrStart;
		void* v = mVUsearchProg<vuIndex>(startPC, (uptr)&pBlock->pStateEnd);
		jc.prog = mVU.prog.quick[startPC/8].prog;
		jc.x86ptrStart = v;
		return v;
	}
	else { // When !doJumpCaching, pBlock param is really a microRegInfo pointer
		//return mVUblockFetch(mVUx, startPC, ptr);
		return mVUsearchProg<vuIndex>(startPC, ptr); // Find and set correct program
	}
}
