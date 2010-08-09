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
#include "IPU.h"
#include "mpeg2lib/Mpeg.h"


__aligned16 IPU_Fifo ipu_fifo;

void IPU_Fifo::init()
{
	out.readpos = 0;
	out.writepos = 0;
	in.readpos = 0;
	in.writepos = 0;
	memzero(in.data);
	memzero(out.data);
}

void IPU_Fifo_Input::clear()
{
	memzero(data);
	g_BP.IFC = 0;
	ipuRegs->ctrl.IFC = 0;
	readpos = 0;
	writepos = 0;
}

void IPU_Fifo_Output::clear()
{
	memzero(data);
	ipuRegs->ctrl.OFC = 0;
	readpos = 0;
	writepos = 0;
}

void IPU_Fifo::clear()
{
	in.clear();
	out.clear();
}

wxString IPU_Fifo_Input::desc() const
{
	return wxsFormat(L"IPU Fifo Input: readpos = 0x%x, writepos = 0x%x, data = 0x%x", readpos, writepos, data);
}

wxString IPU_Fifo_Output::desc() const
{
	return wxsFormat(L"IPU Fifo Output: readpos = 0x%x, writepos = 0x%x, data = 0x%x", readpos, writepos, data);
}

int IPU_Fifo_Input::write(u32* pMem, int size)
{
	int transsize;
	int firsttrans = min(size, 8 - (int)g_BP.IFC);

	g_BP.IFC += firsttrans;
	transsize = firsttrans;

	while (transsize-- > 0)
	{
		for (int i = 0; i <= 3; i++)
		{
			data[writepos + i] = pMem[i];
		}
		writepos = (writepos + 4) & 31;
		pMem += 4;
	}

	return firsttrans;
}

int IPU_Fifo_Output::write(const u32 *value, int size)
{
	int transsize, firsttrans;

	if ((int)ipuRegs->ctrl.OFC >= 8) IPU0dma();

	transsize = min(size, 8 - (int)ipuRegs->ctrl.OFC);
	firsttrans = transsize;

	while (transsize-- > 0)
	{
		for (int i = 0; i <= 3; i++)
		{
			data[writepos + i] = ((u32*)value)[i];
		}
		writepos = (writepos + 4) & 31;
		value += 4;
	}

	ipuRegs->ctrl.OFC += firsttrans;
	IPU0dma();

	return firsttrans;
}

int IPU_Fifo_Input::read(void *value)
{
	// wait until enough data to ensure proper streaming.
	if (g_BP.IFC < 4)
	{
		// IPU FIFO is empty and DMA is waiting so lets tell the DMA we are ready to put data in the FIFO
		if(cpuRegs.eCycle[4] == 0x9999)
		{
			CPU_INT( DMAC_TO_IPU, 4 );
		}
		
		if (g_BP.IFC == 0) return 0;
		pxAssert(g_BP.IFC > 0);
	}

	// transfer 1 qword, split into two transfers
	for (int i = 0; i <= 3; i++)
	{
		((u32*)value)[i] = data[readpos + i];
		data[readpos + i] = 0;
	}

	readpos = (readpos + 4) & 31;
	g_BP.IFC--;
	return 1;
}

void IPU_Fifo_Output::_readsingle(void *value)
{
	// transfer 1 qword, split into two transfers
	for (int i = 0; i <= 3; i++)
	{
		((u32*)value)[i] = data[readpos + i];
		data[readpos + i] = 0;
	}
	readpos = (readpos + 4) & 31;
}

void IPU_Fifo_Output::read(void *value, int size)
{
	ipuRegs->ctrl.OFC -= size;
	while (size > 0)
	{
		_readsingle(value);
		value = (u32*)value + 4;
		size--;
	}
}

void IPU_Fifo_Output::readsingle(void *value)
{
	if (ipuRegs->ctrl.OFC > 0)
	{
		ipuRegs->ctrl.OFC--;
		_readsingle(value);
	}
}

__fi bool decoder_t::ReadIpuData(u128* out)
{
	if(decoder.ipu0_data == 0) return false;
	_mm_store_ps((float*)out, _mm_load_ps((float*)GetIpuDataPtr()));

	--ipu0_data;
	++ipu0_idx;

	return true;
}

void __fastcall ReadFIFO_page_7(u32 mem, u64 *out)
{
	pxAssert( (mem >= IPUout_FIFO) && (mem < D0_CHCR) );

	// All addresses in this page map to 0x7000 and 0x7010:
	mem &= 0x10;

	if (mem == 0) // IPUout_FIFO
	{
		if (decoder.ReadIpuData((u128*)out))
		{
			ipu_fifo.out.readpos = (ipu_fifo.out.readpos + 4) & 31;
		}
	}
	else // IPUin_FIFO
		ipu_fifo.out.readsingle((void*)out);
}
