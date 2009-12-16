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

#define xMaskWrite(regX) {					\
	xMOVAPS(xmm7, ptr32[ecx]);				\
	int offX = aMin(curCycle, 4);			\
	xPAND(regX, ptr32[nVifMask[0][offX]]);	\
	xPAND(xmm7, ptr32[nVifMask[1][offX]]);	\
	xPOR (regX, ptr32[nVifMask[2][offX]]);	\
	xPOR (regX, xmm7);						\
	xMOVAPS(ptr32[ecx], regX);				\
}
#define xMovDest(regX) {						 \
	if (mask==0) { xMOVAPS (ptr32[ecx], regX); } \
	else		 { xMaskWrite(regX); }			 \
}
#define xShiftR(regX, n) {			\
	if (usn) { xPSRL.D(regX, n); }	\
	else	 { xPSRA.D(regX, n); }	\
}

struct VifUnpackIndexer {
	int	usn, mask;
	int	curCycle, cyclesToWrite;
	
	nVifCall& GetCall(int packType) const {
		int usnpart		= usn*2*16;
		int maskpart	= mask*16;
		int packpart	= packType;
		int curpart		= curCycle;

		return nVifUpk[((usnpart+maskpart+packpart)*4) + (curpart)];
	}
	
	void xSetCall(int packType) const {
		GetCall( packType ) = (nVifCall)xGetAlignedCallTarget();
	}

	void xSetNullCall(int packType) const {
		GetCall( packType ) = NULL;
	}
};
// xMOVSS doesn't seem to have all overloads defined with new emitter
#define xMOVSSS(regX, loc) SSE_MOVSS_Rm_to_XMM(0, 2, 0)

#define xMOV8(regX, loc)	xMOVSSS(regX, loc)
#define xMOV16(regX, loc)	xMOVSSS(regX, loc)
#define xMOV32(regX, loc)	xMOVSSS(regX, loc)
#define xMOV64(regX, loc)	xMOVUPS(regX, loc)
#define xMOV128(regX, loc)	xMOVUPS(regX, loc)

// ecx = dest, edx = src
void nVifGen(int usn, int mask, int curCycle) {
	const VifUnpackIndexer indexer = { usn, mask, curCycle, 0 };

	indexer.xSetCall(0x0); // S-32
		xMOV32     (xmm0, ptr32[edx]);
		xPSHUF.D   (xmm1, xmm0, _v0);
		xMovDest   (xmm1);
	xRET();

	indexer.xSetCall(0x1); // S-16
		xMOV16     (xmm0, ptr32[edx]);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 16);
		xPSHUF.D   (xmm1, xmm0, _v0);
		xMovDest   (xmm1);
	xRET();

	indexer.xSetCall(0x2); // S-8
		xMOV8      (xmm0, ptr32[edx]);
		xPUNPCK.LBW(xmm0, xmm0);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 24);
		xPSHUF.D   (xmm1, xmm0, _v0);
		xMovDest   (xmm1);
	xRET();

	indexer.xSetNullCall(0x3); // ----

	indexer.xSetCall(0x4); // V2-32
		xMOV64     (xmm0, ptr32[edx]);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0x5); // V2-16
		xMOV32     (xmm0, ptr32[edx]);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 16);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0x6); // V2-8
		xMOV16     (xmm0, ptr32[edx]);
		xPUNPCK.LBW(xmm0, xmm0);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 24);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetNullCall(0x7); // ----

	indexer.xSetCall(0x8); // V3-32
		xMOV128    (xmm0, ptr32[edx]);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0x9); // V3-16
		xMOV64     (xmm0, ptr32[edx]);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 16);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0xa); // V3-8
		xMOV32     (xmm0, ptr32[edx]);
		xPUNPCK.LBW(xmm0, xmm0);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 24);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetNullCall(0xb); // ----

	indexer.xSetCall(0xc); // V4-32
		xMOV128    (xmm0, ptr32[edx]);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0xd); // V4-16
		xMOV64     (xmm0, ptr32[edx]);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 16);
		xMovDest   (xmm0);
	xRET();

	indexer.xSetCall(0xe); // V4-8
		xMOV32     (xmm0, ptr32[edx]);
		xPUNPCK.LBW(xmm0, xmm0);
		xPUNPCK.LWD(xmm0, xmm0);
		xShiftR    (xmm0, 24);
		xMovDest   (xmm0);
	xRET();

	// A | B5 | G5 | R5
	// ..0.. A 0000000 | ..0.. B 000 | ..0.. G 000 | ..0.. R 000
	indexer.xSetCall(0xf); // V4-5
		xMOV16		(xmm0, ptr32[edx]);
		xMOVAPS		(xmm1, xmm0);
		xPSLL.D		(xmm1, 3);   // ABG|R5.000
		xMOVAPS		(xmm2, xmm1);// R5.000 (garbage upper bits)
		xPSRL.D		(xmm1, 8);   // ABG
		xPSLL.D		(xmm1, 3);   // AB|G5.000
		xMOVAPS		(xmm3, xmm1);// G5.000 (garbage upper bits)
		xPSRL.D		(xmm1, 8);   // AB
		xPSLL.D		(xmm1, 3);   // A|B5.000
		xMOVAPS		(xmm4, xmm1);// B5.000 (garbage upper bits)
		xPSRL.D		(xmm1, 8);   // A
		xPSLL.D		(xmm1, 7);   // A.0000000
		xPSHUF.D	(xmm1, xmm1, _v0); // A|A|A|A
		xPSHUF.D	(xmm3, xmm3, _v0); // G|G|G|G
		xPSHUF.D	(xmm4, xmm4, _v0); // B|B|B|B
		mVUmergeRegs(XMM2, XMM1, 0x3); // A|x|x|R
		mVUmergeRegs(XMM2, XMM3, 0x4); // A|x|G|R
		mVUmergeRegs(XMM2, XMM4, 0x2); // A|B|G|R
		xPSLL.D		(xmm2, 24); // can optimize to
		xPSRL.D		(xmm2, 24); // single AND...
		xMovDest	(xmm2);
	xRET();

	pxAssert( ((uptr)xGetPtr() - (uptr)nVifUpkExec) < sizeof(nVifUpkExec) );
}
