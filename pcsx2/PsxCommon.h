/*  Pcsx2 - Pc Ps2 Emulator *  Copyright (C) 2002-2008  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__

#include "PS2Etypes.h"

#include <assert.h>

#include "System.h"

extern long LoadCdBios;
extern int cdOpenCase;

#define PSXCLK	(36864000ULL)	/* 36.864 Mhz */

#include "Plugins.h"
#include "Misc.h"
#include "SaveState.h"

#include "R3000A.h"
#include "IopMem.h"
#include "IopHw.h"
#include "IopBios.h"
#include "IopDma.h"
#include "IopCounters.h"
#include "CdRom.h"
#include "Sio.h"
#include "DebugTools/Debug.h"
#include "IopSio2.h"
#include "CDVD.h"
#include "Memory.h"
#include "Hw.h"
#include "Sif.h"

#endif /* __PSXCOMMON_H__ */
