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
// Micro VU - Reg Loading/Saving/Shuffling/Unpacking/Merging...
//------------------------------------------------------------------

void mVUunpack_xyzw(const xmm& dstreg, const xmm& srcreg, int xyzw)
{
	switch ( xyzw ) {
		case 0: xPSHUF.D(dstreg, srcreg, 0x00); break; // XXXX
		case 1: xPSHUF.D(dstreg, srcreg, 0x55); break; // YYYY
		case 2: xPSHUF.D(dstreg, srcreg, 0xaa); break; // ZZZZ
		case 3: xPSHUF.D(dstreg, srcreg, 0xff); break; // WWWW
	}
}

void mVUloadReg(const xmm& reg, xAddressVoid ptr, int xyzw)
{
	switch( xyzw ) {
		case 8:		xMOVSSZX(reg, ptr32[ptr]);		break; // X
		case 4:		xMOVSSZX(reg, ptr32[ptr+4]);	break; // Y
		case 2:		xMOVSSZX(reg, ptr32[ptr+8]);	break; // Z
		case 1:		xMOVSSZX(reg, ptr32[ptr+12]);	break; // W
		default:	xMOVAPS (reg, ptr128[ptr]);		break;
	}
}

void mVUloadIreg(const xmm& reg, int xyzw, VURegs* vuRegs)
{
	xMOVSSZX(reg, ptr32[&vuRegs->VI[REG_I].UL]);
	if (!_XYZWss(xyzw)) xSHUF.PS(reg, reg, 0);
}

// Modifies the Source Reg!
void mVUsaveReg(const xmm& reg, xAddressVoid ptr, int xyzw, bool modXYZW)
{
	/*xMOVAPS(xmmT2, ptr128[ptr]);
	if (modXYZW && (xyzw == 8 || xyzw == 4 || xyzw == 2 || xyzw == 1)) {
		mVUunpack_xyzw(reg, reg, 0);
	}
	mVUmergeRegs(xmmT2, reg, xyzw);

	xMOVAPS(ptr128[ptr], xmmT2);
	return;*/

	switch ( xyzw ) {
		case 5:		if (x86caps.hasStreamingSIMD4Extensions) {
						xEXTRACTPS(ptr32[ptr+4], reg, 1);
						xEXTRACTPS(ptr32[ptr+12], reg, 3);
					}
					else {
						xPSHUF.D(reg, reg, 0xe1); //WZXY
						xMOVSS(ptr32[ptr+4], reg);
						xPSHUF.D(reg, reg, 0xff); //WWWW
						xMOVSS(ptr32[ptr+12], reg);
					}
					break; // YW
		case 6:		xPSHUF.D(reg, reg, 0xc9);
					xMOVL.PS(ptr64[ptr+4], reg);
					break; // YZ
		case 7:		if (x86caps.hasStreamingSIMD4Extensions) {
						xMOVH.PS(ptr64[ptr+8], reg);
						xEXTRACTPS(ptr32[ptr+4],  reg, 1);
					}
					else {
						xPSHUF.D(reg, reg, 0x93); //ZYXW
						xMOVH.PS(ptr64[ptr+4], reg);
						xMOVSS(ptr32[ptr+12], reg);
					}
					break; // YZW
		case 9:		if (x86caps.hasStreamingSIMD4Extensions) {
						xMOVSS(ptr32[ptr], reg);
						xEXTRACTPS(ptr32[ptr+12], reg, 3);
					}
					else {
						xMOVSS(ptr32[ptr], reg);
						xPSHUF.D(reg, reg, 0xff); //WWWW
						xMOVSS(ptr32[ptr+12], reg);
					}
					break; // XW
		case 10:	if (x86caps.hasStreamingSIMD4Extensions) {
						xMOVSS(ptr32[ptr], reg);
						xEXTRACTPS(ptr32[ptr+8], reg, 2);
					}
					else {
						xMOVSS(ptr32[ptr], reg);
						xMOVHL.PS(reg, reg);
						xMOVSS(ptr32[ptr+8], reg);
					}
					break; //XZ
		case 11:	xMOVSS(ptr32[ptr], reg);
					xMOVH.PS(ptr64[ptr+8], reg);
					break; //XZW
		case 13:	if (x86caps.hasStreamingSIMD4Extensions) {
						xMOVL.PS(ptr64[ptr], reg);
						xEXTRACTPS(ptr32[ptr+12], reg, 3);
					}
					else {
						xPSHUF.D(reg, reg, 0x4b); //YXZW				
						xMOVH.PS(ptr64[ptr], reg);
						xMOVSS(ptr32[ptr+12], reg);
					}
					break; // XYW
		case 14:	if (x86caps.hasStreamingSIMD4Extensions) {
						xMOVL.PS(ptr64[ptr], reg);
						xEXTRACTPS(ptr32[ptr+8], reg, 2);
					}
					else {
						xMOVL.PS(ptr64[ptr], reg);
						xMOVHL.PS(reg, reg);
						xMOVSS(ptr32[ptr+8], reg);
					}
					break; // XYZ
		case 4:		if (!modXYZW) mVUunpack_xyzw(reg, reg, 1);
					xMOVSS(ptr32[ptr+4], reg);		
					break; // Y
		case 2:		if (!modXYZW) mVUunpack_xyzw(reg, reg, 2);
					xMOVSS(ptr32[ptr+8], reg);	
					break; // Z
		case 1:		if (!modXYZW) mVUunpack_xyzw(reg, reg, 3);
					xMOVSS(ptr32[ptr+12], reg);	
					break; // W
		case 8:		xMOVSS(ptr32[ptr], reg);	 break; // X
		case 12:	xMOVL.PS(ptr64[ptr], reg);	 break; // XY
		case 3:		xMOVH.PS(ptr64[ptr+8], reg); break; // ZW
		default:	xMOVAPS(ptr128[ptr], reg);	 break; // XYZW
	}
}

// Modifies the Source Reg! (ToDo: Optimize modXYZW = 1 cases)
void mVUmergeRegs(const xmm& dest, const xmm& src, int xyzw, bool modXYZW)
{
	xyzw &= 0xf;
	if ( (dest != src) && (xyzw != 0) ) {
		if (x86caps.hasStreamingSIMD4Extensions && (xyzw != 0x8) && (xyzw != 0xf)) {
			if (modXYZW) {
				if		(xyzw == 1) { xINSERTPS(dest, src, _MM_MK_INSERTPS_NDX(0, 3, 0)); return; }
				else if (xyzw == 2) { xINSERTPS(dest, src, _MM_MK_INSERTPS_NDX(0, 2, 0)); return; }
				else if (xyzw == 4) { xINSERTPS(dest, src, _MM_MK_INSERTPS_NDX(0, 1, 0)); return; }
			}
			xyzw = ((xyzw & 1) << 3) | ((xyzw & 2) << 1) | ((xyzw & 4) >> 1) | ((xyzw & 8) >> 3);
			xBLEND.PS(dest, src, xyzw);
		}
		else {
			switch (xyzw) {
				case 1:  if (modXYZW) mVUunpack_xyzw(src, src, 0);
						 xMOVHL.PS(src, dest);		 // src = Sw Sz Dw Dz
						 xSHUF.PS(dest, src, 0xc4); // 11 00 01 00
						 break;
				case 2:  if (modXYZW) mVUunpack_xyzw(src, src, 0);
						 xMOVHL.PS(src, dest);
						 xSHUF.PS(dest, src, 0x64);
						 break;
				case 3:	 xSHUF.PS(dest, src, 0xe4);
						 break;
				case 4:	 if (modXYZW) mVUunpack_xyzw(src, src, 0);
						 xMOVSS(src, dest);
						 xMOVSD(dest, src);
						 break;
				case 5:	 xSHUF.PS(dest, src, 0xd8);
						 xPSHUF.D(dest, dest, 0xd8);
						 break;
				case 6:	 xSHUF.PS(dest, src, 0x9c);
						 xPSHUF.D(dest, dest, 0x78);
						 break;
				case 7:	 xMOVSS(src, dest);
						 xMOVAPS(dest, src);
						 break;
				case 8:	 xMOVSS(dest, src);
						 break;
				case 9:	 xSHUF.PS(dest, src, 0xc9);
						 xPSHUF.D(dest, dest, 0xd2);
						 break;
				case 10: xSHUF.PS(dest, src, 0x8d);
						 xPSHUF.D(dest, dest, 0x72);
						 break;
				case 11: xMOVSS(dest, src);
						 xSHUF.PS(dest, src, 0xe4);
						 break;
				case 12: xMOVSD(dest, src);
						 break;
				case 13: xMOVHL.PS(dest, src);
						 xSHUF.PS(src, dest, 0x64);
						 xMOVAPS(dest, src);
						 break;
				case 14: xMOVHL.PS(dest, src);
						 xSHUF.PS(src, dest, 0xc4);
						 xMOVAPS(dest, src);
						 break;
				default: xMOVAPS(dest, src);
						 break;
			}
		}
	}
}

//------------------------------------------------------------------
// Micro VU - Misc Functions
//------------------------------------------------------------------

// Transforms the Address in gprReg to valid VU0/VU1 Address
_f void mVUaddrFix(mV, const x32& gprReg)
{
	if (isVU1) {
		xAND(gprReg, 0x3ff); // wrap around
		xSHL(gprReg, 4);
	}
	else {
		xCMP(gprReg, 0x400);
		xForwardJL8 jmpA; // if addr >= 0x4000, reads VU1's VF regs and VI regs
			xAND(gprReg, 0x43f); // ToDo: theres a potential problem if VU0 overrides VU1's VF0/VI0 regs!
			xForwardJump8 jmpB;
		jmpA.SetTarget();
			xAND(gprReg, 0xff); // if addr < 0x4000, wrap around
		jmpB.SetTarget();
		xSHL(gprReg, 4); // multiply by 16 (shift left by 4)
	}
}

// Backup Volatile Regs (EAX, ECX, EDX, MM0~7, XMM0~7, are all volatile according to 32bit Win/Linux ABI)
_f void mVUbackupRegs(microVU* mVU)
{
	mVU->regAlloc->flushAll();
	xMOVAPS(ptr128[mVU->xmmPQb], xmmPQ);
}

// Restore Volatile Regs
_f void mVUrestoreRegs(microVU* mVU)
{
	xMOVAPS(xmmPQ, ptr128[mVU->xmmPQb]);
}

//------------------------------------------------------------------
// Micro VU - Custom SSE Instructions
//------------------------------------------------------------------

struct SSEMaskPair { u32 mask1[4], mask2[4]; };

static const __aligned16 SSEMaskPair MIN_MAX =
{
	{0xffffffff, 0x80000000, 0xffffffff, 0x80000000},
	{0x00000000, 0x40000000, 0x00000000, 0x40000000}
};


// Warning: Modifies t1 and t2
void MIN_MAX_PS(microVU* mVU, const xmm& to, const xmm& from, const xmm& t1in, const xmm& t2in, bool min)
{
	const xmm& t1 = t1in.IsEmpty() ? mVU->regAlloc->allocReg() : t1in;
	const xmm& t2 = t2in.IsEmpty() ? mVU->regAlloc->allocReg() : t2in;
	// ZW
	xPSHUF.D(t1, to, 0xfa);
	xPAND   (t1, ptr128[MIN_MAX.mask1]);
	xPOR    (t1, ptr128[MIN_MAX.mask2]);
	xPSHUF.D(t2, from, 0xfa);
	xPAND   (t2, ptr128[MIN_MAX.mask1]);
	xPOR    (t2, ptr128[MIN_MAX.mask2]);
	if (min) xMIN.PD(t1, t2);
	else     xMAX.PD(t1, t2);

	// XY
	xPSHUF.D(t2, from, 0x50);
	xPAND   (t2, ptr128[MIN_MAX.mask1]);
	xPOR    (t2, ptr128[MIN_MAX.mask2]);
	xPSHUF.D(to, to, 0x50);
	xPAND   (to, ptr128[MIN_MAX.mask1]);
	xPOR    (to, ptr128[MIN_MAX.mask2]);
	if (min) xMIN.PD(to, t2);
	else     xMAX.PD(to, t2);

	xSHUF.PS(to, t1, 0x88);
	if (t1 != t1in) mVU->regAlloc->clearNeeded(t1);
	if (t2 != t2in) mVU->regAlloc->clearNeeded(t2);
}

// Warning: Modifies to's upper 3 vectors, and t1
void MIN_MAX_SS(mV, const xmm& to, const xmm& from, const xmm& t1in, bool min)
{
	const xmm& t1 = t1in.IsEmpty() ? mVU->regAlloc->allocReg() : t1in;
	xSHUF.PS(to, from, 0);
	xPAND	(to, ptr128[MIN_MAX.mask1]);
	xPOR	(to, ptr128[MIN_MAX.mask2]);
	xPSHUF.D(t1, to, 0xee);
	if (min) xMIN.PD(to, t1);
	else	 xMAX.PD(to, t1);
	if (t1 != t1in) mVU->regAlloc->clearNeeded(t1);
}

// Warning: Modifies all vectors in 'to' and 'from', and Modifies xmmT1 and xmmT2
void ADD_SS(microVU* mVU, const xmm& to, const xmm& from, const xmm& t1in, const xmm& t2in)
{
	const xmm& t1 = t1in.IsEmpty() ? mVU->regAlloc->allocReg() : t1in;
	const xmm& t2 = t2in.IsEmpty() ? mVU->regAlloc->allocReg() : t2in;

	xMOVAPS(t1, to);
	xMOVAPS(t2, from);
	xMOVD(ecx, to);
	xSHR(ecx, 23);
	xMOVD(eax, from);
	xSHR(eax, 23);
	xAND(ecx, 0xff);
	xAND(eax, 0xff);
	xSUB(ecx, eax); //ecx = exponent difference

	xCMP(ecx, 25);
	xForwardJGE8 case2;
	xCMP(ecx, 0);
	xForwardJG8 case3;
	xForwardJE8 toend1;
	xCMP(ecx, -25);
	xForwardJLE8 case4;

	// negative small
		xNOT(ecx); // -ecx - 1
		xMOV(eax, 0xffffffff);
		xSHL(eax, cl);
		xPCMP.EQB(to, to);
		xMOVDZX(from, eax);
		xMOVSS(to, from);
		xPCMP.EQB(from, from);
	xForwardJump8 toend2;

	case2.SetTarget(); // positive large
		xMOV(eax, 0x80000000);
		xPCMP.EQB(from, from);
		xMOVDZX(to, eax);
		xMOVSS(from, to);
		xPCMP.EQB(to, to);
	xForwardJump8 toend3;

	case3.SetTarget(); // positive small
		xDEC(ecx);
		xMOV(eax, 0xffffffff);
		xSHL(eax, cl);
		xPCMP.EQB(from, from);
		xMOVDZX(to, eax);
		xMOVSS(from, to);
		xPCMP.EQB(to, to);
	xForwardJump8 toend4;

	case4.SetTarget(); // negative large
		xMOV(eax, 0x80000000);
		xPCMP.EQB(to, to);
		xMOVDZX(from, eax);
		xMOVSS(to, from);
		xPCMP.EQB(from, from);

	toend1.SetTarget();
	toend2.SetTarget();
	toend3.SetTarget();
	toend4.SetTarget();

	xAND.PS(to,   t1); // to   contains mask
	xAND.PS(from, t2); // from contains mask
	xADD.SS(to, from);
	if (t1 != t1in) mVU->regAlloc->clearNeeded(t1);
	if (t2 != t2in) mVU->regAlloc->clearNeeded(t2);
}

#define clampOp(opX, isPS) {					\
	mVUclamp3(mVU, to,   t1, (isPS)?0xf:0x8);	\
	mVUclamp3(mVU, from, t1, (isPS)?0xf:0x8);	\
	opX(to, from);								\
	mVUclamp4(to, t1, (isPS)?0xf:0x8);			\
}

void SSE_MAXPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	if (CHECK_VU_MINMAXHACK) { xMAX.PS(to, from); }
	else					 { MIN_MAX_PS(mVU, to, from, t1, t2, 0); }
}
void SSE_MINPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	if (CHECK_VU_MINMAXHACK) { xMIN.PS(to, from); }
	else					 { MIN_MAX_PS(mVU, to, from, t1, t2, 1); }
}
void SSE_MAXSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	if (CHECK_VU_MINMAXHACK) { xMAX.SS(to, from); }
	else					 { MIN_MAX_SS(mVU, to, from, t1, 0); }
}
void SSE_MINSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	if (CHECK_VU_MINMAXHACK) { xMIN.SS(to, from); }
	else					 { MIN_MAX_SS(mVU, to, from, t1, 1); }
}
void SSE_ADD2SS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	if (!CHECK_VUADDSUBHACK) { clampOp(xADD.SS, 0); }
	else					 { ADD_SS(mVU, to, from, t1, t2); }
}

// FIXME: why do we need two identical definitions with different names?
void SSE_ADD2PS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xADD.PS, 1);
}
void SSE_ADDPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xADD.PS, 1);
}
void SSE_ADDSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xADD.SS, 0);
}
void SSE_SUBPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xSUB.PS, 1);
}
void SSE_SUBSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xSUB.SS, 0);
}
void SSE_MULPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xMUL.PS, 1);
}
void SSE_MULSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xMUL.SS, 0);
}
void SSE_DIVPS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xDIV.PS, 1);
}
void SSE_DIVSS(mV, const xmm& to, const xmm& from, const xmm& t1 = xEmptyReg, const xmm& t2 = xEmptyReg)
{
	clampOp(xDIV.SS, 0);
}

//------------------------------------------------------------------
// Micro VU - Custom Quick Search
//------------------------------------------------------------------

static __pagealigned u8 mVUsearchXMM[__pagesize];

// Generates a custom optimized block-search function
// Note: Structs must be 16-byte aligned! (GCC doesn't guarantee this)
void mVUcustomSearch() {
	HostSys::MemProtectStatic(mVUsearchXMM, Protect_ReadWrite, false);
	memset_8<0xcc,__pagesize>(mVUsearchXMM);
	xSetPtr(mVUsearchXMM);

	xMOVAPS  (xmm0, ptr32[ecx]);
	xPCMP.EQD(xmm0, ptr32[edx]);
	xMOVAPS  (xmm1, ptr32[ecx + 0x10]);
	xPCMP.EQD(xmm1, ptr32[edx + 0x10]);
	xPAND	 (xmm0, xmm1);

	xMOVMSKPS(eax, xmm0);
	xCMP	 (eax, 0xf);
	xForwardJL8 exitPoint;

	xMOVAPS  (xmm0, ptr32[ecx + 0x20]);
	xPCMP.EQD(xmm0, ptr32[edx + 0x20]);
	xMOVAPS	 (xmm1, ptr32[ecx + 0x30]);
	xPCMP.EQD(xmm1, ptr32[edx + 0x30]);
	xPAND	 (xmm0, xmm1);

	xMOVAPS  (xmm2, ptr32[ecx + 0x40]);
	xPCMP.EQD(xmm2, ptr32[edx + 0x40]);
	xMOVAPS  (xmm3, ptr32[ecx + 0x50]);
	xPCMP.EQD(xmm3, ptr32[edx + 0x50]);
	xPAND	 (xmm2, xmm3);

	xMOVAPS	 (xmm4, ptr32[ecx + 0x60]);
	xPCMP.EQD(xmm4, ptr32[edx + 0x60]);
	xMOVAPS	 (xmm5, ptr32[ecx + 0x70]);
	xPCMP.EQD(xmm5, ptr32[edx + 0x70]);
	xPAND	 (xmm4, xmm5);

	xMOVAPS  (xmm6, ptr32[ecx + 0x80]);
	xPCMP.EQD(xmm6, ptr32[edx + 0x80]);
	xMOVAPS  (xmm7, ptr32[ecx + 0x90]);
	xPCMP.EQD(xmm7, ptr32[edx + 0x90]);
	xPAND	 (xmm6, xmm7);

	xPAND (xmm0, xmm2);
	xPAND (xmm4, xmm6);
	xPAND (xmm0, xmm4);
	xMOVMSKPS(eax, xmm0);

	exitPoint.SetTarget();
	xRET();
	HostSys::MemProtectStatic(mVUsearchXMM, Protect_ReadOnly, true);
}
