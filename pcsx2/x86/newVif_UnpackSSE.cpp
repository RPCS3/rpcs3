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
#include "newVif_UnpackSSE.h"

#define xMOV8(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV16(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV32(regX, loc)	xMOVSSZX(regX, loc)
#define xMOV64(regX, loc)	xMOVUPS(regX, loc)
#define xMOV128(regX, loc)	xMOVUPS(regX, loc)

static __pagealigned u8 nVifUpkExec[__pagesize*4];

// Merges xmm vectors without modifying source reg
void mergeVectors(int dest, int src, int temp, int xyzw) {
	if (x86caps.hasStreamingSIMD4Extensions  || (xyzw==15) 
	|| (xyzw==12) || (xyzw==11) || (xyzw==8) || (xyzw==3)) {
		mVUmergeRegs(dest, src, xyzw);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(temp, src);
		mVUmergeRegs(dest, temp, xyzw);
	}
}

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

// =====================================================================================================
//  VifUnpackSSE_Base Section
// =====================================================================================================
VifUnpackSSE_Base::VifUnpackSSE_Base()
	: dstIndirect(ecx)		// parameter 1 of __fastcall
	, srcIndirect(edx)		// parameter 2 of __fastcall
	, workReg( xmm1 )
	, destReg( xmm0 )
{
}

void VifUnpackSSE_Base::xMovDest() const {
	if (IsUnmaskedOp())	{ xMOVAPS (ptr[dstIndirect], destReg); }
	else				{ doMaskWrite(destReg); }
}

void VifUnpackSSE_Base::xShiftR(const xRegisterSSE& regX, int n) const {
	if (usn)	{ xPSRL.D(regX, n); }
	else		{ xPSRA.D(regX, n); }
}

void VifUnpackSSE_Base::xPMOVXX8(const xRegisterSSE& regX) const {
	if (usn)	xPMOVZX.BD(regX, ptr32[srcIndirect]);
	else		xPMOVSX.BD(regX, ptr32[srcIndirect]);
}

void VifUnpackSSE_Base::xPMOVXX16(const xRegisterSSE& regX) const {
	if (usn)	xPMOVZX.WD(regX, ptr64[srcIndirect]);
	else		xPMOVSX.WD(regX, ptr64[srcIndirect]);
}

void VifUnpackSSE_Base::xUPK_S_32() const {
	xMOV32     (workReg, ptr32[srcIndirect]);
	xPSHUF.D   (destReg, workReg, _v0);
}

void VifUnpackSSE_Base::xUPK_S_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (workReg);
}
else {
	xMOV16     (workReg, ptr32[srcIndirect]);
	xPUNPCK.LWD(workReg, workReg);
	xShiftR    (workReg, 16);
}
	xPSHUF.D   (destReg, workReg, _v0);
}

void VifUnpackSSE_Base::xUPK_S_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (workReg);
}
else {
	xMOV8      (workReg, ptr32[srcIndirect]);
	xPUNPCK.LBW(workReg, workReg);
	xPUNPCK.LWD(workReg, workReg);
	xShiftR    (workReg, 24);
}
	xPSHUF.D   (destReg, workReg, _v0);
}

void VifUnpackSSE_Base::xUPK_V2_32() const {
	xMOV64     (destReg, ptr32[srcIndirect]);
}

void VifUnpackSSE_Base::xUPK_V2_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (destReg);
}
else {
	xMOV32     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 16);
}
}

void VifUnpackSSE_Base::xUPK_V2_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (destReg);
}
else {
	xMOV16     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LBW(destReg, destReg);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 24);
}
}

void VifUnpackSSE_Base::xUPK_V3_32() const {
	xMOV128    (destReg, ptr32[srcIndirect]);
}

void VifUnpackSSE_Base::xUPK_V3_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (destReg);
}
else {
	xMOV64     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 16);
}
}

void VifUnpackSSE_Base::xUPK_V3_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (destReg);
}
else {
	xMOV32     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LBW(destReg, destReg);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 24);
}
}

void VifUnpackSSE_Base::xUPK_V4_32() const {
	xMOV128    (destReg, ptr32[srcIndirect]);
}

void VifUnpackSSE_Base::xUPK_V4_16() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX16  (destReg);
}
else {
	xMOV64     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 16);
}
}

void VifUnpackSSE_Base::xUPK_V4_8() const {
if (x86caps.hasStreamingSIMD4Extensions) {
	xPMOVXX8   (destReg);
}
else {
	xMOV32     (destReg, ptr32[srcIndirect]);
	xPUNPCK.LBW(destReg, destReg);
	xPUNPCK.LWD(destReg, destReg);
	xShiftR    (destReg, 24);
}
}

void VifUnpackSSE_Base::xUPK_V4_5() const {
	xMOV16		(workReg, ptr32[srcIndirect]);
	xPSHUF.D	(workReg, workReg, _v0);
	xPSLL.D		(workReg, 3);			// ABG|R5.000
	xMOVAPS		(destReg, workReg);		// x|x|x|R
	xPSRL.D		(workReg, 8);			// ABG
	xPSLL.D		(workReg, 3);			// AB|G5.000
	mVUmergeRegs(destReg.Id, workReg.Id, 0x4);	// x|x|G|R
	xPSRL.D		(workReg, 8);			// AB
	xPSLL.D		(workReg, 3);			// A|B5.000
	mVUmergeRegs(destReg.Id, workReg.Id, 0x2);	// x|B|G|R
	xPSRL.D		(workReg, 8);			// A
	xPSLL.D		(workReg, 7);			// A.0000000
	mVUmergeRegs(destReg.Id, workReg.Id, 0x1);	// A|B|G|R
	xPSLL.D		(destReg, 24); // can optimize to
	xPSRL.D		(destReg, 24); // single AND...
}

void VifUnpackSSE_Base::xUnpack( int upknum ) const
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
//  VifUnpackSSE_Simple
// =====================================================================================================

VifUnpackSSE_Simple::VifUnpackSSE_Simple(bool usn_, bool domask_, int curCycle_)
{
	curCycle	= curCycle_;
	usn			= usn_;
	doMask		= domask_;
}

void VifUnpackSSE_Simple::doMaskWrite(const xRegisterSSE& regX) const {
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

	VifUnpackSSE_Simple vpugen( !!usn, !!mask, curCycle );

	for( int i=0; i<16; ++i )
	{
		nVifCall& ucall( nVifUpk[((usnpart+maskpart+i) * 4) + curCycle] );
		ucall = NULL;
		if( nVifT[i] == 0 ) continue;
		
		ucall = (nVifCall)xGetAlignedCallTarget();
		vpugen.xUnpack(i);
		vpugen.xMovDest();
		xRET();

		pxAssert( ((uptr)xGetPtr() - (uptr)nVifUpkExec) < sizeof(nVifUpkExec) );
	}
}

void VifUnpackSSE_Init()
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
