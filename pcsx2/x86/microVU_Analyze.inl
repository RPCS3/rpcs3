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

#define aReg(x) mVUregs.VF[x]
#define bReg(x) mVUregsTemp.VFreg[0] = x; mVUregsTemp.VF[0]
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

#define analyzeQreg(x) { mVUregsTemp.q = x; mVUstall = aMax(mVUstall, mVUregs.q); }
#define analyzePreg(x) { mVUregsTemp.p = x; mVUstall = aMax(mVUstall, ((mVUregs.p) ? (mVUregs.p - 1) : 0)); }

microVUt(void) mVUanalyzeFDIV(int Fs, int Fsf, int Ft, int Ftf, u8 xCycles) {
	microVU* mVU = mVUx;
	analyzeReg5(Fs, Fsf);
	analyzeReg5(Ft, Ftf);
	analyzeQreg(xCycles);
}

//------------------------------------------------------------------
// EFU - EFU Opcodes
//------------------------------------------------------------------

microVUt(void) mVUanalyzeEFU1(int Fs, int Fsf, u8 xCycles) {
	microVU* mVU = mVUx;
	analyzeReg5(Fs, Fsf);
	analyzePreg(xCycles);
}

microVUt(void) mVUanalyzeEFU2(int Fs, u8 xCycles) {
	microVU* mVU = mVUx;
	analyzeReg1(Fs);
	analyzePreg(xCycles);
}

//------------------------------------------------------------------
// R*** - R Reg Opcodes
//------------------------------------------------------------------

#define analyzeRreg() { mVUregsTemp.r = 1; }

microVUt(void) mVUanalyzeR1(int Fs, int Fsf) {
	microVU* mVU = mVUx;
	analyzeReg5(Fs, Fsf);
	analyzeRreg();
}

microVUt(void) mVUanalyzeR2(int Ft, bool canBeNOP) {
	microVU* mVU = mVUx;
	if (!Ft) { mVUinfo |= ((canBeNOP) ? _isNOP : _noWriteVF);  return; }
	analyzeReg2(Ft);
	analyzeRreg();
}

//------------------------------------------------------------------
// Sflag - Status Flag Opcodes
//------------------------------------------------------------------

#define analyzeVIreg1(reg)			{ if (reg) { mVUstall = aMax(mVUstall, mVUregs.VI[reg]); } }
#define analyzeVIreg2(reg, aCycles)	{ if (reg) { mVUregsTemp.VIreg = reg; mVUregsTemp.VI = aCycles; } }

microVUt(void) mVUanalyzeSflag(int It) {
	microVU* mVU = mVUx;
	if (!It) { mVUinfo |= _isNOP; return; }
	mVUinfo |= _isSflag;
	analyzeVIreg2(It, 1);
}

//------------------------------------------------------------------
// XGkick
//------------------------------------------------------------------

#define analyzeXGkick1()  { mVUstall = aMax(mVUstall, mVUregs.xgkick); }
#define analyzeXGkick2(x) { mVUregsTemp.xgkick = x; }

microVUt(void) mVUanalyzeXGkick(int Fs, int xCycles) {
	microVU* mVU = mVUx;
	analyzeVIreg1(Fs);
	analyzeXGkick1();
	analyzeXGkick2(xCycles);
}

#endif //PCSX2_MICROVU
