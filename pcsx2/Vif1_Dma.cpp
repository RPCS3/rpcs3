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
#include "Gif.h"
#include "VUmicro.h"
#include "newVif.h"


__forceinline void vif1FLUSH()
{
	if(g_packetsizeonvu > vif1.vifpacketsize && g_vu1Cycles > 0) 
	{
		//DevCon.Warning("Adding on same packet");
		if( ((g_packetsizeonvu - vif1.vifpacketsize) >> 1) > g_vu1Cycles)
			g_vu1Cycles -= (g_packetsizeonvu - vif1.vifpacketsize) >> 1;
		else g_vu1Cycles = 0;
	}
	if(g_vu1Cycles > 0)
	{
		//DevCon.Warning("Adding %x cycles to VIF1", g_vu1Cycles * BIAS);
		g_vifCycles += g_vu1Cycles;
		g_vu1Cycles = 0;
		
	} 
	g_vu1Cycles = 0;//else DevCon.Warning("VIF1 Different Packet, how can i work this out :/");
	if (VU0.VI[REG_VPU_STAT].UL & 0x100)
	{
		int _cycles = VU1.cycle;
		vu1Finish();
		//DevCon.Warning("VIF1 adding %x cycles", (VU1.cycle - _cycles) * BIAS);
		g_vifCycles += (VU1.cycle - _cycles) * BIAS;
	}
	if(gifRegs->stat.P1Q == true && (vif1.cmd & 0x7f) != 0x14 && (vif1.cmd & 0x7f) != 0x17)
	{
		vif1.vifstalled = true;
		vif1Regs->stat.VGW = true;
		vif1.GifWaitState = 2;
	}
	
}

void vif1TransferToMemory()
{
	u32 size;
	u64* pMem = (u64*)dmaGetAddr(vif1ch->madr, false);

	// VIF from gsMemory
	if (pMem == NULL)  						//Is vif0ptag empty?
	{
		Console.WriteLn("Vif1 Tag BUSERR");
		dmacRegs->stat.BEIS = true;      //Bus Error
		vif1Regs->stat.FQC = 0;

		vif1ch->qwc = 0;
		vif1.done = true;
		CPU_INT(DMAC_VIF1, 0);
		return;						   //An error has occurred.
	}

	// MTGS concerns:  The MTGS is inherently disagreeable with the idea of downloading
	// stuff from the GS.  The *only* way to handle this case safely is to flush the GS
	// completely and execute the transfer there-after.
	//Console.Warning("Real QWC %x", vif1ch->qwc);
    XMMRegisters::Freeze();

	size = min((u32)vif1ch->qwc, vif1.GSLastDownloadSize);

	if (GSreadFIFO2 == NULL)
	{
		for (;size > 0; --size)
		{
			GetMTGS().WaitGS();
			GSreadFIFO(&psHu64(VIF1_FIFO));

			pMem[0] = psHu64(VIF1_FIFO);
			pMem[1] = psHu64(VIF1_FIFO + 8);
			pMem += 2;
		}
		if(vif1ch->qwc > vif1.GSLastDownloadSize)
		{
			DevCon.Warning("GS Transfer < VIF QWC, Clearing end of space");
			for (size = vif1ch->qwc - vif1.GSLastDownloadSize; size > 0; --size)
			{
				psHu64(VIF1_FIFO) = 0;
				psHu64(VIF1_FIFO + 8) = 0;
				pMem[0] = psHu64(VIF1_FIFO);
				pMem[1] = psHu64(VIF1_FIFO + 8);
				pMem += 2;
			}
		}
	}
	else
	{
		GetMTGS().WaitGS();
		GSreadFIFO2(pMem, size);

		// set incase read
		psHu64(VIF1_FIFO) = pMem[2*size-2];
		psHu64(VIF1_FIFO + 8) = pMem[2*size-1];
		pMem += size * 2;
		if(vif1ch->qwc > vif1.GSLastDownloadSize)
		{
			DevCon.Warning("GS Transfer < VIF QWC, Clearing end of space");
			for (size = vif1ch->qwc - vif1.GSLastDownloadSize; size > 0; --size)
			{
				psHu64(VIF1_FIFO) = 0;
				psHu64(VIF1_FIFO + 8) = 0;
				pMem[0] = psHu64(VIF1_FIFO);
				pMem[1] = psHu64(VIF1_FIFO + 8);
				pMem += 2;
			}
		}
	}


    XMMRegisters::Thaw();

	g_vifCycles += vif1ch->qwc * 2;
	vif1ch->madr += vif1ch->qwc * 16; // mgs3 scene changes
	if(vif1.GSLastDownloadSize >= vif1ch->qwc)
	{
		vif1.GSLastDownloadSize -= vif1ch->qwc;
		vif1Regs->stat.FQC = min((u32)16, vif1.GSLastDownloadSize);
	}
	else
	{
		vif1Regs->stat.FQC = 0;
		vif1.GSLastDownloadSize = 0;
	}

	vif1ch->qwc = 0;
}

bool _VIF1chain()
{
	u32 *pMem;

	if (vif1ch->qwc == 0)
	{
		vif1.inprogress = 0;
		vif1.irqoffset = 0;
		return true;
	}

	// Clarification - this is TO memory mode, for some reason i used the other way round >.<
	if (vif1.dmamode == VIF_NORMAL_TO_MEM_MODE)
	{
		vif1TransferToMemory();
		vif1.inprogress = 0;
		return true;
	}

	pMem = (u32*)dmaGetAddr(vif1ch->madr, !vif1ch->chcr.DIR);
	if (pMem == NULL)
	{
		vif1.cmd = 0;
		vif1.tag.size = 0;
		vif1ch->qwc = 0;
		return true;
	}

	VIF_LOG("VIF1chain size=%d, madr=%lx, tadr=%lx",
	        vif1ch->qwc, vif1ch->madr, vif1ch->tadr);

	if (vif1.vifstalled)
		return VIF1transfer(pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset);
	else
		return VIF1transfer(pMem, vif1ch->qwc * 4);
}

__forceinline void vif1SetupTransfer()
{
    tDMA_TAG *ptag;

	switch (vif1.dmamode)
	{
		case VIF_NORMAL_TO_MEM_MODE:
		case VIF_NORMAL_FROM_MEM_MODE:
			vif1.inprogress = 1;
			vif1.done = true;
			g_vifCycles = 2;
			break;

		case VIF_CHAIN_MODE:
			ptag = dmaGetAddr(vif1ch->tadr, false); //Set memory pointer to TADR

			if (!(vif1ch->transfer("Vif1 Tag", ptag))) return;

			vif1ch->madr = ptag[1]._u32;            //MADR = ADDR field + SPR
			g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag

			// Transfer dma tag if tte is set

			VIF_LOG("VIF1 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			        ptag[1]._u32, ptag[0]._u32, vif1ch->qwc, ptag->ID, vif1ch->madr, vif1ch->tadr);

			if (!vif1.done && ((dmacRegs->ctrl.STD == STD_VIF1) && (ptag->ID == TAG_REFS)))   // STD == VIF1
			{
				// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
				if ((vif1ch->madr + vif1ch->qwc * 16) >= dmacRegs->stadr.ADDR)
				{
					// stalled
					hwDmacIrq(DMAC_STALL_SIS);
					return;
				}
			}

			vif1.inprogress = 1;

			if (vif1ch->chcr.TTE)
			{
			    bool ret;

				if (vif1.vifstalled)
					ret = VIF1transfer((u32*)ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset);  //Transfer Tag on stall
				else
					ret = VIF1transfer((u32*)ptag + 2, 2);  //Transfer Tag
				
				if ((ret == false) && vif1.irqoffset < 2)
				{
					vif1.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
					return;        //IRQ set by VIFTransfer
					
				} //else vif1.vifstalled = false;
			}

			vif1.irqoffset = 0;
			vif1.done |= hwDmacSrcChainWithStack(vif1ch, ptag->ID);

			//Check TIE bit of CHCR and IRQ bit of tag
			if (vif1ch->chcr.TIE && ptag->IRQ)
			{
				VIF_LOG("dmaIrq Set");

                //End Transfer
				vif1.done = true;
				return;
			}
			break;
	}
}

bool CheckPath2GIF(int channel)
{
	if ((vif1Regs->stat.VGW))
	{
		if( vif1.GifWaitState == 0 ) //DIRECT/HL Check
		{
			if(GSTransferStatus.PTH3 < IDLE_MODE || gifRegs->stat.P1Q == true)
			{
				if(gifRegs->stat.IMT && GSTransferStatus.PTH3 <= IMAGE_MODE && (vif1.cmd & 0x7f) == 0x50 && gifRegs->stat.P1Q == false)
				{
					vif1Regs->stat.VGW = false;
				}
				else
				{
					//DevCon.Warning("VIF1-0 stall P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs->stat.P1Q, gifRegs->stat.P2Q, gifRegs->stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
					CPU_INT(channel, 8);
					return false;
				}
			}
			else
			{
				vif1Regs->stat.VGW = false;
			}
		}
		else if( vif1.GifWaitState == 1 ) // Else we're flushing path3 :), but of course waiting for the microprogram to finish
		{
			if(gifRegs->stat.P1Q == true)
			{
				//DevCon.Warning("VIF1-1 stall P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs->stat.P1Q, gifRegs->stat.P2Q, gifRegs->stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
				CPU_INT(channel, 8);
				return false;
			}

			if (GSTransferStatus.PTH3 < PENDINGSTOP_MODE)
			{
				//DevCon.Warning("VIF1-11 stall P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs->stat.P1Q, gifRegs->stat.P2Q, gifRegs->stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
				//DevCon.Warning("PTH3 %x P1Q %x P3Q %x IP3 %x", GSTransferStatus.PTH3, gifRegs->stat.P1Q, gifRegs->stat.P3Q, gifRegs->stat.IP3 );
				CPU_INT(channel, 8);
				return false;
			}
			else
			{
				vif1Regs->stat.VGW = false;
			}
		}
		else if( vif1.GifWaitState == 3 ) // Else we're flushing path3 :), but of course waiting for the microprogram to finish
		{
			if (gifRegs->ctrl.PSE)
			{
				//DevCon.Warning("VIF1-1 stall P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs->stat.P1Q, gifRegs->stat.P2Q, gifRegs->stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
				CPU_INT(channel, 8);
				return false;
			}
			else
			{
				vif1Regs->stat.VGW = false;
			}
		}
		else //Normal Flush
		{
			if(gifRegs->stat.P1Q == true)
			{
				//DevCon.Warning("VIF1-2 stall P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs->stat.P1Q, gifRegs->stat.P2Q, gifRegs->stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
				CPU_INT(channel, 8);
				return false;
			}
			else
			{
				vif1Regs->stat.VGW = false;
			}
		}
	}
	return true;
}
__forceinline void vif1Interrupt()
{
	VIF_LOG("vif1Interrupt: %8.8x", cpuRegs.cycle);

	g_vifCycles = 0;

	if(GSTransferStatus.PTH2 == STOPPED_MODE && gifRegs->stat.APATH == GIF_APATH2)
	{
		gifRegs->stat.OPH = false;
		gifRegs->stat.APATH = GIF_APATH_IDLE;
		
	}

	if (schedulepath3msk & 0x10) 
	{
		Vif1MskPath3();
		CPU_INT(DMAC_VIF1, 8);
		return;
	}
	//Some games (Fahrenheit being one) start vif first, let it loop through blankness while it sets MFIFO mode, so we need to check it here.
	if (dmacRegs->ctrl.MFD == MFD_VIF1)   // VIF MFIFO
	{
		//Console.WriteLn("VIFMFIFO\n");
		// Test changed because the Final Fantasy 12 opening somehow has the tag in *Undefined* mode, which is not in the documentation that I saw.
		if (vif1ch->chcr.MOD == NORMAL_MODE) Console.WriteLn("MFIFO mode is normal (which isn't normal here)! %x", vif1ch->chcr._u32);
		vif1Regs->stat.FQC = min((u16)0x10, vif1ch->qwc);
		vifMFIFOInterrupt();
		return;
	}

	if(vif1ch->chcr.DIR && CheckPath2GIF(DMAC_VIF1) == false) return;
	//We need to check the direction, if it is downloading from the GS, we handle that seperately (KH2 for testing)
	if (vif1ch->chcr.DIR)vif1Regs->stat.FQC = min(vif1ch->qwc, (u16)16);
	//Simulated GS transfer time done, clear the flags
	
	

	

	if (!(vif1ch->chcr.STR)) Console.WriteLn("Vif1 running when CHCR == %x", vif1ch->chcr._u32);

	if (vif1.cmd) 
	{
		if(vif1.done == true && vif1ch->qwc == 0)	vif1Regs->stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif1Regs->stat.VPS = VPS_IDLE;
	}

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat.INT = true;
		hwIntcIrq(VIF1intc);
		--vif1.irq;
		if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			//vif1Regs->stat.FQC = 0;

			//NFSHPS stalls when the whole packet has gone across (it stalls int he last 32bit cmd)
			//In this case VIF will end
			if(vif1ch->qwc > 0 || !vif1.done)	return;
		}
	}

	if (vif1.inprogress & 0x1)
	{
		_VIF1chain();
		// VIF_NORMAL_FROM_MEM_MODE is a very slow operation.
		// Timesplitters 2 depends on this beeing a bit higher than 128.
		if (vif1ch->chcr.DIR) vif1Regs->stat.FQC = min(vif1ch->qwc, (u16)16);
		// Refraction - Removing voodoo timings for now, completely messes a lot of Path3 masked games.
		/*if (vif1.dmamode == VIF_NORMAL_FROM_MEM_MODE ) CPU_INT(DMAC_VIF1, 1024);
		else */CPU_INT(DMAC_VIF1, g_vifCycles /*VifCycleVoodoo*/);
		return;
	}

	if (!vif1.done)
	{

		if (!(dmacRegs->ctrl.DMAE))
		{
			Console.WriteLn("vif1 dma masked");
			return;
		}

		if ((vif1.inprogress & 0x1) == 0) vif1SetupTransfer();
		if (vif1ch->chcr.DIR) vif1Regs->stat.FQC = min(vif1ch->qwc, (u16)16);
		CPU_INT(DMAC_VIF1, g_vifCycles);
		return;
	}

	if (vif1.vifstalled && vif1.irq)
	{
		DevCon.WriteLn("VIF1 looping on stall\n");
		CPU_INT(DMAC_VIF1, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif1ch->qwc > 0) Console.WriteLn("VIF1 Ending with %x QWC left");
	if (vif1.cmd != 0) Console.WriteLn("vif1.cmd still set %x tag size %x", vif1.cmd, vif1.tag.size);
#endif

	
	if((vif1ch->chcr.DIR == VIF_NORMAL_TO_MEM_MODE) && vif1.GSLastDownloadSize <= 16) 
	{   //Reverse fifo has finished and nothing is left, so lets clear the outputting flag
		gifRegs->stat.OPH = false;
	}
	vif1ch->chcr.STR = false;
	vif1.vifstalled = false;
	g_vifCycles = 0;
	g_vu1Cycles = 0;
	VIF_LOG("VIF1 End");
	hwDmacIrq(DMAC_VIF1);

}

void dmaVIF1()
{
	VIF_LOG("dmaVIF1 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif1ch->chcr._u32, vif1ch->madr, vif1ch->qwc,
	        vif1ch->tadr, vif1ch->asr0, vif1ch->asr1);

	vif1.done = false;
	
	if(vif1.irqoffset != 0 && vif1.vifstalled == true) DevCon.Warning("Offset on VIF1 start!");
	vif1.irqoffset = 0;
	vif1.vifstalled = false;
	g_vifCycles = 0;
	g_vu1Cycles = 0;
	vif1.inprogress = 0;

#ifdef PCSX2_DEVBUILD
	if (dmacRegs->ctrl.STD == STD_VIF1)
	{
		//DevCon.WriteLn("VIF Stall Control Source = %x, Drain = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3);
	}
#endif

	if ((vif1ch->chcr.MOD == NORMAL_MODE) || vif1ch->qwc > 0)   // Normal Mode
	{

		if (dmacRegs->ctrl.STD == STD_VIF1)
			Console.WriteLn("DMA Stall Control on VIF1 normal");

		if (vif1ch->chcr.DIR)  // to Memory
			vif1.dmamode = VIF_NORMAL_FROM_MEM_MODE;
		else
			vif1.dmamode = VIF_NORMAL_TO_MEM_MODE;

		if(vif1ch->chcr.MOD == CHAIN_MODE && vif1ch->qwc > 0) 
		{
			vif1.dmamode = VIF_CHAIN_MODE;
			//DevCon.Warning(L"VIF1 QWC on Chain CHCR " + vif1ch->chcr.desc());
			vif1.inprogress |= 0x1;
			if(((vif1ch->chcr.TAG >> 12) & 0x7) == 0x0 || ((vif1ch->chcr.TAG >> 12) & 0x7) == 0x7)
			{
				vif1.done = true;
			}
		}
	}
	else
	{
		vif1.dmamode = VIF_CHAIN_MODE;
	}

	if (vif1ch->chcr.DIR) vif1Regs->stat.FQC = min((u16)0x10, vif1ch->qwc);

	// Chain Mode
	vif1Interrupt();
}
