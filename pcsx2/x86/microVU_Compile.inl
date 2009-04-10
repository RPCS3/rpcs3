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
#ifdef PCSX2_MICROVU

#define createBlock(blockEndPtr) {									\
	block.pipelineState = pipelineState;							\
	block.x86ptrStart = x86ptrStart;								\
	block.x86ptrEnd = blockEndPtr;									\
	/*block.x86ptrBranch;*/											\
	if (!(pipelineState & 1)) {										\
		memcpy_fast(&block.pState, pState, sizeof(microRegInfo));	\
	}																\
}

#define startLoop()			{ mVUdebug1(); mVUstall = 0; memset(&mVUregsTemp, 0, sizeof(mVUregsTemp)); }
#define calcCycles(reg, x)	{ reg = ((reg > x) ? (reg - x) : 0); }

microVUt(void) mVUincCycles(int x) {
	mVUcycles += x;
	for (int z = 31; z > 0; z--) {
		calcCycles(mVUregs.VF[z].x, x);
		calcCycles(mVUregs.VF[z].y, x);
		calcCycles(mVUregs.VF[z].z, x);
		calcCycles(mVUregs.VF[z].w, x);
	}
	for (int z = 16; z > 0; z--) {
		calcCycles(mVUregs.VI[z], x);
	}
	if (mVUregs.q) {
		calcCycles(mVUregs.q, x);
		if (!mVUregs.q) {} // Do Status Flag Merging Stuff?
	}
	calcCycles(mVUregs.p, x);
	calcCycles(mVUregs.r, x);
	calcCycles(mVUregs.xgkick, x);
}

microVUt(void) mVUsetCycles() {
	microVU* mVU = mVUx;
	incCycles(mVUstall);
	if (mVUregsTemp.VFreg[0] == mVUregsTemp.VFreg[1] && !mVUregsTemp.VFreg[0]) { // If upper Op && lower Op write to same VF reg
		mVUinfo |= (mVUregsTemp.r || mVUregsTemp.VI) ? _noWriteVF : _isNOP;		 // If lower Op doesn't modify anything else, then make it a NOP
		//mVUregsTemp.VF[1].reg = mVUregsTemp.VF[0];							 // Just use cycles from upper Op (incorrect?)
		mVUregsTemp.VF[1].x = aMax(mVUregsTemp.VF[0].x, mVUregsTemp.VF[1].x);	 // Use max cycles from each vector (correct?)
		mVUregsTemp.VF[1].y = aMax(mVUregsTemp.VF[0].y, mVUregsTemp.VF[1].y);
		mVUregsTemp.VF[1].z = aMax(mVUregsTemp.VF[0].z, mVUregsTemp.VF[1].z);
		mVUregsTemp.VF[1].w = aMax(mVUregsTemp.VF[0].w, mVUregsTemp.VF[1].w);
	}
	mVUregs.VF[mVUregsTemp.VFreg[0]].reg = mVUregsTemp.VF[0].reg;
	mVUregs.VF[mVUregsTemp.VFreg[1]].reg = mVUregsTemp.VF[1].reg;
	mVUregs.VI[mVUregsTemp.VIreg]		 = mVUregsTemp.VI;
	mVUregs.q							 = mVUregsTemp.q;
	mVUregs.p							 = mVUregsTemp.p;
	mVUregs.r							 = mVUregsTemp.r;
	mVUregs.xgkick						 = mVUregsTemp.xgkick;
}

microVUx(void) mVUcompile(u32 startPC, u32 pipelineState, microRegInfo* pState, u8* x86ptrStart) {
	microVU* mVU = mVUx;
	microBlock block;
	iPC = startPC / 4;
	
	// Searches for Existing Compiled Block (if found, then returns; else, compile)
	microBlock* pblock = mVUblock[iPC]->search(pipelineState, pState);
	if (block) { x86SetPtr(pblock->x86ptrEnd); return; }

	// First Pass
	setCode();
	mVUbranch	= 0;
	mVUstartPC	= iPC;
	mVUcycles	= 1; // Skips "M" phase, and starts counting cycles at "T" stage
	for (int branch = 0;; ) {
		startLoop();
		mVUopU<vuIndex, 0>();
		if (curI & _Ebit_)	  { branch = 1; }
		if (curI & _MDTbit_)  { branch = 2; }
		if (curI & _Ibit_)	  { incPC(1); mVUinfo |= _isNOP; }
		else				  { incPC(1); mVUopL<vuIndex, 0>(); }
		mVUsetCycles<vuIndex>();
		if		(branch >= 2) { mVUinfo |= _isEOB | ((branch == 3) ? _isBdelay : 0); if (mVUbranch) { Console::Error("microVU Warning: Branch in E-bit/Branch delay slot!"); mVUinfo |= _isNOP; } break; }
		else if (branch == 1) { branch = 2; }
		if		(mVUbranch)	  { branch = 3; mVUbranch = 0; mVUinfo |= _isBranch; }
		incPC(1);
		incCycles(1);
	}

	// Second Pass
	iPC = mVUstartPC;
	setCode();
	for (bool x = 1; x; ) {
		//
		// ToDo: status/mac flag stuff?
		//
		if (isEOB)			{ x = 0; }
		else if (isBranch)	{ mVUopU<vuIndex, 1>(); incPC(2); }
		
		if (isNop)			{ mVUopU<vuIndex, 1>(); if (curI & _Ibit_) { incPC(1); mVU->iReg = curI; } else { incPC(1); } }
		else if (!swapOps)	{ mVUopU<vuIndex, 1>(); incPC(1); mVUopL<vuIndex, 1>(); }
		else				{ incPC(1); mVUopL<vuIndex, 1>(); incPC(-1); mVUopU<vuIndex, 1>(); incPC(1); }

		if (!isBdelay) { incPC(1); }
		else { 
			incPC(-2); // Go back to Branch Opcode
			mVUopL<vuIndex, 1>(); // Run Branch Opcode
			switch (mVUbranch) {
				case 1: break;
				case 2: break;
				case 3: break;
			}
			break;
		}
	}
}

#endif //PCSX2_MICROVU
