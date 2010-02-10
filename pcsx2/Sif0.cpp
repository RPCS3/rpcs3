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

_sif sif0;

static bool done = false;

static __forceinline void Sif0Init()
{
	done = false;
	sif0.ee.cycles = 0;
	sif0.iop.cycles = 0;
}

// Write from Fifo to EE.
static __forceinline bool WriteFifoToEE()
{
	const int readSize = min((s32)sif0dma->qwc, sif0.fifo.size >> 2);
	//if (readSize > 0)
	//{
		tDMA_TAG *ptag;
		
		//SIF_LOG(" EE SIF doing transfer %04Xqw to %08X", readSize, sif0dma->madr);
		SIF_LOG("Write Fifo to EE: ----------- %lX of %lX", readSize << 2, sif0dma->qwc << 2);

		ptag = sif0dma->getAddr(sif0dma->madr, DMAC_SIF0);
		if (ptag == NULL)
		{
			DevCon.Warning("Write Fifo to EE: ptag == NULL");
			return false;
		}

		sif0.fifo.read((u32*)ptag, readSize << 2);

		// Clearing handled by vtlb memory protection and manual blocks.
		//Cpu->Clear(sif0dma->madr, readSize*4);

		sif0dma->madr += readSize << 4;
		sif0.ee.cycles += readSize;	// fixme : BIAS is factored in above
		sif0dma->qwc -= readSize;
	//}
	//else
	//{
		//DevCon.Warning("Write Fifo to EE: readSize is 0");
	//	return false;
	//}
	return true;
}

// Write IOP to Fifo.
static __forceinline bool WriteIOPtoFifo()
{
	// There's some data ready to transfer into the fifo..
	const int writeSize = min(sif0.iop.counter, sif0.fifo.free());
	
	//if (writeSize <= 0)
	//{
		//DevCon.Warning("Write IOP to Fifo: writeSize is 0"); 
	//	return false;
	//}
	//else
	//{
	SIF_LOG("Write IOP to Fifo: +++++++++++ %lX of %lX", writeSize, sif0.iop.counter);

	sif0.fifo.write((u32*)iopPhysMem(hw_dma(9).madr), writeSize);
	hw_dma(9).madr += writeSize << 2;
	
	// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords).
	sif0.iop.cycles += (writeSize >> 2) * BIAS;		// fixme : should be >> 4
	sif0.iop.counter -= writeSize;
	//}
	return true;
}

// Read Fifo into an ee tag, transfer it to sif0dma, and process it.
static __forceinline bool ProcessEETag()
{
	static __aligned16 u32 tag[4];
			
	sif0.fifo.read((u32*)&tag[0], 4); // Tag
	SIF_LOG("SIF0 EE read tag: %x %x %x %x", tag[0], tag[1], tag[2], tag[3]);

	sif0dma->unsafeTransfer(((tDMA_TAG*)(tag)));
	sif0dma->madr = tag[1];
	tDMA_TAG ptag(tag[0]);

	SIF_LOG("SIF0 EE dest chain tag madr:%08X qwc:%04X id:%X irq:%d(%08X_%08X)", 
		sif0dma->madr, sif0dma->qwc, ptag.ID, ptag.IRQ, tag[1], tag[0]);
	
	if (sif0dma->chcr.TIE && ptag.IRQ)
	{
		//Console.WriteLn("SIF0 TIE");
		sif0.ee.end = true;
	}
					
	switch (ptag.ID)
	{
		case TAG_REFE:
			sif0.ee.end = true;
			if (dmacRegs->ctrl.STS != NO_STS) 
				dmacRegs->stadr.ADDR = sif0dma->madr + (sif0dma->qwc * 16);
				break;
                            
		case TAG_REFS:
			if (dmacRegs->ctrl.STS != NO_STS) 
				dmacRegs->stadr.ADDR = sif0dma->madr + (sif0dma->qwc * 16);
				break;

		case TAG_END:
			sif0.ee.end = true;
			break;
	}
	return true;
}

// Read Fifo into an iop tag, and transfer it to hw_dma(9). And presumably process it.
static __forceinline bool ProcessIOPTag()
{
	// Process DMA tag at hw_dma(9).tadr
	sif0.iop.data = *(sifData *)iopPhysMem(hw_dma(9).tadr);
	sif0.iop.data.words = (sif0.iop.data.words + 3) & 0xfffffffc; // Round up to nearest 4.
	sif0.fifo.write((u32*)iopPhysMem(hw_dma(9).tadr + 8), 4);

	hw_dma(9).tadr += 16; ///hw_dma(9).madr + 16 + sif0.sifData.words << 2;
	
	// Looks like we are only copying the first 24 bits.
#ifdef CHOP_OFF_DATA
	hw_dma(9).madr = sif0data & 0xFFFFFF;
#else
	hw_dma(9).madr = sif0data;
#endif
	sif0.iop.counter = sif0words;
	//if (sif0words != ( sif0words & 0xFFFFFF)) DevCon.WriteLn("sif0words more then 24 bit.");
	
	if (sif0tag.IRQ  || (sif0tag.ID & 4)) sif0.iop.end = true;
	SIF_LOG("SIF0 IOP Tag: madr=%lx, tadr=%lx, counter=%lx (%08X_%08X)", hw_dma(9).madr, hw_dma(9).tadr, sif0.iop.counter, sif0words, sif0data);
	return true;
}

// Stop transferring ee, and signal an interrupt.
static __forceinline void EndEE()
{
	SIF_LOG("Sif0: End EE");
	sif0.ee.end = false;
	sif0.ee.busy = false;
	if (sif0.ee.cycles == 0) 
	{
		 // No transfer happened,
		DevCon.Warning("SIF0 EE: cycles = 0");
	}
	else 
	{
		// hence no Interrupt.
		CPU_INT(DMAC_SIF0, sif0.ee.cycles*BIAS); 
	}
}

// Stop transferring iop, and signal an interrupt.
static __forceinline void EndIOP()
{
	SIF_LOG("Sif0: End IOP");
	sif0data = 0;
	sif0.iop.end = false;
	sif0.iop.busy = false;
					
	if (sif0.iop.cycles == 0) 
	{
		// No transfer happened,
		DevCon.Warning("SIF0 IOP: cycles = 0"); 
	}
	else 
	{
		 // hence no Interrupt.
		// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords)
		// So when we're all done, the equation looks like thus:
		//PSX_INT(IopEvt_SIF0, ( ( sif0.iop.cycles*BIAS ) / 4 ) / 8);
		PSX_INT(IopEvt_SIF0, sif0.iop.cycles);
	}
}

// Handle the EE transfer.
static __forceinline void HandleEETransfer()
{
#ifdef PCSX2_DEVBUILD
	if (dmacRegs->ctrl.STS == STS_SIF0)
	{
		SIF_LOG("SIF0 stall control");
	}
#endif
	
	if (sif0dma->qwc <= 0)
	{
		if ((sif0dma->chcr.MOD == NORMAL_MODE) || sif0.ee.end)
		{
			// Stop transferring ee, and signal an interrupt.
			done = true;
			EndEE();
		}
		else if (sif0.fifo.size >= 4) // Read a tag
		{
			// Read Fifo into an ee tag, transfer it to sif0dma
			// and process it.
			ProcessEETag();
		}
	}
	
	if (sif0dma->qwc > 0) // If we're writing something, continue to do so.
	{
		// Write from Fifo to EE.
		WriteFifoToEE();
	}
}

// Handle the IOP transfer.
// Note: Test any changes in this function against Grandia III.
// What currently happens is this:
// SIF0 DMA start...
// SIF + 4 = 4 (pos=4)
// SIF0 IOP Tag: madr=19870, tadr=179cc, counter=8 (00000008_80019870)
// SIF - 4 = 0 (pos=4)
// SIF0 EE read tag: 90000002 935c0 0 0
// SIF0 EE dest chain tag madr:000935C0 qwc:0002 id:1 irq:1(000935C0_90000002)
// Write Fifo to EE: ----------- 0 of 8
// SIF - 0 = 0 (pos=4)
// Write IOP to Fifo: +++++++++++ 8 of 8
// SIF + 8 = 8 (pos=12)
// Write Fifo to EE: ----------- 8 of 8
// SIF - 8 = 0 (pos=12)
// Sif0: End IOP
// Sif0: End EE
// SIF0 DMA end...

// What happens if (sif0.iop.counter > 0) is handled first is this

// SIF0 DMA start...
// ...
// SIF + 8 = 8 (pos=12)
// Sif0: End IOP
// Write Fifo to EE: ----------- 8 of 8
// SIF - 8 = 0 (pos=12)
// SIF0 DMA end...

static __forceinline void HandleIOPTransfer()
{
	if (sif0.iop.counter <= 0) // If there's no more to transfer
	{
		if (sif0.iop.end)
		{
			// Stop transferring iop, and signal an interrupt.
			done = true;
			EndIOP();
		}
		else
		{
			// Read Fifo into an iop tag, and transfer it to hw_dma(9). 
			// And presumably process it.
			ProcessIOPTag();
		}
	}
	else
	{
		// Write IOP to Fifo.
		WriteIOPtoFifo();
	}
}

static __forceinline void Sif0End()
{
}

// Transfer IOP to EE, putting data in the fifo as an intermediate step.
__forceinline void SIF0Dma()
{
	SIF_LOG("SIF0 DMA start...");
	Sif0Init();
	
	do
	{
		if (sif0.iop.busy) HandleIOPTransfer();
		if (sif0.ee.busy) HandleEETransfer();
	} while (!done); // Substituting (sif0.ee.busy || sif0.iop.busy) breaks things.
	
	SIF_LOG("SIF0 DMA end...");
	Sif0End();
}

__forceinline void  sif0Interrupt()
{
	HW_DMA9_CHCR &= ~0x01000000;
	psxDmaInterrupt2(2);
}

__forceinline void  EEsif0Interrupt()
{
	hwDmacIrq(DMAC_SIF0);
	sif0dma->chcr.STR = false;
}

__forceinline void dmaSIF0()
{
	SIF_LOG(wxString(L"dmaSIF0" + sif0dma->cmqt_to_str()).To8BitData());
	
	if (sif0.fifo.readPos != sif0.fifo.writePos)
	{
		SIF_LOG("warning, sif0.fifoReadPos != sif0.fifoWritePos");
	}

	psHu32(SBUS_F240) |= 0x2000;
	sif0.ee.busy = true;
	
	if (sif0.iop.busy)
	{
        XMMRegisters::Freeze();
		hwIntcIrq(INTC_SBUS);
		SIF0Dma();
		psHu32(SBUS_F240) &= ~0x20;
		psHu32(SBUS_F240) &= ~0x2000;
        XMMRegisters::Thaw();
	}
}
