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
#include "Vif_Dma.h"
#include "GS.h"
#include "Gif_Unit.h"
#include "VUmicro.h"
#include "newVif.h"

u32 g_vif1Cycles = 0;

__fi void vif1FLUSH()
{
	if(vif1Regs.stat.VEW == true)
	{
		vif1.waitforvu = true;
		vif1.vifstalled.enabled = VifStallEnable(vif1ch);
		vif1.vifstalled.value = VIF_TIMING_BREAK;
	}
}

void vif1TransferToMemory()
{
	u128* pMem = (u128*)dmaGetAddr(vif1ch.madr, false);

	// VIF from gsMemory
	if (pMem == NULL) { // Is vif0ptag empty?
		Console.WriteLn("Vif1 Tag BUSERR");
		dmacRegs.stat.BEIS = true; // Bus Error
		vif1Regs.stat.FQC = 0;

		vif1ch.qwc = 0;
		vif1.done = true;
		CPU_INT(DMAC_VIF1, 0);
		return; // An error has occurred.
	}

	// MTGS concerns:  The MTGS is inherently disagreeable with the idea of downloading
	// stuff from the GS.  The *only* way to handle this case safely is to flush the GS
	// completely and execute the transfer there-after.
	//Console.Warning("Real QWC %x", vif1ch.qwc);
	const u32   size = min(vif1.GSLastDownloadSize, (u32)vif1ch.qwc);
	const u128* pMemEnd  = vif1.GSLastDownloadSize + pMem;
	
	if (size) {
		// Checking if any crazy game does a partial
		// gs primitive and then does a gs download...
		Gif_Path& p1 = gifUnit.gifPath[GIF_PATH_1];
		Gif_Path& p2 = gifUnit.gifPath[GIF_PATH_2];
		Gif_Path& p3 = gifUnit.gifPath[GIF_PATH_3];
		pxAssert(p1.isDone() || !p1.gifTag.isValid);
		pxAssert(p2.isDone() || !p2.gifTag.isValid);
		pxAssert(p3.isDone() || !p3.gifTag.isValid);
	}

	GetMTGS().SendPointerPacket(GS_RINGTYPE_READ_FIFO2, size, pMem);
	GetMTGS().WaitGS();
	pMem += size;

	if(pMem < pMemEnd) {
		//DevCon.Warning("GS Transfer < VIF QWC, Clearing end of space");
		
		__m128 zeroreg = _mm_setzero_ps();
		do {
			_mm_store_ps((float*)pMem, zeroreg);
		} while (++pMem < pMemEnd);
	}

	g_vif1Cycles += vif1ch.qwc * 2;
	vif1ch.madr += vif1ch.qwc * 16; // mgs3 scene changes
	if (vif1.GSLastDownloadSize >= vif1ch.qwc) {
		vif1.GSLastDownloadSize -= vif1ch.qwc;
		vif1Regs.stat.FQC = min((u32)16, vif1.GSLastDownloadSize);
	}
	else {
		vif1Regs.stat.FQC = 0;
		vif1.GSLastDownloadSize = 0;
	}

	vif1ch.qwc = 0;
}

bool _VIF1chain()
{
	u32 *pMem;

	if (vif1ch.qwc == 0)
	{
		vif1.inprogress &= ~1;
		vif1.irqoffset.value = 0;
		vif1.irqoffset.enabled = false;
		return true;
	}

	// Clarification - this is TO memory mode, for some reason i used the other way round >.<
	if (vif1.dmamode == VIF_NORMAL_TO_MEM_MODE)
	{
		vif1TransferToMemory();
		vif1.inprogress &= ~1;
		return true;
	}

	pMem = (u32*)dmaGetAddr(vif1ch.madr, !vif1ch.chcr.DIR);
	if (pMem == NULL)
	{
		vif1.cmd = 0;
		vif1.tag.size = 0;
		vif1ch.qwc = 0;
		return true;
	}

	VIF_LOG("VIF1chain size=%d, madr=%lx, tadr=%lx",
	        vif1ch.qwc, vif1ch.madr, vif1ch.tadr);

	if (vif1.irqoffset.enabled)
		return VIF1transfer(pMem + vif1.irqoffset.value, vif1ch.qwc * 4 - vif1.irqoffset.value, false);
	else
		return VIF1transfer(pMem, vif1ch.qwc * 4, false);
}

__fi void vif1SetupTransfer()
{
    tDMA_TAG *ptag;
	
	ptag = dmaGetAddr(vif1ch.tadr, false); //Set memory pointer to TADR

	if (!(vif1ch.transfer("Vif1 Tag", ptag))) return;

	vif1ch.madr = ptag[1]._u32;            //MADR = ADDR field + SPR
	g_vif1Cycles += 1; // Add 1 g_vifCycles from the QW read for the tag
	vif1.inprogress &= ~1;

	VIF_LOG("VIF1 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
			ptag[1]._u32, ptag[0]._u32, vif1ch.qwc, ptag->ID, vif1ch.madr, vif1ch.tadr);

	if (!vif1.done && ((dmacRegs.ctrl.STD == STD_VIF1) && (ptag->ID == TAG_REFS)))   // STD == VIF1
	{
		// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
		if ((vif1ch.madr + vif1ch.qwc * 16) >= dmacRegs.stadr.ADDR)
		{
			//DevCon.Warning("VIF1 DMA Stall");
			// stalled
			hwDmacIrq(DMAC_STALL_SIS);
			return;
		}
	}

			
			

	if (vif1ch.chcr.TTE)
	{
		// Transfer dma tag if tte is set

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
			//Some games (like killzone) do Tags mid unpack, the nops will just write blank data
			//to the VU's, which breaks stuff, this is where the 128bit packet will fail, so we ignore the first 2 words
			vif1.irqoffset.value = 2;
			vif1.irqoffset.enabled = true;
			ret = VIF1transfer((u32*)&masked_tag + 2, 2, true);  //Transfer Tag
			//ret = VIF1transfer((u32*)ptag + 2, 2);  //Transfer Tag
		}
				
		if (!ret && vif1.irqoffset.enabled)
		{
			vif1.inprogress &= ~1; //Better clear this so it has to do it again (Jak 1)
			return;        //IRQ set by VIFTransfer
		}
	}
	vif1.irqoffset.value = 0;
	vif1.irqoffset.enabled = false;

	vif1.done |= hwDmacSrcChainWithStack(vif1ch, ptag->ID);

	if(vif1ch.qwc > 0) vif1.inprogress |= 1;

	//Check TIE bit of CHCR and IRQ bit of tag
	if (vif1ch.chcr.TIE && ptag->IRQ)
	{
		VIF_LOG("dmaIrq Set");

        //End Transfer
		vif1.done = true;
		return;
	}
}

__fi void vif1VUFinish()
{
	if (VU0.VI[REG_VPU_STAT].UL & 0x100)
	{
		int _cycles = VU1.cycle;
		//DevCon.Warning("Finishing VU1");
		vu1Finish();
		CPU_INT(VIF_VU1_FINISH, (VU1.cycle - _cycles) * BIAS); 
		return;
	}

	vif1Regs.stat.VEW = false;
	VIF_LOG("VU1 finished");

	if( gifRegs.stat.APATH == 1 )
	{
		VIF_LOG("Clear APATH1");
		gifRegs.stat.APATH = 0;
		gifRegs.stat.OPH = 0;
		vif1Regs.stat.VGW = false; //Let vif continue if it's stuck on a flush

		if(!vif1.waitforvu) 
		{
			if(gifUnit.checkPaths(0,1,1)) gifUnit.Execute(false, true);
		}

	}
	if(vif1.waitforvu == true)
	{
		vif1.waitforvu = false;
		ExecuteVU(1);
		//Check if VIF is already scheduled to interrupt, if it's waiting, kick it :P
		if((cpuRegs.interrupt & (1<<DMAC_VIF1 | 1 << DMAC_MFIFO_VIF)) == 0 && vif1ch.chcr.STR == true && !vif1Regs.stat.INT)
		{
			if(dmacRegs.ctrl.MFD == MFD_VIF1)
				vifMFIFOInterrupt();
			else
				vif1Interrupt();
		}
	}
	
	//DevCon.Warning("VU1 state cleared");
}

__fi void vif1Interrupt()
{
	VIF_LOG("vif1Interrupt: %8.8x chcr %x, done %x, qwc %x", cpuRegs.cycle, vif1ch.chcr._u32, vif1.done, vif1ch.qwc);

	g_vif1Cycles = 0;

	if( gifRegs.stat.APATH == 2  && gifUnit.gifPath[GIF_PATH_2].isDone())
	{
		gifRegs.stat.APATH = 0;
		gifRegs.stat.OPH = 0;
		vif1Regs.stat.VGW = false; //Let vif continue if it's stuck on a flush

		if(gifUnit.checkPaths(1,0,1)) gifUnit.Execute(false, true);
	}
	//Some games (Fahrenheit being one) start vif first, let it loop through blankness while it sets MFIFO mode, so we need to check it here.
	if (dmacRegs.ctrl.MFD == MFD_VIF1) {
		//Console.WriteLn("VIFMFIFO\n");
		// Test changed because the Final Fantasy 12 opening somehow has the tag in *Undefined* mode, which is not in the documentation that I saw.
		if (vif1ch.chcr.MOD == NORMAL_MODE) Console.WriteLn("MFIFO mode is normal (which isn't normal here)! %x", vif1ch.chcr._u32);
		vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
		vifMFIFOInterrupt();
		return;
	}

	// We need to check the direction, if it is downloading
	// from the GS then we handle that separately (KH2 for testing)
	if (vif1ch.chcr.DIR) {
		bool isDirect   = (vif1.cmd & 0x7f) == 0x50;
		bool isDirectHL = (vif1.cmd & 0x7f) == 0x51;
		if((isDirect   && !gifUnit.CanDoPath2())
		|| (isDirectHL && !gifUnit.CanDoPath2HL())) {
			GUNIT_WARN("vif1Interrupt() - Waiting for Path 2 to be ready");
			CPU_INT(DMAC_VIF1, 128);
			if(gifRegs.stat.APATH == 3) vif1Regs.stat.VGW = 1; //We're waiting for path 3. Gunslinger II
			return;
		}
		vif1Regs.stat.VGW = 0; //Path 3 isn't busy so we don't need to wait for it.
		vif1Regs.stat.FQC = min(vif1ch.qwc, (u16)16);
		//Simulated GS transfer time done, clear the flags
	}
	
	if(vif1.waitforvu == true)
	{
		//DevCon.Warning("Waiting on VU1");
		//CPU_INT(DMAC_VIF1, 16);
		return;
	}
	if (!vif1ch.chcr.STR) Console.WriteLn("Vif1 running when CHCR == %x", vif1ch.chcr._u32);

	if (vif1.irq && vif1.tag.size == 0 &&vif1.cmd == 0)
	{
		VIF_LOG("VIF IRQ Firing");
		vif1Regs.stat.INT = true;
		
		//Yakuza watches VIF_STAT so lets do this here.
		if (((vif1Regs.code >> 24) & 0x7f) != 0x7) {
			vif1Regs.stat.VIS = true;
		}

		hwIntcIrq(VIF1intc);
		--vif1.irq;

		if (vif1Regs.stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			//vif1Regs.stat.FQC = 0;

			//NFSHPS stalls when the whole packet has gone across (it stalls in the last 32bit cmd)
			//In this case VIF will end
			vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);
			if((vif1ch.qwc > 0 || !vif1.done) && !CHECK_VIF1STALLHACK)	
			{
				VIF_LOG("VIF1 Stalled");
				return;
			}
		}
	}

	vif1.vifstalled.enabled = false;

	//Mirroring change to VIF0
	if (vif1.cmd) 
	{
		if (vif1.done && (vif1ch.qwc == 0)) vif1Regs.stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif1Regs.stat.VPS = VPS_IDLE;
	}
	
	if (vif1.inprogress & 0x1)
    {
            _VIF1chain();
            // VIF_NORMAL_FROM_MEM_MODE is a very slow operation.
            // Timesplitters 2 depends on this beeing a bit higher than 128.
            if (vif1ch.chcr.DIR) vif1Regs.stat.FQC = min(vif1ch.qwc, (u16)16);
		
			if(!(vif1Regs.stat.VGW && gifUnit.gifPath[GIF_PATH_3].state != GIF_PATH_IDLE)) //If we're waiting on GIF, stop looping, (can be over 1000 loops!)
				CPU_INT(DMAC_VIF1, g_vif1Cycles);
            return;
    }

    if (!vif1.done)
    {

            if (!(dmacRegs.ctrl.DMAE))
            {
                    Console.WriteLn("vif1 dma masked");
                    return;
            }

            if ((vif1.inprogress & 0x1) == 0) vif1SetupTransfer();
            if (vif1ch.chcr.DIR) vif1Regs.stat.FQC = min(vif1ch.qwc, (u16)16);

			if(!(vif1Regs.stat.VGW && gifUnit.gifPath[GIF_PATH_3].state != GIF_PATH_IDLE)) //If we're waiting on GIF, stop looping, (can be over 1000 loops!)
	            CPU_INT(DMAC_VIF1, g_vif1Cycles);
            return;
	}

	if (vif1.vifstalled.enabled && vif1.done)
	{
		DevCon.WriteLn("VIF1 looping on stall at end\n");
		CPU_INT(DMAC_VIF1, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif1ch.qwc > 0) Console.WriteLn("VIF1 Ending with %x QWC left", vif1ch.qwc);
	if (vif1.cmd != 0) Console.WriteLn("vif1.cmd still set %x tag size %x", vif1.cmd, vif1.tag.size);
#endif

	if((vif1ch.chcr.DIR == VIF_NORMAL_TO_MEM_MODE) && vif1.GSLastDownloadSize <= 16)
	{
		//Reverse fifo has finished and nothing is left, so lets clear the outputting flag
		gifRegs.stat.OPH = false;
	}

	if (vif1ch.chcr.DIR) vif1Regs.stat.FQC = min(vif1ch.qwc, (u16)16);

	vif1ch.chcr.STR = false;
	vif1.vifstalled.enabled = false;
	vif1.irqoffset.enabled = false;
	if(vif1.queued_program == true) vifExecQueue(1);
	g_vif1Cycles = 0;
	DMA_LOG("VIF1 DMA End");
	hwDmacIrq(DMAC_VIF1);

}

void dmaVIF1()
{
	VIF_LOG("dmaVIF1 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif1ch.chcr._u32, vif1ch.madr, vif1ch.qwc,
	        vif1ch.tadr, vif1ch.asr0, vif1ch.asr1);

	g_vif1Cycles = 0;

#ifdef PCSX2_DEVBUILD
	if (dmacRegs.ctrl.STD == STD_VIF1)
	{
		//DevCon.WriteLn("VIF Stall Control Source = %x, Drain = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3);
	}
#endif

	if (vif1ch.qwc > 0)   // Normal Mode
	{
		
		// ignore tag if it's a GS download (Def Jam Fight for NY)
		if(vif1ch.chcr.MOD == CHAIN_MODE && vif1ch.chcr.DIR) 
		{
			vif1.dmamode = VIF_CHAIN_MODE;
			//DevCon.Warning(L"VIF1 QWC on Chain CHCR " + vif1ch.chcr.desc());
			
			if ((vif1ch.chcr.tag().ID == TAG_REFE) || (vif1ch.chcr.tag().ID == TAG_END) || (vif1ch.chcr.tag().IRQ && vif1ch.chcr.TIE))
			{
				vif1.done = true;
			}
			else 
			{
				vif1.done = false;
			}
		}
		else //Assume normal mode for reverse FIFO and Normal.
		{
			if (dmacRegs.ctrl.STD == STD_VIF1)
				Console.WriteLn("DMA Stall Control on VIF1 normal not implemented - Report which game to PCSX2 Team");

			if (vif1ch.chcr.DIR)  // from Memory
				vif1.dmamode = VIF_NORMAL_FROM_MEM_MODE;
			else
				vif1.dmamode = VIF_NORMAL_TO_MEM_MODE;

			if(vif1.irqoffset.enabled == true && vif1.done == false) DevCon.Warning("Warning! VIF1 starting a Normal transfer with vif offset set (Possible force stop?)");
			vif1.done = true;
		}

		vif1.inprogress |= 1;
	}
	else
	{
		if(vif1.irqoffset.enabled == true && vif1.done == false) DevCon.Warning("Warning! VIF1 starting a new Chain transfer with vif offset set (Possible force stop?)");
		vif1.dmamode = VIF_CHAIN_MODE;
		vif1.done = false;
		vif1.inprogress &= ~0x1;
	}

	if (vif1ch.chcr.DIR) vif1Regs.stat.FQC = min((u16)0x10, vif1ch.qwc);

	// Chain Mode
	CPU_INT(DMAC_VIF1, 4);
}
