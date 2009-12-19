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

#pragma once

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
}

_f void dVifRecLimit(int idx) {
	if (nVif[idx].recPtr > nVif[idx].recEnd) {
		nVif[idx].vifBlocks->clear();
		DevCon.WriteLn("nVif Rec - Out of Rec Cache! [%x > %x]", nVif[idx].recPtr, nVif[idx].recEnd);
	}
}

_f void dVifSetMasks(nVifStruct& v, int mask, int mode, int cS) {
	u32 m0 = v.vifBlock->mask;
	u32 m1 =  m0 & 0xaaaaaaaa;
	u32 m2 =(~m1>>1) &  m0;
	u32 m3 = (m1>>1) & ~m0;
	u32* row = (v.idx) ? g_vifmask.Row1 : g_vifmask.Row0;
	u32* col = (v.idx) ? g_vifmask.Col1 : g_vifmask.Col0;
	if((m2&&mask) || mode) { xMOVAPS(xmmRow, ptr32[row]); }
	if (m3&&mask) {
		xMOVAPS(xmmCol0, ptr32[col]); 
		if ((cS>=2) && (m3&0x0000ff00)) xPSHUF.D(xmmCol1, xmmCol0, _v1);
		if ((cS>=3) && (m3&0x00ff0000)) xPSHUF.D(xmmCol2, xmmCol0, _v2);
		if ((cS>=4) && (m3&0xff000000)) xPSHUF.D(xmmCol3, xmmCol0, _v3);
		if ((cS>=1) && (m3&0x000000ff)) xPSHUF.D(xmmCol0, xmmCol0, _v0);
	}
	//if (mask||mode) loadRowCol(v);
}

void dVifRecompile(nVifStruct& v, nVifBlock* vB) {
	const bool isFill		= (vB->cl < vB->wl);
	const int  usn			= (vB->upkType>>5)&1;
	const int  doMask		= (vB->upkType>>4)&1;
	const int  upkNum		=  vB->upkType & 0xf;
	const u32& vift			=  nVifT[upkNum];
	const int  doMode		=  vifRegs->mode & 3;
	const int  cycleSize	=  isFill ?  vifRegs->cycle.cl : vifRegs->cycle.wl;
	const int  blockSize	=  isFill ?  vifRegs->cycle.wl : vifRegs->cycle.cl;
	const int  skipSize		=  blockSize - cycleSize;
	const bool simpleBlock	= (vifRegs->num == 1);
	const int  backupCL		=  vif->cl;
	const int  backupNum	=  vifRegs->num;
	if (vif->cl >= blockSize)  vif->cl = 0;

	v.vifBlock = vB;
	xSetPtr(v.recPtr);
	xAlignPtr(16);
	vB->startPtr = xGetPtr();
	dVifSetMasks(v, doMask, doMode, cycleSize);
	
	while (vifRegs->num) {
		if (vif->cl < cycleSize) { 
			xUnpack[upkNum](&v, doMode<<1 | doMask);
			if (!simpleBlock) xADD(edx, vift);
			if (!simpleBlock) xADD(ecx, 16);
			vifRegs->num--;
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else if (isFill) {
			DevCon.WriteLn("filling mode!");
			xUnpack[upkNum](&v, 1);
			xADD(ecx, 16);
			vifRegs->num--;
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else {
			xADD(ecx, 16 * skipSize);
			vif->cl = 0;
		}
	}
	if (doMode==2) writeBackRow(v);
	xMOV(ptr32[&vif->cl],	   vif->cl);
	xMOV(ptr32[&vifRegs->num], vifRegs->num);
	xRET();
	v.recPtr	 = xGetPtr();
	vif->cl		 = backupCL;
	vifRegs->num = backupNum;
}

static nVifBlock _vBlock = {0};

_f u8* dVifsetVUptr(nVifStruct& v, int offset) {
	u8* ptr	   = (u8*)(v.VU->Mem + (offset & v.vuMemLimit));
	u8* endPtr = ptr + _vBlock.num * 16;
	if (endPtr > v.vuMemEnd) {
		DevCon.WriteLn("nVif - VU Mem Ptr Overflow!");
		ptr = NULL; // Fall Back to Interpreters which have wrap-around logic
	}
	return ptr;
}

void dVifUnpack(int idx, u8 *data, u32 size) {

	nVifStruct& v	  = nVif[idx];
	vif				  = v.vif;
	vifRegs			  = v.vifRegs;
	const u8 upkType  = vif->tag.cmd & 0x1f | ((!!(vif->usn)) << 5);

	_vBlock.upkType   = upkType;
	_vBlock.num		  = *(u8*)&vifRegs->num;
	_vBlock.mode	  = *(u8*)&vifRegs->mode;
	_vBlock.scl		  = vif->cl;
	_vBlock.cl		  = vifRegs->cycle.cl;
	_vBlock.wl		  = vifRegs->cycle.wl;
	_vBlock.mask	  = vifRegs->mask;

	if (nVifBlock* b = v.vifBlocks->find(&_vBlock)) {
		u8* dest = dVifsetVUptr(v, vif->tag.addr);
		if (!dest) {
			//DevCon.WriteLn("Running Interpreter Block");
			_nVifUnpack(idx, data, size);
		}
		else {
			//DevCon.WriteLn("Running Recompiled Block!");
			((nVifrecCall)b->startPtr)((uptr)dest, (uptr)data);
		}
		return;
	}
	static int recBlockNum = 0;
	DevCon.WriteLn("nVif: Recompiled Block! [%d]", recBlockNum++);
	nVifBlock* vB = new nVifBlock();
	memcpy(vB, &_vBlock, sizeof(nVifBlock));
	dVifRecompile(v, vB);
	v.vifBlocks->add(vB);
	dVifRecLimit(idx);
	dVifUnpack(idx, data, size);
}
