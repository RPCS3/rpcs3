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

microVUt(void)  mVUincCycles(mV, int x);
microVUr(void*) mVUcompile(microVU* mVU, u32 startPC, uptr pState);

#define blockCreate(addr) { if (!mVUblocks[addr]) mVUblocks[addr] = new microBlockManager(); }
#define sI ((mVUpBlock->pState.needExactMatch & 1) ? 3 : ((mVUpBlock->pState.flags >> 0) & 3))
#define cI ((mVUpBlock->pState.needExactMatch & 4) ? 3 : ((mVUpBlock->pState.flags >> 2) & 3))

microVUt(void) mVUendProgram(mV, microFlagCycles* mFC, int isEbit) {

	int fStatus = (isEbit) ? findFlagInst(mFC->xStatus, 0x7fffffff) : sI;
	int fMac	= (isEbit) ? findFlagInst(mFC->xMac,	0x7fffffff) : 0;
	int fClip	= (isEbit) ? findFlagInst(mFC->xClip,   0x7fffffff) : cI;
	int qInst	= 0;
	int pInst	= 0;
	mVU->regAlloc->flushAll();

	if (isEbit) {
		mVUprint("mVUcompile ebit");
		memset(&mVUinfo,	 0, sizeof(mVUinfo));
		memset(&mVUregsTemp, 0, sizeof(mVUregsTemp));
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

// Recompiles Code for Proper Flags and Q/P regs on Block Linkings
microVUt(void) mVUsetupBranch(mV, microFlagCycles& mFC) {
	mVUprint("mVUsetupBranch");

	// Flush Allocated Regs
	mVU->regAlloc->flushAll();

	// Shuffle Flag Instances
	mVUsetupFlags(mVU, mFC);

	// Shuffle P/Q regs since every block starts at instance #0
	if (mVU->p || mVU->q) { SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, shufflePQ); }
}

void condBranch(mV, microFlagCycles& mFC, microBlock* &pBlock, int JMPcc) {
	using namespace x86Emitter;
	mVUsetupBranch(mVU, mFC);
	xCMP(ptr16[&mVU->branch], 0);
	if (mVUup.eBit) { // Conditional Branch With E-Bit Set
		mVUendProgram(mVU, &mFC, 2);
		xForwardJump8 eJMP((JccComparisonType)JMPcc);
			incPC(1); // Set PC to First instruction of Non-Taken Side
			xMOV(ptr32[&mVU->regs->VI[REG_TPC].UL], xPC);
			xJMP(mVU->exitFunct);
		eJMP.SetTarget();
		incPC(-4); // Go Back to Branch Opcode to get branchAddr
		iPC = branchAddr/4;
		xMOV(ptr32[&mVU->regs->VI[REG_TPC].UL], xPC);
		xJMP(mVU->exitFunct);
		return;
	}
	else { // Normal Conditional Branch
		microBlock* bBlock;
		incPC2(1); // Check if Branch Non-Taken Side has already been recompiled
		blockCreate(iPC/2);
		bBlock = mVUblocks[iPC/2]->search((microRegInfo*)&mVUregs);
		incPC2(-1);
		if (bBlock)	{ // Branch non-taken has already been compiled
			xJcc( xInvertCond((JccComparisonType)JMPcc), bBlock->x86ptrStart );

			// Check if branch-block has already been compiled
			incPC(-3); // Go back to branch opcode (to get branch imm addr)
			blockCreate(branchAddr/8);
			pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
			if (pBlock)	{ xJMP( pBlock->x86ptrStart ); }
			else		{ mVUblockFetch(mVU, branchAddr, (uptr)&mVUregs); }
		}
		else { 
			s32* ajmp = xJcc32((JccComparisonType)JMPcc); 
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
	}
}

void normBranch(mV, microFlagCycles& mFC) {
	using namespace x86Emitter;
	microBlock* pBlock;
	incPC(-3); // Go back to branch opcode (to get branch imm addr)

	// E-bit Branch
	if (mVUup.eBit) { iPC = branchAddr/4; mVUendProgram(mVU, &mFC, 1); return; }
	mVUsetupBranch(mVU, mFC);

	// Check if branch-block has already been compiled
	blockCreate(branchAddr/8);
	pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
	if (pBlock)	{ xJMP(pBlock->x86ptrStart); }
	else		{ mVUcompile(mVU, branchAddr, (uptr)&mVUregs); }
}

void normJump(mV, microFlagCycles& mFC, microBlock* &pBlock) {
	using namespace x86Emitter;
	mVUprint("mVUcompile JR/JALR");
	incPC(-3); // Go back to jump opcode

	if (mVUlow.constJump.isValid) {
		if (mVUup.eBit) { // E-bit Jump
			iPC = (mVUlow.constJump.regValue*2)&(mVU->progSize-1);
			mVUendProgram(mVU, &mFC, 1);
		}
		else {
			int jumpAddr = (mVUlow.constJump.regValue*8)&(mVU->microMemSize-8);
			mVUsetupBranch(mVU, mFC);
			// Check if jump-to-block has already been compiled
			blockCreate(jumpAddr/8);
			pBlock = mVUblocks[jumpAddr/8]->search((microRegInfo*)&mVUregs);
			if (pBlock)	{ xJMP(pBlock->x86ptrStart); }
			else		{ mVUcompile(mVU, jumpAddr, (uptr)&mVUregs); }
		}
		return;
	}

	if (mVUup.eBit) { // E-bit Jump
		mVUendProgram(mVU, &mFC, 2);
		MOV32MtoR(gprT1, (uptr)&mVU->branch);
		MOV32RtoM((uptr)&mVU->regs->VI[REG_TPC].UL, gprT1);
		xJMP(mVU->exitFunct);
		return;
	}

	memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
	mVUsetupBranch(mVU, mFC);

	mVUbackupRegs(mVU);
	MOV32MtoR(gprT2, (uptr)&mVU->branch);	  // Get startPC (ECX first argument for __fastcall)
	MOV32ItoR(gprR, (u32)&pBlock->pStateEnd); // Get pState (EDX second argument for __fastcall)

	if (!mVU->index) xCALL(mVUcompileJIT<0>); //(u32 startPC, uptr pState)
	else			 xCALL(mVUcompileJIT<1>);
	mVUrestoreRegs(mVU);
	JMPR(gprT1);  // Jump to rec-code address
}
