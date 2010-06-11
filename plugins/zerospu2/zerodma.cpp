/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "zerospu2.h"
#include "zerodma.h"

#include <assert.h>
#include <stdlib.h>

#include "soundtouch/SoundTouch.h"

void CALLBACK SPU2readDMAMem(u16 *pMem, int size, int channel)
{
	u32 spuaddr = C_SPUADDR(channel);

	SPU2_LOG("SPU2 readDMAMem(%d) size %x, addr: %x\n", channel,  size, pMem);

	for (uint i = 0; i < (uint)size; i++)
	{
		*pMem++ = *(u16*)(spu2mem + spuaddr);
		if (spu2attr(channel).irq && (C_IRQA(channel) == spuaddr))
		{
			C_SPUADDR_SET(spuaddr, channel);
			IRQINFO |= (4 * (channel + 1));
			SPU2_LOG("SPU2readDMAMem(%d):interrupt\n", channel);
			irqCallbackSPU2();
		}

		spuaddr++;		   // inc spu addr
		if (spuaddr > 0x0fffff) spuaddr=0; // wrap at 2Mb
	}

	spuaddr += 19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
	C_SPUADDR_SET(spuaddr, channel);

	 // DMA complete
	spu2stat_clear_80(channel);
	SPUStartCycle[channel] = SPUCycles;
	SPUTargetCycle[channel] = size;
	interrupt |=  (1 << (1 + channel));
}

void CALLBACK SPU2readDMA4Mem(u16 *pMem, int size)
{
	LOG_CALLBACK("SPU2readDMA4Mem()\n");
	return SPU2readDMAMem(pMem, size, 0);
}

void CALLBACK SPU2readDMA7Mem(u16* pMem, int size)
{
	LOG_CALLBACK("SPU2readDMA7Mem()\n");
	return SPU2readDMAMem(pMem, size, 1);
}

// WRITE

// AutoDMA's are used to transfer to the DIRECT INPUT area of the spu2 memory
// Left and Right channels are always interleaved together in the transfer so
// the AutoDMA's deinterleaves them and transfers them. An interrupt is
// generated when half of the buffer (256 short-words for left and 256
// short-words for right ) has been transferred. Another interrupt occurs at
// the end of the transfer.

int ADMASWrite(int channel)
{
	u32 spuaddr;
	ADMA *Adma = &adma[channel];

	if (interrupt & 0x2)
	{
		WARN_LOG("ADMASWrite(%d) returning for interrupt\n", channel);
		return 0;
	}

	if (Adma->AmountLeft <= 0)
	{
		WARN_LOG("ADMASWrite(%d) amount left is 0\n", channel);
		return 1;
	}

	assert( Adma->AmountLeft >= 512 );
	spuaddr = C_SPUADDR(channel);
	u32 left_addr = spuaddr + 0x2000 + c_offset(channel);
	u32 right_addr = left_addr + 0x200;

	// SPU2 Deinterleaves the Left and Right Channels
	memcpy((s16*)(spu2mem + left_addr),(s16*)Adma->MemAddr,512);
	Adma->MemAddr += 256;
	memcpy((s16*)(spu2mem + right_addr),(s16*)Adma->MemAddr,512);
	Adma->MemAddr += 256;

	if (spu2attr(channel).irq && (irq_test1(channel, spuaddr) || irq_test2(channel, spuaddr)))
	{
		IRQINFO |= (4 * (channel + 1));
		WARN_LOG("ADMAMem access(%d): interrupt\n", channel);
		irqCallbackSPU2();
	}

	spuaddr = (spuaddr + 256) & 511;
	C_SPUADDR_SET(spuaddr, channel);

	Adma->AmountLeft -= 512;

	return (Adma->AmountLeft <= 0);
}

void SPU2writeDMAMem(u16* pMem, int size, int channel)
{
	u32 spuaddr;
	ADMA *Adma = &adma[channel];
	s32 offset = (channel == 0) ? 0 : 0x400;

	SPU2_LOG("SPU2 writeDMAMem size %x, addr: %x(spu2:%x)"/*, ctrl: %x, adma: %x\n"*/, \
	size, pMem, C_SPUADDR(channel)/*, spu2Ru16(REG_C0_CTRL + offset), spu2Ru16(REG_C0_ADMAS + offset)*/);

	if (spu2admas(channel) && (spu2attr(channel).dma == 0) && size)
	{
		if (!Adma->Enabled ) Adma->Index = 0;

		Adma->MemAddr = pMem;
		Adma->AmountLeft = size;
		SPUTargetCycle[channel] = size;
		spu2stat_clear_80(channel);
		if (!Adma->Enabled || (Adma->Index > 384))
		{
			C_SPUADDR_SET(0, channel);
			if (ADMASWrite(channel))
			{
				SPUStartCycle[channel] = SPUCycles;
				interrupt |= (1 << (1 + channel));
			}
		}
		Adma->Enabled = 1;

		return;
	}

#ifdef ZEROSPU2_DEVBUILD
	if ((conf.Log && conf.options & OPTION_RECORDING) && (channel == 1))
		LogPacketSound(pMem, 0x8000);
#endif

	spuaddr = C_SPUADDR(channel);
	memcpy((u8*)(spu2mem + spuaddr),(u8*)pMem,size << 1);
	spuaddr += size;
	C_SPUADDR_SET(spuaddr, channel);

	if (spu2attr(channel).irq && (spuaddr < C_IRQA(channel) && (C_IRQA(channel) <= (spuaddr + 0x20))))
	{
		IRQINFO |= 4 * (channel + 1);
		SPU2_LOG("SPU2writeDMAMem:interrupt\n");
		irqCallbackSPU2();
	}

	if (spuaddr > 0xFFFFE) spuaddr = 0x2800;
	C_SPUADDR_SET(spuaddr, channel);

	MemAddr[channel] += size << 1;
	spu2stat_clear_80(channel);
	SPUStartCycle[channel] = SPUCycles;
	SPUTargetCycle[channel] = size;
	interrupt |= (1 << (channel + 1));
}

void CALLBACK SPU2interruptDMA(int channel)
{
	SPU2_LOG("SPU2 interruptDMA(%d)\n", channel);
	spu2attr(channel).dma = 0;
	spu2stat_set_80(channel);
}

#ifndef ENABLE_NEW_IOPDMA_SPU2
void CALLBACK SPU2writeDMA4Mem(u16* pMem, int size)
{
	LOG_CALLBACK("SPU2writeDMA4Mem()\n");
	SPU2writeDMAMem(pMem, size, 0);
}

void CALLBACK SPU2writeDMA7Mem(u16* pMem, int size)
{
	LOG_CALLBACK("SPU2writeDMA7Mem()\n");
	SPU2writeDMAMem(pMem, size, 1);
}

void CALLBACK SPU2interruptDMA4()
{
	LOG_CALLBACK("SPU2interruptDMA4()\n");
	SPU2interruptDMA(4);
}

void CALLBACK SPU2interruptDMA7()
{
	LOG_CALLBACK("SPU2interruptDMA7()\n");
	SPU2interruptDMA(7);
}
#else
s32 CALLBACK SPU2dmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	// Needs implementation.
	return 0;
}

s32 CALLBACK SPU2dmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	// Needs implementation.
	return 0;
 }

void CALLBACK SPU2dmaInterrupt(s32 channel)
 {
	LOG_CALLBACK("SPU2dmaInterruptDMA()\n");
	SPU2interruptDMA(channel);
 }
#endif
