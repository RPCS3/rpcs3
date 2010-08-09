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
#include "newVif.h"

//------------------------------------------------------------------
// VifCode Transfer Interpreter (Vif0/Vif1)
//------------------------------------------------------------------

// Doesn't stall if the next vifCode is the Mark command
_vifT bool runMark(u32* &data) {
	if (((vifXRegs->code >> 24) & 0x7f) == 0x7) {
		Console.WriteLn("Vif%d: Running Mark with I-bit", idx);
		return 1; // No Stall?
	}
	return 1; // Stall
}

// Returns 1 if i-bit && finished vifcode && i-bit not masked
_vifT bool analyzeIbit(u32* &data, int iBit) {
	vifStruct& vifX = GetVifX;
	if (iBit && !vifX.cmd && !vifXRegs->err.MII) {
		//DevCon.WriteLn("Vif I-Bit IRQ");
		vifX.irq++;

		// Okay did some testing with Max Payne, it does this:
		// VifMark  value = 0x666   (i know, evil!)
		// NOP with I Bit
		// VifMark  value = 0
		//
		// If you break after the 2nd Mark has run, the game reports invalid mark 0 and the game dies.
		// So it has to occur here, testing a theory that it only doesn't stall if the command with
		// the iBit IS mark, but still sends the IRQ to let the cpu know the mark is there. (Refraction)
		//
		// --------------------------
		//
		// This is how it probably works: i-bit sets the IRQ flag, and VIF keeps running until it encounters
		// a non-MARK instruction.  This includes the *current* instruction.  ie, execution only continues
		// unimpeded if MARK[i] is specified, and keeps executing unimpeded until any non-MARK command.
		// Any other command with an I bit should stall immediately.
		// Example:
		//
		// VifMark[i] value = 0x321   (with I bit)
		// VifMark    value = 0
		// VifMark    value = 0x333
		// NOP
		//
		// ... the VIF should not stall and raise the interrupt until after the NOP is processed.
		// So the final value for MARK as the game sees it will be 0x333. --air
		
		return runMark<idx>(data);
	}
	return 0;
}

// Interprets packet
_vifT void vifTransferLoop(u32* &data) {
	vifStruct& vifX = GetVifX;

	u32& pSize = vifX.vifpacketsize;
	int  iBit  = vifX.cmd >> 7;

	vifXRegs->stat.VPS |= VPS_TRANSFERRING;
	vifXRegs->stat.ER1  = false;

	while (pSize > 0 && !vifX.vifstalled) {

		if(!vifX.cmd) { // Get new VifCode
			vifX.lastcmd = (vifXRegs->code >> 24) & 0x7f;
			vifXRegs->code = data[0];
			vifX.cmd	   = data[0] >> 24;
			iBit		   = data[0] >> 31;
			VIF_LOG("New VifCMD %x tagsize %x", vifX.cmd, vifX.tag.size);
			vifCmdHandler[idx][vifX.cmd & 0x7f](0, data);
			data++; pSize--;
			if (analyzeIbit<idx>(data, iBit)) break;
			continue;
		}

		int ret = vifCmdHandler[idx][vifX.cmd & 0x7f](1, data);
		data   += ret;
		pSize  -= ret;
		if (analyzeIbit<idx>(data, iBit)) break;
	}
	
	if (pSize)	  vifX.vifstalled	 = true;
}

_vifT static __fi bool vifTransfer(u32 *data, int size) {
	vifStruct& vifX = GetVifX;

	// irqoffset necessary to add up the right qws, or else will spin (spiderman)
	int transferred = vifX.vifstalled ? vifX.irqoffset : 0;

	vifX.irqoffset  = 0;
	vifX.vifstalled = false;
	vifX.stallontag = false;
	vifX.vifpacketsize = size;
	g_packetsizeonvu = size;
	vifTransferLoop<idx>(data);


	transferred   += size - vifX.vifpacketsize;

	g_vifCycles   +=((transferred * BIAS) >> 2) ; /* guessing */

	if(!idx && g_vu0Cycles > 0)
	{
		if(g_vifCycles < g_vu0Cycles) g_vu0Cycles -= g_vifCycles;
		else if(g_vifCycles >= g_vu0Cycles)g_vu0Cycles = 0;
	}
	else if(idx && g_vu1Cycles > 0)
	{
		if(g_vifCycles < g_vu1Cycles) g_vu1Cycles -= g_vifCycles;
		else if(g_vifCycles >= g_vu1Cycles)g_vu1Cycles = 0;
	}

	vifX.irqoffset = transferred % 4; // cannot lose the offset

	transferred   = transferred >> 2;

	vifXch->madr +=(transferred << 4);
	vifXch->qwc  -= transferred;

	if (!vifXch->qwc && !vifX.irqoffset) vifX.inprogress &= ~0x1;

	if (vifX.irq && vifX.cmd == 0) {
		//DevCon.WriteLn("Vif IRQ!");
		if(((vifXRegs->code >> 24) & 0x7f) != 0x7)
		{
			vifX.vifstalled    = true;
			vifXRegs->stat.VIS = true; // Note: commenting this out fixes WALL-E?
		}

		if (!vifXch->qwc && !vifX.irqoffset) vifX.inprogress = 0;
		return false;
	}

	return !vifX.vifstalled;
}

bool VIF0transfer(u32 *data, int size) {
	return vifTransfer<0>(data, size);
}
bool VIF1transfer(u32 *data, int size) {
	return vifTransfer<1>(data, size);
}
