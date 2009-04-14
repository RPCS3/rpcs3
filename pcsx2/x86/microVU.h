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
#define mVUdebug // Prints Extra Info to Console
#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "ix86/ix86.h"
#include "microVU_Alloc.h"
#include "microVU_Misc.h"

struct microBlock {
	microRegInfo pState; // Detailed State of Pipeline
	u32 pipelineState;	 // | FDiv x 4 | EFU x 6 | Needs pState Info? x 1 | // Simple State of Pipeline
	u8* x86ptrStart;	 // Start of code
	u8* x86ptrEnd;		 // End of code (first byte outside of block)
	u8* x86ptrBranch;	 // 
	u32 size;			 // Number of 64bit VU Instructions in Block
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
	microBlockManager()		{ init(); }
	~microBlockManager()	{ close(); }
	void init() {
		listSize = -1;
		callerSize = -1;
		//ZeroMemory(&blockList, sizeof(blockList)); // Can be Omitted?
		//ZeroMemory(&blockList, sizeof(callersList)); // Can be Omitted?
	}
	void reset() { init(); };
	void close() {}; // Can be Omitted?
	/*void add(u32 pipelineState, u8* x86ptrStart) {
		if (!search(pipelineState)) {
			listSize++;
			listSize &= MaxBlocks;
			blockList[listSize].pipelineState = pipelineState;
			blockList[listSize].x86ptrStart = x86ptrStart;
		}
	}*/
	microBlock* search(u32 pipelineState, microRegInfo* pState) {
		if (pipelineState & 1) { // Needs Detailed Search (Exact Match of Pipeline State)
			for (int i = 0; i < listSize; i++) {
				if (!memcmp(pState, &blockList[i].pState, sizeof(microRegInfo))) return &blockList[i];
			}
		}
		else { // Can do Simple Search (Only Matches the Important Pipeline Stuff)
			for (int i = 0; i < listSize; i++) {
				if (blockList[i].pipelineState == pipelineState) return &blockList[i];
			}
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
	u32 data[progSize/4];
	u32 used;		// Number of times its been used
	u8* x86ptr;		// Pointer to program's recompilation code
	u8* x86start;	// Start of program's rec-cache
	u8* x86end;		// Limit of program's rec-cache
	microBlockManager* block[progSize/8];
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
	u32 progSize;	// VU Micro Program Size (microSize/4)
	static const u32 cacheSize = 0x500000; // VU Cache Size

	microProgManager<0x4000> prog; // Micro Program Data
	
	VURegs*	regs;		 // VU Regs Struct
	u8*		cache;		 // Dynarec Cache Start (where we will start writing the recompiled code to)
	u8*		startFunct;	 // Ptr Function to the Start code for recompiled programs
	u8*		exitFunct;	 // Ptr Function to the Exit code for recompiled programs
	u8*		ptr;		 // Pointer to next place to write recompiled code to
	u32		code;		 // Contains the current Instruction
	u32		iReg;		 // iReg (only used in recompilation, not execution)
	u32		clipFlag[4]; // 4 instances of clip flag (used in execution)
	u32		divFlag;	 // 1 instance of I/D flags
	u32		VIbackup[2]; // Holds a backup of a VI reg if modified before a branch
	u32		branch;		 // Holds branch compare result (IBxx) OR Holds address to Jump to (JALR/JR)
	u32		p;			 // Holds current P instance index
	u32		q;			 // Holds current Q instance index
};

// microVU rec structs
extern PCSX2_ALIGNED16(microVU microVU0);
extern PCSX2_ALIGNED16(microVU microVU1);

// Opcode Tables
extern void (*mVU_UPPER_OPCODE[64])( VURegs* VU, s32 info );
extern void (*mVU_LOWER_OPCODE[128])( VURegs* VU, s32 info );

// Main Functions
microVUt(void) mVUinit(VURegs*);
microVUt(void) mVUreset();
microVUt(void) mVUclose();
microVUt(void) mVUclear(u32, u32);

// Private Functions
__forceinline void	mVUclearProg(microVU* mVU, int progIndex);
__forceinline int	mVUfindLeastUsedProg(microVU* mVU);
__forceinline int	mVUsearchProg(microVU* mVU);
__forceinline void	mVUcacheProg(microVU* mVU, int progIndex);
void* __fastcall	mVUexecuteVU0(u32 startPC, u32 cycles);
void* __fastcall	mVUexecuteVU1(u32 startPC, u32 cycles);

#ifndef __LINUX__
typedef void (__fastcall *mVUrecCall)(u32, u32);
#else
typedef void (*mVUrecCall)(u32, u32) __attribute__((__fastcall)); // Not sure if this is correct syntax (should be close xD)
#endif

// Include all the *.inl files (Needed because C++ sucks with templates and *.cpp files)
#include "microVU_Misc.inl"
#include "microVU_Analyze.inl"
#include "microVU_Alloc.inl"
#include "microVU_Tables.inl"
#include "microVU_Compile.inl"
#include "microVU_Execute.inl"
