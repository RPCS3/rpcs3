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

// newVif Dynarec - Dynamically Recompiles Vif 'unpack' Packets
// authors: cottonvibes(@gmail.com)
//			Jake.Stine (@gmail.com)

#include "PrecompiledHeader.h"
#include "VifUnpackSSE.h"

#if newVif

static __aligned16 nVifBlock _vBlock = {0};
static __pagealigned u8 nVifMemCmp[__pagesize];

static void emitCustomCompare() {
	HostSys::MemProtectStatic(nVifMemCmp, Protect_ReadWrite, false);
	memset8<0xcc>(nVifMemCmp);
	xSetPtr(nVifMemCmp);

	xMOVAPS  (xmm0, ptr32[ecx]);
	xPCMP.EQD(xmm0, ptr32[edx]);
	xMOVMSKPS(eax, xmm0);
	xAND	 (eax, 0x7);		// ignore top 4 bytes (recBlock pointer)

	xRET();
	HostSys::MemProtectStatic(nVifMemCmp, Protect_ReadOnly, true);
}

void dVifInit(int idx) {
	nVif[idx].idx		=  idx;
	nVif[idx].VU		=  idx ? &VU1     : &VU0;
	nVif[idx].vif		=  idx ? &vif1    : &vif0;
	nVif[idx].vifRegs	=  idx ? vif1Regs : vif0Regs;
	nVif[idx].vuMemEnd  =  idx ? ((u8*)(VU1.Mem + 0x4000)) : ((u8*)(VU0.Mem + 0x1000));
	nVif[idx].vuMemLimit=  idx ? 0x3ff0 : 0xff0;
	nVif[idx].vifCache	=  new BlockBuffer(_1mb*4); // 4mb Rec Cache
	nVif[idx].vifBlocks =  new HashBucket<_tParams>();
	nVif[idx].recPtr	=  nVif[idx].vifCache->getBlock();
	nVif[idx].recEnd	= &nVif[idx].recPtr[nVif[idx].vifCache->getSize()-(_1mb/4)]; // .25mb Safe Zone
	//emitCustomCompare();
}

// Loads Row/Col Data from vifRegs instead of g_vifmask
// Useful for testing vifReg and g_vifmask inconsistency.
static void loadRowCol(nVifStruct& v) {
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

VifUnpackSSE_Dynarec::VifUnpackSSE_Dynarec(const nVifStruct& vif_, const nVifBlock& vifBlock_)
	: v(vif_)
	, vB(vifBlock_)
{
	isFill		= (vB.cl < vB.wl);
	usn			= (vB.upkType>>5) & 1;
	doMask		= (vB.upkType>>4) & 1;
	doMode		= vB.mode & 3;
}

#define makeMergeMask(x) {									\
	x = ((x&0x40)>>6) | ((x&0x10)>>3) | (x&4) | ((x&1)<<3);	\
}

_f void VifUnpackSSE_Dynarec::SetMasks(int cS) const {
	u32 m0 = vB.mask;
	u32 m1 =  m0 & 0xaaaaaaaa;
	u32 m2 =(~m1>>1) &  m0;
	u32 m3 = (m1>>1) & ~m0;
	u32* row = (v.idx) ? g_vifmask.Row1 : g_vifmask.Row0;
	u32* col = (v.idx) ? g_vifmask.Col1 : g_vifmask.Col0;
	if((m2&&doMask) || doMode) { xMOVAPS(xmmRow, ptr32[row]); }
	if (m3&&doMask) {
		xMOVAPS(xmmCol0, ptr32[col]); 
		if ((cS>=2) && (m3&0x0000ff00)) xPSHUF.D(xmmCol1, xmmCol0, _v1);
		if ((cS>=3) && (m3&0x00ff0000)) xPSHUF.D(xmmCol2, xmmCol0, _v2);
		if ((cS>=4) && (m3&0xff000000)) xPSHUF.D(xmmCol3, xmmCol0, _v3);
		if ((cS>=1) && (m3&0x000000ff)) xPSHUF.D(xmmCol0, xmmCol0, _v0);
	}
	//if (doMask||doMode) loadRowCol((nVifStruct&)v);
}

void VifUnpackSSE_Dynarec::doMaskWrite(const xRegisterSSE& regX) const {
	pxAssumeDev(regX.Id <= 1, "Reg Overflow! XMM2 thru XMM6 are reserved for masking.");
	int t  =  regX.Id ? 0 : 1; // Get Temp Reg
	int cc =  aMin(vCL, 3);
	u32 m0 = (vB.mask >> (cc * 8)) & 0xff;
	u32 m1 =  m0 & 0xaa;
	u32 m2 =(~m1>>1) &  m0;
	u32 m3 = (m1>>1) & ~m0;
	u32 m4 = (m1>>1) &  m0;
	makeMergeMask(m2);
	makeMergeMask(m3);
	makeMergeMask(m4);
	if (doMask&&m4) { xMOVAPS(xmmTemp, ptr[dstIndirect]);			} // Load Write Protect
	if (doMask&&m2) { mergeVectors(regX.Id, xmmRow.Id,		t, m2); } // Merge Row
	if (doMask&&m3) { mergeVectors(regX.Id, xmmCol0.Id+cc,	t, m3); } // Merge Col
	if (doMask&&m4) { mergeVectors(regX.Id, xmmTemp.Id,		t, m4); } // Merge Write Protect
	if (doMode) {
		u32 m5 = (~m1>>1) & ~m0;
		if (!doMask)  m5 = 0xf;
		else		  makeMergeMask(m5);
		if (m5 < 0xf) {
			xPXOR(xmmTemp, xmmTemp);
			mergeVectors(xmmTemp.Id, xmmRow.Id, t, m5);
			xPADD.D(regX, xmmTemp);
			if (doMode==2) mergeVectors(xmmRow.Id, regX.Id, t, m5);
		}
		else if (m5 == 0xf) {
			xPADD.D(regX, xmmRow);
			if (doMode==2) xMOVAPS(xmmRow, regX);
		}
	}
	xMOVAPS(ptr32[dstIndirect], regX);	
}

void VifUnpackSSE_Dynarec::writeBackRow() const {
	u32* row = (v.idx) ? g_vifmask.Row1 : g_vifmask.Row0;
	xMOVAPS(ptr32[row], xmmRow);
	DevCon.WriteLn("nVif: writing back row reg! [doMode = 2]");
	// ToDo: Do we need to write back to vifregs.rX too!? :/
}

static void ShiftDisplacementWindow( xAddressInfo& addr, const xRegister32& modReg )
{
	// Shifts the displacement factor of a given indirect address, so that the address
	// remains in the optimal 0xf0 range (which allows for byte-form displacements when
	// generating instructions).

	int addImm = 0;
	while( addr.Displacement >= 0x80 )
	{
		addImm += 0xf0;
		addr -= 0xf0;
	}
	if(addImm) xADD(modReg, addImm);
}
static bool UsesTwoRegs[] = 
{
	true,  true,  true,  true,
	false, false, false, false,
	false, false, false, false,
	false, false, false, true,

};

void VifUnpackSSE_Dynarec::CompileRoutine() {
	const int  upkNum	 = v.vif->cmd & 0xf;
	const u8&  vift		 = nVifT[upkNum];
	const int  cycleSize = isFill ? vB.cl : vB.wl;
	const int  blockSize = isFill ? vB.wl : vB.cl;
	const int  skipSize	 = blockSize - cycleSize;

	int vNum = v.vifRegs->num;
	vCL		 = v.vif->cl;
	doMode	 = upkNum == 0xf ? 0 : doMode;

	SetMasks(cycleSize);

	while (vNum) {

		ShiftDisplacementWindow( srcIndirect, edx );
		ShiftDisplacementWindow( dstIndirect, ecx );

		if (vCL < cycleSize) { 
			xUnpack(upkNum);
			xMovDest();

			dstIndirect += 16;
			srcIndirect += vift;

			if( IsUnmaskedOp() ) {
				++destReg;
				++workReg;
			}
			
			vNum--;
			if (++vCL == blockSize) vCL = 0;
		}
		else if (isFill) {
			DevCon.WriteLn("filling mode!");
			VifUnpackSSE_Dynarec fill( VifUnpackSSE_Dynarec::FillingWrite( *this ) );
			fill.xUnpack(upkNum);
			fill.xMovDest();

			dstIndirect += 16;
			vNum--;
			if (++vCL == blockSize) vCL = 0;
		}
		else {
			dstIndirect += (16 * skipSize);
			vCL = 0;
		}
	}

	if (doMode==2) writeBackRow();
	xMOV(ptr32[&v.vif->cl],	     vCL);
	xMOV(ptr32[&v.vifRegs->num], vNum);
	xRET();
}

static _f u8* dVifsetVUptr(const nVifStruct& v, int cl, int wl, bool isFill) {
	u8* endPtr; // Check if we need to wrap around VU memory
	u8* ptr = (u8*)(v.VU->Mem + (v.vif->tag.addr & v.vuMemLimit));
	if (!isFill) { // Account for skip-cycles
		int skipSize  = cl - wl;
		int blocks    = _vBlock.num / wl;
		int skips	  = (blocks * skipSize + _vBlock.num) * 16;
		endPtr = ptr + skips;
	}
	else endPtr = ptr + (_vBlock.num * 16);
	if ( endPtr > v.vuMemEnd ) {
		DevCon.WriteLn("nVif - VU Mem Ptr Overflow; falling back to interpreter.");
		ptr = NULL; // Fall Back to Interpreters which have wrap-around logic
	}
	return ptr;
}

static _f void dVifRecLimit(int idx) {
	if (nVif[idx].recPtr > nVif[idx].recEnd) {
		DevCon.WriteLn("nVif Rec - Out of Rec Cache! [%x > %x]", nVif[idx].recPtr, nVif[idx].recEnd);
		nVif[idx].vifBlocks->clear();
		nVif[idx].recPtr = nVif[idx].vifCache->getBlock();
	}
}

_f void dVifUnpack(int idx, u8 *data, u32 size, bool isFill) {

	const nVifStruct& v		= nVif[idx];
	const u8	upkType		= v.vif->cmd & 0x1f | ((!!v.vif->usn) << 5);
	const int	doMask		= v.vif->cmd & 0x10;
	const int	cycle_cl	= v.vifRegs->cycle.cl;
	const int	cycle_wl	= v.vifRegs->cycle.wl;
	const int	cycleSize	= isFill ? cycle_cl : cycle_wl;
	const int	blockSize	= isFill ? cycle_wl : cycle_cl;

	if (v.vif->cl >= blockSize) v.vif->cl = 0;

	_vBlock.upkType = upkType;
	_vBlock.num		= (u8&)v.vifRegs->num;
	_vBlock.mode	= (u8&)v.vifRegs->mode;
	_vBlock.scl		= v.vif->cl;
	_vBlock.cl		= cycle_cl;
	_vBlock.wl		= cycle_wl;

	// Zero out the mask parameter if it's unused -- games leave random junk
	// values here which cause false recblock cache misses.
	_vBlock.mask	= doMask ? v.vifRegs->mask : 0;

	if (nVifBlock* b = v.vifBlocks->find(&_vBlock)) {
		if (u8* dest = dVifsetVUptr(v, cycle_cl, cycle_wl, isFill)) {
			//DevCon.WriteLn("Running Recompiled Block!");
			((nVifrecCall)b->startPtr)((uptr)dest, (uptr)data);
		}
		else {
			//DevCon.WriteLn("Running Interpreter Block");
			_nVifUnpack(idx, data, size, isFill);
		}
		return;
	}
	static int recBlockNum = 0;
	DevCon.WriteLn("nVif: Recompiled Block! [%d]", recBlockNum++);
	//DevCon.WriteLn(L"[num=% 3d][upkType=0x%02x][scl=%d][cl=%d][wl=%d][mode=%d][m=%d][mask=%s]",
	//	_vBlock.num, _vBlock.upkType, _vBlock.scl, _vBlock.cl, _vBlock.wl, _vBlock.mode,
	//	doMask >> 4, doMask ? wxsFormat( L"0x%08x", _vBlock.mask ).c_str() : L"ignored"
	//);

	xSetPtr(v.recPtr);
	_vBlock.startPtr = (uptr)xGetAlignedCallTarget();
	v.vifBlocks->add(_vBlock);
	VifUnpackSSE_Dynarec( v, _vBlock ).CompileRoutine();
	nVif[idx].recPtr = xGetPtr();

	dVifRecLimit(idx);
	
	// Run the block we just compiled.  Various conditions may force us to still use
	// the interpreter unpacker though, so a recursive call is the safest way here...
	dVifUnpack(idx, data, size, isFill);
}

#endif
