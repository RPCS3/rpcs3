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

#include "Pcsx2Defs.h"

#include "System.h"
#include "Memory.h"
#include "Hw.h"
#include "Dmac.h"
#include "R5900.h"

#include "SaveState.h"
#include "DebugTools/Debug.h"

static const u32 BIAS = 2;   // Bus is half of the actual ps2 speed
static const u32 PS2CLK = 294912000; //hz	/* 294.912 mhz */
//#define PS2CLK   36864000	/* 294.912 mhz */

static const ConsoleColors ConColor_IOP = Color_Yellow;
static const ConsoleColors ConColor_EE = Color_Cyan;

extern wxString ShiftJIS_ConvertString( const char* src );
extern wxString ShiftJIS_ConvertString( const char* src, int maxlen );

// Some homeless externs.  This is as good a spot as any for now...

extern void SetCPUState(SSE_MXCSR sseMXCSR, SSE_MXCSR sseVUMXCSR);
extern SSE_MXCSR g_sseVUMXCSR, g_sseMXCSR;
