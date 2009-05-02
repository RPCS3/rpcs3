/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#ifndef __COMMON_H__
#define __COMMON_H__

#include "Pcsx2Defs.h"

#define BIAS 2   // Bus is half of the actual ps2 speed
//#define PS2CLK   36864000	/* 294.912 mhz */
//#define PSXCLK	 9216000	/* 36.864 Mhz */
//#define PSXCLK	186864000	/* 36.864 Mhz */
#define PS2CLK 294912000 //hz	/* 294.912 mhz */

#define PCSX2_VERSION "(beta)"

#include "System.h"

#include "Plugins.h"
#include "SaveState.h"

#include "DebugTools/Debug.h"
#include "Memory.h"
#include "Hw.h"

#include "R5900.h"
#include "Elfheader.h"
#include "Patch.h"

#endif /* __COMMON_H__ */
