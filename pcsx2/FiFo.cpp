/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "Hw.h"
#include "GS.h"

#include "Vif.h"
#include "VifDma.h"

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

extern int FIFOto_write(u32* pMem, int size);
extern void FIFOfrom_readsingle(void *value);

extern int g_nIPU0Data;
extern u8* g_pIPU0Pointer;
extern int FOreadpos;

//////////////////////////////////////////////////////////////////////////
// ReadFIFO Pages

void __fastcall ReadFIFO_page_4(u32 mem, u64 *out)
{
	jASSUME( (mem >= VIF0_FIFO) && (mem < VIF1_FIFO) );
	
	VIF_LOG("ReadFIFO/VIF0 0x%08X", mem);
	//out[0] = psHu64(mem  );
	//out[1] = psHu64(mem+8);

	out[0] = psHu64(0x4000);
	out[1] = psHu64(0x4008);
}

void __fastcall ReadFIFO_page_5(u32 mem, u64 *out)
{
	jASSUME( (mem >= VIF1_FIFO) && (mem < GIF_FIFO) );

	VIF_LOG("ReadFIFO/VIF1, addr=0x%08X", mem);

	if( vif1Regs->stat & (VIF1_STAT_INT|VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS) )
		DevCon::Notice( "Reading from vif1 fifo when stalled" );

	if (vif1Regs->stat & VIF1_STAT_FDR)
	{
		if (--psHu32(D1_QWC) == 0)
			vif1Regs->stat&= ~VIF1_STAT_FQC;
	}

	//out[0] = psHu64(mem  );
	//out[1] = psHu64(mem+8);

	out[0] = psHu64(0x5000);
	out[1] = psHu64(0x5008);
}

void __fastcall ReadFIFO_page_6(u32 mem, u64 *out)
{
	jASSUME( (mem >= GIF_FIFO) && (mem < IPUout_FIFO) );

	DevCon::Notice( "ReadFIFO/GIF, addr=0x%x", params mem );

	//out[0] = psHu64(mem  );
	//out[1] = psHu64(mem+8);

	out[0] = psHu64(0x6000);
	out[1] = psHu64(0x6008);	
}

void __fastcall ReadFIFO_page_7(u32 mem, u64 *out)
{
	jASSUME( (mem >= IPUout_FIFO) && (mem < D0_CHCR) );

	// All addresses in this page map to 0x7000 and 0x7010:
	mem &= 0x10;

	if( mem == 0 )
	{
		if( g_nIPU0Data > 0 )
		{
			out[0] = *(u64*)(g_pIPU0Pointer);
			out[1] = *(u64*)(g_pIPU0Pointer+8);
			FOreadpos = (FOreadpos + 4) & 31;
			g_nIPU0Data--;
			g_pIPU0Pointer += 16;
		}
	}
	else
		FIFOfrom_readsingle((void*)out);
}

//////////////////////////////////////////////////////////////////////////
// WriteFIFO Pages

void __fastcall WriteFIFO_page_4(u32 mem, const mem128_t *value)
{
	jASSUME( (mem >= VIF0_FIFO) && (mem < VIF1_FIFO) );

	VIF_LOG("WriteFIFO/VIF0, addr=0x%08X", mem);
	
	//psHu64(mem  ) = value[0];
	//psHu64(mem+8) = value[1];

	psHu64(0x4000) = value[0];
	psHu64(0x4008) = value[1];
	
	vif0ch->qwc += 1;
	int ret = VIF0transfer((u32*)value, 4, 0);
	assert( ret == 0 ); // vif stall code not implemented
}
		
void __fastcall WriteFIFO_page_5(u32 mem, const mem128_t *value)
{
	jASSUME( (mem >= VIF1_FIFO) && (mem < GIF_FIFO) );

	VIF_LOG("WriteFIFO/VIF1, addr=0x%08X", mem);
	
	//psHu64(mem  ) = value[0];
	//psHu64(mem+8) = value[1];

	psHu64(0x5000) = value[0];
	psHu64(0x5008) = value[1];

	if(vif1Regs->stat & VIF1_STAT_FDR)
		DevCon::Notice("writing to fifo when fdr is set!");
	if( vif1Regs->stat & (VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) )
		DevCon::Notice("writing to vif1 fifo when stalled");

	vif1ch->qwc += 1;
	int ret = VIF1transfer((u32*)value, 4, 0);
	assert( ret == 0 ); // vif stall code not implemented
}

void __fastcall WriteFIFO_page_6(u32 mem, const mem128_t *value)
{
	jASSUME( (mem >= GIF_FIFO) && (mem < IPUout_FIFO) );
	GIF_LOG("WriteFIFO/GIF, addr=0x%08X", mem);

	//psHu64(mem  ) = value[0];
	//psHu64(mem+8) = value[1];

	psHu64(0x6000) = value[0];
	psHu64(0x6008) = value[1];

	FreezeRegs(1);
	const uint count = mtgsThread->PrepDataPacket( GIF_PATH_3, value, 1 );
	jASSUME( count == 1 );
	u64* data = (u64*)mtgsThread->GetDataPacketPtr();
	data[0] = value[0];
	data[1] = value[1];
	mtgsThread->SendDataPacket();
	FreezeRegs(0);
}
		
void __fastcall WriteFIFO_page_7(u32 mem, const mem128_t *value)
{
	jASSUME( (mem >= IPUout_FIFO) && (mem < D0_CHCR) );

	// All addresses in this page map to 0x7000 and 0x7010:
	mem &= 0x10;

	IPU_LOG( "WriteFIFO/IPU, addr=0x%x", params mem );

	if( mem == 0 )
	{
		// Should this raise a PS2 exception or just ignore silently?
		Console::Notice( "WriteFIFO/IPUout (ignored)" );
	}
	else
	{
		IPU_LOG("WriteFIFO IPU_in[%d] <- %8.8X_%8.8X_%8.8X_%8.8X",
			mem/16, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);

		//committing every 16 bytes
		while( FIFOto_write((u32*)value, 1) == 0 )
		{
			Console::WriteLn("IPU sleeping");
			Threading::Timeslice();
		}
	}
}
