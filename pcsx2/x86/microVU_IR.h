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

union __aligned16 microRegInfo { // Ordered for Faster Compares
	struct  
	{
		u32 vi15;			// Constant Prop Info for vi15 (only valid if sign-bit set)

		union
		{
			struct
			{
				u8 needExactMatch;	// If set, block needs an exact match of pipeline state
				u8 q;
				u8 p;
				u8 r;
				u8 xgkick;
				u8 viBackUp;		// VI reg number that was written to on branch-delay slot
				u8 blockType;		// 0 = Normal; 1,2 = Compile one instruction (E-bit/Branch Ending)
			};
			u32 simple32[2];
		};
	
		struct
		{
			u8 VI[16];
			regInfo VF[32];
			u8 flags;			// clip x2 :: status x2
		};
	};
	
	u128 full128[160/sizeof(u128)];
	u64 full64[160/sizeof(u64)];
	u32 full32[160/sizeof(u32)];
};

C_ASSERT( sizeof(microRegInfo) == 160 );

struct __aligned16 microBlock {
	microRegInfo pState;	// Detailed State of Pipeline
	microRegInfo pStateEnd; // Detailed State of Pipeline at End of Block (needed by JR/JALR opcodes)
	u8* x86ptrStart;		// Start of code
};

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

class microRegAlloc {
protected:
	static const u32   xmmTotal = 7; // Don't allocate PQ?
	microMapXMM	xmmMap[xmmTotal];
	int			counter; // Current allocation count
	int			index;   // VU0 or VU1

	// Helper functions to get VU regs
	VURegs& regs()				 const	{ return ::vuRegs[index]; }
	__fi REG_VI& getVI(uint reg) const	{ return regs().VI[reg]; }
	__fi VECTOR& getVF(uint reg) const	{ return regs().VF[reg]; }

	__ri void loadIreg(const xmm& reg, int xyzw) {
		xMOVSSZX(reg, ptr32[&getVI(REG_I)]);
		if (!_XYZWss(xyzw)) xSHUF.PS(reg, reg, 0);
	}
	
	int findFreeRegRec(int startIdx) {
		for (int i = startIdx; i < xmmTotal; i++) {
			if (!xmmMap[i].isNeeded) {
				int x = findFreeRegRec(i+1);
				if (x == -1) return i;
				return ((xmmMap[i].count < xmmMap[x].count) ? i : x);
			}
		}
		return -1;
	}

	int findFreeReg() {
		for (int i = 0; i < xmmTotal; i++) {
			if (!xmmMap[i].isNeeded && (xmmMap[i].VFreg < 0)) {
				return i; // Reg is not needed and was a temp reg
			}
		}
		int x = findFreeRegRec(0);
		pxAssumeDev( x >= 0, "microVU register allocation failure!" );
		return x;
	}

public:
	microRegAlloc(int _index) {
		index = _index;
	}

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

	void clearReg(const xmm& reg) { clearReg(reg.Id); }
	void clearReg(int regId) {
		microMapXMM& clear( xmmMap[regId] );
		clear.VFreg		= -1;
		clear.count		=  0;
		clear.xyzw		=  0;
		clear.isNeeded	=  0;
	}

	void clearRegVF(int VFreg) {
		for (int i = 0; i < xmmTotal; i++) {
			if (xmmMap[i].VFreg == VFreg) clearReg(i);
		}
	}

	void writeBackReg(const xmm& reg, bool invalidateRegs = 1) {
		microMapXMM& write( xmmMap[reg.Id] );

		if ((write.VFreg > 0) && write.xyzw) { // Reg was modified and not Temp or vf0
			if		(write.VFreg == 33) xMOVSS(ptr32[&getVI(REG_I)], reg);
			else if (write.VFreg == 32) mVUsaveReg(reg, ptr[&regs().ACC],		  write.xyzw, 1);
			else						mVUsaveReg(reg, ptr[&getVF(write.VFreg)], write.xyzw, 1);
			if (invalidateRegs) {
				for (int i = 0; i < xmmTotal; i++) {
					microMapXMM& imap (xmmMap[i]);
					if ((i == reg.Id) || imap.isNeeded) continue;
					if (imap.VFreg == write.VFreg) {
						if (imap.xyzw && imap.xyzw < 0xf) DevCon.Error("microVU Error: writeBackReg() [%d]", imap.VFreg);
						clearReg(i); // Invalidate any Cached Regs of same vf Reg
					}
				}
			}
			if (write.xyzw == 0xf) { // Make Cached Reg if All Vectors were Modified
				write.count	 = counter;
				write.xyzw	 = 0;
				write.isNeeded = 0;
				return;
			}
		}
		clearReg(reg); // Clear Reg
	}

	void clearNeeded(const xmm& reg) {

		if ((reg.Id < 0) || (reg.Id >= xmmTotal)) return;

		microMapXMM& clear (xmmMap[reg.Id]);
		clear.isNeeded = 0;
		if (clear.xyzw) { // Reg was modified
			if (clear.VFreg > 0) {
				int mergeRegs = 0;
				if (clear.xyzw < 0xf) { mergeRegs = 1; } // Try to merge partial writes
				for (int i = 0; i < xmmTotal; i++) { // Invalidate any other read-only regs of same vfReg
					if (i == reg.Id) continue;
					microMapXMM& imap (xmmMap[i]);
					if (imap.VFreg == clear.VFreg) {
						if (imap.xyzw && imap.xyzw < 0xf) DevCon.Error("microVU Error: clearNeeded() [%d]", imap.VFreg);
						if (mergeRegs == 1) {
							mVUmergeRegs(xmm(i), reg, clear.xyzw, 1);
							imap.xyzw = 0xf;
							imap.count = counter;
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

	const xmm& allocReg(int vfLoadReg = -1, int vfWriteReg = -1, int xyzw = 0, bool cloneWrite = 1) {
		counter++;
		if (vfLoadReg >= 0) { // Search For Cached Regs
			for (int i = 0; i < xmmTotal; i++) {
				const xmm& xmmi(xmm::GetInstance(i));
				microMapXMM& imap (xmmMap[i]);
				if ((imap.VFreg == vfLoadReg) && (!imap.xyzw // Reg Was Not Modified
				||  (imap.VFreg && (imap.xyzw==0xf)))) {	 // Reg Had All Vectors Modified and != VF0
					int z = i;
					if (vfWriteReg >= 0) { // Reg will be modified
						if (cloneWrite) {  // Clone Reg so as not to use the same Cached Reg
							z = findFreeReg();
							const xmm& xmmz(xmm::GetInstance(z));
							writeBackReg(xmmz);
							if (z!=i && xyzw==8) xMOVAPS (xmmz, xmmi);
							else if (xyzw == 4)  xPSHUF.D(xmmz, xmmi, 1);
							else if (xyzw == 2)  xPSHUF.D(xmmz, xmmi, 2);
							else if (xyzw == 1)  xPSHUF.D(xmmz, xmmi, 3);
							else if (z != i)	 xMOVAPS (xmmz, xmmi);
							imap.count = counter; // Reg i was used, so update counter
						}
						else { // Don't clone reg, but shuffle to adjust for SS ops
							if ((vfLoadReg != vfWriteReg) || (xyzw != 0xf)) { writeBackReg(xmmi); }
							if		(xyzw == 4) xPSHUF.D(xmmi, xmmi, 1);
							else if (xyzw == 2) xPSHUF.D(xmmi, xmmi, 2);
							else if (xyzw == 1) xPSHUF.D(xmmi, xmmi, 3);
						}
						xmmMap[z].VFreg  = vfWriteReg;
						xmmMap[z].xyzw = xyzw;
					}
					xmmMap[z].count	   = counter;
					xmmMap[z].isNeeded = 1;
					return xmm::GetInstance(z);
				}
			}
		}
		int x = findFreeReg();
		const xmm& xmmx = xmm::GetInstance(x);
		writeBackReg(xmmx);

		if (vfWriteReg >= 0) { // Reg Will Be Modified (allow partial reg loading)
			if	   ((vfLoadReg ==  0) && !(xyzw & 1)) { xPXOR(xmmx, xmmx); }
			else if	(vfLoadReg == 33) loadIreg  (xmmx, xyzw);
			else if	(vfLoadReg == 32) mVUloadReg(xmmx, ptr[&regs().ACC], xyzw);
			else if (vfLoadReg >=  0) mVUloadReg(xmmx, ptr[&getVF(vfLoadReg)], xyzw);
			xmmMap[x].VFreg  = vfWriteReg;
			xmmMap[x].xyzw = xyzw;
		}
		else { // Reg Will Not Be Modified (always load full reg for caching)
			if		(vfLoadReg == 33) loadIreg(xmmx, 0xf);
			else if	(vfLoadReg == 32) xMOVAPS (xmmx, ptr128[&regs().ACC]);
			else if (vfLoadReg >=  0) xMOVAPS (xmmx, ptr128[&getVF(vfLoadReg)]);
			xmmMap[x].VFreg  = vfLoadReg;
			xmmMap[x].xyzw = 0;
		}
		xmmMap[x].count	   = counter;
		xmmMap[x].isNeeded = 1;
		return xmmx;
	}
};
