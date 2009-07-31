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
#pragma pack(1)
#pragma warning(disable:4996)
#endif

__declspec(align(16)) struct microRegInfo { // Ordered for Faster Compares
	u32 needExactMatch;	// If set, block needs an exact match of pipeline state
	u32 vi15;			// Constant Prop Info for vi15 (only valid if sign-bit set)
	u8 q;
	u8 p;
	u8 r;
	u8 xgkick;
	u8 VI[16];
	regInfo VF[32];
	u8 flags;			// clip x2 :: status x2
	u8 padding[3];		// 160 bytes
#if defined(_MSC_VER)
};
#else
} __attribute__((packed));
#endif

__declspec(align(16)) struct microBlock {
	microRegInfo pState;	// Detailed State of Pipeline
	microRegInfo pStateEnd; // Detailed State of Pipeline at End of Block (needed by JR/JALR opcodes)
	u8* x86ptrStart;		// Start of code
#if defined(_MSC_VER)
};
#pragma pack()
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

struct microOp {
	u8	 stall;			 // Info on how much current instruction stalled
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

void mVUmergeRegs(int dest, int src,  int xyzw, bool modXYZW);
void mVUsaveReg(int reg, uptr offset, int xyzw, bool modXYZW);
void mVUloadReg(int reg, uptr offset, int xyzw);

struct microXMM {
	int  reg;		// VF Reg Number Stored (-1 = Temp; 0 = vf0 and will not be written back; 32 = ACC)
	int  xyzw;		// xyzw to write back (0 = Don't write back anything AND cached vfReg has all vectors valid)
	int  count;		// Count of when last used
	bool isNeeded;	// Is needed for current instruction
};

#define xmmTotal 7	// Don't allocate PQ?
class microRegAlloc {
private:
	microXMM xmmReg[xmmTotal];
	VURegs*  vuRegs;
	int		 counter;
	int findFreeRegRec(int startIdx) {
		for (int i = startIdx; i < xmmTotal; i++) {
			if (!xmmReg[i].isNeeded) {
				int x = findFreeRegRec(i+1);
				if (x == -1) return i;
				return ((xmmReg[i].count < xmmReg[x].count) ? i : x);
			}
		}
		return -1;
	}
	int findFreeReg() {
		for (int i = 0; i < xmmTotal; i++) {
			if (!xmmReg[i].isNeeded && (xmmReg[i].reg < 0)) {
				return i; // Reg is not needed and was a temp reg
			}
		}
		int x = findFreeRegRec(0);
		if (x < 0) { DevCon::Error("microVU Allocation Error!"); return 0; }
		return x;
	}

public:
	microRegAlloc(VURegs* vuRegsPtr) { 
		vuRegs = vuRegsPtr;
		reset(); 
	}
	void reset() {
		for (int i = 0; i < xmmTotal; i++) {
			clearReg(i);
		}
		counter = 0;
	}
	void flushAll(bool clearState = 1) {
		for (int i = 0; i < xmmTotal; i++) {
			writeBackReg(i);
			if (clearState) clearReg(i);
		}
	}
	void clearReg(int reg) {
		xmmReg[reg].reg		 = -1;
		xmmReg[reg].count	 =  0;
		xmmReg[reg].xyzw	 =  0;
		xmmReg[reg].isNeeded =  0;
	}
	void writeBackReg(int reg, bool invalidateRegs = 1) {
		if ((xmmReg[reg].reg > 0) && xmmReg[reg].xyzw) { // Reg was modified and not Temp or vf0
			if (xmmReg[reg].reg == 32) mVUsaveReg(reg, (uptr)&vuRegs->ACC.UL[0], xmmReg[reg].xyzw, 1);
			else mVUsaveReg(reg, (uptr)&vuRegs->VF[xmmReg[reg].reg].UL[0], xmmReg[reg].xyzw, 1);
			if (invalidateRegs) {
				for (int i = 0; i < xmmTotal; i++) {
					if ((i == reg) || xmmReg[i].isNeeded) continue;
					if (xmmReg[i].reg == xmmReg[reg].reg) {
						if (xmmReg[i].xyzw && xmmReg[i].xyzw < 0xf) DevCon::Error("microVU Error: writeBackReg() [%d]", params xmmReg[i].reg);
						clearReg(i); // Invalidate any Cached Regs of same vf Reg
					}
				}
			}
			if (xmmReg[reg].xyzw == 0xf) { // Make Cached Reg if All Vectors were Modified
				xmmReg[reg].count	 = counter;
				xmmReg[reg].xyzw	 = 0;
				xmmReg[reg].isNeeded = 0;
				return;
			}
		}
		clearReg(reg); // Clear Reg
	}
	void clearNeeded(int reg) {
		if ((reg < 0) || (reg >= xmmTotal)) return;
		xmmReg[reg].isNeeded = 0;
		if (xmmReg[reg].xyzw) { // Reg was modified
			if (xmmReg[reg].reg > 0) {
				int mergeRegs = 0;
				if (xmmReg[reg].xyzw < 0xf) { mergeRegs = 1; } // Try to merge partial writes
				for (int i = 0; i < xmmTotal; i++) { // Invalidate any other read-only regs of same vfReg
					if (i == reg) continue;
					if (xmmReg[i].reg == xmmReg[reg].reg) {
						if (xmmReg[i].xyzw && xmmReg[i].xyzw < 0xf) DevCon::Error("microVU Error: clearNeeded() [%d]", params xmmReg[i].reg);
						if (mergeRegs == 1) { 
							mVUmergeRegs(i, reg, xmmReg[reg].xyzw, 1);
							xmmReg[i].xyzw = 0xf;
							xmmReg[i].count = counter;
							mergeRegs = 2; 
						}
						else clearReg(i);
					}
				}
				if (mergeRegs == 2) clearReg(reg);	   // Clear Current Reg if Merged
				else if (mergeRegs) writeBackReg(reg); // Write Back Partial Writes if couldn't merge
			}
			else clearReg(reg); // If Reg was temp or vf0, then invalidate itself
		}
	}
	int allocReg(int vfLoadReg = -1, int vfWriteReg = -1, int xyzw = 0, bool cloneWrite = 1) {
		counter++;
		if (vfLoadReg >= 0) { // Search For Cached Regs
			for (int i = 0; i < xmmTotal; i++) {
				if ((xmmReg[i].reg == vfLoadReg) && (!xmmReg[i].xyzw // Reg Was Not Modified
				||  (xmmReg[i].reg && (xmmReg[i].xyzw==0xf)))) {	 // Reg Had All Vectors Modified and != VF0
					int z = i;
					if (vfWriteReg >= 0) { // Reg will be modified
						if (cloneWrite) {  // Clone Reg so as not to use the same Cached Reg
							z = findFreeReg();
							writeBackReg(z);
							if (z!=i && xyzw==8) SSE_MOVAPS_XMM_to_XMM (z, i);
							else if (xyzw == 4)  SSE2_PSHUFD_XMM_to_XMM(z, i, 1);
							else if (xyzw == 2)  SSE2_PSHUFD_XMM_to_XMM(z, i, 2);
							else if (xyzw == 1)  SSE2_PSHUFD_XMM_to_XMM(z, i, 3);
							else if (z != i)	 SSE_MOVAPS_XMM_to_XMM (z, i);
							xmmReg[i].count = counter; // Reg i was used, so update counter
						}
						else { // Don't clone reg, but shuffle to adjust for SS ops
							if ((vfLoadReg != vfWriteReg) || (xyzw != 0xf)) { writeBackReg(z); }
							if		(xyzw == 4) SSE2_PSHUFD_XMM_to_XMM(z, i, 1);
							else if (xyzw == 2) SSE2_PSHUFD_XMM_to_XMM(z, i, 2);
							else if (xyzw == 1) SSE2_PSHUFD_XMM_to_XMM(z, i, 3);
						}
						xmmReg[z].reg  = vfWriteReg;
						xmmReg[z].xyzw = xyzw;
					}
					xmmReg[z].count	   = counter;
					xmmReg[z].isNeeded = 1;
					return z;
				}
			}
		}
		int x = findFreeReg();
		writeBackReg(x);

		if (vfWriteReg >= 0) { // Reg Will Be Modified (allow partial reg loading)
			if	   ((vfLoadReg ==  0) && !(xyzw & 1)) { SSE2_PXOR_XMM_to_XMM(x, x); }
			else if	(vfLoadReg == 32) mVUloadReg(x, (uptr)&vuRegs->ACC.UL[0], xyzw);
			else if (vfLoadReg >=  0) mVUloadReg(x, (uptr)&vuRegs->VF[vfLoadReg].UL[0], xyzw);
			xmmReg[x].reg  = vfWriteReg;
			xmmReg[x].xyzw = xyzw;
		}
		else { // Reg Will Not Be Modified (always load full reg for caching)
			if		(vfLoadReg == 32) SSE_MOVAPS_M128_to_XMM(x, (uptr)&vuRegs->ACC.UL[0]);
			else if (vfLoadReg >=  0) SSE_MOVAPS_M128_to_XMM(x, (uptr)&vuRegs->VF[vfLoadReg].UL[0]);
			xmmReg[x].reg  = vfLoadReg;
			xmmReg[x].xyzw = 0;
		}
		xmmReg[x].count	   = counter;
		xmmReg[x].isNeeded = 1;
		return x;
	}
};
