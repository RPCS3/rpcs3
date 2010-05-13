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

	Freeze(sif0);
	Freeze(sif1);
}
