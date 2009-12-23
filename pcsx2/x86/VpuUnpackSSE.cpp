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

#include "PrecompiledHeader.h"
#include "VpuUnpackSSE.h"

#define xMOV8(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV16(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV32(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV64(regX, loc)	xMOVUPS(regX, loc)
#define xMOV128(regX, loc)	xMOVUPS(regX, loc)

static __pagealigned u8 nVifUpkExec[__pagesize*4];

// =====================================================================================================
//  VpuUnpackSSE_Base Section
// =====================================================================================================
VpuUnpackSSE_Base::VpuUnpackSSE_Base()
	: dstIndirect(ecx)		// parameter 1 of __fastcall
	, srcIndirect(edx)		// parameter 2 of __fastcall
{
}

void VpuUnpackSSE_Base::xMovDest(const xRegisterSSE& srcReg) const {
	if (!doMode && !doMask)	{ xMOVAPS (ptr[dstIndirect], srcReg); }
	else					{ doMaskWrite(srcReg); }
}

void VpuUnpackSSE_Base::xShiftR(const xRegisterSSE& regX, int n) const {
	if (usn)	{ xPSRL.D(regX, n); }
	else		{ xPSRA.D(regX, n); }
}

void VpuUnpackSSE_Base::xPMOVXX8(const xRegisterSSE& regX) const {
	if (usn)	xPMOVZX.BD(regX, ptr32[srcIndirect]);
	else		xPMOVSX.BD(regX, ptr32[srcIndirect]);
}

void VpuUnpackSSE_Base::xPMOVXX16(const xRegisterSSE& regX) const {
	if (usn)	xPMOVZX.WD(regX, ptr64[srcIndirect]);
	else		xPMOVSX.WD(regX, ptr64[srcIndirect]);
}

void VpuUnpackSSE_Base::xUPK_S_32() const {
	xMOV32     (xmm0, ptr32[srcIndirect]);
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

void VpuUnpackSSE_Base::xUPK_S_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0);
}
else {
	xMOV16     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

void VpuUnpackSSE_Base::xUPK_S_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0);
}
else {
	xMOV8      (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xPSHUF.D   (xmm1, xmm0, _v0);
	xMovDest   (xmm1);
}

void VpuUnpackSSE_Base::xUPK_V2_32() const {
	xMOV64     (xmm0, ptr32[srcIndirect]);
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V2_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0);
}
else {
	xMOV32     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V2_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0);
}
else {
	xMOV16     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V3_32() const {
	xMOV128    (xmm0, ptr32[srcIndirect]);
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V3_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0);
}
else {
	xMOV64     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V3_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0);
}
else {
	xMOV32     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V4_32() const {
	xMOV128    (xmm0, ptr32[srcIndirect]);
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V4_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (xmm0);
}
else {
	xMOV64     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 16);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V4_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (xmm0);
}
else {
	xMOV32     (xmm0, ptr32[srcIndirect]);
	xPUNPCK.LBW(xmm0, xmm0);
	xPUNPCK.LWD(xmm0, xmm0);
	xShiftR    (xmm0, 24);
}
	xMovDest   (xmm0);
}

void VpuUnpackSSE_Base::xUPK_V4_5() const {
	xMOV16		(xmm0, ptr32[srcIndirect]);
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

void VpuUnpackSSE_Base::xUnpack( int upknum )
{
	switch( upknum )
	{
		case 0:	 xUPK_S_32();	break;
		case 1:  xUPK_S_16();	break;
		case 2:  xUPK_S_8();	break;

		case 4:  xUPK_V2_32();	break;
		case 5:  xUPK_V2_16();	break;
		case 6:  xUPK_V2_8();	break;

		case 8:  xUPK_V3_32();	break;
		case 9:  xUPK_V3_16();	break;
		case 10: xUPK_V3_8();	break;

		case 12: xUPK_V4_32();	break;
		case 13: xUPK_V4_16();	break;
		case 14: xUPK_V4_8();	break;
		case 15: xUPK_V4_5();	break;

		case 3:  
		case 7:  
		case 11:
			pxFailRel( wxsFormat( L"Vpu/Vif - Invalid Unpack! [%d]", upknum ) );
		break;
	}
}

// =====================================================================================================
//  VpuUnpackSSE_Simple
// =====================================================================================================

VpuUnpackSSE_Simple::VpuUnpackSSE_Simple(bool usn_, bool domask_, int curCycle_)
{
	curCycle	= curCycle_;
	usn			= usn_;
	doMask		= domask_;
}

void VpuUnpackSSE_Simple::doMaskWrite(const xRegisterSSE& regX) const {
	xMOVAPS(xmm7, ptr[dstIndirect]);
	int offX = aMin(curCycle, 3);
	xPAND(regX, ptr32[nVifMask[0][offX]]);
	xPAND(xmm7, ptr32[nVifMask[1][offX]]);
	xPOR (regX, ptr32[nVifMask[2][offX]]);
	xPOR (regX, xmm7);
	xMOVAPS(ptr[dstIndirect], regX);
}

// ecx = dest, edx = src
static void nVifGen(int usn, int mask, int curCycle) {

	int usnpart		= usn*2*16;
	int maskpart	= mask*16;
	int curpart		= curCycle;

	VpuUnpackSSE_Simple vpugen( !!usn, !!mask, curCycle );

	for( int i=0; i<16; ++i )
	{
		nVifCall& ucall( nVifUpk[((usnpart+maskpart+i) * 4) + (curpart)] );
		ucall = NULL;
		if( nVifT[i] == 0 ) continue;
		
		ucall = (nVifCall)xGetAlignedCallTarget();
		vpugen.xUnpack(i);
		xRET();

		pxAssert( ((uptr)xGetPtr() - (uptr)nVifUpkExec) < sizeof(nVifUpkExec) );
	}
}

void VpuUnpackSSE_Init()
{
	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadWrite, false);
	memset8<0xcc>( nVifUpkExec );

	xSetPtr( nVifUpkExec );

	for (int a = 0; a < 2; a++) {
		for (int b = 0; b < 2; b++) {
			for (int c = 0; c < 4; c++) {
				nVifGen(a, b, c);
			}}}

	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadOnly, true);
}