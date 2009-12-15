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

static __aligned16 nVifStruct nVif[2];

void initNewVif(int idx) {
	nVif[idx].idx		= idx;
	nVif[idx].VU		= idx ? &VU1     : &VU0;
	nVif[idx].vif		= idx ? &vif1    : &vif0;
	nVif[idx].vifRegs	= idx ? vif1Regs : vif0Regs;
	nVif[idx].vifBlock	= new BlockBuffer(0x2000); // 8kb Block Buffer
	nVif[idx].vuMemEnd  = idx ? ((u8*)(VU1.Mem + 0x4000)) : ((u8*)(VU0.Mem + 0x1000));
	nVif[idx].vuMemLimit= idx ? 0x3ff0 : 0xff0;

	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadWrite, false);
	memset8<0xcc>( nVifUpkExec );

	xSetPtr( nVifUpkExec );

	for (int a = 0; a < 2; a++) {
	for (int b = 0; b < 2; b++) {
	for (int c = 0; c < 4; c++) {
	for (int d = 0; d < 3; d++) {
		nVifGen(a, b, c, d);
	}}}}

	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadOnly, true);
}

int nVifUnpack(int idx, u32 *data) {
	XMMRegisters::Freeze();
	//BlockBuffer* vB = nVif[idx].vifBlock;
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

static void setMasks(const VIFregisters& v) {
	for (int i = 0; i < 16; i++) {
		int m = (v.mask >> (i*2)) & 3;
		switch (m) {
			case 0: // Data
				nVifMask[0][i/4][i%4] = 0xffffffff;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = 0;
				break;
			case 1: // Row
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v.r0)[(i%4)*4];
				break;
			case 2: // Col
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v.c0)[(i/4)*4];
				break;
			case 3: // Write Protect
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0xffffffff;
				nVifMask[2][i/4][i%4] = 0;
				break;
		}
	}
}

// ----------------------------------------------------------------------------
//  Unpacking Optimization notes:
// ----------------------------------------------------------------------------
//  Some games send a LOT of small packets.  This is a problem because the new VIF unpacker
//  has a lot of setup code to establish which unpack function to call.  The best way to
//  optimize this is to cache the unpack function's base (see fnbase below) and update it
//  when the variables it depends on are modified: writes to vif->tag.cmd and vif->usn.
//  Problem: vif->tag.cmd is modified a lot.  Like, constantly.  So won't work.
//
//  A secondary optimization would be adding special handlers for packets where vifRegs->num==1.
//  (which would remove the loop, simplify the incVUptr code, etc).  But checking for it has
//  to be simple enough that it doesn't offset the benefits (which I'm not sure is possible).
//   -- air


//template< int idx, bool doMode, bool isFill >
//__releaseinline void __fastcall _nVifUnpackLoop( u8 *data, u32 size )
__releaseinline void __fastcall _nVifUnpackLoop( int idx, u8 *data, u32 size )
{
	// comment out the following 2 lines to test templated version...
	const bool	doMode	= !!vifRegs->mode;
	const bool	isFill	= (vifRegs->cycle.cl < vifRegs->cycle.wl);

	const int	usn		= !!(vif->usn);
	const int	doMask	= !!(vif->tag.cmd & 0x10);
	const int	upkNum	= vif->tag.cmd & 0xf;
	const u32&	vift	= nVifT[upkNum];

	u8* dest					 = setVUptr(idx, vif->tag.addr);
	const VIFUnpackFuncTable& ft = VIFfuncTable[upkNum];
	UNPACKFUNCTYPE func			 = usn ? ft.funcU : ft.funcS;

	// Did a bunch of work to make it so I could optimize this index lookup to outside
	// the main loop but it was for naught -- too often the loop is only 1-2 iterations,
	// so this setup code ends up being slower (1 iter) or same speed (2 iters).
	const nVifCall*	fnbase = &nVifUpk[ ((usn*2*16) + (doMask*16) + (upkNum)) * (4*4) ];

	const int cycleSize = isFill ? vifRegs->cycle.cl : vifRegs->cycle.wl;
	const int blockSize = isFill ? vifRegs->cycle.wl : vifRegs->cycle.cl;

	if (doMask)
		setMasks(*vifRegs);

	if (vif->cl >= blockSize) {
	
		// This condition doesn't appear to ever occur, and really it never should.
		// Normally it wouldn't matter, but even simple setup code matters here (see 
		// optimization notes above) >_<

		vif->cl  = 0;
	}

	while (vifRegs->num > 0) {
		if (vif->cl  < cycleSize) { 
			//if (size <= 0) { DbgCon.WriteLn("_nVifUnpack: Out of Data!"); break; }
			if (doMode /*|| doMask*/) {
				//if (doMask)
				//DevCon.WriteLn("Non SSE; unpackNum = %d", upkNum);
				func((u32*)dest, (u32*)data, ft.qsize);
				data += ft.gsize;
				size -= ft.gsize;
				vifRegs->num--;
			}
			else if (1) {
				//DevCon.WriteLn("SSE Unpack!");
				fnbase[aMin(vif->cl, 4) * 4](dest, data);
				data += vift;
				size -= vift;
				vifRegs->num--;
			}
			else {
				//DevCon.WriteLn("SSE Unpack!");
				int c = aMin((cycleSize - vif->cl), 3);
				size -= vift * c;
				//if (c>1)	  { DevCon.WriteLn("C > 1!"); }
				if (c<0||c>3) { DbgCon.WriteLn("C wtf!"); }
				if (size < 0) { DbgCon.WriteLn("Size Shit"); size+=vift*c;c=1;size-=vift*c;}
				fnbase[(aMin(vif->cl, 4) * 4) + c-1](dest, data);
				data += vift * c;
				vifRegs->num -= c;
			}
		}
		else if (isFill) {
			func((u32*)dest, (u32*)data, ft.qsize);
			vifRegs->num--;
		}
		incVUptr(idx, dest, 16);
		
		// Removing this modulo was a huge speedup for God of War start menu. (62->73 fps)
		// (GoW and tri-ace games both use a lot of blockSize==1 packets, resulting in tons
		//  of loops -- so the biggest factor in performance ends up being the top-level
		//  conditionals of the loop, and also the loop prep code.) --air

		//vif->cl = (vif->cl+1) % blockSize;
		if( ++vif->cl == blockSize ) vif->cl = 0;
	}
}

void _nVifUnpack(int idx, u8 *data, u32 size) {
	/*if (nVif[idx].vifRegs->cycle.cl >= nVif[idx].vifRegs->cycle.wl) { // skipping write
		if (!idx) VIFunpack<0>((u32*)data, &vif0.tag, size>>2);
		else	  VIFunpack<1>((u32*)data, &vif1.tag, size>>2);
		return;
	}
	else*/ { // filling write

		vif        = nVif[idx].vif;
		vifRegs    = nVif[idx].vifRegs;

#if 1
		_nVifUnpackLoop( idx, data, size );
#else		
		// Eh... template attempt, tho it didn't help much.  There's too much setup code,
		// and the template only optimizes code inside the loop, which often times seems to
		// only be run once or twice anyway.  Better to use recompilation than templating
		// anyway, but I'll leave it in for now for reference. -- air

		const bool	doMode	= !!vifRegs->mode;
		const bool	isFill	= (vifRegs->cycle.cl < vifRegs->cycle.wl);

		//UnpackLoopTable[idx][doMode][isFill]( data, size );

		if( idx )
		{
			if( doMode )
			{
				if( isFill )
					_nVifUnpackLoop<1,true,true>( data, size );
				else
					_nVifUnpackLoop<1,true,false>( data, size );
			}
			else
			{
				if( isFill )
					_nVifUnpackLoop<1,false,true>( data, size );
				else
					_nVifUnpackLoop<1,false,false>( data, size );
			}
		}
		else
		{
			pxFailDev( "No VIF0 support yet, sorry!" );
		}
#endif
		//if (isFill)
		//DevCon.WriteLn("%s Write! [num = %d][%s]", (isFill?"Filling":"Skipping"), vifRegs->num, (vifRegs->num%3 ? "bad!" : "ok"));
		//DevCon.WriteLn("%s Write! [mask = %08x][type = %02d][num = %d]", (isFill?"Filling":"Skipping"), vifRegs->mask, upkNum, vifRegs->num);
		
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
