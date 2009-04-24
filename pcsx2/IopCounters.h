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

#ifndef __PSXCOUNTERS_H__
#define __PSXCOUNTERS_H__

struct psxCounter {
	u64 count, target;
    u32 mode;
	u32 rate, interrupt;
	u32 sCycleT;
	s32 CycleT;
};

#ifdef ENABLE_NEW_IOPDMA
#	define NUM_COUNTERS 9
#else
#	define NUM_COUNTERS 8
#endif

extern psxCounter psxCounters[NUM_COUNTERS];
extern s32 psxNextCounter;
extern u32 psxNextsCounter;

void psxRcntInit();
void psxRcntUpdate();
void cntspu2async();
void psxRcntWcount16(int index, u32 value);
void psxRcntWcount32(int index, u32 value);
void psxRcnt0Wmode(u32 value);
void psxRcnt1Wmode(u32 value);
void psxRcnt2Wmode(u32 value);
void psxRcnt3Wmode(u32 value);
void psxRcnt4Wmode(u32 value);
void psxRcnt5Wmode(u32 value);
void psxRcntWtarget16(int index, u32 value);
void psxRcntWtarget32(int index, u32 value);
u16  psxRcntRcount16(int index);
u32  psxRcntRcount32(int index);
u64  psxRcntCycles(int index);

void psxVBlankStart();
void psxVBlankEnd();
void psxCheckStartGate16(int i);
void psxCheckEndGate16(int i);
//static void psxCheckStartGate32(int i);
//static void psxCheckEndGate32(int i);

#endif /* __PSXCOUNTERS_H__ */
