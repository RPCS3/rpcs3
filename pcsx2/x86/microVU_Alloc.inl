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

__fi static const x32& getFlagReg(uint fInst)
{
	static const x32* const gprFlags[4] = { &gprF0, &gprF1, &gprF2, &gprF3 };
	pxAssert(fInst < 4);
	return *gprFlags[fInst];
}

__fi void setBitSFLAG(const x32& reg, const x32& regT, int bitTest, int bitSet)
{
	xTEST(regT, bitTest);
	xForwardJZ8 skip;
	xOR(reg, bitSet);
	skip.SetTarget();
}

__fi void setBitFSEQ(const x32& reg, int bitX)
{
	xTEST(reg, bitX);
	xForwardJump8 skip(Jcc_Zero);
	xOR(reg, bitX);
	skip.SetTarget();
}

__fi void mVUallocSFLAGa(const x32& reg, int fInstance)
{
	xMOV(reg, getFlagReg(fInstance));
}

__fi void mVUallocSFLAGb(const x32& reg, int fInstance)
{
	xMOV(getFlagReg(fInstance), reg);
}

// Normalize Status Flag
__ri void mVUallocSFLAGc(const x32& reg, const x32& regT, int fInstance)
{
	xXOR(reg, reg);
	mVUallocSFLAGa(regT, fInstance);
	setBitSFLAG(reg, regT, 0x0f00, 0x0001); // Z  Bit
	setBitSFLAG(reg, regT, 0xf000, 0x0002); // S  Bit
	setBitSFLAG(reg, regT, 0x000f, 0x0040); // ZS Bit
	setBitSFLAG(reg, regT, 0x00f0, 0x0080); // SS Bit
	xAND(regT, 0xffff0000); // DS/DI/OS/US/D/I/O/U Bits
	xSHR(regT, 14);
	xOR (reg, regT);
}

// Denormalizes Status Flag
// If setAllflags is false, then returns result in eax (gprT1)
// else all microVU flag regs (gprF0..F3) get the result.
__ri void mVUallocSFLAGd(u32* memAddr, bool setAllflags) {

	// When this function is used by mVU0 macro the EErec
	// needs EBP (gprF1) and ESI (gprF2) to be preserved.
	const xAddressReg& t0 = setAllflags ? gprT1 : gprF3;
	const xAddressReg& t1 = setAllflags ? gprT2 : gprF2;
	const xAddressReg& t2 = setAllflags ? gprT3 : gprF0;

	xMOV(t2, ptr32[memAddr]);
	xMOV(t0, t2);
	xSHR(t0, 3);
	xAND(t0, 0x18);

	xMOV(t1, t2);
	xSHL(t1, 11);
	xAND(t1, 0x1800);
	xOR (t0, t1);

	xSHL(t2, 14);
	xAND(t2, 0x3cf0000);
	xOR (t0, t2);

	if (setAllflags) {
		// this code should be run in mVU micro mode only, so writing to
		// EBP (gprF1) and ESI (gprF2) is ok (and needed for vuMicro).
		xMOV(gprF0, gprF3); // gprF3 is t0
		xMOV(gprF1, gprF3);
		xMOV(gprF2, gprF3);
	}
}

__fi void mVUallocMFLAGa(mV, const x32& reg, int fInstance)
{
	xMOVZX(reg, ptr16[&mVU.macFlag[fInstance]]);
}

__fi void mVUallocMFLAGb(mV, const x32& reg, int fInstance)
{
	//xAND(reg, 0xffff);
	if (fInstance < 4) xMOV(ptr32[&mVU.macFlag[fInstance]], reg);			// microVU
	else			   xMOV(ptr32[&mVU.regs().VI[REG_MAC_FLAG].UL], reg);	// macroVU
}

__fi void mVUallocCFLAGa(mV, const x32& reg, int fInstance)
{
	if (fInstance < 4) xMOV(reg, ptr32[&mVU.clipFlag[fInstance]]);			// microVU
	else			   xMOV(reg, ptr32[&mVU.regs().VI[REG_CLIP_FLAG].UL]);	// macroVU
}

__fi void mVUallocCFLAGb(mV, const x32& reg, int fInstance)
{
	if (fInstance < 4) xMOV(ptr32[&mVU.clipFlag[fInstance]], reg);			// microVU
	else			   xMOV(ptr32[&mVU.regs().VI[REG_CLIP_FLAG].UL], reg);	// macroVU
}

//------------------------------------------------------------------
// VI Reg Allocators
//------------------------------------------------------------------

__ri void mVUallocVIa(mV, const x32& GPRreg, int _reg_, bool signext = false)
{
	if  (!_reg_)  xXOR  (GPRreg, GPRreg);
	elif(signext) xMOVSX(GPRreg, ptr16[&mVU.regs().VI[_reg_].SL]);
	else          xMOVZX(GPRreg, ptr16[&mVU.regs().VI[_reg_].UL]);
}

__ri void mVUallocVIb(mV, const x32& GPRreg, int _reg_)
{
	if (mVUlow.backupVI) { // Backs up reg to memory (used when VI is modified b4 a branch)
		xMOVZX(gprT3, ptr16[&mVU.regs().VI[_reg_].UL]);
		xMOV  (ptr32[&mVU.VIbackup], gprT3);
	}
	if   (_reg_ == 0) { return; }
	elif (_reg_ < 16) { xMOV(ptr16[&mVU.regs().VI[_reg_].UL], xRegister16(GPRreg.Id)); }
}

//------------------------------------------------------------------
// P/Q Reg Allocators
//------------------------------------------------------------------

__fi void getPreg(mV, const xmm& reg)
{
	mVUunpack_xyzw(reg, xmmPQ, (2 + mVUinfo.readP));
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT1, 15);*/
}

__fi void getQreg(const xmm& reg, int qInstance)
{
	mVUunpack_xyzw(reg, xmmPQ, qInstance);
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 15);*/
}

__ri void writeQreg(const xmm& reg, int qInstance)
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
