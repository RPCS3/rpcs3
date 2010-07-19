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
#include "GS.h"
#include "Vif.h"
#include "Vif_Dma.h"
#include "IPU/IPU.h"
#include "IPU/IPU_Fifo.h"

//////////////////////////////////////////////////////////////////////////
/////////////////////////// Quick & dirty FIFO :D ////////////////////////
//////////////////////////////////////////////////////////////////////////

// ** NOTE: cannot use XMM/MMX regs **

// Notes on FIFO implementation
//
// The FIFO consists of four separate pages of HW register memory, each mapped to a
// PS2 device.  They are listed as follows:
//
// 0x4000 - 0x5000 : VIF0  (all registers map to 0x4000)
// 0x5000 - 0x6000 : VIF1  (all registers map to 0x5000)
// 0x6000 - 0x7000 : GS    (all registers map to 0x6000)
// 0x7000 - 0x8000 : IPU   (registers map to 0x7000 and 0x7010, respectively)

//////////////////////////////////////////////////////////////////////////
// ReadFIFO Pages

void __fastcall ReadFIFO_page_4(u32 mem, u64 *out)
{
	pxAssert( (mem >= VIF0_FIFO) && (mem < VIF1_FIFO) );

	VIF_LOG("ReadFIFO/VIF0 0x%08X", mem);

	out[0] = psHu64(VIF0_FIFO);
	out[1] = psHu64(VIF0_FIFO + 8);
}

void __fastcall ReadFIFO_page_5(u32 mem, u64 *out)
{
	pxAssert( (mem >= VIF1_FIFO) && (mem < GIF_FIFO) );

	VIF_LOG("ReadFIFO/VIF1, addr=0x%08X", mem);

	if (vif1Regs->stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) )
		DevCon.Warning( "Reading from vif1 fifo when stalled" );

	if(vif1Regs->stat.FQC == 0) Console.Warning("FQC = 0 on VIF FIFO READ!");
	if (vif1Regs->stat.FDR)
	{
		if(vif1Regs->stat.FQC > vif1.GSLastDownloadSize)
		{
			DevCon.Warning("Warning! GS Download size < FIFO count!");
		}
		if (vif1Regs->stat.FQC > 0)
		{
			GetMTGS().WaitGS();
			GSreadFIFO(&psHu64(VIF1_FIFO));
			vif1.GSLastDownloadSize--;
			if (vif1.GSLastDownloadSize <= 16)
				gifRegs->stat.OPH = false;
			vif1Regs->stat.FQC = min((u32)16, vif1.GSLastDownloadSize);
		}
	}

	out[0] = psHu64(VIF1_FIFO);
	out[1] = psHu64(VIF1_FIFO + 8);
}

void __fastcall ReadFIFO_page_6(u32 mem, u64 *out)
{
	pxAssert( (mem >= GIF_FIFO) && (mem < IPUout_FIFO) );

	DevCon.Warning( "ReadFIFO/GIF, addr=0x%x", mem );

	out[0] = psHu64(GIF_FIFO);
	out[1] = psHu64(GIF_FIFO + 8);
}

void __fastcall ReadFIFO_page_7(u32 mem, u64 *out)
{
	pxAssert( (mem >= IPUout_FIFO) && (mem < D0_CHCR) );

	// All addresses in this page map to 0x7000 and 0x7010:
	mem &= 0x10;

	if( mem == 0 ) // IPUout_FIFO
	{
		if( g_nIPU0Data > 0 )
		{
			out[0] = *(u64*)(g_pIPU0Pointer);
			out[1] = *(u64*)(g_pIPU0Pointer+8);
			ipu_fifo.out.readpos = (ipu_fifo.out.readpos + 4) & 31;
			g_nIPU0Data--;
			g_pIPU0Pointer += 16;
		}
	}
	else // IPUin_FIFO
		ipu_fifo.out.readsingle((void*)out);
}

//////////////////////////////////////////////////////////////////////////
// WriteFIFO Pages

void __fastcall WriteFIFO_page_4(u32 mem, const mem128_t *value)
{
	pxAssert( (mem >= VIF0_FIFO) && (mem < VIF1_FIFO) );

	VIF_LOG("WriteFIFO/VIF0, addr=0x%08X", mem);

	psHu64(VIF0_FIFO) = value[0];
	psHu64(VIF0_FIFO + 8) = value[1];

	vif0ch->qwc += 1;
	if(vif0.irqoffset != 0 && vif0.vifstalled == true) DevCon.Warning("Offset on VIF0 FIFO start!");
	bool ret = VIF0transfer((u32*)value, 4);

	if (vif0.cmd) 
	{
		if(vif0.done == true && vif0ch->qwc == 0)	vif0Regs->stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif0Regs->stat.VPS = VPS_IDLE;
	}

	pxAssertDev( ret, "vif stall code not implemented" );
}

void __fastcall WriteFIFO_page_5(u32 mem, const mem128_t *value)
{
	pxAssert( (mem >= VIF1_FIFO) && (mem < GIF_FIFO) );

	VIF_LOG("WriteFIFO/VIF1, addr=0x%08X", mem);

	psHu64(VIF1_FIFO) = value[0];
	psHu64(VIF1_FIFO + 8) = value[1];

	if (vif1Regs->stat.FDR)
		DevCon.Warning("writing to fifo when fdr is set!");
	if (vif1Regs->stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) )
		DevCon.Warning("writing to vif1 fifo when stalled");

	vif1ch->qwc += 1;
	if(vif1.irqoffset != 0 && vif1.vifstalled == true) DevCon.Warning("Offset on VIF1 FIFO start!");
	bool ret = VIF1transfer((u32*)value, 4);

	if(GSTransferStatus.PTH2 == STOPPED_MODE && gifRegs->stat.APATH == GIF_APATH2)
	{
		gifRegs->stat.APATH = GIF_APATH_IDLE;
		if(gifRegs->stat.P1Q) gsPath1Interrupt();
	}
	if (vif1.cmd) 
	{
		if(vif1.done == true && vif1ch->qwc == 0)	vif1Regs->stat.VPS = VPS_WAITING;
	}
	else		 
	{
		vif1Regs->stat.VPS = VPS_IDLE;
	}

	pxAssertDev( ret, "vif stall code not implemented" );
}

// Dummy GIF-TAG Packet to Guarantee Count = 1
__aligned16 u32 nloop0_packet[4] = {0, 0, 0, 0};

void __fastcall WriteFIFO_page_6(u32 mem, const mem128_t *value)
{
	pxAssert( (mem >= GIF_FIFO) && (mem < IPUout_FIFO) );
	GIF_LOG("WriteFIFO/GIF, addr=0x%08X", mem);

	psHu64(GIF_FIFO) = value[0];
	psHu64(GIF_FIFO + 8) = value[1];

	nloop0_packet[0] = psHu32(GIF_FIFO);
	nloop0_packet[1] = psHu32(GIF_FIFO + 4);
	nloop0_packet[2] = psHu32(GIF_FIFO + 8);
	nloop0_packet[3] = psHu32(GIF_FIFO + 12);
	GetMTGS().PrepDataPacket(GIF_PATH_3, 1);
	//u64* data = (u64*)GetMTGS().GetDataPacketPtr();
	GIFPath_CopyTag( GIF_PATH_3, (u128*)nloop0_packet, 1 );
	GetMTGS().SendDataPacket();
	if(GSTransferStatus.PTH3 == STOPPED_MODE && gifRegs->stat.APATH == GIF_APATH3 )
	{
		gifRegs->stat.APATH = GIF_APATH_IDLE;
		if(gifRegs->stat.P1Q) gsPath1Interrupt();
	}
}

void __fastcall WriteFIFO_page_7(u32 mem, const mem128_t *value)
{
	pxAssert( (mem >= IPUout_FIFO) && (mem < D0_CHCR) );

	// All addresses in this page map to 0x7000 and 0x7010:
	mem &= 0x10;

	IPU_LOG( "WriteFIFO/IPU, addr=0x%x", mem );

	if( mem == 0 )
	{
		// Should this raise a PS2 exception or just ignore silently?
		Console.Warning( "WriteFIFO/IPUout (ignored)" );
	}
	else
	{
		IPU_LOG("WriteFIFO IPU_in[%d] <- %8.8X_%8.8X_%8.8X_%8.8X",
			mem/16, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);

		//committing every 16 bytes
		while( ipu_fifo.in.write((u32*)value, 1) == 0 )
		{
			Console.WriteLn("IPU sleeping");
			Threading::Timeslice();
		}
	}
}
