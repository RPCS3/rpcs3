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


#include "PrecompiledHeader.h"

#include <time.h>

#include "IopCommon.h"
#include "Stats.h"

void statsOpen() {
	stats.vsyncCount = 0;
	stats.vsyncTime = time(NULL);
	stats.eeCycles = 0;
	stats.eeSCycle = 0;
	stats.iopCycles = 0;
	stats.iopSCycle = 0;
}

void statsClose() {
/*
	time_t t;
	FILE *f;

	t = time(NULL) - stats.vsyncTime;
#ifdef _WIN32
	f = fopen(LOGS_DIR "\\stats.txt", "w");
#else
	f = fopen(LOGS_DIR "/stats.txt", "w");
#endif
	if (!f) { Console::WriteLn("Can't open stats.txt"); return; }
	fprintf(f, "-- PCSX2 v%s statics--\n\n", PCSX2_VERSION);
	fprintf(f, "Ran for %d seconds\n", t);
	fprintf(f, "Total VSyncs: %d (%s)\n", stats.vsyncCount, Config.PsxType ? "PAL" : "NTSC");
	fprintf(f, "VSyncs per Seconds: %g\n", (double)stats.vsyncCount / t);
	fprintf(f, "Total EE  Instructions Executed: %lld\n", stats.eeCycles);
	fprintf(f, "Total IOP Instructions Executed: %lld\n", stats.iopCycles);
	if (!CHECK_EEREC) fprintf(f, "Interpreter Mode\n");
	else fprintf(f, "Recompiler Mode: VUrec1 %s, VUrec0 %s\n", 
		CHECK_VU1REC ? "Enabled" : "Disabled", CHECK_VU0REC ? "Enabled" : "Disabled");
	fclose(f);
*/
}

void statsVSync() {
	//static u64 accum = 0, accumvu1 = 0;
	//static u32 frame = 0;

	stats.eeCycles+= cpuRegs.cycle - stats.eeSCycle;
	stats.eeSCycle = cpuRegs.cycle;
	stats.iopCycles+= psxRegs.cycle - stats.iopSCycle;
	stats.iopSCycle = psxRegs.cycle;
	stats.vsyncCount++;
	stats.vif1count = 0;
	stats.vu1count = 0;
}
