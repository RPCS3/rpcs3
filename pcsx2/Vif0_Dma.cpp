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
#include "VUmicro.h"
#include "newVif.h"

u32 g_vif0Cycles = 0;

// Run VU0 until finish, don't add cycles to EE
// because its vif stalling not the EE core...
__fi void vif0FLUSH()
{
	if(vif0Regs.stat.VEW == true)
	{
		vif0.waitforvu = true;
		vif0.vifstalled.enabled = VifStallEnable(vif0ch);
		vif0.vifstalled.value = VIF_TIMING_BREAK;
	}
	return;
}

bool _VIF0chain()
{
	u32 *pMem;

	if (vif0ch.qwc == 0)
	{
		vif0.inprogress = 0;
		return true;
	}

	pMem = (u32*)dmaGetAddr(vif0ch.madr, false);
	if (pMem == NULL)
	{
		vif0.cmd = 0;
		vif0.tag.size = 0;
		vif0ch.qwc = 0;
		return true;
	}

	VIF_LOG("VIF0chain size=%d, madr=%lx, tadr=%lx",
	        vif0ch.qwc, vif0ch.madr, vif0ch.tadr);

	if (vif0.irqoffset.enabled)
		return VIF0transfer(pMem + vif0.irqoffset.value, vif0ch.qwc * 4 - vif0.irqoffset.value);
	else
		return VIF0transfer(pMem, vif0ch.qwc * 4);
}

__fi void vif0SetupTransfer()
{
    tDMA_TAG *ptag;

	ptag = dmaGetAddr(vif0ch.tadr, false); //Set memory pointer to TADR

	if (!(vif0ch.transfer("vif0 Tag", ptag))) return;

	vif0ch.madr = ptag[1]._u32;            //MADR = ADDR field + SPR
	g_vif0Cycles += 1; // Add 1 g_vifCycles from the QW read for the tag

	// Transfer dma tag if tte is set

	VIF_LOG("vif0 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
			ptag[1]._u32, ptag[0]._u32, vif0ch.qwc, ptag->ID, vif0ch.madr, vif0ch.tadr);

	vif0.inprogress = 0;

	if (vif0ch.chcr.TTE)
	{
		// Transfer dma tag if tte is set

		bool ret;

		static __aligned16 u128 masked_tag;

		masked_tag._u64[0] = 0;
		masked_tag._u64[1] = *((u64*)ptag + 1);

		VIF_LOG("\tVIF0 SrcChain TTE=1, data = 0x%08x.%08x", masked_tag._u32[3], masked_tag._u32[2]);

		if (vif0.irqoffset.enabled)
		{
			ret = VIF0transfer((u32*)&masked_tag + vif0.irqoffset.value, 4 - vif0.irqoffset.value, true);  //Transfer Tag on stall
			//ret = VIF0transfer((u32*)ptag + (2 + vif0.irqoffset), 2 - vif0.irqoffset);  //Transfer Tag on stall
		}
		else
		{
			//Some games (like killzone) do Tags mid unpack, the nops will just write blank data
			//to the VU's, which breaks stuff, this is where the 128bit packet will fail, so we ignore the first 2 words
			vif0.irqoffset.value = 2;
			vif0.irqoffset.enabled = true;
			ret = VIF0transfer((u32*)&masked_tag + 2, 2, true);  //Transfer Tag
			//ret = VIF0transfer((u32*)ptag + 2, 2);  //Transfer Tag
		}
				
		if (!ret && vif0.irqoffset.enabled)
		{
			vif0.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
			return;        //IRQ set by VIFTransfer
					
		}
	}

	vif0.irqoffset.value = 0;
	vif0.irqoffset.enabled = false;
	vif0.done |= hwDmacSrcChainWithStack(vif0ch, ptag->ID);

	if(vif0ch.qwc > 0) vif0.inprogress = 1;
	//Check TIE bit of CHCR and IRQ bit of tag
	if (vif0ch.chcr.TIE && ptag->IRQ)
	{
		VIF_LOG("dmaIrq Set");

        //End Transfer
		vif0.done = true;
		return;
	}
}

__fi void vif0VUFinish()
{
	if ((VU0.VI[REG_VPU_STAT].UL & 1))
	{
		int _cycles = VU0.cycle;
		//DevCon.Warning("Finishing VU0");
		vu0Finish();
		_cycles = VU0.cycle - _cycles;
		//DevCon.Warning("Finishing VU0 %d cycles", _cycles);
		CPU_INT(VIF_VU0_FINISH, _cycles * BIAS); 
		return;
	}
	vif0Regs.stat.VEW = false;
	VIF_LOG("VU0 finished");
	if(vif0.waitforvu == true)
	{
		vif0.waitforvu = false;
		ExecuteVU(0);
		//Make sure VIF0 isnt already scheduled to spin.
		if(!(cpuRegs.interrupt & 0x1) && vif0ch.chcr.STR == true && !vif0Regs.stat.INT)
			vif0Interrupt();
	}
	//DevCon.Warning("VU0 state cleared");
}

__fi void vif0Interrupt()
{
	VIF_LOG("vif0Interrupt: %8.8x", cpuRegs.cycle);

	g_vif0Cycles = 0;

	vif0Regs.stat.FQC = min(vif0ch.qwc, (u16)8);

	if (!(vif0ch.chcr.STR)) Console.WriteLn("vif0 running when CHCR == %x", vif0ch.chcr._u32);

	if (vif0.irq && vif0.tag.size == 0 && vif0.cmd == 0)
	{
		vif0Regs.stat.INT = true;
		
		//Yakuza watches VIF_STAT so lets do this here.
		if (((vif0Regs.code >> 24) & 0x7f) != 0x7) {
			vif0Regs.stat.VIS = true;
		}

		hwIntcIrq(VIF0intc);
		--vif0.irq;

		if (vif0Regs.stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
		{
			//vif0Regs.stat.FQC = 0;

			// One game doesn't like vif stalling at end, can't remember what. Spiderman isn't keen on it tho
			//vif0ch.chcr.STR = false;
			vif0Regs.stat.FQC = min((u16)0x8, vif0ch.qwc);
			if(vif0ch.qwc > 0 || !vif0.done)	
			{
				VIF_LOG("VIF0 Stalled");
				return;
			}
		}
	}

	if(vif0.waitforvu == true)
	{
		//DevCon.Warning("Waiting on VU0");
		//CPU_INT(DMAC_VIF0, 16);
		return;
	}

	vif0.vifstalled.enabled = false;

	//Must go after the Stall, incase it's still in progress, GTC africa likes to see it still transferring.
	if (vif0.cmd) 
	{
		if(vif0.done == true && vif0ch.qwc == 0)	vif0Regs.stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif0Regs.stat.VPS = VPS_IDLE;
	}

	if (vif0.inprogress & 0x1)
	{
		_VIF0chain();
		vif0Regs.stat.FQC = min(vif0ch.qwc, (u16)8);
		CPU_INT(DMAC_VIF0, g_vif0Cycles);
		return;
	}

	if (!vif0.done)
	{

		if (!(dmacRegs.ctrl.DMAE))
		{
			Console.WriteLn("vif0 dma masked");
			return;
		}

		if ((vif0.inprogress & 0x1) == 0) vif0SetupTransfer();
		vif0Regs.stat.FQC = min(vif0ch.qwc, (u16)8);
		CPU_INT(DMAC_VIF0, g_vif0Cycles);
		return;
	}

	if (vif0.vifstalled.enabled && vif0.done)
	{
		DevCon.WriteLn("VIF0 looping on stall at end\n");
		CPU_INT(DMAC_VIF0, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif0ch.qwc > 0) Console.WriteLn("vif0 Ending with %x QWC left", vif0ch.qwc);
	if (vif0.cmd != 0) Console.WriteLn("vif0.cmd still set %x tag size %x", vif0.cmd, vif0.tag.size);
#endif

	vif0ch.chcr.STR = false;
	vif0Regs.stat.FQC = min((u16)0x8, vif0ch.qwc);
	vif0.vifstalled.enabled = false;
	vif0.irqoffset.enabled = false;
	if(vif0.queued_program == true) vifExecQueue(0);
	g_vif0Cycles = 0;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs.stat.FQC = 0;
	DMA_LOG("VIF0 DMA End");
}

void dmaVIF0()
{
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif0ch.chcr._u32, vif0ch.madr, vif0ch.qwc,
	        vif0ch.tadr, vif0ch.asr0, vif0ch.asr1);

	g_vif0Cycles = 0;
		
	if (vif0ch.qwc > 0)   // Normal Mode
	{
		if (vif0ch.chcr.MOD == CHAIN_MODE)
		{
			vif0.dmamode = VIF_CHAIN_MODE;

			if ((vif0ch.chcr.tag().ID == TAG_REFE) || (vif0ch.chcr.tag().ID == TAG_END) || (vif0ch.chcr.tag().IRQ && vif0ch.chcr.TIE))
			{
				vif0.done = true;
			}
			else
			{
				vif0.done = false;
			}
		}
		else //Assume Normal mode.
		{
			vif0.dmamode = VIF_NORMAL_FROM_MEM_MODE;

			if (vif0.irqoffset.enabled == true && vif0.done == false) DevCon.Warning("Warning! VIF0 starting a Normal transfer with vif offset set (Possible force stop?)");
			vif0.done = true;
		}

		vif0.inprogress |= 1;
	}
	else
	{
		if (vif0.irqoffset.enabled == true && vif0.done == false) DevCon.Warning("Warning! VIF0 starting a new Chain transfer with vif offset set (Possible force stop?)");
		vif0.dmamode = VIF_CHAIN_MODE;
		vif0.done = false;
		vif0.inprogress &= ~0x1;
	}

	vif0Regs.stat.FQC = min((u16)0x8, vif0ch.qwc);

	//Using a delay as Beyond Good and Evil does the DMA twice with 2 different TADR's (no checks in the middle, all one block of code),
	//the first bit it sends isnt required for it to work.
	//Also being an end chain it ignores the second lot, this causes infinite loops ;p
	// Chain Mode
	CPU_INT(DMAC_VIF0, 4);
}
