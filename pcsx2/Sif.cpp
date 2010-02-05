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

#define _PC_	// disables MIPS opcode macros.

#include "IopCommon.h"
#include "Sif.h"

void sifInit()
{
	memzero(sif0);
	memzero(sif1);
}

__forceinline void dmaSIF2()
{
	SIF_LOG(wxString(L"dmaSIF2" + sif2dma->cmq_to_str()).To8BitData());

	sif2dma->chcr.STR = false;
	hwDmacIrq(DMAC_SIF2);
	Console.WriteLn("*PCSX2*: dmaSIF2");
}


void SaveStateBase::sifFreeze()
{
	FreezeTag("SIFdma");

	// Nasty backwards compatability stuff.
	old_sif_structure old_sif0, old_sif1;
	bool ee_busy[2], iop_busy[2];
	
	old_sif0.fifo = sif0.fifo;
	old_sif1.fifo = sif1.fifo;
	
	ee_busy[0] = sif0.ee.busy;
	ee_busy[1] = sif1.ee.busy;
	iop_busy[0] = sif0.iop.busy;
	iop_busy[1] = sif1.iop.busy;
	
	old_sif0.end = (sif0.ee.end) ? 1 : 0;
	old_sif1.end = (sif1.iop.end) ? 1 : 0;
	
	old_sif0.counter = sif0.iop.counter;
	old_sif1.counter = sif1.iop.counter;
	
	old_sif0.data = sif0.iop.data;
	old_sif1.data = sif1.iop.data;
	
	Freeze(old_sif0);
	Freeze(old_sif1);
	Freeze(ee_busy);
	Freeze(iop_busy);
	
	// When we break save state, switch to just freezing sif0 & sif1.
	
//	Freeze(sif0);
//	Freeze(sif1);
}
