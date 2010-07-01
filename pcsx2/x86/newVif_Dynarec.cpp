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

// newVif Dynarec - Dynamically Recompiles Vif 'unpack' Packets
// authors: cottonvibes(@gmail.com)
//			Jake.Stine (@gmail.com)

#include "PrecompiledHeader.h"
#include "newVif_UnpackSSE.h"

static __aligned16 nVifBlock _vBlock = {0};

void dVifReset(int idx) {
	// If the VIF cache is greater than 12mb, then it's due for a complete reset back
	// down to a reasonable starting point of 4mb.
	if( nVif[idx].vifCache && (nVif[idx].vifCache->getAllocSize() > _1mb*12) )
		safe_delete(nVif[idx].vifCache);

	if( !nVif[idx].vifCache )
		nVif[idx].vifCache = new BlockBuffer(_1mb*4);
	else
		nVif[idx].vifCache->clear();

	if( !nVif[idx].vifBlocks )
		nVif[idx].vifBlocks = new HashBucket<_tParams>();
	else
		nVif[idx].vifBlocks->clear();

	nVif[idx].numBlocks =  0;

	nVif[idx].recPtr	=  nVif[idx].vifCache->getBlock();
	nVif[idx].recEnd	= &nVif[idx].recPtr[nVif[idx].vifCache->getAllocSize()-(_1mb/4)]; // .25mb Safe Zone
}

void dVifClose(int idx) {
	nVif[idx].numBlocks = 0;
	safe_delete(nVif[idx].vifCache);
	safe_delete(nVif[idx].vifBlocks);
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
	if((m2&&(doMask||isFill))||doMode) { xMOVAPS(xmmRow, ptr32[row]); }
	if (m3&&(doMask||isFill)) {
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

void VifUnpackSSE_Dynarec::CompileRoutine() {
	const int  upkNum	 = v.vif->cmd & 0xf;
	const u8&  vift		 = nVifT[upkNum];
	const int  cycleSize = isFill ? vB.cl : vB.wl;
	const int  blockSize = isFill ? vB.wl : vB.cl;
	const int  skipSize	 = blockSize - cycleSize;

	int vNum = v.vifRegs->num;
	vCL		 = v.vif->cl;
	doMode	 = upkNum == 0xf ? 0 : doMode;

	// Value passed determines # of col regs we need to load
	SetMasks(isFill ? blockSize : cycleSize);

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
			//DevCon.WriteLn("filling mode!");
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

		//We must do skips - 1 here else skip calculation adds an extra skip which can overflow
		//causing the emu to drop back to the interpreter (do not need to skip on last block write) - Refraction
		if(skipSize > 0) skips -= skipSize * 16;
		endPtr = ptr + skips;
	}
	else endPtr = ptr + (_vBlock.num * 16);
	if ( endPtr > v.vuMemEnd ) {
		//DevCon.WriteLn("nVif%x - VU Mem Ptr Overflow; falling back to interpreter. Start = %x End = %x num = %x, wl = %x, cl = %x", v.idx, v.vif->tag.addr, v.vif->tag.addr + (_vBlock.num * 16), _vBlock.num, wl, cl);
		ptr = NULL; // Fall Back to Interpreters which have wrap-around logic
	}
	return ptr;
}

// [TODO] :  Finish implementing support for VIF's growable recBlocks buffer.  Currently
//    it clears the buffer only.
static _f void dVifRecLimit(int idx) {
	if (nVif[idx].recPtr > nVif[idx].recEnd) {
		DevCon.WriteLn("nVif Rec - Out of Rec Cache! [%x > %x]", nVif[idx].recPtr, nVif[idx].recEnd);
		nVif[idx].vifBlocks->clear();
		nVif[idx].recPtr = nVif[idx].vifCache->getBlock();
		nVif[idx].recEnd = &nVif[idx].recPtr[nVif[idx].vifCache->getAllocSize()-(_1mb/4)]; // .25mb Safe Zone
	}
}

// Gcc complains about recursive functions being inlined.
void dVifUnpack(int idx, u8 *data, u32 size, bool isFill) {

	const nVifStruct& v		= nVif[idx];
	const u8	upkType		= v.vif->cmd & 0x1f | ((!!v.vif->usn) << 5);
	const int	doMask		= v.vif->cmd & 0x10;
	const int	cycle_cl	= v.vifRegs->cycle.cl;
	const int	cycle_wl	= v.vifRegs->cycle.wl;
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
	//DevCon.WriteLn("nVif%d: Recompiled Block! [%d]", idx, nVif[idx].numBlocks++);
	//DevCon.WriteLn(L"[num=% 3d][upkType=0x%02x][scl=%d][cl=%d][wl=%d][mode=%d][m=%d][mask=%s]",
	//	_vBlock.num, _vBlock.upkType, _vBlock.scl, _vBlock.cl, _vBlock.wl, _vBlock.mode,
	//	doMask >> 4, doMask ? wxsFormat( L"0x%08x", _vBlock.mask ).c_str() : L"ignored"
	//);

	xSetPtr(v.recPtr);
	_vBlock.startPtr = (uptr)xGetAlignedCallTarget();
	v.vifBlocks->add(_vBlock);
	VifUnpackSSE_Dynarec( v, _vBlock ).CompileRoutine();
	nVif[idx].recPtr = xGetPtr();

	// [TODO] : Ideally we should test recompile buffer limits prior to each instruction,
	//   which would be safer and more memory efficient than using an 0.25 meg recEnd marker.
	dVifRecLimit(idx);

	// Run the block we just compiled.  Various conditions may force us to still use
	// the interpreter unpacker though, so a recursive call is the safest way here...
	dVifUnpack(idx, data, size, isFill);
}
