/***********************************************************************/
/*  This file is part of the uVision/ARM development tools             */
/*  Copyright KEIL ELEKTRONIK GmbH 2002-2006                           */
/***********************************************************************/
/*                                                                     */
/*  USBREG.H:  Header file for Philips LPC214X USB                     */
/*                                                                     */
/***********************************************************************/

#ifndef __USBREG_H
#define __USBREG_H


#define SCB_BASE_ADDR       0xE01FC000  /* System Control Block */

/* PLL48 Registers */
#define PLL48CON            (*(volatile unsigned int*)(SCB_BASE_ADDR + 0xA0))
#define PLL48CFG            (*(volatile unsigned int*)(SCB_BASE_ADDR + 0xA4))
#define PLL48STAT           (*(volatile unsigned int*)(SCB_BASE_ADDR + 0xA8))
#define PLL48FEED           (*(volatile unsigned int*)(SCB_BASE_ADDR + 0xAC))

/* PLL48 Bit Definitions */
#define PLLCON_PLLE         (1<<0)      /* PLL Enable */
#define PLLCON_PLLC         (1<<1)      /* PLL Connect */
#define PLLCFG_MSEL         (0x1F<<0)   /* PLL Multiplier */
#define PLLCFG_PSEL         (0x03<<5)   /* PLL Divider */
#define PLLSTAT_PLOCK       (1<<10)     /* PLL Lock Status */


#define USB_BASE_ADDR       0xE0090000  /* USB Base Address */

/* Device Interrupt Registers */
#define DEV_INT_STAT        (*(volatile unsigned int*)(USB_BASE_ADDR + 0x00))
#define DEV_INT_EN          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x04))
#define DEV_INT_CLR         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x08))
#define DEV_INT_SET         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x0C))
#define DEV_INT_PRIO        (*(volatile unsigned int*)(USB_BASE_ADDR + 0x2C))

/* Endpoint Interrupt Registers */
#define EP_INT_STAT         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x30))
#define EP_INT_EN           (*(volatile unsigned int*)(USB_BASE_ADDR + 0x34))
#define EP_INT_CLR          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x38))
#define EP_INT_SET          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x3C))
#define EP_INT_PRIO         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x40))

/* Endpoint Realization Registers */
#define REALIZE_EP          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x44))
#define EP_INDEX            (*(volatile unsigned int*)(USB_BASE_ADDR + 0x48))
#define MAXPACKET_SIZE      (*(volatile unsigned int*)(USB_BASE_ADDR + 0x4C))

/* Command Reagisters */
#define CMD_CODE            (*(volatile unsigned int*)(USB_BASE_ADDR + 0x10))
#define CMD_DATA            (*(volatile unsigned int*)(USB_BASE_ADDR + 0x14))

/* Data Transfer Registers */
#define RX_DATA             (*(volatile unsigned int*)(USB_BASE_ADDR + 0x18))
#define TX_DATA             (*(volatile unsigned int*)(USB_BASE_ADDR + 0x1C))
#define RX_PLENGTH          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x20))
#define TX_PLENGTH          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x24))
#define USB_CTRL            (*(volatile unsigned int*)(USB_BASE_ADDR + 0x28))

/* System Register */
#define DC_REVISION         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x7C))

/* DMA Registers */
#define DMA_REQ_STAT        (*(volatile unsigned int*)(USB_BASE_ADDR + 0x50))
#define DMA_REQ_CLR         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x54))
#define DMA_REQ_SET         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x58))
#define UDCA_HEAD           (*(volatile unsigned int*)(USB_BASE_ADDR + 0x80))
#define EP_DMA_STAT         (*(volatile unsigned int*)(USB_BASE_ADDR + 0x84))
#define EP_DMA_EN           (*(volatile unsigned int*)(USB_BASE_ADDR + 0x88))
#define EP_DMA_DIS          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x8C))
#define DMA_INT_STAT        (*(volatile unsigned int*)(USB_BASE_ADDR + 0x90))
#define DMA_INT_EN          (*(volatile unsigned int*)(USB_BASE_ADDR + 0x94))
#define EOT_INT_STAT        (*(volatile unsigned int*)(USB_BASE_ADDR + 0xA0))
#define EOT_INT_CLR         (*(volatile unsigned int*)(USB_BASE_ADDR + 0xA4))
#define EOT_INT_SET         (*(volatile unsigned int*)(USB_BASE_ADDR + 0xA8))
#define NDD_REQ_INT_STAT    (*(volatile unsigned int*)(USB_BASE_ADDR + 0xAC))
#define NDD_REQ_INT_CLR     (*(volatile unsigned int*)(USB_BASE_ADDR + 0xB0))
#define NDD_REQ_INT_SET     (*(volatile unsigned int*)(USB_BASE_ADDR + 0xB4))
#define SYS_ERR_INT_STAT    (*(volatile unsigned int*)(USB_BASE_ADDR + 0xB8))
#define SYS_ERR_INT_CLR     (*(volatile unsigned int*)(USB_BASE_ADDR + 0xBC))
#define SYS_ERR_INT_SET     (*(volatile unsigned int*)(USB_BASE_ADDR + 0xC0))
#define MODULE_ID           (*(volatile unsigned int*)(USB_BASE_ADDR + 0xFC))


/* Device Interrupt Bit Definitions */
#define FRAME_INT           0x00000001
#define EP_FAST_INT         0x00000002
#define EP_SLOW_INT         0x00000004
#define DEV_STAT_INT        0x00000008
#define CCEMTY_INT          0x00000010
#define CDFULL_INT          0x00000020
#define RxENDPKT_INT        0x00000040
#define TxENDPKT_INT        0x00000080
#define EP_RLZED_INT        0x00000100
#define ERR_INT             0x00000200

/* Rx & Tx Packet Length Definitions */
#define PKT_LNGTH_MASK      0x000003FF
#define PKT_DV              0x00000400
#define PKT_RDY             0x00000800

/* USB Control Definitions */
#define CTRL_RD_EN          0x00000001
#define CTRL_WR_EN          0x00000002

/* Command Codes */
#define CMD_SET_ADDR        0x00D00500
#define CMD_CFG_DEV         0x00D80500
#define CMD_SET_MODE        0x00F30500
#define CMD_RD_FRAME        0x00F50500
#define DAT_RD_FRAME        0x00F50200
#define CMD_RD_TEST         0x00FD0500
#define DAT_RD_TEST         0x00FD0200
#define CMD_SET_DEV_STAT    0x00FE0500
#define CMD_GET_DEV_STAT    0x00FE0500
#define DAT_GET_DEV_STAT    0x00FE0200
#define CMD_GET_ERR_CODE    0x00FF0500
#define DAT_GET_ERR_CODE    0x00FF0200
#define CMD_RD_ERR_STAT     0x00FB0500
#define DAT_RD_ERR_STAT     0x00FB0200
#define DAT_WR_BYTE(x)     (0x00000100 | ((x) << 16))
#define CMD_SEL_EP(x)      (0x00000500 | ((x) << 16))
#define DAT_SEL_EP(x)      (0x00000200 | ((x) << 16))
#define CMD_SEL_EP_CLRI(x) (0x00400500 | ((x) << 16))
#define DAT_SEL_EP_CLRI(x) (0x00400200 | ((x) << 16))
#define CMD_SET_EP_STAT(x) (0x00400500 | ((x) << 16))
#define CMD_CLR_BUF         0x00F20500
#define DAT_CLR_BUF         0x00F20200
#define CMD_VALID_BUF       0x00FA0500

/* Device Address Register Definitions */
#define DEV_ADDR_MASK       0x7F
#define DEV_EN              0x80

/* Device Configure Register Definitions */
#define CONF_DVICE          0x01

/* Device Mode Register Definitions */
#define AP_CLK              0x01
#define INAK_CI             0x02
#define INAK_CO             0x04
#define INAK_II             0x08
#define INAK_IO             0x10
#define INAK_BI             0x20
#define INAK_BO             0x40

/* Device Status Register Definitions */
#define DEV_CON             0x01
#define DEV_CON_CH          0x02
#define DEV_SUS             0x04
#define DEV_SUS_CH          0x08
#define DEV_RST             0x10

/* Error Code Register Definitions */
#define ERR_EC_MASK         0x0F
#define ERR_EA              0x10

/* Error Status Register Definitions */
#define ERR_PID             0x01
#define ERR_UEPKT           0x02
#define ERR_DCRC            0x04
#define ERR_TIMOUT          0x08
#define ERR_EOP             0x10
#define ERR_B_OVRN          0x20
#define ERR_BTSTF           0x40
#define ERR_TGL             0x80

/* Endpoint Select Register Definitions */
#define EP_SEL_F            0x01
#define EP_SEL_ST           0x02
#define EP_SEL_STP          0x04
#define EP_SEL_PO           0x08
#define EP_SEL_EPN          0x10
#define EP_SEL_B_1_FULL     0x20
#define EP_SEL_B_2_FULL     0x40

/* Endpoint Status Register Definitions */
#define EP_STAT_ST          0x01
#define EP_STAT_DA          0x20
#define EP_STAT_RF_MO       0x40
#define EP_STAT_CND_ST      0x80

/* Clear Buffer Register Definitions */
#define CLR_BUF_PO          0x01


/* DMA Interrupt Bit Definitions */
#define EOT_INT             0x01
#define NDD_REQ_INT         0x02
#define SYS_ERR_INT         0x04


#endif  /* __USBREG_H */
