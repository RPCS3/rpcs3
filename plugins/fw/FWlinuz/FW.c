/*  FW 
 *  Copyright (C) 2004 PCSX2 Team
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

#include "FW.h"

const unsigned char version  = PS2E_FW_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 1;    // increase that with each version

static char *libraryName     = "FWlinuz Driver";

s8 *fwregs;
u8 phyregs[8];
u8 portregs[3][8];
u8 vidregs[8];
u8 vderegs[8];

#define fwRs32(mem)	(*(s32*)&fwregs[(mem) & 0xffff])
#define fwRu32(mem)	(*(u32*)&fwregs[(mem) & 0xffff])


u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_FW;
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
	vfprintf(fwLog, fmt, list);
	va_end(list);
}

s32 CALLBACK FWinit() {
    LoadConfig();
#ifdef FW_LOG
	fwLog = fopen("logs/fwLog.txt", "w");
	setvbuf(fwLog, NULL,  _IONBF, 0);
	FW_LOG("FWlinuz plugin version %d,%d\n",revision,build);
	FW_LOG("FW init\n");
#endif

	fwregs = (s8*)malloc(0x10000);
	if (fwregs == NULL) {
		SysMessage("Error allocating Memory\n"); return -1;
	}
	memset(phyregs, 0, sizeof(phyregs));
	memset(portregs, 0, sizeof(portregs));
	memset(vidregs, 0, sizeof(vidregs));
	memset(vderegs, 0, sizeof(vderegs));
	fwRu32(0x8400) = 0xffc00001;
	phyregs[0] = 0xc1;
	portregs[0][0] = 0xe5;
	vidregs[0] = 0x01;

	return 0;
}

void CALLBACK FWshutdown() {
	free(fwregs);

#ifdef FW_LOG
	fclose(fwLog);
#endif
}

s32 CALLBACK FWopen(void *pDsp) {
#ifdef FW_LOG
	FW_LOG("FW open\n");
#endif

	return 0;
}

void CALLBACK FWclose() {
}

u32  CALLBACK FWread32(u32 addr) {
	u32 ret=0;

	switch (addr) {
		case 0x1f808410:
			ret = 0x8;
			break;

		default:
			ret = fwRu32(addr);
			break;
	}
#ifdef FW_LOG
	FW_LOG("FW read mem 0x%x: 0x%x\n", addr, ret);
#endif	

	return ret;
}

void _irq0(int num) {
	fwRu32(0x8420)|= 1<<num;
	if (fwRu32(0x8420) & fwRu32(0x8424)) {
		FWirq();
	}
}

void phy_read() {
	int reg = (PHYACC >> 24) & 0xf;
	int port;
	u8 value=0;

	PHYACC = (PHYACC & ~0xf00) | (reg << 8);
	PHYACC&= ~0x80000000;

	if (reg > 7) {
		reg-= 8;
		switch (phyregs[7] & 0x7) {
			case 0:
				port = phyregs[7] >> 16;
				if (port < 3) {
					value = portregs[port][reg];
				} else {
					value = 0;
				}
				printf("port[%d] read 0x%x: 0x%2.2x\n", port, reg, value);
#ifdef FW_LOG
				FW_LOG("port[%d] read 0x%x: 0x%2.2x\n", port, reg, value);
#endif	
				break;
			case 1:
				value = vidregs[reg];
				printf("vid  read 0x%x: 0x%2.2x\n", reg, value);
#ifdef FW_LOG
				FW_LOG("vid  read 0x%x: 0x%2.2x\n", reg, value);
#endif	
				break;
			case 3:
				value = 0x02;
				printf("vid  read 0x%x: 0x%2.2x\n", reg, value);
#ifdef FW_LOG
				FW_LOG("vid  read 0x%x: 0x%2.2x\n", reg, value);
#endif	
				break;
			case 7:
				printf("vde  read 0x%x: 0x%2.2x\n", reg, vderegs[reg]);
#ifdef FW_LOG
				FW_LOG("vde  read 0x%x: 0x%2.2x\n", reg, vderegs[reg]);
#endif	
				value = vderegs[reg];
				break;
		}
	} else {
		printf("phy  read 0x%x: 0x%2.2x\n", reg, phyregs[reg]);
#ifdef FW_LOG
		FW_LOG("phy  read 0x%x: 0x%2.2x\n", reg, phyregs[reg]);
#endif	
		value = phyregs[reg];
	}

	PHYACC = (PHYACC & ~0xff) | value;
	_irq0(30);
}

void phy_write() {
	int reg = (PHYACC >> 24) & 0xf;
	int port;
	u8 value = (PHYACC >> 16) & 0xff;

	if (reg > 7) {
		reg-= 8;
		switch (phyregs[7] & 0x7) {
			case 0:
				port = phyregs[7] >> 4;
				printf("port[%d] write 0x%x: 0x%2.2x\n", port, reg, value);
#ifdef FW_LOG
				FW_LOG("port[%d] write 0x%x: 0x%2.2x\n", port, reg, value);
#endif	
				if (port < 3) {
					switch (reg) {
						case 0x0: portregs[port][reg] = (portregs[port][reg] & 0x7f) | (value & ~0x7f); break;
						case 0x1: portregs[port][reg] = (portregs[port][reg] & 0x07) | (value & ~0x07); break;
					}
				}
				break;

			case 1:
				printf("vid  write 0x%x: 0x%2.2x\n", reg, value);
#ifdef FW_LOG
				FW_LOG("vid  write 0x%x: 0x%2.2x\n", reg, value);
#endif	
				break;

			case 7:
				printf("vde  write 0x%x: 0x%2.2x\n", reg, value);
#ifdef FW_LOG
				FW_LOG("vde  write 0x%x: 0x%2.2x\n", reg, value);
#endif	
				switch (reg) {
					case 0x0: vderegs[reg] = value; break;
				}
				break;
		}
	} else {
		printf("phy write 0x%x: 0x%2.2x\n", reg, value);
#ifdef FW_LOG
		FW_LOG("phy write 0x%x: 0x%2.2x\n", reg, value);
#endif	
		switch (reg) {
			case 0x0: break;
			case 0x2: break;
			case 0x3: break;
			case 0x4: phyregs[reg] = (phyregs[reg] & 0x1c) | (value & ~0x1c); break;
			case 0x6: break;
			default: phyregs[reg] = value;
		}
	}

	PHYACC&= ~0x40000000;
}

void CALLBACK FWwrite32(u32 addr, u32 value) {
	switch (addr) {
		case 0x1f808400:
			fwRu32(addr) = (fwRu32(addr) & 0x003f0001) | (value & ~0x003f0001);
			break;

		case 0x1f808414:
			fwRu32(addr) = (fwRu32(addr) & 0xfff) | (value & ~0xfff);
			if (value & 0x80000000) phy_read();
			if (value & 0x40000000) phy_write();
			break;

		case 0x1f808420:
		case 0x1f808428:
		case 0x1f808430:
			fwRu32(addr) = (fwRu32(addr) & ~value);
			break;

		default:
			fwRu32(addr) = value;
			break;
	}
#ifdef FW_LOG
	FW_LOG("FW write mem 0x%x: 0x%x\n", addr, value);
#endif
}

void CALLBACK FWirqCallback(void (*callback)()) {
	FWirq = callback;
}

s32 CALLBACK FWfreeze(int mode, freezeData *data) {
	return 0;
}

s32  CALLBACK FWtest() {
	return 0;
}

