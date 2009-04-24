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

#ifndef __PSXHW_H__
#define __PSXHW_H__

#include "R3000A.h"
#include "IopMem.h"

#define HW_USB_START 0x1f801600
#define HW_USB_END 0x1f801700
#define HW_FW_START 0x1f808400
#define HW_FW_END 0x1f808550
#define HW_SPU2_START 0x1f801c00
#define HW_SPU2_END 0x1f801e00

/* Registers for the IOP Counters */
enum IOPCountRegs
{
	IOP_T0_COUNT = 0x1f801100,
	IOP_T1_COUNT = 0x1f801110,
	IOP_T2_COUNT = 0x1f801120,
	IOP_T3_COUNT = 0x1f801480,
	IOP_T4_COUNT = 0x1f801490,
	IOP_T5_COUNT =  0x1f8014a0,
			
	IOP_T0_MODE = 0x1f801104,
	IOP_T1_MODE = 0x1f801114,
	IOP_T2_MODE = 0x1f801124,
	IOP_T3_MODE = 0x1f801484,
	IOP_T4_MODE = 0x1f801494,
	IOP_T5_MODE = 0x1f8014a4,
			
	IOP_T0_TARGET= 0x1f801108,
	IOP_T1_TARGET = 0x1f801118,
	IOP_T2_TARGET = 0x1f801128,
	IOP_T3_TARGET = 0x1f801488,
	IOP_T4_TARGET = 0x1f801498,
	IOP_T5_TARGET = 0x1f8014a8
};

// fixme: I'm sure there's a better way to do this. --arcum42
#define DmaExec(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR & (8 << (n * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

#define DmaExec2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

#ifdef ENABLE_NEW_IOPDMA
#define DmaExecNew(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR & (8 << (n * 4))) { \
		IopDmaStart(n, HW_DMA##n##_CHCR, HW_DMA##n##_MADR, HW_DMA##n##_BCR); \
	} \
}

#define DmaExecNew2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		IopDmaStart(n, HW_DMA##n##_CHCR, HW_DMA##n##_MADR, HW_DMA##n##_BCR); \
	} \
}
#else
#define DmaExecNew(n) DmaExec(n)
#define DmaExecNew2(n) DmaExec2(n)
#endif

#define HW_DMA0_MADR (psxHu32(0x1080)) // MDEC in DMA
#define HW_DMA0_BCR  (psxHu32(0x1084))
#define HW_DMA0_CHCR (psxHu32(0x1088))

#define HW_DMA1_MADR (psxHu32(0x1090)) // MDEC out DMA
#define HW_DMA1_BCR  (psxHu32(0x1094))
#define HW_DMA1_CHCR (psxHu32(0x1098))

#define HW_DMA2_MADR (psxHu32(0x10a0)) // GPU DMA
#define HW_DMA2_BCR  (psxHu32(0x10a4))
#define HW_DMA2_CHCR (psxHu32(0x10a8))
#define HW_DMA2_TADR (psxHu32(0x10ac))

#define HW_DMA3_MADR (psxHu32(0x10b0)) // CDROM DMA
#define HW_DMA3_BCR  (psxHu32(0x10b4))
#define HW_DMA3_BCR_L16 (psxHu16(0x10b4))
#define HW_DMA3_BCR_H16 (psxHu16(0x10b6))
#define HW_DMA3_CHCR (psxHu32(0x10b8))

#define HW_DMA4_MADR (psxHu32(0x10c0)) // SPU DMA
#define HW_DMA4_BCR  (psxHu32(0x10c4))
#define HW_DMA4_CHCR (psxHu32(0x10c8))
#define HW_DMA4_TADR (psxHu32(0x10cc))

#define HW_DMA6_MADR (psxHu32(0x10e0)) // GPU DMA (OT)
#define HW_DMA6_BCR  (psxHu32(0x10e4))
#define HW_DMA6_CHCR (psxHu32(0x10e8))

#define HW_DMA7_MADR (psxHu32(0x1500)) // SPU2 DMA
#define HW_DMA7_BCR  (psxHu32(0x1504))
#define HW_DMA7_CHCR (psxHu32(0x1508))

#define HW_DMA8_MADR (psxHu32(0x1510)) // DEV9 DMA
#define HW_DMA8_BCR  (psxHu32(0x1514))
#define HW_DMA8_CHCR (psxHu32(0x1518))

#define HW_DMA9_MADR (psxHu32(0x1520)) // SIF0 DMA
#define HW_DMA9_BCR  (psxHu32(0x1524))
#define HW_DMA9_CHCR (psxHu32(0x1528))
#define HW_DMA9_TADR (psxHu32(0x152c))

#define HW_DMA10_MADR (psxHu32(0x1530)) // SIF1 DMA
#define HW_DMA10_BCR  (psxHu32(0x1534))
#define HW_DMA10_CHCR (psxHu32(0x1538))

#define HW_DMA11_MADR (psxHu32(0x1540)) // SIO2 in
#define HW_DMA11_BCR  (psxHu32(0x1544))
#define HW_DMA11_CHCR (psxHu32(0x1548))

#define HW_DMA12_MADR (psxHu32(0x1550)) // SIO2 out
#define HW_DMA12_BCR  (psxHu32(0x1554))
#define HW_DMA12_CHCR (psxHu32(0x1558))

#define HW_DMA_PCR   (psxHu32(0x10f0))
#define HW_DMA_ICR   (psxHu32(0x10f4))

#define HW_DMA_PCR2  (psxHu32(0x1570))
#define HW_DMA_ICR2  (psxHu32(0x1574))

enum IopEventId
{
	IopEvt_Cdvd = 5		// General Cdvd commands (Seek, Standby, Break, etc)
,	IopEvt_SIF0 = 9
,	IopEvt_SIF1 = 10
,	IopEvt_Dma11 = 11
,	IopEvt_Dma12 = 12
,	IopEvt_SIO = 16
,	IopEvt_Cdrom = 17
,	IopEvt_CdromRead = 18
,	IopEvt_CdvdRead = 19
,	IopEvt_DEV9 = 20
,	IopEvt_USB = 21
};

extern void PSX_INT( IopEventId n, s32 ecycle);

extern void psxSetNextBranch( u32 startCycle, s32 delta );
extern void psxSetNextBranchDelta( s32 delta );

void psxHwReset();
u8   psxHwRead8 (u32 add);
u16  psxHwRead16(u32 add);
u32  psxHwRead32(u32 add);

void psxHwWrite8 (u32 add, u8  value);
void psxHwWrite16(u32 add, u16 value);
void psxHwWrite32(u32 add, u32 value);

u8   psxHw4Read8 (u32 add);
void psxHw4Write8(u32 add, u8  value);

void psxDmaInterrupt(int n);
void psxDmaInterrupt2(int n);

int  psxHwFreeze(gzFile f, int Mode);

int psxHwConstRead8(u32 x86reg, u32 add, u32 sign);
int psxHwConstRead16(u32 x86reg, u32 add, u32 sign);
int psxHwConstRead32(u32 x86reg, u32 add);
void psxHwConstWrite8(u32 add, int mmreg);
void psxHwConstWrite16(u32 add, int mmreg);
void psxHwConstWrite32(u32 add, int mmreg);
int psxHw4ConstRead8 (u32 x86reg, u32 add, u32 sign);
void psxHw4ConstWrite8(u32 add, int mmreg);

#endif /* __PSXHW_H__ */
