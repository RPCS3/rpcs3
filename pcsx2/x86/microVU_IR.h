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

union regInfo {
	u32 reg;
	struct {
		u8 x;
		u8 y;
		u8 z;
		u8 w;
	};
};

#ifdef _MSC_VER
#	pragma pack(1)
#endif

struct __aligned16 microRegInfo { // Ordered for Faster Compares
	u32 vi15;			// Constant Prop Info for vi15 (only valid if sign-bit set)
	u8 needExactMatch;	// If set, block needs an exact match of pipeline state
	u8 q;
	u8 p;
	u8 r;
	u8 xgkick;
	u8 viBackUp;		// VI reg number that was written to on branch-delay slot
	u8 VI[16];
	regInfo VF[32];
	u8 flags;			// clip x2 :: status x2
	u8 blockType;		// 0 = Normal; 1,2 = Compile one instruction (E-bit/Branch Ending)
	u8 padding[5];		// 160 bytes
} __packed;

struct __aligned16 microBlock {
	microRegInfo pState;	// Detailed State of Pipeline
	microRegInfo pStateEnd; // Detailed State of Pipeline at End of Block (needed by JR/JALR opcodes)
	u8* x86ptrStart;		// Start of code
} __packed;

#ifdef _MSC_VER
#	pragma pack()
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

struct microConstInfo {
	u8	isValid;  // Is the constant in regValue valid?
	u32	regValue; // Constant Value
};

struct microUpperOp {
	bool eBit;				// Has E-bit set
	bool iBit;				// Has I-bit set
	bool mBit;				// Has M-bit set
	microVFreg VF_write;	// VF Vectors written to by this instruction
	microVFreg VF_read[2];	// VF Vectors read by this instruction
};

struct microLowerOp {
	microVFreg VF_write;	  // VF Vectors written to by this instruction
	microVFreg VF_read[2];	  // VF Vectors read by this instruction
	microVIreg VI_write;	  // VI reg written to by this instruction
	microVIreg VI_read[2];	  // VI regs read by this instruction
	microConstInfo constJump; // Constant Reg Info for JR/JARL instructions
	u32  branch;	// Branch Type (0 = Not a Branch, 1 = B. 2 = BAL, 3~8 = Conditional Branches, 9 = JALR, 10 = JR)
	bool badBranch; // This instruction is a Branch who has another branch in its Delay Slot
	bool evilBranch;// This instruction is a Branch in a Branch Delay Slot (Instruction after badBranch)
	bool isNOP;		// This instruction is a NOP
	bool isFSSET;	// This instruction is a FSSET
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

struct microFlagCycles {
	int xStatus[4];
	int xMac[4];
	int xClip[4];
	int cycles;
};

struct microOp {
	u8	 stall;			 // Info on how much current instruction stalled
	bool isBadOp;		 // Cur Instruction is a bad opcode (not a legal instruction)
	bool isEOB;			 // Cur Instruction is last instruction in block (End of Block)
	bool isBdelay;		 // Cur Instruction in Branch Delay slot
	bool swapOps;		 // Run Lower Instruction before Upper Instruction
	bool backupVF;		 // Backup mVUlow.VF_write.reg, and restore it before the Upper Instruction is called
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
	microBlock		 block;			// Block/Pipeline info
	microBlock*		 pBlock;		// Pointer to a block in mVUblocks
	microTempRegInfo regsTemp;		// Temp Pipeline info (used so that new pipeline info isn't conflicting between upper and lower instructions in the same cycle)
	microOp			 info[pSize/2];	// Info for Instructions in current block
	microConstInfo	 constReg[16];	// Simple Const Propagation Info for VI regs within blocks
	u8  branch;
	u32 cycles;		// Cycles for current block
	u32 count;		// Number of VU 64bit instructions ran (starts at 0 for each block)
	u32 curPC;		// Current PC
	u32 startPC;	// Start PC for Cur Block
	u32 sFlagHack;	// Optimize out all Status flag updates if microProgram doesn't use Status flags
};

//------------------------------------------------------------------
// Reg Alloc
//------------------------------------------------------------------

struct microMapXMM {
	int  VFreg;		// VF Reg Number Stored (-1 = Temp; 0 = vf0 and will not be written back; 32 = ACC; 33 = I reg)
	int  xyzw;		// xyzw to write back (0 = Don't write back anything AND cached vfReg has all vectors valid)
	int  count;		// Count of when last used
	bool isNeeded;	// Is needed for current instruction
};

#define xmmTotal 7	// Don't allocate PQ?
class microRegAlloc {
protected:
	microMapXMM	xmmMap[xmmTotal];
	int			counter;
	microVU*	mVU;
	
	int findFreeRegRec(int startIdx);
	int findFreeReg();

public:
	microRegAlloc(microVU* _mVU);
	
	void reset() {
		for (int i = 0; i < xmmTotal; i++) {
			clearReg(i);
		}
		counter = 0;
	}
	void flushAll(bool clearState = 1) {
		for (int i = 0; i < xmmTotal; i++) {
			writeBackReg(xmm(i));
			if (clearState) clearReg(i);
		}
	}
	void clearReg(int regId);
	void clearReg(const xmm& reg) { clearReg(reg.Id); }
	void clearRegVF(int VFreg) {
		for (int i = 0; i < xmmTotal; i++) {
			if (xmmMap[i].VFreg == VFreg) clearReg(i);
		}
	}
	void writeBackReg(const xmm& reg, bool invalidateRegs = 1);
	void clearNeeded(const xmm& reg);
	const xmm& allocReg(int vfLoadReg = -1, int vfWriteReg = -1, int xyzw = 0, bool cloneWrite = 1);
};
