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

_sif sif1;

static bool done = false;

static __forceinline void Sif1Init()
{
	SIF_LOG("SIF1 DMA start...");
	done = false;
	sif1.ee.cycles = 0;
	sif1.iop.cycles = 0;
}

// Write from the EE to Fifo.
static __forceinline bool WriteEEtoFifo()
{
	// There's some data ready to transfer into the fifo..

	SIF_LOG("Sif 1: Write EE to Fifo");
	const int writeSize = min((s32)sif1dma->qwc, sif1.fifo.free() >> 2);

	tDMA_TAG *ptag;

	ptag = sif1dma->getAddr(sif1dma->madr, DMAC_SIF1, false);
	if (ptag == NULL)
	{
		DevCon.Warning("Write EE to Fifo: ptag == NULL");
		return false;
	}

	sif1.fifo.write((u32*)ptag, writeSize << 2);

	sif1dma->madr += writeSize << 4;
	sif1.ee.cycles += writeSize;		// fixme : BIAS is factored in above
	sif1dma->qwc -= writeSize;

	return true;
}

// Read from the fifo and write to IOP
static __forceinline bool WriteFifoToIOP()
{
	// If we're reading something, continue to do so.

	SIF_LOG("Sif1: Write Fifo to IOP");
	const int readSize = min (sif1.iop.counter, sif1.fifo.size);

	SIF_LOG("Sif 1 IOP doing transfer %04X to %08X", readSize, HW_DMA10_MADR);

	sif1.fifo.read((u32*)iopPhysMem(hw_dma(10).madr), readSize);
	psxCpu->Clear(hw_dma(10).madr, readSize);
	hw_dma(10).madr += readSize << 2;
	sif1.iop.cycles += readSize >> 2;		// fixme: should be >> 4
	sif1.iop.counter -= readSize;

	return true;
}

// Get a tag and process it.
static __forceinline bool ProcessEETag()
{
	// Chain mode
	tDMA_TAG *ptag;
	SIF_LOG("Sif1: ProcessEETag");

	// Process DMA tag at sif1dma->tadr
	ptag = sif1dma->DMAtransfer(sif1dma->tadr, DMAC_SIF1);
	if (ptag == NULL)
	{
		Console.WriteLn("Sif1 ProcessEETag: ptag = NULL");
		return false;
	}

	if (sif1dma->chcr.TTE)
	{
		Console.WriteLn("SIF1 TTE");
		sif1.fifo.write((u32*)ptag + 2, 2);
	}

	if (sif1dma->chcr.TIE && ptag->IRQ)
	{
		Console.WriteLn("SIF1 TIE");
		sif1.ee.end = true;
	}

	SIF_LOG(wxString(ptag->tag_to_str()).To8BitData());
	switch (ptag->ID)
	{
		case TAG_REFE:
			sif1.ee.end = true;
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_CNT:
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = sif1dma->madr + (sif1dma->qwc << 4);
			break;

		case TAG_NEXT:
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = ptag[1]._u32;
			break;

		case TAG_REF:
		case TAG_REFS:
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_END:
			sif1.ee.end = true;
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = sif1dma->madr + (sif1dma->qwc << 4);
			break;

		default:
			Console.WriteLn("Bad addr1 source chain");
	}
	return true;
}

// Write fifo to data, and put it in IOP.
static __forceinline bool SIFIOPReadTag()
{
	// Read a tag.
	sif1.fifo.read((u32*)&sif1.iop.data, 4);
	//sif1words = (sif1words + 3) & 0xfffffffc; // Round up to nearest 4.
	SIF_LOG("SIF 1 IOP: dest chain tag madr:%08X wc:%04X id:%X irq:%d",
		sif1data & 0xffffff, sif1words, sif1tag.ID, sif1tag.IRQ);

	// Only use the first 24 bits.
	hw_dma(10).madr = sif1data & 0xffffff;

	sif1.iop.counter = sif1words;
	if (sif1tag.IRQ  || (sif1tag.ID & 4)) sif1.iop.end = true;

	return true;
}

// Stop processing EE, and signal an interrupt.
static __forceinline void EndEE()
{
	sif1.ee.end = false;
	sif1.ee.busy = false;
	SIF_LOG("Sif 1: End EE");

	// Voodoocycles : Okami wants around 100 cycles when booting up
	// Other games reach like 50k cycles here, but the EE will long have given up by then and just retry.
	// (Cause of double interrupts on the EE)
	if (sif1.ee.cycles == 0)
	{
		SIF_LOG("SIF1 EE: cycles = 0");
		sif1.ee.cycles = 1;
	}


	CPU_INT(DMAC_SIF1, /*min((int)(*/sif1.ee.cycles*BIAS/*), 384)*/);
}

// Stop processing IOP, and signal an interrupt.
static __forceinline void EndIOP()
{
	sif1data = 0;
	sif1.iop.end = false;
	sif1.iop.busy = false;
	SIF_LOG("Sif 1: End IOP");

	//Fixme ( voodoocycles ):
	//The *24 are needed for ecco the dolphin (CDVD hangs) and silver surfer (Pad not detected)
	//Greater than *35 break rebooting when trying to play Tekken5 arcade history
	//Total cycles over 1024 makes SIF too slow to keep up the sound stream in so3...
	if (sif1.iop.cycles == 0)
	{
		DevCon.Warning("SIF1 IOP: cycles = 0");
		sif1.iop.cycles = 1;
	}
	// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords)
	PSX_INT(IopEvt_SIF1, /*min((*/sif1.iop.cycles/* * 26*//*), 1024)*/);
}

// Handle the EE transfer.
static __forceinline void HandleEETransfer()
{
	if(sif1dma->chcr.STR == false)
	{
		DevCon.Warning("Replacement for irq prevention hack EE SIF1");
		sif1.ee.end = false;
		sif1.ee.busy = false;
		return;
	}
	if (dmacRegs->ctrl.STD == STD_SIF1)
	{
		DevCon.Warning("SIF1 stall control"); // STD == fromSIF1
	}

	/*if (sif1dma->qwc == 0)
		if (sif1dma->chcr.MOD == NORMAL_MODE)
			if (!sif1.ee.end){
				DevCon.Warning("sif1 irq prevented CHCR %x QWC %x", sif1dma->chcr, sif1dma->qwc);
				done = true;
				return;
			}*/

	// If there's no more to transfer.
	if (sif1dma->qwc <= 0)
	{
		// If NORMAL mode or end of CHAIN then stop DMA.
		if ((sif1dma->chcr.MOD == NORMAL_MODE) || sif1.ee.end)
		{
			done = true;
			EndEE();
		}
		else
		{
			done = false;
			if (!ProcessEETag()) return;
		}
	}
	else
	{
		if (sif1.fifo.free() > 0)
		{
			WriteEEtoFifo();
		}
	}
}

// Handle the IOP transfer.
static __forceinline void HandleIOPTransfer()
{
	if (sif1.iop.counter > 0)
	{
		if (sif1.fifo.size > 0)
		{
			WriteFifoToIOP();
		}
	}

	if (sif1.iop.counter <= 0)
	{
		if (sif1.iop.end)
		{
			done = true;
			EndIOP();
		}
		else if (sif1.fifo.size >= 4)
		{

			done = false;
			SIFIOPReadTag();
		}
	}
}

static __forceinline void Sif1End()
{
	SIF_LOG("SIF1 DMA end...");
}

// Transfer EE to IOP, putting data in the fifo as an intermediate step.
__forceinline void SIF1Dma()
{
	int BusyCheck = 0;
	Sif1Init();

	do
	{
		//I realise this is very hacky in a way but its an easy way of checking if both are doing something
		BusyCheck = 0;

		if (sif1.ee.busy)
		{
			if(sif1.fifo.free() > 0 || (sif1.ee.end == true && sif1dma->qwc == 0)) 
			{
				BusyCheck++;
				HandleEETransfer();
			}
		}

		if (sif1.iop.busy)
		{
			if(sif1.fifo.size >= 4 || (sif1.iop.end == true && sif1.iop.counter == 0)) 
			{
				BusyCheck++;
				HandleIOPTransfer();
			}
		}

	} while (/*!done &&*/ BusyCheck > 0);

	Sif1End();
}

__forceinline void  sif1Interrupt()
{
	HW_DMA10_CHCR &= ~0x01000000; //reset TR flag
	psxDmaInterrupt2(3);
}

__forceinline void  EEsif1Interrupt()
{
	hwDmacIrq(DMAC_SIF1);
	sif1dma->chcr.STR = false;
}

// Do almost exactly the same thing as psxDma10 in IopDma.cpp.
// Main difference is this checks for iop, where psxDma10 checks for ee.
__forceinline void dmaSIF1()
{
	SIF_LOG(wxString(L"dmaSIF1" + sif1dma->cmqt_to_str()).To8BitData());

	if (sif1.fifo.readPos != sif1.fifo.writePos)
	{
		SIF_LOG("warning, sif1.fifoReadPos != sif1.fifoWritePos");
	}

	//if(sif1dma->chcr.MOD == CHAIN_MODE && sif1dma->qwc > 0) DevCon.Warning(L"SIF1 QWC on Chain CHCR " + sif1dma->chcr.desc());

	psHu32(SBUS_F240) |= 0x4000;
	sif1.ee.busy = true;

	/*if (sif1.iop.busy)
	{*/
        XMMRegisters::Freeze();
		SIF1Dma();
		psHu32(SBUS_F240) &= ~0x40;
		psHu32(SBUS_F240) &= ~0x100;
		psHu32(SBUS_F240) &= ~0x4000;
        XMMRegisters::Thaw();
	//}
}
