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
#include "Vif_Dma.h"
#include "VUmicro.h"
#include "newVif.h"

// Run VU0 until finish, don't add cycles to EE
// because its vif stalling not the EE core...
__forceinline void vif0FLUSH() 
{
	if (!(VU0.VI[REG_VPU_STAT].UL & 1)) return;
	int _cycles = VU0.cycle;
	vu0Finish();
	g_vifCycles += (VU0.cycle - _cycles) * BIAS;
}

bool _VIF0chain()
{
	u32 *pMem;

	if (vif0ch->qwc == 0)
	{
		vif0.inprogress = 0;
		return true;
	}

	pMem = (u32*)dmaGetAddr(vif0ch->madr);
	if (pMem == NULL)
	{
		vif0.cmd = 0;
		vif0.tag.size = 0;
		vif0ch->qwc = 0;
		return true;
	}

	VIF_LOG("VIF0chain size=%d, madr=%lx, tadr=%lx",
	        vif0ch->qwc, vif0ch->madr, vif0ch->tadr);

	if (vif0.vifstalled)
		return VIF0transfer(pMem + vif0.irqoffset, vif0ch->qwc * 4 - vif0.irqoffset, false);
	else
		return VIF0transfer(pMem, vif0ch->qwc * 4, false);
}

__forceinline void vif0SetupTransfer()
{
    tDMA_TAG *ptag;
    
	switch (vif0.dmamode)
	{
		case VIF_NORMAL_TO_MEM_MODE:
			vif0.inprogress = 1;
			vif0.done = true;
			g_vifCycles = 2;
			break;

		case VIF_CHAIN_MODE:
			ptag = dmaGetAddr(vif0ch->tadr); //Set memory pointer to TADR

			if (!(vif0ch->transfer("vif0 Tag", ptag))) return;

			vif0ch->madr = ptag[1]._u32;            //MADR = ADDR field + SPR
			g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag

			// Transfer dma tag if tte is set

			VIF_LOG("vif0 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			        ptag[1]._u32, ptag[0]._u32, vif0ch->qwc, ptag->ID, vif0ch->madr, vif0ch->tadr);

			vif0.inprogress = 1;

			if (vif0ch->chcr.TTE)
			{
			    bool ret;

				if (vif0.vifstalled)
					ret = VIF0transfer((u32*)ptag + (2 + vif0.irqoffset), 2 - vif0.irqoffset, true);  //Transfer Tag on stall
				else
					ret = VIF0transfer((u32*)ptag + 2, 2, true);  //Transfer Tag

				if ((ret == false) && vif0.irqoffset < 2)
				{
					vif0.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
					return;       //There has been an error or an interrupt
				}
			}

			vif0.irqoffset = 0;
			vif0.done |= hwDmacSrcChainWithStack(vif0ch, ptag->ID);

			//Check TIE bit of CHCR and IRQ bit of tag
			if (vif0ch->chcr.TIE && ptag->IRQ)
			{
				VIF_LOG("dmaIrq Set");

                //End Transfer
				vif0.done = true;
				return;
			}
			break;
	}
}

__forceinline void vif0Interrupt()
{
	VIF_LOG("vif0Interrupt: %8.8x", cpuRegs.cycle);

	g_vifCycles = 0;

	if (!(vif0ch->chcr.STR)) Console.WriteLn("vif0 running when CHCR == %x", vif0ch->chcr._u32);

	if (vif0.irq && vif0.tag.size == 0)
	{
		vif0Regs->stat.INT = true;
		hwIntcIrq(VIF0intc);
		--vif0.irq;
		if (vif0Regs->stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
		{
			vif0Regs->stat.FQC = 0;

			// One game doesn't like vif stalling at end, can't remember what. Spiderman isn't keen on it tho
			vif0ch->chcr.STR = false;
			return;
		}
		else if ((vif0ch->qwc > 0) || (vif0.irqoffset > 0))
		{
			if (vif0.stallontag)
				vif0SetupTransfer();
			else
				_VIF0chain();//CPU_INT(DMAC_STALL_SIS, vif0ch->qwc * BIAS);
		}
	}

	if (vif0.inprogress & 0x1)
	{
		_VIF0chain();
		// VIF_NORMAL_FROM_MEM_MODE is a very slow operation. 
		// Timesplitters 2 depends on this beeing a bit higher than 128.
		if (vif0.dmamode == VIF_NORMAL_FROM_MEM_MODE ) CPU_INT(DMAC_VIF0, 1024);
		else CPU_INT(DMAC_VIF0, g_vifCycles/*VifCycleVoodoo*/);
		return;
	}

	if (!vif0.done)
	{

		if (!(dmacRegs->ctrl.DMAE))
		{
			Console.WriteLn("vif0 dma masked");
			return;
		}

		if ((vif0.inprogress & 0x1) == 0) vif0SetupTransfer();

		CPU_INT(DMAC_VIF0, g_vifCycles /*VifCycleVoodoo*/);
		return;
	}

	if (vif0.vifstalled && vif0.irq)
	{
		DevCon.WriteLn("VIF0 looping on stall\n");
		CPU_INT(DMAC_VIF0, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif0ch->qwc > 0) Console.WriteLn("vif0 Ending with %x QWC left");
	if (vif0.cmd != 0) Console.WriteLn("vif0.cmd still set %x tag size %x", vif0.cmd, vif0.tag.size);
#endif

	vif0Regs->stat.VPS = VPS_IDLE; //Vif goes idle as the stall happened between commands;
	vif0ch->chcr.STR = false;
	g_vifCycles = 0;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs->stat.FQC = 0;
}

void dmaVIF0()
{
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif0ch->chcr._u32, vif0ch->madr, vif0ch->qwc,
	        vif0ch->tadr, vif0ch->asr0, vif0ch->asr1);

	g_vifCycles = 0;
	vif0.inprogress = 0;

	if ((vif0ch->chcr.MOD == NORMAL_MODE) || vif0ch->qwc > 0)   // Normal Mode
	{
			vif0.dmamode = VIF_NORMAL_TO_MEM_MODE;
	}
	else
	{
		vif0.dmamode = VIF_CHAIN_MODE;
	}

	if (vif0.dmamode != VIF_NORMAL_FROM_MEM_MODE)
		vif0Regs->stat.FQC = 0x8;
	else
		vif0Regs->stat.FQC = min((u16)0x8, vif0ch->qwc);

	// Chain Mode
	vif0.done = false;
	vif0Interrupt();
}