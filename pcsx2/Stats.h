/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#ifndef __STATS_H__
#define __STATS_H__

#include <time.h>

struct Stats {
	time_t vsyncTime;
	u32 vsyncCount;
	u32 eeCycles;
	u32 eeSCycle;
	u32 iopCycles;
	u32 iopSCycle;

	u32 ticko;
	u32 framecount;
	u32 vu1count;
	u32 vif1count;
};

Stats stats;

void statsOpen();
void statsClose();
void statsVSync();

#endif /* __STATS_H__ */
