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

#include "Gif.h"
#include "Gif_Unit.h"
#include "GS.h"
#include "Vif.h"
#include "Vif_Dma.h"
#include "IPU/IPU.h"
#include "IPU/IPU_Fifo.h"

//////////////////////////////////////////////////////////////////////////
/////////////////////////// Quick & dirty FIFO :D ////////////////////////
//////////////////////////////////////////////////////////////////////////

// Notes on FIFO implementation
//
// The FIFO consists of four separate pages of HW register memory, each mapped to a
// PS2 device.  They are listed as follows:
//
// 0x4000 - 0x5000 : VIF0  (all registers map to 0x4000)
// 0x5000 - 0x6000 : VIF1  (all registers map to 0x5000)
// 0x6000 - 0x7000 : GS    (all registers map to 0x6000)
// 0x7000 - 0x8000 : IPU   (registers map to 0x7000 and 0x7010, respectively)

void __fastcall ReadFIFO_VIF1(mem128_t* out)
{
	if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) )
		DevCon.Warning( "Reading from vif1 fifo when stalled" );

	pxAssertRel(vif1Regs.stat.FQC != 0, "FQC = 0 on VIF FIFO READ!");
	if (vif1Regs.stat.FDR)
	{
		if(vif1Regs.stat.FQC > vif1.GSLastDownloadSize)
		{
			DevCon.Warning("Warning! GS Download size < FIFO count!");
		}
		if (vif1Regs.stat.FQC > 0)
		{
			GetMTGS().WaitGS();
			GSreadFIFO((u64*)out);
			vif1.GSLastDownloadSize--;
			GUNIT_LOG("ReadFIFO_VIF1");
			if (vif1.GSLastDownloadSize <= 16)
				gifRegs.stat.OPH = false;
			vif1Regs.stat.FQC = min((u32)16, vif1.GSLastDownloadSize);
		}
	}

	VIF_LOG("ReadFIFO/VIF1 -> %ls", out->ToString().c_str());
}

//////////////////////////////////////////////////////////////////////////
// WriteFIFO Pages
//
void __fastcall WriteFIFO_VIF0(const mem128_t *value)
{
	VIF_LOG("WriteFIFO/VIF0 <- %ls", value->ToString().c_str());

	vif0ch.qwc += 1;
	if(vif0.irqoffset != 0 && vif0.vifstalled == true) DevCon.Warning("Offset on VIF0 FIFO start!");
	bool ret = VIF0transfer((u32*)value, 4);

	if (vif0.cmd) 
	{
		if(vif0.done && vif0ch.qwc == 0)	vif0Regs.stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif0Regs.stat.VPS = VPS_IDLE;
	}

	pxAssertDev( ret, "vif stall code not implemented" );
}

void __fastcall WriteFIFO_VIF1(const mem128_t *value)
{
	VIF_LOG("WriteFIFO/VIF1 <- %ls", value->ToString().c_str());

	if (vif1Regs.stat.FDR) {
		DevCon.Warning("writing to fifo when fdr is set!");
	}
	if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) ) {
		DevCon.Warning("writing to vif1 fifo when stalled");
	}
	if (vif1.irqoffset != 0 && vif1.vifstalled == true) {
		DevCon.Warning("Offset on VIF1 FIFO start!");
	}

	vif1ch.qwc += 1;

	bool ret = VIF1transfer((u32*)value, 4);

	if (vif1.cmd) {
		if (vif1.done && !vif1ch.qwc) vif1Regs.stat.VPS = VPS_WAITING;
	}
	else vif1Regs.stat.VPS = VPS_IDLE;

	pxAssertDev( ret, "vif stall code not implemented" );
}

void __fastcall WriteFIFO_GIF(const mem128_t *value)
{
	GUNIT_LOG("WriteFIFO_GIF()");
	gifUnit.TransferGSPacketData(GIF_TRANS_FIFO, (u8*)value, 16);
}
