/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#define vUPK(x) void x(nVifStruct* v, int doMask)
#define _doUSN  (v->vifBlock->upkType & 0x20)
#undef  xMovDest
#undef  xShiftR
#undef  xPMOVXX8
#undef  xPMOVXX16
#undef  xMaskWrite
#define makeMergeMask(x) {									\
	x = ((x&0x40)>>6) | ((x&0x10)>>3) | (x&4) | ((x&1)<<3);	\
}
void doMaskWrite(const xRegisterSSE& regX, nVifStruct* v, int doMask) {
	if (regX.Id > 1) DevCon.WriteLn("Reg Overflow!!!");
	int doMode = doMask>>1; doMask &= 1;
	int cc =  aMin(v->vif->cl, 3);
	u32 m0 = (v->vifBlock->mask >> (cc * 8)) & 0xff;
	u32 m1 =  m0 & 0xaaaa;
	u32 m2 =(~m1>>1) &  m0;
	u32 m3 = (m1>>1) & ~m0;
	u32 m4 = (m1>>1) &  m0;
	makeMergeMask(m2);
	makeMergeMask(m3);
	makeMergeMask(m4);
	if (doMask&&m4) { xMOVAPS(xmmTemp, ptr32[ecx]);				 } // Load Write Protect
	if (doMask&&m2) { mVUmergeRegs(regX.Id, xmmRow.Id,		m2); } // Merge Row
	if (doMask&&m3) { mVUmergeRegs(regX.Id, xmmCol0.Id+cc,	m3); } // Merge Col
	if (doMask&&m4) { mVUmergeRegs(regX.Id, xmmTemp.Id,		m4); } // Merge Write Protect
	if (doMode) {
		u32 m5 = (~m1>>1) & ~m0;
		if (!doMask)  m5 = 0xf;
		else		  makeMergeMask(m5);
		if (m5 < 0xf) {
			xPXOR(xmmTemp, xmmTemp);
			mVUmergeRegs(xmmTemp.Id, xmmRow.Id, m5);
			xPADD.D(regX, xmmTemp);
			if (doMode==2) mVUmergeRegs(xmmRow.Id, regX.Id, m5);
		}
		else if (m5 == 0xf) {
			xPADD.D(regX, xmmRow);
			if (doMode==2) xMOVAPS(xmmRow, regX);
		}
	}
	xMOVAPS(ptr32[ecx], regX);	
}
#define xMovDest(regX) {							\
	if (!doMask){ xMOVAPS (ptr32[ecx], regX); }		\
	else		{ doMaskWrite(regX, v, doMask); }	\
}
#define xShiftR(regX, n) {							\
	if (_doUSN)	{ xPSRL.D(regX, n); }				\
	else		{ xPSRA.D(regX, n); }				\
}
#define xPMOVXX8(regX, src) {						\
	if (_doUSN) xPMOVZX.BD(regX, src);				\
	else		xPMOVSX.BD(regX, src);				\
}
#define xPMOVXX16(regX, src) {						\
	if (_doUSN) xPMOVZX.WD(regX, src);				\
	else		xPMOVSX.WD(regX, src);				\
}

// ecx = dest, edx = src
vUPK(nVif_S_32) {
	xMOV32     (xmm0, ptr32[edx]);
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

vUPK(nVif_S_16) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0, ptr64[edx]);
}
else {
	xMOV16     (xmm0, ptr32[edx]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

vUPK(nVif_S_8) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0, ptr32[edx]);
}
else {
	xMOV8      (xmm0, ptr32[edx]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

vUPK(nVif_V2_32) {
	xMOV64     (xmm0, ptr32[edx]);
	xMovDest   (xmm0);
}

vUPK(nVif_V2_16) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0, ptr64[edx]);
}
else {
	xMOV32     (xmm0, ptr32[edx]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V2_8) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0, ptr32[edx]);
}
else {
	xMOV16     (xmm0, ptr32[edx]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V3_32) {
	xMOV128    (xmm0, ptr32[edx]);
	xMovDest   (xmm0);
}

vUPK(nVif_V3_16) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0, ptr64[edx]);
}
else {
	xMOV64     (xmm0, ptr32[edx]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V3_8) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0, ptr32[edx]);
}
else {
	xMOV32     (xmm0, ptr32[edx]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V4_32) {
	xMOV128    (xmm0, ptr32[edx]);
	xMovDest   (xmm0);
}

vUPK(nVif_V4_16) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0, ptr64[edx]);
}
else {
	xMOV64     (xmm0, ptr32[edx]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V4_8) {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0, ptr32[edx]);
}
else {
	xMOV32     (xmm0, ptr32[edx]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

vUPK(nVif_V4_5) {
	xMOV16		(xmm0, ptr32[edx]);
	xPSHUF.D	(xmm0, xmm0, _v0);
	xPSLL.D		(xmm0, 3);			// ABG|R5.000
	xMOVAPS		(xmm1, xmm0);		// x|x|x|R
	xPSRL.D		(xmm0, 8);			// ABG
	xPSLL.D		(xmm0, 3);			// AB|G5.000
	mVUmergeRegs(XMM1, XMM0, 0x4);	// x|x|G|R
	xPSRL.D		(xmm0, 8);			// AB
	xPSLL.D		(xmm0, 3);			// A|B5.000
	mVUmergeRegs(XMM1, XMM0, 0x2);	// x|B|G|R
	xPSRL.D		(xmm0, 8);			// A
	xPSLL.D		(xmm0, 7);			// A.0000000
	mVUmergeRegs(XMM1, XMM0, 0x1);	// A|B|G|R
	xPSLL.D		(xmm1, 24); // can optimize to
	xPSRL.D		(xmm1, 24); // single AND...
	xMovDest	(xmm1);
}

vUPK(nVif_unkown) {
	Console.Error("nVif%d - Invalid Unpack! [%d]", v->idx, v->vif->tag.cmd & 0xf);
}

void (*xUnpack[16])(nVifStruct* v, int doMask) = {
	nVif_S_32,
	nVif_S_16,
	nVif_S_8,
	nVif_unkown,
	nVif_V2_32,
	nVif_V2_16,
	nVif_V2_8,
	nVif_unkown,
	nVif_V3_32,
	nVif_V3_16,
	nVif_V3_8,
	nVif_unkown,
	nVif_V4_32,
	nVif_V4_16,
	nVif_V4_8,
	nVif_V4_5,
};

// Loads Row/Col Data from vifRegs instead of g_vifmask
// Useful for testing vifReg and g_vifmask inconsistency.
void loadRowCol(nVifStruct& v) {
	xMOVAPS(xmm0, ptr32[&v.vifRegs->r0]);
	xMOVAPS(xmm1, ptr32[&v.vifRegs->r1]);
	xMOVAPS(xmm2, ptr32[&v.vifRegs->r2]);
	xMOVAPS(xmm6, ptr32[&v.vifRegs->r3]);
	xPSHUF.D(xmm0, xmm0, _v0);
	xPSHUF.D(xmm1, xmm1, _v0);
	xPSHUF.D(xmm2, xmm2, _v0);
	xPSHUF.D(xmm6, xmm6, _v0);
	mVUmergeRegs(XMM6, XMM0, 8);
	mVUmergeRegs(XMM6, XMM1, 4);
	mVUmergeRegs(XMM6, XMM2, 2);
	xMOVAPS(xmm2, ptr32[&v.vifRegs->c0]);
	xMOVAPS(xmm3, ptr32[&v.vifRegs->c1]);
	xMOVAPS(xmm4, ptr32[&v.vifRegs->c2]);
	xMOVAPS(xmm5, ptr32[&v.vifRegs->c3]);
	xPSHUF.D(xmm2, xmm2, _v0);
	xPSHUF.D(xmm3, xmm3, _v0);
	xPSHUF.D(xmm4, xmm4, _v0);
	xPSHUF.D(xmm5, xmm5, _v0);
}

void writeBackRow(nVifStruct& v) {
	u32* row = (v.idx) ? g_vifmask.Row1 : g_vifmask.Row0;
	xMOVAPS(ptr32[row], xmmRow);
	DevCon.WriteLn("nVif: writing back row reg! [doMode = 2]");
	// ToDo: Do we need to write back to vifregs.rX too!? :/
}

static __pagealigned u8 nVifMemCmp[__pagesize];

void emitCustomCompare() {
	HostSys::MemProtectStatic(nVifMemCmp, Protect_ReadWrite, false);
	memset_8<0xcc,__pagesize>(nVifMemCmp);
	xSetPtr(nVifMemCmp);

	xMOVAPS  (xmm0, ptr32[ecx]);
	xPCMP.EQD(xmm0, ptr32[edx]);
	xMOVMSKPS(eax, xmm0);

	xRET();
	HostSys::MemProtectStatic(nVifMemCmp, Protect_ReadOnly, true);
}