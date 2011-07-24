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

void dVifReserve(int idx)
{
	if (!nVif[idx].recReserve)
		nVif[idx].recReserve = new RecompiledCodeReserve(pxsFmt(L"VIF%u Unpack Recompiler Cache", idx));

	nVif[idx].recReserve->Reserve( nVif[idx].recReserveSizeMB * _1mb, idx ? HostMemoryMap::VIF1rec : HostMemoryMap::VIF0rec );
}

void dVifReset(int idx) {

	pxAssertDev(nVif[idx].recReserve, "Dynamic VIF recompiler reserve must be created prior to VIF use or reset!");

	if (!nVif[idx].vifBlocks)
		nVif[idx].vifBlocks = new HashBucket<_tParams>();
	else
		nVif[idx].vifBlocks->clear();

	nVif[idx].recReserve->Reset();

	nVif[idx].numBlocks		=  0;
	nVif[idx].recWritePtr	= nVif[idx].recReserve->GetPtr();
}

void dVifClose(int idx) {
	nVif[idx].numBlocks = 0;
	if (nVif[idx].recReserve)
		nVif[idx].recReserve->Reset();

	safe_delete(nVif[idx].vifBlocks);
}

void dVifRelease(int idx) {
	dVifClose(idx);
	safe_delete(nVif[idx].recReserve);
}

VifUnpackSSE_Dynarec::VifUnpackSSE_Dynarec(const nVifStruct& vif_, const nVifBlock& vifBlock_)
	: v(vif_)
	, vB(vifBlock_)
{
	isFill		= (vB.cl < vB.wl);
	usn			= (vB.upkType>>5) & 1;
	doMask		= (vB.upkType>>4) & 1;
	doMode		= vB.mode & 3;
	vCL			= 0;
}

#define makeMergeMask(x) {									\
	x = ((x&0x40)>>6) | ((x&0x10)>>3) | (x&4) | ((x&1)<<3);	\
}

__fi void VifUnpackSSE_Dynarec::SetMasks(int cS) const {
	const vifStruct& vif = v.idx ? vif1 : vif0;

	u32 m0 = vB.mask;
	u32 m1 =  m0 & 0xaaaaaaaa;
	u32 m2 =(~m1>>1) &  m0;
	u32 m3 = (m1>>1) & ~m0;
	if((m2&&doMask)||doMode) { xMOVAPS(xmmRow, ptr128[&vif.MaskRow]); }
	if (m3&&doMask) {
		xMOVAPS(xmmCol0, ptr128[&vif.MaskCol]);
		if ((cS>=2) && (m3&0x0000ff00)) xPSHUF.D(xmmCol1, xmmCol0, _v1);
		if ((cS>=3) && (m3&0x00ff0000)) xPSHUF.D(xmmCol2, xmmCol0, _v2);
		if ((cS>=4) && (m3&0xff000000)) xPSHUF.D(xmmCol3, xmmCol0, _v3);
		if ((cS>=1) && (m3&0x000000ff)) xPSHUF.D(xmmCol0, xmmCol0, _v0);
	}
	//if (doMask||doMode) loadRowCol((nVifStruct&)v);
}

void VifUnpackSSE_Dynarec::doMaskWrite(const xRegisterSSE& regX) const {
	pxAssertDev(regX.Id <= 1, "Reg Overflow! XMM2 thru XMM6 are reserved for masking.");
	xRegisterSSE t  =  regX == xmm0 ? xmm1 : xmm0; // Get Temp Reg
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
	if (doMask&&m2) { mergeVectors(regX, xmmRow,						t, m2); } // Merge MaskRow
	if (doMask&&m3) { mergeVectors(regX, xRegisterSSE(xmmCol0.Id+cc),	t, m3); } // Merge MaskCol
	if (doMask&&m4) { mergeVectors(regX, xmmTemp,						t, m4); } // Merge Write Protect
	if (doMode) {
		u32 m5 = (~m1>>1) & ~m0;
		if (!doMask)  m5 = 0xf;
		else		  makeMergeMask(m5);
		if (m5 < 0xf) {
			xPXOR(xmmTemp, xmmTemp);
			mergeVectors(xmmTemp, xmmRow, t, m5);
			xPADD.D(regX, xmmTemp);
			if (doMode==2) mergeVectors(xmmRow, regX, t, m5);
		}
		else if (m5 == 0xf) {
			xPADD.D(regX, xmmRow);
			if (doMode==2) xMOVAPS(xmmRow, regX);
		}
	}
	xMOVAPS(ptr32[dstIndirect], regX);
}

void VifUnpackSSE_Dynarec::writeBackRow() const {
	xMOVAPS(ptr128[&((v.idx ? vif1 : vif0).MaskRow)], xmmRow);
	DevCon.WriteLn("nVif: writing back row reg! [doMode = 2]");
	// ToDo: Do we need to write back to vifregs.rX too!? :/
}

static void ShiftDisplacementWindow( xAddressVoid& addr, const xRegister32& modReg )
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
	const int  upkNum	 = vB.upkType & 0xf;
	const u8&  vift		 = nVifT[upkNum];
	const int  cycleSize = isFill ? vB.cl : vB.wl;
	const int  blockSize = isFill ? vB.wl : vB.cl;
	const int  skipSize	 = blockSize - cycleSize;
	
	uint vNum	= vB.num ? vB.num : 256;
	doMode		= (upkNum == 0xf) ? 0 : doMode;		// V4_5 has no mode feature.

	pxAssume(vCL == 0);

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
			//Filling doesn't need anything fancy, it's pretty much a normal write, just doesnt increment the source.
			//DevCon.WriteLn("filling mode!");
			xUnpack(upkNum);
			xMovDest();

			dstIndirect += 16;

			if( IsUnmaskedOp() ) {
				++destReg;
				++workReg;
			}

			vNum--;
			if (++vCL == blockSize) vCL = 0;
		}
		else {
			dstIndirect += (16 * skipSize);
			vCL = 0;
		}
	}

	if (doMode==2) writeBackRow();
	xRET();
}

_vifT static __fi u8* dVifsetVUptr(uint cl, uint wl, bool isFill) {
	vifStruct& vif			= GetVifX;
	const VURegs& VU		= vuRegs[idx];
	const uint vuMemLimit	= idx ? 0x4000 : 0x1000;

	u8* startmem = VU.Mem + (vif.tag.addr & (vuMemLimit-0x10));
	u8* endmem = VU.Mem + vuMemLimit;
	uint length = (_vBlock.num > 0) ? (_vBlock.num * 16) : 4096; // 0 = 256

	if (!isFill) {
		// Accounting for skipping mode: Subtract the last skip cycle, since the skipped part of the run
		// shouldn't count as wrapped data.  Otherwise, a trailing skip can cause the emu to drop back
		// to the interpreter. -- Refraction (test with MGS3)

		uint skipSize  = (cl - wl) * 16;
		uint blocks    = _vBlock.num / wl;
		length += (blocks-1) * skipSize;
	}

	if ( (startmem+length) <= endmem ) {
		return startmem;
	}
	//Console.WriteLn("nVif%x - VU Mem Ptr Overflow; falling back to interpreter. Start = %x End = %x num = %x, wl = %x, cl = %x", v.idx, vif.tag.addr, vif.tag.addr + (_vBlock.num * 16), _vBlock.num, wl, cl);
	return NULL; // Fall Back to Interpreters which have wrap-around logic
}

// [TODO] :  Finish implementing support for VIF's growable recBlocks buffer.  Currently
//    it clears the buffer only.
static __fi void dVifRecLimit(int idx) {
	if (nVif[idx].recWritePtr > (nVif[idx].recReserve->GetPtrEnd() - _256kb)) {
		DevCon.WriteLn(L"nVif Recompiler Cache Reset! [%s > %s]",
			pxsPtr(nVif[idx].recWritePtr), pxsPtr(nVif[idx].recReserve->GetPtrEnd())
		);
		nVif[idx].recReserve->Reset();
		nVif[idx].recWritePtr = nVif[idx].recReserve->GetPtr();
	}
}

_vifT static __fi bool dVifExecuteUnpack(const u8* data, bool isFill)
{
	const nVifStruct& v		= nVif[idx];
	VIFregisters& vifRegs	= vifXRegs;

	if (nVifBlock* b = v.vifBlocks->find(&_vBlock)) {
		if (u8* dest = dVifsetVUptr<idx>(vifRegs.cycle.cl, vifRegs.cycle.wl, isFill)) {
			//DevCon.WriteLn("Running Recompiled Block!");
			((nVifrecCall)b->startPtr)((uptr)dest, (uptr)data);
		}
		else {
			//DevCon.WriteLn("Running Interpreter Block");
			_nVifUnpack(idx, data, vifRegs.mode, isFill);
		}
		return true;
	}
	return false;
}

_vifT __fi void dVifUnpack(const u8* data, bool isFill) {

	const nVifStruct& v		= nVif[idx];
	vifStruct& vif			= GetVifX;
	VIFregisters& vifRegs	= vifXRegs;

	const u8	upkType		= (vif.cmd & 0x1f) | (vif.usn << 5);
	const int	doMask		= isFill? 1 : (vif.cmd & 0x10);

	_vBlock.upkType = upkType;
	_vBlock.num		= (u8&)vifRegs.num;
	_vBlock.mode	= (u8&)vifRegs.mode;
	_vBlock.cl		= vifRegs.cycle.cl;
	_vBlock.wl		= vifRegs.cycle.wl;

	// Zero out the mask parameter if it's unused -- games leave random junk
	// values here which cause false recblock cache misses.
	_vBlock.mask	= doMask ? vifRegs.mask : 0;

	//DevCon.WriteLn("nVif%d: Recompiled Block! [%d]", idx, nVif[idx].numBlocks++);
	//DevCon.WriteLn(L"[num=% 3d][upkType=0x%02x][scl=%d][cl=%d][wl=%d][mode=%d][m=%d][mask=%s]",
	//	_vBlock.num, _vBlock.upkType, _vBlock.scl, _vBlock.cl, _vBlock.wl, _vBlock.mode,
	//	doMask >> 4, doMask ? wxsFormat( L"0x%08x", _vBlock.mask ).c_str() : L"ignored"
	//);

	if (dVifExecuteUnpack<idx>(data, isFill)) return;

	xSetPtr(v.recWritePtr);
	_vBlock.startPtr = (uptr)xGetAlignedCallTarget();
	v.vifBlocks->add(_vBlock);
	VifUnpackSSE_Dynarec( v, _vBlock ).CompileRoutine();
	nVif[idx].recWritePtr = xGetPtr();

	// [TODO] : Ideally we should test recompile buffer limits prior to each instruction,
	//   which would be safer and more memory efficient than using an 0.25 meg recEnd marker.
	dVifRecLimit(idx);

	// Run the block we just compiled.  Various conditions may force us to still use
	// the interpreter unpacker though, so a recursive call is the safest way here...
	dVifExecuteUnpack<idx>(data, isFill);
}

template void dVifUnpack<0>(const u8* data, bool isFill);
template void dVifUnpack<1>(const u8* data, bool isFill);
