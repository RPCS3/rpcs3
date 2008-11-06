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

#ifndef __COUNTERS_H__
#define __COUNTERS_H__

typedef struct {
	u32 count, mode, target, hold;
	u32 rate, interrupt;
	u32 Cycle, sCycle;
	u32 CycleT, sCycleT;
} Counter;

extern Counter counters[6];
extern u32 nextCounter, nextsCounter;

void rcntInit();
void rcntUpdate();
void rcntStartGate(int mode);
void rcntEndGate(int mode);
void rcntWcount(int index, u32 value);
void rcntWmode(int index, u32 value);
void rcntWtarget(int index, u32 value);
void rcntWhold(int index, u32 value);
u16  rcntRcount(int index);
u32 rcntCycle(int index);
int  rcntFreeze(gzFile f, int Mode);

void UpdateVSyncRate();

#endif /* __COUNTERS_H__ */
