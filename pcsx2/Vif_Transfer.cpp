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
#include "newVif.h"

//------------------------------------------------------------------
// VifCode Transfer Interpreter (Vif0/Vif1)
//------------------------------------------------------------------

// Runs the next vifCode if its the Mark command
_vifT void runMark(u32* &data) {
	if (vifX.vifpacketsize && (((data[0]>>24)&0x7f)==7)) {
		vifX.vifpacketsize--;
		vifXCode[7](0, data++);
		DevCon.WriteLn("Vif%d: Running Mark on I-bit", idx);
	}
}

// Returns 1 if i-bit && finished vifcode && i-bit not masked
_vifT bool analyzeIbit(u32* &data, int iBit) {
	if (iBit && !vifX.cmd && !vifXRegs->err.MII) {
		//DevCon.WriteLn("Vif I-Bit IRQ");
		vifX.irq++;
		// On i-bit, the command is run, vif stalls etc, 
		// however if the vifcode is MASK, you do NOT stall, just send IRQ. - Max Payne shows this up.
		if((vifX.cmd & 0x7f) == 0x7) return 0;
		else return 1;
	}
	return 0;
}

// Interprets packet
_vifT void vifTransferLoop(u32* &data) {
	u32& pSize = vifX.vifpacketsize;
	int  iBit  = vifX.cmd >> 7;

	vifXRegs->stat.VPS |= VPS_TRANSFERRING;
	vifXRegs->stat.ER1  = false;

	while (pSize > 0 && !vifX.vifstalled) {

		if(!vifX.cmd) { // Get new VifCode
			vifXRegs->code = data[0];
			vifX.cmd	   = data[0] >> 24;
			iBit		   = data[0] >> 31;

			vifXCode[vifX.cmd & 0x7f](0, data);
			data++; pSize--;
			if (analyzeIbit<idx>(data, iBit)) break;
			continue;
		}
		
		int ret = vifXCode[vifX.cmd & 0x7f](1, data);
		data   += ret;
		pSize  -= ret;
		if (analyzeIbit<idx>(data, iBit)) break;
	}
	if (vifX.cmd) vifXRegs->stat.VPS = VPS_WAITING;
	else		  vifXRegs->stat.VPS = VPS_IDLE;
	if (pSize)	  vifX.vifstalled	 = true;
}

_vifT _f bool vifTransfer(u32 *data, int size) {
	// irqoffset necessary to add up the right qws, or else will spin (spiderman)
	int transferred = vifX.vifstalled ? vifX.irqoffset : 0;

	vifX.irqoffset  = 0;
	vifX.vifstalled = false;
	vifX.stallontag = false;
	vifX.vifpacketsize = size;

	vifTransferLoop<idx>(data);

	transferred   += size - vifX.vifpacketsize;
	g_vifCycles   +=(transferred >> 2) * BIAS; /* guessing */
	vifX.irqoffset = transferred % 4; // cannot lose the offset

	transferred   = transferred >> 2;
	vifXch->madr +=(transferred << 4);
	vifXch->qwc  -= transferred;

	if (!vifXch->qwc && !vifX.irqoffset) vifX.inprogress &= ~0x1;

	if (vifX.irq && vifX.cmd == 0) {
		//DevCon.WriteLn("Vif IRQ!");
		vifX.vifstalled    = true;
		vifXRegs->stat.VIS = true; // Note: commenting this out fixes WALL-E?

		if (!vifXch->qwc && !vifX.irqoffset) vifX.inprogress = 0;
		return false;
	}

	return !vifX.vifstalled;
}

bool VIF0transfer(u32 *data, int size, bool istag) {
	return vifTransfer<0>(data, size);
}
bool VIF1transfer(u32 *data, int size, bool istag) {
	return vifTransfer<1>(data, size);
}
