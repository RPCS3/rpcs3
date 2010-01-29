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

static __forceinline bool SifEEWrite(int &cycles)
{
	// There's some data ready to transfer into the fifo..
		
	const int writeSize = min((s32)sif1dma->qwc, sif1.fifo.free() >> 2);
	//if (writeSize <= 0)
	//{
		//DevCon.Warning("SifEEWrite writeSize is 0");
	//	return false;
	//}
	//else
	//{
		tDMA_TAG *ptag;
		
		ptag = sif1dma->getAddr(sif1dma->madr, DMAC_SIF1);
		if (ptag == NULL) 
		{
			DevCon.Warning("SIFEEWrite: ptag == NULL");
			return false;
		}

		sif1.fifo.write((u32*)ptag, writeSize << 2);

		sif1dma->madr += writeSize << 4;
		cycles += writeSize;		// fixme : BIAS is factored in above
		sif1dma->qwc -= writeSize;
	//}
	return true;
}

static __forceinline bool SifIOPRead(int &psxCycles)
{
	// If we're reading something, continue to do so.
	const int readSize = min (sif1.counter, sif1.fifo.size);
	//if (readSize <= 0)
	//{
		//DevCon.Warning("SifIOPRead readSize is 0");
	//	return false;
	//}
	//else
	//{
		SIF_LOG(" IOP SIF doing transfer %04X to %08X", readSize, HW_DMA10_MADR);

		sif1.fifo.read((u32*)iopPhysMem(hw_dma(10).madr), readSize);
		psxCpu->Clear(hw_dma(10).madr, readSize);
		hw_dma(10).madr += readSize << 2;
		psxCycles += readSize >> 2;		// fixme: should be / 16
		sif1.counter -= readSize;
	//}
	return true;
}

static __forceinline bool SIFEEWriteTag()
{
	// Chain mode
	tDMA_TAG *ptag;
			
	// Process DMA tag at sif1dma->tadr
	ptag = sif1dma->DMAtransfer(sif1dma->tadr, DMAC_SIF1);
	if (ptag == NULL)
	{
		Console.WriteLn("SIF1EEDma: ptag = NULL");
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
		case TAG_REFE: // refe
			SIF_LOG("   REFE %08X", ptag[1]._u32);
			sif1.end = 1;
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_CNT: // cnt
			SIF_LOG("   CNT");
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = sif1dma->madr + (sif1dma->qwc << 4);
			break;

		case TAG_NEXT: // next
			SIF_LOG("   NEXT %08X", ptag[1]._u32);
			sif1dma->madr = sif1dma->tadr + 16;
			sif1dma->tadr = ptag[1]._u32;
			break;

		case TAG_REF: // ref
		case TAG_REFS: // refs
			SIF_LOG("   REF %08X", ptag[1]._u32);
			sif1dma->madr = ptag[1]._u32;
			sif1dma->tadr += 16;
			break;

		case TAG_END: // end
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

static __forceinline bool SIFIOPReadTag()
{
	// Read a tag.
	sif1.fifo.read((u32*)&sif1.data, 4);
	SIF_LOG(" IOP SIF dest chain tag madr:%08X wc:%04X id:%X irq:%d", 
		sif1.data.data & 0xffffff, sif1.data.words, DMA_TAG(sif1.data.data).ID, 
		DMA_TAG(sif1.data.data).IRQ);
				
	hw_dma(10).madr = sif1.data.data & 0xffffff;
	sif1.counter = sif1.data.words;
	return true;
}

static __forceinline void SIF1EEend(int &cycles)
{
	// Stop & signal interrupts on EE
	sif1.end = 0;
	eesifbusy[1] = false;
			
	// Voodoocycles : Okami wants around 100 cycles when booting up
	// Other games reach like 50k cycles here, but the EE will long have given up by then and just retry.
	// (Cause of double interrupts on the EE)
	if (cycles == 0) DevCon.Warning("EESIF1cycles  = 0"); // No transfer happened
	else CPU_INT(DMAC_SIF1, min((int)(cycles*BIAS), 384)); // Hence no Interrupt (fixes Eternal Poison reboot when selecting new game)
}

static __forceinline void SIF1IOPend(int &psxCycles)
{
	// Stop & signal interrupts on IOP
	sif1.data.data = 0;
	iopsifbusy[1] = false;

	//Fixme ( voodoocycles ):
	//The *24 are needed for ecco the dolphin (CDVD hangs) and silver surfer (Pad not detected)
	//Greater than *35 break rebooting when trying to play Tekken5 arcade history
	//Total cycles over 1024 makes SIF too slow to keep up the sound stream in so3...
	if (psxCycles == 0) DevCon.Warning("IOPSIF1cycles = 0"); // No transfer happened
	else PSX_INT(IopEvt_SIF1, min((psxCycles * 26), 1024)); // Hence no Interrupt
}

static __forceinline void SIF1EEDma(int &cycles, bool &done)
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
			SIF1EEend(cycles);
		}
		else
		{
			done = false;
			if (!SIFEEWriteTag()) return;
		}
	}
	else
	{
		SifEEWrite(cycles);
	}
}

static __forceinline void SIF1IOPDma(int &psxCycles, bool &done)
{
	if (sif1.counter > 0)
	{
		SifIOPRead(psxCycles);
	}
	
	if (sif1.counter <= 0)
	{
		if (sif1_tag.IRQ  || (sif1_tag.ID & 4))
		{
			done = true;
			SIF1IOPend(psxCycles);
		}
		else if (sif1.fifo.size >= 4)
		{
			
			done = false;
			SIFIOPReadTag();
		}
	}
}

__forceinline void SIF1Dma()
{
	bool done = false;
	int cycles = 0, psxCycles = 0;

	do
	{
		if (eesifbusy[1]) SIF1EEDma(cycles, done);
		if (iopsifbusy[1]) SIF1IOPDma(psxCycles, done);

	} while (!done);
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
