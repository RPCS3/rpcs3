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

// Micro VU recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "microVU.h"

//------------------------------------------------------------------
// Micro VU - Global Variables
//------------------------------------------------------------------

PCSX2_ALIGNED16(microVU microVU0);
PCSX2_ALIGNED16(microVU microVU1);

declareAllVariables // Declares All Global Variables :D

//------------------------------------------------------------------
// Micro VU - Main Functions
//------------------------------------------------------------------

// Only run this once per VU! ;)
microVUf(void) mVUinit(VURegs* vuRegsPtr) {

	microVU* mVU	= mVUx;
	mVU->regs		= vuRegsPtr;
	mVU->index		= vuIndex;
	mVU->microMemSize	= (vuIndex ? 0x4000 : 0x1000);
	mVU->progMemSize	= (vuIndex ? 0x4000 : 0x1000) / 4;
	mVU->cache		= NULL;
	mVU->cacheSize	= mVUcacheSize;
	memset(&mVU->prog, 0, sizeof(mVU->prog));
	mVUprint((vuIndex) ? "microVU1: init" : "microVU0: init");

	mVU->cache = SysMmapEx((vuIndex ? 0x5f240000 : 0x5e240000), mVU->cacheSize + 0x1000, 0, (vuIndex ? "Micro VU1" : "Micro VU0"));
	if ( mVU->cache == NULL ) throw Exception::OutOfMemory(fmt_string( "microVU Error: Failed to allocate recompiler memory! (addr: 0x%x)", (u32)mVU->cache));
	
	mVUemitSearch();
	mVUreset<vuIndex>();
}

// Resets Rec Data
microVUx(void) mVUreset() {

	microVU* mVU = mVUx;
	mVUprint((vuIndex) ? "microVU1: reset" : "microVU0: reset");

	// Delete Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progMemSize / 2); j++) {
			microBlockManager::Delete( mVU->prog.prog[i].block[j] );
		}
	}

	// Dynarec Cache
	memset(mVU->cache, 0xcc, mVU->cacheSize + 0x1000);

	// Setup Entrance/Exit Points
	x86SetPtr(mVU->cache);
	mVUdispatcherA(mVU);
	mVUdispatcherB(mVU);

	// Clear All Program Data
	memset(&mVU->prog, 0, sizeof(mVU->prog));

	// Program Variables
	mVU->prog.isSame = -1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
	memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));
	//mVU->prog.lpState = &mVU->prog.prog[15].allocInfo.block.pState; // Blank Pipeline State (ToDo: finish implementation)

	// Setup Dynarec Cache Limits for Each Program
	u8* z = (mVU->cache + 0x1000); // Dispatcher Code is in first page of cache
	for (int i = 0; i <= mVU->prog.max; i++) {
		mVU->prog.prog[i].x86start = z;
		mVU->prog.prog[i].x86ptr = z;
		z += (mVU->cacheSize / (mVU->prog.max + 1));
		mVU->prog.prog[i].x86end = z;
		mVU->prog.prog[i].range[0] = -1; // Set range to 
		mVU->prog.prog[i].range[1] = -1; // indeterminable status
	}
}

// Free Allocated Resources
microVUf(void) mVUclose() {

	microVU* mVU = mVUx;
	mVUprint((vuIndex) ? "microVU1: close" : "microVU0: close");

	if ( mVU->cache ) { HostSys::Munmap( mVU->cache, mVU->cacheSize ); mVU->cache = NULL; }

	// Delete Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progMemSize / 2); j++) {
			if (mVU->prog.prog[i].block[j]) {
				microBlockManager::Delete( mVU->prog.prog[i].block[j] );
			}
		}
	}
}

// Clears Block Data in specified range
microVUf(void) mVUclear(u32 addr, u32 size) {
	microVU* mVU = mVUx;
	if (!mVU->prog.cleared) {
		memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));
		mVU->prog.cleared = 1; // Next execution searches/creates a new microprogram
	}
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Clears program data (Sets used to 1 because calling this function implies the program will be used at least once)
microVUf(void) mVUclearProg(int progIndex) {
	microVU* mVU = mVUx;
	mVU->prog.prog[progIndex].used = 1;
	mVU->prog.prog[progIndex].last_used = 3;
	mVU->prog.prog[progIndex].range[0] = -1;
	mVU->prog.prog[progIndex].range[1] = -1;
	mVU->prog.prog[progIndex].x86ptr = mVU->prog.prog[progIndex].x86start;
	for (u32 i = 0; i < (mVU->progMemSize / 2); i++) {
		if (mVU->prog.prog[progIndex].block[i])
			mVU->prog.prog[progIndex].block[i]->reset();
	}
}

// Caches Micro Program
microVUf(void) mVUcacheProg(int progIndex) {
	microVU* mVU = mVUx;
	memcpy_fast(mVU->prog.prog[progIndex].data, mVU->regs->Micro, mVU->microMemSize);
	mVUdumpProg(progIndex);
}

// Finds the least used program, (if program list full clears and returns an old program; if not-full, returns free program)
microVUf(int) mVUfindLeastUsedProg() {
	microVU* mVU = mVUx;
	if (mVU->prog.total < mVU->prog.max) {
		mVU->prog.total++;
		mVUcacheProg<vuIndex>(mVU->prog.total); // Cache Micro Program
		mVU->prog.prog[mVU->prog.total].used = 1;
		mVU->prog.prog[mVU->prog.total].last_used = 3;
		Console::Notice("microVU%d: Cached MicroPrograms = %d", params vuIndex, mVU->prog.total+1);
		return mVU->prog.total;
	}
	else {
	
		int startidx = (mVU->prog.cur + 1) & mVU->prog.max;
		int endidx = mVU->prog.cur;
		int smallidx = startidx;
		u32 smallval = mVU->prog.prog[startidx].used;

		for (int i = startidx; i != endidx; i = (i+1)&mVU->prog.max) {
			u32 used = mVU->prog.prog[i].used;
			if (smallval > used) {
				smallval = used;
				smallidx = i;
			}
		}

		mVUclearProg<vuIndex>(smallidx); // Clear old data if overwriting old program
		mVUcacheProg<vuIndex>(smallidx); // Cache Micro Program
		//Console::Notice("microVU%d: Overwriting existing program in slot %d [%d times used]", params vuIndex, smallidx, smallval );
		return smallidx;
	}
}

// mVUvsyncUpdate -->
// This should be run at 30fps intervals from Counters.cpp (or 60fps works too, but 30fps is
// probably all we need for accurate results)
//
// To fix the program cache to more efficiently dispose of "obsolete" programs, we need to use a
// frame-based decrementing system in combination with a program-execution-based incrementing
// system.  In english:  if last_used >= 2 it means the program has been used for the current
// or prev frame.  if it's 0, the program hasn't been used for a while.
microVUf(void) mVUvsyncUpdate() {

	microVU* mVU = mVUx;
	if (mVU->prog.total < mVU->prog.max) return;

	for (int i = 0; i <= mVU->prog.total; i++) {
		if (mVU->prog.prog[i].last_used != 0) {
			if (mVU->prog.prog[i].last_used >= 3) {

				if (mVU->prog.prog[i].used < 0x4fffffff) // program has been used recently. Give it a
					mVU->prog.prog[i].used += 0x200;	 // 'weighted' bonus signifying it's importance
			}
			mVU->prog.prog[i].last_used--;
		}
		else mVU->prog.prog[i].used /= 2; // penalize unused programs.
	}
}

// Compare Cached microProgram to mVU->regs->Micro
microVUf(int) mVUcmpProg(int progIndex, bool progUsed, bool needOverflowCheck, bool cmpWholeProg) {
	microVU* mVU = mVUx;
	
	if (progUsed) {
		if (cmpWholeProg && (!memcmp_mmx((u8*)mVUprogI.data, mVU->regs->Micro, mVU->microMemSize)) ||
		  (!cmpWholeProg && (!memcmp_mmx((u8*)mVUprogI.data + mVUprogI.range[0], (u8*)mVU->regs->Micro + mVUprogI.range[0], ((mVUprogI.range[1] + 8) - mVUprogI.range[0]))))) {
			mVU->prog.cur = progIndex;
			mVU->prog.cleared = 0;
			mVU->prog.isSame = cmpWholeProg ? 1 : -1;
			mVU->prog.prog[progIndex].last_used = 3;
			if (needOverflowCheck && (mVU->prog.prog[progIndex].used < 0x7fffffff)) {
				mVU->prog.prog[progIndex].used++; // increment 'used' (avoiding overflows if necessary)
			}
			return 1;
		}
	}
	return 0;
}

// Searches for Cached Micro Program and sets prog.cur to it (returns 1 if program found, else returns 0)
microVUf(int) mVUsearchProg() {
	microVU* mVU = mVUx;
	
	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (mVUcmpProg<vuIndex>(i, !!mVU->prog.prog[i].used, 1, 0))
				return 1; // Check Recently Used Programs
		}
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (mVUcmpProg<vuIndex>(i,  !mVU->prog.prog[i].used, 0, 0))
				return 1; // Check Older Programs
		}
		mVU->prog.cur = mVUfindLeastUsedProg<vuIndex>(); // If cleared and program not found, make a new program instance
		mVU->prog.cleared = 0;
		mVU->prog.isSame  = 1;
		return 0;
	}
	mVU->prog.prog[mVU->prog.cur].used++;
	mVU->prog.prog[mVU->prog.cur].last_used = 3;
	return 1; // If !cleared, then we're still on the same program as last-time ;)
}

//------------------------------------------------------------------
// JIT recompiler -- called from recompiled code when register jumps are made.
//------------------------------------------------------------------

microVUf(void*) __fastcall mVUcompileJIT(u32 startPC, uptr pState) {

	return mVUblockFetch( mVUx, startPC, pState );
}

//------------------------------------------------------------------
// Recompiler
//------------------------------------------------------------------

static void* __fastcall mVUcompile( microVU* mVU, u32 startPC, uptr pState )
{
	using namespace x86Emitter;

	// Setup Program Bounds/Range
	mVUsetupRange(mVU, startPC);

	microBlock*	pBlock	= NULL;
	u8*			thisPtr	= x86Ptr;

	const u32 microSizeDiv8 = (mVU->microMemSize-1) / 8;

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

	for (int branch = 0;  mVUcount < microSizeDiv8; ) {
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
	uint x;
	for (x = 0; x < microSizeDiv8; x++) {
		if (mVUinfo.isEOB)			{ x = 0xffff; }
		if (mVUup.mBit)				{ OR32ItoM((uptr)&mVU->regs->flags, VUFLAG_MFLAGSET); }
		if (mVUlow.isNOP)			{ incPC(1); doUpperOp(); doIbit(); }
		else if (!mVUinfo.swapOps)	{ incPC(1); doUpperOp(); doLowerOp(); }
		else						{ doSwapOp(); }
		if (mVUinfo.doXGKICK)		{ mVU_XGKICK_DELAY(mVU, 1); }
		
		if (!mVUinfo.isBdelay) { incPC(1); }
		else {
			microBlock* bBlock = NULL;
			s32* ajmp = 0;
			mVUsetupRange(mVU, xPC);
			mVUdebugNOW(1);

			switch (mVUbranch) {
				case 3: branchCase(Jcc_Equal);	 // IBEQ
				case 4: branchCase(Jcc_GreaterOrEqual); // IBGEZ
				case 5: branchCase(Jcc_Greater);	 // IBGTZ
				case 6: branchCase(Jcc_LessOrEqual); // IBLEQ
				case 7: branchCase(Jcc_Less);	 // IBLTZ
				case 8: branchCase(Jcc_NotEqual); // IBNEQ
				case 1: case 2: // B/BAL

					mVUprint("mVUcompile B/BAL");
					incPC(-3); // Go back to branch opcode (to get branch imm addr)

					if (mVUup.eBit) { iPC = branchAddr/4; mVUendProgram(mVU, 1, xStatus, xMac, xClip); } // E-bit Branch
					mVUsetupBranch(mVU, xStatus, xMac, xClip, xCycles);

					if (mVUblocks[branchAddr/8] == NULL)
						mVUblocks[branchAddr/8] = microBlockManager::AlignedNew();

					// Check if branch-block has already been compiled
					pBlock = mVUblocks[branchAddr/8]->search((microRegInfo*)&mVUregs);
					if (pBlock)		   { xJMP(pBlock->x86ptrStart); }
					else			   { mVUcompile(mVU, branchAddr, (uptr)&mVUregs); }
					return thisPtr;
				case 9: case 10: // JR/JALR

					mVUprint("mVUcompile JR/JALR");
					incPC(-3); // Go back to jump opcode

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
					MOV32MtoR(gprT2, (uptr)&mVU->branch);		 // Get startPC (ECX first argument for __fastcall)
					MOV32ItoR(gprR, (u32)&pBlock->pStateEnd);	 // Get pState (EDX second argument for __fastcall)

					if (!mVU->index) xCALL( mVUcompileJIT<0> ); //(u32 startPC, uptr pState)
					else			 xCALL( mVUcompileJIT<1> );
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
				if (pBlock)			  { xJMP( pBlock->x86ptrStart ); }
				else if (!mVU->index) { mVUblockFetch(mVU, branchAddr, (uptr)&mVUregs); }
				else				  { mVUblockFetch(mVU, branchAddr, (uptr)&mVUregs); }
			}
			else {
				uptr jumpAddr;
				u32 bPC = iPC; // mVUcompile can modify iPC and mVUregs so back them up
				memcpy_fast(&pBlock->pStateEnd, &mVUregs, sizeof(microRegInfo));
	
				incPC2(1);  // Get PC for branch not-taken
				mVUcompile(mVU, xPC, (uptr)&mVUregs);

				iPC = bPC;
				incPC(-3); // Go back to branch opcode (to get branch imm addr)
				if (!mVU->index) jumpAddr = (uptr)mVUblockFetch(mVU, branchAddr, (uptr)&pBlock->pStateEnd);
				else			 jumpAddr = (uptr)mVUblockFetch(mVU, branchAddr, (uptr)&pBlock->pStateEnd);
				*ajmp = (jumpAddr - ((uptr)ajmp + 4));
			}
			return thisPtr;
		}
	}
	if (x == microSizeDiv8) { Console::Error("microVU%d: Possible infinite compiling loop!", params mVU->index); }

	// E-bit End
	mVUendProgram(mVU, 1, xStatus, xMac, xClip);
	return thisPtr;
}


microVUt(void*) mVUblockFetch( microVU* mVU, u32 startPC, uptr pState )
{
	using namespace x86Emitter;

	if (startPC > mVU->microMemSize-8) { Console::Error("microVU%d: invalid startPC", params mVU->index); }
	startPC &= mVU->microMemSize-8;
	
	if (mVUblocks[startPC/8] == NULL) {
		mVUblocks[startPC/8] = microBlockManager::AlignedNew();
	}

	// Searches for Existing Compiled Block (if found, then returns; else, compile)
	microBlock* pBlock = mVUblocks[startPC/8]->search((microRegInfo*)pState);
	if (pBlock) { return pBlock->x86ptrStart; }

	return mVUcompile( mVU, startPC, pState );
}


//------------------------------------------------------------------
// Wrapper Functions - Called by other parts of the Emu
//------------------------------------------------------------------

void initVUrec(VURegs* vuRegs, const int vuIndex) {
	if (!vuIndex)	mVUinit<0>(vuRegs);
	else			mVUinit<1>(vuRegs);
}

void closeVUrec(const int vuIndex) {
	if (!vuIndex)	mVUclose<0>();
	else			mVUclose<1>();
}

void resetVUrec(const int vuIndex) {
	if (!vuIndex)	mVUreset<0>();
	else			mVUreset<1>();
}

void clearVUrec(u32 addr, u32 size, const int vuIndex) {
	if (!vuIndex)	mVUclear<0>(addr, size);
	else			mVUclear<1>(addr, size);
}

void runVUrec(u32 startPC, u32 cycles, const int vuIndex) {
	if (!vuIndex)	startVU0(startPC, cycles);
	else			startVU1(startPC, cycles);
}

void vsyncVUrec(const int vuIndex) {
	if (!vuIndex)	mVUvsyncUpdate<0>();
	else			mVUvsyncUpdate<1>();
}
