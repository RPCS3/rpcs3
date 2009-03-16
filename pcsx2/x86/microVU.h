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

#pragma once
#define _EmitterId_ (vuIndex+1)
#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "ix86/ix86.h"
#include "microVU_Alloc.h"

struct microBlock {
	u32 pipelineState; // FMACx|y|z|w | FDiv | EFU | IALU | BRANCH // Still thinking of how I'm going to do this
	u8* x86ptrStart;
	u8* x86ptrEnd;
	u8* x86ptrBranch;
	//u32 size;
};

#define mMaxBlocks 32 // Max Blocks With Different Pipeline States (For n = 1, 2, 4, 8, 16, etc...)
class microBlockManager {
private:
	static const int MaxBlocks = mMaxBlocks - 1;
	u32 startPC;
	u32 endPC;
	int listSize; // Total Items - 1
	int callerSize; // Total Callers - 1
	microBlock blockList[mMaxBlocks];
	microBlock callersList[mMaxBlocks]; // Foreign Blocks that call Local Blocks

public:
	microBlockManager()	{ init(); }
	~microBlockManager()	{ close(); }
	void init() {
		listSize = -1;
		callerSize = -1;
		//ZeroMemory(&blockList, sizeof(blockList)); // Can be Omitted?
		//ZeroMemory(&blockList, sizeof(callersList)); // Can be Omitted?
	}
	void reset() { init(); };
	void close() {}; // Can be Omitted?
	void add(u32 pipelineState, u8* x86ptrStart) {
		if (!search(pipelineState)) {
			listSize++;
			listSize &= MaxBlocks;
			blockList[listSize].pipelineState = pipelineState;
			blockList[listSize].x86ptrStart = x86ptrStart;
		}
	}
	microBlock* search(u32 pipelineState) {
		for (int i = 0; i < listSize; i++) {
			if (blockList[i].pipelineState == pipelineState) return &blockList[i];
		}
		return NULL;
	}
	void clearFast() {
		listSize = -1;
		for ( ; callerSize >= 0; callerSize--) {
			//callerList[callerSize]. // ToDo: Implement Branch Link Removal Code
		}
	}
	int clear() {
		if (listSize >= 0) { clearFast(); return 1; }
		else return 0;
	}
};

template<u32 progSize>
struct microProgram {
	u32 data[progSize];
	u32 used;	// Number of times its been used
	microBlockManager* block[progSize / 2];
	microAllocInfo<progSize> allocInfo;
};

#define mMaxProg 16 // The amount of Micro Programs Recs will 'remember' (For n = 1, 2, 4, 8, 16, etc...)
template<u32 pSize>
struct microProgManager {
	microProgram<pSize>	prog[mMaxProg];	// Store MicroPrograms in memory
	static const int	max = mMaxProg - 1; 
	int					cur;			// Index to Current MicroProgram thats running (-1 = uncached)
	int					total;			// Total Number of valid MicroPrograms minus 1
	int					cleared;		// Micro Program is Indeterminate so must be searched for (and if no matches are found then recompile a new one)
	int					finished;		// Completed MicroProgram by E-bit Termination
	u32					lastPipelineState; // Pipeline state from where it left off (useful for continuing execution)
};

struct microVU {
	u32 index;		// VU Index (VU0 or VU1)
	u32 microSize;	// VU Micro Memory Size
	u32 progSize;	// VU Micro Program Size (microSize/8)
	u32 cacheAddr;	// VU Cache Start Address
	static const u32 cacheSize = 0x500000; // VU Cache Size

	microProgManager<0x1000> prog; // Micro Program Data
	
	VURegs*	regs;		 // VU Regs Struct
	u8*		cache;		 // Dynarec Cache Start (where we will start writing the recompiled code to)
	u8*		ptr;		 // Pointer to next place to write recompiled code to
	u32		code;		 // Contains the current Instruction
	u32		iReg;		 // iReg (only used in recompilation, not execution)
	u32		clipFlag[4]; // 4 instances of clip flag (used in execution)

/*
	uptr x86eax; // Accumulator register. Used in arithmetic operations.
	uptr x86ecx; // Counter register. Used in shift/rotate instructions.
	uptr x86edx; // Data register. Used in arithmetic operations and I/O operations.
	uptr x86ebx; // Base register. Used as a pointer to data (located in DS in segmented mode).
	uptr x86esp; // Stack Pointer register. Pointer to the top of the stack.
	uptr x86ebp; // Stack Base Pointer register. Used to point to the base of the stack.
	uptr x86esi; // Source register. Used as a pointer to a source in stream operations.
	uptr x86edi; // Destination register. Used as a pointer to a destination in stream operations.
*/
};

// microVU rec structs
extern PCSX2_ALIGNED16(microVU microVU0);
extern PCSX2_ALIGNED16(microVU microVU1);

// Opcode Tables
extern void (*mVU_UPPER_OPCODE[64])( VURegs* VU, s32 info );
extern void (*mVU_LOWER_OPCODE[128])( VURegs* VU, s32 info );

__forceinline void	mVUclearProg(microVU* mVU, int progIndex);
__forceinline int	mVUfindLeastUsedProg(microVU* mVU);
__forceinline int	mVUsearchProg(microVU* mVU);
__forceinline void	mVUcacheProg(microVU* mVU, int progIndex);

#ifdef __LINUX__
microVUt(void) mVUreset();
microVUt(void) mVUclose();
#endif

#include "microVU_Misc.h"
#include "microVU_Alloc.inl"
#include "microVU_Tables.inl"
#include "microVU_Compile.inl"
