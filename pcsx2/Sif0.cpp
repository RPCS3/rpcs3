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

static __forceinline bool SifEERead(int &cycles)
{
	const int readSize = min((s32)sif0dma->qwc, sif0.fifo.size >> 2);
	//if (readSize <= 0)
	//{
		tDMA_TAG *ptag;
		
		//SIF_LOG(" EE SIF doing transfer %04Xqw to %08X", readSize, sif0dma->madr);
		SIF_LOG("----------- %lX of %lX", readSize << 2, sif0dma->qwc << 2);

		ptag = sif0dma->getAddr(sif0dma->madr, DMAC_SIF0);
		if (ptag == NULL)
		{
			DevCon.Warning("SIFEERead: ptag == NULL");
			return false;
		}

		sif0.fifo.read((u32*)ptag, readSize << 2);

		// Clearing handled by vtlb memory protection and manual blocks.
		//Cpu->Clear(sif0dma->madr, readSize*4);

		sif0dma->madr += readSize << 4;
		cycles += readSize;	// fixme : BIAS is factored in above
		sif0dma->qwc -= readSize;
	//}
	//else
	//{
		//DevCon.Warning("SifEERead readSize is 0");
	//	return false;
	//}
	return true;
}

static __forceinline bool SifIOPWrite(int &psxCycles)
{
	// There's some data ready to transfer into the fifo..
	const int writeSize = min(sif0.counter, sif0.fifo.free());
	
	//if (writeSize <= 0)
	//{
		//DevCon.Warning("SifIOPWrite writeSize is 0"); 
	//	return false;
	//}
	//else
	//{
	SIF_LOG("+++++++++++ %lX of %lX", writeSize, sif0.counter);

	sif0.fifo.write((u32*)iopPhysMem(hw_dma(9).madr), writeSize);
	hw_dma(9).madr += writeSize << 2;
	psxCycles += (writeSize >> 2) * BIAS;		// fixme : should be >> 4
	sif0.counter -= writeSize;
	//}
	return true;
}

static __forceinline bool SIFEEReadTag()
{
	static __aligned16 u32 tag[4];
			
	sif0.fifo.read((u32*)&tag[0], 4); // Tag
	SIF_LOG(" EE SIF read tag: %x %x %x %x", tag[0], tag[1], tag[2], tag[3]);

	sif0dma->unsafeTransfer(((tDMA_TAG*)(tag)));
	sif0dma->madr = tag[1];
	tDMA_TAG ptag(tag[0]);

	SIF_LOG(" EE SIF dest chain tag madr:%08X qwc:%04X id:%X irq:%d(%08X_%08X)", 
		sif0dma->madr, sif0dma->qwc, ptag.ID, ptag.IRQ, tag[1], tag[0]);
	
	if (sif0dma->chcr.TIE && ptag.IRQ)
	{
		//Console.WriteLn("SIF0 TIE");
		sif0.end = 1;
	}
					
	switch (ptag.ID)
	{
		case TAG_REFE:
			sif0.end = 1;
			if (dmacRegs->ctrl.STS != NO_STS) 
				dmacRegs->stadr.ADDR = sif0dma->madr + (sif0dma->qwc * 16);
				break;
                            
		case TAG_REFS:
			if (dmacRegs->ctrl.STS != NO_STS) 
				dmacRegs->stadr.ADDR = sif0dma->madr + (sif0dma->qwc * 16);
				break;

		case TAG_END:
			sif0.end = 1;
			break;
	}
	return true;
}

static __forceinline bool SIFIOPWriteTag()
{
	// Process DMA tag at HW_DMA9_TADR
	sif0.data = *(sifData *)iopPhysMem(hw_dma(9).tadr);
	sif0.data.words = (sif0.data.words + 3) & 0xfffffffc; // Round up to nearest 4.
	sif0.fifo.write((u32*)iopPhysMem(hw_dma(9).tadr + 8), 4);

	hw_dma(9).tadr += 16; ///hw_dma(9).madr + 16 + sif0.sifData.words << 2;
	hw_dma(9).madr = sif0.data.data & 0xFFFFFF;
	sif0.counter = sif0.data.words & 0xFFFFFF;

	SIF_LOG(" SIF0 Tag: madr=%lx, tadr=%lx, counter=%lx (%08X_%08X)", HW_DMA9_MADR, HW_DMA9_TADR, sif0.counter, sif0.data.words, sif0.data.data);

	return true;
}

static __forceinline void SIF0EEend(int &cycles)
{
	// Stop & signal interrupts on EE
	sif0.end = 0;
	eesifbusy[0] = false;
	if (cycles == 0) DevCon.Warning("EESIF0cycles = 0"); // No transfer happened
	else CPU_INT(DMAC_SIF0, cycles*BIAS); // Hence no Interrupt
}

static __forceinline void SIF0IOPend(int &psxCycles)
{
	// Stop & signal interrupts on IOP
	sif0.data.data = 0;
	iopsifbusy[0] = false;
					
	// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords)
	// So when we're all done, the equation looks like thus:
	//PSX_INT(IopEvt_SIF0, ( ( psxCycles*BIAS ) / 4 ) / 8);
	if (psxCycles == 0) DevCon.Warning("IOPSIF0cycles = 0"); // No transfer happened
	else PSX_INT(IopEvt_SIF0, psxCycles); // Hence no Interrupt
}

static __forceinline void SIF0EEDma(int &cycles, bool &done)
{
#ifdef PCSX2_DEVBUILD
	if (dmacRegs->ctrl.STS == STS_SIF0)
	{
		SIF_LOG("SIF0 stall control");
	}
#endif
	
	if (sif0dma->qwc <= 0)
	{
		if ((sif0dma->chcr.MOD == NORMAL_MODE) || sif0.end)
		{
			done = true;
			SIF0EEend(cycles);
		}
		else if (sif0.fifo.size >= 4) // Read a tag
		{
			done = false;
			SIFEEReadTag();
		}
	}
	
	if (sif0dma->qwc > 0) // If we're reading something continue to do so
	{
		SifEERead(cycles);
	}
}

// Note: Test any changes in this function against Grandia III.
static __forceinline void SIF0IOPDma(int &psxCycles, bool &done)
{
	if (sif0.counter <= 0) // If there's no more to transfer
	{
		if (sif0_tag.IRQ  || (sif0_tag.ID & 4))
		{
			done = true;
			SIF0IOPend(psxCycles);
		}
		else  // Chain mode
		{
			done = false;
			SIFIOPWriteTag();
		}
	}
	else
	{
		SifIOPWrite(psxCycles);
	}
}

__forceinline void SIF0Dma()
{
	bool done = false;
	int cycles = 0, psxCycles = 0;

	SIF_LOG("SIF0 DMA start...");
	
	do
	{
		if (iopsifbusy[0]) SIF0IOPDma(psxCycles, done);
		if (eesifbusy[0]) SIF0EEDma(cycles, done);
	} while (!done);
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
	eesifbusy[0] = true;
	
	if (iopsifbusy[0])
	{
        XMMRegisters::Freeze();
		hwIntcIrq(INTC_SBUS);
		SIF0Dma();
		psHu32(SBUS_F240) &= ~0x20;
		psHu32(SBUS_F240) &= ~0x2000;
        XMMRegisters::Thaw();
	}
}
