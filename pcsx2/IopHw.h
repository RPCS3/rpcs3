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

#pragma once

#include "IopMem.h"

static const u32
	HW_USB_START	= 0x1f801600,
	HW_USB_END		= 0x1f801700,
	HW_FW_START		= 0x1f808400,
	HW_FW_END		= 0x1f808550,	// end addr for FW is a guess...
	HW_SPU2_START	= 0x1f801c00,
	HW_SPU2_END		= 0x1f801e00;
	
static const u32
	HW_SSBUS_SPD_ADDR	= 0x1f801000,
	HW_SSBUS_PIO_ADDR	= 0x1f801004,
	HW_SSBUS_SPD_DELAY	= 0x1f801008,
	HW_SSBUS_DEV1_DELAY	= 0x1f80100C,
	HW_SSBUS_ROM_DELAY	= 0x1f801010,
	HW_SSBUS_SPU_DELAY	= 0x1f801014,
	HW_SSBUS_DEV5_DELAY	= 0x1f801018,
	HW_SSBUS_PIO_DELAY	= 0x1f80101c,
	HW_SSBUS_COM_DELAY	= 0x1f801020,
	
	HW_SIO_DATA			= 0x1f801040,	// SIO read/write register
	HW_SIO_STAT			= 0x1f801044,
	HW_SIO_MODE			= 0x1f801048,
	HW_SIO_CTRL			= 0x1f80104a,
	HW_SIO_BAUD			= 0x1f80104e,

    HW_RAM_SIZE         = 0x1f801060,
	HW_IREG				= 0x1f801070,
	HW_IMASK			= 0x1f801074,
	HW_ICTRL			= 0x1f801078,

	HW_SSBUS_DEV1_ADDR	= 0x1f801400,
	HW_SSBUS_SPU_ADDR	= 0x1f801404,
	HW_SSBUS_DEV5_ADDR	= 0x1f801408,
	HW_SSBUS_SPU1_ADDR	= 0x1f80140c,
	HW_SSBUS_DEV9_ADDR3	= 0x1f801410,
	HW_SSBUS_SPU1_DELAY	= 0x1f801414,
	HW_SSBUS_DEV9_DELAY2= 0x1f801418,
	HW_SSBUS_DEV9_DELAY3= 0x1f80141c,
	HW_SSBUS_DEV9_DELAY1= 0x1f801420,

	HW_ICFG				= 0x1f801450,
	HW_DEV9_DATA		= 0x1f80146e,	// DEV9 read/write register

	// CDRom registers are used for various command, status, and data stuff.

	HW_CDR_DATA0		= 0x1f801800,	// CDROM multipurpose data register 1
	HW_CDR_DATA1		= 0x1f801801,	// CDROM multipurpose data register 2
	HW_CDR_DATA2		= 0x1f801802,	// CDROM multipurpose data register 3
	HW_CDR_DATA3		= 0x1f801803,	// CDROM multipurpose data register 4

	// SIO2 is a DMA interface for the SIO.

	HW_SIO2_DATAIN		= 0x1F808260,
	HW_SIO2_FIFO		= 0x1f808264,
	HW_SIO2_CTRL		= 0x1f808268,
	HW_SIO2_RECV1		= 0x1f80826c,
	HW_SIO2_RECV2		= 0x1f808270,
	HW_SIO2_RECV3		= 0x1f808274,
	HW_SIO2_8278        = 0x1F808278, // May as well add defs
	HW_SIO2_827C        = 0x1F80827C, // for these 2...
	HW_SIO2_INTR		= 0x1f808280;

enum DMAMadrAddresses
{
    HWx_DMA0_MADR  = 0x1f801080,
    HWx_DMA1_MADR  = 0x1f801090,
    HWx_DMA2_MADR  = 0x1f8010a0,
    HWx_DMA3_MADR  = 0x1f8010b0,
    HWx_DMA4_MADR  = 0x1f8010c0,
    HWx_DMA5_MADR  = 0x1f8010d0,
    HWx_DMA6_MADR  = 0x1f8010e0,
    HWx_DMA7_MADR  = 0x1f801500,
    HWx_DMA8_MADR  = 0x1f801510,
    HWx_DMA9_MADR  = 0x1f801520,
    HWx_DMA10_MADR = 0x1f801530,
    HWx_DMA11_MADR = 0x1f801540,
    HWx_DMA12_MADR = 0x1f801550
};

enum DMABcrAddresses
{
    HWx_DMA0_BCR  = 0x1f801084,
    HWx_DMA1_BCR  = 0x1f801094,
    HWx_DMA2_BCR  = 0x1f8010a4,
    HWx_DMA3_BCR  = 0x1f8010b4,
    HWx_DMA3_BCR_L16  = 0x1f8010b4,
    HWx_DMA3_BCR_H16  = 0x1f8010b6,
    HWx_DMA4_BCR  = 0x1f8010c4,
    HWx_DMA5_BCR  = 0x1f8010d4,
    HWx_DMA6_BCR  = 0x1f8010e4,
    HWx_DMA7_BCR  = 0x1f801504,
    HWx_DMA8_BCR  = 0x1f801514,
    HWx_DMA9_BCR  = 0x1f801524,
    HWx_DMA10_BCR = 0x1f801534,
    HWx_DMA11_BCR = 0x1f801544,
    HWx_DMA12_BCR = 0x1f801554
};

enum DMAChcrAddresses
{
    HWx_DMA0_CHCR  = 0x1f801088,
    HWx_DMA1_CHCR  = 0x1f801098,
    HWx_DMA2_CHCR  = 0x1f8010a8,
    HWx_DMA3_CHCR  = 0x1f8010b8,
    HWx_DMA4_CHCR  = 0x1f8010c8,
    HWx_DMA5_CHCR  = 0x1f8010d8,
    HWx_DMA6_CHCR  = 0x1f8010e8,
    HWx_DMA7_CHCR  = 0x1f801508,
    HWx_DMA8_CHCR  = 0x1f801518,
    HWx_DMA9_CHCR  = 0x1f801528,
    HWx_DMA10_CHCR = 0x1f801538,
    HWx_DMA11_CHCR = 0x1f801548,
    HWx_DMA12_CHCR = 0x1f801558
};

enum DMATadrAddresses
{
    HWx_DMA0_TADR  = 0x1f80108c,
    HWx_DMA1_TADR  = 0x1f80109c,
    HWx_DMA2_TADR  = 0x1f8010ac,
    HWx_DMA3_TADR  = 0x1f8010bc,
    HWx_DMA4_TADR  = 0x1f8010cc,
    HWx_DMA5_TADR  = 0x1f8010dc,
    HWx_DMA6_TADR  = 0x1f8010ec,
    HWx_DMA7_TADR  = 0x1f80150c,
    HWx_DMA8_TADR  = 0x1f80151c,
    HWx_DMA9_TADR  = 0x1f80152c,
    HWx_DMA10_TADR = 0x1f80153c,
    HWx_DMA11_TADR = 0x1f80154c,
    HWx_DMA12_TADR = 0x1f80155c
};

/* Registers for the IOP Counters */
enum IOPCountRegs
{
	IOP_T0_COUNT = 0x1f801100,
	IOP_T1_COUNT = 0x1f801110,
	IOP_T2_COUNT = 0x1f801120,
	IOP_T3_COUNT = 0x1f801480,
	IOP_T4_COUNT = 0x1f801490,
	IOP_T5_COUNT = 0x1f8014a0,
			
	IOP_T0_MODE = 0x1f801104,
	IOP_T1_MODE = 0x1f801114,
	IOP_T2_MODE = 0x1f801124,
	IOP_T3_MODE = 0x1f801484,
	IOP_T4_MODE = 0x1f801494,
	IOP_T5_MODE = 0x1f8014a4,
			
	IOP_T0_TARGET = 0x1f801108,
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
		IopDmaStart(n); \
	} \
}

#define DmaExecNew2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		IopDmaStart(n); \
	} \
}
#endif

struct dma_mbc
{
	u32 madr;
	u32 bcr;
	u32 chcr;
	
	u16 bcr_lower() const
	{
		return (u16)(bcr);
	}
	u16 bcr_upper() const
	{
		return (bcr >> 16);
	}
};

struct dma_mbct
{
	u32 madr;
	u32 bcr;
	u32 chcr;
	u32 tadr;
	
	u16 bcr_lower() const
	{
		return (u16)(bcr);
	}
	u16 bcr_upper() const
	{
		return (bcr >> 16);
	}
};

#define hw_dma0		(*(dma_mbc*) &psxH[0x1080])
#define hw_dma1		(*(dma_mbc*) &psxH[0x1090])
#define hw_dma2		(*(dma_mbct*)&psxH[0x10a0])
#define hw_dma3		(*(dma_mbc*) &psxH[0x10b0])
#define hw_dma4		(*(dma_mbct*)&psxH[0x10c0])
#define hw_dma6		(*(dma_mbc*) &psxH[0x10e0])
#define hw_dma7		(*(dma_mbc*) &psxH[0x1500])
#define hw_dma8		(*(dma_mbc*) &psxH[0x1510])
#define hw_dma9		(*(dma_mbct*)&psxH[0x1520])
#define hw_dma10	(*(dma_mbc*) &psxH[0x1530])
#define hw_dma11	(*(dma_mbc*) &psxH[0x1540])
#define hw_dma12	(*(dma_mbc*) &psxH[0x1550])
#define hw_dma(x)	hw_dma##x

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
	IopEvt_SIFhack = 1	// The SIF likes to fall asleep and never wake up.  This sends intermittent SBUS flags to rewake it.
,	IopEvt_Cdvd = 5		// General Cdvd commands (Seek, Standby, Break, etc)
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
extern int iopTestCycle( u32 startCycle, s32 delta );
extern void _iopTestInterrupts();

extern void psxHwReset();
extern u8   psxHw4Read8 (u32 add);
extern void psxHw4Write8(u32 add, u8  value);

extern void psxDmaInterrupt(int n);
extern void psxDmaInterrupt2(int n);
