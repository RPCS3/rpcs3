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
#include "Common.h"
#include "Vif.h"
#include "Gif.h"
#include "Vif_Dma.h"

VIFregisters *vifRegs;
vifStruct	 *vif;
u16 vifqwc = 0;
int g_vifCycles = 0;

__aligned16 VifMaskTypes g_vifmask;

extern int g_vifCycles;

static __forceinline bool mfifoVIF1rbTransfer()
{
	u32 maddr = dmacRegs->rbor.ADDR;
	u32 msize = dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16;
	u16 mfifoqwc = std::min(vif1ch->qwc, vifqwc);
	u32 *src;
	bool ret;

	/* Check if the transfer should wrap around the ring buffer */
	if ((vif1ch->madr + (mfifoqwc << 4)) > (msize))
	{
		int s1 = ((msize) - vif1ch->madr) >> 2;

		SPR_LOG("Split MFIFO");

		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return false;

		if (vif1.vifstalled)
			ret = VIF1transfer(src + vif1.irqoffset, s1 - vif1.irqoffset, false);
		else
			ret = VIF1transfer(src, s1, false);

		if (ret)
		{
            /* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
            vif1ch->madr = maddr;

            src = (u32*)PSM(maddr);
            if (src == NULL) return false;
            VIF1transfer(src, ((mfifoqwc << 2) - s1), false);
		}
	}
	else
	{
		SPR_LOG("Direct MFIFO");

		/* it doesn't, so just transfer 'qwc*4' words */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return false;

		if (vif1.vifstalled)
			ret = VIF1transfer(src + vif1.irqoffset, mfifoqwc * 4 - vif1.irqoffset, false);
		else
			ret = VIF1transfer(src, mfifoqwc << 2, false);
	}
	return ret;
}

static __forceinline bool mfifo_VIF1chain()
{
    bool ret;

	/* Is QWC = 0? if so there is nothing to transfer */
	if ((vif1ch->qwc == 0) && (!vif1.vifstalled))
	{
		vif1.inprogress &= ~1;
		return true;
	}

	if (vif1ch->madr >= dmacRegs->rbor.ADDR &&
	        vif1ch->madr <= (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK))
	{
		u16 startqwc = vif1ch->qwc;
		ret = mfifoVIF1rbTransfer();
		vifqwc -= startqwc - vif1ch->qwc;
	}
	else
	{
		tDMA_TAG *pMem = dmaGetAddr(vif1ch->madr, !vif1ch->chcr.DIR);
		SPR_LOG("Non-MFIFO Location");

		if (pMem == NULL) return false;

		if (vif1.vifstalled)
			ret = VIF1transfer((u32*)pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset, false);
		else
			ret = VIF1transfer((u32*)pMem, vif1ch->qwc << 2, false);
	}
	return ret;
}

static u32 qwctag(u32 mask)
{
	return (dmacRegs->rbor.ADDR + (mask & dmacRegs->rbsr.RMSK));
}

void mfifoVIF1transfer(int qwc)
{
	tDMA_TAG *ptag;

	g_vifCycles = 0;

	if (qwc > 0)
	{
		vifqwc += qwc;
		SPR_LOG("Added %x qw to mfifo, total now %x - Vif CHCR %x Stalled %x done %x", qwc, vifqwc, vif1ch->chcr._u32, vif1.vifstalled, vif1.done);
		if (vif1.inprogress & 0x10)
		{
			if (vif1ch->madr >= dmacRegs->rbor.ADDR && vif1ch->madr <= (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK))
				CPU_INT(10, 0);
			else
				CPU_INT(10, vif1ch->qwc * BIAS);

			vif1Regs->stat.FQC = 0x10; // FQC=16
		}
		vif1.inprogress &= ~0x10;

		return;
	}

	if (vif1ch->qwc == 0 && vifqwc > 0)
	{
		ptag = dmaGetAddr(vif1ch->tadr, false);

		if (vif1ch->chcr.TTE)
		{
            bool ret;

			if (vif1.stallontag)
				ret = VIF1transfer((u32*)ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset, true);  //Transfer Tag on Stall
			else
				ret = VIF1transfer((u32*)ptag + 2, 2, true);  //Transfer Tag

			if (!(ret))
			{
				VIF_LOG("MFIFO Stall on tag");
				vif1.stallontag	= true;
				return;        //IRQ set by VIFTransfer
			}
		}

        vif1ch->unsafeTransfer(ptag);

		vif1ch->madr = ptag[1]._u32;
		vifqwc--;

		SPR_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
        ptag[1]._u32, ptag[0]._u32, vif1ch->qwc, ptag->ID, vif1ch->madr, vif1ch->tadr, vifqwc, spr0->madr);

		switch (ptag->ID)
		{
			case TAG_REFE: // Refe - Transfer Packet According to ADDR field
				vif1ch->tadr = qwctag(vif1ch->tadr + 16);
				vif1.done = true;										//End Transfer
				break;

			case TAG_CNT: // CNT - Transfer QWC following the tag.
				vif1ch->madr = qwctag(vif1ch->tadr + 16);						//Set MADR to QW after Tag
				vif1ch->tadr = qwctag(vif1ch->madr + (vif1ch->qwc << 4));			//Set TADR to QW following the data
				vif1.done = false;
				break;

			case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
			{
				int temp = vif1ch->madr;								//Temporarily Store ADDR
				vif1ch->madr = qwctag(vif1ch->tadr + 16); 					  //Set MADR to QW following the tag
				vif1ch->tadr = temp;								//Copy temporarily stored ADDR to Tag
				if ((temp & dmacRegs->rbsr.RMSK) != dmacRegs->rbor.ADDR) Console.WriteLn("Next tag = %x outside ring %x size %x", temp, psHu32(DMAC_RBOR), psHu32(DMAC_RBSR));
				vif1.done = false;
				break;
			}

			case TAG_REF: // Ref - Transfer QWC from ADDR field
			case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
				vif1ch->tadr = qwctag(vif1ch->tadr + 16);							//Set TADR to next tag
				vif1.done = false;
				break;

			case TAG_END: // End - Transfer QWC following the tag
				vif1ch->madr = qwctag(vif1ch->tadr + 16);		//Set MADR to data following the tag
				vif1ch->tadr = qwctag(vif1ch->madr + (vif1ch->qwc << 4));			//Set TADR to QW following the data
				vif1.done = true;										//End Transfer
				break;
		}

		if (vif1ch->chcr.TIE && ptag->IRQ)
		{
			VIF_LOG("dmaIrq Set");
			vif1.done = true;
		}
	}
	vif1Regs->stat.FQC = min(vif1ch->qwc, (u16)16);
	vif1.inprogress |= 1;

	SPR_LOG("mfifoVIF1transfer end %x madr %x, tadr %x vifqwc %x", vif1ch->chcr._u32, vif1ch->madr, vif1ch->tadr, vifqwc);
}

void vifMFIFOInterrupt()
{
	g_vifCycles = 0;
	VIF_LOG("vif mfifo interrupt");

	if (schedulepath3msk) Vif1MskPath3();

	if ((vif1Regs->stat.VGW))
	{
		if (GSTransferStatus.PTH3 < STOPPED_MODE || GSTransferStatus.PTH1 != STOPPED_MODE)
		{
			CPU_INT(10, 4);
			return;
		}
		else
		{
			vif1Regs->stat.VGW = false;
		}

	}

	if ((spr0->chcr.STR) && (spr0->qwc == 0))
	{
		spr0->chcr.STR = false;
		hwDmacIrq(DMAC_FROM_SPR);
	}

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat.INT = true;
		hwIntcIrq(INTC_VIF1);
		--vif1.irq;

		if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			vif1Regs->stat.FQC = 0; // FQC=0
			vif1ch->chcr.STR = false;
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
				//	Console.WriteLn("Empty 1");
					vifqwc = 0;
					vif1.inprogress |= 0x10;
					vif1Regs->stat.FQC = 0;
					hwDmacIrq(DMAC_MFIFO_EMPTY);
					return;
				}

                mfifoVIF1transfer(0);
                if ((vif1ch->madr >= dmacRegs->rbor.ADDR) && (vif1ch->madr <= (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK)))
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
		//Console.WriteLn("Empty 2");
		//vif1.inprogress |= 0x10;
		vif1Regs->stat.FQC = 0; // FQC=0
		hwDmacIrq(DMAC_MFIFO_EMPTY);
	}*/

	vif1.done = 1;
	g_vifCycles = 0;
	vif1ch->chcr.STR = false;
	hwDmacIrq(DMAC_VIF1);
	VIF_LOG("vif mfifo dma end");

	vif1Regs->stat.FQC = 0;
}
