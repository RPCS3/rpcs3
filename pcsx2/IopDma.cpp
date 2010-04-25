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
#include "IopCommon.h"

#include "Sif.h"

using namespace R3000A;

// Dma0/1   in Mdec.c
// Dma3     in CdRom.c
// Dma8     in PsxSpd.c
// Dma11/12 in PsxSio2.c

#ifndef ENABLE_NEW_IOPDMA_SPU2
static void __fastcall psxDmaGeneric(u32 madr, u32 bcr, u32 chcr, u32 spuCore, _SPU2writeDMA4Mem spu2WriteFunc, _SPU2readDMA4Mem spu2ReadFunc)
{
	const char dmaNum = spuCore ? '7' : '4';

	/*if (chcr & 0x400) DevCon.Status("SPU 2 DMA %c linked list chain mode! chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);
	if (chcr & 0x40000000) DevCon.Warning("SPU 2 DMA %c Unusual bit set on 'to' direction chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);
	if ((chcr & 0x1) == 0) DevCon.Status("SPU 2 DMA %c loading from spu2 memory chcr = %x madr = %x bcr = %x\n", dmaNum, chcr, madr, bcr);*/

	const int size = (bcr >> 16) * (bcr & 0xFFFF);

	// Update the spu2 to the current cycle before initiating the DMA

	if (SPU2async)
	{
		SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);
		//Console.Status("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);

		psxCounters[6].sCycleT = psxRegs.cycle;
		psxCounters[6].CycleT = size * 3;

		psxNextCounter -= (psxRegs.cycle - psxNextsCounter);
		psxNextsCounter = psxRegs.cycle;
		if (psxCounters[6].CycleT < psxNextCounter)
			psxNextCounter = psxCounters[6].CycleT;

		if((g_psxNextBranchCycle - psxNextsCounter) > (u32)psxNextCounter)
		{
			//DevCon.Warning("SPU2async Setting new counter branch, old %x new %x ((%x - %x = %x) > %x delta)", g_psxNextBranchCycle, psxNextsCounter + psxNextCounter, g_psxNextBranchCycle, psxNextsCounter, (g_psxNextBranchCycle - psxNextsCounter), psxNextCounter);
			g_psxNextBranchCycle = psxNextsCounter + psxNextCounter;
		}
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
			Console.Error("*** DMA %c - SPU unknown *** %x addr = %x size = %x", dmaNum, chcr, madr, bcr);
			break;
	}
}

void psxDma4(u32 madr, u32 bcr, u32 chcr)		// SPU2's Core 0
{
	psxDmaGeneric(madr, bcr, chcr, 0, SPU2writeDMA4Mem, SPU2readDMA4Mem);
}

int psxDma4Interrupt()
{
	#ifdef SPU2IRQTEST
		Console.Warning("psxDma4Interrupt()");
	#endif
	HW_DMA4_CHCR &= ~0x01000000;
	psxDmaInterrupt(4);
	iopIntcIrq(9);
	return 1;
}

void spu2DMA4Irq()
{
	#ifdef SPU2IRQTEST
		Console.Warning("spu2DMA4Irq()");
	#endif
	SPU2interruptDMA4();
	HW_DMA4_CHCR &= ~0x01000000;
	psxDmaInterrupt(4);
}

void psxDma7(u32 madr, u32 bcr, u32 chcr)		// SPU2's Core 1
{
	psxDmaGeneric(madr, bcr, chcr, 1, SPU2writeDMA7Mem, SPU2readDMA7Mem);
}

int psxDma7Interrupt()
{
	#ifdef SPU2IRQTEST
		Console.Warning("psxDma7Interrupt()");
	#endif
	HW_DMA7_CHCR &= ~0x01000000;
	psxDmaInterrupt2(0);
	return 1;

}

void spu2DMA7Irq()
{
	#ifdef SPU2IRQTEST
		Console.Warning("spu2DMA7Irq()");
	#endif
	SPU2interruptDMA7();
	HW_DMA7_CHCR &= ~0x01000000;
	psxDmaInterrupt2(0);
}

#endif

#ifndef DISABLE_PSX_GPU_DMAS
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
#endif

#ifndef ENABLE_NEW_IOPDMA_DEV9
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
#endif

void psxDma9(u32 madr, u32 bcr, u32 chcr)
{
	SIF_LOG("IOP: dmaSIF0 chcr = %lx, madr = %lx, bcr = %lx, tadr = %lx",	chcr, madr, bcr, HW_DMA9_TADR);

	sif0.iop.busy = true;
	psHu32(SBUS_F240) |= 0x2000;

	if (sif0.ee.busy)
	{
		SIF0Dma();
		psHu32(SBUS_F240) &= ~0x20;
		psHu32(SBUS_F240) &= ~0x2000;
	}
}

void psxDma10(u32 madr, u32 bcr, u32 chcr)
{
	SIF_LOG("IOP: dmaSIF1 chcr = %lx, madr = %lx, bcr = %lx",	chcr, madr, bcr);

	sif1.iop.busy = true;
	psHu32(SBUS_F240) |= 0x4000;

	if (sif1.ee.busy)
	{
        XMMRegisters::Freeze();
		SIF1Dma();
		psHu32(SBUS_F240) &= ~0x40;
		psHu32(SBUS_F240) &= ~0x100;
		psHu32(SBUS_F240) &= ~0x4000;
        XMMRegisters::Thaw();
	}
}

/* psxDma11 & psxDma 12 are in IopSio2.cpp, along with the appropriate interrupt functions. */

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Gigaherz's "Improved DMA Handling" Engine WIP...
//

#ifdef ENABLE_NEW_IOPDMA

//////////////////////////////////////////////////////////////////////////////////////////////
// Local Declarations

// in IopSio2.cpp
extern s32  CALLBACK sio2DmaStart(s32 channel, u32 madr, u32 bcr, u32 chcr);
extern s32  CALLBACK sio2DmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
extern s32  CALLBACK sio2DmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
extern void CALLBACK sio2DmaInterrupt(s32 channel);

// implemented below
s32 CALLBACK errDmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
s32 CALLBACK errDmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);

// pointer types
typedef s32  (CALLBACK * DmaHandler)(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
typedef void (CALLBACK * DmaIHandler)(s32 channel);
typedef s32  (CALLBACK * DmaSHandler)(s32 channel, u32 madr, u32 bcr, u32 chcr);

// constants
struct DmaHandlerInfo
{
	const char* Name;

	// doubles as a "disable" flag
	u32 DirectionFlags;
	u32 DmacRegisterBase;
	DmaHandler  Read;
	DmaHandler  Write;
	DmaIHandler Interrupt;
	DmaSHandler Start;

	__forceinline u32& REG_MADR(void) const { return psxHu32(DmacRegisterBase + 0x0); }
	__forceinline u32& REG_BCR(void)  const { return psxHu32(DmacRegisterBase + 0x4); }
	__forceinline u32& REG_CHCR(void) const { return psxHu32(DmacRegisterBase + 0x8); }
	__forceinline u32& REG_TADR(void) const { return psxHu32(DmacRegisterBase + 0xC); }
};

#define MEM_BASE1 0x1f801080
#define MEM_BASE2 0x1f801500

#define CHANNEL_BASE1(ch) (MEM_BASE1 + ((ch)<<4))
#define CHANNEL_BASE2(ch) (MEM_BASE2 + ((ch)<<4))

// channel disabled
#define _D__ 0
#define _D_W 1
#define _DR_ 2
#define _DRW 3
// channel enabled
#define _E__ 4
#define _E_W 5
#define _ER_ 6
#define _ERW 7

//////////////////////////////////////////////////////////////////////////////////////////////
// Plugin interface accessors

#ifdef ENABLE_NEW_IOPDMA_SPU2
s32  CALLBACK spu2DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { return SPU2dmaRead(channel,data,bytesLeft,bytesProcessed); }
s32  CALLBACK spu2DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { return SPU2dmaWrite(channel,data,bytesLeft,bytesProcessed); }
void CALLBACK spu2DmaInterrupt (s32 channel) { SPU2dmaInterrupt(channel); }
#else
s32  CALLBACK spu2DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { *bytesProcessed=0; return 0; }
s32  CALLBACK spu2DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { *bytesProcessed=0; return 0; }
void CALLBACK spu2DmaInterrupt (s32 channel) { }
#endif
#ifdef ENABLE_NEW_IOPDMA_DEV9
s32  CALLBACK dev9DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { return DEV9dmaRead(channel,data,bytesLeft,bytesProcessed); }
s32  CALLBACK dev9DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { return DEV9dmaWrite(channel,data,bytesLeft,bytesProcessed); }
void CALLBACK dev9DmaInterrupt (s32 channel) { DEV9dmaInterrupt(channel); }
#else
s32  CALLBACK dev9DmaRead  (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { *bytesProcessed=0; return 0; }
s32  CALLBACK dev9DmaWrite (s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed) { *bytesProcessed=0; return 0; }
void CALLBACK dev9DmaInterrupt (s32 channel) { }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Dma channel definitions

const DmaHandlerInfo IopDmaHandlers[DMA_CHANNEL_MAX] =
{
	// First DMAC, same as PS1
	{"Ps1 Mdec in",    _D__}, //0
	{"Ps1 Mdec out",   _D__}, //1
	{"Ps1 Gpu",        _D__}, //2
#ifdef ENABLE_NEW_IOPDMA_CDVD
	{"CDVD",           _ER_, CHANNEL_BASE1(3), cdvdDmaRead, errDmaWrite,  cdvdDmaInterrupt}, //3:  CDVD
#else
	{"CDVD",           _D__}, //3:  CDVD
#endif
#ifdef ENABLE_NEW_IOPDMA_SPU2
	{"SPU2 Core0",     _ERW, CHANNEL_BASE1(4), spu2DmaRead, spu2DmaWrite, spu2DmaInterrupt}, //4:  Spu/Spu2 Core0
#else
	{"SPU2 Core0",     _D__}, //4:  Spu/Spu2 Core0
#endif
	{"Ps1 PIO",            _D__}, //5: PIO
	{"Ps1 OTC",             _D__}, //6: "reverse clear OT" - PSX GPU related

	// Second DMAC, new in PS2 IOP
#ifdef ENABLE_NEW_IOPDMA_SPU2
	{"SPU2 Core1",     _ERW, CHANNEL_BASE2(0), spu2DmaRead, spu2DmaWrite, spu2DmaInterrupt}, //7:  Spu2 Core1
#else
	{"SPU2 Core1",     _D__}, //7:  Spu2 Core1
#endif
#ifdef ENABLE_NEW_IOPDMA_DEV9
	{"Dev9",		   _ERW, CHANNEL_BASE2(1), dev9DmaRead, dev9DmaWrite, dev9DmaInterrupt}, //8:  Dev9
#else
	{"Dev9",		   _D__}, //8:  Dev9
#endif
#ifdef ENABLE_NEW_IOPDMA_SIF
	{"Sif0",           _ERW, CHANNEL_BASE2(2), sif0DmaRead, sif0DmaWrite, sif0DmaInterrupt}, //9:  SIF0
	{"Sif1",           _ERW, CHANNEL_BASE2(3), sif1DmaRead, sif1DmaWrite, sif1DmaInterrupt}, //10: SIF1
#else
	{"Sif0",           _D__}, //9:  SIF0
	{"Sif1",           _D__}, //10: SIF1
#endif
#ifdef ENABLE_NEW_IOPDMA_SIO
	{"Sio2 (writes)",  _E_W, CHANNEL_BASE2(4), errDmaRead, sio2DmaWrite, sio2DmaInterrupt, sio2DmaStart}, //11: Sio2
	{"Sio2 (reads)",   _ER_, CHANNEL_BASE2(5), sio2DmaRead, errDmaWrite, sio2DmaInterrupt, sio2DmaStart}, //12: Sio2
#else
	{"Sio2 (writes)",  _D__}, //11: Sio2
	{"Sio2 (reads)",   _D__}, //12: Sio2
#endif
	{"?",              _D__}, //13
	// if each dmac has 7 channels, the list would end here, but I'm not sure :p
};

// runtime variables
struct DmaChannelInfo
{
	s32 ByteCount;
	s32 NextUpdate;
} IopDmaChannels[DMA_CHANNEL_MAX] = {0};


//////////////////////////////////////////////////////////////////////////////////////////////
// Tool functions
void SetDmaUpdateTarget(u32 delay)
{
	psxCounters[8].CycleT = delay;
	if (delay < psxNextCounter)
		psxNextCounter = delay;
}

void RaiseDmaIrq(u32 channel)
{
	if(channel<7)
		psxDmaInterrupt(channel);
	else
		psxDmaInterrupt2(channel-7);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// IopDmaStart: Called from IopHwWrite to test and possibly start a dma transfer

void IopDmaStart(int channel)
{
	if(!(IopDmaHandlers[channel].DirectionFlags&_E__))
		return;

	int chcr = IopDmaHandlers[channel].REG_CHCR();

	int pcr = (channel>=7)?(HW_DMA_PCR2 & (8 << ((channel-7) * 4))):(HW_DMA_PCR & (8 << (channel * 4)));

	if ( !(chcr & 0x01000000) || !pcr)
		return;

	// I dont' really understand this, but it's used above. Is this BYTES OR WHAT?
	int bcr = IopDmaHandlers[channel].REG_BCR();
	int bcr_size = (bcr & 0xFFFF);
	int bcr_count = (bcr >> 16);
	int size = 4* bcr_count * bcr_size;

	int dirf = IopDmaHandlers[channel].DirectionFlags&3;

	if(dirf != 3)
	{
		bool ok = (chcr & DMA_CTRL_DIRECTION)? (dirf==_D_W) : (dirf==_DR_);
		if(!ok)
		{
			// hack?!
			IopDmaHandlers[channel].REG_CHCR() &= ~DMA_CTRL_ACTIVE;
			return;
		}
	}

	if(IopDmaHandlers[channel].Start)
	{
		int ret = IopDmaHandlers[channel].Start(channel,
									IopDmaHandlers[channel].REG_MADR(),
									IopDmaHandlers[channel].REG_BCR(),
									IopDmaHandlers[channel].REG_CHCR());
		if(ret < 0)
		{
			IopDmaHandlers[channel].REG_CHCR() &= ~DMA_CTRL_ACTIVE;
			return;
		}
	}

	//Console.WriteLn(Color_StrongOrange,"Starting NewDMA ch=%d, size=%d(0x%08x), dir=%d", channel, size, bcr, chcr&DMA_CTRL_DIRECTION);

	IopDmaHandlers[channel].REG_CHCR() |= DMA_CTRL_ACTIVE;
	IopDmaChannels[channel].ByteCount = size;
	IopDmaChannels[channel].NextUpdate = 0;

	//SetDmaUpdateTarget(1);
	{
		const s32 difference = psxRegs.cycle - psxCounters[8].sCycleT;

		psxCounters[8].sCycleT = psxRegs.cycle;
		psxCounters[8].CycleT = psxCounters[8].rate;
		IopDmaUpdate(difference);

		s32 c = psxCounters[8].CycleT;
		if (c < psxNextCounter) psxNextCounter = c;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
// IopDmaProcessChannel: Called from IopDmaUpdate (below) to process a dma channel

template<int channel>
static void __releaseinline IopDmaProcessChannel(int elapsed, int& MinDelay)
{
	// Hopefully the compiler would be able to optimize the whole function away if this doesn't pass.
	if(!(IopDmaHandlers[channel].DirectionFlags&_E__))
		return;

	DmaChannelInfo *ch = IopDmaChannels + channel;
	const DmaHandlerInfo *hh = IopDmaHandlers + channel;

	if (hh->REG_CHCR()&DMA_CTRL_ACTIVE)
	{
		ch->NextUpdate -= elapsed;
		if (ch->NextUpdate <= 0) // Refresh target passed
		{
			if (ch->ByteCount <= 0) // No more data left, finish dma
			{
				ch->NextUpdate = 0x7fffffff;

				hh->REG_CHCR() &= ~DMA_CTRL_ACTIVE;
				RaiseDmaIrq(channel);
				hh->Interrupt(channel);
			}
			else // let the handlers transfer more data
			{
				int chcr = hh->REG_CHCR();

				DmaHandler handler = (chcr & DMA_CTRL_DIRECTION) ? hh->Write : hh->Read;

				u32 ProcessedBytes = 0;
				s32 RequestedDelay = (handler) ? handler(channel, (u32*)iopPhysMem(hh->REG_MADR()), ch->ByteCount, &ProcessedBytes) : 0;

				if(ProcessedBytes>0 && (!(chcr & DMA_CTRL_DIRECTION)))
				{
					psxCpu->Clear(hh->REG_MADR(), ProcessedBytes/4);
				}

				int NextUpdateDelay = 100;
				if (RequestedDelay < 0) // error code
				{
					// TODO: ... What to do if the handler gives an error code? :P
					DevCon.Warning("ERROR on channel %d",channel);
					hh->REG_CHCR() &= ~DMA_CTRL_ACTIVE;
					RaiseDmaIrq(channel);
					hh->Interrupt(channel);
				}
				else if (ProcessedBytes > 0) // if not an error, continue transfer
				{
					//DevCon.WriteLn("Transfer channel %d, ProcessedBytes = %d",i,ProcessedBytes);
					hh->REG_MADR()+= ProcessedBytes;
					ch->ByteCount -= ProcessedBytes;

					NextUpdateDelay = ProcessedBytes/2; // / ch->Width;
				}
				else if(RequestedDelay==0)
					DevCon.Warning("What now? :p"); // its ok as long as there's a delay requeste, autodma requires this.

				if (RequestedDelay != 0) NextUpdateDelay = RequestedDelay;

				// SPU2 adma early interrupts. PCSX2 likes those better currently.
				if((channel==4 || channel==7) && (ch->ByteCount<=0) && (ProcessedBytes <= 1024))
				{
				 	ch->NextUpdate = 0;
	 			}
				else
					ch->NextUpdate += NextUpdateDelay;

				//ch->NextUpdate += NextUpdateDelay;
			}
		}

		int nTarget = ch->NextUpdate;
		if(nTarget < 0) nTarget = 0;

		if (nTarget<MinDelay)
			MinDelay = nTarget;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
// IopDmaProcessChannel: Called regularly to update the active channels

void IopDmaUpdate(u32 elapsed)
{
	s32 MinDelay=0;

	do {
		MinDelay = 0x7FFFFFFF; // max possible value

		// Unrolled
		//IopDmaProcessChannel<0>(elapsed, MinDelay);
		//IopDmaProcessChannel<1>(elapsed, MinDelay);
		//IopDmaProcessChannel<2>(elapsed, MinDelay);
		IopDmaProcessChannel<3>(elapsed, MinDelay);
		IopDmaProcessChannel<4>(elapsed, MinDelay);
		//IopDmaProcessChannel<5>(elapsed, MinDelay);
		//IopDmaProcessChannel<6>(elapsed, MinDelay);
		IopDmaProcessChannel<7>(elapsed, MinDelay);
		IopDmaProcessChannel<8>(elapsed, MinDelay);
		IopDmaProcessChannel<9>(elapsed, MinDelay);
		IopDmaProcessChannel<10>(elapsed, MinDelay);
		IopDmaProcessChannel<11>(elapsed, MinDelay);
		IopDmaProcessChannel<12>(elapsed, MinDelay);
		//IopDmaProcessChannel<13>(elapsed, MinDelay);

		// reset elapsed time in case we loop
		elapsed=0;
	}
	while(MinDelay <= 0);

	if(MinDelay<0x7FFFFFFF)
	{
		// tell the iop when to call this function again
		SetDmaUpdateTarget(MinDelay);
	}
	else
	{
		// bogus value so the function gets called again, not sure if it's necessary anymore
		SetDmaUpdateTarget(10000);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Error functions: dummy functions for unsupported dma "directions"

s32 CALLBACK errDmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	Console.Error("ERROR: Tried to read using DMA %d (%s). Ignoring.", channel, IopDmaHandlers[channel]);

	*bytesProcessed = bytesLeft;
	return 0;
}

s32 CALLBACK errDmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	Console.Error("ERROR: Tried to write using DMA %d (%s). Ignoring.", channel, IopDmaHandlers[channel]);

	*bytesProcessed = bytesLeft;
	return 0;
}

void SaveStateBase::iopDmacFreeze()
{
	FreezeTag("iopDmac");

	Freeze(IopDmaChannels);

	if( IsLoading() )
	{
		SetDmaUpdateTarget(10000); // Might be needed to kickstart the main updater :p
	}
}

#endif
