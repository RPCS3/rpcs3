/*  USBnull
 *  Copyright (C) 2002-2009  pcsx2 Team
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
#include "PS2Eext.h"

typedef struct {
  int Log;
} Config;

extern USBcallback USBirq;
extern Config conf;

// Previous USB plugins have needed this in ohci.
static const s64 PSXCLK = 36864000;	/* 36.864 Mhz */

extern s8 *usbregs, *ram;

#define usbRs8(mem)		usbregs[(mem) & 0xffff]
#define usbRs16(mem)	(*(s16*)&usbregs[(mem) & 0xffff])
#define usbRs32(mem)	(*(s32*)&usbregs[(mem) & 0xffff])
#define usbRu8(mem)		(*(u8*) &usbregs[(mem) & 0xffff])
#define usbRu16(mem)	(*(u16*)&usbregs[(mem) & 0xffff])
#define usbRu32(mem)	(*(u32*)&usbregs[(mem) & 0xffff])

extern void SaveConfig();
extern void LoadConfig();
extern void setLoggingState();

#endif
