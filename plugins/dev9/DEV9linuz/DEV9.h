#ifndef __DEV9_H__
#define __DEV9_H__

#include <stdio.h>

#define DEV9defs
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#include "PS2Edefs.h"

#ifdef __WIN32__


#define usleep(x)	Sleep(x / 1000)
#include <windows.h>
#include <windowsx.h>
#include <winioctl.h>

#else

#include <gtk/gtk.h>

#define __inline inline

#endif

#define DEV9_LOG __Log

#define ETH_DEF		"eth0"
#define HDD_DEF		"DEV9hdd.raw"

typedef struct {
	char Eth[256];
	char Hdd[256];
	int HddSize;

	int hddEnable;
	int ethEnable;
} Config;

Config config;

typedef struct {
	s8 dev9R[0x10000];
	int eeprompos;
	u8 eeprom[65];
	u8 buffer[2048];
	u16 atabuf[1024];
	u32 atacount;
	u32 atasize;
	u16 phyregs[32];
	int irqcause;
	u8  atacmd;
	u32 atasector;
	u32 atansector;
} dev9Struct;

dev9Struct dev9;

#define dev9Rs8(mem)	dev9.dev9R[(mem) & 0xffff]
#define dev9Rs16(mem)	(*(s16*)&dev9.dev9R[(mem) & 0xffff])
#define dev9Rs32(mem)	(*(s32*)&dev9.dev9R[(mem) & 0xffff])
#define dev9Ru8(mem)	(*(u8*) &dev9.dev9R[(mem) & 0xffff])
#define dev9Ru16(mem)	(*(u16*)&dev9.dev9R[(mem) & 0xffff])
#define dev9Ru32(mem)	(*(u32*)&dev9.dev9R[(mem) & 0xffff])

void *hdd;
int ThreadRun;

s32  _DEV9open();
void _DEV9close();
DEV9callback DEV9irq;
void DEV9thread();

FILE *dev9Log;
void __Log(char *fmt, ...);

void SysMessage(char *fmt, ...);

#define DEV9_R_REV	0x1f80146e


/*
 * SPEED (ASIC on SMAP) register definitions.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * * code included from the ps2smap iop driver, modified by linuzappz  *
 */

#define SPD_REGBASE			0x10000000

#define SPD_R_REV			(SPD_REGBASE + 0x00)
#define SPD_R_REV_1			(SPD_REGBASE + 0x02)
#define SPD_R_REV_3			(SPD_REGBASE + 0x04)
#define SPD_R_0e			(SPD_REGBASE + 0x0e)

#define SPD_R_DMA_CTRL			(SPD_REGBASE + 0x24)
#define SPD_R_INTR_STAT			(SPD_REGBASE + 0x28)
#define SPD_R_INTR_MASK			(SPD_REGBASE + 0x2a)
#define SPD_R_PIO_DIR			(SPD_REGBASE + 0x2c)
#define SPD_R_PIO_DATA			(SPD_REGBASE + 0x2e)
#define	  SPD_PP_DOUT		(1<<4)	/* Data output, read port */
#define	  SPD_PP_DIN		(1<<5)	/* Data input,  write port */
#define	  SPD_PP_SCLK		(1<<6)	/* Clock,       write port */
#define	  SPD_PP_CSEL		(1<<7)	/* Chip select, write port */
/* Operation codes */
#define	  SPD_PP_OP_READ	2
#define	  SPD_PP_OP_WRITE	1
#define	  SPD_PP_OP_EWEN	0
#define	  SPD_PP_OP_EWDS	0

#define SPD_R_XFR_CTRL			(SPD_REGBASE + 0x32)
#define SPD_R_IF_CTRL			(SPD_REGBASE + 0x64)
#define   SPD_IF_ATA_RESET	0x80
#define   SPD_IF_DMA_ENABLE	0x04
#define SPD_R_PIO_MODE			(SPD_REGBASE + 0x70)
#define SPD_R_MWDMA_MODE		(SPD_REGBASE + 0x72)
#define SPD_R_UDMA_MODE			(SPD_REGBASE + 0x74)


/*
 * SMAP (PS2 Network Adapter) register definitions.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * * code included from the ps2smap iop driver, modified by linuzappz  *
 */


/* SMAP interrupt status bits (selected from the SPEED device).  */
#define	SMAP_INTR_EMAC3			(1<<6)
#define	SMAP_INTR_RXEND			(1<<5)
#define	SMAP_INTR_TXEND			(1<<4)
#define	SMAP_INTR_RXDNV			(1<<3)	/* descriptor not valid */
#define	SMAP_INTR_TXDNV			(1<<2)	/* descriptor not valid */
#define	SMAP_INTR_CLR_ALL		(SMAP_INTR_RXEND|SMAP_INTR_TXEND|SMAP_INTR_RXDNV)
#define	SMAP_INTR_ENA_ALL		(SMAP_INTR_EMAC3|SMAP_INTR_CLR_ALL)
#define	SMAP_INTR_BITMSK		0x7C

/* SMAP Register Definitions.  */

#define SMAP_REGBASE			(SPD_REGBASE + 0x100)

#define	SMAP_R_BD_MODE			(SMAP_REGBASE + 0x02)
#define	  SMAP_BD_SWAP		(1<<0)

#define	SMAP_R_INTR_CLR			(SMAP_REGBASE + 0x28)

/* SMAP FIFO Registers.  */

#define	SMAP_R_TXFIFO_CTRL		(SMAP_REGBASE + 0xf00)
#define	  SMAP_TXFIFO_RESET	(1<<0)
#define   SMAP_TXFIFO_DMAEN	(1<<1)
#define	SMAP_R_TXFIFO_WR_PTR		(SMAP_REGBASE + 0xf04)
#define SMAP_R_TXFIFO_SIZE		(SMAP_REGBASE + 0xf08)
#define	SMAP_R_TXFIFO_FRAME_CNT		(SMAP_REGBASE + 0xf0C)
#define	SMAP_R_TXFIFO_FRAME_INC		(SMAP_REGBASE + 0xf10)
#define	SMAP_R_TXFIFO_DATA		(SMAP_REGBASE + 0x1000)

#define	SMAP_R_RXFIFO_CTRL		(SMAP_REGBASE + 0xf30)
#define	  SMAP_RXFIFO_RESET	(1<<0)
#define   SMAP_RXFIFO_DMAEN	(1<<1)
#define	SMAP_R_RXFIFO_RD_PTR		(SMAP_REGBASE + 0xf34)
#define SMAP_R_RXFIFO_SIZE		(SMAP_REGBASE + 0xf38)
#define	SMAP_R_RXFIFO_FRAME_CNT		(SMAP_REGBASE + 0xf3C)
#define	SMAP_R_RXFIFO_FRAME_DEC		(SMAP_REGBASE + 0xf40)
#define	SMAP_R_RXFIFO_DATA		(SMAP_REGBASE + 0x1100)

#define	SMAP_R_FIFO_ADDR		(SMAP_REGBASE + 0x1200)
#define	  SMAP_FIFO_CMD_READ	(1<<1)
#define	  SMAP_FIFO_DATA_SWAP	(1<<0)
#define	SMAP_R_FIFO_DATA		(SMAP_REGBASE + 0x1208)

/* EMAC3 Registers.  */

#define	SMAP_EMAC3_REGBASE		(SMAP_REGBASE + 0x1f00)

#define	SMAP_R_EMAC3_MODE0_L		(SMAP_EMAC3_REGBASE + 0x00)
#define	  SMAP_E3_RXMAC_IDLE		(1<<15)
#define	  SMAP_E3_TXMAC_IDLE		(1<<14)
#define	  SMAP_E3_SOFT_RESET		(1<<13)
#define	  SMAP_E3_TXMAC_ENABLE		(1<<12)
#define	  SMAP_E3_RXMAC_ENABLE		(1<<11)
#define	  SMAP_E3_WAKEUP_ENABLE		(1<<10)
#define	SMAP_R_EMAC3_MODE0_H		(SMAP_EMAC3_REGBASE + 0x02)

#define	SMAP_R_EMAC3_MODE1		(SMAP_EMAC3_REGBASE + 0x04)
#define	SMAP_R_EMAC3_MODE1_L		(SMAP_EMAC3_REGBASE + 0x04)
#define	SMAP_R_EMAC3_MODE1_H		(SMAP_EMAC3_REGBASE + 0x06)
#define	  SMAP_E3_FDX_ENABLE		(1<<31)
#define	  SMAP_E3_INLPBK_ENABLE		(1<<30)	/* internal loop back */
#define	  SMAP_E3_VLAN_ENABLE		(1<<29)
#define	  SMAP_E3_FLOWCTRL_ENABLE	(1<<28)	/* integrated flow ctrl(pause frame) */
#define	  SMAP_E3_ALLOW_PF		(1<<27)	/* allow pause frame */
#define	  SMAP_E3_ALLOW_EXTMNGIF	(1<<25)	/* allow external management IF */
#define	  SMAP_E3_IGNORE_SQE		(1<<24)
#define	  SMAP_E3_MEDIA_FREQ_BITSFT	(22)
#define	    SMAP_E3_MEDIA_10M		(0<<22)
#define	    SMAP_E3_MEDIA_100M		(1<<22)
#define	    SMAP_E3_MEDIA_1000M		(2<<22)
#define	    SMAP_E3_MEDIA_MSK		(3<<22)
#define	  SMAP_E3_RXFIFO_SIZE_BITSFT	(20)
#define	    SMAP_E3_RXFIFO_512		(0<<20)
#define	    SMAP_E3_RXFIFO_1K		(1<<20)
#define	    SMAP_E3_RXFIFO_2K		(2<<20)
#define	    SMAP_E3_RXFIFO_4K		(3<<20)
#define	  SMAP_E3_TXFIFO_SIZE_BITSFT	(18)
#define	    SMAP_E3_TXFIFO_512		(0<<18)
#define	    SMAP_E3_TXFIFO_1K		(1<<18)
#define	    SMAP_E3_TXFIFO_2K		(2<<18)
#define	  SMAP_E3_TXREQ0_BITSFT		(15)
#define	    SMAP_E3_TXREQ0_SINGLE	(0<<15)
#define	    SMAP_E3_TXREQ0_MULTI	(1<<15)
#define	    SMAP_E3_TXREQ0_DEPEND	(2<<15)
#define	  SMAP_E3_TXREQ1_BITSFT		(13)
#define	    SMAP_E3_TXREQ1_SINGLE	(0<<13)
#define	    SMAP_E3_TXREQ1_MULTI	(1<<13)
#define	    SMAP_E3_TXREQ1_DEPEND	(2<<13)
#define	  SMAP_E3_JUMBO_ENABLE		(1<<12)

#define	SMAP_R_EMAC3_TxMODE0_L		(SMAP_EMAC3_REGBASE + 0x08)
#define	  SMAP_E3_TX_GNP_0			(1<<15)	/* get new packet */
#define	  SMAP_E3_TX_GNP_1			(1<<14)	/* get new packet */
#define	  SMAP_E3_TX_GNP_DEPEND		(1<<13)	/* get new packet */
#define	  SMAP_E3_TX_FIRST_CHANNEL	(1<<12)
#define	SMAP_R_EMAC3_TxMODE0_H		(SMAP_EMAC3_REGBASE + 0x0A)

#define	SMAP_R_EMAC3_TxMODE1_L		(SMAP_EMAC3_REGBASE + 0x0C)
#define	SMAP_R_EMAC3_TxMODE1_H		(SMAP_EMAC3_REGBASE + 0x0E)
#define	  SMAP_E3_TX_LOW_REQ_MSK	(0x1F)	/* low priority request */
#define	  SMAP_E3_TX_LOW_REQ_BITSFT	(27)	/* low priority request */
#define	  SMAP_E3_TX_URG_REQ_MSK	(0xFF)	/* urgent priority request */
#define	  SMAP_E3_TX_URG_REQ_BITSFT	(16)	/* urgent priority request */

#define	SMAP_R_EMAC3_RxMODE				(SMAP_EMAC3_REGBASE + 0x10)
#define	SMAP_R_EMAC3_RxMODE_L			(SMAP_EMAC3_REGBASE + 0x10)
#define	SMAP_R_EMAC3_RxMODE_H			(SMAP_EMAC3_REGBASE + 0x12)
#define	  SMAP_E3_RX_STRIP_PAD			(1<<31)
#define	  SMAP_E3_RX_STRIP_FCS			(1<<30)
#define	  SMAP_E3_RX_RX_RUNT_FRAME		(1<<29)
#define	  SMAP_E3_RX_RX_FCS_ERR			(1<<28)
#define	  SMAP_E3_RX_RX_TOO_LONG_ERR	(1<<27)
#define	  SMAP_E3_RX_RX_IN_RANGE_ERR	(1<<26)
#define	  SMAP_E3_RX_PROP_PF			(1<<25)	/* propagate pause frame */
#define	  SMAP_E3_RX_PROMISC			(1<<24)
#define	  SMAP_E3_RX_PROMISC_MCAST		(1<<23)
#define	  SMAP_E3_RX_INDIVID_ADDR		(1<<22)
#define	  SMAP_E3_RX_INDIVID_HASH		(1<<21)
#define	  SMAP_E3_RX_BCAST				(1<<20)
#define	  SMAP_E3_RX_MCAST				(1<<19)

#define	SMAP_R_EMAC3_INTR_STAT		(SMAP_EMAC3_REGBASE + 0x14)
#define	SMAP_R_EMAC3_INTR_STAT_L	(SMAP_EMAC3_REGBASE + 0x14)
#define	SMAP_R_EMAC3_INTR_STAT_H	(SMAP_EMAC3_REGBASE + 0x16)
#define	SMAP_R_EMAC3_INTR_ENABLE	(SMAP_EMAC3_REGBASE + 0x18)
#define	SMAP_R_EMAC3_INTR_ENABLE_L	(SMAP_EMAC3_REGBASE + 0x18)
#define	SMAP_R_EMAC3_INTR_ENABLE_H	(SMAP_EMAC3_REGBASE + 0x1C)
#define	  SMAP_E3_INTR_OVERRUN		(1<<25)	/* this bit does NOT WORKED */
#define	  SMAP_E3_INTR_PF			(1<<24)
#define	  SMAP_E3_INTR_BAD_FRAME	(1<<23)
#define	  SMAP_E3_INTR_RUNT_FRAME	(1<<22)
#define	  SMAP_E3_INTR_SHORT_EVENT	(1<<21)
#define	  SMAP_E3_INTR_ALIGN_ERR	(1<<20)
#define	  SMAP_E3_INTR_BAD_FCS		(1<<19)
#define	  SMAP_E3_INTR_TOO_LONG		(1<<18)
#define	  SMAP_E3_INTR_OUT_RANGE_ERR	(1<<17)
#define	  SMAP_E3_INTR_IN_RANGE_ERR	(1<<16)
#define	  SMAP_E3_INTR_DEAD_DEPEND	(1<<9)
#define	  SMAP_E3_INTR_DEAD_0		(1<<8)
#define	  SMAP_E3_INTR_SQE_ERR_0	(1<<7)
#define	  SMAP_E3_INTR_TX_ERR_0		(1<<6)
#define	  SMAP_E3_INTR_DEAD_1		(1<<5)
#define	  SMAP_E3_INTR_SQE_ERR_1	(1<<4)
#define	  SMAP_E3_INTR_TX_ERR_1		(1<<3)
#define	  SMAP_E3_INTR_MMAOP_SUCCESS	(1<<1)
#define	  SMAP_E3_INTR_MMAOP_FAIL	(1<<0)
#define	  SMAP_E3_INTR_ALL	\
	(SMAP_E3_INTR_OVERRUN|SMAP_E3_INTR_PF|SMAP_E3_INTR_BAD_FRAME| \
	 SMAP_E3_INTR_RUNT_FRAME|SMAP_E3_INTR_SHORT_EVENT| \
	 SMAP_E3_INTR_ALIGN_ERR|SMAP_E3_INTR_BAD_FCS| \
	 SMAP_E3_INTR_TOO_LONG|SMAP_E3_INTR_OUT_RANGE_ERR| \
	 SMAP_E3_INTR_IN_RANGE_ERR| \
	 SMAP_E3_INTR_DEAD_DEPEND|SMAP_E3_INTR_DEAD_0| \
	 SMAP_E3_INTR_SQE_ERR_0|SMAP_E3_INTR_TX_ERR_0| \
	 SMAP_E3_INTR_DEAD_1|SMAP_E3_INTR_SQE_ERR_1| \
	 SMAP_E3_INTR_TX_ERR_1| \
	 SMAP_E3_INTR_MMAOP_SUCCESS|SMAP_E3_INTR_MMAOP_FAIL)
#define	  SMAP_E3_DEAD_ALL	\
	(SMAP_E3_INTR_DEAD_DEPEND|SMAP_E3_INTR_DEAD_0| \
	 SMAP_E3_INTR_DEAD_1)

#define	SMAP_R_EMAC3_ADDR_HI		(SMAP_EMAC3_REGBASE + 0x1C)
#define	SMAP_R_EMAC3_ADDR_LO		(SMAP_EMAC3_REGBASE + 0x20)

#define	SMAP_R_EMAC3_VLAN_TPID		(SMAP_EMAC3_REGBASE + 0x24)
#define	  SMAP_E3_VLAN_ID_MSK		0xFFFF

#define	SMAP_R_EMAC3_VLAN_TCI		(SMAP_EMAC3_REGBASE + 0x28)
#define	  SMAP_E3_VLAN_TCITAG_MSK	0xFFFF

#define	SMAP_R_EMAC3_PAUSE_TIMER	(SMAP_EMAC3_REGBASE + 0x2C)
#define	  SMAP_E3_PTIMER_MSK		0xFFFF

#define	SMAP_R_EMAC3_INDIVID_HASH1	(SMAP_EMAC3_REGBASE + 0x30)
#define	SMAP_R_EMAC3_INDIVID_HASH2	(SMAP_EMAC3_REGBASE + 0x34)
#define	SMAP_R_EMAC3_INDIVID_HASH3	(SMAP_EMAC3_REGBASE + 0x38)
#define	SMAP_R_EMAC3_INDIVID_HASH4	(SMAP_EMAC3_REGBASE + 0x3C)
#define	SMAP_R_EMAC3_GROUP_HASH1	(SMAP_EMAC3_REGBASE + 0x40)
#define	SMAP_R_EMAC3_GROUP_HASH2	(SMAP_EMAC3_REGBASE + 0x44)
#define	SMAP_R_EMAC3_GROUP_HASH3	(SMAP_EMAC3_REGBASE + 0x48)
#define	SMAP_R_EMAC3_GROUP_HASH4	(SMAP_EMAC3_REGBASE + 0x4C)
#define	  SMAP_E3_HASH_MSK		0xFFFF

#define	SMAP_R_EMAC3_LAST_SA_HI		(SMAP_EMAC3_REGBASE + 0x50)
#define	SMAP_R_EMAC3_LAST_SA_LO		(SMAP_EMAC3_REGBASE + 0x54)

#define	SMAP_R_EMAC3_INTER_FRAME_GAP	(SMAP_EMAC3_REGBASE + 0x58)
#define	  SMAP_E3_IFGAP_MSK		0x3F

#define	SMAP_R_EMAC3_STA_CTRL_L		(SMAP_EMAC3_REGBASE + 0x5C)
#define	SMAP_R_EMAC3_STA_CTRL_H		(SMAP_EMAC3_REGBASE + 0x5E)
#define	  SMAP_E3_PHY_DATA_MSK		(0xFFFF)
#define	  SMAP_E3_PHY_DATA_BITSFT	(16)
#define	  SMAP_E3_PHY_OP_COMP		(1<<15)	/* operation complete */
#define	  SMAP_E3_PHY_ERR_READ		(1<<14)
#define	  SMAP_E3_PHY_STA_CMD_BITSFT	(12)
#define	    SMAP_E3_PHY_READ		(1<<12)
#define	    SMAP_E3_PHY_WRITE		(2<<12)
#define	  SMAP_E3_PHY_OPBCLCK_BITSFT	(10)
#define	    SMAP_E3_PHY_50M		(0<<10)
#define	    SMAP_E3_PHY_66M		(1<<10)
#define	    SMAP_E3_PHY_83M		(2<<10)
#define	    SMAP_E3_PHY_100M		(3<<10)
#define	  SMAP_E3_PHY_ADDR_MSK		(0x1F)
#define	  SMAP_E3_PHY_ADDR_BITSFT	(5)
#define	  SMAP_E3_PHY_REG_ADDR_MSK	(0x1F)

#define	SMAP_R_EMAC3_TX_THRESHOLD	(SMAP_EMAC3_REGBASE + 0x60)
#define	  SMAP_E3_TX_THRESHLD_MSK	(0x1F)
#define	  SMAP_E3_TX_THRESHLD_BITSFT	(27)

#define	SMAP_R_EMAC3_RX_WATERMARK	(SMAP_EMAC3_REGBASE + 0x64)
#define	  SMAP_E3_RX_LO_WATER_MSK	(0x1FF)
#define	  SMAP_E3_RX_LO_WATER_BITSFT	(23)
#define	  SMAP_E3_RX_HI_WATER_MSK	(0x1FF)
#define	  SMAP_E3_RX_HI_WATER_BITSFT	(7)

#define	SMAP_R_EMAC3_TX_OCTETS		(SMAP_EMAC3_REGBASE + 0x68)
#define	SMAP_R_EMAC3_RX_OCTETS		(SMAP_EMAC3_REGBASE + 0x6C)

/* Buffer descriptors.  */

typedef struct _smap_bd {
	u16	ctrl_stat;
	u16	reserved;	/* must be zero */
	u16	length;		/* number of bytes in pkt */
	u16	pointer;
} smap_bd_t;

#define	SMAP_BD_REGBASE			(SMAP_REGBASE + 0x2f00)
#define	SMAP_BD_TX_BASE			(SMAP_BD_REGBASE + 0x0000)
#define SMAP_TX_BASE			(SMAP_REGBASE + 0x1000)
#define SMAP_TX_BUFSIZE			4096
#define	SMAP_BD_RX_BASE			(SMAP_BD_REGBASE + 0x0200)
#define	  SMAP_BD_SIZE		512
#define	  SMAP_BD_MAX_ENTRY	64

/* TX Control */
#define	SMAP_BD_TX_READY	(1<<15)	/* set:driver, clear:HW */
#define	SMAP_BD_TX_GENFCS	(1<<9)	/* generate FCS */
#define	SMAP_BD_TX_GENPAD	(1<<8)	/* generate padding */
#define	SMAP_BD_TX_INSSA	(1<<7)	/* insert source address */
#define	SMAP_BD_TX_RPLSA	(1<<6)	/* replace source address */
#define	SMAP_BD_TX_INSVLAN	(1<<5)	/* insert VLAN Tag */
#define	SMAP_BD_TX_RPLVLAN	(1<<4)	/* replace VLAN Tag */

/* TX Status */
#define	SMAP_BD_TX_READY	(1<<15) /* set:driver, clear:HW */
#define	SMAP_BD_TX_BADFCS	(1<<9)	/* bad FCS */
#define	SMAP_BD_TX_BADPKT	(1<<8)	/* bad previous pkt in dependent mode */
#define	SMAP_BD_TX_LOSSCR	(1<<7)	/* loss of carrior sense */
#define	SMAP_BD_TX_EDEFER	(1<<6)	/* excessive deferal */
#define	SMAP_BD_TX_ECOLL	(1<<5)	/* excessive collision */
#define	SMAP_BD_TX_LCOLL	(1<<4)	/* late collision */
#define	SMAP_BD_TX_MCOLL	(1<<3)	/* multiple collision */
#define	SMAP_BD_TX_SCOLL	(1<<2)	/* single collision */
#define	SMAP_BD_TX_UNDERRUN	(1<<1)	/* underrun */
#define	SMAP_BD_TX_SQE		(1<<0)	/* SQE */

#define SMAP_BD_TX_ERROR (SMAP_BD_TX_LOSSCR|SMAP_BD_TX_EDEFER|SMAP_BD_TX_ECOLL|	\
		SMAP_BD_TX_LCOLL|SMAP_BD_TX_UNDERRUN)

/* RX Control */
#define	SMAP_BD_RX_EMPTY	(1<<15)	/* set:driver, clear:HW */

/* RX Status */
#define	SMAP_BD_RX_EMPTY	(1<<15)	/* set:driver, clear:HW */
#define	SMAP_BD_RX_OVERRUN	(1<<9)	/* overrun */
#define	SMAP_BD_RX_PFRM		(1<<8)	/* pause frame */
#define	SMAP_BD_RX_BADFRM	(1<<7)	/* bad frame */
#define	SMAP_BD_RX_RUNTFRM	(1<<6)	/* runt frame */
#define	SMAP_BD_RX_SHORTEVNT	(1<<5)	/* short event */
#define	SMAP_BD_RX_ALIGNERR	(1<<4)	/* alignment error */
#define	SMAP_BD_RX_BADFCS	(1<<3)	/* bad FCS */
#define	SMAP_BD_RX_FRMTOOLONG	(1<<2)	/* frame too long */
#define	SMAP_BD_RX_OUTRANGE	(1<<1)	/* out of range error */
#define	SMAP_BD_RX_INRANGE	(1<<0)	/* in range error */

#define SMAP_BD_RX_ERROR (SMAP_BD_RX_OVERRUN|SMAP_BD_RX_RUNTFRM|SMAP_BD_RX_SHORTEVNT|	\
		SMAP_BD_RX_ALIGNERR|SMAP_BD_RX_BADFCS|SMAP_BD_RX_FRMTOOLONG|		\
		SMAP_BD_RX_OUTRANGE|SMAP_BD_RX_INRANGE)

/* PHY registers (National Semiconductor DP83846A).  */

#define	SMAP_NS_OUI			0x080017
#define	SMAP_DsPHYTER_ADDRESS		0x1

#define	SMAP_DsPHYTER_BMCR		0x00
#define	  SMAP_PHY_BMCR_RST	(1<<15)		/* ReSeT */
#define	  SMAP_PHY_BMCR_LPBK	(1<<14)		/* LooPBacK */
#define	  SMAP_PHY_BMCR_100M	(1<<13)		/* speed select, 1:100M, 0:10M */
#define	  SMAP_PHY_BMCR_10M	(0<<13)		/* speed select, 1:100M, 0:10M */
#define	  SMAP_PHY_BMCR_ANEN	(1<<12)		/* Auto-Negotiation ENable */
#define	  SMAP_PHY_BMCR_PWDN	(1<<11)		/* PoWer DowN */
#define	  SMAP_PHY_BMCR_ISOL	(1<<10)		/* ISOLate */
#define	  SMAP_PHY_BMCR_RSAN	(1<<9)		/* ReStart Auto-Negotiation */
#define	  SMAP_PHY_BMCR_DUPM	(1<<8)		/* DUPlex Mode, 1:FDX, 0:HDX */
#define	  SMAP_PHY_BMCR_COLT	(1<<7)		/* COLlision Test */

#define	SMAP_DsPHYTER_BMSR		0x01
#define	  SMAP_PHY_BMSR_ANCP	(1<<5)		/* Auto-Negotiation ComPlete */
#define	  SMAP_PHY_BMSR_LINK	(1<<2)		/* LINK status */

#define	SMAP_DsPHYTER_PHYIDR1		0x02
#define	  SMAP_PHY_IDR1_VAL	(((SMAP_NS_OUI<<2)>>8)&0xffff)

#define	SMAP_DsPHYTER_PHYIDR2		0x03
#define	  SMAP_PHY_IDR2_VMDL	0x2		/* Vendor MoDeL number */
#define	  SMAP_PHY_IDR2_VAL	\
		(((SMAP_NS_OUI<<10)&0xFC00)|((SMAP_PHY_IDR2_VMDL<<4)&0x3F0))
#define	  SMAP_PHY_IDR2_MSK	0xFFF0
#define	  SMAP_PHY_IDR2_REV_MSK	0x000F

#define	SMAP_DsPHYTER_ANAR		0x04
#define	SMAP_DsPHYTER_ANLPAR		0x05
#define	SMAP_DsPHYTER_ANLPARNP		0x05
#define	SMAP_DsPHYTER_ANER		0x06
#define	SMAP_DsPHYTER_ANNPTR		0x07

/* Extended registers.  */
#define	SMAP_DsPHYTER_PHYSTS		0x10
#define	  SMAP_PHY_STS_REL	(1<<13)		/* Receive Error Latch */
#define	  SMAP_PHY_STS_POST	(1<<12)		/* POlarity STatus */
#define	  SMAP_PHY_STS_FCSL	(1<<11)		/* False Carrier Sense Latch */
#define	  SMAP_PHY_STS_SD	(1<<10)		/* 100BT unconditional Signal Detect */
#define	  SMAP_PHY_STS_DSL	(1<<9)		/* 100BT DeScrambler Lock */
#define	  SMAP_PHY_STS_PRCV	(1<<8)		/* Page ReCeiVed */
#define	  SMAP_PHY_STS_RFLT	(1<<6)		/* Remote FauLT */
#define	  SMAP_PHY_STS_JBDT	(1<<5)		/* JaBber DetecT */
#define	  SMAP_PHY_STS_ANCP	(1<<4)		/* Auto-Negotiation ComPlete */
#define	  SMAP_PHY_STS_LPBK	(1<<3)		/* LooPBacK status */
#define	  SMAP_PHY_STS_DUPS	(1<<2)		/* DUPlex Status,1:FDX,0:HDX */
#define	  SMAP_PHY_STS_FDX	(1<<2)		/* Full Duplex */
#define	  SMAP_PHY_STS_HDX	(0<<2)		/* Half Duplex */
#define	  SMAP_PHY_STS_SPDS	(1<<1)		/* SPeeD Status */
#define	  SMAP_PHY_STS_10M	(1<<1)		/* 10Mbps */
#define	  SMAP_PHY_STS_100M	(0<<1)		/* 100Mbps */
#define	  SMAP_PHY_STS_LINK	(1<<0)		/* LINK status */
#define	SMAP_DsPHYTER_FCSCR		0x14
#define	SMAP_DsPHYTER_RECR		0x15
#define	SMAP_DsPHYTER_PCSR		0x16
#define	SMAP_DsPHYTER_PHYCTRL		0x19
#define	SMAP_DsPHYTER_10BTSCR		0x1A
#define	SMAP_DsPHYTER_CDCTRL		0x1B


/*
 * ATA hardware types and definitions.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * * code included from the ps2drv iop driver, modified by linuzappz  *
 */


#define ATA_DEV9_HDD_BASE		(SPD_REGBASE + 0x40)
/* AIF on T10Ks - Not supported yet.  */
#define ATA_AIF_HDD_BASE		(SPD_REGBASE + 0x4000000 + 0x60)

/* A port contains all of the ATA controller registers.  */
typedef struct _ata_hwport {
	u16	r_data;		/* 00 */
	u16	r_error;	/* 02 */
#define r_feature r_error
	u16	r_nsector;	/* 04 */
	u16	r_sector;	/* 06 */
	u16	r_lcyl;		/* 08 */
	u16	r_hcyl;		/* 0a */
	u16	r_select;	/* 0c */
	u16	r_status;	/* 0e */
#define r_command r_status
	u16	pad[6];
	u16	r_control;	/* 1c */
} ata_hwport_t;

ata_hwport_t *ata_hwport;

#define ATA_R_DATA			(ATA_DEV9_HDD_BASE + 0x00)
#define ATA_R_ERROR			(ATA_DEV9_HDD_BASE + 0x02)
#define ATA_R_NSECTOR		(ATA_DEV9_HDD_BASE + 0x04)
#define ATA_R_SECTOR		(ATA_DEV9_HDD_BASE + 0x06)
#define ATA_R_LCYL			(ATA_DEV9_HDD_BASE + 0x08)
#define ATA_R_HCYL			(ATA_DEV9_HDD_BASE + 0x0a)
#define ATA_R_SELECT		(ATA_DEV9_HDD_BASE + 0x0c)
#define ATA_R_STATUS		(ATA_DEV9_HDD_BASE + 0x0e)
#define ATA_R_CONTROL		(ATA_DEV9_HDD_BASE + 0x1c)

/* r_error bits.  */
#define ATA_ERR_MARK		0x01
#define ATA_ERR_TRACK0		0x02
#define ATA_ERR_ABORT		0x04
#define ATA_ERR_MCR		0x08
#define ATA_ERR_ID		0x10
#define ATA_ERR_MC		0x20
#define ATA_ERR_ECC		0x40
#define ATA_ERR_ICRC		0x80

/* r_status bits.  */
#define ATA_STAT_ERR		0x01
#define ATA_STAT_INDEX		0x02
#define ATA_STAT_ECC		0x04
#define	ATA_STAT_DRQ		0x08
#define ATA_STAT_SEEK		0x10
#define ATA_STAT_WRERR		0x20
#define ATA_STAT_READY		0x40
#define ATA_STAT_BUSY		0x80




/* ATA/ATAPI Commands pre T13 Spec */
#define WIN_NOP				0x00
/*
 *	0x01->0x02 Reserved
 */
#define CFA_REQ_EXT_ERROR_CODE		0x03 /* CFA Request Extended Error Code */
/*
 *	0x04->0x07 Reserved
 */
#define WIN_SRST			0x08 /* ATAPI soft reset command */
#define WIN_DEVICE_RESET		0x08
/*
 *	0x09->0x0F Reserved
 */
#define WIN_RECAL			0x10
#define WIN_RESTORE			WIN_RECAL
/*
 *	0x10->0x1F Reserved
 */
#define WIN_READ			0x20 /* 28-Bit */
#define WIN_READ_ONCE			0x21 /* 28-Bit without retries */
#define WIN_READ_LONG			0x22 /* 28-Bit */
#define WIN_READ_LONG_ONCE		0x23 /* 28-Bit without retries */
#define WIN_READ_EXT			0x24 /* 48-Bit */
#define WIN_READDMA_EXT			0x25 /* 48-Bit */
#define WIN_READDMA_QUEUED_EXT		0x26 /* 48-Bit */
#define WIN_READ_NATIVE_MAX_EXT		0x27 /* 48-Bit */
/*
 *	0x28
 */
#define WIN_MULTREAD_EXT		0x29 /* 48-Bit */
/*
 *	0x2A->0x2F Reserved
 */
#define WIN_WRITE			0x30 /* 28-Bit */
#define WIN_WRITE_ONCE			0x31 /* 28-Bit without retries */
#define WIN_WRITE_LONG			0x32 /* 28-Bit */
#define WIN_WRITE_LONG_ONCE		0x33 /* 28-Bit without retries */
#define WIN_WRITE_EXT			0x34 /* 48-Bit */
#define WIN_WRITEDMA_EXT		0x35 /* 48-Bit */
#define WIN_WRITEDMA_QUEUED_EXT		0x36 /* 48-Bit */
#define WIN_SET_MAX_EXT			0x37 /* 48-Bit */
#define CFA_WRITE_SECT_WO_ERASE		0x38 /* CFA Write Sectors without erase */
#define WIN_MULTWRITE_EXT		0x39 /* 48-Bit */
/*
 *	0x3A->0x3B Reserved
 */
#define WIN_WRITE_VERIFY		0x3C /* 28-Bit */
/*
 *	0x3D->0x3F Reserved
 */
#define WIN_VERIFY			0x40 /* 28-Bit - Read Verify Sectors */
#define WIN_VERIFY_ONCE			0x41 /* 28-Bit - without retries */
#define WIN_VERIFY_EXT			0x42 /* 48-Bit */
/*
 *	0x43->0x4F Reserved
 */
#define WIN_FORMAT			0x50
/*
 *	0x51->0x5F Reserved
 */
#define WIN_INIT			0x60
/*
 *	0x61->0x5F Reserved
 */
#define WIN_SEEK			0x70 /* 0x70-0x7F Reserved */
#define CFA_TRANSLATE_SECTOR		0x87 /* CFA Translate Sector */
#define WIN_DIAGNOSE			0x90
#define WIN_SPECIFY			0x91 /* set drive geometry translation */
#define WIN_DOWNLOAD_MICROCODE		0x92
#define WIN_STANDBYNOW2			0x94
#define WIN_STANDBY2			0x96
#define WIN_SETIDLE2			0x97
#define WIN_CHECKPOWERMODE2		0x98
#define WIN_SLEEPNOW2			0x99
/*
 *	0x9A VENDOR
 */
#define WIN_PACKETCMD			0xA0 /* Send a packet command. */
#define WIN_PIDENTIFY			0xA1 /* identify ATAPI device	*/
#define WIN_QUEUED_SERVICE		0xA2
#define WIN_SMART			0xB0 /* self-monitoring and reporting */
#define CFA_ERASE_SECTORS       	0xC0
#define WIN_MULTREAD			0xC4 /* read sectors using multiple mode*/
#define WIN_MULTWRITE			0xC5 /* write sectors using multiple mode */
#define WIN_SETMULT			0xC6 /* enable/disable multiple mode */
#define WIN_READDMA_QUEUED		0xC7 /* read sectors using Queued DMA transfers */
#define WIN_READDMA			0xC8 /* read sectors using DMA transfers */
#define WIN_READDMA_ONCE		0xC9 /* 28-Bit - without retries */
#define WIN_WRITEDMA			0xCA /* write sectors using DMA transfers */
#define WIN_WRITEDMA_ONCE		0xCB /* 28-Bit - without retries */
#define WIN_WRITEDMA_QUEUED		0xCC /* write sectors using Queued DMA transfers */
#define CFA_WRITE_MULTI_WO_ERASE	0xCD /* CFA Write multiple without erase */
#define WIN_GETMEDIASTATUS		0xDA	
#define WIN_ACKMEDIACHANGE		0xDB /* ATA-1, ATA-2 vendor */
#define WIN_POSTBOOT			0xDC
#define WIN_PREBOOT			0xDD
#define WIN_DOORLOCK			0xDE /* lock door on removable drives */
#define WIN_DOORUNLOCK			0xDF /* unlock door on removable drives */
#define WIN_STANDBYNOW1			0xE0
#define WIN_IDLEIMMEDIATE		0xE1 /* force drive to become "ready" */
#define WIN_STANDBY             	0xE2 /* Set device in Standby Mode */
#define WIN_SETIDLE1			0xE3
#define WIN_READ_BUFFER			0xE4 /* force read only 1 sector */
#define WIN_CHECKPOWERMODE1		0xE5
#define WIN_SLEEPNOW1			0xE6
#define WIN_FLUSH_CACHE			0xE7
#define WIN_WRITE_BUFFER		0xE8 /* force write only 1 sector */
#define WIN_WRITE_SAME			0xE9 /* read ata-2 to use */
	/* SET_FEATURES 0x22 or 0xDD */
#define WIN_FLUSH_CACHE_EXT		0xEA /* 48-Bit */
#define WIN_IDENTIFY			0xEC /* ask drive to identify itself	*/
#define WIN_MEDIAEJECT			0xED
#define WIN_IDENTIFY_DMA		0xEE /* same as WIN_IDENTIFY, but DMA */
#define WIN_SETFEATURES			0xEF /* set special drive features */
#define EXABYTE_ENABLE_NEST		0xF0
#define WIN_SECURITY_SET_PASS		0xF1
#define WIN_SECURITY_UNLOCK		0xF2
#define WIN_SECURITY_ERASE_PREPARE	0xF3
#define WIN_SECURITY_ERASE_UNIT		0xF4
#define WIN_SECURITY_FREEZE_LOCK	0xF5
#define WIN_SECURITY_DISABLE		0xF6
#define WIN_READ_NATIVE_MAX		0xF8 /* return the native maximum address */
#define WIN_SET_MAX			0xF9
#define DISABLE_SEAGATE			0xFB

/* WIN_SMART sub-commands */

#define SMART_READ_VALUES		0xD0
#define SMART_READ_THRESHOLDS		0xD1
#define SMART_AUTOSAVE			0xD2
#define SMART_SAVE			0xD3
#define SMART_IMMEDIATE_OFFLINE		0xD4
#define SMART_READ_LOG_SECTOR		0xD5
#define SMART_WRITE_LOG_SECTOR		0xD6
#define SMART_WRITE_THRESHOLDS		0xD7
#define SMART_ENABLE			0xD8
#define SMART_DISABLE			0xD9
#define SMART_STATUS			0xDA
#define SMART_AUTO_OFFLINE		0xDB

/* Password used in TF4 & TF5 executing SMART commands */

#define SMART_LCYL_PASS			0x4F
#define SMART_HCYL_PASS			0xC2
		
/* WIN_SETFEATURES sub-commands */
#define SETFEATURES_EN_8BIT	0x01	/* Enable 8-Bit Transfers */
#define SETFEATURES_EN_WCACHE	0x02	/* Enable write cache */
#define SETFEATURES_XFER	0x03	/* Set transfer mode */
#	define XFER_UDMA_7	0x47	/* 0100|0111 */
#	define XFER_UDMA_6	0x46	/* 0100|0110 */
#	define XFER_UDMA_5	0x45	/* 0100|0101 */
#	define XFER_UDMA_4	0x44	/* 0100|0100 */
#	define XFER_UDMA_3	0x43	/* 0100|0011 */
#	define XFER_UDMA_2	0x42	/* 0100|0010 */
#	define XFER_UDMA_1	0x41	/* 0100|0001 */
#	define XFER_UDMA_0	0x40	/* 0100|0000 */
#	define XFER_MW_DMA_2	0x22	/* 0010|0010 */
#	define XFER_MW_DMA_1	0x21	/* 0010|0001 */
#	define XFER_MW_DMA_0	0x20	/* 0010|0000 */
#	define XFER_SW_DMA_2	0x12	/* 0001|0010 */
#	define XFER_SW_DMA_1	0x11	/* 0001|0001 */
#	define XFER_SW_DMA_0	0x10	/* 0001|0000 */
#	define XFER_PIO_4	0x0C	/* 0000|1100 */
#	define XFER_PIO_3	0x0B	/* 0000|1011 */
#	define XFER_PIO_2	0x0A	/* 0000|1010 */
#	define XFER_PIO_1	0x09	/* 0000|1001 */
#	define XFER_PIO_0	0x08	/* 0000|1000 */
#	define XFER_PIO_SLOW	0x00	/* 0000|0000 */
#define SETFEATURES_DIS_DEFECT	0x04	/* Disable Defect Management */
#define SETFEATURES_EN_APM	0x05	/* Enable advanced power management */
#define SETFEATURES_EN_SAME_R	0x22	/* for a region ATA-1 */
#define SETFEATURES_DIS_MSN	0x31	/* Disable Media Status Notification */
#define SETFEATURES_DIS_RETRY	0x33	/* Disable Retry */
#define SETFEATURES_EN_AAM	0x42	/* Enable Automatic Acoustic Management */
#define SETFEATURES_RW_LONG	0x44	/* Set Lenght of VS bytes */
#define SETFEATURES_SET_CACHE	0x54	/* Set Cache segments to SC Reg. Val */
#define SETFEATURES_DIS_RLA	0x55	/* Disable read look-ahead feature */
#define SETFEATURES_EN_RI	0x5D	/* Enable release interrupt */
#define SETFEATURES_EN_SI	0x5E	/* Enable SERVICE interrupt */
#define SETFEATURES_DIS_RPOD	0x66	/* Disable reverting to power on defaults */
#define SETFEATURES_DIS_ECC	0x77	/* Disable ECC byte count */
#define SETFEATURES_DIS_8BIT	0x81	/* Disable 8-Bit Transfers */
#define SETFEATURES_DIS_WCACHE	0x82	/* Disable write cache */
#define SETFEATURES_EN_DEFECT	0x84	/* Enable Defect Management */
#define SETFEATURES_DIS_APM	0x85	/* Disable advanced power management */
#define SETFEATURES_EN_ECC	0x88	/* Enable ECC byte count */
#define SETFEATURES_EN_MSN	0x95	/* Enable Media Status Notification */
#define SETFEATURES_EN_RETRY	0x99	/* Enable Retry */
#define SETFEATURES_EN_RLA	0xAA	/* Enable read look-ahead feature */
#define SETFEATURES_PREFETCH	0xAB	/* Sets drive prefetch value */
#define SETFEATURES_EN_REST	0xAC	/* ATA-1 */
#define SETFEATURES_4B_RW_LONG	0xBB	/* Set Lenght of 4 bytes */
#define SETFEATURES_DIS_AAM	0xC2	/* Disable Automatic Acoustic Management */
#define SETFEATURES_EN_RPOD	0xCC	/* Enable reverting to power on defaults */
#define SETFEATURES_DIS_RI	0xDD	/* Disable release interrupt ATAPI */
#define SETFEATURES_EN_SAME_M	0xDD	/* for a entire device ATA-1 */
#define SETFEATURES_DIS_SI	0xDE	/* Disable SERVICE interrupt ATAPI */

/* WIN_SECURITY sub-commands */

#define SECURITY_SET_PASSWORD		0xBA
#define SECURITY_UNLOCK			0xBB
#define SECURITY_ERASE_PREPARE		0xBC
#define SECURITY_ERASE_UNIT		0xBD
#define SECURITY_FREEZE_LOCK		0xBE
#define SECURITY_DISABLE_PASSWORD	0xBF

struct hd_geometry {
      unsigned char heads;
      unsigned char sectors;
      unsigned short cylinders;
      unsigned long start;
};

/* BIG GEOMETRY */
struct hd_big_geometry {
	unsigned char heads;
	unsigned char sectors;
	unsigned int cylinders;
	unsigned long start;
};

/* hd/ide ctl's that pass (arg) ptrs to user space are numbered 0x030n/0x031n */
#define HDIO_GETGEO		0x0301	/* get device geometry */
#define HDIO_GET_UNMASKINTR	0x0302	/* get current unmask setting */
#define HDIO_GET_MULTCOUNT	0x0304	/* get current IDE blockmode setting */
#define HDIO_GET_QDMA		0x0305	/* get use-qdma flag */

#define HDIO_SET_XFER		0x0306	/* set transfer rate via proc */

#define HDIO_OBSOLETE_IDENTITY	0x0307	/* OBSOLETE, DO NOT USE: returns 142 bytes */
#define HDIO_GET_KEEPSETTINGS	0x0308	/* get keep-settings-on-reset flag */
#define HDIO_GET_32BIT		0x0309	/* get current io_32bit setting */
#define HDIO_GET_NOWERR		0x030a	/* get ignore-write-error flag */
#define HDIO_GET_DMA		0x030b	/* get use-dma flag */
#define HDIO_GET_NICE		0x030c	/* get nice flags */
#define HDIO_GET_IDENTITY	0x030d	/* get IDE identification info */
#define HDIO_GET_WCACHE		0x030e	/* get write cache mode on|off */
#define HDIO_GET_ACOUSTIC	0x030f	/* get acoustic value */
#define	HDIO_GET_ADDRESS	0x0310	/* */

#define HDIO_GET_BUSSTATE	0x031a	/* get the bus state of the hwif */
#define HDIO_TRISTATE_HWIF	0x031b	/* execute a channel tristate */
#define HDIO_DRIVE_RESET	0x031c	/* execute a device reset */
#define HDIO_DRIVE_TASKFILE	0x031d	/* execute raw taskfile */
#define HDIO_DRIVE_TASK		0x031e	/* execute task and special drive command */
#define HDIO_DRIVE_CMD		0x031f	/* execute a special drive command */

#define HDIO_DRIVE_CMD_AEB	HDIO_DRIVE_TASK

/* hd/ide ctl's that pass (arg) non-ptr values are numbered 0x032n/0x033n */
#define HDIO_SET_MULTCOUNT	0x0321	/* change IDE blockmode */
#define HDIO_SET_UNMASKINTR	0x0322	/* permit other irqs during I/O */
#define HDIO_SET_KEEPSETTINGS	0x0323	/* keep ioctl settings on reset */
#define HDIO_SET_32BIT		0x0324	/* change io_32bit flags */
#define HDIO_SET_NOWERR		0x0325	/* change ignore-write-error flag */
#define HDIO_SET_DMA		0x0326	/* change use-dma flag */
#define HDIO_SET_PIO_MODE	0x0327	/* reconfig interface to new speed */
#define HDIO_SCAN_HWIF		0x0328	/* register and (re)scan interface */
#define HDIO_SET_NICE		0x0329	/* set nice flags */
#define HDIO_UNREGISTER_HWIF	0x032a  /* unregister interface */
#define HDIO_SET_WCACHE		0x032b	/* change write cache enable-disable */
#define HDIO_SET_ACOUSTIC	0x032c	/* change acoustic behavior */
#define HDIO_SET_BUSSTATE	0x032d	/* set the bus state of the hwif */
#define HDIO_SET_QDMA		0x032e	/* change use-qdma flag */
#define HDIO_SET_ADDRESS	0x032f	/* change lba addressing modes */

/* bus states */
enum {
	BUSSTATE_OFF = 0,
	BUSSTATE_ON,
	BUSSTATE_TRISTATE
};

/* hd/ide ctl's that pass (arg) ptrs to user space are numbered 0x033n/0x033n */
#define HDIO_GETGEO_BIG		0x0330	/* */
#define HDIO_GETGEO_BIG_RAW	0x0331	/* */

#define HDIO_SET_IDE_SCSI	0x0338
#define HDIO_SET_SCSI_IDE	0x0339

#define __NEW_HD_DRIVE_ID
/* structure returned by HDIO_GET_IDENTITY,
 * as per ANSI NCITS ATA6 rev.1b spec
 */
struct hd_driveid {
	unsigned short	config;		/* lots of obsolete bit flags */
	unsigned short	cyls;		/* Obsolete, "physical" cyls */
	unsigned short	reserved2;	/* reserved (word 2) */
	unsigned short	heads;		/* Obsolete, "physical" heads */
	unsigned short	track_bytes;	/* unformatted bytes per track */
	unsigned short	sector_bytes;	/* unformatted bytes per sector */
	unsigned short	sectors;	/* Obsolete, "physical" sectors per track */
	unsigned short	vendor0;	/* vendor unique */
	unsigned short	vendor1;	/* vendor unique */
	unsigned short	vendor2;	/* Retired vendor unique */
	unsigned char	serial_no[20];	/* 0 = not_specified */
	unsigned short	buf_type;	/* Retired */
	unsigned short	buf_size;	/* Retired, 512 byte increments
					 * 0 = not_specified
					 */
	unsigned short	ecc_bytes;	/* for r/w long cmds; 0 = not_specified */
	unsigned char	fw_rev[8];	/* 0 = not_specified */
	unsigned char	model[40];	/* 0 = not_specified */
	unsigned char	max_multsect;	/* 0=not_implemented */
	unsigned char	vendor3;	/* vendor unique */
	unsigned short	dword_io;	/* 0=not_implemented; 1=implemented */
	unsigned char	vendor4;	/* vendor unique */
	unsigned char	capability;	/* (upper byte of word 49)
					 *  3:	IORDYsup
					 *  2:	IORDYsw
					 *  1:	LBA
					 *  0:	DMA
					 */
	unsigned short	reserved50;	/* reserved (word 50) */
	unsigned char	vendor5;	/* Obsolete, vendor unique */
	unsigned char	tPIO;		/* Obsolete, 0=slow, 1=medium, 2=fast */
	unsigned char	vendor6;	/* Obsolete, vendor unique */
	unsigned char	tDMA;		/* Obsolete, 0=slow, 1=medium, 2=fast */
	unsigned short	field_valid;	/* (word 53)
					 *  2:	ultra_ok	word  88
					 *  1:	eide_ok		words 64-70
					 *  0:	cur_ok		words 54-58
					 */
	unsigned short	cur_cyls;	/* Obsolete, logical cylinders */
	unsigned short	cur_heads;	/* Obsolete, l heads */
	unsigned short	cur_sectors;	/* Obsolete, l sectors per track */
	unsigned short	cur_capacity0;	/* Obsolete, l total sectors on drive */
	unsigned short	cur_capacity1;	/* Obsolete, (2 words, misaligned int)     */
	unsigned char	multsect;	/* current multiple sector count */
	unsigned char	multsect_valid;	/* when (bit0==1) multsect is ok */
	unsigned int	lba_capacity;	/* Obsolete, total number of sectors */
	unsigned short	dma_1word;	/* Obsolete, single-word dma info */
	unsigned short	dma_mword;	/* multiple-word dma info */
	unsigned short  eide_pio_modes; /* bits 0:mode3 1:mode4 */
	unsigned short  eide_dma_min;	/* min mword dma cycle time (ns) */
	unsigned short  eide_dma_time;	/* recommended mword dma cycle time (ns) */
	unsigned short  eide_pio;       /* min cycle time (ns), no IORDY  */
	unsigned short  eide_pio_iordy; /* min cycle time (ns), with IORDY */
	unsigned short	words69_70[2];	/* reserved words 69-70
					 * future command overlap and queuing
					 */
	/* HDIO_GET_IDENTITY currently returns only words 0 through 70 */
	unsigned short	words71_74[4];	/* reserved words 71-74
					 * for IDENTIFY PACKET DEVICE command
					 */
	unsigned short  queue_depth;	/* (word 75)
					 * 15:5	reserved
					 *  4:0	Maximum queue depth -1
					 */
	unsigned short  words76_79[4];	/* reserved words 76-79 */
	unsigned short  major_rev_num;	/* (word 80) */
	unsigned short  minor_rev_num;	/* (word 81) */
	unsigned short  command_set_1;	/* (word 82) supported
					 * 15:	Obsolete
					 * 14:	NOP command
					 * 13:	READ_BUFFER
					 * 12:	WRITE_BUFFER
					 * 11:	Obsolete
					 * 10:	Host Protected Area
					 *  9:	DEVICE Reset
					 *  8:	SERVICE Interrupt
					 *  7:	Release Interrupt
					 *  6:	look-ahead
					 *  5:	write cache
					 *  4:	PACKET Command
					 *  3:	Power Management Feature Set
					 *  2:	Removable Feature Set
					 *  1:	Security Feature Set
					 *  0:	SMART Feature Set
					 */
	unsigned short  command_set_2;	/* (word 83)
					 * 15:	Shall be ZERO
					 * 14:	Shall be ONE
					 * 13:	FLUSH CACHE EXT
					 * 12:	FLUSH CACHE
					 * 11:	Device Configuration Overlay
					 * 10:	48-bit Address Feature Set
					 *  9:	Automatic Acoustic Management
					 *  8:	SET MAX security
					 *  7:	reserved 1407DT PARTIES
					 *  6:	SetF sub-command Power-Up
					 *  5:	Power-Up in Standby Feature Set
					 *  4:	Removable Media Notification
					 *  3:	APM Feature Set
					 *  2:	CFA Feature Set
					 *  1:	READ/WRITE DMA QUEUED
					 *  0:	Download MicroCode
					 */
	unsigned short  cfsse;		/* (word 84)
					 * cmd set-feature supported extensions
					 * 15:	Shall be ZERO
					 * 14:	Shall be ONE
					 * 13:6	reserved
					 *  5:	General Purpose Logging
					 *  4:	Streaming Feature Set
					 *  3:	Media Card Pass Through
					 *  2:	Media Serial Number Valid
					 *  1:	SMART selt-test supported
					 *  0:	SMART error logging
					 */
	unsigned short  cfs_enable_1;	/* (word 85)
					 * command set-feature enabled
					 * 15:	Obsolete
					 * 14:	NOP command
					 * 13:	READ_BUFFER
					 * 12:	WRITE_BUFFER
					 * 11:	Obsolete
					 * 10:	Host Protected Area
					 *  9:	DEVICE Reset
					 *  8:	SERVICE Interrupt
					 *  7:	Release Interrupt
					 *  6:	look-ahead
					 *  5:	write cache
					 *  4:	PACKET Command
					 *  3:	Power Management Feature Set
					 *  2:	Removable Feature Set
					 *  1:	Security Feature Set
					 *  0:	SMART Feature Set
					 */
	unsigned short  cfs_enable_2;	/* (word 86)
					 * command set-feature enabled
					 * 15:	Shall be ZERO
					 * 14:	Shall be ONE
					 * 13:	FLUSH CACHE EXT
					 * 12:	FLUSH CACHE
					 * 11:	Device Configuration Overlay
					 * 10:	48-bit Address Feature Set
					 *  9:	Automatic Acoustic Management
					 *  8:	SET MAX security
					 *  7:	reserved 1407DT PARTIES
					 *  6:	SetF sub-command Power-Up
					 *  5:	Power-Up in Standby Feature Set
					 *  4:	Removable Media Notification
					 *  3:	APM Feature Set
					 *  2:	CFA Feature Set
					 *  1:	READ/WRITE DMA QUEUED
					 *  0:	Download MicroCode
					 */
	unsigned short  csf_default;	/* (word 87)
					 * command set-feature default
					 * 15:	Shall be ZERO
					 * 14:	Shall be ONE
					 * 13:6	reserved
					 *  5:	General Purpose Logging enabled
					 *  4:	Valid CONFIGURE STREAM executed
					 *  3:	Media Card Pass Through enabled
					 *  2:	Media Serial Number Valid
					 *  1:	SMART selt-test supported
					 *  0:	SMART error logging
					 */
	unsigned short  dma_ultra;	/* (word 88) */
	unsigned short	trseuc;		/* time required for security erase */
	unsigned short	trsEuc;		/* time required for enhanced erase */
	unsigned short	CurAPMvalues;	/* current APM values */
	unsigned short	mprc;		/* master password revision code */
	unsigned short	hw_config;	/* hardware config (word 93)
					 * 15:	Shall be ZERO
					 * 14:	Shall be ONE
					 * 13:
					 * 12:
					 * 11:
					 * 10:
					 *  9:
					 *  8:
					 *  7:
					 *  6:
					 *  5:
					 *  4:
					 *  3:
					 *  2:
					 *  1:
					 *  0:	Shall be ONE
					 */
	unsigned short	acoustic;	/* (word 94)
					 * 15:8	Vendor's recommended value
					 *  7:0	current value
					 */
	unsigned short	msrqs;		/* min stream request size */
	unsigned short	sxfert;		/* stream transfer time */
	unsigned short	sal;		/* stream access latency */
	unsigned int	spg;		/* stream performance granularity */
	u64             lba_capacity_2; /* 48-bit total number of sectors */
	unsigned short	words104_125[22];/* reserved words 104-125 */
	unsigned short	last_lun;	/* (word 126) */
	unsigned short	word127;	/* (word 127) Feature Set
					 * Removable Media Notification
					 * 15:2	reserved
					 *  1:0	00 = not supported
					 *	01 = supported
					 *	10 = reserved
					 *	11 = reserved
					 */
	unsigned short	dlf;		/* (word 128)
					 * device lock function
					 * 15:9	reserved
					 *  8	security level 1:max 0:high
					 *  7:6	reserved
					 *  5	enhanced erase
					 *  4	expire
					 *  3	frozen
					 *  2	locked
					 *  1	en/disabled
					 *  0	capability
					 */
	unsigned short  csfo;		/*  (word 129)
					 * current set features options
					 * 15:4	reserved
					 *  3:	auto reassign
					 *  2:	reverting
					 *  1:	read-look-ahead
					 *  0:	write cache
					 */
	unsigned short	words130_155[26];/* reserved vendor words 130-155 */
	unsigned short	word156;	/* reserved vendor word 156 */
	unsigned short	words157_159[3];/* reserved vendor words 157-159 */
	unsigned short	cfa_power;	/* (word 160) CFA Power Mode
					 * 15 word 160 supported
					 * 14 reserved
					 * 13
					 * 12
					 * 11:0
					 */
	unsigned short	words161_175[15];/* Reserved for CFA */
	unsigned short	words176_205[30];/* Current Media Serial Number */
	unsigned short	words206_254[49];/* reserved words 206-254 */
	unsigned short	integrity_word;	/* (word 255)
					 * 15:8 Checksum
					 *  7:0 Signature
					 */
};

/*
 * NAND Flash via Dev9 driver definitions
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * * code included from the ps2sdk iop driver *
 */

#define FLASH_ID_64MBIT		0xe6
#define FLASH_ID_128MBIT	0x73
#define FLASH_ID_256MBIT	0x75
#define FLASH_ID_512MBIT	0x76
#define FLASH_ID_1024MBIT	0x79

/* SmartMedia commands.  */
#define SM_CMD_READ1		0x00
#define SM_CMD_READ2		0x01
#define SM_CMD_READ3		0x50
#define SM_CMD_RESET		0xff
#define SM_CMD_WRITEDATA	0x80
#define SM_CMD_PROGRAMPAGE	0x10
#define SM_CMD_ERASEBLOCK	0x60
#define SM_CMD_ERASECONFIRM	0xd0
#define SM_CMD_GETSTATUS	0x70
#define SM_CMD_READID		0x90

typedef struct {
	u32	id;
	u32	mbits;
	u32	page_bytes;	/* bytes/page */
	u32	block_pages;	/* pages/block */
	u32	blocks;
} flash_info_t;

/*
static flash_info_t devices[] = {
	{ FLASH_ID_64MBIT, 64, 528, 16, 1024 },
	{ FLASH_ID_128MBIT, 128, 528, 32, 1024 },
	{ FLASH_ID_256MBIT, 256, 528, 32, 2048 },
	{ FLASH_ID_512MBIT, 512, 528, 32, 4096 },
	{ FLASH_ID_1024MBIT, 1024, 528, 32, 8192 }
};
#define NUM_DEVICES	(sizeof(devices)/sizeof(flash_info_t))
*/

// definitions added by Florin

#define FLASH_REGBASE			0x10004800

#define FLASH_R_DATA		(FLASH_REGBASE + 0x00)
#define FLASH_R_CMD			(FLASH_REGBASE + 0x04)
#define FLASH_R_ADDR		(FLASH_REGBASE + 0x08)
#define FLASH_R_CTRL		(FLASH_REGBASE + 0x0C)
#define   FLASH_PP_READY	(1<<0)	// r/w	/BUSY
#define	  FLASH_PP_WRITE	(1<<7)	// -/w	WRITE data
#define	  FLASH_PP_CSEL		(1<<8)	// -/w	CS
#define	  FLASH_PP_READ		(1<<11)	// -/w	READ data
#define	  FLASH_PP_NOECC	(1<<12)	// -/w	ECC disabled
//#define FLASH_R_10		(FLASH_REGBASE + 0x10)
#define FLASH_R_ID			(FLASH_REGBASE + 0x14)

#define FLASH_REGSIZE			0x20

void CALLBACK FLASHinit();
u32  CALLBACK FLASHread32(u32 addr, int size);
void CALLBACK FLASHwrite32(u32 addr, u32 value, int size);

#define DVR_REGBASE			0x10004000

#define DVR_R_STAT			(DVR_REGBASE + 0x200)
#define DVR_R_ACK			(DVR_REGBASE + 0x204)
#define DVR_R_MASK			(DVR_REGBASE + 0x208)
#define DVR_R_TASK			(DVR_REGBASE + 0x230)

#define DVR_REGSIZE			0x300

void CALLBACK DVRinit();
u32  CALLBACK DVRread32(u32 addr, int size);
void CALLBACK DVRwrite32(u32 addr, u32 value, int size);

#endif
