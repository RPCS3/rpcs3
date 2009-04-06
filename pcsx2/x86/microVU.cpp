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
	mVU->progSize	= (vuIndex ? 0x4000 : 0x1000) / 8;
	mVU->cacheAddr	= (vuIndex ? 0x1e840000 : 0x0e840000);
	mVU->cache		= NULL;

	mVUreset<vuIndex>();
}

// Will Optimize later
microVUt(void) mVUreset() {

	microVU* mVU = mVUx;
	mVUclose<vuIndex>(); // Close

	// Create Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progSize / 2); j++) {
			mVU->prog.prog[i].block[j] = new microBlockManager();
		}
	}

	// Dynarec Cache
	mVU->cache = SysMmapEx(mVU->cacheAddr, mVU->cacheSize, 0, (vuIndex ? "Micro VU1" : "Micro VU0"));
	if ( mVU->cache == NULL ) throw Exception::OutOfMemory(fmt_string( "microVU Error: Failed to allocate recompiler memory! (addr: 0x%x)", params (u32)mVU->cache));
	
	// Other Variables
	memset(&mVU->prog, 0, sizeof(mVU->prog));
	mVU->prog.finished = 1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
}

// Free Allocated Resources
microVUt(void) mVUclose() {

	microVU* mVU = mVUx;

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
	mVU->prog.cleared = 1; // Next execution searches/creates a new microprogram
	// Note: It might be better to copy old recompiled blocks to the new microprogram rec data
	// however, if games primarily do big writes, its probably not worth it.
	// The cost of invalidating bad blocks is also kind of expensive, which is another reason
	// that its probably not worth it...
}

// Executes for number of cycles
microVUt(void*) __fastcall mVUexecute(u32 startPC, u32 cycles) {
/*	
	Pseudocode: (ToDo: implement # of cycles)
	1) Search for existing program
	2) If program not found, goto 5
	3) Search for recompiled block
	4) If recompiled block found, goto 6
	5) Recompile as much blocks as possible
	6) Return start execution address of block
*/
	microVU* mVU = mVUx;
	if ( mVUsearchProg(mVU) ) { // Found Program
		//microBlock* block = mVU->prog.prog[mVU->prog.cur].block[startPC]->search(mVU->prog.lastPipelineState);
		//if (block) return block->x86ptrStart; // Found Block
	}
	// Recompile code
	return NULL;
}

void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles) {
	return mVUexecute<0>(startPC, cycles);
}
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles) {
	return mVUexecute<1>(startPC, cycles);
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Clears program data (Sets used to 1 because calling this function implies the program will be used at least once)
__forceinline void mVUclearProg(microVU* mVU, int progIndex) {
	mVU->prog.prog[progIndex].used = 1;
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		mVU->prog.prog[progIndex].block[i]->reset();
	}
}

// Caches Micro Program
__forceinline void mVUcacheProg(microVU* mVU, int progIndex) {
	memcpy_fast(mVU->prog.prog[progIndex].data, mVU->regs->Micro, mVU->microSize);
}

// Finds the least used program, (if program list full clears and returns an old program; if not-full, returns free program)
__forceinline int mVUfindLeastUsedProg(microVU* mVU) {
	if (mVU->prog.total < mVU->prog.max) {
		mVU->prog.total++;
		mVUcacheProg(mVU, mVU->prog.total); // Cache Micro Program
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
		mVUclearProg(mVU, j); // Clear old data if overwriting old program
		mVUcacheProg(mVU, j); // Cache Micro Program
		return j;
	}
}

// Searches for Cached Micro Program and sets prog.cur to it (returns 1 if program found, else returns 0)
__forceinline int mVUsearchProg(microVU* mVU) {
	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			//if (i == mVU->prog.cur) continue; // We can skip the current program. (ToDo: Verify that games don't clear, and send the same microprogram :/)
			if (!memcmp_mmx(mVU->prog.prog[i].data, mVU->regs->Micro, mVU->microSize)) {
				if (i == mVU->prog.cur) SysPrintf("microVU: Same micro program sent!\n");
				mVU->prog.cur = i;
				mVU->prog.cleared = 0;
				mVU->prog.prog[i].used++;
				return 1;
			}
		}
		mVU->prog.cur = mVUfindLeastUsedProg(mVU); // If cleared and program not found, make a new program instance
		mVU->prog.cleared = 0;
		return 0;
	}
	mVU->prog.prog[mVU->prog.cur].used++;
	return 1; // If !cleared, then we're still on the same program as last-time ;)
}

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

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

#ifdef _MSC_VER
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
		/* Should set xmmZ? */
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
		emms

		ret
	}
}
#else
extern "C" {
	extern void __fastcall startVU0(u32 startPC, u32 cycles);
	extern void __fastcall startVU1(u32 startPC, u32 cycles);
	extern void __fastcall endVU0(u32 startPC, u32 cycles);
}
#endif

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
