/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
 
#pragma once

#include "R3000A.h"
#include "Common.h"

#include "CDVD/CDVD.h"
#include "CDVD/CdRom.h"

#include "IopDma.h"
#include "IopMem.h"
#include "IopHw.h"
#include "IopBios.h"
#include "IopCounters.h"
#include "IopSio2.h"

static const s64 PSXCLK = 36864000;	/* 36.864 Mhz */
//#define PSXCLK	 9216000	/* 36.864 Mhz */
//#define PSXCLK	186864000	/* 36.864 Mhz */

// Uncomment to make pcsx2 print each spu2 interrupt it receives
//#define SPU2IRQTEST

