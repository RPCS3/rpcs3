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

#ifndef __PSXDMA_H__
#define __PSXDMA_H__

#include "PS2Edefs.h"

//#define ENABLE_NEW_IOPDMA

#ifdef ENABLE_NEW_IOPDMA

typedef s32(* DmaHandler)(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
typedef void (* DmaIHandler)(s32 channel);

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
	s32 Target;
};

// FIXME: Dummy constants, to be "filled in" with proper values later
#define DMA_CTRL_ACTIVE		0x01000000
#define DMA_CTRL_DIRECTION	0x00000001

#define DMA_CHANNEL_MAX		16 /* ? */

// WARNING: CALLER ****[MUST]**** CALL IopDmaUpdate RIGHT AFTER THIS!
void IopDmaStart(int channel, u32 chcr, u32 madr, u32 bcr);
void IopDmaUpdate(u32 elapsed);

// external dma handlers
extern s32 cdvdDmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
extern void cdvdDmaInterrupt(s32 channel);

//#else
#endif

void psxDma2(u32 madr, u32 bcr, u32 chcr);
void psxDma3(u32 madr, u32 bcr, u32 chcr);
void psxDma4(u32 madr, u32 bcr, u32 chcr);
void psxDma6(u32 madr, u32 bcr, u32 chcr);
void psxDma7(u32 madr, u32 bcr, u32 chcr);
void psxDma8(u32 madr, u32 bcr, u32 chcr);
void psxDma9(u32 madr, u32 bcr, u32 chcr);
void psxDma10(u32 madr, u32 bcr, u32 chcr);

int  psxDma4Interrupt();
int  psxDma7Interrupt();
void  dev9Interrupt();
void dev9Irq(int cycles);
void  usbInterrupt();
void usbIrq(int cycles);
void fwIrq();
void spu2Irq();

extern void iopIntcIrq( uint irqType );
extern void iopTestIntc();

extern DEV9handler dev9Handler;
extern USBhandler usbHandler;

#endif /* __PSXDMA_H__ */
