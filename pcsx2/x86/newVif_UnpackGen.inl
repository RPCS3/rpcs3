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

#define xMaskWrite(regX, x) {					\
	if (x==0) xMOVAPS(xmm7, ptr32[ecx]);		\
	if (x==1) xMOVAPS(xmm7, ptr32[ecx+0x10]);	\
	if (x==2) xMOVAPS(xmm7, ptr32[ecx+0x20]);	\
	int offX = aMin(curCycle+x, 4);				\
	xPAND(regX, ptr32[nVifMask[0][offX]]);		\
	xPAND(xmm7, ptr32[nVifMask[1][offX]]);		\
	xPOR (regX, ptr32[nVifMask[2][offX]]);		\
	xPOR (regX, xmm7);							\
	if (x==0) xMOVAPS(ptr32[ecx],      regX);	\
	if (x==1) xMOVAPS(ptr32[ecx+0x10], regX);	\
	if (x==2) xMOVAPS(ptr32[ecx+0x20], regX);	\
}

#define xMovDest(reg0, reg1, reg2) {						\
	if (mask==0) {											\
		if (cycles>=0) { xMOVAPS (ptr32[ecx],      reg0); }	\
		if (cycles>=1) { xMOVAPS (ptr32[ecx+0x10], reg1); }	\
		if (cycles>=2) { xMOVAPS (ptr32[ecx+0x20], reg2); }	\
	}														\
	else {													\
		if (cycles>=0) { xMaskWrite(reg0, 0); }				\
		if (cycles>=1) { xMaskWrite(reg1, 1); }				\
		if (cycles>=2) { xMaskWrite(reg2, 2); }				\
	}														\
}

// xmm2 gets result
void convertRGB() {
	xPSLL.D  (xmm1, 3);   // ABG|R5.000
	xMOVAPS  (xmm2, xmm1);// R5.000 (garbage upper bits)
	xPSRL.D	 (xmm1, 8);   // ABG
	xPSLL.D  (xmm1, 3);   // AB|G5.000
	xMOVAPS  (xmm3, xmm1);// G5.000 (garbage upper bits)
	xPSRL.D	 (xmm1, 8);   // AB
	xPSLL.D  (xmm1, 3);   // A|B5.000
	xMOVAPS  (xmm4, xmm1);// B5.000 (garbage upper bits)
	xPSRL.D	 (xmm1, 8);   // A
	xPSLL.D  (xmm1, 7);   // A.0000000
	
	xPSHUF.D (xmm1, xmm1, _v0); // A|A|A|A
	xPSHUF.D (xmm3, xmm3, _v0); // G|G|G|G
	xPSHUF.D (xmm4, xmm4, _v0); // B|B|B|B
	mVUmergeRegs(XMM2, XMM1, 0x3); // A|x|x|R
	mVUmergeRegs(XMM2, XMM3, 0x4); // A|x|G|R
	mVUmergeRegs(XMM2, XMM4, 0x2); // A|B|G|R

	xPSLL.D  (xmm2, 24); // can optimize to
	xPSRL.D	 (xmm2, 24); // single AND...
}

struct VifUnpackIndexer
{
	int	usn, mask;
	int	curCycle, cyclesToWrite;
	
	nVifCall& GetCall( int packType ) const
	{
		int usnpart		= usn*2*16;
		int maskpart	= mask*16;
		int packpart	= packType;

		int curpart		= curCycle*4;
		int cycpespart	= cyclesToWrite;

		return nVifUpk[((usnpart+maskpart+packpart)*(4*4)) + (curpart+cycpespart)];
	}
	
	void xSetCall( int packType ) const
	{
		GetCall( packType ) = (nVifCall)xGetAlignedCallTarget();
	}

	void xSetNullCall( int packType ) const
	{
		GetCall( packType ) = NULL;
	}
};

// ecx = dest, edx = src
void nVifGen(int usn, int mask, int curCycle, int cycles) {
	const VifUnpackIndexer indexer = { usn, mask, curCycle, cycles };

	indexer.xSetCall(0x0); // S-32
		if (cycles>=0) xMOVUPS  (xmm0, ptr32[edx]);
		if (cycles>=0) xPSHUF.D (xmm1, xmm0, _v0);
		if (cycles>=1) xPSHUF.D (xmm2, xmm0, _v1);
		if (cycles>=2) xPSHUF.D (xmm3, xmm0, _v2);
		if (cycles>=0) xMovDest (xmm1, xmm2, xmm3);
	xRET();

	indexer.xSetCall(0x1); // S-16
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=0) xShiftR    (xmm0, 16);
		if (cycles>=0) xPSHUF.D   (xmm1, xmm0, _v0);
		if (cycles>=1) xPSHUF.D   (xmm2, xmm0, _v1);
		if (cycles>=2) xPSHUF.D   (xmm3, xmm0, _v2);
		if (cycles>=0) xMovDest   (xmm1, xmm2, xmm3);
	xRET();

	indexer.xSetCall(0x2); // S-8
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=0) xPUNPCK.LBW(xmm0, xmm0);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=0) xShiftR    (xmm0, 24);
		if (cycles>=0) xPSHUF.D	  (xmm1, xmm0, _v0);
		if (cycles>=1) xPSHUF.D   (xmm2, xmm0, _v1);
		if (cycles>=2) xPSHUF.D   (xmm3, xmm0, _v2);
		if (cycles>=0) xMovDest   (xmm1, xmm2, xmm3);
	xRET();

	indexer.xSetNullCall(0x3); // ----

	indexer.xSetCall(0x4); // V2-32
		if (cycles>=0) xMOVUPS  (xmm0, ptr32[edx]);
		if (cycles>=2) xMOVUPS  (xmm2, ptr32[edx+0x10]);
		if (cycles>=1) xPSHUF.D (xmm1, xmm0, 0xe);
		if (cycles>=0) xMovDest (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0x5); // V2-16
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=2) xPSHUF.D   (xmm2, xmm0, _v2);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 16);
		if (cycles>=2) xShiftR    (xmm2, 16);
		if (cycles>=1) xPSHUF.D   (xmm1, xmm0, 0xe);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0x6); // V2-8
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=0) xPUNPCK.LBW(xmm0, xmm0);
		if (cycles>=2) xPSHUF.D   (xmm2, xmm0, _v2);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 24);
		if (cycles>=2) xShiftR    (xmm2, 24);
		if (cycles>=1) xPSHUF.D   (xmm1, xmm0, 0xe);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetNullCall(0x7); // ----

	indexer.xSetCall(0x8); // V3-32
		if (cycles>=0) xMOVUPS  (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS  (xmm1, ptr32[edx+12]);
		if (cycles>=2) xMOVUPS  (xmm2, ptr32[edx+24]);
		if (cycles>=0) xMovDest (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0x9); // V3-16
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS    (xmm1, ptr32[edx+6]);
		if (cycles>=2) xMOVUPS    (xmm2, ptr32[edx+12]);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LWD(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 16);
		if (cycles>=1) xShiftR    (xmm1, 16);
		if (cycles>=2) xShiftR    (xmm2, 16);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0xa); // V3-8
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS    (xmm1, ptr32[edx+3]);
		if (cycles>=2) xMOVUPS    (xmm2, ptr32[edx+6]);
		if (cycles>=0) xPUNPCK.LBW(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LBW(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LBW(xmm2, xmm2);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LWD(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 24);
		if (cycles>=1) xShiftR    (xmm1, 24);
		if (cycles>=2) xShiftR    (xmm2, 24);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetNullCall(0xb); // ----

	indexer.xSetCall(0xc); // V4-32
		if (cycles>=0) xMOVUPS  (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS  (xmm1, ptr32[edx+0x10]);
		if (cycles>=2) xMOVUPS  (xmm2, ptr32[edx+0x20]);
		if (cycles>=0) xMovDest (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0xd); // V4-16
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS    (xmm1, ptr32[edx+0x10]);
		if (cycles>=2) xMOVUPS    (xmm2, ptr32[edx+0x20]);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LWD(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 16);
		if (cycles>=1) xShiftR    (xmm1, 16);
		if (cycles>=2) xShiftR    (xmm2, 16);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	indexer.xSetCall(0xe); // V4-8
		if (cycles>=0) xMOVUPS    (xmm0, ptr32[edx]);
		if (cycles>=1) xMOVUPS    (xmm1, ptr32[edx+4]);
		if (cycles>=2) xMOVUPS    (xmm2, ptr32[edx+8]);
		if (cycles>=0) xPUNPCK.LBW(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LBW(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LBW(xmm2, xmm2);
		if (cycles>=0) xPUNPCK.LWD(xmm0, xmm0);
		if (cycles>=1) xPUNPCK.LWD(xmm1, xmm1);
		if (cycles>=2) xPUNPCK.LWD(xmm2, xmm2);
		if (cycles>=0) xShiftR    (xmm0, 24);
		if (cycles>=1) xShiftR    (xmm1, 24);
		if (cycles>=2) xShiftR    (xmm2, 24);
		if (cycles>=0) xMovDest   (xmm0, xmm1, xmm2);
	xRET();

	// A | B5 | G5 | R5
	// ..0.. A 0000000 | ..0.. B 000 | ..0.. G 000 | ..0.. R 000
	indexer.xSetCall(0xf); // V4-5
		if (cycles>=0) xMOVUPS   (xmm0, ptr32[edx]);
		if (cycles>=0) xMOVAPS   (xmm1, xmm0);
		if (cycles>=0) convertRGB();
		if (cycles>=0) xMOVAPS   (ptr32[ecx],      xmm2);
		if (cycles>=1) xMOVAPS   (xmm1, xmm0);
		if (cycles>=1) xPSRL.D	 (xmm1, 16);
		if (cycles>=1) convertRGB();
		if (cycles>=1) xMOVAPS   (ptr32[ecx+0x10], xmm2);
		if (cycles>=2) xPSHUF.D  (xmm1, xmm0, _v1);
		if (cycles>=2) convertRGB();
		if (cycles>=2) xMOVAPS   (ptr32[ecx+0x20], xmm2);
	xRET();

	pxAssert( ((uptr)xGetPtr() - (uptr)nVifUpkExec) < sizeof(nVifUpkExec) );
}
