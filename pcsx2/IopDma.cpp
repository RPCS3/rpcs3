/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "IopCommon.h"

using namespace R3000A;

// Dma0/1   in Mdec.c
// Dma3     in CdRom.c
// Dma8     in PsxSpd.c
// Dma11/12 in PsxSio2.c

// Should be a bool, and will be next time I break savestate. --arcum42
int iopsifbusy[2] = { 0, 0 };

static void __fastcall psxDmaGeneric(u32 madr, u32 bcr, u32 chcr, u32 spuCore, _SPU2writeDMA4Mem spu2WriteFunc, _SPU2readDMA4Mem spu2ReadFunc)
{
	const char dmaNum = spuCore ? '7' : '4';

	/*if (chcr & 0x400) DevCon::Status("SPU 2 DMA %c linked list chain mode! chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);
	if (chcr & 0x40000000) DevCon::Notice("SPU 2 DMA %c Unusual bit set on 'to' direction chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);
	if ((chcr & 0x1) == 0) DevCon::Status("SPU 2 DMA %c loading from spu2 memory chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);*/

	const int size = (bcr >> 16) * (bcr & 0xFFFF);

	// Update the spu2 to the current cycle before initiating the DMA

	if (SPU2async)
	{
		SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);
		//Console::Status("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);

		psxCounters[6].sCycleT = psxRegs.cycle;
		psxCounters[6].CycleT = size * 3;

		psxNextCounter -= (psxRegs.cycle - psxNextsCounter);
		psxNextsCounter = psxRegs.cycle;
		if (psxCounters[6].CycleT < psxNextCounter)
			psxNextCounter = psxCounters[6].CycleT;
	}

	switch (chcr)
	{
		case 0x01000201: //cpu to spu2 transfer
			PSXDMA_LOG("*** DMA %c - mem2spu *** %x addr = %x size = %x", dmaNum, chcr, madr, bcr);
			spu2WriteFunc((u16 *)iopPhysMem(madr), size*2);
			break;

		case 0x01000200: //spu2 to cpu transfer
			PSXDMA_LOG("*** DMA %c - spu2mem *** %x addr = %x size = %x", dmaNum, chcr, madr, bcr);
			spu2ReadFunc((u16 *)iopPhysMem(madr), size*2);
			psxCpu->Clear(spuCore ? HW_DMA7_MADR : HW_DMA4_MADR, size);
			break;

		default:
			Console::Error("*** DMA %c - SPU unknown *** %x addr = %x size = %x", params dmaNum, chcr, madr, bcr);
			break;
	}
}

void psxDma4(u32 madr, u32 bcr, u32 chcr)		// SPU2's Core 0
{
	psxDmaGeneric(madr, bcr, chcr, 0, SPU2writeDMA4Mem, SPU2readDMA4Mem);
}

int psxDma4Interrupt()
{
	HW_DMA4_CHCR &= ~0x01000000;
	psxDmaInterrupt(4);
	iopIntcIrq(9);
	return 1;
}

void psxDma2(u32 madr, u32 bcr, u32 chcr)		// GPU
{
	HW_DMA2_CHCR &= ~0x01000000;
	psxDmaInterrupt(2);
}

void psxDma6(u32 madr, u32 bcr, u32 chcr)
{
	u32 *mem = (u32 *)iopPhysMem(madr);

	PSXDMA_LOG("*** DMA 6 - OT *** %lx addr = %lx size = %lx", chcr, madr, bcr);

	if (chcr == 0x11000002)
	{
		while (bcr--)
		{
			*mem-- = (madr - 4) & 0xffffff;
			madr -= 4;
		}
		mem++;
		*mem = 0xffffff;
	}
	else
	{
		// Unknown option
		PSXDMA_LOG("*** DMA 6 - OT unknown *** %lx addr = %lx size = %lx", chcr, madr, bcr);
	}
	HW_DMA6_CHCR &= ~0x01000000;
	psxDmaInterrupt(6);
}

void psxDma7(u32 madr, u32 bcr, u32 chcr)		// SPU2's Core 1
{
	psxDmaGeneric(madr, bcr, chcr, 1, SPU2writeDMA7Mem, SPU2readDMA7Mem);
}

int psxDma7Interrupt()
{
	HW_DMA7_CHCR &= ~0x01000000;
	psxDmaInterrupt2(0);
	return 1;

}
extern int eesifbusy[2];
void psxDma9(u32 madr, u32 bcr, u32 chcr)
{
	SIF_LOG("IOP: dmaSIF0 chcr = %lx, madr = %lx, bcr = %lx, tadr = %lx",	chcr, madr, bcr, HW_DMA9_TADR);

	iopsifbusy[0] = 1;
	psHu32(0x1000F240) |= 0x2000;

	if (eesifbusy[0] == 1)
	{
		SIF0Dma();
		psHu32(0x1000F240) &= ~0x20;
		psHu32(0x1000F240) &= ~0x2000;
	}
}

void psxDma10(u32 madr, u32 bcr, u32 chcr)
{
	SIF_LOG("IOP: dmaSIF1 chcr = %lx, madr = %lx, bcr = %lx",	chcr, madr, bcr);

	iopsifbusy[1] = 1;
	psHu32(0x1000F240) |= 0x4000;

	if (eesifbusy[1] == 1)
	{
		FreezeXMMRegs(1);
		SIF1Dma();
		psHu32(0x1000F240) &= ~0x40;
		psHu32(0x1000F240) &= ~0x100;
		psHu32(0x1000F240) &= ~0x4000;
		FreezeXMMRegs(0);
	}
}

void psxDma8(u32 madr, u32 bcr, u32 chcr)
{

	const int size = (bcr >> 16) * (bcr & 0xFFFF) * 8;

	switch (chcr & 0x01000201)
	{
		case 0x01000201: //cpu to dev9 transfer
			PSXDMA_LOG("*** DMA 8 - DEV9 mem2dev9 *** %lx addr = %lx size = %lx", chcr, madr, bcr);
			DEV9writeDMA8Mem((u32*)iopPhysMem(madr), size);
			break;

		case 0x01000200: //dev9 to cpu transfer
			PSXDMA_LOG("*** DMA 8 - DEV9 dev9mem *** %lx addr = %lx size = %lx", chcr, madr, bcr);
			DEV9readDMA8Mem((u32*)iopPhysMem(madr), size);
			break;

		default:
			PSXDMA_LOG("*** DMA 8 - DEV9 unknown *** %lx addr = %lx size = %lx", chcr, madr, bcr);
			break;
	}
	HW_DMA8_CHCR &= ~0x01000000;
	psxDmaInterrupt2(1);
}

void  dev9Interrupt()
{
	if ((dev9Handler != NULL) && (dev9Handler() != 1)) return;

	iopIntcIrq(13);
	hwIntcIrq(INTC_SBUS);
}

void dev9Irq(int cycles)
{
	PSX_INT(IopEvt_DEV9, cycles);
}

void  usbInterrupt()
{
	if (usbHandler != NULL && (usbHandler() != 1)) return;

	iopIntcIrq(22);
	hwIntcIrq(INTC_SBUS);
}

void usbIrq(int cycles)
{
	PSX_INT(IopEvt_USB, cycles);
}

void fwIrq()
{
	iopIntcIrq(24);
	hwIntcIrq(INTC_SBUS);
}

void spu2DMA4Irq()
{
	SPU2interruptDMA4();
	HW_DMA4_CHCR &= ~0x01000000;
	psxDmaInterrupt(4);
}

void spu2DMA7Irq()
{
	SPU2interruptDMA7();
	HW_DMA7_CHCR &= ~0x01000000;
	psxDmaInterrupt2(0);
}

void spu2Irq()
{
	iopIntcIrq(9);
	hwIntcIrq(INTC_SBUS);
}

void iopIntcIrq(uint irqType)
{
	psxHu32(0x1070) |= 1 << irqType;
	iopTestIntc();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Gigaherz's "Improved DMA Handling" Engine WIP...
//

// fixme: Is this in progress?
#if FALSE

typedef s32(* DmaHandler)(s32 channel, u32* data, u32 wordsLeft, u32* wordsProcessed);
typedef void (* DmaIHandler)(s32 channel);

s32 errDmaWrite(s32 channel, u32* data, u32 wordsLeft, u32* wordsProcessed);
s32 errDmaRead(s32 channel, u32* data, u32 wordsLeft, u32* wordsProcessed);

struct DmaHandlerInfo
{
	DmaHandler  Read;
	DmaHandler  Write;
	DmaIHandler Interrupt;
};

struct DmaStatusInfo
{
	u32 Control;
	u32 Width;		// bytes/word, for timing purposes
	u32 MemAddr;
	u32 ByteCount;
	u32 Target;
};

// FIXME: Dummy constants, to be "filled in" with proper values later
#define DMA_CTRL_ACTIVE		0x80000000
#define DMA_CTRL_DIRECTION	0x00000001

#define DMA_CHANNEL_MAX		16 /* ? */

DmaStatusInfo  IopChannels[DMA_CHANNEL_MAX]; // I dont' knwo how many there are, 10?

DmaHandlerInfo IopDmaHandlers[DMA_CHANNEL_MAX] =
{
	{0}, //0
	{0}, //1
	{0}, //2
	{cdvdDmaRead, errDmaWrite,  cdvdDmaInterrupt}, //3:  CDVD
	{spu2DmaRead, spu2DmaWrite, spu2DmaInterrupt}, //4:  Spu Core0
	{0}, //5
	{0}, //6: OT?
	{spu2DmaRead, spu2DmaWrite, spu2DmaInterrupt}, //7:  Spu Core1
	{dev9DmaRead, dev9DmaWrite, dev9DmaInterrupt}, //8:  Dev9
	{sif0DmaRead, sif0DmaWrite, sif0DmaInterrupt}, //9:  SIF0
	{sif1DmaRead, sif1DmaWrite, sif1DmaInterrupt}, //10: SIF1
	{0}, // Sio2
	{0}, // Sio2
};

const char* IopDmaNames[DMA_CHANNEL_MAX] =
{
	"Ps1 Mdec",
	"Ps1 Mdec",
	"Ps1 Gpu",
	"CDVD",
	"SPU/SPU2 Core0",
	"?",
	"OT",
	"SPU2 Core1", //7:  Spu Core1
	"Dev9", //8:  Dev9
	"Sif0", //9:  SIF0
	"Sif1", //10: SIF1
	"Sio2",//...
	"Sio2",
	"?", "?", "?"
};
};

// Prototypes. To be implemented later (or in other parts of the emulator)
void SetDmaUpdateTarget(u32 delay);
void RaiseDmaIrq(u32 channel);

// WARNING: CALLER ****[MUST]**** CALL IopDmaUpdate RIGHT AFTER THIS!
void IopDmaStart(int channel, u32 chcr, u32 madr, u32 bcr)
{
	// I dont' really understand this, but it's used above. Is this BYTES OR WHAT?
	int size = (bcr >> 16) * (bcr & 0xFFFF);

	IopChannels[channel].Control = chcr | DMA_CTRL_ACTIVE;
	IopChannels[channel].MemAddr = madr;
	IopChannels[channel].ByteCount = size;
}

void IopDmaUpdate(u32 elapsed)
{
	u32 MinDelay = 0xFFFFFFFF;

	for (int i = 0;i < DMA_CHANNEL_MAX;i++)
	{
		DmaStatusInfo *ch = IopChannels + i;

		if (ch->Control&DMA_CTRL_ACTIVE)
		{
			ch->Target -= elapsed;
			if (ch->Target <= 0)
			{
				if (ch->ByteCount <= 0)
				{
					ch->Control &= ~DMA_CTRL_ACTIVE;
					RaiseDmaIrq(i);
					IopDmaHandlers[i].Interrupt(i);
				}
				else
				{
					// TODO: Make sure it's the right order
					DmaHandler handler = (ch->Control & DMA_CTRL_DIRECTION) ? IopDmaHandlers[i].Read : IopDmaHandlers[i].Write;

					u32 BCount = 0;
					s32 Target = (handler) ? handler(i, (u32*)PSXM(ch->MemAddr), ch->ByteCount, &BCount) : 0;

					ch->Target = 100;
					if (Target < 0)
					{
						// TODO: ... What to do if the plugin errors? :P
					}
					else if (BCount != 0)
					{
						ch->MemAddr   += BCount;
						ch->ByteCount -= BCount;

						ch->Target = BCount / ch->Width;
					}

					if (Target != 0) ch->Target = Target;
				}
			}
		}
	}
}

s32 errDmaRead(s32 channel, u32* data, u32 wordsLeft, u32* wordsProcessed)
{
	Console::Error("ERROR: Tried to read using DMA %d (%s). Ignoring.", 0, channel, IopDmaNames[channel]);

	*wordsProcessed = wordsLeft;
	return 0;
}

s32 errDmaWrite(s32 channel, u32* data, u32 wordsLeft, u32* wordsProcessed)
{
	Console::Error("ERROR: Tried to write using DMA %d (%s). Ignoring.", 0, channel, IopDmaNames[channel]);

	*wordsProcessed = wordsLeft;
	return 0;
}


#endif
