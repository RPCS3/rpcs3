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

// newVif! - author: cottonvibes(@gmail.com)

#pragma once

struct nVifStruct {
	u32				idx;		// VIF0 or VIF1
	vifStruct*		vif;		// Vif Struct ptr
	VIFregisters*	vifRegs;	// Vif Regs   ptr
	VURegs*			VU;			// VU  Regs   ptr
	u8*				vuMemEnd;   // End of VU Memory
	u32				vuMemLimit; // Use for fast AND
	BlockBuffer*	vifBlock;	// Block Buffer
};
nVifStruct nVif[2];

void initNewVif(int idx) {
	nVif[idx].idx		= idx;
	nVif[idx].VU		= idx ? &VU1     : &VU0;
	nVif[idx].vif		= idx ? &vif1    : &vif0;
	nVif[idx].vifRegs	= idx ? vif1Regs : vif0Regs;
	nVif[idx].vifBlock	= new BlockBuffer(0x2000); // 8kb Block Buffer
	nVif[idx].vuMemEnd  = idx ? ((u8*)(VU1.Mem + 0x4000)) : ((u8*)(VU0.Mem + 0x1000));
	nVif[idx].vuMemLimit= idx ? 0x3ff0 : 0xff0;
	memset_8<0xcc,sizeof(nVifUpk)>(nVifUpk);
	for (int a = 0; a < 2; a++) {
	for (int b = 0; b < 2; b++) {
	for (int c = 0; c < 4; c++) {
	for (int d = 0; d < 3; d++) {
		nVifGen(a, b, c, d); //nVifUpk[2][2][4][3][16];
	}}}}
}

int nVifUnpack(int idx, u32 *data) {
	XMMRegisters::Freeze();
	BlockBuffer* vB = nVif[idx].vifBlock;
	int			ret = aMin(vif1.vifpacketsize, vif1.tag.size);
	vif1.tag.size -= ret;
	_nVifUnpack(idx, (u8*)data, ret<<2);
	if (vif1.tag.size <= 0) vif1.tag.size = 0;
	if (vif1.tag.size <= 0) vif1.cmd      = 0;
	XMMRegisters::Thaw();
	return ret;
}

_f u8* setVUptr(int idx, int offset) {
	return (u8*)(nVif[idx].VU->Mem + (offset & nVif[idx].vuMemLimit));
}

_f void incVUptr(int idx, u8* &ptr, int amount) {
	ptr += amount;
	int diff = ptr - nVif[idx].vuMemEnd;
	if (diff >= 0) {
		ptr = nVif[idx].VU->Mem + diff;
	}
	if ((uptr)ptr & 0xf) DevCon.WriteLn("unaligned wtf :(");
}

_f void setMasks(VIFregisters* v) {
	for (int i = 0; i < 16; i++) {
		int m = (v->mask >> (i*2)) & 3;
		switch (m) {
			case 0: // Data
				nVifMask[0][i/4][i%4] = 0xffffffff;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = 0;
				break;
			case 1: // Row
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v->r0)[(i%4)*4];
				break;
			case 2: // Col
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v->c0)[(i/4)*4];
				break;
			case 3: // Write Protect
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0xffffffff;
				nVifMask[2][i/4][i%4] = 0;
				break;
		}
	}
}

_f void _nVifUnpack(int idx, u8 *data, u32 size) {
	/*if (nVif[idx].vifRegs->cycle.cl >= nVif[idx].vifRegs->cycle.wl) { // skipping write
		if (!idx) VIFunpack<0>((u32*)data, &vif0.tag, size>>2);
		else	  VIFunpack<1>((u32*)data, &vif1.tag, size>>2);
		return;
	}
	else*/ { // filling write
		vif        = nVif[idx].vif;
		vifRegs    = nVif[idx].vifRegs;
		int isFill = !!(vifRegs->cycle.cl < vifRegs->cycle.wl);
		int usn    = !!(vif->usn);
		int doMask = !!(vif->tag.cmd & 0x10);
		int upkNum = vif->tag.cmd & 0xf;
		int doMode = !!(vifRegs->mode);
		if (doMask) setMasks(vifRegs);
		
		//if (isFill)
		//DevCon.WriteLn("%s Write! [num = %d][%s]", (isFill?"Filling":"Skipping"), vifRegs->num, (vifRegs->num%3 ? "bad!" : "ok"));
		//DevCon.WriteLn("%s Write! [mask = %08x][type = %02d][num = %d]", (isFill?"Filling":"Skipping"), vifRegs->mask, upkNum, vifRegs->num);
		
		u8* dest					 = setVUptr(idx, vif->tag.addr);
		const VIFUnpackFuncTable* ft = &VIFfuncTable[vif->tag.cmd & 0xf];
		UNPACKFUNCTYPE func			 = vif->usn ? ft->funcU : ft->funcS;
		int cycleSize = isFill ? vifRegs->cycle.cl : vifRegs->cycle.wl;
		int blockSize = isFill ? vifRegs->cycle.wl : vifRegs->cycle.cl;
		//vif->cl = 0;
		while (vifRegs->num > 0) {
			if (vif->cl >= blockSize) {
				vif->cl  = 0;
			}
			if (vif->cl  < cycleSize) { 
				if (size <= 0) { DevCon.WriteLn("_nVifUnpack: Out of Data!"); break; }
				if (doMode /*|| doMask*/) {
					//if (doMask)
					//DevCon.WriteLn("Non SSE; unpackNum = %d", upkNum);
					func((u32*)dest, (u32*)data, ft->qsize);
					data += ft->gsize;
					size -= ft->gsize;
					vifRegs->num--;
				}
				else if (1) {
					//DevCon.WriteLn("SSE Unpack!");
					nVifUnpackF(dest, data, usn, doMask, aMin(vif->cl, 4), 0, upkNum); 
					data += nVifT[upkNum];
					size -= nVifT[upkNum];
					vifRegs->num--;
				}
				else {
					//DevCon.WriteLn("SSE Unpack!");
					int c = aMin((cycleSize - vif->cl), 3);
					int t = nVifT[upkNum];
					size -= t * c;
					//if (c>1)	  { DevCon.WriteLn("C > 1!"); }
					if (c<0||c>3) { DevCon.WriteLn("C wtf!"); }
					if (size < 0) { DevCon.WriteLn("Size Shit"); size+=t*c;c=1;size-=t*c;}
					nVifUnpackF(dest, data, usn, doMask, aMin(vif->cl, 4), c-1, upkNum); 
					data += t * c;
					vifRegs->num -= c;
				}
			}
			else if (isFill) {
				func((u32*)dest, (u32*)data, ft->qsize);
				vifRegs->num--;
			}
			incVUptr(idx, dest, 16);
			vif->cl = (vif->cl+1) % blockSize;
		}
	}
}

//int nVifUnpack(int idx, u32 *data) {
//	XMMRegisters::Freeze();
//	BlockBuffer* vB = nVif[idx].vifBlock;
//	int			ret = aMin(vif1.vifpacketsize, vif1.tag.size);
//	//vB->append(data, ret<<2);
//	vif1.tag.size -= ret;
//	//DevCon.WriteLn("2 [0x%x][%d][%d]", vif1.tag.addr, vB->getSize(), vif1.tag.size<<2);
//	//if (vif1.tag.size <= 0) {
//		//DevCon.WriteLn("3 [0x%x][%d][%d]", vif1.tag.addr, vB->getSize(), vif1.tag.size<<2);
//		//VIFunpack<1>(vB->getBlock(), &vif1.tag, vB->getSize()>>2);
//		//_nVifUnpack(idx, vB->getBlock(), vB->getSize());
//		_nVifUnpack(idx, (u8*)data, ret<<2);
//		if (vif1.tag.size <= 0) vif1.tag.size = 0;
//		if (vif1.tag.size <= 0) vif1.cmd      = 0;
//		//vB->clear();
//	//}
//	//else { vif1.tag.size+=ret; ret = -1; vB->clear(); }
//	XMMRegisters::Thaw();
//	return ret;
//}
