/*  FWnull
 *  Copyright (C) 2004-20010  PCSX2 Dev Team
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

#ifndef __FW_H__
#define __FW_H__

#include <stdio.h>

#define FWdefs
#include "PS2Edefs.h"
#include "PS2Eext.h"

// Our main memory storage, and defines for accessing it.
extern s8 *fwregs;
#define fwRs32(mem)	(*(s32*)&fwregs[(mem) & 0xffff])
#define fwRu32(mem)	(*(u32*)&fwregs[(mem) & 0xffff])

//PHY Access Address for ease of use :P
#define PHYACC fwRu32(0x8414)

typedef struct
{
	s32 Log;
} Config;

extern Config conf;

extern void (*FWirq)();

extern void SaveConfig();
extern void LoadConfig();
extern void setLoggingState();

#endif
