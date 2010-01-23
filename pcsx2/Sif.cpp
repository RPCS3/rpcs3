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

DMACh *sif0ch, *sif1ch, *sif2ch;
static _sif sif0, sif1;

bool eesifbusy[2] = { false, false };
extern bool iopsifbusy[2];

void sifInit()
{
	memzero(sif0);
	memzero(sif1);
	memzero(eesifbusy);
	memzero(iopsifbusy);
}

//General format of all the SIF#(EE/IOP)Dma functions is this:
//First, SIF#Dma does a while loop, calling SIF#EEDma if EE is active,
//and then calling SIF#IOPDma if IOP is active.
//
//What the EE function does is the following:
//Check if qwc <= 0. If it is, check if we have any data left to pull.
//If we don't, set EE not to be active, call CPU_INT, and get out of the loop.
//If we do, pass data from fifo to ee if sif 0, and to ee from fifo if sif 1.
//And if qwc > 0, we do much the same thing.
//
//The IOP function is similar:
//We check if counter <= 0. If it is, check if we have data left to pull.
//If we don't, set IOP not to be active, call PSX_INT, and get out of the loop.
//If we do, pass data from fifo to iop if sif 0, and to iop from fifo if sif 1.
//And if qwc > 0, we do much the same thing.
//
// And with the IOP function, counter would be 0 initially, so the initialization
// is done in the same section of code as the exit, which is odd.
//
// This is, of course, simplified, and more study of these functions is needed.

__forceinline void SIF0EEDma(int &cycles, int &psxCycles, bool &done)
{
	if (dmacRegs->ctrl.STS == STS_SIF0)
	{
		SIF_LOG("SIF0 stall control");
	}
	
	if (sif0dma->qwc <= 0)
	{
		// Stop if TIE & the IRQ are set, or at the end.
		// Remind me to look closer at this. (the IRQ tag of a chcr?)
		if ((sif0dma->chcr.TIE && DMA_TAG(sif0dma->chcr._u32).IRQ) || sif0.end)
		{
			if (sif0.end)
				SIF_LOG(" EE SIF end"); 
			else
				SIF_LOG(" EE SIF interrupt");
				
			eesifbusy[0] = false;
			done = true;

			CPU_INT(5, cycles*BIAS);
		}
		else if (sif0.fifo.size >= 4) // Read a tag
		{
			static __aligned16 u32 tag[4];
					
			sif0.fifo.read((u32*)&tag[0], 4); // Tag
			SIF_LOG(" EE SIF read tag: %x %x %x %x", tag[0], tag[1], tag[2], tag[3]);

			sif0dma->unsafeTransfer(((tDMA_TAG*)(tag)));
			sif0dma->madr = tag[1];

			SIF_LOG(" EE SIF dest chain tag madr:%08X qwc:%04X id:%X irq:%d(%08X_%08X)", sif0dma->madr, sif0dma->qwc, (tag[0] >> 28)&3, (tag[0] >> 31)&1, tag[1], tag[0]);
					
			switch (DMA_TAG(tag[0]).ID)
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
					
			done = false;
		}
	}
	
	if (sif0dma->qwc > 0) // If we're reading something continue to do so
	{
		tDMA_TAG *ptag;
		int readSize = min((s32)sif0dma->qwc, (sif0.fifo.size >> 2));

		//SIF_LOG(" EE SIF doing transfer %04Xqw to %08X", readSize, sif0dma->madr);
		SIF_LOG("----------- %lX of %lX", readSize << 2, sif0dma->qwc << 2);

		ptag = safeDmaGetAddr(sif0dma, sif0dma->madr, DMAC_SIF0);
		if (ptag == NULL) return;

		sif0.fifo.read((u32*)ptag, readSize << 2);

		// Clearing handled by vtlb memory protection and manual blocks.
		//Cpu->Clear(sif0dma->madr, readSize*4);

		cycles += readSize;	// fixme : BIAS is factored in below
		sif0dma->qwc -= readSize;
		sif0dma->madr += readSize << 4;
	}
}

__forceinline void SIF1EEDma(int &cycles, int &psxCycles, bool &done)
{
	if (dmacRegs->ctrl.STD == STD_SIF1)
	{
		SIF_LOG("SIF1 stall control"); // STD == fromSIF1
	}

	// If there's no more to transfer.
	if (sif1dma->qwc <= 0)
	{
		// If NORMAL mode or end of CHAIN then stop DMA.
		if ((sif1dma->chcr.MOD == NORMAL_MODE) || sif1.end)
		{
			SIF_LOG("EE SIF1 End %x", sif1.end);
			
			// Stop & signal interrupts on EE
			sif1.end = 0;
			eesifbusy[1] = false;
			done = true;
			
			// Voodoocycles : Okami wants around 100 cycles when booting up
			// Other games reach like 50k cycles here, but the EE will long have given up by then and just retry.
			// (Cause of double interrupts on the EE)
			CPU_INT(6, min( (int)(cycles*BIAS), 384 ) );
		}
		else
		{
			// Chain mode
			tDMA_TAG *ptag;
			
			// Process DMA tag at sif1dma->tadr
			done = false;
			ptag = safeDmaGetAddr(sif1dma, sif1dma->tadr, DMAC_SIF1);
			if (ptag == NULL) return;
					
			sif1dma->unsafeTransfer(ptag);

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
		}
	}
	if (sif1dma->qwc > 0)
	{
		// There's some data ready to transfer into the fifo..
		tDMA_TAG *pTag;
		
		const int qwTransfer = min((s32)sif1dma->qwc, (FIFO_SIF_W - sif1.fifo.size) / 4);
	
		pTag = safeDmaGetAddr(sif1dma, sif1dma->madr, DMAC_SIF1);
		if (pTag == NULL) return;

		sif1.fifo.write((u32*)pTag, qwTransfer << 2);

		sif1dma->madr += qwTransfer << 4;
		cycles += qwTransfer;		// fixme : BIAS is factored in above
		sif1dma->qwc -= qwTransfer;
	}
}

__forceinline void SIF0IOPDma(int &cycles, int &psxCycles, bool &done)
{
	if (sif0.counter <= 0) // If there's no more to transfer
	{
		// What this is supposed to do is stop DMA if it is the end of a chain, an interrupt is called, or in normal mode.
		// It currently doesn't check for normal mode.
		// (Further note: I'm not sure that it needs to check for normal mode.)
		//
		// The old code for this was:
		// if (sif0.sifData.data & 0xC0000000) 
		// which checks if the tag type is refe or end, or if the irq flag is set.
		// If the tag is refe or end, sif0.end gets set, so I'm replacing it with something easier to read: --arcum42
		if (/*(sif0dma->chcr.MOD == NORMAL_MODE) ||*/ sif0.end || DMA_TAG(sif0.data.data).IRQ)
		{
			SIF_LOG(" IOP SIF Stopped");

			// Stop & signal interrupts on IOP
			sif0.data.data = 0;
			iopsifbusy[0] = false;
			done = true;
					
			// iop is 1/8th the clock rate of the EE and psxcycles is in words (not quadwords)
			// So when we're all done, the equation looks like thus:
			//PSX_INT(IopEvt_SIF0, ( ( psxCycles*BIAS ) / 4 ) / 8);
			PSX_INT(IopEvt_SIF0, psxCycles);
		}
		else  // Chain mode
		{
			// Process DMA tag at HW_DMA9_TADR
			sif0.data = *(sifData *)iopPhysMem(HW_DMA9_TADR);
			sif0.data.words = (sif0.data.words + 3) & 0xfffffffc; // Round up to nearest 4.
			sif0.fifo.write((u32*)iopPhysMem(HW_DMA9_TADR + 8), 4);

			HW_DMA9_TADR += 16; ///HW_DMA9_MADR + 16 + sif0.sifData.words << 2;
			HW_DMA9_MADR = sif0.data.data & 0xFFFFFF;
			sif0.counter = sif0.data.words & 0xFFFFFF;

			SIF_LOG(" SIF0 Tag: madr=%lx, tadr=%lx, counter=%lx (%08X_%08X)", HW_DMA9_MADR, HW_DMA9_TADR, sif0.counter, sif0.data.words, sif0.data.data);
					
			u32 tagId = DMA_TAG(sif0.data.data).ID;
			if ((tagId == TAG_REFE) || (tagId == TAG_END))
				SIF_LOG("   END");
			else
				SIF_LOG("   CNT %08X, %08X", sif0.data.data, sif0.data.words);
					
			done = false;
		}
	}
	//if (sif0.counter > 0)
	else
	{
		// There's some data ready to transfer into the fifo..
		int wTransfer = min(sif0.counter, FIFO_SIF_W - sif0.fifo.size); // HW_DMA9_BCR >> 16;

		SIF_LOG("+++++++++++ %lX of %lX", wTransfer, sif0.counter /*(HW_DMA9_BCR >> 16)*/);

		sif0.fifo.write((u32*)iopPhysMem(HW_DMA9_MADR), wTransfer);
		HW_DMA9_MADR += wTransfer << 2;
		psxCycles += (wTransfer / 4) * BIAS;		// fixme : should be / 16
		sif0.counter -= wTransfer;
	}
}


__forceinline void SIF1IOPDma(int &cycles, int &psxCycles, bool &done)
{
	if (sif1.counter <= 0)
	{
		// Stop on tag IRQ or END
		
		if ((sif1.tagMode & 0x80) || (sif1.tagMode & 0x40))
		{
			if (sif1.tagMode & 0x40)
				SIF_LOG(" IOP SIF end");
			else
				SIF_LOG(" IOP SIF interrupt");
				
			// Stop & signal interrupts on IOP
			sif1.tagMode = 0;
			iopsifbusy[1] = false;
			done = true;

			//Fixme ( voodoocycles ):
			//The *24 are needed for ecco the dolphin (CDVD hangs) and silver surfer (Pad not detected)
			//Greater than *35 break rebooting when trying to play Tekken5 arcade history
			//Total cycles over 1024 makes SIF too slow to keep up the sound stream in so3...
			PSX_INT(IopEvt_SIF1, min ( (psxCycles * 24), 1024) );
		}
		else if (sif1.fifo.size >= 4)
		{
			// Read a tag.
			sif1.fifo.read((u32*)&sif1.data, 4);
			SIF_LOG(" IOP SIF dest chain tag madr:%08X wc:%04X id:%X irq:%d", 
				sif1.data.data & 0xffffff, sif1.data.words, DMA_TAG(sif1.data.data).ID, 
				DMA_TAG(sif1.data.data).IRQ);
				
			HW_DMA10_MADR = sif1.data.data & 0xffffff;
			sif1.counter = sif1.data.words;
			sif1.tagMode = (sif1.data.data >> 24) & 0xFF;
			done = false;
		}
	}
	
	if (sif1.counter > 0)
	{
		// If we're reading something, continue to do so.
		const int readSize = min (sif1.counter, sif1.fifo.size);
		
		SIF_LOG(" IOP SIF doing transfer %04X to %08X", readSize, HW_DMA10_MADR);

		sif1.fifo.read((u32*)iopPhysMem(HW_DMA10_MADR), readSize);
		psxCpu->Clear(HW_DMA10_MADR, readSize);
		psxCycles += readSize / 4;		// fixme: should be / 16
		sif1.counter -= readSize;
		HW_DMA10_MADR += readSize << 2;
	}
}

__forceinline void SIF0Dma()
{
	bool done = false;
	int cycles = 0, psxCycles = 0;

	SIF_LOG("SIF0 DMA start...");

	do
	{
		if (iopsifbusy[0])
		{
			// If EE SIF0 is enabled.
			SIF0IOPDma(cycles, psxCycles, done);
		}

		if (eesifbusy[0])
		{
			// If EE SIF enabled and there's something to transfer.
			SIF0EEDma(cycles, psxCycles, done);
		}
	}
	while (!done);
}

__forceinline void SIF1Dma()
{
	bool done = false;
	int cycles = 0, psxCycles = 0;
	do
	{
		if (eesifbusy[1])
		{
			// If EE SIF1 is enabled.
			SIF1EEDma(cycles, psxCycles, done);
		}

		if (iopsifbusy[1])
		{
			// If IOP SIF enabled and there's something to transfer.
			SIF1IOPDma(cycles, psxCycles, done);
		}
	}
	while (!done);
}

__forceinline void  sif0Interrupt()
{
	HW_DMA9_CHCR &= ~0x01000000;
	psxDmaInterrupt2(2);
}

__forceinline void  sif1Interrupt()
{
	HW_DMA10_CHCR &= ~0x01000000; //reset TR flag
	psxDmaInterrupt2(3);
}

__forceinline void  EEsif0Interrupt()
{
	hwDmacIrq(DMAC_SIF0);
	sif0dma->chcr.STR = false;
}

__forceinline void  EEsif1Interrupt()
{
	hwDmacIrq(DMAC_SIF1);
	sif1dma->chcr.STR = false;
}

__forceinline void dmaSIF0()
{
	SIF_LOG("EE: dmaSIF0 chcr = %lx, madr = %lx, qwc  = %lx, tadr = %lx",
	        sif0dma->chcr._u32, sif0dma->madr, sif0dma->qwc, sif0dma->tadr);

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

__forceinline void dmaSIF1()
{
	SIF_LOG("EE: dmaSIF1 chcr = %lx, madr = %lx, qwc  = %lx, tadr = %lx",
	        sif1dma->chcr._u32, sif1dma->madr, sif1dma->qwc, sif1dma->tadr);

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

__forceinline void dmaSIF2()
{
	SIF_LOG("dmaSIF2 chcr = %lx, madr = %lx, qwc  = %lx",
	        sif2dma->chcr._u32, sif2dma->madr, sif2dma->qwc);

	sif2dma->chcr.STR = false;
	hwDmacIrq(DMAC_SIF2);
	Console.WriteLn("*PCSX2*: dmaSIF2");
}


void SaveStateBase::sifFreeze()
{
	FreezeTag("SIFdma");

	Freeze(sif0);
	Freeze(sif1);

	Freeze(eesifbusy);
	Freeze(iopsifbusy);
}
