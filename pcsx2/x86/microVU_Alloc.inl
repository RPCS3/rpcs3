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

#define getFlagReg(regX, fInst) {														\
	switch (fInst) {																	\
		case 0: regX = gprF0;	break;													\
		case 1: regX = gprF1;	break;													\
		case 2: regX = gprF2;	break;													\
		case 3: regX = gprF3;	break;													\
		default:																		\
			Console::Error("microVU: Flag Instance Error (fInst = %d)", params fInst);	\
			regX = gprF0;																\
			break;																		\
	}																					\
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

microVUt(void) mVUallocMFLAGa(mV, int reg, int fInstance) {
	MOVZX32M16toR(reg, (uptr)&mVU->macFlag[fInstance]);
}

microVUt(void) mVUallocMFLAGb(mV, int reg, int fInstance) {
	//AND32ItoR(reg, 0xffff);
	MOV32RtoM((uptr)&mVU->macFlag[fInstance], reg);
}

microVUt(void) mVUallocCFLAGa(mV, int reg, int fInstance) {
	MOV32MtoR(reg, (uptr)&mVU->clipFlag[fInstance]);
}

microVUt(void) mVUallocCFLAGb(mV, int reg, int fInstance) {
	MOV32RtoM((uptr)&mVU->clipFlag[fInstance], reg);
}

//------------------------------------------------------------------
// VI Reg Allocators
//------------------------------------------------------------------

microVUt(void) mVUallocVIa(mV, int GPRreg, int _reg_) {
	if (!_reg_)	{ XOR32RtoR(GPRreg, GPRreg); }
	else		{ MOVZX32Rm16toR(GPRreg, gprR, (_reg_ - 9) * 16); }
}

microVUt(void) mVUallocVIb(mV, int GPRreg, int _reg_) {
	if (mVUlow.backupVI) { // Backs up reg to memory (used when VI is modified b4 a branch)
		MOVZX32M16toR(gprR, (uptr)&mVU->regs->VI[_reg_].UL);
		MOV32RtoM((uptr)&mVU->VIbackup, gprR);
		MOV32ItoR(gprR, Roffset);
	}
	if		(_reg_ == 0) { return; }
	else if (_reg_ < 16) { MOV16RtoRm(gprR, GPRreg, (_reg_ - 9) * 16); }
}

//------------------------------------------------------------------
// I/Q/P Reg Allocators
//------------------------------------------------------------------

#define getIreg(reg, modXYZW) {															\
	SSE_MOVSS_M32_to_XMM(reg, (uptr)&mVU->regs->VI[REG_I].UL);							\
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, -1, 8);									\
	if (!((_XYZW_SS && modXYZW) || (_X_Y_Z_W == 8))) { mVUunpack_xyzw(reg, reg, 0); }	\
}

#define getQreg(reg) {														\
	mVUunpack_xyzw(reg, xmmPQ, mVUinfo.readQ);								\
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 15);*/	\
}

#define getPreg(reg) {											\
	mVUunpack_xyzw(reg, xmmPQ, (2 + mVUinfo.readP));			\
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT1, 15);*/	\
}

//------------------------------------------------------------------
// Lower Instruction Allocator Helpers
//------------------------------------------------------------------

// VF to GPR
#define getReg8(GPRreg, _reg_, _fxf_) {														\
	if (!_reg_ && (_fxf_ < 3))	{ XOR32RtoR(GPRreg, GPRreg); }								\
	else						{ MOV32MtoR(GPRreg, (uptr)&mVU->regs->VF[_reg_].UL[0]); }	\
}
