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

union regInfo {
	u32 reg;
	struct {
		u8 x;
		u8 y;
		u8 z;
		u8 w;
	};
};

#if defined(_MSC_VER)
#pragma pack(push, 1)
#pragma warning(disable:4996)
#endif
struct microRegInfo {
	regInfo VF[32];
	u8 VI[32];
	u8 q;
	u8 p;
	u8 r;
	u8 xgkick;
	u8 flags; // clip x2 :: status x2
	u32 needExactMatch; // If set, block needs an exact match of pipeline state (needs to be last 2 bytes in struct)
#if defined(_MSC_VER)
};
#pragma pack(pop)
#else
} __attribute__((packed));
#endif

struct microTempRegInfo {
	regInfo VF[2];	// Holds cycle info for Fd, VF[0] = Upper Instruction, VF[1] = Lower Instruction
	u8 VFreg[2];	// Index of the VF reg
	u8 VI;			// Holds cycle info for Id
	u8 VIreg;		// Index of the VI reg
	u8 q;			// Holds cycle info for Q reg
	u8 p;			// Holds cycle info for P reg
	u8 r;			// Holds cycle info for R reg (Will never cause stalls, but useful to know if R is modified)
	u8 xgkick;		// Holds the cycle info for XGkick
};

struct microBlock {
	microRegInfo pState;	// Detailed State of Pipeline
	microRegInfo pStateEnd;	// Detailed State of Pipeline at End of Block (needed by JR/JALR opcodes)
	u8* x86ptrStart;		// Start of code
};

template<u32 pSize>
struct microAllocInfo {
	microBlock*		 pBlock;   // Pointer to a block in mVUblocks
	microBlock		 block;	   // Block/Pipeline info
	microTempRegInfo regsTemp; // Temp Pipeline info (used so that new pipeline info isn't conflicting between upper and lower instructions in the same cycle)
	u8  branch;			// 0 = No Branch, 1 = B. 2 = BAL, 3~8 = Conditional Branches, 9 = JALR, 10 = JR
	u32 cycles;			// Cycles for current block
	u32 count;			// Number of VU 64bit instructions ran (starts at 0 for each block)
	u32 curPC;			// Current PC
	u32 startPC;		// Start PC for Cur Block
	u32 sFlagHack;		// Optimize out all Status flag updates if microProgram doesn't use Status flags
	u32 info[pSize/2];	// Info for Instructions in current block
	u8 stall[pSize/2];	// Info on how much each instruction stalled (stores the max amount of cycles to stall for the current opcodes)
};
