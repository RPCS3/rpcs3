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

// Micro VU recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "microVU.h"
#ifdef PCSX2_MICROVU

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
microVUt(void) mVUinit(VURegs* vuRegsPtr) {

	microVU* mVU	= mVUx;
	mVU->regs		= vuRegsPtr;
	mVU->index		= vuIndex;
	mVU->microSize	= (vuIndex ? 0x4000 : 0x1000);
	mVU->progSize	= (vuIndex ? 0x4000 : 0x1000) / 4;
	mVU->cache		= NULL;
	memset(&mVU->prog, 0, sizeof(mVU->prog));
	mVUprint((vuIndex) ? "microVU1: init" : "microVU0: init");

	mVUreset<vuIndex>();
}

// Will Optimize later
microVUt(void) mVUreset() {

	microVU* mVU = mVUx;
	mVUclose<vuIndex>(); // Close

	mVUprint((vuIndex) ? "microVU1: reset" : "microVU0: reset");

	// Dynarec Cache
	mVU->cache = SysMmapEx((vuIndex ? 0x1e840000 : 0x0e840000), mVU->cacheSize, 0, (vuIndex ? "Micro VU1" : "Micro VU0"));
	if ( mVU->cache == NULL ) throw Exception::OutOfMemory(fmt_string( "microVU Error: Failed to allocate recompiler memory! (addr: 0x%x)", params (u32)mVU->cache));
	memset(mVU->cache, 0xcc, mVU->cacheSize);

	// Setup Entrance/Exit Points
	x86SetPtr(mVU->cache);
	mVUdispatcherA<vuIndex>();
	mVUdispatcherB<vuIndex>();

	// Clear All Program Data
	memset(&mVU->prog, 0, sizeof(mVU->prog));

	// Create Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progSize / 2); j++) {
			mVU->prog.prog[i].block[j] = new microBlockManager();
		}
	}

	// Program Variables
	mVU->prog.finished = 1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
	//mVU->prog.lpState = &mVU->prog.prog[15].allocInfo.block.pState; // Blank Pipeline State (ToDo: finish implementation)

	// Setup Dynarec Cache Limits for Each Program
	u8* z = (mVU->cache + 512); // Dispatcher Code is in first 512 bytes
	for (int i = 0; i <= mVU->prog.max; i++) {
		mVU->prog.prog[i].x86start = z;
		mVU->prog.prog[i].x86ptr = z;
		z += (mVU->cacheSize / (mVU->prog.max + 1));
		mVU->prog.prog[i].x86end = z;
	}
}

// Free Allocated Resources
microVUt(void) mVUclose() {

	microVU* mVU = mVUx;
	mVUprint((vuIndex) ? "microVU1: close" : "microVU0: close");

	if ( mVU->cache ) { HostSys::Munmap( mVU->cache, mVU->cacheSize ); mVU->cache = NULL; }

	// Delete Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progSize / 2); j++) {
			if (mVU->prog.prog[i].block[j]) delete mVU->prog.prog[i].block[j];
		}
	}
}

// Clears Block Data in specified range
microVUt(void) mVUclear(u32 addr, u32 size) {

	microVU* mVU = mVUx;
	memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));
	mVU->prog.cleared = 1; // Next execution searches/creates a new microprogram
	// Note: It might be better to copy old recompiled blocks to the new microprogram rec data
	// however, if games primarily do big writes, its probably not worth it.
	// The cost of invalidating bad blocks is also kind of expensive, which is another reason
	// that its probably not worth it...
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Clears program data (Sets used to 1 because calling this function implies the program will be used at least once)
microVUt(void) mVUclearProg(int progIndex) {
	microVU* mVU = mVUx;
	mVU->prog.prog[progIndex].used = 1;
	mVU->prog.prog[progIndex].x86ptr = mVU->prog.prog[progIndex].x86start;
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		mVU->prog.prog[progIndex].block[i]->reset();
	}
}

// Caches Micro Program
microVUt(void) mVUcacheProg(int progIndex) {
	microVU* mVU = mVUx;
	memcpy_fast(mVU->prog.prog[progIndex].data, mVU->regs->Micro, mVU->microSize);
	mVUdumpProg(progIndex);
}

// Finds the least used program, (if program list full clears and returns an old program; if not-full, returns free program)
microVUt(int) mVUfindLeastUsedProg() {
	microVU* mVU = mVUx;
	if (mVU->prog.total < mVU->prog.max) {
		mVU->prog.total++;
		mVUcacheProg<vuIndex>(mVU->prog.total); // Cache Micro Program
		Console::Notice("microVU%d: Cached MicroPrograms = %d", params vuIndex, mVU->prog.total+1);
		return mVU->prog.total;
	}
	else {
		int j = (mVU->prog.cur + 1) & mVU->prog.max;
		/*u32 smallest = mVU->prog.prog[j].used;
		for (int i = ((j+1)&mVU->prog.max), z = 0; z < mVU->prog.max; i = (i+1)&mVU->prog.max, z++) {
			if (smallest > mVU->prog.prog[i].used) {
				smallest = mVU->prog.prog[i].used;
				j = i;
			}
		}*/
		mVUclearProg<vuIndex>(j); // Clear old data if overwriting old program
		mVUcacheProg<vuIndex>(j); // Cache Micro Program
		Console::Notice("microVU%d: MicroProgram Cache Full!", params vuIndex);
		return j;
	}
}

// Searches for Cached Micro Program and sets prog.cur to it (returns 1 if program found, else returns 0)
microVUt(int) mVUsearchProg() {
	microVU* mVU = mVUx;
	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (!memcmp_mmx(mVU->prog.prog[i].data, mVU->regs->Micro, mVU->microSize)) {
				mVU->prog.cur = i;
				mVU->prog.cleared = 0;
				mVU->prog.prog[i].used++;
				return 1;
			}
		}
		mVU->prog.cur = mVUfindLeastUsedProg<vuIndex>(); // If cleared and program not found, make a new program instance
		mVU->prog.cleared = 0;
		return 0;
	}
	mVU->prog.prog[mVU->prog.cur].used++;
	return 1; // If !cleared, then we're still on the same program as last-time ;)
}
/*
// Block Invalidation
__forceinline void mVUinvalidateBlock(microVU* mVU, u32 addr, u32 size) {

	int i = addr/8;
	int end = i+((size+(8-(size&7)))/8); // ToDo: Can be simplified to addr+size if Size is always a multiple of 8

	if (!mVU->prog.cleared) {
		for ( ; i < end; i++) {
			if ( mVU->prog.prog[mVU->prog.cur].block[i]->clear() ) {
				mVU->prog.cleared = 1;
				i++;
				break;
			}
		}
	}
	for ( ; i < end; i++) {
		mVU->prog.prog[mVU->prog.cur].block[i]->clearFast();
	}
}
*/
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

#endif // PCSX2_MICROVU
