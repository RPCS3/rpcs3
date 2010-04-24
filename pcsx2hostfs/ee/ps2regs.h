/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#ifndef PS2REGS_H
#define PS2REGS_H

/* TIMERS */
#define	T0_COUNT	*((volatile unsigned int   *)0x10000000)
#define	T0_MODE		*((volatile unsigned short *)0x10000010)
#define	T0_COMP		*((volatile unsigned short *)0x10000020)
#define	T0_HOLD		*((volatile unsigned short *)0x10000030)

#define	T1_COUNT	*((volatile unsigned int   *)0x10000800)
#define	T1_MODE		*((volatile unsigned short *)0x10000810)
#define	T1_COMP		*((volatile unsigned short *)0x10000820)
#define	T1_HOLD		*((volatile unsigned short *)0x10000830)

#define	T2_COUNT	*((volatile unsigned int   *)0x10001000)
#define	T2_MODE		*((volatile unsigned short *)0x10001010)
#define	T2_COMP		*((volatile unsigned short *)0x10001020)

#define	T3_COUNT	*((volatile unsigned int   *)0x10001800)
#define	T3_MODE		*((volatile unsigned short *)0x10001810)
#define	T3_COMP		*((volatile unsigned short *)0x10001820)

/* IPU */
#define	IPU_CMD		*((volatile unsigned long *)0x10002000)
#define	IPU_CTRL	*((volatile unsigned int  *)0x10002010)
#define	IPU_BP		*((volatile unsigned int  *)0x10002020)
#define	IPU_TOP		*((volatile unsigned long *)0x10002030)

/* GIF */
#define	GIF_CTRL	*((volatile unsigned int *)0x10003000)
#define	GIF_MODE	*((volatile unsigned int *)0x10003010)
#define	GIF_STAT	*((volatile unsigned int *)0x10003020)

#define	GIF_TAG0	*((volatile unsigned int *)0x10003040)
#define	GIF_TAG1	*((volatile unsigned int *)0x10003050)
#define	GIF_TAG2	*((volatile unsigned int *)0x10003060)
#define	GIF_TAG3	*((volatile unsigned int *)0x10003070)
#define	GIF_CNT		*((volatile unsigned int *)0x10003080)
#define	GIF_P3CNT	*((volatile unsigned int *)0x10003090)
#define	GIF_P3TAG	*((volatile unsigned int *)0x100030a0)

/* VIF */
#define	VIF0_STAT	*((volatile unsigned int *)0x10003800)
#define	VIF0_FBRST	*((volatile unsigned int *)0x10003810)
#define	VIF0_ERR	*((volatile unsigned int *)0x10003820)
#define	VIF0_MARK	*((volatile unsigned int *)0x10003830)
#define	VIF0_CYCLE	*((volatile unsigned int *)0x10003840)
#define	VIF0_MODE	*((volatile unsigned int *)0x10003850)
#define	VIF0_NUM	*((volatile unsigned int *)0x10003860)
#define	VIF0_MASK	*((volatile unsigned int *)0x10003870)
#define	VIF0_CODE	*((volatile unsigned int *)0x10003880)
#define	VIF0_ITOPS	*((volatile unsigned int *)0x10003890)

#define	VIF0_ITOP	*((volatile unsigned int *)0x100038d0)

#define	VIF0_R0		*((volatile unsigned int *)0x10003900)
#define	VIF0_R1		*((volatile unsigned int *)0x10003910)
#define	VIF0_R2		*((volatile unsigned int *)0x10003920)
#define	VIF0_R3		*((volatile unsigned int *)0x10003930)
#define	VIF0_C0		*((volatile unsigned int *)0x10003940)
#define	VIF0_C1		*((volatile unsigned int *)0x10003950)
#define	VIF0_C2		*((volatile unsigned int *)0x10003960)
#define	VIF0_C3		*((volatile unsigned int *)0x10003970)

#define	VIF1_STAT	*((volatile unsigned int *)0x10003c00)
#define	VIF1_FBRST	*((volatile unsigned int *)0x10003c10)
#define	VIF1_ERR	*((volatile unsigned int *)0x10003c20)
#define	VIF1_MARK	*((volatile unsigned int *)0x10003c30)
#define	VIF1_CYCLE	*((volatile unsigned int *)0x10003c40)
#define	VIF1_MODE	*((volatile unsigned int *)0x10003c50)
#define	VIF1_NUM	*((volatile unsigned int *)0x10003c60)
#define	VIF1_MASK	*((volatile unsigned int *)0x10003c70)
#define	VIF1_CODE	*((volatile unsigned int *)0x10003c80)
#define	VIF1_ITOPS	*((volatile unsigned int *)0x10003c90)
#define	VIF1_BASE	*((volatile unsigned int *)0x10003ca0)
#define	VIF1_OFST	*((volatile unsigned int *)0x10003cb0)
#define	VIF1_TOPS	*((volatile unsigned int *)0x10003cc0)
#define	VIF1_ITOP	*((volatile unsigned int *)0x10003cd0)
#define	VIF1_TOP	*((volatile unsigned int *)0x10003ce0)

#define	VIF1_R0		*((volatile unsigned int *)0x10003d00)
#define	VIF1_R1		*((volatile unsigned int *)0x10003d10)
#define	VIF1_R2		*((volatile unsigned int *)0x10003d20)
#define	VIF1_R3		*((volatile unsigned int *)0x10003d30)
#define	VIF1_C0		*((volatile unsigned int *)0x10003d40)
#define	VIF1_C1		*((volatile unsigned int *)0x10003d50)
#define	VIF1_C2		*((volatile unsigned int *)0x10003d60)
#define	VIF1_C3		*((volatile unsigned int *)0x10003d70)

/* FIFO */
//#define	VIF0_FIFO(write)		*((volatile unsigned int128 *)0x10004000)
//#define	VIF1_FIFO(read/write)	*((volatile unsigned int128 *)0x10005000)
//#define	GIF_FIFO(write)			*((volatile unsigned int128 *)0x10006000)
//#define	IPU_out_FIFO(read)		*((volatile unsigned int128 *)0x10007000)
//#define	IPU_in_FIFO(write)		*((volatile unsigned int128 *)0x10007010)

/* DMAC */
#define CHCR		0x00
#define MADR		0x04
#define QWC			0x08
#define TADR		0x0C
#define ASR0		0x10
#define ASR1		0x14

#define DMA0		((volatile unsigned int *)0x10008000)
#define DMA1		((volatile unsigned int *)0x10009000)
#define DMA2		((volatile unsigned int *)0x1000a000)
#define DMA3		((volatile unsigned int *)0x1000b000)
#define DMA4		((volatile unsigned int *)0x1000b400)
#define DMA5		((volatile unsigned int *)0x1000c000)
#define DMA6		((volatile unsigned int *)0x1000c400)
#define DMA7		((volatile unsigned int *)0x1000c800)
#define DMA8		((volatile unsigned int *)0x1000d000)
#define DMA9		((volatile unsigned int *)0x1000d400)

#define	D0_CHCR		*((volatile unsigned int *)0x10008000)
#define	D0_MADR		*((volatile unsigned int *)0x10008010)
#define	D0_QWC		*((volatile unsigned int *)0x10008020)
#define	D0_TADR		*((volatile unsigned int *)0x10008030)
#define	D0_ASR0		*((volatile unsigned int *)0x10008040)
#define	D0_ASR1		*((volatile unsigned int *)0x10008050)

#define	D1_CHCR		*((volatile unsigned int *)0x10009000)
#define	D1_MADR		*((volatile unsigned int *)0x10009010)
#define	D1_QWC		*((volatile unsigned int *)0x10009020)
#define	D1_TADR		*((volatile unsigned int *)0x10009030)
#define	D1_ASR0		*((volatile unsigned int *)0x10009040)
#define	D1_ASR1		*((volatile unsigned int *)0x10009050)

#define	D2_CHCR		*((volatile unsigned int *)0x1000a000)
#define	D2_MADR		*((volatile unsigned int *)0x1000a010)
#define	D2_QWC		*((volatile unsigned int *)0x1000a020)
#define	D2_TADR		*((volatile unsigned int *)0x1000a030)
#define	D2_ASR0		*((volatile unsigned int *)0x1000a040)
#define	D2_ASR1		*((volatile unsigned int *)0x1000a050)

#define	D3_CHCR		*((volatile unsigned int *)0x1000b000)
#define	D3_MADR		*((volatile unsigned int *)0x1000b010)
#define	D3_QWC		*((volatile unsigned int *)0x1000b020)

#define	D4_CHCR		*((volatile unsigned int *)0x1000b400)
#define	D4_MADR		*((volatile unsigned int *)0x1000b410)
#define	D4_QWC		*((volatile unsigned int *)0x1000b420)
#define	D4_TADR		*((volatile unsigned int *)0x1000b430)

#define	D5_CHCR		*((volatile unsigned int *)0x1000c000)
#define	D5_MADR		*((volatile unsigned int *)0x1000c010)
#define	D5_QWC		*((volatile unsigned int *)0x1000c020)

#define	D6_CHCR		*((volatile unsigned int *)0x1000c400)
#define	D6_MADR		*((volatile unsigned int *)0x1000c410)
#define	D6_QWC		*((volatile unsigned int *)0x1000c420)

#define	D7_CHCR		*((volatile unsigned int *)0x1000c800)
#define	D7_MADR		*((volatile unsigned int *)0x1000c810)
#define	D7_QWC		*((volatile unsigned int *)0x1000c820)

#define	D8_CHCR		*((volatile unsigned int *)0x1000d000)
#define	D8_MADR		*((volatile unsigned int *)0x1000d010)
#define	D8_QWC		*((volatile unsigned int *)0x1000d020)

#define	D8_SADR		*((volatile unsigned int *)0x1000d080)

#define	D9_CHCR		*((volatile unsigned int *)0x1000d400)
#define	D9_MADR		*((volatile unsigned int *)0x1000d410)
#define	D9_QWC		*((volatile unsigned int *)0x1000d420)
#define	D9_TADR		*((volatile unsigned int *)0x1000d430)

#define	D9_SADR		*((volatile unsigned int *)0x1000d480)

/* DMAC */
#define	D_CTRL		*((volatile unsigned int *)0x1000e000)
#define	D_STAT		*((volatile unsigned int *)0x1000e010)
#define	D_PCR		*((volatile unsigned int *)0x1000e020)
#define	D_SQWC		*((volatile unsigned int *)0x1000e030)
#define	D_RBSR		*((volatile unsigned int *)0x1000e040)
#define	D_RBOR		*((volatile unsigned int *)0x1000e050)
#define	D_STADR		*((volatile unsigned int *)0x1000e060)
/* INTC */
//#define	I_STAT 0x1000f000
//#define	I_MASK 0x1000f010

/* TOOL putchar */
// .byte KPUTCHAR (0x1000f180)
/* SIF */
// #define	SB_SMFLG (0x1000f230)

/* DMAC */
#define	D_ENABLER	*((volatile unsigned int *)0x1000f520)
#define	D_ENABLEW	*((volatile unsigned int *)0x1000f590)

/* VU Mem */
#define	VUMicroMem0	*((volatile unsigned int *)0x11000000)
#define	VUMem0		*((volatile unsigned int *)0x11004000)
#define	VUMicroMem1	*((volatile unsigned int *)0x11008000)
#define	VUMem1		*((volatile unsigned int *)0x1100c000)

/* GS Privileged */
#define	GS_PMODE	*((volatile unsigned long *)0x12000000)
#define	GS_SMODE1	*((volatile unsigned long *)0x12000010)
#define	GS_SMODE2	*((volatile unsigned long *)0x12000020)
#define	GS_SRFSH	*((volatile unsigned long *)0x12000030)
#define	GS_SYNCH1	*((volatile unsigned long *)0x12000040)
#define	GS_SYNCH2	*((volatile unsigned long *)0x12000050)
#define	GS_SYNCV	*((volatile unsigned long *)0x12000060)
#define	GS_DISPFB1	*((volatile unsigned long *)0x12000070)
#define	GS_DISPLAY1	*((volatile unsigned long *)0x12000080)
#define	GS_DISPFB2	*((volatile unsigned long *)0x12000090)
#define	GS_DISPLAY2	*((volatile unsigned long *)0x120000a0)
#define	GS_EXTBUF	*((volatile unsigned long *)0x120000b0)
#define	GS_EXTDATA	*((volatile unsigned long *)0x120000c0)
#define	GS_EXTWRITE	*((volatile unsigned long *)0x120000d0)
#define	GS_BGCOLOR	*((volatile unsigned long *)0x120000e0)

#define	GS_CSR		*((volatile unsigned long *)0x12001000)
#define	GS_IMR		*((volatile unsigned long *)0x12001010)

#define	GS_BUSDIR	*((volatile unsigned long *)0x12001040)

#define	GS_SIGLBLID	*((volatile unsigned long *)0x12001080)

/* GS General */
#define GS_PRIM			0x00
#define GS_RGBAQ		0x01
#define GS_ST			0x02
#define GS_UV			0x03
#define GS_XYZF2		0x04
#define GS_XYZ2			0x05
#define GS_TEX0_1		0x06
#define GS_TEX0_2		0x07
#define GS_CLAMP_1		0x08
#define GS_CLAMP_2		0x09
#define GS_FOG			0x0a
#define GS_XYZF3		0x0c
#define GS_XYZ3			0x0d
#define GS_AD			0x0e
#define GS_TEX1_1		0x14
#define GS_TEX1_2		0x15
#define GS_TEX2_1		0x16
#define GS_TEX2_2		0x17
#define GS_XYOFFSET_1	0x18
#define GS_XYOFFSET_2	0x19
#define GS_PRMODECONT	0x1a
#define GS_PRMODE		0x1b
#define GS_TEXCLUT		0x1c
#define GS_SCANMSK		0x22
#define GS_MIPTBP1_1	0x34
#define GS_MIPTBP1_2	0x35
#define GS_MIPTBP2_1	0x36
#define GS_MIPTBP2_2	0x37
#define GS_TEXA			0x3b
#define GS_FOGCOL		0x3d
#define GS_TEXFLUSH		0x3f
#define GS_SCISSOR_1	0x40
#define GS_SCISSOR_2	0x41
#define GS_ALPHA_1		0x42
#define GS_ALPHA_2		0x43
#define GS_DIMX			0x44
#define GS_DTHE			0x45
#define GS_COLCLAMP		0x46
#define GS_TEST_1		0x47
#define GS_TEST_2		0x48
#define GS_PABE			0x49
#define GS_FBA_1		0x4a
#define GS_FBA_2		0x4b
#define GS_FRAME_1		0x4c
#define GS_FRAME_2		0x4d
#define GS_ZBUF_1		0x4e
#define GS_ZBUF_2		0x4f
#define GS_BITBLTBUF	0x50
#define GS_TRXPOS		0x51
#define GS_TRXREG		0x52
#define GS_TRXDIR		0x53
#define GS_HWREG		0x54
#define GS_SIGNAL		0x60
#define GS_FINISH		0x61
#define GS_LABEL		0x62

#endif
