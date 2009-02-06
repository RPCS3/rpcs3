/*  Pcsx2-Playground - Pc Ps2 Emulator
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

// Mega VU(micro) recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "megaVU.h"
#ifdef PCSX2_MEGAVU

//------------------------------------------------------------------
// Mega VU - Global Variables
//------------------------------------------------------------------

megaVU megaVU0;
megaVU megaVU1;

//------------------------------------------------------------------
// Mega VU - Main Functions
//------------------------------------------------------------------

// Only run this once! ;)
__forceinline void mVUinit(megaVU* mVU, VURegs* vuRegsPtr, const int vuIndex) {

	mVU->regs		= vuRegsPtr;
	mVU->index		= vuIndex;
	mVU->microSize	= (vuIndex ? 0x4000 : 0x1000);
	mVU->progSize	= (vuIndex ? 0x4000 : 0x1000) / 8;
	mVU->cacheAddr	= 0xC0000000 + (vuIndex ? mVU->cacheSize : 0);
	mVU->cache		= NULL;

	for (int i; i <= mVU->prog.max; i++) {
		for (u32 j; j < mVU->progSize; j++) {
			mVU->prog.prog[i].block[j] = new megaBlockManager();
		}
	}

	mVUreset(mVU);
}

// Will Optimize later
__forceinline void mVUreset(megaVU* mVU) {

	mVUclose(mVU); // Close

	// Dynarec Cache
	mVU->cache = SysMmap(mVU->cacheAddr, mVU->cacheSize, 0x10000000, "Mega VU");
	if ( mVU->cache == NULL ) throw Exception::OutOfMemory(fmt_string( "megaVU Error: failed to allocate recompiler memory! (addr: 0x%x)", params (u32)mVU->cache));

	// Other Variables
	ZeroMemory(&mVU->prog, sizeof(mVU->prog));
	mVU->prog.finished = 1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
}

// Free Allocated Resources
__forceinline void mVUclose(megaVU* mVU) {

	if ( mVU->cache ) { SysMunmap( mVU->cache, mVU->cacheSize ); mVU->cache = NULL; }

	for (int i; i <= mVU->prog.max; i++) {
		for (u32 j; j < mVU->progSize; j++) {
			if (mVU->prog.prog[i].block[j]) delete mVU->prog.prog[i].block[j];
		}
	}
}

// Clears Block Data in specified range (Caches current microProgram if a difference has been found)
__forceinline void mVUclear(megaVU* mVU, u32 addr, u32 size) {

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
void* mVUexecute(megaVU* mVU, u32 startPC, u32 cycles) {
	return NULL;
}

// Executes till finished
void* mVUexecuteF(megaVU* mVU, u32 startPC) {
	//if (!mProg.finished) {
	//	runMicroProgram(startPC);
	//}
	//if (mProg.cleared && !mProg.finished) {

	//}
	return NULL;
}

//------------------------------------------------------------------
// Mega VU - Private Functions
//------------------------------------------------------------------

// Finds the least used program
__forceinline int mVUfindLeastUsedProg(megaVU* mVU) {
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
__forceinline void mVUcacheProg(megaVU* mVU) {
	if (!mVU->prog.prog[mVU->prog.cur].cached) { // If uncached, then cache
		memcpy_fast(mVU->prog.prog[mVU->prog.cur].data, mVU->regs->Micro, mVU->microSize);
	}
}

// Searches for Cached Micro Program and sets prog.cur (returns -1 if no program found)
__forceinline int mVUsearchProg(megaVU* mVU) {
	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (!memcmp_mmx(mVU->prog.prog[i].data, mVU->regs->Micro, mVU->microSize)) {
				return i;
			}
		}
		mVU->prog.cur = mVUfindLeastUsedProg(mVU);
		return -1;
	}
	else return mVU->prog.cur;
}

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

// Runs till finished
__declspec(naked) void runVU0(megaVU* mVU, u32 startPC) {
	__asm {
		mov eax, dword ptr [esp]
		mov megaVU0.x86callstack, eax
		add esp, 4
		call mVUexecuteF

		/*backup cpu state*/
		mov megaVU0.x86ebp, ebp
		mov megaVU0.x86esi, esi
		mov megaVU0.x86edi, edi
		mov megaVU0.x86ebx, ebx
		/*mov megaVU0.x86esp, esp*/

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}
__declspec(naked) void runVU1(megaVU* mVU, u32 startPC) {
	__asm {
		mov eax, dword ptr [esp]
		mov megaVU1.x86callstack, eax
		add esp, 4
		call mVUexecuteF

		/*backup cpu state*/
		mov megaVU1.x86ebp, ebp
		mov megaVU1.x86esi, esi
		mov megaVU1.x86edi, edi
		mov megaVU1.x86ebx, ebx
		/*mov megaVU1.x86esp, esp*/

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}

// Runs for number of cycles
__declspec(naked) void runVU0(megaVU* mVU, u32 startPC, u32 cycles) {
	__asm {
		mov eax, dword ptr [esp]
		mov megaVU0.x86callstack, eax
		add esp, 4
		call mVUexecute

		/*backup cpu state*/
		mov megaVU0.x86ebp, ebp
		mov megaVU0.x86esi, esi
		mov megaVU0.x86edi, edi
		mov megaVU0.x86ebx, ebx
		/*mov megaVU0.x86esp, esp*/

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}
__declspec(naked) void runVU1(megaVU* mVU, u32 startPC, u32 cycles) {
	__asm {
		mov eax, dword ptr [esp]
		mov megaVU1.x86callstack, eax
		add esp, 4
		call mVUexecute

		/*backup cpu state*/
		mov megaVU1.x86ebp, ebp
		mov megaVU1.x86esi, esi
		mov megaVU1.x86edi, edi
		mov megaVU1.x86ebx, ebx
		/*mov megaVU1.x86esp, esp*/

		ldmxcsr g_sseVUMXCSR

		jmp eax
	}
}
//------------------------------------------------------------------
// Wrapper Functions - Called by other parts of the Emu
//------------------------------------------------------------------

__forceinline void initVUrec(VURegs* vuRegs, int vuIndex) {
	if (!vuIndex) mVUinit(&megaVU0, vuRegs, 0);
	else		  mVUinit(&megaVU1, vuRegs, 1);
}

__forceinline void closeVUrec(int vuIndex) {
	if (!vuIndex) mVUclose(&megaVU0);
	else		  mVUclose(&megaVU1);
}

__forceinline void resetVUrec(int vuIndex) {
	if (!vuIndex) mVUreset(&megaVU0);
	else		  mVUreset(&megaVU1);
}

__forceinline void clearVUrec(u32 addr, u32 size, int vuIndex) {
	if (!vuIndex) mVUclear(&megaVU0, addr, size);
	else		  mVUclear(&megaVU1, addr, size);
}

__forceinline void runVUrec(u32 startPC, int vuIndex) {
	if (!vuIndex) runVU0(&megaVU0, startPC);
	else		  runVU1(&megaVU1, startPC);
}

__forceinline void runVUrec(u32 startPC, u32 cycles, int vuIndex) {
	if (!vuIndex) runVU0(&megaVU0, startPC, cycles);
	else		  runVU1(&megaVU1, startPC, cycles);
}

#endif // PCSX2_MEGAVU
