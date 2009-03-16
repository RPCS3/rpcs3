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

#define mVUbranch	mVUallocInfo.branch
#define iPC			mVUcurProg.curPC
#define curI		mVUcurProg.data[iPC]
#define setCode() { mVU->code = curI; }
#define incPC()	  { iPC = ((iPC + 1) & (mVU->progSize-1)); setCode();}

microVUx(void) mVUcompile(u32 startPC, u32 pipelineState, u8* x86ptrStart) {
	microVU* mVU = mVUx;
	int x;
	iPC = startPC;
	setCode();
	for (x = 0; ; x++) {
		if (curI & _Ibit_) { SysPrintf("microVU: I-bit set!\n"); }
		if (curI & _Ebit_) { SysPrintf("microVU: E-bit set!\n"); }
		if (curI & _Mbit_) { SysPrintf("microVU: M-bit set!\n"); }
		if (curI & _Dbit_) { SysPrintf("microVU: D-bit set!\n"); mVUbranch = 4; }
		if (curI & _Tbit_) { SysPrintf("microVU: T-bit set!\n"); mVUbranch = 4; }
		mVUopU<vuIndex, 0>();
		incPC();
		mVUopL<vuIndex, 0>();
		if (mVUbranch == 4) { mVUbranch = 0; break; }
		else if (mVUbranch) { mVUbranch = 4; }
	}
	iPC = startPC;
	setCode();
	for (int i = 0; i < x; i++) {
		mVUopU<vuIndex, 1>();
		incPC();
		if (!isNop) mVUopL<vuIndex, 1>();
	}
}

#endif //PCSX2_MICROVU
