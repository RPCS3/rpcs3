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

//------------------------------------------------------------------
// Micro VU - Pass 2 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Flag Allocators
//------------------------------------------------------------------

_f static x32 getFlagReg(int fInst)
{
	if (fInst >= 0 && fInst < 4)
	{
		return gprF[fInst];
	}
	else
	{
		Console.Error("microVU Error: fInst = %d", fInst);
		return gprF[0];
	}
}

_f void setBitSFLAG(x32 reg, x32 regT, int bitTest, int bitSet)
{
	xTEST(regT, bitTest);
	xForwardJZ8 skip;
	xOR(reg, bitSet);
	skip.SetTarget();
}

_f void setBitFSEQ(x32 reg, int bitX)
{
	xTEST(reg, bitX);
	xForwardJump8 skip(Jcc_Zero);
	xOR(reg, bitX);
	skip.SetTarget();
}

_f void mVUallocSFLAGa(x32 reg, int fInstance)
{
	xMOV(reg, getFlagReg(fInstance));
}

_f void mVUallocSFLAGb(x32 reg, int fInstance)
{
	xMOV(getFlagReg(fInstance), reg);
}

// Normalize Status Flag
_f void mVUallocSFLAGc(x32 reg, x32 regT, int fInstance)
{
	xXOR(reg, reg);
	mVUallocSFLAGa(regT, fInstance);
	setBitSFLAG(reg, regT, 0x0f00, 0x0001); // Z  Bit
	setBitSFLAG(reg, regT, 0xf000, 0x0002); // S  Bit
	setBitSFLAG(reg, regT, 0x000f, 0x0040); // ZS Bit
	setBitSFLAG(reg, regT, 0x00f0, 0x0080); // SS Bit
	xAND(regT, 0xffff0000); // DS/DI/OS/US/D/I/O/U Bits
	xSHR(regT, 14);
	xOR(reg, regT);
}

// Denormalizes Status Flag
_f void mVUallocSFLAGd(u32* memAddr, bool setAllflags) {

	// Cannot use EBP (gprF[1]) here; as this function is used by mVU0 macro and
	// the EErec needs EBP preserved.

	xMOV(gprF[0], ptr32[memAddr]);
	xMOV(gprF[3], gprF[0]);
	xSHR(gprF[3], 3);
	xAND(gprF[3], 0x18);

	xMOV(gprF[2], gprF[0]);
	xSHL(gprF[2], 11);
	xAND(gprF[2], 0x1800);
	xOR (gprF[3], gprF[2]);

	xSHL(gprF[0], 14);
	xAND(gprF[0], 0x3cf0000);
	xOR (gprF[3], gprF[0]);

	if (setAllflags) {

		// this code should be run in mVU micro mode only, so writing to
		// EBP (gprF[1]) is ok (and needed for vuMicro optimizations).

		xMOV(gprF[0], gprF[3]);
		xMOV(gprF[1], gprF[3]);
		xMOV(gprF[2], gprF[3]);
	}
}

_f void mVUallocMFLAGa(mV, x32 reg, int fInstance)
{
	xMOVZX(reg, ptr16[&mVU->macFlag[fInstance]]);
}

_f void mVUallocMFLAGb(mV, x32 reg, int fInstance)
{
	//xAND(reg, 0xffff);
	if (fInstance < 4) xMOV(ptr32[&mVU->macFlag[fInstance]], reg);			// microVU
	else			   xMOV(ptr32[&mVU->regs->VI[REG_MAC_FLAG].UL], reg);	// macroVU
}

_f void mVUallocCFLAGa(mV, x32 reg, int fInstance)
{
	if (fInstance < 4) xMOV(reg, ptr32[&mVU->clipFlag[fInstance]]);			// microVU
	else			   xMOV(reg, ptr32[&mVU->regs->VI[REG_CLIP_FLAG].UL]);	// macroVU
}

_f void mVUallocCFLAGb(mV, x32 reg, int fInstance)
{
	if (fInstance < 4) xMOV(ptr32[&mVU->clipFlag[fInstance]], reg);			// microVU
	else			   xMOV(ptr32[&mVU->regs->VI[REG_CLIP_FLAG].UL], reg);	// macroVU
}

//------------------------------------------------------------------
// VI Reg Allocators
//------------------------------------------------------------------

_f void mVUallocVIa(mV, x32 GPRreg, int _reg_, bool signext = false)
{
	if (!_reg_)
		xXOR(GPRreg, GPRreg);
	else
		if (signext)
			xMOVSX(GPRreg, ptr16[&mVU->regs->VI[_reg_].SL]);
		else
			xMOVZX(GPRreg, ptr16[&mVU->regs->VI[_reg_].UL]);
}

_f void mVUallocVIb(mV, x32 GPRreg, int _reg_)
{
	if (mVUlow.backupVI) { // Backs up reg to memory (used when VI is modified b4 a branch)
		xMOVZX(edx, ptr16[&mVU->regs->VI[_reg_].UL]);
		xMOV(ptr32[&mVU->VIbackup], edx);
	}
	if		(_reg_ == 0) { return; }
	else if (_reg_ < 16) { xMOV(ptr16[&mVU->regs->VI[_reg_].UL], xRegister16(GPRreg.Id)); }
}

//------------------------------------------------------------------
// P/Q Reg Allocators
//------------------------------------------------------------------

_f void getPreg(mV, xmm reg)
{
	mVUunpack_xyzw(reg, xmmPQ, (2 + mVUinfo.readP));
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT1, 15);*/
}

_f void getQreg(xmm reg, int qInstance)
{
	mVUunpack_xyzw(reg, xmmPQ, qInstance);
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 15);*/
}

_f void writeQreg(xmm reg, int qInstance)
{
	if (qInstance) {
		if (!x86caps.hasStreamingSIMD4Extensions) {
			xPSHUF.D(xmmPQ, xmmPQ, 0xe1);
			xMOVSS(xmmPQ, reg);
			xPSHUF.D(xmmPQ, xmmPQ, 0xe1);
		}
		else xINSERTPS(xmmPQ, reg, _MM_MK_INSERTPS_NDX(0, 1, 0));
	}
	else xMOVSS(xmmPQ, reg);
}
