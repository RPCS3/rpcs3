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
//#define mVUdebug		// Prints Extra Info to Console
//#define mVUlogProg	// Dumps MicroPrograms to \logs\*.html

#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "ix86/ix86.h"
#include "microVU_IR.h"
#include "microVU_Misc.h"


#define mMaxBlocks 32 // Max Blocks With Different Pipeline States (For n = 1, 2, 4, 8, 16, etc...)
class microBlockManager {
private:
	static const int MaxBlocks = mMaxBlocks - 1;
	microBlock blockList[mMaxBlocks]; // Should always be first in the class to ensure 16-byte alignment
	int listSize; // Total Items - 1
	int listI;	  // Index to Add new block

public:
	// Aligned replacement for 'new'
	static microBlockManager* AlignedNew() {
		microBlockManager* alloc = (microBlockManager*)_aligned_malloc(sizeof(microBlockManager), 16);
		new (alloc) microBlockManager();
		return alloc;
	}
	// Use instead of normal 'delete'
	static void Delete(microBlockManager* dead) {
		if (dead == NULL) return;
		dead->~microBlockManager();
		_aligned_free(dead);
	}

	microBlockManager()	 { reset(); }
	~microBlockManager() {}
	void reset()		 { listSize = -1; listI = -1; };
	microBlock* add(microBlock* pBlock) {
		microBlock* thisBlock = search(&pBlock->pState);
		if (!thisBlock) {
			listI++;
			if (listSize < MaxBlocks) { listSize++; }
			if (listI    > MaxBlocks) { Console::Error("microVU Warning: Block List Overflow"); listI = 0; }
			memcpy_fast(&blockList[listI], pBlock, sizeof(microBlock));
			thisBlock = &blockList[listI];
		}
		return thisBlock;
	}
	__forceinline microBlock* search(microRegInfo* pState) {
		if (listSize < 0) return NULL;
		if (pState->needExactMatch) { // Needs Detailed Search (Exact Match of Pipeline State)
			for (int i = 0; i <= listSize; i++) {
				if (mVUquickSearch((void*)pState, (void*)&blockList[i].pState, sizeof(microRegInfo))) return &blockList[i];
			}
		}
		else { // Can do Simple Search (Only Matches the Important Pipeline Stuff)
			for (int i = 0; i <= listSize; i++) {
				if ((blockList[i].pState.q == pState->q)
				&&  (blockList[i].pState.p == pState->p)
				&&  (blockList[i].pState.xgkick == pState->xgkick)
				&&  (blockList[i].pState.flags  == pState->flags)
				&& !(blockList[i].pState.needExactMatch & 0xf0f)) { return &blockList[i]; }
			}
		}
		return NULL;
	}
};

#define mMaxRanges 128
struct microRange { 
	static const int max = mMaxRanges - 1;
	int total;
	s32 range[mMaxRanges][2];
};

#define mProgSize 0x4000/4
struct microProgram {
	u32				   data [mProgSize];   // Holds a copy of the VU microProgram
	microBlockManager* block[mProgSize/2]; // Array of Block Managers
	microIR<mProgSize> allocInfo;		   // IR information
	microRange		   ranges;			   // The ranges of the microProgram that have already been recompiled
	u64 used;		// Number of times its been used
	u32 last_used;	// Counters # of frames since last use (starts at 3 and counts backwards to 0 for each 30fps vSync)
	u8* x86ptr;		// Pointer to program's recompilation code
	u8* x86start;	// Start of program's rec-cache
	u8* x86end;		// Limit of program's rec-cache
};

#define mMaxProg ((mVU->index)?64:8) // The amount of Micro Programs Recs will 'remember' (For n = 1, 2, 4, 8, 16, etc...)
struct microProgManager {
	microProgram*	prog;	 // Store MicroPrograms in memory
	int				max;	 // Max Number of MicroPrograms minus 1
	int				total;	 // Total Number of valid MicroPrograms minus 1
	int				cur;	 // Index to Current MicroProgram thats running (-1 = uncached)
	int				isSame;	 // Current cached microProgram is Exact Same program as mVU->regs->Micro (-1 = unknown, 0 = No, 1 = Yes)
	int				cleared; // Micro Program is Indeterminate so must be searched for (and if no matches are found then recompile a new one)
	microRegInfo	lpState; // Pipeline state from where program left off (useful for continuing execution)
};

#define mVUcacheSize (mMaxProg * 0xCCCCC) // 0.8mb per program
struct microVU {

	PCSX2_ALIGNED16(u32 macFlag[4]);  // 4 instances of mac  flag (used in execution)
	PCSX2_ALIGNED16(u32 clipFlag[4]); // 4 instances of clip flag (used in execution)
	PCSX2_ALIGNED16(u32 xmmPQb[4]);   // Backup for xmmPQ
	PCSX2_ALIGNED16(u32 xmmVFb[4]);   // Backup for VF regs

	u32 index;			// VU Index (VU0 or VU1)
	u32 vuMemSize;		// VU Main  Memory Size (in bytes)
	u32 microMemSize;	// VU Micro Memory Size (in bytes)
	u32 progSize;		// VU Micro Memory Size (in u32's)
	u32 cacheSize;		// VU Cache Size

	microProgManager prog; // Micro Program Data

	FILE*	logFile;	 // Log File Pointer
	VURegs*	regs;		 // VU Regs Struct
	u8*		cache;		 // Dynarec Cache Start (where we will start writing the recompiled code to)
	u8*		startFunct;	 // Ptr Function to the Start code for recompiled programs
	u8*		exitFunct;	 // Ptr Function to the Exit  code for recompiled programs
	u32		code;		 // Contains the current Instruction
	u32		divFlag;	 // 1 instance of I/D flags
	u32		VIbackup;	 // Holds a backup of a VI reg if modified before a branch
	u32		VIxgkick;	 // Holds a backup of a VI reg used for xgkick-delays
	u32		branch;		 // Holds branch compare result (IBxx) OR Holds address to Jump to (JALR/JR)
	u32		p;			 // Holds current P instance index
	u32		q;			 // Holds current Q instance index
	u32		totalCycles; // Total Cycles that mVU is expected to run for
	u32		cycles;		 // Cycles Counter
};

// microVU rec structs
extern PCSX2_ALIGNED16(microVU microVU0);
extern PCSX2_ALIGNED16(microVU microVU1);

// Debug Helper
extern int mVUdebugNow;

// Main Functions
microVUt(void) mVUinit(VURegs*, int);
microVUt(void) mVUreset(mV);
microVUt(void) mVUclose(mV, bool isReset);
microVUt(void) mVUclear(mV, u32, u32);
microVUt(void*) mVUblockFetch(microVU* mVU, u32 startPC, uptr pState);
microVUx(void*) __fastcall mVUcompileJIT(u32 startPC, uptr pState);

// Prototypes for Linux
void  __fastcall mVUcleanUpVU0();
void  __fastcall mVUcleanUpVU1();
mVUop(mVUopU);
mVUop(mVUopL);

// Private Functions
microVUf(int)		mVUfindLeastUsedProg(microVU* mVU);
microVUf(int)		mVUsearchProg();
microVUf(void)		mVUcacheProg(int progIndex);
void* __fastcall	mVUexecuteVU0(u32 startPC, u32 cycles);
void* __fastcall	mVUexecuteVU1(u32 startPC, u32 cycles);

// recCall Function Pointer
typedef void (__fastcall *mVUrecCall)(u32, u32);


// Include all the *.inl files (Needed because C++ sucks with templates and *.cpp files)
#include "microVU_Misc.inl"
#include "microVU_Log.inl"
#include "microVU_Analyze.inl"
#include "microVU_Alloc.inl"
#include "microVU_Upper.inl"
#include "microVU_Lower.inl"
#include "microVU_Tables.inl"
#include "microVU_Flags.inl"
#include "microVU_Compile.inl"
#include "microVU_Execute.inl"
