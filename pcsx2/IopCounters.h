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

extern void psxRcntInit();
extern void psxRcntUpdate();
extern void psxRcntWcount16(int index, u16 value);
extern void psxRcntWcount32(int index, u32 value);
extern void psxRcntWmode16(int index, u32 value);
extern void psxRcntWmode32(int index, u32 value);
extern void psxRcntWtarget16(int index, u32 value);
extern void psxRcntWtarget32(int index, u32 value);
extern u16  psxRcntRcount16(int index);
extern u32  psxRcntRcount32(int index);
extern u64  psxRcntCycles(int index);

extern void psxVBlankStart();
extern void psxVBlankEnd();
extern void psxCheckStartGate16(int i);
extern void psxCheckEndGate16(int i);

#endif /* __PSXCOUNTERS_H__ */
