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

_sif sif1;

static bool done = false;
static int cycles = 0, psxCycles = 0;
	
static __forceinline void Sif1Init()
{
	done = false;
	cycles = 0;
	psxCycles = 0;
	//if (sif1.end == 1) SIF_LOG("Starting with sif1.end set.");
	//memzero(sif1);
	//sif1.end = 0;
	//sif1.data.data = 0;
}

// Write from the EE to Fifo.
static __forceinline bool WriteEEtoFifo()
{
	// There's some data ready to transfer into the fifo..
	
	SIF_LOG("Sif 1: Write EE to Fifo");
	const int writeSize = min((s32)sif1dma->qwc, sif1.fifo.free() >> 2);
	//if (writeSize <= 0)
	//{
		//DevCon.Warning("WriteEEtoFifo: writeSize is 0");
	//	return false;
	//}
	//else
	//{
		tDMA_TAG *ptag;
		
		ptag = sif1dma->getAddr(sif1dma->madr, DMAC_SIF1);
		if (ptag == NULL) 
		{
			DevCon.Warning("Write EE to Fifo: ptag == NULL");
			return false;
		}

		sif1.fifo.write((u32*)ptag, writeSize << 2);

		sif1dma->madr += writeSize << 4;
		cycles += writeSize;		// fixme : BIAS is factored in above
		sif1dma->qwc -= writeSize;
	//}
	return true;
}

// Read from the fifo and write to IOP
static __forceinline bool WriteFifoToIOP()
{
	// If we're reading something, continue to do so.
	
	SIF_LOG("Sif1: Write Fifo to IOP");
	const int readSize = min (sif1.counter, sif1.fifo.size);
	//if (readSize <= 0)
	//{
		//DevCon.Warning("WriteFifoToIOP: readSize is 0");
	//	return false;
	//}
	//else
	//{
		SIF_LOG("Sif 1 IOP doing transfer %04X to %08X", readSize, HW_DMA10_MADR);

		sif1.fifo.read((u32*)iopPhysMem(hw_dma(10).madr), readSize);
		psxCpu->Clear(hw_dma(10).madr, readSize);
		hw_dma(10).madr += readSize << 2;
		psxCycles += readSize >> 2;		// fixme: should be >> 4
		sif1.counter -= readSize;
	//}
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
		sif1.end = 1;
	}

	switch (ptag->ID)
	{
		case TAG_REFE:
			SIF_LOG("   REFE %08X", ptag[1]._u32);
			sif1.end = 1;
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_CNT:
			SIF_LOG("   CNT");
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = sif1dma->madr + (sif1dma->qwc << 4);
			break;

		case TAG_NEXT:
			SIF_LOG("   NEXT %08X", ptag[1]._u32);
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = ptag[1]._u32;
			break;

		case TAG_REF:
		case TAG_REFS:
			SIF_LOG("   REF %08X", ptag[1]._u32);
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_END:
			SIF_LOG("   END");
			sif1.end = 1;
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
	sif1.fifo.read((u32*)&sif1.data, 4);
	SIF_LOG("SIF 1 IOP: dest chain tag madr:%08X wc:%04X id:%X irq:%d", 
		sif1.data.data & 0xffffff, sif1.data.words, DMA_TAG(sif1.data.data).ID, 
		DMA_TAG(sif1.data.data).IRQ);
				
	hw_dma(10).madr = sif1.data.data & 0xffffff;
	sif1.counter = sif1.data.words;
	return true;
}

// Stop processing EE, and signal an interrupt.
static __forceinline void EndEE()
{
	sif1.end = 0;
	eesifbusy[1] = false;
	SIF_LOG("Sif 1: End EE");
	
	// Voodoocycles : Okami wants around 100 cycles when booting up
	// Other games reach like 50k cycles here, but the EE will long have given up by then and just retry.
	// (Cause of double interrupts on the EE)
	if (cycles == 0) 
	{
		// No transfer happened
		DevCon.Warning("SIF1 EE: cycles = 0");
	}
	else 
	{
		// Hence no Interrupt (fixes Eternal Poison reboot when selecting new game)
		CPU_INT(DMAC_SIF1, min((int)(cycles*BIAS), 384)); 
	}
}

// Stop processing IOP, and signal an interrupt.
static __forceinline void EndIOP()
{
	sif1.data.data = 0;
	iopsifbusy[1] = false;
	SIF_LOG("Sif 1: End IOP");

	//Fixme ( voodoocycles ):
	//The *24 are needed for ecco the dolphin (CDVD hangs) and silver surfer (Pad not detected)
	//Greater than *35 break rebooting when trying to play Tekken5 arcade history
	//Total cycles over 1024 makes SIF too slow to keep up the sound stream in so3...
	if (psxCycles == 0) 
	{
		// No transfer happened
		DevCon.Warning("SIF1 IOP: cycles = 0"); 
	}
	else 
	{
		// Hence no Interrupt
		PSX_INT(IopEvt_SIF1, min((psxCycles * 26), 1024)); 
	}
}

// Handle the EE transfer.
static __forceinline void HandleEETransfer()
{
#ifdef PCSX2_DEVBUILD
	if (dmacRegs->ctrl.STD == STD_SIF1)
	{
		SIF_LOG("SIF1 stall control"); // STD == fromSIF1
	}
#endif

	// If there's no more to transfer.
	if (sif1dma->qwc <= 0)
	{
		// If NORMAL mode or end of CHAIN then stop DMA.
		if ((sif1dma->chcr.MOD == NORMAL_MODE) || sif1.end)
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
		WriteEEtoFifo();
	}
}

// Handle the IOP transfer.
static __forceinline void HandleIOPTransfer()
{
	if (sif1.counter > 0)
	{
		WriteFifoToIOP();
	}
	
	if (sif1.counter <= 0)
	{
		if (sif1_tag.IRQ  || (sif1_tag.ID & 4))
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
}

// Transfer EE to IOP, putting data in the fifo as an intermediate step.
__forceinline void SIF1Dma()
{
	SIF_LOG("SIF1 DMA start...");
	Sif1Init();
	
	do
	{
		if (eesifbusy[1]) HandleEETransfer();
		if (iopsifbusy[1]) HandleIOPTransfer();
	} while (!done);
	
	SIF_LOG("SIF1 DMA end...");
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

	psHu32(SBUS_F240) |= 0x4000;
	eesifbusy[1] = true;
	
	if (iopsifbusy[1])
	{
        XMMRegisters::Freeze();
		SIF1Dma();
		psHu32(SBUS_F240) &= ~0x40;
		psHu32(SBUS_F240) &= ~0x100;
		psHu32(SBUS_F240) &= ~0x4000;
        XMMRegisters::Thaw();
	}
}
