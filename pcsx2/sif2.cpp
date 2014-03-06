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

_sif sif2;

static bool done = false;

static __fi void Sif2Init()
{
	SIF_LOG("SIF2 DMA start... free %x iop busy %x", sif2.fifo.sif_free(), sif2.iop.busy);
	done = false;
	sif2.ee.cycles = 0;
	sif2.iop.cycles = 0;
}

__fi bool WriteFifoSingleWord()
{
	// There's some data ready to transfer into the fifo..

	SIF_LOG("Write Single word to SIF2 Fifo");
	
	sif2.fifo.write((u32*)&psxHu32(HW_PS1_GPU_DATA), 1);
	if (sif2.fifo.size > 0) psxHu32(0x1000f300) &= ~0x4000000;
	return true;
}

__fi bool ReadFifoSingleWord()
{
	u32 ptag[4];

	//SIF_LOG(" EE SIF doing transfer %04Xqw to %08X", readSize, sif2dma.madr);
	SIF_LOG("Read Fifo SIF2 Single Word IOP Busy %x Fifo Size %x SIF2 CHCR %x", sif2.iop.busy, sif2.fifo.size, HW_DMA2_CHCR);


	sif2.fifo.read((u32*)&ptag[0], 1);
	psHu32(0x1000f3e0) = ptag[0];
	if (sif2.fifo.size == 0) psxHu32(0x1000f300) |= 0x4000000;
	if (sif2.iop.busy && sif2.fifo.size <= 8)SIF2Dma();
	return true;
}

// Write from Fifo to EE.
static __fi bool WriteFifoToEE()
{
	const int readSize = min((s32)sif2dma.qwc, sif2.fifo.size >> 2);

	tDMA_TAG *ptag;

	//SIF_LOG(" EE SIF doing transfer %04Xqw to %08X", readSize, sif2dma.madr);
	SIF_LOG("Write Fifo to EE: ----------- %lX of %lX", readSize << 2, sif2dma.qwc << 2);

	ptag = sif2dma.getAddr(sif2dma.madr, DMAC_SIF2, true);
	if (ptag == NULL)
	{
		DevCon.Warning("Write Fifo to EE: ptag == NULL");
		return false;
	}

	sif2.fifo.read((u32*)ptag, readSize << 2);

	// Clearing handled by vtlb memory protection and manual blocks.
	//Cpu->Clear(sif2dma.madr, readSize*4);

	sif2dma.madr += readSize << 4;
	sif2.ee.cycles += readSize;	// fixme : BIAS is factored in above
	sif2dma.qwc -= readSize;
	
	return true;
}

// Write IOP to Fifo.
static __fi bool WriteIOPtoFifo()
{
	// There's some data ready to transfer into the fifo..
	const int writeSize = min(sif2.iop.counter, sif2.fifo.sif_free());

	SIF_LOG("Write IOP to Fifo: +++++++++++ %lX of %lX", writeSize, sif2.iop.counter);
	
	sif2.fifo.write((u32*)iopPhysMem(hw_dma2.madr), writeSize);
	hw_dma2.madr += writeSize << 2;

	// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords).
	sif2.iop.cycles += (writeSize >> 2)/* * BIAS*/;		// fixme : should be >> 4
	sif2.iop.counter -= writeSize;
	//PSX_INT(IopEvt_SIF2, sif2.iop.cycles);
	if (sif2.iop.counter == 0) hw_dma2.madr = sif2data & 0xffffff;
	if (sif2.fifo.size > 0) psxHu32(0x1000f300) &= ~0x4000000;
	return true;
}

// Read Fifo into an ee tag, transfer it to sif2dma, and process it.
static __fi bool ProcessEETag()
{
	static __aligned16 u32 tag[4];
	tDMA_TAG& ptag(*(tDMA_TAG*)tag);

	sif2.fifo.read((u32*)&tag[0], 4); // Tag
	SIF_LOG("SIF2 EE read tag: %x %x %x %x", tag[0], tag[1], tag[2], tag[3]);

	sif2dma.unsafeTransfer(&ptag);
	sif2dma.madr = tag[1];

	SIF_LOG("SIF2 EE dest chain tag madr:%08X qwc:%04X id:%X irq:%d(%08X_%08X)",
		sif2dma.madr, sif2dma.qwc, ptag.ID, ptag.IRQ, tag[1], tag[0]);

	if (sif2dma.chcr.TIE && ptag.IRQ)
	{
		//Console.WriteLn("SIF2 TIE");
		sif2.ee.end = true;
	}

	switch (ptag.ID)
	{
	case TAG_CNT:	break;

	case TAG_CNTS:
		break;

	case TAG_END:
		sif2.ee.end = true;
		break;
	}
	return true;
}

// Read Fifo into an iop tag, and transfer it to hw_dma9. And presumably process it.
static __fi bool ProcessIOPTag()
{
	//sif2.iop.data = *(sifData *)iopPhysMem(hw_dma2.madr); //comment this out and replace words below
	// Process DMA tag at hw_dma9.tadr
	if (HW_DMA2_CHCR & 0x400) DevCon.Warning("First bit %x", sif2.iop.data.data);
	
	sif2.iop.data.words = sif2.iop.data.data >> 24; // Round up to nearest 4.

	// send the EE's side of the DMAtag.  The tag is only 64 bits, with the upper 64 bits
	// ignored by the EE.
	
	// We're only copying the first 24 bits.  Bits 30 and 31 (checked below) are Stop/IRQ bits.
	
	//psxHu32(HW_PS1_GPU_DATA) += 4;
	sif2.iop.counter =  (HW_DMA2_BCR_H16 * HW_DMA2_BCR_L16); //makes it do more stuff?? //sif2words;
	/*if (HW_DMA2_CHCR & 0x400)
	{
		if (sif2.iop.counter == 0) hw_dma2.madr = sif2data & 0xFFFFFF;
		else hw_dma2.madr += 2;
	}
	else hw_dma2.madr += 2;
	// IOP tags have an IRQ bit and an End of Transfer bit:
	if ((sif2data & 0xFFFFFF) == 0xFFFFFF || (HW_DMA2_CHCR & 0x200)) */sif2.iop.end = true;
	DevCon.Warning("SIF2 IOP Tag: madr=%lx, counter=%lx (%08X_%08X)", hw_dma2.madr, sif2.iop.counter, sif2words, sif2data);

	return true;
}

// Stop transferring ee, and signal an interrupt.
static __fi void EndEE()
{
	SIF_LOG("Sif2: End EE");
	sif2.ee.end = false;
	sif2.ee.busy = false;
	if (sif2.ee.cycles == 0)
	{
		SIF_LOG("SIF2 EE: cycles = 0");
		sif2.ee.cycles = 1;
	}

	CPU_INT(DMAC_SIF2, sif2.ee.cycles*BIAS);
}

// Stop transferring iop, and signal an interrupt.
static __fi void EndIOP()
{
	SIF_LOG("Sif2: End IOP");
	sif2data = 0;
	//sif2.iop.end = false;
	sif2.iop.busy = false;

	if (sif2.iop.cycles == 0)
	{
		DevCon.Warning("SIF2 IOP: cycles = 0");
		sif2.iop.cycles = 1;
	}
	// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords)
	// So when we're all done, the equation looks like thus:
	//PSX_INT(IopEvt_SIF2, ( ( sif2.iop.cycles*BIAS ) / 4 ) / 8);
	PSX_INT(IopEvt_SIF2, sif2.iop.cycles);
}

// Handle the EE transfer.
static __fi void HandleEETransfer()
{
	if (sif2dma.chcr.STR == false)
	{
		//DevCon.Warning("Replacement for irq prevention hack EE SIF2");
		sif2.ee.end = false;
		sif2.ee.busy = false;
		return;
	}
	
	/*if (sif2dma.qwc == 0)
	if (sif2dma.chcr.MOD == NORMAL_MODE)
	if (!sif2.ee.end){
	DevCon.Warning("sif2 irq prevented");
	done = true;
	return;
	}*/

	if (sif2dma.qwc <= 0)
	{
		if ((sif2dma.chcr.MOD == NORMAL_MODE) || sif2.ee.end)
		{
			// Stop transferring ee, and signal an interrupt.
			done = true;
			EndEE();
		}
		else if (sif2.fifo.size >= 4) // Read a tag
		{
			// Read Fifo into an ee tag, transfer it to sif2dma
			// and process it.
			DevCon.Warning("SIF2 EE Chain?!");
			ProcessEETag();
		}
	}

	if (sif2dma.qwc > 0) // If we're writing something, continue to do so.
	{
		// Write from Fifo to EE.
		if (sif2.fifo.size > 0)
		{
			WriteFifoToEE();
		}
	}
}

// Handle the IOP transfer.
// Note: Test any changes in this function against Grandia III.
// What currently happens is this:
// SIF2 DMA start...
// SIF + 4 = 4 (pos=4)
// SIF2 IOP Tag: madr=19870, tadr=179cc, counter=8 (00000008_80019870)
// SIF - 4 = 0 (pos=4)
// SIF2 EE read tag: 90000002 935c0 0 0
// SIF2 EE dest chain tag madr:000935C0 qwc:0002 id:1 irq:1(000935C0_90000002)
// Write Fifo to EE: ----------- 0 of 8
// SIF - 0 = 0 (pos=4)
// Write IOP to Fifo: +++++++++++ 8 of 8
// SIF + 8 = 8 (pos=12)
// Write Fifo to EE: ----------- 8 of 8
// SIF - 8 = 0 (pos=12)
// Sif0: End IOP
// Sif0: End EE
// SIF2 DMA end...

// What happens if (sif2.iop.counter > 0) is handled first is this

// SIF2 DMA start...
// ...
// SIF + 8 = 8 (pos=12)
// Sif0: End IOP
// Write Fifo to EE: ----------- 8 of 8
// SIF - 8 = 0 (pos=12)
// SIF2 DMA end...

static __fi void HandleIOPTransfer()
{
	if (sif2.iop.counter <= 0) // If there's no more to transfer
	{
		if (sif2.iop.end)
		{
			// Stop transferring iop, and signal an interrupt.
			done = true;
			EndIOP();
		}
		else
		{
			// Read Fifo into an iop tag, and transfer it to hw_dma9.
			// And presumably process it.
			ProcessIOPTag();
		}
	}
	else
	{
		// Write IOP to Fifo.
		if (sif2.fifo.sif_free() > 0)
		{
			WriteIOPtoFifo();
		}
		else DevCon.Warning("Nothing free!");
	}
}

static __fi void Sif2End()
{
	psHu32(SBUS_F240) &= ~0x80;
	psHu32(SBUS_F240) &= ~0x8000;

	DMA_LOG("SIF2 DMA End");
}

// Transfer IOP to EE, putting data in the fifo as an intermediate step.
__fi void SIF2Dma()
{
	int BusyCheck = 0;
	Sif2Init();

	do
	{
		//I realise this is very hacky in a way but its an easy way of checking if both are doing something
		BusyCheck = 0;

		if (sif2.iop.busy)
		{
			if (sif2.fifo.sif_free() > 0 || (sif2.iop.end == true && sif2.iop.counter == 0))
			{
				BusyCheck++;
				HandleIOPTransfer();
			}
		}
		if (sif2.ee.busy)
		{
			if (sif2.fifo.size >= 4 || (sif2.ee.end == true && sif2dma.qwc == 0))
			{
				BusyCheck++;
				HandleEETransfer();
			}
		}
	} while (/*!done && */BusyCheck > 0); // Substituting (sif2.ee.busy || sif2.iop.busy) breaks things.

	Sif2End();
}

__fi void  sif2Interrupt()
{
	if (sif2.iop.end == false || sif2.iop.counter > 0)
	{
		SIF2Dma();
		return;
	}
	
	SIF_LOG("SIF2 IOP Intr end");
	HW_DMA2_CHCR &= ~0x01000000;
	psxDmaInterrupt2(2);
}

__fi void  EEsif2Interrupt()
{
	hwDmacIrq(DMAC_SIF2);
	sif2dma.chcr.STR = false;
}

__fi void dmaSIF2()
{
	DevCon.Warning("SIF2 EE CHCR %x", sif2dma.chcr._u32);
	SIF_LOG(wxString(L"dmaSIF2" + sif2dma.cmqt_to_str()).To8BitData());

	if (sif2.fifo.readPos != sif2.fifo.writePos)
	{
		SIF_LOG("warning, sif2.fifoReadPos != sif2.fifoWritePos");
	}

	//if(sif2dma.chcr.MOD == CHAIN_MODE && sif2dma.qwc > 0) DevCon.Warning(L"SIF2 QWC on Chain CHCR " + sif2dma.chcr.desc());
	psHu32(SBUS_F240) |= 0x8000;
	sif2.ee.busy = true;

	// Okay, this here is needed currently (r3644). 
	// FFX battles in the thunder plains map die otherwise, Phantasy Star 4 as well
	// These 2 games could be made playable again by increasing the time the EE or the IOP run,
	// showing that this is very timing sensible.
	// Doing this DMA unfortunately brings back an old warning in Legend of Legaia though, but it still works.

	//Updated 23/08/2011: The hangs are caused by the EE suspending SIF1 DMA and restarting it when in the middle 
	//of processing a "REFE" tag, so the hangs can be solved by forcing the ee.end to be false
	// (as it should always be at the beginning of a DMA).  using "if iop is busy" flags breaks Tom Clancy Rainbow Six.
	// Legend of Legaia doesn't throw a warning either :)
	//sif2.ee.end = false;
	
	SIF2Dma();

}
