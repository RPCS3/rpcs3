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

//------------------------------------------------------------------
// Micro VU - recPass 0 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// FMAC1 - Normal FMAC Opcodes
//------------------------------------------------------------------

#define aReg(x) mVUallocInfo.regs.VF[x]
#define bReg(x) mVUallocInfo.regsTemp.VFreg[0] = x; mVUallocInfo.regsTemp.VF[0]
#define aMax(x, y) ((x > y) ? x : y)

#define analyzeReg1(reg) {									\
	if (reg) {												\
		if (_X) { mVUstall = aMax(mVUstall, aReg(reg).x); }	\
		if (_Y) { mVUstall = aMax(mVUstall, aReg(reg).y); }	\
		if (_Z) { mVUstall = aMax(mVUstall, aReg(reg).z); }	\
		if (_W) { mVUstall = aMax(mVUstall, aReg(reg).w); } \
	}														\
}

#define analyzeReg2(reg) {				\
	if (reg) {							\
		if (_X) { bReg(reg).x = 4; }	\
		if (_Y) { bReg(reg).y = 4; }	\
		if (_Z) { bReg(reg).z = 4; }	\
		if (_W) { bReg(reg).w = 4; }	\
	}									\
}

microVUt(void) mVUanalyzeFMAC1(int Fd, int Fs, int Ft) {
	microVU* mVU = mVUx;
	mVUinfo |= _doStatus;
	analyzeReg1(Fs);
	analyzeReg1(Ft);
	analyzeReg2(Fd);
}

//------------------------------------------------------------------
// FMAC2 - ABS/FTOI/ITOF Opcodes
//------------------------------------------------------------------

microVUt(void) mVUanalyzeFMAC2(int Fs, int Ft) {
	microVU* mVU = mVUx;
	analyzeReg1(Fs);
	analyzeReg2(Ft);
}

//------------------------------------------------------------------
// FMAC3 - BC(xyzw) FMAC Opcodes
//------------------------------------------------------------------

#define analyzeReg3(reg) {											\
	if (reg) {														\
		if (_bc_x)		{ mVUstall = aMax(mVUstall, aReg(reg).x); } \
		else if (_bc_y) { mVUstall = aMax(mVUstall, aReg(reg).y); }	\
		else if (_bc_z) { mVUstall = aMax(mVUstall, aReg(reg).z); }	\
		else			{ mVUstall = aMax(mVUstall, aReg(reg).w); } \
	}																\
}

microVUt(void) mVUanalyzeFMAC3(int Fd, int Fs, int Ft) {
	microVU* mVU = mVUx;
	mVUinfo |= _doStatus;
	analyzeReg1(Fs);
	analyzeReg3(Ft);
	analyzeReg2(Fd);
}

//------------------------------------------------------------------
// FMAC4 - Clip FMAC Opcode
//------------------------------------------------------------------

#define analyzeReg4(reg) {								 \
	if (reg) { mVUstall = aMax(mVUstall, aReg(reg).w); } \
}

microVUt(void) mVUanalyzeFMAC4(int Fs, int Ft) {
	microVU* mVU = mVUx;
	analyzeReg1(Fs);
	analyzeReg4(Ft);
}

//------------------------------------------------------------------
// FDIV - DIV/SQRT/RSQRT Opcodes
//------------------------------------------------------------------

#define analyzeReg5(reg, fxf) {										\
	if (reg) {														\
		switch (fxf) {												\
			case 0: mVUstall = aMax(mVUstall, aReg(reg).x); break;	\
			case 1: mVUstall = aMax(mVUstall, aReg(reg).y); break;	\
			case 2: mVUstall = aMax(mVUstall, aReg(reg).z); break;	\
			case 3: mVUstall = aMax(mVUstall, aReg(reg).w); break;	\
		}															\
	}																\
}

microVUt(void) mVUanalyzeFDIV(int Fs, int Fsf, int Ft, int Ftf) {
	microVU* mVU = mVUx;
	analyzeReg5(Fs, Fsf);
	analyzeReg5(Ft, Ftf);
}

#endif //PCSX2_MICROVU
