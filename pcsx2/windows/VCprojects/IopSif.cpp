#include "PrecompiledHeader.h"
#include "IopCommon.h"

#include "Sif.h"

#if FALSE

const u32 fifoSize = 0x1000; // in bytes

s32 PrepareEEWrite()
{

	return 0;
}

s32 PrepareEERead()
{
	static __aligned16 u32 tag[4];

	// Process DMA tag at hw_dma(9).tadr
	sif0.iop.data = *(sifData *)iopPhysMem(hw_dma(9).tadr);
	sif0.iop.data.words = (sif0.iop.data.words + 3) & 0xfffffffc; // Round up to nearest 4.
	memcpy(tag, (u32*)iopPhysMem(hw_dma(9).tadr + 8), 16);

	hw_dma(9).tadr += 16; ///hw_dma(9).madr + 16 + sif0.sifData.words << 2;

	// We're only copying the first 24 bits.
	hw_dma(9).madr = sif0data & 0xFFFFFF;
	sif0.iop.counter = sif0words;

	if (sif0tag.IRQ  || (sif0tag.ID & 4)) sif0.iop.end = true;
	SIF_LOG("SIF0 IOP to EE Tag: madr=%lx, tadr=%lx, counter=%lx (%08X_%08X)"
		"\n\tread tag: %x %x %x %x", hw_dma(9).madr, hw_dma(9).tadr, sif0.iop.counter, sif0words, sif0data,
		tag[0], tag[1], tag[2], tag[3]);

	sif0dma.unsafeTransfer(((tDMA_TAG*)(tag)));
	sif0dma.madr = tag[1];
	tDMA_TAG ptag(tag[0]);

	SIF_LOG("SIF0 EE dest chain tag madr:%08X qwc:%04X id:%X irq:%d(%08X_%08X)",
		sif0dma.madr, sif0dma.qwc, ptag.ID, ptag.IRQ, tag[1], tag[0]);

	if (sif0dma.chcr.TIE && ptag.IRQ)
	{
		//Console.WriteLn("SIF0 TIE");
		sif0.ee.end = true;
	}

	switch (ptag.ID)
	{
	case TAG_REFE:
		sif0.ee.end = true;
		if (dmacRegs.ctrl.STS != NO_STS)
			dmacRegs.stadr.ADDR = sif0dma.madr + (sif0dma.qwc * 16);
		break;

	case TAG_REFS:
		if (dmacRegs.ctrl.STS != NO_STS)
			dmacRegs.stadr.ADDR = sif0dma.madr + (sif0dma.qwc * 16);
		break;

	case TAG_END:
		sif0.ee.end = true;
		break;
	}
	return true;
}

void FinalizeEERead()
{
	SIF_LOG("Sif0: End EE");
	sif0.ee.end = false;
	sif0.ee.busy = false;
	SIF_LOG("CPU INT FIRED SIF0");
	CPU_INT(DMAC_SIF0, 16);
}

s32 DoSIFWrite(u32 iopAvailable)
{
	u32 eeAvailable = PrepareEEWrite();

}

s32 DoSifRead(u32 iopAvailable)
{
	u32 eeAvailable = PrepareEERead();

	u32 transferSizeBytes = min(min(iopAvailable,eeAvailable),fifoSize);
	u32 transferSizeWords = transferSizeBytes >> 2;
	u32 transferSizeQWords = transferSizeBytes >> 4;

	SIF_LOG("Write IOP to EE: +++++++++++ %lX of %lX", transferSizeWords, sif0.iop.counter);

	tDMA_TAG *ptag = sif0dma.getAddr(sif0dma.madr, DMAC_SIF0, true);
	if (ptag == NULL)
	{
		DevCon.Warning("Write IOP to EE: ptag == NULL");
		return false;
	}

	memcpy((u32*)ptag, (u32*)iopPhysMem(hw_dma(9).madr), transferSizeBytes);

	// Clearing handled by vtlb memory protection and manual blocks.
	//Cpu->Clear(sif0dma.madr, readSize*4);

	sif0dma.madr += transferSizeBytes;
	sif0.ee.cycles += transferSizeQWords * 2;
	sif0dma.qwc -= transferSizeQWords;

	return transferSizeBytes;
}

s32  CALLBACK sif0DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	s32 processed = DoSIFWrite(bytesLeft);

	if(processed>0)
	{
		bytesLeft -= processed;
		if(bytesLeft == 0)
			FinalizeEERead();

		*bytesProcessed = processed;
		return 0;
	}


	*bytesProcessed=0;
	return -processed;
}

s32  CALLBACK sif0DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	DevCon.Warning("SIF0 Dma Write to iop?!");
	*bytesProcessed=0; return 0;
}

void CALLBACK sif0DmaInterrupt (s32 channel)
{

}

s32  CALLBACK sif1DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	DevCon.Warning("SIF1 Dma Read from iop?!");
	*bytesProcessed=0; return 0;
}

s32  CALLBACK sif1DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	*bytesProcessed=0;
	return 0;
}

void CALLBACK sif1DmaInterrupt (s32 channel)
{

}

#endif
