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

typedef s32(* DmaHandler)(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
typedef void (* DmaIHandler)(s32 channel);

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

struct DmaHandlerInfo
{
public:
	const char* Name;
	u32 DirectionFlags;
	u32 DmacRegisterBase;
	DmaHandler  Read;
	DmaHandler  Write;
	DmaIHandler Interrupt;

	// runtime variables
	bool Activated; // this is turned on when the channel is first handled,
				 // and serves as a signal that the channel shoudl be handled by the loop.
				 // temporary until I code a better method
	u32 ByteCount;
	s32 NextUpdate;

	u32& REG_MADR(void);
	u32& REG_BCR(void);
	u32& REG_CHCR(void);
	u32& REG_TADR(void);
};

// FIXME: Dummy constants, to be "filled in" with proper values later
#define DMA_CTRL_ACTIVE		0x01000000
#define DMA_CTRL_DIRECTION	0x00000001

#define DMA_CHANNEL_MAX		16 /* ? */

extern void IopDmaStart(int channel);
extern void IopDmaUpdate(u32 elapsed);

// external dma handlers
extern s32 cdvdDmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed);
extern void cdvdDmaInterrupt(s32 channel);

//#else
#endif

extern void psxDma2(u32 madr, u32 bcr, u32 chcr);
extern void psxDma3(u32 madr, u32 bcr, u32 chcr);
extern void psxDma4(u32 madr, u32 bcr, u32 chcr);
extern void psxDma6(u32 madr, u32 bcr, u32 chcr);
extern void psxDma7(u32 madr, u32 bcr, u32 chcr);
extern void psxDma8(u32 madr, u32 bcr, u32 chcr);
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
