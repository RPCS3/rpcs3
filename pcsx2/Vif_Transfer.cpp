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
	if (((vifXRegs.code >> 24) & 0x7f) == 0x7) {
		//DevCon.WriteLn("Vif%d: Running Mark with I-bit", idx);
		return 1; // No Stall?
	}
	return 1; // Stall
}

// Returns 1 if i-bit && finished vifcode && i-bit not masked
_vifT bool analyzeIbit(u32* &data, int iBit) {
	vifStruct& vifX = GetVifX;
	if (iBit && !vifX.cmd && !vifXRegs.err.MII) {
		//DevCon.WriteLn("Vif I-Bit IRQ");
		vifX.irq++;

		if(CHECK_VIF1STALLHACK) return 0;
		else return 1;
	}
	return 0;
}

// Interprets packet
_vifT void vifTransferLoop(u32* &data) {
	vifStruct& vifX = GetVifX;

	u32& pSize = vifX.vifpacketsize;
	int  iBit  = vifX.cmd >> 7;
	int ret = 0;

	vifXRegs.stat.VPS |= VPS_TRANSFERRING;
	vifXRegs.stat.ER1  = false;

	while (pSize > 0 && !vifX.vifstalled) {

		if(!vifX.cmd) { // Get new VifCode
			
			vifXRegs.code = data[0];
			vifX.cmd	  = data[0] >> 24;
			iBit		  = data[0] >> 31;

			//VIF_LOG("New VifCMD %x tagsize %x", vifX.cmd, vifX.tag.size);
			if (IsDevBuild && SysTrace.EE.VIFcode.IsActive()) {
				// Pass 2 means "log it"
				vifCmdHandler[idx][vifX.cmd & 0x7f](2, data);
			}
		}

		ret = vifCmdHandler[idx][vifX.cmd & 0x7f](vifX.pass, data);
		data   += ret;
		pSize  -= ret;
		if (analyzeIbit<idx>(data, iBit)) break;
	}

	if (pSize) vifX.vifstalled = true;
}

_vifT static __fi bool vifTransfer(u32 *data, int size, bool TTE) {
	vifStruct& vifX = GetVifX;

	// irqoffset necessary to add up the right qws, or else will spin (spiderman)
	int transferred = vifX.irqoffset;

	vifX.irqoffset  = 0;
	vifX.vifstalled = false;
	vifX.stallontag = false;
	vifX.vifpacketsize = size;
	vifTransferLoop<idx>(data);

	transferred += size - vifX.vifpacketsize;

	//Make this a minimum of 1 cycle so if it's the end of the packet it doesnt just fall through.
	//Metal Saga can do this, just to be safe :)
	if (!idx) g_vif0Cycles += max(1, (int)((transferred * BIAS) >> 2));
	else	  g_vif1Cycles += max(1, (int)((transferred * BIAS) >> 2));

	vifX.irqoffset = transferred % 4; // cannot lose the offset

	if (!TTE) {// *WARNING* - Tags CAN have interrupts! so lets just ignore the dma modifying stuffs (GT4)
		transferred  = transferred >> 2;
		vifXch.madr +=(transferred << 4);
		vifXch.qwc  -= transferred;
		if (vifXch.chcr.STR) hwDmacSrcTadrInc(vifXch);
		if(!vifXch.qwc) {
			vifX.inprogress &= ~0x1;
			vifX.vifstalled = false;
		}
	}
	else {
		if (!vifX.irqoffset) vifX.vifstalled = false;
	}

	if (vifX.irq && vifX.cmd == 0) {
		//DevCon.WriteLn("Vif IRQ!");
		if(((vifXRegs.code >> 24) & 0x7f) != 0x7) {
			vifXRegs.stat.VIS = true; // Note: commenting this out fixes WALL-E?
			vifX.vifstalled = true;
		}		
	}	

	return !vifX.vifstalled;
}

// When TTE is set to 1, MADR and QWC are not updated as part of the transfer.
bool VIF0transfer(u32 *data, int size, bool TTE) {
	return vifTransfer<0>(data, size, TTE);
}
bool VIF1transfer(u32 *data, int size, bool TTE) {
	return vifTransfer<1>(data, size, TTE);
}
