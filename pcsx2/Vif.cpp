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
#include "Vif_Dma.h"

void vif0Init() { initNewVif(0); }
void vif1Init() { initNewVif(1); }

void vif0Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars and all the registers */
	memzero(vif0);
	memzero(*vif0Regs);

	psHu64(VIF0_FIFO) = 0;
	psHu64(VIF0_FIFO + 8) = 0;
	
	vif0Regs->stat.VPS = VPS_IDLE;
	vif0Regs->stat.FQC = 0;

	vif0.done = true;

	resetNewVif(0);
}

void vif1Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars, and all the registers */
	memzero(vif1);
	memzero(*vif1Regs);

	psHu64(VIF1_FIFO) = 0;
	psHu64(VIF1_FIFO + 8) = 0;

	vif1Regs->stat.VPS = VPS_IDLE;
	vif1Regs->stat.FQC = 0; // FQC=0

	vif1.done = true;
	cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's

	resetNewVif(1);
}

void SaveStateBase::vif0Freeze()
{
	static u32 g_vif0Masks[64];   // Dummy Var for saved state compatibility
	static u32 g_vif0HasMask3[4]; // Dummy Var for saved state compatibility
	FreezeTag("VIFdma");

	// Dunno if this one is needed, but whatever, it's small. :)
	Freeze(g_vifCycles);

	// mask settings for VIF0 and VIF1
	Freeze(g_vifmask);

	Freeze(vif0);
	Freeze(g_vif0HasMask3);	// Not Used Anymore
	Freeze(g_vif0Masks);	// Not Used Anymore
}

void SaveStateBase::vif1Freeze()
{
	static u32 g_vif1Masks[64];   // Dummy Var for saved state compatibility
	static u32 g_vif1HasMask3[4]; // Dummy Var for saved state compatibility
	Freeze(vif1);

	Freeze(g_vif1HasMask3);	// Not Used Anymore
	Freeze(g_vif1Masks);	// Not Used Anymore
}
