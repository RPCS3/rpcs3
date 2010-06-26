/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
//#define mVUlogProg // Dumps MicroPrograms to \logs\*.html

class AsciiFile;
using namespace std;
using namespace x86Emitter;

#include <deque>

#include "VU.h"
#include "GS.h"
#include "Gif.h"
#include "iR5900.h"
#include "R5900OpcodeTables.h"
#include "x86emitter/x86emitter.h"
#include "SamplProf.h"
#include "microVU_IR.h"
#include "microVU_Misc.h"

struct microBlockLink {
	microBlock*		block;
	microBlockLink*	next;
};

class microBlockManager {
private:
	microBlockLink  blockList;
	microBlockLink* blockEnd;
	int listI;

public:
	microBlockManager() {
		listI = -1;
		blockList.block = NULL;
		blockList.next  = NULL;
		blockEnd = &blockList;
	}
	~microBlockManager() { reset(); }
	void reset() {
		if (listI >= 0) {
			microBlockLink* linkI = &blockList;
			microBlockLink* linkD = NULL;
			for (int i = 0; i <= listI; i++) {
				safe_aligned_free(linkI->block);
				linkI = linkI->next;
				safe_delete(linkD);
				linkD = linkI;
			}
			safe_delete(linkI);
		}
		listI = -1;
		blockEnd = &blockList;
	};
	microBlock* add(microBlock* pBlock) {
		microBlock* thisBlock = search(&pBlock->pState);
		if (!thisBlock) {
			listI++;
			blockEnd->block = (microBlock*)_aligned_malloc(sizeof(microBlock), 16);
			blockEnd->next  = new microBlockLink;
			memcpy_const(blockEnd->block, pBlock, sizeof(microBlock));
			thisBlock =  blockEnd->block;
			blockEnd  =  blockEnd->next;
		}
		return thisBlock;
	}
	__releaseinline microBlock* search(microRegInfo* pState) {
		microBlockLink* linkI = &blockList;
		if (pState->needExactMatch) { // Needs Detailed Search (Exact Match of Pipeline State)
			for (int i = 0; i <= listI; i++) {
				if (mVUquickSearch((void*)pState, (void*)&linkI->block->pState, sizeof(microRegInfo))) return linkI->block;
				linkI = linkI->next;
			}
		}
		else { // Can do Simple Search (Only Matches the Important Pipeline Stuff)
			for (int i = 0; i <= listI; i++) {
				if ((linkI->block->pState.q			== pState->q)
				&&  (linkI->block->pState.p			== pState->p)
				&& ((linkI->block->pState.vi15		== pState->vi15) || !CHECK_VU_CONSTPROP)
				&&  (linkI->block->pState.flags		== pState->flags)
				&&  (linkI->block->pState.xgkick	== pState->xgkick)
				&&  (linkI->block->pState.viBackUp	== pState->viBackUp)
				&&  (linkI->block->pState.blockType	== pState->blockType)
				&& !(linkI->block->pState.needExactMatch & 5)) { return linkI->block; }
				linkI = linkI->next;
			}
		}
		return NULL;
	}
	void printInfo(int pc) {
		if (listI < 7) return;
		microBlockLink* linkI = &blockList;
		for (int i = 0; i <= listI; i++) {
			u32 viCRC = 0, vfCRC = 0, crc = 0, z = sizeof(microRegInfo)/4;
			for (u32 j = 0; j < 4;  j++) viCRC -= ((u32*)linkI->block->pState.VI)[j];
			for (u32 j = 0; j < 32; j++) vfCRC -= linkI->block->pState.VF[j].reg;
			for (u32 j = 0; j < z;  j++) crc   -= ((u32*)&linkI->block->pState)[j];
			DevCon.WriteLn(Color_Green, "[%04x][Block #%d][crc=%08x][q=%02d][p=%02d][xgkick=%d][vi15=%08x][viBackup=%02d]"
			"[flags=%02x][exactMatch=%x][blockType=%d][viCRC=%08x][vfCRC=%08x]", pc, i, crc, linkI->block->pState.q, 
			linkI->block->pState.p, linkI->block->pState.xgkick, linkI->block->pState.vi15, linkI->block->pState.viBackUp, 
			linkI->block->pState.flags, linkI->block->pState.needExactMatch, linkI->block->pState.blockType, viCRC, vfCRC);
			linkI = linkI->next;
		}
	}
};

struct microRange {
	s32 start; // Start PC (The opcode the block starts at)
	s32 end;   // End PC   (The opcode the block ends with)
};

#define mProgSize (0x4000/4)
struct microProgram {
	u32				   data [mProgSize];   // Holds a copy of the VU microProgram
	microBlockManager* block[mProgSize/2]; // Array of Block Managers
	deque<microRange>* ranges;			   // The ranges of the microProgram that have already been recompiled
	u32 startPC; // Start PC of this program
	int idx;	 // Program index
};

struct microProgramList {
	deque<microProgram*>* list;  // List of microPrograms who start with the same startPC value
};

struct microProgramQuick {
	microBlockManager*    block; // Quick reference to valid microBlockManager for current startPC
	microProgram*		  prog;	 // The microProgram who is the owner of 'block'
};

struct microProgManager {
	microIR<mProgSize>	IRinfo;				// IR information
	microProgramList	prog [mProgSize/2];	// List of microPrograms indexed by startPC values
	microProgramQuick	quick[mProgSize/2];	// Quick reference to valid microPrograms for current execution
	microProgram*		cur;				// Pointer to currently running MicroProgram
	int					total;				// Total Number of valid MicroPrograms
	int					isSame;				// Current cached microProgram is Exact Same program as mVU->regs->Micro (-1 = unknown, 0 = No, 1 = Yes)
	int					cleared;			// Micro Program is Indeterminate so must be searched for (and if no matches are found then recompile a new one)
	u32					curFrame;			// Frame Counter
	u8*					x86ptr;				// Pointer to program's recompilation code
	u8*					x86start;			// Start of program's rec-cache
	u8*					x86end;				// Limit of program's rec-cache
	microRegInfo		lpState;			// Pipeline state from where program left off (useful for continuing execution)
};

#define mVUcacheSize ((mVU->index) ? (_1mb * 20) : (_1mb * 8))
struct microVU {

	__aligned16 u32 macFlag[4];  // 4 instances of mac  flag (used in execution)
	__aligned16 u32 clipFlag[4]; // 4 instances of clip flag (used in execution)
	__aligned16 u32 xmmPQb[4];   // Backup for xmmPQ
	__aligned16 u32 xmmCTemp[4]; // Backup used in mVUclamp2()

	u32 index;			// VU Index (VU0 or VU1)
	u32 cop2;			// VU is in COP2 mode?  (No/Yes)
	u32 vuMemSize;		// VU Main  Memory Size (in bytes)
	u32 microMemSize;	// VU Micro Memory Size (in bytes)
	u32 progSize;		// VU Micro Memory Size (in u32's)
	u32 cacheSize;		// VU Cache Size

	microProgManager prog;		// Micro Program Data
	microRegAlloc*	 regAlloc;	// Reg Alloc Class

	AsciiFile* logFile;	 // Log File Pointer
	VURegs*	regs;		 // VU Regs Struct
	u8*		cache;		 // Dynarec Cache Start (where we will start writing the recompiled code to)
	u8*		startFunct;	 // Ptr Function to the Start code for recompiled programs
	u8*		exitFunct;	 // Ptr Function to the Exit  code for recompiled programs
	u32		code;		 // Contains the current Instruction
	u32		divFlag;	 // 1 instance of I/D flags
	u32		VIbackup;	 // Holds a backup of a VI reg if modified before a branch
	u32		VIxgkick;	 // Holds a backup of a VI reg used for xgkick-delays
	u32		branch;		 // Holds branch compare result (IBxx) OR Holds address to Jump to (JALR/JR)
	u32		badBranch;	 // For Branches in Branch Delay Slots, holds Address the first Branch went to + 8
	u32		evilBranch;	 // For Branches in Branch Delay Slots, holds Address to Jump to
	u32		p;			 // Holds current P instance index
	u32		q;			 // Holds current Q instance index
	u32		totalCycles; // Total Cycles that mVU is expected to run for
	u32		cycles;		 // Cycles Counter
};

// microVU rec structs
extern __aligned16 microVU microVU0;
extern __aligned16 microVU microVU1;

// Debug Helper
extern int mVUdebugNow;

// Main Functions
_f void  mVUinit(VURegs*, int);
_f void  mVUreset(mV);
_f void  mVUclose(mV);
_f void  mVUclear(mV, u32, u32);
_f void* mVUblockFetch(microVU* mVU, u32 startPC, uptr pState);
_mVUt void* __fastcall mVUcompileJIT(u32 startPC, uptr pState);

// Prototypes for Linux
void  __fastcall mVUcleanUpVU0();
void  __fastcall mVUcleanUpVU1();
mVUop(mVUopU);
mVUop(mVUopL);

// Private Functions
_mVUt _f void  mVUcacheProg (microProgram&  prog);
_mVUt _f void  mVUdeleteProg(microProgram*& prog);
_mVUt _f void* mVUsearchProg(u32 startPC, uptr pState);
_mVUt _f microProgram* mVUfindLeastUsedProg();
void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles);
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles);

// recCall Function Pointer
typedef void (__fastcall *mVUrecCall)(u32, u32);


// Include all the *.inl files (Needed because C++ sucks with templates and *.cpp files)
#include "microVU_Clamp.inl"
#include "microVU_Misc.inl"
#include "microVU_Log.inl"
#include "microVU_Analyze.inl"
#include "microVU_Alloc.inl"
#include "microVU_Upper.inl"
#include "microVU_Lower.inl"
#include "microVU_Tables.inl"
#include "microVU_Flags.inl"
#include "microVU_Branch.inl"
#include "microVU_Compile.inl"
#include "microVU_Execute.inl"
#include "microVU_Macro.inl"
