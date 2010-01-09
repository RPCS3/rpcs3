/*  dev9null
 *  Copyright (C) 2002-2010 pcsx2 Team
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

#ifndef __DEV9_H__
#define __DEV9_H__

#include <stdio.h>

#define DEV9defs
#include "PS2Edefs.h"
#include "PS2Eext.h"

typedef struct {
  s32 Log;
} Config;

extern Config conf;

extern const unsigned char version;
extern const unsigned char revision;
extern const unsigned char build;
extern const unsigned int minor;
extern const char *libraryName;

void SaveConfig();
void LoadConfig();

extern void (*DEV9irq)(int);

extern __aligned16 s8 dev9regs[0x10000];
#define dev9Rs8(mem)	dev9regs[(mem) & 0xffff]
#define dev9Rs16(mem)	(*(s16*)&dev9regs[(mem) & 0xffff])
#define dev9Rs32(mem)	(*(s32*)&dev9regs[(mem) & 0xffff])
#define dev9Ru8(mem)	(*(u8*) &dev9regs[(mem) & 0xffff])
#define dev9Ru16(mem)	(*(u16*)&dev9regs[(mem) & 0xffff])
#define dev9Ru32(mem)	(*(u32*)&dev9regs[(mem) & 0xffff])

#endif
