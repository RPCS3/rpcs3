/*  Cpudetection lib
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

#include "Utilities/RedtapeWindows.h"
#include "x86emitter/tools.h"

// --------------------------------------------------------------------------------------
//  Thread Affinity Lock
// --------------------------------------------------------------------------------------
// Assign a single CPU/core for this thread's affinity to ensure rdtsc() accuracy.
// (rdtsc for each CPU/core can differ, causing skewed results)

class SingleCoreAffinity
{
protected:

#ifdef _WINDOWS_
	HANDLE	s_threadId;
	DWORD	s_oldmask;
#endif

public:
	SingleCoreAffinity();
	virtual ~SingleCoreAffinity() throw();
};

// --------------------------------------------------------------------------------------
//  SIMD "Manual" Detection, using Invalid Instruction Checks
// --------------------------------------------------------------------------------------
//
// Note: This API doesn't support GCC/Linux.  Looking online it seems the only
// way to simulate the Microsoft SEH model is to use unix signals, and the 'sigaction'
// function specifically.  A linux coder could implement this using sigaction at a later
// date, however its not really a big deal:  CPUID should be 99-100% accurate, as no modern
// software would work on the CPU if it mis-reported capabilities.  However there are known
// cases of a CPU failing to report supporting instruction sets it does in fact support.
// This secondary test fixes such cases (although apparently a CMOS reset does as well).
//

extern bool CanEmitShit();
extern bool CanTestInstructionSets();
extern bool _test_instruction( void* pfnCall );
