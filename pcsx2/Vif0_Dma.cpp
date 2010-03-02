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

bool  _VIF0chain()
{
	u32 *pMem;

	if ((vif0ch->qwc == 0) && !vif0.vifstalled) return true;

	pMem = (u32*)dmaGetAddr(vif0ch->madr);
	if (pMem == NULL)
	{
		vif0.cmd = 0;
		vif0.tag.size = 0;
		vif0ch->qwc = 0;
		return true;
	}

	if (vif0.vifstalled)
		return VIF0transfer(pMem + vif0.irqoffset, vif0ch->qwc * 4 - vif0.irqoffset, false);
	else
		return VIF0transfer(pMem, vif0ch->qwc * 4, false);
}

bool _chainVIF0()
{
    tDMA_TAG *ptag;
    
	ptag = dmaGetAddr(vif0ch->tadr); //Set memory pointer to TADR

	if (!(vif0ch->transfer("Vif0 Tag", ptag))) return false;

	vif0ch->madr = ptag[1]._u32;		// MADR = ADDR field + SPR
    g_vifCycles += 1; 				// Increase the QW read for the tag

	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        ptag[1]._u32, ptag[0]._u32, vif0ch->qwc, ptag->ID, vif0ch->madr, vif0ch->tadr);

	// Transfer dma tag if tte is set
	if (vif0ch->chcr.TTE)
	{
	    bool ret;

		if (vif0.vifstalled)
			ret = VIF0transfer((u32*)ptag + (2 + vif0.irqoffset), 2 - vif0.irqoffset, true);  //Transfer Tag on stall
		else
			ret = VIF0transfer((u32*)ptag + 2, 2, true);  //Transfer Tag

		if (!(ret)) return false;        //IRQ set by VIFTransfer
	}

	vif0.done |= hwDmacSrcChainWithStack(vif0ch, ptag->ID);

	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        ptag[1]._u32, ptag[0]._u32, vif0ch->qwc, ptag->ID, vif0ch->madr, vif0ch->tadr);

	_VIF0chain();											   //Transfers the data set by the switch

	if (vif0ch->chcr.TIE && ptag->IRQ)  //Check TIE bit of CHCR and IRQ bit of tag
	{
		VIF_LOG("dmaIrq Set\n");
		vif0.done = true; //End Transfer
	}
	
	return vif0.done;
}

void vif0Interrupt()
{
	g_vifCycles = 0; //Reset the cycle count, Wouldn't reset on stall if put lower down.
	VIF_LOG("vif0Interrupt: %8.8x", cpuRegs.cycle);

	if (vif0.irq && (vif0.tag.size == 0))
	{
		vif0Regs->stat.INT = true;
		hwIntcIrq(VIF0intc);
		--vif0.irq;

		if (vif0Regs->stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
		{
			vif0Regs->stat.FQC = 0;
			vif0ch->chcr.STR = false;
			return;
		}

		if (vif0ch->qwc > 0 || vif0.irqoffset > 0)
		{
			if (vif0.stallontag)
				_chainVIF0();
			else
				_VIF0chain();

			CPU_INT(DMAC_VIF0, /*g_vifCycles*/ VifCycleVoodoo);
			return;
		}
	}

	if (!vif0ch->chcr.STR) Console.WriteLn("Vif0 running when CHCR = %x", vif0ch->chcr._u32);

	if ((vif0ch->chcr.MOD == CHAIN_MODE) && (!vif0.done) && (!vif0.vifstalled))
	{

		if (!(dmacRegs->ctrl.DMAE))
		{
			Console.WriteLn("vif0 dma masked");
			return;
		}

		if (vif0ch->qwc > 0)
			_VIF0chain();
		else
			_chainVIF0();

		CPU_INT(DMAC_VIF0, /*g_vifCycles*/ VifCycleVoodoo);
		return;
	}

	if (vif0ch->qwc > 0) Console.WriteLn("VIF0 Ending with QWC left");
	if (vif0.cmd != 0) Console.WriteLn("vif0.cmd still set %x", vif0.cmd);

	vif0ch->chcr.STR = false;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs->stat.FQC = 0;
}

void dmaVIF0()
{
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx\n",
	        vif0ch->chcr._u32, vif0ch->madr, vif0ch->qwc,
	        vif0ch->tadr, vif0ch->asr0, vif0ch->asr1);

	g_vifCycles = 0;

	vif0Regs->stat.FQC = 0x8; // FQC=8

	if (!(vif0ch->chcr.MOD & 0x1) || vif0ch->qwc > 0)   // Normal Mode
	{
		if (!_VIF0chain())
		{
			Console.WriteLn(L"Stall on normal vif0 " + vif0Regs->stat.desc());

			vif0.vifstalled = true;
			return;
		}

		vif0.done = true;
		CPU_INT(DMAC_VIF0, /*g_vifCycles*/ VifCycleVoodoo);
		return;
	}

	// Chain Mode
	vif0.done = false;
	CPU_INT(DMAC_VIF0, 0);
}
