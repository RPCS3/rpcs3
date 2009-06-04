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

struct microVFreg {
	u8 reg; // Reg Index
	u8 x;	// X vector read/written to?
	u8 y;	// Y vector read/written to?
	u8 z;	// Z vector read/written to?
	u8 w;	// W vector read/written to?
};

struct microVIreg {
	u8 reg;		// Reg Index
	u8 used;	// Reg is Used? (Read/Written)
};

struct microUpperOp {
	bool eBit;				// Has E-bit set
	bool iBit;				// Has I-bit set
	microVFreg VF_write;	// VF Vectors written to by this instruction
	microVFreg VF_read[2];	// VF Vectors read by this instruction
};

struct microLowerOp {
	microVFreg VF_write;	// VF Vectors written to by this instruction
	microVFreg VF_read[2];	// VF Vectors read by this instruction
	microVIreg VI_write;	// VI reg written to by this instruction
	microVIreg VI_read[2];	// VI regs read by this instruction
	bool isNOP;		// This instruction is a NOP
	bool isFSSET;	// This instruction is a FSSET
	bool useSflag;	// This instruction uses/reads Sflag
	u32  branch;	// Branch Type (0 = Not a Branch, 1 = B. 2 = BAL, 3~8 = Conditional Branches, 9 = JALR, 10 = JR)
	bool noWriteVF;	// Don't write back the result of a lower op to VF reg if upper op writes to same reg (or if VF = 0)
	bool backupVI;	// Backup VI reg to memory if modified before branch (branch uses old VI value unless opcode is ILW or ILWR)
	bool memReadIs;	// Read Is (VI reg) from memory (used by branches)
	bool memReadIt;	// Read If (VI reg) from memory (used by branches)
	bool readFlags; // Current Instruction reads Status, Mac, or Clip flags
};

struct microFlagInst {
	bool doFlag;	  // Update Flag on this Instruction
	bool doNonSticky; // Update O,U,S,Z (non-sticky) bits on this Instruction (status flag only)
	u8	 write;		  // Points to the instance that should be written to (s-stage write)
	u8	 lastWrite;	  // Points to the instance that was last written to (most up-to-date flag)
	u8	 read;		  // Points to the instance that should be read by a lower instruction (t-stage read)
};

struct microOp {
	u8	 stall;			 // Info on how much current instruction stalled
	bool isEOB;			 // Cur Instruction is last instruction in block (End of Block)
	bool isBdelay;		 // Cur Instruction in Branch Delay slot
	bool swapOps;		 // Run Lower Instruction before Upper Instruction
	bool doXGKICK;		 // Do XGKICK transfer on this instruction
	bool doDivFlag;		 // Transfer Div flag to Status Flag on this instruction
	int	 readQ;			 // Q instance for reading
	int	 writeQ;		 // Q instance for writing
	int	 readP;			 // P instance for reading
	int	 writeP;		 // P instance for writing
	microFlagInst sFlag; // Status Flag Instance Info
	microFlagInst mFlag; // Mac	   Flag Instance Info
	microFlagInst cFlag; // Clip   Flag Instance Info
	microUpperOp  uOp;	 // Upper Op Info
	microLowerOp  lOp;	 // Lower Op Info
};

template<u32 pSize>
struct microIR {
	microBlock*		 pBlock;   // Pointer to a block in mVUblocks
	microBlock		 block;	   // Block/Pipeline info
	microTempRegInfo regsTemp; // Temp Pipeline info (used so that new pipeline info isn't conflicting between upper and lower instructions in the same cycle)
	microOp			 info[pSize/2];	// Info for Instructions in current block
	u8  branch;			
	u32 cycles;			// Cycles for current block
	u32 count;			// Number of VU 64bit instructions ran (starts at 0 for each block)
	u32 curPC;			// Current PC
	u32 startPC;		// Start PC for Cur Block
	u32 sFlagHack;		// Optimize out all Status flag updates if microProgram doesn't use Status flags
};
