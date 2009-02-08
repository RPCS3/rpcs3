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
// VU Micro - Global Variables
//------------------------------------------------------------------

microVU microVU0;
microVU microVU1;

//------------------------------------------------------------------
// Micro VU - Main Functions
//------------------------------------------------------------------

// Only run this once! ;)
__forceinline void mVUinit(microVU* mVU, VURegs* vuRegsPtr, const int vuIndex) {

	mVU->regs		= vuRegsPtr;
	mVU->index		= vuIndex;
	mVU->microSize	= (vuIndex ? 0x4000 : 0x1000);
	mVU->progSize	= (vuIndex ? 0x4000 : 0x1000) / 8;
	mVU->cacheAddr	= 0xC0000000 + (vuIndex ? mVU->cacheSize : 0);
	mVU->cache		= NULL;

	mVUreset(mVU);
}

// Will Optimize later
__forceinline void mVUreset(microVU* mVU) {

	mVUclose(mVU); // Close

	// Create Block Managers
	for (int i; i <= mVU->prog.max; i++) {
		for (u32 j; j < mVU->progSize; j++) {
			mVU->prog.prog[i].block[j] = new microBlockManager();
		}
	}

	// Dynarec Cache
	mVU->cache = SysMmapEx(mVU->cacheAddr, mVU->cacheSize, 0x10000000, "Micro VU");
	if ( mVU->cache == NULL ) throw Exception::OutOfMemory(fmt_string( "microVU Error: failed to allocate recompiler memory! (addr: 0x%x)", params (u32)mVU->cache));

	// Other Variables
	ZeroMemory(&mVU->prog, sizeof(mVU->prog));
	mVU->prog.finished = 1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
}

// Free Allocated Resources
__forceinline void mVUclose(microVU* mVU) {

	if ( mVU->cache ) { SysMunmap( mVU->cache, mVU->cacheSize ); mVU->cache = NULL; }

	// Delete Block Managers
	for (int i; i <= mVU->prog.max; i++) {
		for (u32 j; j < mVU->progSize; j++) {
			if (mVU->prog.prog[i].block[j]) delete mVU->prog.prog[i].block[j];
		}
	}
}

// Clears Block Data in specified range (Caches current microProgram if a difference has been found)
__forceinline void mVUclear(microVU* mVU, u32 addr, u32 size) {

	int i = addr/8;
	int end = i+((size+(8-(size&7)))/8); // ToDo: Can be simplified to addr+size if Size is always a multiple of 8
	
	if (!mVU->prog.cleared) {
		for ( ; i < end; i++) {
			if ( mVU->prog.prog[mVU->prog.cur].block[i]->clear() ) {
				mVUcacheProg(mVU);
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

// Executes for number of cycles
void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles) {
/*	Pseudocode: (ToDo: implement # of cycles)
	1) Search for existing program
	2) If program not found, goto 5
	3) Search for recompiled block
	4) If recompiled block found, goto 6
	5) Recompile as much blocks as possible
	6) Return start execution address of block
*/
	if ( mVUsearchProg(&microVU0) ) { // Found Program
		microBlock* block = microVU0.prog.prog[microVU0.prog.cur].block[startPC]->search(microVU0.prog.lastPipelineState);
		if (block) return block->x86ptrStart;
	}
	// Recompile code
	return NULL;
}
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles) {
	return NULL;
}

/*
// Executes till finished
void* mVUexecuteF(microVU* mVU, u32 startPC) {
	//if (!mProg.finished) {
	//	runMicroProgram(startPC);
	//}
	//if (mProg.cleared && !mProg.finished) {

	//}
	return NULL;
}
*/

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Finds the least used program
__forceinline int mVUfindLeastUsedProg(microVU* mVU) {
	if (mVU->prog.total < mVU->prog.max) {
		mVU->prog.total++;
		return mVU->prog.total;
	}
	else {
		int j = 0;
		u32 smallest = mVU->prog.prog[0].used;
		for (int i = 1; i <= mVU->prog.total; i++) {
			if (smallest > mVU->prog.prog[i].used) {
				smallest = mVU->prog.prog[i].used;
				j = i;
			}
		}
		return j;
	}
}

// Caches Micro Program if appropriate
__forceinline void mVUcacheProg(microVU* mVU) {
	if (!mVU->prog.prog[mVU->prog.cur].cached) { // If uncached, then cache
		memcpy_fast(mVU->prog.prog[mVU->prog.cur].data, mVU->regs->Micro, mVU->microSize);
		mVU->prog.prog[mVU->prog.cur].cached = 1;
	}
}

// Searches for Cached Micro Program and sets prog.cur to it (returns 1 if program found, else returns 0)
__forceinline int mVUsearchProg(microVU* mVU) {
	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (i == mVU->prog.cur) continue; // We can skip the current program.
			if (mVU->prog.prog[i].cached) {
				if (!memcmp_mmx(mVU->prog.prog[i].data, mVU->regs->Micro, mVU->microSize)) {
					mVU->prog.cur = i;
					return 1;
				}
			}
		}
		mVU->prog.cur = mVUfindLeastUsedProg(mVU); // If cleared and program not cached, make a new program instance
		// ToDo: Clear old data if overwriting old program
		return 0;
	}
	else return 1; // If !cleared, then we're still on the same program as last-time ;)
}

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

// Runs VU0 for number of cycles
__declspec(naked) void __fastcall startVU0(u32 startPC, u32 cycles) {
	__asm {
		// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers; all other arguments are passed right to left.
		call mVUexecuteVU0

		/*backup cpu state*/
		push ebx;
		push ebp;
		push esi;
		push edi;

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}

// Runs VU1 for number of cycles
__declspec(naked) void __fastcall startVU1(u32 startPC, u32 cycles) {
	__asm {

		call mVUexecuteVU1

		/*backup cpu state*/
		push ebx;
		push ebp;
		push esi;
		push edi;

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}

// Exit point
__declspec(naked) void __fastcall endVU0(u32 startPC, u32 cycles) {
	__asm {

		//call mVUcleanUpVU0

		/*restore cpu state*/
		pop edi;
		pop esi;
		pop ebp;
		pop ebx;
		
		ldmxcsr g_sseMXCSR

		ret
	}
}
//------------------------------------------------------------------
// Wrapper Functions - Called by other parts of the Emu
//------------------------------------------------------------------

__forceinline void initVUrec(VURegs* vuRegs, int vuIndex) {
	if (!vuIndex) mVUinit(&microVU0, vuRegs, 0);
	else		  mVUinit(&microVU1, vuRegs, 1);
}

__forceinline void closeVUrec(int vuIndex) {
	if (!vuIndex) mVUclose(&microVU0);
	else		  mVUclose(&microVU1);
}

__forceinline void resetVUrec(int vuIndex) {
	if (!vuIndex) mVUreset(&microVU0);
	else		  mVUreset(&microVU1);
}

__forceinline void clearVUrec(u32 addr, u32 size, int vuIndex) {
	if (!vuIndex) mVUclear(&microVU0, addr, size);
	else		  mVUclear(&microVU1, addr, size);
}

__forceinline void runVUrec(u32 startPC, u32 cycles, int vuIndex) {
	if (!vuIndex) startVU0(startPC, cycles);
	else		  startVU1(startPC, cycles);
}

#endif // PCSX2_MICROVU
