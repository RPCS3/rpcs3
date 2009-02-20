/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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

#include <assert.h>
#include <stdlib.h>

#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

void CALLBACK SPU2readDMAMem(u16 *pMem, int size, int core)
{
	u32 spuaddr;
	int i, dma, offset;
	
	if ( core == 0)
	{
		dma = 4;
		offset = 0;
	}
	else
	{
		dma = 7;
		offset = 0x0400;
	}	

	spuaddr = C_SPUADDR(core);
	
	SPU2_LOG("SPU2 readDMA%dMem size %x, addr: %x\n", dma,  size, pMem);

	for (i=0; i < size; i++)
	{
		*pMem++ = *(u16*)(spu2mem + spuaddr);
		if ((spu2Rs16(REG_C0_CTRL + offset) & 0x40) && (C_IRQA(core) == spuaddr))
		{
			C_SPUADDR_SET(spuaddr, core);
			IRQINFO |= (4 * (core + 1));
			SPU2_LOG("SPU2readDMA%dMem:interrupt\n", dma);
			irqCallbackSPU2();
		}

		spuaddr++;		   // inc spu addr
		if (spuaddr > 0x0fffff) spuaddr=0; // wrap at 2Mb
	}

	spuaddr += 19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
	C_SPUADDR_SET(spuaddr, core);

	 // DMA complete
	spu2Ru16(REG_C0_SPUSTAT + offset) &= ~0x80;									
	SPUStartCycle[core] = SPUCycles;
	SPUTargetCycle[core] = size;
	interrupt |=  (1 << (1 + core));
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

int ADMASWrite(int core)
{
	u32 spuaddr;
	ADMA *Adma;
	int dma, offset;
	
	if (core == 0)
	{
		Adma = &Adma4;
		dma = 4;
		offset = 0;
	}
	else
	{
		Adma = &Adma7;
		dma = 7;
		offset = 0x0400;
	}
	
	
	if (interrupt & 0x2)
	{
		WARN_LOG("%d returning for interrupt\n", dma);
		return 0;
	}
	if (Adma->AmountLeft <= 0)
	{
		WARN_LOG("%d amount left is 0\n", dma);
		return 1;
	}

	assert( Adma->AmountLeft >= 512 );
	spuaddr = C_SPUADDR(core);
	
	// SPU2 Deinterleaves the Left and Right Channels
	memcpy((s16*)(spu2mem + spuaddr + 0x2000 + offset),(s16*)Adma->MemAddr,512);
	Adma->MemAddr += 256;
	memcpy((s16*)(spu2mem + spuaddr + 0x2200 + offset),(s16*)Adma->MemAddr,512);
	Adma->MemAddr += 256;
	
	if ((spu2Ru16(REG_C0_CTRL + offset) & 0x40) && ((spuaddr + 0x2400) <= C_IRQA(core) &&  (spuaddr + 0x2400 + 256) >= C_IRQA(core)))
	{
		IRQINFO |= (4 * (core + 1));
		WARN_LOG("ADMA %d Mem access:interrupt\n", dma);
		irqCallbackSPU2();
	}
	
	if ((spu2Ru16(REG_C0_CTRL + offset) & 0x40) && ((spuaddr + 0x2600) <= C_IRQA(core) &&  (spuaddr + 0x2600 + 256) >= C_IRQA(core)))
	{
		IRQINFO |= (4 * (core + 1));
		WARN_LOG("ADMA %d Mem access:interrupt\n", dma);
		irqCallbackSPU2();
	}

	spuaddr = (spuaddr + 256) & 511;
	C_SPUADDR_SET(spuaddr, core);
	
	Adma->AmountLeft -= 512;

	if (Adma->AmountLeft > 0) 
		return 0;
	else 
		return 1;
}

void CALLBACK SPU2writeDMAMem(u16* pMem, int size, int core)
{
	u32 spuaddr;
	ADMA *Adma;
	int dma, offset;
	
	if (core == 0)
	{
		Adma = &Adma4;
		dma = 4;
		offset = 0;
	}
	else
	{
		Adma = &Adma7;
		dma = 7;
		offset = 0x0400;
	}

	SPU2_LOG("SPU2 writeDMA%dMem size %x, addr: %x(spu2:%x), ctrl: %x, adma: %x\n", \
	dma, size, pMem, C_SPUADDR(core), spu2Ru16(REG_C0_CTRL + offset), spu2Ru16(REG_C0_ADMAS + offset));

	if ((spu2Ru16(REG_C0_ADMAS + offset) & 0x1 * (core + 1)) && ((spu2Ru16(REG_C0_CTRL + offset) & 0x30) == 0) && size)
	{
		if (!Adma->Enabled ) Adma->Index = 0;
	
		Adma->MemAddr = pMem;
		Adma->AmountLeft = size;
		SPUTargetCycle[core] = size;
		spu2Ru16(REG_C0_SPUSTAT + offset) &= ~0x80;
		if (!Adma->Enabled || (Adma->Index > 384)) 
		{
			C_SPUADDR_SET(0, core);
			if (ADMASWrite(core))
			{
				SPUStartCycle[core] = SPUCycles;
				interrupt |= (1 << (1 + core));
			}
		}
		Adma->Enabled = 1;

		return;
	}

#ifdef ZEROSPU2_DEVBUILD
	if ((conf.Log && conf.options & OPTION_RECORDING) && (core == 1))
		LogPacketSound(pMem, 0x8000);
#endif

	spuaddr = C_SPUADDR(core);
	memcpy((u8*)(spu2mem + spuaddr),(u8*)pMem,size << 1);
	spuaddr += size;
	C_SPUADDR_SET(spuaddr, core);
	
	if ((spu2Ru16(REG_C0_CTRL + offset)&0x40) && (spuaddr < C_IRQA(core) && (C_IRQA(core) <= (spuaddr+0x20))))
	{
		IRQINFO |= 4 * (core + 1);
		SPU2_LOG("SPU2writeDMA%dMem:interrupt\n", dma);
		irqCallbackSPU2();
	}
	
	if (spuaddr > 0xFFFFE) spuaddr = 0x2800;
	C_SPUADDR_SET(spuaddr, core);

	MemAddr[core] += size << 1;
	spu2Ru16(REG_C0_SPUSTAT + offset) &= ~0x80;
	SPUStartCycle[core] = SPUCycles;
	SPUTargetCycle[core] = size;
	interrupt |= (1 << (core + 1));
}

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

void CALLBACK SPU2interruptDMA(int core)
{
	int dma, offset;
	
	if (core == 0)
	{
		dma = 4;
		offset = 0;
	}
	else
	{
		dma = 7;
		offset = 0x0400;
	}
	
	SPU2_LOG("SPU2 interruptDMA%d\n", dma);
	spu2Rs16(REG_C0_CTRL + offset) &= ~0x30;
	spu2Ru16(REG_C0_SPUSTAT + offset) |= 0x80;
}

void CALLBACK SPU2interruptDMA4()
{
	LOG_CALLBACK("SPU2interruptDMA4()\n");
	SPU2interruptDMA(0);
}

void CALLBACK SPU2interruptDMA7()
{
	LOG_CALLBACK("SPU2interruptDMA7()\n");
	SPU2interruptDMA(1);
}