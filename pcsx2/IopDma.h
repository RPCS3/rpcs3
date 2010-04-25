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

// defined in PS2Edefs.h

#ifdef ENABLE_NEW_IOPDMA

// unused for now
class DmaBcrReg
{
public:
	union {
		struct {
			u32 size:16;
			u32 count:16;
		};
		u32 whole;
	};

	DmaBcrReg(u32& value)
	{
		whole=value;
	}

	u32 Bytes()
	{
		return 4*size*count;
	}
};

#define DMA_CTRL_ACTIVE		0x01000000
#define DMA_CTRL_DIRECTION	0x00000001

#define DMA_CHANNEL_MAX		14 /* ? */

extern void IopDmaStart(int channel);
extern void IopDmaUpdate(u32 elapsed);

// external dma handlers
extern s32 CALLBACK cdvdDmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
extern void CALLBACK cdvdDmaInterrupt(s32 channel);

//#else
#endif

extern void psxDma2(u32 madr, u32 bcr, u32 chcr);
extern void psxDma3(u32 madr, u32 bcr, u32 chcr);
extern void psxDma6(u32 madr, u32 bcr, u32 chcr);
#ifndef ENABLE_NEW_IOPDMA_SPU2
extern void psxDma4(u32 madr, u32 bcr, u32 chcr);
extern void psxDma7(u32 madr, u32 bcr, u32 chcr);
#endif
#ifndef ENABLE_NEW_IOPDMA_DEV9
extern void psxDma8(u32 madr, u32 bcr, u32 chcr);
#endif
extern void psxDma9(u32 madr, u32 bcr, u32 chcr);
extern void psxDma10(u32 madr, u32 bcr, u32 chcr);

extern int  psxDma4Interrupt();
extern int  psxDma7Interrupt();
extern void dev9Interrupt();
extern void dev9Irq(int cycles);
extern void usbInterrupt();
extern void usbIrq(int cycles);
extern void fwIrq();
extern void spu2Irq();

extern void iopIntcIrq( uint irqType );
extern void iopTestIntc();

extern DEV9handler dev9Handler;
extern USBhandler usbHandler;

#endif /* __PSXDMA_H__ */
