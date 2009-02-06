/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "PS2Etypes.h"

struct TESTRUNARGS
{
	u8 enabled;
	u8 jpgcapture;

	int frame; // if < 0, frame is unlimited (run until crash).
	int numimages;
	int curimage;
	u32 autopad; // mask for auto buttons
	bool efile;
	int snapdone;

	const char* ptitle;
	const char* pimagename;
	const char* plogname;
	const char* pgsdll, *pcdvddll, *pspudll;

};

extern TESTRUNARGS g_TestRun;

#define BIAS 2   // Bus is half of the actual ps2 speed
//#define PS2CLK   36864000	/* 294.912 mhz */
//#define PSXCLK	 9216000	/* 36.864 Mhz */
//#define PSXCLK	186864000	/* 36.864 Mhz */
#define PS2CLK 294912000 //hz	/* 294.912 mhz */


/* Config.PsxType == 1: PAL:
	 VBlank interlaced		50.00 Hz
	 VBlank non-interlaced	49.76 Hz
	 HBlank					15.625 KHz 
   Config.PsxType == 0: NSTC
	 VBlank interlaced		59.94 Hz
	 VBlank non-interlaced	59.82 Hz
	 HBlank					15.73426573 KHz */

//Misc Clocks
#define PSXPIXEL        ((int)(PSXCLK / 13500000))
#define PSXSOUNDCLK		((int)(48000))

#include "Plugins.h"
#include "Misc.h"
#include "SaveState.h"
#include "DebugTools/Debug.h"
#include "R5900.h"
#include "Memory.h"
#include "Elfheader.h"
#include "Hw.h"
// Moving this before one of the other includes causes compilation issues. 
//#include "Misc.h"
#include "Patch.h"

#define PCSX2_VERSION "(beta)"

#endif /* __COMMON_H__ */
