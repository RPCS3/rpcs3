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

#define branchCase(JMPcc, nJMPcc, ebitJMP)												\
	mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);									\
	CMP16ItoM((uptr)&mVU->branch, 0);													\
	if (mVUup.eBit) { /* Conditional Branch With E-Bit Set */							\
		mVUendProgram(mVU, 2, xStatus, xMac, xClip);									\
		u8* eJMP = ebitJMP(0);															\
			incPC(1); /* Set PC to First instruction of Non-Taken Side */				\
			MOV32ItoM((uptr)&mVU->regs->VI[REG_TPC].UL, xPC);							\
			JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));							\
		x86SetJ8(eJMP);																	\
		incPC(-4); /* Go Back to Branch Opcode to get branchAddr */						\
		iPC = branchAddr/4;																\
		MOV32ItoM((uptr)&mVU->regs->VI[REG_TPC].UL, xPC);								\
		JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));								\
		return thisPtr;																	\
	}																					\
	else { /* Normal Conditional Branch */												\
		incPC2(1); /* Check if Branch Non-Taken Side has already been recompiled */		\
		if (!mVUblocks[iPC/2]) { mVUblocks[iPC/2] = microBlockManager::AlignedNew(); }	\
		bBlock = mVUblocks[iPC/2]->search((microRegInfo*)&mVUregs);						\
		incPC2(-1);																		\
		if (bBlock)	{ nJMPcc((uptr)bBlock->x86ptrStart - ((uptr)x86Ptr + 6)); }			\
		else		{ ajmp = JMPcc((uptr)0); }											\
	}																					\
	break

#define branchWarning() {																						\
	if (mVUbranch) {																							\
		Console::Error("microVU%d Warning: Branch in E-bit/Branch delay slot! [%04x]", params vuIndex, xPC);	\
		mVUlow.isNOP = 1;																						\
	}																											\
}

#define doBackupVF1() {																	\
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {										\
		DevCon::Status("microVU%d: Backing Up VF Reg [%04x]", params getIndex, xPC);	\
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VF[mVUlow.VF_write.reg].UL[0]);	\
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->xmmVFb, xmmT1);								\
	}																					\
}

#define doBackupVF2() {																	\
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {										\
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->xmmVFb);								\
		SSE_MOVAPS_M128_to_XMM(xmmT2, (uptr)&mVU->regs->VF[mVUlow.VF_write.reg].UL[0]);	\
		SSE_MOVAPS_XMM_to_M128((uptr)&mVU->regs->VF[mVUlow.VF_write.reg].UL[0], xmmT1);	\
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->xmmVFb, xmmT2);								\
	}																					\
}

#define doBackupVF3() {																	\
	if (mVUinfo.backupVF && !mVUlow.noWriteVF) {										\
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->xmmVFb);								\
		SSE_MOVAPS_XMM_to_M128((uptr)&mVU->regs->VF[mVUlow.VF_write.reg].UL[0], xmmT1);	\
	}																					\
}

#define startLoop() {								\
	mVUdebug1();									\
	memset(&mVUinfo,	 0, sizeof(mVUinfo));		\
	memset(&mVUregsTemp, 0, sizeof(mVUregsTemp));	\
}

#define calcCycles(reg, x)	{ reg = ((reg > x) ? (reg - x) : 0); }
#define tCycles(dest, src)	{ dest = aMax(dest, src); }
#define incP()				{ mVU->p = (mVU->p+1) & 1; }
#define incQ()				{ mVU->q = (mVU->q+1) & 1; }
#define doUpperOp()			{ mVUopU(mVU, 1); mVUdivSet(mVU); }
#define doLowerOp()			{ incPC(-1); mVUopL(mVU, 1); incPC(1); }
#define doSwapOp()			{ doBackupVF1(); mVUopL(mVU, 1); doBackupVF2(); incPC(1); doUpperOp(); doBackupVF3(); }
#define doIbit()			{ if (mVUup.iBit) { incPC(-1); MOV32ItoM((uptr)&mVU->regs->VI[REG_I].UL, curI); incPC(1); } }

//------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------

// Used by mVUsetupRange
microVUt(void) mVUcheckIsSame(mV) {

	if (mVU->prog.isSame == -1) {
		mVU->prog.isSame = !memcmp_mmx(mVU->prog.prog[mVU->prog.cur].data, mVU->regs->Micro, mVU->microSize);
	}
	if (mVU->prog.isSame == 0) {
		if (!isVU1)	mVUcacheProg<0>(mVU->prog.cur);
		else		mVUcacheProg<1>(mVU->prog.cur);
		mVU->prog.isSame = 1;
	}
}

// Sets up microProgram PC ranges based on whats been recompiled
microVUt(void) mVUsetupRange(mV, u32 pc) {

	if (mVUcurProg.range[0] == -1) { 
		mVUcurProg.range[0] = (s32)pc;
		mVUcurProg.range[1] = (s32)pc;
	}
	else if (mVUcurProg.range[0] > (s32)pc) {
		mVUcurProg.range[0] = (s32)pc;
		mVUcheckIsSame(mVU);
	}
	else if (mVUcurProg.range[1] < (s32)pc) {
		mVUcurProg.range[1] = (s32)pc;
		mVUcheckIsSame(mVU);
	}
}

// Recompiles Code for Proper Flags and Q/P regs on Block Linkings
microVUt(void) mVUsetupBranch(mV, int* xStatus, int* xMac, int* xClip, int xCycles) {
	mVUprint("mVUsetupBranch");

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

	if (isEbit || !isVU1) { // Clear 'is busy' Flags
		AND32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, (isVU1 ? ~0x100 : ~0x001)); // VBS0/VBS1 flag
		AND32ItoM((uptr)&mVU->regs->vifRegs->stat, ~0x4); // Clear VU 'is busy' signal for vif
	}

	if (isEbit != 2) { // Save PC, and Jump to Exit Point
		mVUsetupRange(mVU, xPC);
		MOV32ItoM((uptr)&mVU->regs->VI[REG_TPC].UL, xPC);
		JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));
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

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

microVUf(void*) __fastcall mVUcompile(u32 startPC, uptr pState) {
	microVU* mVU = mVUx;
	u8* thisPtr = x86Ptr;
	
	if (startPC > ((vuIndex) ? 0x3fff : 0xfff)) { Console::Error("microVU%d: invalid startPC", params vuIndex); }
	startPC &= (vuIndex ? 0x3ff8 : 0xff8);
	
	if (mVUblocks[startPC/8] == NULL) {
		mVUblocks[startPC/8] = microBlockManager::AlignedNew();
	}

	// Searches for Existing Compiled Block (if found, then returns; else, compile)
	microBlock* pBlock = mVUblocks[startPC/8]->search((microRegInfo*)pState);
	if (pBlock) { return pBlock->x86ptrStart; }
	
	// Setup Program Bounds/Range
	mVUsetupRange(mVU, startPC);

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
	pBlock = mVUblocks[startPC/8]->add(&mVUblock); // Add this block to block manager
	mVUpBlock		= pBlock;
	mVUregs.flags	= 0;
	mVUflagInfo		= 0;
	mVUsFlagHack	= CHECK_VU_FLAGHACK;

	for (int branch = 0;  mVUcount < (vuIndex ? (0x3fff/8) : (0xfff/8)); ) {
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
		mVUcount++;
	}

	// Sets Up Flag instances
	int xStatus[4], xMac[4], xClip[4];
	int xCycles = mVUsetFlags(mVU, xStatus, xMac, xClip);
	mVUtestCycles(mVU);

	// Second Pass
	iPC = mVUstartPC;
	setCode();
	mVUbranch = 0;
	int x;
	for (x = 0; x < (vuIndex ? (0x3fff/8) : (0xfff/8)); x++) {
		if (mVUinfo.isEOB)			{ x = 0xffff; }
		if (mVUup.mBit)				{ OR32ItoM((uptr)&mVU->regs->flags, VUFLAG_MFLAGSET); }
		if (mVUlow.isNOP)			{ incPC(1); doUpperOp(); doIbit(); }
		else if (!mVUinfo.swapOps)	{ incPC(1); doUpperOp(); doLowerOp(); }
		else						{ doSwapOp(); }
		if (mVUinfo.doXGKICK)		{ mVU_XGKICK_DELAY(mVU, 1); }
		
		if (!mVUinfo.isBdelay) { incPC(1); }
		else {
			microBlock* bBlock = NULL;
			u32* ajmp = 0;
			mVUsetupRange(mVU, xPC);
			mVUdebugNOW(1);

			switch (mVUbranch) {
				case 3: branchCase(JE32,  JNE32,  JE8);	 // IBEQ
				case 4: branchCase(JGE32, JNGE32, JGE8); // IBGEZ
				case 5: branchCase(JG32,  JNG32,  JG8);	 // IBGTZ
				case 6: branchCase(JLE32, JNLE32, JLE8); // IBLEQ
				case 7: branchCase(JL32,  JNL32,  JL8);	 // IBLTZ
				case 8: branchCase(JNE32, JE32,   JNE8); // IBNEQ
				case 1: case 2: // B/BAL

					mVUprint("mVUcompile B/BAL");
					incPC(-3); // Go back to branch opcode (to get branch imm addr)

					if (mVUup.eBit) { iPC = branchAddr/4; mVUendProgram(mVU, 1, xStatus, xMac, xClip); } // E-bit Branch
					mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);

					if (mVUblocks[branchAddr/8] == NULL)
						mVUblocks[branchAddr/8] = microBlockManager::AlignedNew();

					// Check if branch-block has already been compiled
					pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
					if (pBlock)		   { JMP32((uptr)pBlock->x86ptrStart - ((uptr)x86Ptr + 5)); }
					else if (!vuIndex) { mVUcompileVU0(branchAddr, (uptr)&mVUregs); }
					else			   { mVUcompileVU1(branchAddr, (uptr)&mVUregs); }
					return thisPtr;
				case 9: case 10: // JR/JALR

					mVUprint("mVUcompile JR/JALR");
					incPC(-3); // Go back to jump opcode

					if (mVUup.eBit) { // E-bit Jump
						mVUendProgram(mVU, 2, xStatus, xMac, xClip);
						MOV32MtoR(gprT1, (uptr)&mVU->branch);
						MOV32RtoM((uptr)&mVU->regs->VI[REG_TPC].UL, gprT1);
						JMP32((uptr)mVU->exitFunct - ((uptr)x86Ptr + 5));
						return thisPtr;
					}

					memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
					mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);

					mVUbackupRegs(mVU);
					MOV32MtoR(gprT2, (uptr)&mVU->branch);		 // Get startPC (ECX first argument for __fastcall)
					MOV32ItoR(gprR, (u32)&pBlock->pStateEnd);	 // Get pState (EDX second argument for __fastcall)

					if (!vuIndex) CALLFunc((uptr)mVUcompileVU0); //(u32 startPC, uptr pState)
					else		  CALLFunc((uptr)mVUcompileVU1);
					mVUrestoreRegs(mVU);
					JMPR(gprT1);  // Jump to rec-code address
					return thisPtr;
			}
			// Conditional Branches
			mVUprint("mVUcompile conditional branch");
			if (bBlock) {  // Branch non-taken has already been compiled
				incPC(-3); // Go back to branch opcode (to get branch imm addr)

				if (mVUblocks[branchAddr/8] == NULL)
					mVUblocks[branchAddr/8] = microBlockManager::AlignedNew();

				// Check if branch-block has already been compiled
				pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
				if (pBlock)		   { JMP32((uptr)pBlock->x86ptrStart - ((uptr)x86Ptr + 5)); }
				else if (!vuIndex) { mVUcompileVU0(branchAddr, (uptr)&mVUregs); }
				else			   { mVUcompileVU1(branchAddr, (uptr)&mVUregs); }
			}
			else {
				uptr jumpAddr;
				u32 bPC = iPC; // mVUcompile can modify iPC and mVUregs so back them up
				memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
	
				incPC2(1);  // Get PC for branch not-taken
				if (!vuIndex) mVUcompileVU0(xPC, (uptr)&mVUregs);
				else		  mVUcompileVU1(xPC, (uptr)&mVUregs);

				iPC = bPC;
				incPC(-3); // Go back to branch opcode (to get branch imm addr)
				if (!vuIndex) jumpAddr = (uptr)mVUcompileVU0(branchAddr, (uptr)&pBlock->pStateEnd);
				else		  jumpAddr = (uptr)mVUcompileVU1(branchAddr, (uptr)&pBlock->pStateEnd);
				*ajmp = (jumpAddr - ((uptr)ajmp + 4));
			}
			return thisPtr;
		}
	}
	if (x == (vuIndex?(0x3fff/8):(0xfff/8))) { Console::Error("microVU%d: Possible infinite compiling loop!", params vuIndex); }

	// E-bit End
	mVUendProgram(mVU, 1, xStatus, xMac, xClip);
	return thisPtr;
}

void* __fastcall mVUcompileVU0(u32 startPC, uptr pState) { return mVUcompile<0>(startPC, pState); }
void* __fastcall mVUcompileVU1(u32 startPC, uptr pState) { return mVUcompile<1>(startPC, pState); }

