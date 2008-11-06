/*  USBlinuz
 *  Copyright (C) 2002-2004  USBlinuz Team
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


#ifndef __USB_H__
#define __USB_H__

#include <stdio.h>

#define USBdefs
#include "PS2Edefs.h"

#ifdef __WIN32__

#define usleep(x)	Sleep(x / 1000)
#include <windows.h>
#include <windowsx.h>

#else

#include <gtk/gtk.h>

#define __inline inline

#endif

#define USB_LOG __Log

typedef struct {
  int Log;
} Config;

Config conf;

u8 *usbR;
u8 *ram;

typedef struct {
	int unused;
} usbStruct;

usbStruct usb;

#define usbRs8(mem)		usbR[(mem) & 0xffff]
#define usbRs16(mem)	(*(s16*)&usbR[(mem) & 0xffff])
#define usbRs32(mem)	(*(s32*)&usbR[(mem) & 0xffff])
#define usbRu8(mem)		(*(u8*) &usbR[(mem) & 0xffff])
#define usbRu16(mem)	(*(u16*)&usbR[(mem) & 0xffff])
#define usbRu32(mem)	(*(u32*)&usbR[(mem) & 0xffff])


/*
 * URB OHCI HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2001 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * usb-ohci.h
 */

/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined in the OHCI spec. that the host controller is
 * told the base address of.  It must be 256-byte aligned.
 */
 

#define NUM_INTS 32	/* part of the OHCI standard */
struct ohci_hcca {
	u32	int_table[NUM_INTS];	/* Interrupt ED table */
	u16	frame_no;		/* current frame number */
	u16	pad1;			/* set to 0 on each frame_no change */
	u32	done_head;		/* info returned for an interrupt */
	u8	reserved_for_hc[116];
};

/*
 * Maximum number of root hub ports.  
 */
#define MAX_ROOT_PORTS	15	/* maximum OHCI root hub ports */

/*
 * This is the structure of the OHCI controller's memory mapped I/O
 * region.  This is Memory Mapped I/O.  You must use the readl() and
 * writel() macros defined in asm/io.h to access these!!
 */
struct ohci_regs {
	/* control and status registers */
	u32	revision;
	u32	control;
	u32	cmdstatus;
	u32	intrstatus;
	u32	intrenable;
	u32	intrdisable;
	/* memory pointers */
	u32	hcca;
	u32	ed_periodcurrent;
	u32	ed_controlhead;
	u32	ed_controlcurrent;
	u32	ed_bulkhead;
	u32	ed_bulkcurrent;
	u32	donehead;
	/* frame counters */
	u32	fminterval;
	u32	fmremaining;
	u32	fmnumber;
	u32	periodicstart;
	u32	lsthresh;
	/* Root hub ports */
	struct	ohci_roothub_regs {
		u32	a;
		u32	b;
		u32	status;
		u32	portstatus[MAX_ROOT_PORTS];
	} roothub;
};


/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * HcControl (control) register masks
 */
#define OHCI_CTRL_CBSR	(3 << 0)	/* control/bulk service ratio */
#define OHCI_CTRL_PLE	(1 << 2)	/* periodic list enable */
#define OHCI_CTRL_IE	(1 << 3)	/* isochronous enable */
#define OHCI_CTRL_CLE	(1 << 4)	/* control list enable */
#define OHCI_CTRL_BLE	(1 << 5)	/* bulk list enable */
#define OHCI_CTRL_HCFS	(3 << 6)	/* host controller functional state */
#define OHCI_CTRL_IR	(1 << 8)	/* interrupt routing */
#define OHCI_CTRL_RWC	(1 << 9)	/* remote wakeup connected */
#define OHCI_CTRL_RWE	(1 << 10)	/* remote wakeup enable */

/* pre-shifted values for HCFS */
#	define OHCI_USB_RESET	(0 << 6)
#	define OHCI_USB_RESUME	(1 << 6)
#	define OHCI_USB_OPER	(2 << 6)
#	define OHCI_USB_SUSPEND	(3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR	(1 << 0)	/* host controller reset */
#define OHCI_CLF  	(1 << 1)	/* control list filled */
#define OHCI_BLF  	(1 << 2)	/* bulk list filled */
#define OHCI_OCR  	(1 << 3)	/* ownership change request */
#define OHCI_SOC  	(3 << 16)	/* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define OHCI_INTR_SO	(1 << 0)	/* scheduling overrun */
#define OHCI_INTR_WDH	(1 << 1)	/* writeback of done_head */
#define OHCI_INTR_SF	(1 << 2)	/* start frame */
#define OHCI_INTR_RD	(1 << 3)	/* resume detect */
#define OHCI_INTR_UE	(1 << 4)	/* unrecoverable error */
#define OHCI_INTR_FNO	(1 << 5)	/* frame number overflow */
#define OHCI_INTR_RHSC	(1 << 6)	/* root hub status change */
#define OHCI_INTR_OC	(1 << 30)	/* ownership change */
#define OHCI_INTR_MIE	(1 << 31)	/* master interrupt enable */

/**********************/

struct ohci_hcca *hcca;
struct ohci_regs *ohci;

#define PSXCLK	36864000	/* 36.864 Mhz */

USBcallback USBirq;

void SaveConfig();
void LoadConfig();

FILE *usbLog;
void __Log(char *fmt, ...);

void SysMessage(char *fmt, ...);

#endif
