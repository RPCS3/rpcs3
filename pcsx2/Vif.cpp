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
#include "Vif_Dma.h"
#include "newVif.h"
#include "GS.h"
#include "Gif.h"

__aligned16 vifStruct  vif0, vif1;

tGSTransferStatus GSTransferStatus((STOPPED_MODE<<8) | (STOPPED_MODE<<4) | STOPPED_MODE);

void vif0Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars and all the registers */
	memzero(vif0);
	memzero(vif0Regs);

	vif0.regs = &vif0Regs;

	resetNewVif(0);
}

void vif1Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars, and all the registers */
	memzero(vif1);
	memzero(vif1Regs);

	vif1.regs = &vif1Regs;

	resetNewVif(1);
}

void SaveStateBase::vif0Freeze()
{
	FreezeTag("VIFdma");
	Freeze(g_vifCycles); // Dunno if this one is needed, but whatever, it's small. :)
	Freeze(vif0);

	Freeze(nVif[0].bSize);
	FreezeMem(nVif[0].buffer, nVif[0].bSize);
}

void SaveStateBase::vif1Freeze()
{
	Freeze(vif1);

	Freeze(nVif[1].bSize);
	FreezeMem(nVif[1].buffer, nVif[1].bSize);
}

//------------------------------------------------------------------
// Vif0/Vif1 Write32
//------------------------------------------------------------------

extern bool _chainVIF0();
extern bool _VIF0chain();

__fi void vif0FBRST(u32 value) {
	VIF_LOG("VIF0_FBRST write32 0x%8.8x", value);

	if (value & 0x1) // Reset Vif.
	{
		//Console.WriteLn("Vif0 Reset %x", vif0Regs.stat._u32);

		memzero(vif0);
		vif0.regs = &vif0Regs;
		vif0ch.qwc = 0; //?
		cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
		psHu64(VIF0_FIFO) = 0;
		psHu64(VIF0_FIFO + 8) = 0;
		vif0.done = false;
		vif0Regs.err.reset();
		vif0Regs.stat.clear_flags(VIF0_STAT_FQC | VIF0_STAT_INT | VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS | VIF0_STAT_VPS); // FQC=0
	}

	/* Fixme: Forcebreaks are pretty unknown for operation, presumption is it just stops it what its doing
	          usually accompanied by a reset, but if we find a broken game which falls here, we need to see it! (Refraction) */
	if (value & 0x2) // Forcebreak Vif,
	{
		/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
		cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
		vif0Regs.stat.VFS = true;
		vif0Regs.stat.VPS = VPS_IDLE;
		Console.WriteLn("vif0 force break");
	}

	if (value & 0x4) // Stop Vif.
	{
		// Not completely sure about this, can't remember what game, used this, but 'draining' the VIF helped it, instead of
		//  just stoppin the VIF (linuz).
		vif0Regs.stat.VSS = true;
		vif0Regs.stat.VPS = VPS_IDLE;
		vif0.vifstalled = true;
	}

	if (value & 0x8) // Cancel Vif Stall.
	{
		bool cancel = false;

		/* Cancel stall, first check if there is a stall to cancel, and then clear VIF0_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
		if (vif0Regs.stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
			cancel = true;

		vif0Regs.stat.clear_flags(VIF0_STAT_VSS | VIF0_STAT_VFS | VIF0_STAT_VIS |
				    VIF0_STAT_INT | VIF0_STAT_ER0 | VIF0_STAT_ER1);
		if (cancel)
		{
			if (vif0.vifstalled)
			{
				g_vifCycles = 0;

				// loop necessary for spiderman
				//vif0ch.chcr.STR = true;
				 if(vif0ch.chcr.STR) CPU_INT(DMAC_VIF0, 0); // Gets the timing right - Flatout
			}
		}
	}
}

__fi void vif1FBRST(u32 value) {
	VIF_LOG("VIF1_FBRST write32 0x%8.8x", value);

	if (FBRST(value).RST) // Reset Vif.
	{
		memzero(vif1);
		vif1.regs = &vif1Regs;

		//cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
		vif1ch.qwc -= min((int)vif1ch.qwc, 16); //?
		psHu64(VIF1_FIFO) = 0;
		psHu64(VIF1_FIFO + 8) = 0;
		//vif1.done = false;

		
		//DevCon.Warning("VIF FBRST Reset MSK = %x", vif1Regs.mskpath3);
		if(vif1Regs.mskpath3 == 1 && GSTransferStatus.PTH3 == STOPPED_MODE && gifch.chcr.STR == true) 
		{
			//DevCon.Warning("VIF Path3 Resume on FBRST MSK = %x", vif1Regs.mskpath3);
			gsInterrupt();
			vif1Regs.mskpath3 = false;
			gifRegs.stat.M3P = 0;
		}

		vif1Regs.mskpath3 = false;
		gifRegs.stat.M3P = 0;
		vif1Regs.err.reset();
		vif1.inprogress = 0;
		vif1.cmd = 0;
		vif1.vifstalled = false;
		vif1Regs.stat.FQC = 0;
		vif1Regs.stat.clear_flags(VIF1_STAT_FDR | VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS | VIF1_STAT_VPS);
	}

	/* Fixme: Forcebreaks are pretty unknown for operation, presumption is it just stops it what its doing
	          usually accompanied by a reset, but if we find a broken game which falls here, we need to see it! (Refraction) */

	if (FBRST(value).FBK) // Forcebreak Vif.
	{
		/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
		vif1Regs.stat.VFS = true;
		vif1Regs.stat.VPS = VPS_IDLE;
		cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
		Console.WriteLn("vif1 force break");
	}

	if (FBRST(value).STP) // Stop Vif.
	{
		// Not completely sure about this, can't remember what game used this, but 'draining' the VIF helped it, instead of
		//   just stoppin the VIF (linuz).
		vif1Regs.stat.VSS = true;
		vif1Regs.stat.VPS = VPS_IDLE;
		cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
		vif1.vifstalled = true;
	}

	if (FBRST(value).STC) // Cancel Vif Stall.
	{
		bool cancel = false;

		/* Cancel stall, first check if there is a stall to cancel, and then clear VIF1_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
		if (vif1Regs.stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			cancel = true;
		}

		vif1Regs.stat.clear_flags(VIF1_STAT_VSS | VIF1_STAT_VFS | VIF1_STAT_VIS |
				VIF1_STAT_INT | VIF1_STAT_ER0 | VIF1_STAT_ER1);

		if (cancel)
		{
			if (vif1.vifstalled)
			{
				g_vifCycles = 0;
				// loop necessary for spiderman
				switch(dmacRegs.ctrl.MFD)
				{
				    case MFD_VIF1:
                        //Console.WriteLn("MFIFO Stall");
                        if(vif1ch.chcr.STR == true) CPU_INT(DMAC_MFIFO_VIF, 0);
                        break;

                    case NO_MFD:
                    case MFD_RESERVED:
                    case MFD_GIF: // Wonder if this should be with VIF?
                        // Gets the timing right - Flatout
                        if(vif1ch.chcr.STR == true) CPU_INT(DMAC_VIF1, 0);
                        break;
				}

				//vif1ch.chcr.STR = true;
			}
		}
	}
}

__fi void vif1STAT(u32 value) {
	VIF_LOG("VIF1_STAT write32 0x%8.8x", value);

	/* Only FDR bit is writable, so mask the rest */
	if ((vif1Regs.stat.FDR) ^ ((tVIF_STAT&)value).FDR) {
		// different so can't be stalled
		if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS)) {
			DevCon.WriteLn("changing dir when vif1 fifo stalled");
		}
	}

	vif1Regs.stat.FDR = VIF_STAT(value).FDR;

	if (vif1Regs.stat.FDR) // Vif transferring to memory.
	{
	    // Hack but it checks this is true before transfer? (fatal frame)
		// Update Refraction: Use of this function has been investigated and understood.
		// Before this ever happens, a DIRECT/HL command takes place sending the transfer info to the GS
		// One of the registers told about this is TRXREG which tells us how much data is going to transfer (th x tw) in words
		// As far as the GS is concerned, the transfer starts as soon as TRXDIR is accessed, which is why fatal frame
		// was expecting data, the GS should already be sending it over (buffering in the FIFO)

		vif1Regs.stat.FQC = min((u32)16, vif1.GSLastDownloadSize);
		//Console.Warning("Reversing VIF Transfer for %x QWC", vif1.GSLastDownloadSize);

	}
	else // Memory transferring to Vif.
	{
		//Sometimes the value from the GS is bigger than vif wanted, so it just sets it back and cancels it.
		//Other times it can read it off ;)
		vif1Regs.stat.FQC = 0;
	}
}

#define caseVif(x) (idx ? VIF1_##x : VIF0_##x)

_vifT __fi u32 vifRead32(u32 mem) {
	vifStruct& vif = GetVifX;

	switch (mem) {
		case caseVif(ROW0): return vif.MaskRow._u32[0];
		case caseVif(ROW1): return vif.MaskRow._u32[1];
		case caseVif(ROW2): return vif.MaskRow._u32[2];
		case caseVif(ROW3): return vif.MaskRow._u32[3];

		case caseVif(COL0): return vif.MaskCol._u32[0];
		case caseVif(COL1): return vif.MaskCol._u32[1];
		case caseVif(COL2): return vif.MaskCol._u32[2];
		case caseVif(COL3): return vif.MaskCol._u32[3];
	}
	
	return psHu32(mem);
}

// returns FALSE if no writeback is needed (or writeback is handled internally)
// returns TRUE if the caller should writeback the value to the eeHw register map.
_vifT __fi bool vifWrite32(u32 mem, u32 value) {
	vifStruct& vif = GetVifX;

	switch (mem) {
		case caseVif(MARK):
			VIF_LOG("VIF%d_MARK write32 0x%8.8x", idx, value);
			vifXRegs.stat.MRK = false;
			//vifXRegs.mark	   = value;
		break;

		case caseVif(FBRST):
			if (!idx) vif0FBRST(value);
			else	  vif1FBRST(value);
		return false;

		case caseVif(STAT):
			if (idx) { // Only Vif1 does this stuff?
				vif1STAT(value);
			}
		return false;

		case caseVif(ERR):
		case caseVif(MODE):
			// standard register writes -- handled by caller.
		break;

		case caseVif(ROW0): vif.MaskRow._u32[0] = value; return false;
		case caseVif(ROW1): vif.MaskRow._u32[1] = value; return false;
		case caseVif(ROW2): vif.MaskRow._u32[2] = value; return false;
		case caseVif(ROW3): vif.MaskRow._u32[3] = value; return false;

		case caseVif(COL0): vif.MaskCol._u32[0] = value; return false;
		case caseVif(COL1): vif.MaskCol._u32[1] = value; return false;
		case caseVif(COL2): vif.MaskCol._u32[2] = value; return false;
		case caseVif(COL3): vif.MaskCol._u32[3] = value; return false;
	}

	// fall-through case: issue standard writeback behavior.
	return true;
}

template u32 vifRead32<0>(u32 mem);
template u32 vifRead32<1>(u32 mem);

template bool vifWrite32<0>(u32 mem, u32 value);
template bool vifWrite32<1>(u32 mem, u32 value);
