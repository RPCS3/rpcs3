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

//------------------------------------------------------------------
// Micro VU - Pass 2 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Flag Allocators
//------------------------------------------------------------------

#define getFlagReg(regX, fInst) {										\
	switch (fInst) {													\
		case 0: regX = gprF0;	break;									\
		case 1: regX = gprF1;	break;									\
		case 2: regX = gprF2;	break;									\
		case 3: regX = gprF3;	break;									\
		default:														\
			Console::Error("microVU Error: fInst = %d", params fInst);	\
			regX = gprF0;												\
			break;														\
	}																	\
}

#define setBitSFLAG(bitTest, bitSet) {	\
	TEST32ItoR(regT, bitTest);			\
	pjmp = JZ8(0);						\
		OR32ItoR(reg, bitSet);			\
	x86SetJ8(pjmp);						\
}

#define setBitFSEQ(bitX) {	 \
	TEST32ItoR(gprT1, bitX); \
	pjmp = JZ8(0);			 \
	OR32ItoR(gprT1, bitX);	 \
	x86SetJ8(pjmp);			 \
}

microVUt(void) mVUallocSFLAGa(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOV32RtoR(reg, fInstance);
}

microVUt(void) mVUallocSFLAGb(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOV32RtoR(fInstance, reg);
}

// Normalize Status Flag
microVUt(void) mVUallocSFLAGc(int reg, int regT, int fInstance) {
	u8 *pjmp;
	XOR32RtoR(reg, reg);
	mVUallocSFLAGa(regT, fInstance);
	setBitSFLAG(0x0f00, 0x0001); // Z  Bit
	setBitSFLAG(0xf000, 0x0002); // S  Bit
	setBitSFLAG(0x000f, 0x0040); // ZS Bit
	setBitSFLAG(0x00f0, 0x0080); // SS Bit
	AND32ItoR(regT, 0xffff0000); // DS/DI/OS/US/D/I/O/U Bits
	SHR32ItoR(regT, 14);
	OR32RtoR(reg, regT);
}

// Denormalizes Status Flag
microVUt(void) mVUallocSFLAGd(uptr memAddr, bool setAllflags) {
	MOV32MtoR(gprF0, memAddr);
	MOV32RtoR(gprF1, gprF0);
	SHR32ItoR(gprF1, 3);
	AND32ItoR(gprF1, 0x18);

	MOV32RtoR(gprF2, gprF0);
	SHL32ItoR(gprF2, 11);
	AND32ItoR(gprF2, 0x1800);
	OR32RtoR (gprF1, gprF2);

	SHL32ItoR(gprF0, 14);
	AND32ItoR(gprF0, 0x3cf0000);
	OR32RtoR (gprF1, gprF0);

	if (setAllflags) {
		MOV32RtoR(gprF0, gprF1);
		MOV32RtoR(gprF2, gprF1);
		MOV32RtoR(gprF3, gprF1);
	}
}

microVUt(void) mVUallocMFLAGa(mV, int reg, int fInstance) {
	MOVZX32M16toR(reg, (uptr)&mVU->macFlag[fInstance]);
}

microVUt(void) mVUallocMFLAGb(mV, int reg, int fInstance) {
	//AND32ItoR(reg, 0xffff);
	if (fInstance < 4) MOV32RtoM((uptr)&mVU->macFlag[fInstance], reg);			// microVU
	else			   MOV32RtoM((uptr)&mVU->regs->VI[REG_MAC_FLAG].UL, reg);	// macroVU
}

microVUt(void) mVUallocCFLAGa(mV, int reg, int fInstance) {
	if (fInstance < 4) MOV32MtoR(reg, (uptr)&mVU->clipFlag[fInstance]);			// microVU
	else			   MOV32MtoR(reg, (uptr)&mVU->regs->VI[REG_CLIP_FLAG].UL);	// macroVU
}

microVUt(void) mVUallocCFLAGb(mV, int reg, int fInstance) {
	if (fInstance < 4) MOV32RtoM((uptr)&mVU->clipFlag[fInstance], reg);			// microVU
	else			   MOV32RtoM((uptr)&mVU->regs->VI[REG_CLIP_FLAG].UL, reg);	// macroVU
}

//------------------------------------------------------------------
// VI Reg Allocators
//------------------------------------------------------------------

microVUt(void) mVUallocVIa(mV, int GPRreg, int _reg_) {
	if (!_reg_)	{ XOR32RtoR(GPRreg, GPRreg); }
	else		{ MOVZX32M16toR(GPRreg, (uptr)&mVU->regs->VI[_reg_].UL); }
}

microVUt(void) mVUallocVIb(mV, int GPRreg, int _reg_) {
	if (mVUlow.backupVI) { // Backs up reg to memory (used when VI is modified b4 a branch)
		MOVZX32M16toR(gprT3, (uptr)&mVU->regs->VI[_reg_].UL);
		MOV32RtoM((uptr)&mVU->VIbackup, gprT3);
	}
	if		(_reg_ == 0) { return; }
	else if (_reg_ < 16) { MOV16RtoM((uptr)&mVU->regs->VI[_reg_].UL, GPRreg); }
}

//------------------------------------------------------------------
// I/P/Q Reg Allocators
//------------------------------------------------------------------

microVUt(void) getPreg(mV, int reg) {
	mVUunpack_xyzw(reg, xmmPQ, (2 + mVUinfo.readP));
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT1, 15);*/
}

microVUt(void) getQreg(int reg, int qInstance) {
	mVUunpack_xyzw(reg, xmmPQ, qInstance);
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 15);*/
}

microVUt(void) writeQreg(int reg, int qInstance) {
	if (qInstance) {
		if (!cpucaps.hasStreamingSIMD4Extensions) {
			SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, 0xe1);
			SSE_MOVSS_XMM_to_XMM(xmmPQ, reg);
			SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, 0xe1);
		}
		else SSE4_INSERTPS_XMM_to_XMM(xmmPQ, reg, _MM_MK_INSERTPS_NDX(0, 1, 0));
	}
	else SSE_MOVSS_XMM_to_XMM(xmmPQ, reg);
}
