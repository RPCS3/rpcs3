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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "USB.h"


const unsigned char version  = PS2E_USB_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 4;    // increase that with each version

static char *libraryName     = "USBlinuz Driver";

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_USB;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.Log) return;

	va_start(list, fmt);
	vfprintf(usbLog, fmt, list);
	va_end(list);
}

s32 CALLBACK USBinit() {
    LoadConfig();
#ifdef USB_LOG
	usbLog = fopen("logs/usbLog.txt", "w");
	setvbuf(usbLog, NULL,  _IONBF, 0);
	USB_LOG("usblinuz plugin version %d,%d\n",revision,build);
	USB_LOG("USBinit\n");
#endif


	usbR = (s8*)malloc(0x10000);
	if (usbR == NULL) {
		SysMessage("Error allocating memory"); return -1;
	}
	memset(usbR, 0, 0x10000);
	ohci = (struct ohci_regs*)usbR;
	ohci->revision = 0x10;
	ohci->roothub.a = 0x2;

	return 0;
}

void CALLBACK USBshutdown() {
	free(usbR);
#ifdef USB_LOG
	fclose(usbLog);
#endif
}

s32 CALLBACK USBopen(void *pDsp) {
#ifdef USB_LOG
	USB_LOG("USBopen\n");
#endif

	return 0;
}

void CALLBACK USBclose() {
}

u8   CALLBACK USBread8(u32 addr) {
	u8 hard;

	switch (addr) {
		default:
			hard = usbRu8(addr); 
#ifdef USB_LOG
			USB_LOG("*Unknown 8bit read at address %lx value %x\n", addr, hard);
#endif
			return hard;
	}

#ifdef USB_LOG
	USB_LOG("*Known 8bit read at address %lx value %x\n", addr, hard);
#endif
	return hard;
}

u16  CALLBACK USBread16(u32 addr) {
	u16 hard;

	switch (addr) {
		default:
			hard = usbRu16(addr); 
#ifdef USB_LOG
			USB_LOG("*Unknown 16bit read at address %lx value %x\n", addr, hard);
#endif
			return hard;
	}

#ifdef USB_LOG
	USB_LOG("*Known 16bit read at address %lx value %x\n", addr, hard);
#endif
	return hard;
}

u32  CALLBACK USBread32(u32 addr) {
	u32 hard;
	int i; 

	if (addr >= 0x1f801654 &&
		addr <  0x1f801690) {
		i = (addr - 0x1f801654) / 4;
		hard = ohci->roothub.portstatus[i];
#ifdef USB_LOG
		USB_LOG("ohci->roothub.portstatus[%d] read32 %x\n", i, hard);
#endif
		return hard;
	}

	switch (addr) {
#ifdef USB_LOG
		case 0x1f801600:
			hard = ohci->revision;
			USB_LOG("ohci->revision read32 %x\n", hard);
			return hard;
#endif

#ifdef USB_LOG
		case 0x1f801604:
			hard = ohci->control;
			USB_LOG("ohci->control read32 %x\n", hard);
			return hard;
#endif

		case 0x1f801608:
			hard = 0;
#ifdef USB_LOG
			USB_LOG("ohci->cmdstatus read32 %x\n", hard);
#endif
			return hard;

#ifdef USB_LOG
		case 0x1f80160c:
			hard = ohci->intrstatus;
			USB_LOG("ohci->intrstatus read32 %x\n", hard);
			return hard;

		case 0x1f801610:
			hard = ohci->intrenable;
			USB_LOG("ohci->intrenable read32 %x\n", hard);
			return hard;

		case 0x1f801648:
			hard = ohci->roothub.a;
			USB_LOG("ohci->roothub.a read32 %x\n", hard);
			return hard;

		case 0x1f80164C:
			hard = ohci->roothub.b;
			USB_LOG("ohci->roothub.b read32 %x\n", hard);
			return hard;

		case 0x1f801650:
			hard = ohci->roothub.status;
			USB_LOG("ohci->roothub.status read32 %x\n", hard);
			return hard;
#endif

		default:
			hard = usbRu32(addr); 
#ifdef USB_LOG
			USB_LOG("*Unkwnown 32bit read at address %lx: %lx\n", addr, hard);
#endif
			return hard;
	}

#ifdef USB_LOG
	USB_LOG("*Known 32bit read at address %lx: %lx\n", addr, hard);
#endif
	return hard;
}

void CALLBACK USBwrite8(u32 addr,  u8 value) {
	switch (addr) {
		default:
			usbRu8(addr) = value;
#ifdef USB_LOG
			USB_LOG("*Unknown 8bit write at address %lx value %x\n", addr, value);
#endif
			return;
	}
	usbRu8(addr) = value;
#ifdef USB_LOG
	USB_LOG("*Known 8bit write at address %lx value %x\n", addr, value);
#endif
}

void CALLBACK USBwrite16(u32 addr, u16 value) {
	switch (addr) {
		default:
			usbRu16(addr) = value;
#ifdef USB_LOG
			USB_LOG("*Unknown 16bit write at address %lx value %x\n", addr, value);
#endif
			return;
	}
	usbRu16(addr) = value;
#ifdef USB_LOG
	USB_LOG("*Known 16bit write at address %lx value %x\n", addr, value);
#endif
}

void CALLBACK USBwrite32(u32 addr, u32 value) {
	int i;

	if (addr >= 0x1f801654 &&
		addr <  0x1f801690) {
		i = (addr - 0x1f801654) / 4;
//		ohci->roothub.portstatus[i] = value;
			ohci->intrstatus|= OHCI_INTR_SF;
#ifdef USB_LOG
		USB_LOG("ohci->roothub.portstatus[%d] write32 %x\n", i, value);
#endif
		return;
	}

	switch (addr) {
		case 0x1f801600: // ohci->revision (read only)
			return;
		case 0x1f801604:
#ifdef USB_LOG
			USB_LOG("ohci->control write32 %x\n", value);
#endif
			ohci->control = value;
			if ((ohci->control & OHCI_CTRL_HCFS) == OHCI_USB_OPER) {
				USBirq(PSXCLK / 1000000);
				ohci->roothub.portstatus[0] = 0x1;
				ohci->intrstatus|= OHCI_INTR_RHSC;
			}
			return;

		case 0x1f80160c:
#ifdef USB_LOG
			USB_LOG("ohci->intrstatus write32 %x\n", value);
#endif
			ohci->intrstatus&= ~value;
			return;

		case 0x1f801610:
#ifdef USB_LOG
			USB_LOG("ohci->intrenable write32 %x\n", value);
#endif
			for (i=0; i<32; i++) {
				if (value & (1<<i)) {
					ohci->intrenable|= 1<<i;
				}
			}
			return;

		case 0x1f801614:
#ifdef USB_LOG
			USB_LOG("ohci->intrdisable write32 %x\n", value);
#endif
			for (i=0; i<32; i++) {
				if (value & (1<<i)) {
					ohci->intrenable&= ~(1<<i);
				}
			}
			return;

		case 0x1f801618:
			ohci->hcca = value;
			hcca = (struct ohci_hcca*)&ram[ohci->hcca];
			ohci->intrstatus|= OHCI_INTR_SF;
#ifdef USB_LOG
			USB_LOG("ohci->hcca write32 %x\n", value);
#endif
			return;

#ifdef USB_LOG
		case 0x1f801608:
			ohci->cmdstatus = value;
			USB_LOG("ohci->cmdstatus write32 %x\n", value);
			return;

		case 0x1f801630:
			ohci->donehead = value;
			USB_LOG("ohci->donehead write32 %x\n", value);
			return;

		case 0x1f801634:
			ohci->fminterval = value;
			USB_LOG("ohci->fminterval write32 %x\n", value);
			return;

		case 0x1f801640:
			ohci->periodicstart = value;
			USB_LOG("ohci->periodicstart write32 %x\n", value);
			return;

		case 0x1f801644:
			ohci->lsthresh = value;
			USB_LOG("ohci->lsthresh write32 %x\n", value);
			return;

		case 0x1f801648:
			ohci->roothub.a = value;
			USB_LOG("ohci->roothub.a write32 %x\n", value);
			return;

		case 0x1f80164C:
			ohci->roothub.b = value;
			USB_LOG("ohci->roothub.b write32 %x\n", value);
			return;

		case 0x1f801650:
			ohci->roothub.status = value;
			USB_LOG("ohci->roothub.status write32 %x\n", value);
			return;
#endif
		default:
			usbRu32(addr) = value;
#ifdef USB_LOG
			USB_LOG("*Unknown 32bit write at address %lx write %x\n", addr, value);
#endif
			return;
	}
	usbRu32(addr) = value;
#ifdef USB_LOG
	USB_LOG("*Known 32bit write at address %lx value %lx\n", addr, value);
#endif
}

void CALLBACK USBirqCallback(USBcallback callback) {
	USBirq = callback;
}

int CALLBACK _USBirqHandler(void) {
	if ((ohci->control & OHCI_CTRL_HCFS) == OHCI_USB_OPER) {
		USBirq(PSXCLK / 1000000);
	}

#ifdef USB_LOG
//	USB_LOG("_USBirqHandler: %x\n", ohci->intrenable);
#endif

	if (ohci->fmnumber == 0xffff) {
		ohci->fmnumber = 0;
		ohci->intrstatus|= OHCI_INTR_FNO;
	} else {
		ohci->fmnumber++;
	}

	if (ohci->intrenable & OHCI_INTR_MIE &&
		ohci->intrenable & OHCI_INTR_SF &&
		ohci->intrstatus & OHCI_INTR_SF) {
#ifdef USB_LOG
		USB_LOG("_USBirqHandler: SOF\n");
#endif

		return 1;
	}

	if (ohci->intrenable & OHCI_INTR_MIE &&
		ohci->intrenable & OHCI_INTR_FNO &&
		ohci->intrstatus & OHCI_INTR_FNO) {
#ifdef USB_LOG
		USB_LOG("_USBirqHandler: FNO\n");
#endif

		return 1;
	}

	return 0;
}

USBhandler CALLBACK USBirqHandler(void) {
	return (USBhandler)_USBirqHandler;
}

void CALLBACK USBsetRAM(void *mem) {
	ram = mem;
}

// extended funcs

char USBfreezeID[] = "USB STv0";

typedef struct {
	u8 id[32];
	u8 usbR[0x10000];
	usbStruct usb;
} USBfreezeData;

s32 CALLBACK USBfreeze(int mode, freezeData *data) {
	USBfreezeData *usbd;

	if (mode == FREEZE_LOAD) {
		usbd = (USBfreezeData*)data->data;
		if (data->size != sizeof(USBfreezeData)) return -1;
		if (strcmp(usbd->id, USBfreezeID)) return -1;
		memcpy(usbR, usbd->usbR, 0x10000);
		memcpy(&usb, &usbd->usb, sizeof(usbStruct));
	} else
	if (mode == FREEZE_SAVE) {
		data->size = sizeof(USBfreezeData);
		data->data = malloc(data->size);
		if (data->data == NULL) return -1;
		usbd = (USBfreezeData*)data->data;
		strcpy(usbd->id, USBfreezeID);
		memcpy(usbd->usbR, usbR, 0x10000);
		memcpy(&usbd->usb, &usb, sizeof(usbStruct));
	}

	return 0;
}


s32  CALLBACK USBtest() {
	return 0;
}

