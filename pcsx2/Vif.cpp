/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"
#include <cmath>
#include <assert.h>

#include "Common.h"
#include "VUmicro.h"
#include "Vif.h"
#include "VifDma.h"
#include "Tags.h"

VIFregisters *vifRegs;
u32* vifRow = NULL;
u32* vifMaskRegs = NULL;
vifStruct *vif;
u16 vifqwc = 0;

PCSX2_ALIGNED16(u32 g_vifRow0[4]);
PCSX2_ALIGNED16(u32 g_vifCol0[4]);
PCSX2_ALIGNED16(u32 g_vifRow1[4]);
PCSX2_ALIGNED16(u32 g_vifCol1[4]);

extern int g_vifCycles;

enum UnpackOffset
{
	OFFSET_X = 0,
	OFFSET_Y = 1,
	OFFSET_Z = 2,
	OFFSET_W = 3
};

__forceinline static int _limit(int a, int max)
{
	return (a > max) ? max : a;
}
	
static __releaseinline void writeXYZW(u32 offnum, u32 &dest, u32 data)
{
	int n;
	u32 vifRowReg = getVifRowRegs(offnum);
	
	if (vifRegs->code & 0x10000000)
	{
		switch (vif->cl)
		{
			case 0:
				if (offnum == OFFSET_X)
					n = (vifRegs->mask) & 0x3;
				else 
					n = (vifRegs->mask >> (offnum * 2)) & 0x3;
				break;
			case 1:
				n = (vifRegs->mask >> ( 8 + (offnum * 2))) & 0x3;
				break;
			case 2:
				n = (vifRegs->mask >> (16 + (offnum * 2))) & 0x3;
				break;
			default:
				n = (vifRegs->mask >> (24 + (offnum * 2))) & 0x3;
				break;
		}
	}
	else n = 0;

	switch (n)
	{
		case 0:
			if ((vif->cmd & 0x6F) == 0x6f)
			{
				dest = data;
			}
			else switch (vifRegs->mode)
			{
				case 1:
					dest = data + vifRowReg;
					break;
				case 2:
					// vifRowReg isn't used after this, or I would make it equal to dest here.
					dest = setVifRowRegs(offnum, vifRowReg + data);
					break;
				default:
					dest = data;
					break;
			}
			break;
		case 1:
			dest = vifRowReg;
			break;
		case 2:
			dest = getVifColRegs((vif->cl > 2) ? 3 : vif->cl);
			break;
		case 3:
			break;
	}
//	VIF_LOG("writeX %8.8x : Mode %d, r0 = %x, data %8.8x", *dest,vifRegs->mode,vifRegs->r0,data);
}

template <class T>
void __fastcall UNPACK_S(u32 *dest, T *data, int size)
{
	//S-# will always be a complete packet, no matter what. So we can skip the offset bits
	writeXYZW(OFFSET_X, *dest++, *data);
	writeXYZW(OFFSET_Y, *dest++, *data);
	writeXYZW(OFFSET_Z, *dest++, *data);
	writeXYZW(OFFSET_W, *dest  , *data);
}

template <class T>
void __fastcall UNPACK_V2(u32 *dest, T *data, int size)
{
	if (vifRegs->offset == OFFSET_X)
	{
		if (size > 0)
		{
			writeXYZW(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Y;
			size--;
		}
	}
	
	if (vifRegs->offset == OFFSET_Y) 
	{
		if (size > 0)
		{
			writeXYZW(vifRegs->offset, *dest++, *data);
			vifRegs->offset = OFFSET_Z;
			size--;
		}
	}
	
	if (vifRegs->offset == OFFSET_Z)
	{
		writeXYZW(vifRegs->offset, *dest++, *dest-2);
		vifRegs->offset = OFFSET_W;
	}
	
	if (vifRegs->offset == OFFSET_W)
	{
		writeXYZW(vifRegs->offset, *dest, *data);
		vifRegs->offset = OFFSET_X;
	}
}

template <class T>
void __fastcall UNPACK_V3(u32 *dest, T *data, int size)
{
	if(vifRegs->offset == OFFSET_X)
	{
		if (size > 0)
		{
			writeXYZW(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Y;
			size--;
		}
	}
	
	if(vifRegs->offset == OFFSET_Y) 
	{
		if (size > 0)
		{
			writeXYZW(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Z;
			size--;
		}
	}
	
	if(vifRegs->offset == OFFSET_Z)
	{
		if (size > 0)
		{
			writeXYZW(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_W;
			size--;
		}
	}
	
	if(vifRegs->offset == OFFSET_W)
	{
		//V3-# does some bizzare thing with alignment, every 6qw of data the W becomes 0 (strange console!)
		//Ape Escape doesnt seem to like it tho (what the hell?) gonna have to investigate
		writeXYZW(vifRegs->offset, *dest, *data);
		vifRegs->offset = OFFSET_X;
	}
}

template <class T>
void __fastcall UNPACK_V4(u32 *dest, T *data , int size)
{
	while (size > 0)
	{
		writeXYZW(vifRegs->offset, *dest++, *data++); 
		vifRegs->offset++;
		size--;
	}

	if (vifRegs->offset > OFFSET_W) vifRegs->offset = OFFSET_X;
}

void __fastcall UNPACK_V4_5(u32 *dest, u32 *data, int size)
{
	//As with S-#, this will always be a complete packet
	writeXYZW(OFFSET_X, *dest++,  ((*data & 0x001f) << 3));
	writeXYZW(OFFSET_Y, *dest++, ((*data & 0x03e0) >> 2));
	writeXYZW(OFFSET_Z, *dest++, ((*data & 0x7c00) >> 7));
	writeXYZW(OFFSET_W, *dest, ((*data & 0x8000) >> 8));
}

void __fastcall UNPACK_S_32(u32 *dest, u32 *data, int size) 
{
	UNPACK_S(dest, data, size); 
}

void __fastcall UNPACK_S_16s(u32 *dest, u32 *data, int size) 
{ 
	s16 *sdata = (s16*)data;  
	UNPACK_S(dest, sdata, size); 
}

void __fastcall UNPACK_S_16u(u32 *dest, u32 *data, int size) 
{
	u16 *sdata = (u16*)data; 
	UNPACK_S(dest, sdata, size);
}

void __fastcall UNPACK_S_8s(u32 *dest, u32 *data, int size)
{
	s8 *cdata = (s8*)data;
	UNPACK_S(dest, cdata, size);
}

void __fastcall UNPACK_S_8u(u32 *dest, u32 *data, int size)
{
	u8 *cdata = (u8*)data;
	UNPACK_S(dest, cdata, size);
}

void __fastcall UNPACK_V2_32(u32 *dest, u32 *data, int size)
{
	UNPACK_V2(dest, data, size);
}

void __fastcall UNPACK_V2_16s(u32 *dest, u32 *data, int size)
{
	s16 *sdata = (s16*)data;
	UNPACK_V2(dest, sdata, size);
}

void __fastcall UNPACK_V2_16u(u32 *dest, u32 *data, int size)
{
	u16 *sdata = (u16*)data;
	UNPACK_V2(dest, sdata, size);
}

void __fastcall UNPACK_V2_8s(u32 *dest, u32 *data, int size)
{
	s8 *cdata = (s8*)data;
	UNPACK_V2(dest, cdata, size);
}

void __fastcall UNPACK_V2_8u(u32 *dest, u32 *data, int size)
{
	u8 *cdata = (u8*)data;
	UNPACK_V2(dest, cdata, size);
}

void __fastcall UNPACK_V3_32(u32 *dest, u32 *data, int size)
{
	UNPACK_V3(dest, data, size);
}

void __fastcall UNPACK_V3_16s(u32 *dest, u32 *data, int size)
{
	s16 *sdata = (s16*)data;
	UNPACK_V3(dest, sdata, size);
}

void __fastcall UNPACK_V3_16u(u32 *dest, u32 *data, int size)
{
	u16 *sdata = (u16*)data;
	UNPACK_V3(dest, sdata, size);
}

void __fastcall UNPACK_V3_8s(u32 *dest, u32 *data, int size)
{
	s8 *cdata = (s8*)data;
	UNPACK_V3(dest, cdata, size);
}

void __fastcall UNPACK_V3_8u(u32 *dest, u32 *data, int size)
{
	u8 *cdata = (u8*)data;
	UNPACK_V3(dest, cdata, size);
}

void __fastcall UNPACK_V4_32(u32 *dest, u32 *data , int size)
{
	UNPACK_V4(dest, data, size);
}

void __fastcall UNPACK_V4_16s(u32 *dest, u32 *data, int size)
{
	s16 *sdata = (s16*)data;
	UNPACK_V4(dest, sdata, size);
}

void __fastcall UNPACK_V4_16u(u32 *dest, u32 *data, int size)
{
	u16 *sdata = (u16*)data;
	UNPACK_V4(dest, sdata, size);
}

void __fastcall UNPACK_V4_8s(u32 *dest, u32 *data, int size)
{
	s8 *cdata = (s8*)data;
	UNPACK_V4(dest, cdata, size);
}

void __fastcall UNPACK_V4_8u(u32 *dest, u32 *data, int size)
{
	u8 *cdata = (u8*)data;
	UNPACK_V4(dest, cdata, size);
}

static __forceinline int mfifoVIF1rbTransfer()
{
	u32 maddr = psHu32(DMAC_RBOR);
	u32 ret, msize = psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR) + 16;
	u16 mfifoqwc = std::min(vif1ch->qwc, vifqwc);
	u32 *src;

	/* Check if the transfer should wrap around the ring buffer */
	if ((vif1ch->madr + (mfifoqwc << 4)) > (msize))
	{
		int s1 = ((msize) - vif1ch->madr) >> 2;

		SPR_LOG("Split MFIFO");

		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		
		if (vif1.vifstalled)
			ret = VIF1transfer(src + vif1.irqoffset, s1 - vif1.irqoffset, 0);
		else
			ret = VIF1transfer(src, s1, 0);
		
		if (ret == -2) return ret;

		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
		vif1ch->madr = maddr;

		src = (u32*)PSM(maddr);
		if (src == NULL) return -1;
		ret = VIF1transfer(src, ((mfifoqwc << 2) - s1), 0);
	}
	else
	{
		SPR_LOG("Direct MFIFO");

		/* it doesn't, so just transfer 'qwc*4' words */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		
		if (vif1.vifstalled)
			ret = VIF1transfer(src + vif1.irqoffset, mfifoqwc * 4 - vif1.irqoffset, 0);
		else
			ret = VIF1transfer(src, mfifoqwc << 2, 0);
		
		if (ret == -2) return ret;
	}

	return ret;
}

static __forceinline int mfifo_VIF1chain()
{
	int ret;

	/* Is QWC = 0? if so there is nothing to transfer */
	if ((vif1ch->qwc == 0) && (!vif1.vifstalled))
	{
		vif1.inprogress &= ~1;
		return 0;
	}

	if (vif1ch->madr >= psHu32(DMAC_RBOR) &&
	        vif1ch->madr <= (psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)))
	{
		u16 startqwc = vif1ch->qwc;
		ret = mfifoVIF1rbTransfer();
		vifqwc -= startqwc - vif1ch->qwc;
	}
	else
	{
		u32 *pMem = (u32*)dmaGetAddr(vif1ch->madr);
		SPR_LOG("Non-MFIFO Location");

		if (pMem == NULL) return -1;
		if (vif1.vifstalled)
			ret = VIF1transfer(pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset, 0);
		else
			ret = VIF1transfer(pMem, vif1ch->qwc << 2, 0);
	}

	return ret;
}

void mfifoVIF1transfer(int qwc)
{
	u32 *ptag;
	int id;
	int ret;

	g_vifCycles = 0;

	if (qwc > 0)
	{
		vifqwc += qwc;
		SPR_LOG("Added %x qw to mfifo, total now %x - Vif CHCR %x Stalled %x done %x", qwc, vifqwc, vif1ch->chcr, vif1.vifstalled, vif1.done);
		if (vif1.inprogress & 0x10)
		{
			if (vif1ch->madr >= psHu32(DMAC_RBOR) && vif1ch->madr <= (psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)))
				CPU_INT(10, 0);
			else
				CPU_INT(10, vif1ch->qwc * BIAS);

			vif1Regs->stat |= 0x10000000; // FQC=16
		}
		vif1.inprogress &= ~0x10;
		
		return;
	}

	if (vif1ch->qwc == 0 && vifqwc > 0)
	{
		ptag = (u32*)dmaGetAddr(vif1ch->tadr);

		if (CHCR::TTE(vif1ch))
		{
			if (vif1.stallontag)
				ret = VIF1transfer(ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset, 1);  //Transfer Tag on Stall
			else 
				ret = VIF1transfer(ptag + 2, 2, 1);  //Transfer Tag
			
			if (ret == -2)
			{
				VIF_LOG("MFIFO Stallon tag");
				vif1.stallontag	= true;
				return;        //IRQ set by VIFTransfer
			}
		}
		
		Tag::UnsafeTransfer(vif1ch, ptag);

		SPR_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
		        ptag[1], ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr, vifqwc, spr0->madr);
		
		vif1ch->madr = ptag[1];
		id =Tag::Id(ptag);
		vifqwc--;

		switch (id)
		{
			case TAG_REFE: // Refe - Transfer Packet According to ADDR field
				vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));
				vif1.done = true;										//End Transfer
				break;

			case TAG_CNT: // CNT - Transfer QWC following the tag.
				vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));						//Set MADR to QW after Tag
				vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->madr + (vif1ch->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
				vif1.done = false;
				break;

			case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
			{
				int temp = vif1ch->madr;								//Temporarily Store ADDR
				vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR)); 					  //Set MADR to QW following the tag
				vif1ch->tadr = temp;								//Copy temporarily stored ADDR to Tag
				if ((temp & psHu32(DMAC_RBSR)) != psHu32(DMAC_RBOR)) Console::WriteLn("Next tag = %x outside ring %x size %x", params temp, psHu32(DMAC_RBOR), psHu32(DMAC_RBSR));
				vif1.done = false;
				break;
			}

			case TAG_REF: // Ref - Transfer QWC from ADDR field
			case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
				vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));							//Set TADR to next tag
				vif1.done = false;
				break;

			case TAG_END: // End - Transfer QWC following the tag
				vif1ch->madr = psHu32(DMAC_RBOR) + ((vif1ch->tadr + 16) & psHu32(DMAC_RBSR));		//Set MADR to data following the tag
				vif1ch->tadr = psHu32(DMAC_RBOR) + ((vif1ch->madr + (vif1ch->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
				vif1.done = true;										//End Transfer
				break;
		}

		if ((CHCR::TIE(vif1ch)) && (Tag::IRQ(ptag)))
		{
			VIF_LOG("dmaIrq Set");
			vif1.done = true;
		}
	}
	
	vif1.inprogress |= 1;
	
	SPR_LOG("mfifoVIF1transfer end %x madr %x, tadr %x vifqwc %x", vif1ch->chcr, vif1ch->madr, vif1ch->tadr, vifqwc);
}

void vifMFIFOInterrupt()
{
	g_vifCycles = 0;
	
	if (schedulepath3msk) Vif1MskPath3();

	if ((vif1Regs->stat & VIF1_STAT_VGW))
	{
		if (CHCR::STR(gif))
		{			
			CPU_INT(10, 16);
			return;
		} 
		else 
		{
			vif1Regs->stat &= ~VIF1_STAT_VGW;
		}
	
	}

	if ((CHCR::STR(spr0)) && (spr0->qwc == 0))
	{
		CHCR::clearSTR(spr0);
		hwDmacIrq(DMAC_FROM_SPR);
	}

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat |= VIF1_STAT_INT;
		hwIntcIrq(INTC_VIF1);
		--vif1.irq;
		if (vif1Regs->stat & (VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			vif1Regs->stat &= ~0x1F000000; // FQC=0
			CHCR::clearSTR(vif1ch);
			return;
		}
	}
	
	if (vif1.done == false || vif1ch->qwc)
	{
		

		switch(vif1.inprogress & 1)
		{
			case 0: //Set up transfer
				if (vif1ch->tadr == spr0->madr)
				{
				//	Console::WriteLn("Empty 1");
					vifqwc = 0;
					vif1.inprogress |= 0x10;
					vif1Regs->stat &= ~0x1F000000; // FQC=0
					hwDmacIrq(DMAC_14);
					return;
				}

				 mfifoVIF1transfer(0);
				 if (vif1ch->madr >= psHu32(DMAC_RBOR) && vif1ch->madr <= (psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)))
					CPU_INT(10, 0);
				else
					CPU_INT(10, vif1ch->qwc * BIAS);

				return;
			case 1: //Transfer data
				mfifo_VIF1chain();	
				CPU_INT(10, 0);
				return;
		}
		return;
	} 
	
	/*if (vifqwc <= 0)
	{
		//Console::WriteLn("Empty 2");
		//vif1.inprogress |= 0x10;
		vif1Regs->stat &= ~0x1F000000; // FQC=0
		hwDmacIrq(DMAC_14);
	}*/

	vif1.done = 1;
	g_vifCycles = 0;
	CHCR::clearSTR(vif1ch);
	hwDmacIrq(DMAC_VIF1);
	VIF_LOG("vif mfifo dma end");

	vif1Regs->stat &= ~0x1F000000; // FQC=0

}
