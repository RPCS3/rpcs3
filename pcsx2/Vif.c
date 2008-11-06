/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <math.h>
#include <string.h>

#include "Common.h"
#include "ix86/ix86.h"
#include "Vif.h"
#include "VUmicro.h"

#include <assert.h>

VIFregisters *_vifRegs;
u32* _vifMaskRegs = NULL;
PCSX2_ALIGNED16(u32 g_vifRow0[4]);
PCSX2_ALIGNED16(u32 g_vifCol0[4]);
PCSX2_ALIGNED16(u32 g_vifRow1[4]);
PCSX2_ALIGNED16(u32 g_vifCol1[4]);
u32* _vifRow = NULL, *_vifCol = NULL;

vifStruct *_vif;

static int n;
static int i;

__inline static int _limit( int a, int max ) 
{
	return ( a > max ? max : a );
}

#define _UNPACKpart( offnum, func )                         \
	if ( ( size > 0 ) && ( _vifRegs->offset == offnum ) ) { \
		func;                                               \
		size--;                                             \
		_vifRegs->offset++;                                  \
	}

#define _UNPACKpart_nosize( offnum, func )                \
	if ( ( _vifRegs->offset == offnum ) ) { \
		func;                                               \
		_vifRegs->offset++;                                  \
	}

static void writeX( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { 
		switch ( _vif->cl ) {
			case 0:  n = (_vifRegs->mask) & 0x3; break;
			case 1:  n = (_vifRegs->mask >> 8) & 0x3; break;
			case 2:  n = (_vifRegs->mask >> 16) & 0x3; break;
			default: n = (_vifRegs->mask >> 24) & 0x3; break;
		}
	} else n = 0;
	
	switch ( n ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r0;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r0 = data + _vifRegs->r0;
				*dest = _vifRegs->r0;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r0; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeX %8.8x : Mode %d, r0 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r0,data);
#endif*/
}

static void writeY( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { 
		switch ( _vif->cl ) {
			case 0:  n = (_vifRegs->mask >> 2) & 0x3; break;
			case 1:  n = (_vifRegs->mask >> 10) & 0x3; break;
			case 2:  n = (_vifRegs->mask >> 18) & 0x3; break;
			default: n = (_vifRegs->mask >> 26) & 0x3; break;
		}
	} else n = 0;
	
	switch ( n ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r1;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r1 = data + _vifRegs->r1;
				*dest = _vifRegs->r1;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r1; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeY %8.8x : Mode %d, r1 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r1,data);
#endif*/
}

static void writeZ( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { 
		switch ( _vif->cl ) {
			case 0:  n = (_vifRegs->mask >> 4) & 0x3; break;
			case 1:  n = (_vifRegs->mask >> 12) & 0x3; break;
			case 2:  n = (_vifRegs->mask >> 20) & 0x3; break;
			default: n = (_vifRegs->mask >> 28) & 0x3; break;
		}
	} else n = 0;
	
	switch ( n ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r2;
			} else 
			if (_vifRegs->mode == 2) {  
				_vifRegs->r2 = data + _vifRegs->r2;
				*dest = _vifRegs->r2;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r2; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeZ %8.8x : Mode %d, r2 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r2,data);
#endif*/
}

static void writeW( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { 
		switch ( _vif->cl ) {
			case 0:  n = (_vifRegs->mask >> 6) & 0x3; break;
			case 1:  n = (_vifRegs->mask >> 14) & 0x3; break;
			case 2:  n = (_vifRegs->mask >> 22) & 0x3; break;
			default: n = (_vifRegs->mask >> 30) & 0x3; break;
		}
	} else n = 0;
	
	switch ( n ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r3;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r3 = data + _vifRegs->r3;
				*dest = _vifRegs->r3;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r3; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeW %8.8x : Mode %d, r3 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r3,data);
#endif*/
}

void UNPACK_S_32(u32 *dest, u32 *data, int size) {
	_UNPACKpart(0, writeX(dest++, *data) );
	_UNPACKpart(1, writeY(dest++, *data) );
	_UNPACKpart(2, writeZ(dest++, *data) );
	_UNPACKpart(3, writeW(dest  , *data) );
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_S_16s( u32 *dest, u32 *data, int size) {
	s16 *sdata = (s16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata) ); 
	_UNPACKpart(1, writeY(dest++, *sdata) ); 
	_UNPACKpart(2, writeZ(dest++, *sdata) ); 
	_UNPACKpart(3, writeW(dest  , *sdata) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0; 
}

void UNPACK_S_16u( u32 *dest, u32 *data, int size) {
	u16 *sdata = (u16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata) ); 
	_UNPACKpart(1, writeY(dest++, *sdata) ); 
	_UNPACKpart(2, writeZ(dest++, *sdata) ); 
	_UNPACKpart(3, writeW(dest  , *sdata) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_S_8s(u32 *dest, u32 *data, int size) {
	s8 *cdata = (s8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata) ); 
	_UNPACKpart(1, writeY(dest++, *cdata) ); 
	_UNPACKpart(2, writeZ(dest++, *cdata) ); 
	_UNPACKpart(3, writeW(dest  , *cdata) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_S_8u(u32 *dest, u32 *data, int size) {
	u8 *cdata = (u8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata) ); 
	_UNPACKpart(1, writeY(dest++, *cdata) ); 
	_UNPACKpart(2, writeZ(dest++, *cdata) ); 
	_UNPACKpart(3, writeW(dest  , *cdata) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V2_32( u32 *dest, u32 *data, int size ) {
	_UNPACKpart(0, writeX(dest++, *data++));
	_UNPACKpart(1, writeY(dest++, *data--));
	_UNPACKpart_nosize(2, writeZ(dest++, *data));
	_UNPACKpart_nosize(3, writeW(dest  , 0));
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	
}

void UNPACK_V2_16s(u32 *dest, u32 *data, int size) {
	s16 *sdata = (s16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++)); 
	_UNPACKpart(1, writeY(dest++, *sdata--)); 
	_UNPACKpart_nosize(2,writeZ(dest++, *sdata++)); 
	_UNPACKpart_nosize(3,writeW(dest  , *sdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V2_16u(u32 *dest, u32 *data, int size) {
	u16 *sdata = (u16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++)); 
	_UNPACKpart(1, writeY(dest++, *sdata--)); 
	_UNPACKpart_nosize(2,writeZ(dest++, *sdata++)); 
	_UNPACKpart_nosize(3,writeW(dest  , *sdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V2_8s(u32 *dest, u32 *data, int size) {
	s8 *cdata = (s8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++)); 
	_UNPACKpart(1, writeY(dest++, *cdata--)); 
	_UNPACKpart_nosize(2,writeZ(dest++, *cdata++)); 
	_UNPACKpart_nosize(3,writeW(dest  , *cdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V2_8u(u32 *dest, u32 *data, int size) {
	u8 *cdata = (u8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++)); 
	_UNPACKpart(1, writeY(dest++, *cdata--)); 
	_UNPACKpart_nosize(2,writeZ(dest++, *cdata++)); 
	_UNPACKpart_nosize(3,writeW(dest  , *cdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V3_32(u32 *dest, u32 *data, int size) {
	_UNPACKpart(0, writeX(dest++, *data++); );
	_UNPACKpart(1, writeY(dest++, *data++); );
	_UNPACKpart(2, writeZ(dest++, *data++); );
	_UNPACKpart_nosize(3, writeW(dest, *data); );
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V3_16s(u32 *dest, u32 *data, int size) {
	s16 *sdata = (s16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++)); 
	_UNPACKpart(1, writeY(dest++, *sdata++)); 
	_UNPACKpart(2, writeZ(dest++, *sdata++)); 
	_UNPACKpart_nosize(3,writeW(dest, *sdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V3_16u(u32 *dest, u32 *data, int size) {
	u16 *sdata = (u16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++)); 
	_UNPACKpart(1, writeY(dest++, *sdata++)); 
	_UNPACKpart(2, writeZ(dest++, *sdata++)); 
	_UNPACKpart_nosize(3,writeW(dest, *sdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V3_8s(u32 *dest, u32 *data, int size) {
	s8 *cdata = (s8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++)); 
	_UNPACKpart(1, writeY(dest++, *cdata++)); 
	_UNPACKpart(2, writeZ(dest++, *cdata++)); 
	_UNPACKpart_nosize(3,writeW(dest, *cdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V3_8u(u32 *dest, u32 *data, int size) {
	u8 *cdata = (u8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++)); 
	_UNPACKpart(1, writeY(dest++, *cdata++)); 
	_UNPACKpart(2, writeZ(dest++, *cdata++)); 
	_UNPACKpart_nosize(3,writeW(dest, *cdata)); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V4_32( u32 *dest, u32 *data , int size) {
	_UNPACKpart(0, writeX(dest++, *data++) );
	_UNPACKpart(1, writeY(dest++, *data++) );
	_UNPACKpart(2, writeZ(dest++, *data++) );
	_UNPACKpart(3, writeW(dest  , *data  ) );
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

void UNPACK_V4_16s(u32 *dest, u32 *data, int size) {
	s16 *sdata = (s16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++) ); 
	_UNPACKpart(1, writeY(dest++, *sdata++) ); 
	_UNPACKpart(2, writeZ(dest++, *sdata++) ); 
	_UNPACKpart(3, writeW(dest  , *sdata  ) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0; 
}

void UNPACK_V4_16u(u32 *dest, u32 *data, int size) {
	u16 *sdata = (u16*)data;
	_UNPACKpart(0, writeX(dest++, *sdata++) ); 
	_UNPACKpart(1, writeY(dest++, *sdata++) ); 
	_UNPACKpart(2, writeZ(dest++, *sdata++) ); 
	_UNPACKpart(3, writeW(dest  , *sdata  ) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0; 
}

void UNPACK_V4_8s(u32 *dest, u32 *data, int size) {
	s8 *cdata = (s8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++) ); 
	_UNPACKpart(1, writeY(dest++, *cdata++) ); 
	_UNPACKpart(2, writeZ(dest++, *cdata++) ); 
	_UNPACKpart(3, writeW(dest  , *cdata  ) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0; 
}

void UNPACK_V4_8u(u32 *dest, u32 *data, int size) {
	u8 *cdata = (u8*)data;
	_UNPACKpart(0, writeX(dest++, *cdata++) ); 
	_UNPACKpart(1, writeY(dest++, *cdata++) ); 
	_UNPACKpart(2, writeZ(dest++, *cdata++) ); 
	_UNPACKpart(3, writeW(dest  , *cdata  ) ); 
	if (_vifRegs->offset == 4) _vifRegs->offset = 0; 
}

void UNPACK_V4_5(u32 *dest, u32 *data, int size) {
	
	_UNPACKpart(0, writeX(dest++, (*data & 0x001f) << 3); );
	_UNPACKpart(1, writeY(dest++, (*data & 0x03e0) >> 2); );
	_UNPACKpart(2, writeZ(dest++, (*data & 0x7c00) >> 7); );
	_UNPACKpart(3, writeW(dest  , (*data & 0x8000) >> 8); );
	if (_vifRegs->offset == 4) _vifRegs->offset = 0;
}

static int cycles;
extern int g_vifCycles;
int vifqwc = 0;
__inline int mfifoVIF1rbTransfer() {
	u32 maddr = psHu32(DMAC_RBOR);
	int msize = psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR) + 16, ret;
	int mfifoqwc = min(vif1ch->qwc, vifqwc);
	u32 *src;
	
	/* Check if the transfer should wrap around the ring buffer */
	if ((vif1ch->madr+(mfifoqwc << 4)) > (msize)) {
		int s1 = ((msize) - vif1ch->madr) >> 2;

#ifdef SPR_LOG
			SPR_LOG("Split MFIFO\n");
#endif
		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		if(vif1.vifstalled == 1){
			ret = VIF1transfer(src+vif1.irqoffset, s1-vif1.irqoffset, 0);
		}else
			ret = VIF1transfer(src, s1, 0); 
		if(ret == -2) return ret;
	
		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
		vif1ch->madr = maddr;
		
		src = (u32*)PSM(maddr);
		if (src == NULL) return -1;
		ret = VIF1transfer(src, ((mfifoqwc << 2) - s1), 0); 
	} else {
#ifdef SPR_LOG
			SPR_LOG("Direct MFIFO\n");
#endif
		/* it doesn't, so just transfer 'qwc*4' words */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		if(vif1.vifstalled == 1)
			ret = VIF1transfer(src+vif1.irqoffset, mfifoqwc*4-vif1.irqoffset, 0);
		else
			ret = VIF1transfer(src, mfifoqwc << 2, 0); 
		if(ret == -2) return ret;
	}
	

	return ret;
}

__inline int mfifoVIF1chain() {
	int ret;
	
	/* Is QWC = 0? if so there is nothing to transfer */
	if (vif1ch->qwc == 0 && vif1.vifstalled == 0) return 0;

	
	if (vif1ch->madr >= psHu32(DMAC_RBOR) &&
		vif1ch->madr <= (psHu32(DMAC_RBOR)+psHu32(DMAC_RBSR))) {
		u16 startqwc = vif1ch->qwc;
		ret = mfifoVIF1rbTransfer();
		vifqwc -= startqwc - vif1ch->qwc;
	} else {
		u32 *pMem = (u32*)dmaGetAddr(vif1ch->madr);
#ifdef SPR_LOG
			SPR_LOG("Non-MFIFO Location\n");
#endif
		if (pMem == NULL) return -1;
		if(vif1.vifstalled == 1){
			ret = VIF1transfer(pMem+vif1.irqoffset, vif1ch->qwc*4-vif1.irqoffset, 0);
		}else
			ret = VIF1transfer(pMem, vif1ch->qwc << 2, 0); 
	}

	return ret;
}

#define spr0 ((DMACh*)&PS2MEM_HW[0xD000])
static int tempqwc = 0;

void mfifoVIF1transfer(int qwc) {
	u32 *ptag;
	int id;
	int ret, temp;
	
	g_vifCycles = 0;

	if(qwc > 0){
		vifqwc += qwc;
#ifdef SPR_LOG
			SPR_LOG("Added %x qw to mfifo, total now %x\n", qwc, vifqwc);
#endif
		if((vif1ch->chcr & 0x100) == 0 || vif1.vifstalled == 1) return;
	}

	 if(vif1ch->qwc == 0){
			/*if(vif1ch->tadr == spr0->madr) {
		
	#ifdef PCSX2_DEVBUILD
				SysPrintf("vif mfifo tadr==madr but qwc = %d\n", vifqwc);	
	#endif	
				FreezeMMXRegs(0);
				FreezeXMMRegs(0);
				//hwDmacIrq(14);
				return;
			}*/
			
			ptag = (u32*)dmaGetAddr(vif1ch->tadr);

			if (vif1ch->chcr & 0x40) {
				if( vif1.stallontag == 1) ret = VIF1transfer(ptag+(2+vif1.irqoffset), 2-vif1.irqoffset, 1);  //Transfer Tag on Stall
				else ret = VIF1transfer(ptag+2, 2, 1);  //Transfer Tag
				if (ret == -2) {
#ifdef SPR_LOG
			VIF_LOG("MFIFO Stallon tag\n");
#endif
					vif1.stallontag	= 1;				
					INT(10,cycles+g_vifCycles);
					return;        //IRQ set by VIFTransfer
				} 
			}

			id        = (ptag[0] >> 28) & 0x7;
			vif1ch->qwc  = (ptag[0] & 0xffff);
			vif1ch->madr = ptag[1];
			cycles += 2;
			
			vif1ch->chcr = ( vif1ch->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );
			
#ifdef SPR_LOG
			SPR_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x\n",
					ptag[1], ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr, vifqwc, spr0->madr);
#endif	
			vifqwc--;
			
			switch (id) {
				case 0: // Refe - Transfer Packet According to ADDR field
					vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));
					vif1.done = 2;										//End Transfer
					break;

				case 1: // CNT - Transfer QWC following the tag.
					vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));						//Set MADR to QW after Tag            
					vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->madr + (vif1ch->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
					vif1.done = 0;
					break;

				case 2: // Next - Transfer QWC following tag. TADR = ADDR
					temp = vif1ch->madr;								//Temporarily Store ADDR
					vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR)); 					  //Set MADR to QW following the tag
					vif1ch->tadr = temp;								//Copy temporarily stored ADDR to Tag
					if((temp & psHu32(DMAC_RBSR)) != psHu32(DMAC_RBOR)) SysPrintf("Next tag = %x outside ring %x size %x\n", temp, psHu32(DMAC_RBOR), psHu32(DMAC_RBSR));
					vif1.done = 0;
					break;

				case 3: // Ref - Transfer QWC from ADDR field
				case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
					vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));							//Set TADR to next tag
					vif1.done = 0;
					break;

				case 7: // End - Transfer QWC following the tag
					vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));		//Set MADR to data following the tag
					vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->madr + (vif1ch->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
					vif1.done = 2;										//End Transfer
					break;
				}
			
			if ((vif1ch->chcr & 0x80) && (ptag[0] >> 31)) {
#ifdef SPR_LOG
			VIF_LOG("dmaIrq Set\n");
#endif
			vif1.done = 2;
		}
	 }
		ret = mfifoVIF1chain();
		if (ret == -1) {
			SysPrintf("VIF dmaChain error size=%d, madr=%lx, tadr=%lx\n",
					vif1ch->qwc, vif1ch->madr, vif1ch->tadr);
			vif1.done = 1;
			INT(10,g_vifCycles);
		}
		if(ret == -2){

#ifdef VIF_LOG
			VIF_LOG("MFIFO Stall\n");
#endif
			INT(10,g_vifCycles);
			return;
		}
		
	if(vif1.done == 2 && vif1ch->qwc == 0) vif1.done = 1;
	
	 INT(10,g_vifCycles);

#ifdef SPR_LOG
	SPR_LOG("mfifoVIF1transfer end %x madr %x, tadr %x vifqwc %x\n", vif1ch->chcr, vif1ch->madr, vif1ch->tadr, vifqwc);
#endif
}

void vifMFIFOInterrupt()
{
	
	if(vif1.irq && vif1.tag.size == 0) {
			vif1Regs->stat|= VIF1_STAT_INT;
			hwIntcIrq(5);
			--vif1.irq;
			if(vif1Regs->stat & (VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS))
				{
					vif1Regs->stat&= ~0x1F000000; // FQC=0
					vif1ch->chcr &= ~0x100;
					cpuRegs.interrupt &= ~(1 << 10);
					return;
				}		
		}

	if(vif1.done != 1) {
		if(vifqwc <= 0){
			//SysPrintf("Empty\n");
			hwDmacIrq(14);
			cpuRegs.interrupt &= ~(1 << 10); 
			return;
		} 
		mfifoVIF1transfer(0);
		return;
	}

	//if(vifqwc > 0)SysPrintf("VIF MFIFO ending with stuff in it %x\n", vifqwc);
	vifqwc = 0;
	vif1.done = 0;
	vif1ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF1);
#ifdef VIF_LOG
			VIF_LOG("vif mfifo dma end\n");
#endif
	vif1Regs->stat&= ~0x1F000000; // FQC=0
//	}
	cpuRegs.interrupt &= ~(1 << 10);
}
