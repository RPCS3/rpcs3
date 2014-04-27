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

#include "PrecompiledHeader.h"
#include "Common.h"
#include "Vif.h"
#include "Gif_Unit.h"
#include "Vif_Dma.h"

u16 vifqwc = 0;

static u32 qwctag(u32 mask)
{
	return (dmacRegs.rbor.ADDR + (mask & dmacRegs.rbsr.RMSK));
}

static u16 QWCinVIFMFIFO(u32 DrainADDR)
{
	u32 ret;
	

	SPR_LOG("VIF MFIFO Requesting %x QWC from the MFIFO Base %x MFIFO Top %x, SPR MADR %x Drain %x", vif1ch.qwc, dmacRegs.rbor.ADDR, dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16, spr0ch.madr, DrainADDR);
	//Calculate what we have in the fifo.
	if(DrainADDR <= spr0ch.madr)
	{
		//Drain is below the tadr, calculate the difference between them
		ret = (spr0ch.madr - DrainADDR) >> 4;
	}
	else
	{
		u32 limit = dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16;
		//Drain is higher than SPR so it has looped round, 
		//calculate from base to the SPR tag addr and what is left in the top of the ring
		ret = ((spr0ch.madr - dmacRegs.rbor.ADDR) + (limit - DrainADDR)) >> 4;
	}
	SPR_LOG("%x Available of the %x requested", ret, vif1ch.qwc);
	
	return ret;
}
static __fi bool mfifoVIF1rbTransfer()
{
	u32 maddr = dmacRegs.rbor.ADDR;
	u32 msize = dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16;
	u16 mfifoqwc = min(QWCinVIFMFIFO(vif1ch.madr), vif1ch.qwc);
	u32 *src;
	bool ret;

	if(mfifoqwc == 0) return true; //Cant do anything, lets forget it 

	/* Check if the transfer should wrap around the ring buffer */
	if ((vif1ch.madr + (mfifoqwc << 4)) > (msize))
	{
		int s1 = ((msize) - vif1ch.madr) >> 2;

		SPR_LOG("Split MFIFO");

		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		vif1ch.madr = qwctag(vif1ch.madr);

		src = (u32*)PSM(vif1ch.madr);
		if (src == NULL) return false;

		if (vif1.irqoffset.enabled)
			ret = VIF1transfer(src + vif1.irqoffset.value, s1 - vif1.irqoffset.value);
		else
			ret = VIF1transfer(src, s1);

		if (ret)
		{
			if(vif1.irqoffset.value != 0) DevCon.Warning("VIF1 MFIFO Offest != 0! vifoffset=%x", vif1.irqoffset.value);
            /* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
            vif1ch.madr = maddr;

            src = (u32*)PSM(maddr);
            if (src == NULL) return false;
            VIF1transfer(src, ((mfifoqwc << 2) - s1));
		}
		
	}
	else
	{
		SPR_LOG("Direct MFIFO");
		
		/* it doesn't, so just transfer 'qwc*4' words */
		src = (u32*)PSM(vif1ch.madr);
		if (src == NULL) return false;

		if (vif1.irqoffset.enabled)
			ret = VIF1transfer(src + vif1.irqoffset.value, mfifoqwc * 4 - vif1.irqoffset.value);
		else
			ret = VIF1transfer(src, mfifoqwc << 2);

	}
	return ret;
}

static __fi void mfifo_VIF1chain()
{
	/* Is QWC = 0? if so there is nothing to transfer */
	if ((vif1ch.qwc == 0))
	{
		vif1.inprogress &= ~1;
		return;
	}

	if (vif1ch.madr >= dmacRegs.rbor.ADDR &&
	        vif1ch.madr < (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16))
	{		
		//if(vif1ch.madr == (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16)) DevCon.Warning("Edge VIF1");
		
		vif1ch.madr = qwctag(vif1ch.madr);
		mfifoVIF1rbTransfer();
		vif1ch.tadr = qwctag(vif1ch.tadr);
		vif1ch.madr = qwctag(vif1ch.madr);
		if(QWCinVIFMFIFO(vif1ch.madr) == 0) vif1.inprogress |= 0x10;

		//vifqwc -= startqwc - vif1ch.qwc;
		
	}
	else
	{
		tDMA_TAG *pMem = dmaGetAddr(vif1ch.madr, !vif1ch.chcr.DIR);
		SPR_LOG("Non-MFIFO Location");

		//No need to exit on non-mfifo as it is indirect anyway, so it can be transferring this while spr refills the mfifo

		if (pMem == NULL) return;

		if (vif1.irqoffset.enabled)
			VIF1transfer((u32*)pMem + vif1.irqoffset.value, vif1ch.qwc * 4 - vif1.irqoffset.value);
		else
			VIF1transfer((u32*)pMem, vif1ch.qwc << 2);
	}
}

void mfifoVifMaskMem(int id)
{
	switch (id) {
		//These five transfer data following the tag, need to check its within the buffer (Front Mission 4)
		case TAG_CNT:
		case TAG_NEXT:
		case TAG_CALL: 
		case TAG_RET:
		case TAG_END:
			if(vif1ch.madr < dmacRegs.rbor.ADDR) //probably not needed but we will check anyway.
			{
				//DevCon.Warning("VIF MFIFO MADR below bottom of ring buffer, wrapping VIF MADR = %x Ring Bottom %x", vif1ch.madr, dmacRegs.rbor.ADDR);
				vif1ch.madr = qwctag(vif1ch.madr);
			}
			if(vif1ch.madr > (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK)) //Usual scenario is the tag is near the end (Front Mission 4)
			{
				//DevCon.Warning("VIF MFIFO MADR outside top of ring buffer, wrapping VIF MADR = %x Ring Top %x", vif1ch.madr, (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK)+16);
				vif1ch.madr = qwctag(vif1ch.madr);
			}
			break;
		default:
			//Do nothing as the MADR could be outside
			break;
	}
}

void mfifoVIF1transfer(int qwc)
{
	tDMA_TAG *ptag;

	g_vif1Cycles = 0;

	if (qwc > 0)
	{
		//vifqwc += qwc;
		SPR_LOG("Added %x qw to mfifo,Vif CHCR %x Stalled %x done %x", qwc, vif1ch.chcr._u32, vif1.vifstalled.enabled, vif1.done);
		if (vif1.inprogress & 0x10)
		{
			if(vif1ch.chcr.STR == true && !(cpuRegs.interrupt & (1<<DMAC_MFIFO_VIF)))
			{
				SPR_LOG("Data Added, Resuming");
				CPU_INT(DMAC_MFIFO_VIF, 16);
			}

			//Apparently this is bad, i guess so, the data is going to memory rather than the FIFO
			//vif1Regs.stat.FQC = 0x10; // FQC=16
		}
		vif1.inprogress &= ~0x10;

		return;
	}

	if (vif1ch.qwc == 0)
	{
		vif1ch.tadr = qwctag(vif1ch.tadr);
		ptag = dmaGetAddr(vif1ch.tadr, false);

		if (dmacRegs.ctrl.STD == STD_VIF1 && (ptag->ID == TAG_REFS))
		{
			Console.WriteLn("VIF MFIFO DMA Stall not implemented - Report which game to PCSX2 Team");
		}

		if (vif1ch.chcr.TTE)
		{
            bool ret;

			static __aligned16 u128 masked_tag;

			masked_tag._u64[0] = 0;
			masked_tag._u64[1] = *((u64*)ptag + 1);

			VIF_LOG("\tVIF1 SrcChain TTE=1, data = 0x%08x.%08x", masked_tag._u32[3], masked_tag._u32[2]);

			if (vif1.irqoffset.enabled)
			{
				ret = VIF1transfer((u32*)&masked_tag + vif1.irqoffset.value, 4 - vif1.irqoffset.value, true);  //Transfer Tag on stall
				//ret = VIF1transfer((u32*)ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset);  //Transfer Tag on stall
			}
			else
			{
				vif1.irqoffset.value = 2;
				vif1.irqoffset.enabled = true;
				ret = VIF1transfer((u32*)&masked_tag + 2, 2, true);  //Transfer Tag
				//ret = VIF1transfer((u32*)ptag + 2, 2);  //Transfer Tag
			}

			if (!ret && vif1.irqoffset.enabled)
			{
				vif1.inprogress &= ~1;
				return;        //IRQ set by VIFTransfer
				
			} //else vif1.vifstalled.enabled = false;
			g_vif1Cycles += 2;
		}
		
		vif1.irqoffset.value = 0;
		vif1.irqoffset.enabled = false;

        vif1ch.unsafeTransfer(ptag);

		vif1ch.madr = ptag[1]._u32;

		//vifqwc--;

		SPR_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
        ptag[1]._u32, ptag[0]._u32, vif1ch.qwc, ptag->ID, vif1ch.madr, vif1ch.tadr, vifqwc, spr0ch.madr);

		vif1.done |= hwDmacSrcChainWithStack(vif1ch, ptag->ID);

		mfifoVifMaskMem(ptag->ID);

		if (vif1ch.chcr.TIE && ptag->IRQ)
		{
			VIF_LOG("dmaIrq Set");
			vif1.done = true;
		}

		
		if(vif1ch.qwc > 0) 	vif1.inprogress |= 1;

		vif1ch.tadr = qwctag(vif1ch.tadr);

		if(QWCinVIFMFIFO(vif1ch.tadr) == 0) vif1.inprogress |= 0x10;
	}
	else
	{
		DevCon.Warning("Vif MFIFO QWC not 0 on tag");
	}
	

	SPR_LOG("mfifoVIF1transfer end %x madr %x, tadr %x vifqwc %x", vif1ch.chcr._u32, vif1ch.madr, vif1ch.tadr, vifqwc);
}

void vifMFIFOInterrupt()
{
	g_vif1Cycles = 0;
	VIF_LOG("vif mfifo interrupt");

	if( gifRegs.stat.APATH == 2  && gifUnit.gifPath[1].isDone())
	{
		gifRegs.stat.APATH = 0;
		gifRegs.stat.OPH = 0;

		if(gifUnit.checkPaths(1,0,1)) gifUnit.Execute(false, true);
	}

	if (dmacRegs.ctrl.MFD != MFD_VIF1) {
		DevCon.Warning("Not in VIF MFIFO mode! Stopping VIF MFIFO");
		return;
	}

	if (vif1ch.chcr.DIR) {
		bool isDirect   = (vif1.cmd & 0x7f) == 0x50;
		bool isDirectHL = (vif1.cmd & 0x7f) == 0x51;
		if((isDirect   && !gifUnit.CanDoPath2())
		|| (isDirectHL && !gifUnit.CanDoPath2HL())) {
			GUNIT_WARN("vifMFIFOInterrupt() - Waiting for Path 2 to be ready");
			CPU_INT(DMAC_MFIFO_VIF, 128);
			return;
		}
	}
	if(vif1.waitforvu == true)
	{
	//	DevCon.Warning("Waiting on VU1 MFIFO");
		//CPU_INT(DMAC_MFIFO_VIF, 16);
		return;
	}

	// We need to check the direction, if it is downloading from the GS,
	// we handle that separately (KH2 for testing)

	// Simulated GS transfer time done, clear the flags
	
	if (vif1.irq && vif1.tag.size == 0) {
		SPR_LOG("VIF MFIFO Code Interrupt detected");
		vif1Regs.stat.INT = true;

		if (((vif1Regs.code >> 24) & 0x7f) != 0x7) {
			vif1Regs.stat.VIS = true;
		}

		hwIntcIrq(INTC_VIF1);
		--vif1.irq;

		if (vif1Regs.stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS)) {
			//vif1Regs.stat.FQC = 0; // FQC=0
			//vif1ch.chcr.STR = false;
			vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
			if((vif1ch.qwc > 0 || !vif1.done) && !(vif1.inprogress & 0x10)) {
				VIF_LOG("VIF1 MFIFO Stalled");
				return;
			}
		}
	}

	//Mirroring change to VIF0
	if (vif1.cmd) {
		if (vif1.done && vif1ch.qwc == 0) vif1Regs.stat.VPS = VPS_WAITING;
	}
	else {
		vif1Regs.stat.VPS = VPS_IDLE;
	}

	if(vif1.inprogress & 0x10) {
		FireMFIFOEmpty();
		if(!(vif1.done && vif1ch.qwc == 0))return;
	}

	vif1.vifstalled.enabled = false;

	if (vif1.done == false || vif1ch.qwc) {
		switch(vif1.inprogress & 1) {
			case 0: //Set up transfer
				if (QWCinVIFMFIFO(vif1ch.tadr) == 0) {
					vif1.inprogress |= 0x10;
					CPU_INT(DMAC_MFIFO_VIF, 4 );	
					return;
				}
				
                mfifoVIF1transfer(0);
				vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
				
			case 1: //Transfer data
				if(vif1.inprogress & 0x1) //Just in case the tag breaks early (or something wierd happens)!
					mfifo_VIF1chain();
				//Sanity check! making sure we always have non-zero values
				if(!(vif1Regs.stat.VGW && gifUnit.gifPath[GIF_PATH_3].state != GIF_PATH_IDLE)) //If we're waiting on GIF, stop looping, (can be over 1000 loops!)
					CPU_INT(DMAC_MFIFO_VIF, (g_vif1Cycles == 0 ? 4 : g_vif1Cycles) );	

				vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
				return;
		}
		return;
	} 

	vif1.vifstalled.enabled = false;
	vif1.irqoffset.enabled = false;
	vif1.done = 1;
	g_vif1Cycles = 0;
	vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
	vif1ch.chcr.STR = false;
	hwDmacIrq(DMAC_VIF1);
	DMA_LOG("VIF1 MFIFO DMA End");

	vif1Regs.stat.FQC = 0;
}
