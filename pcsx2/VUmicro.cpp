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
#include "Common.h"
#include "VUmicro.h"

#define useDeltaTime 1

#if useDeltaTime

// Executes a Block based on EE delta time
void BaseVUmicroCPU::ExecuteBlock(bool startUp) {
	const u32& stat	= VU0.VI[REG_VPU_STAT].UL;
	const int  test = m_Idx ? 0x100 : 1;
	const int  s = 1024*6; // Kick Start Cycles (Silver Surfer needs this amount)
	const int  c = 1024*1; // Continue Cycles
	if (!(stat & test)) return;
	if (startUp) {  // Start Executing a microprogram
		Execute(s); // Kick start VU

		// Let VUs run behind EE instead of ahead
		if (stat & test) {
			cpuSetNextBranchDelta((s+c)*2);
			m_lastEEcycles = cpuRegs.cycle + (s*2);
		}
	}
	else { // Continue Executing (VU roughly half the mhz of EE)
		s32 delta = (s32)(u32)(cpuRegs.cycle - m_lastEEcycles) & ~1;
		if (delta >   0) {	// Enough time has passed
			delta >>= 1;	// Divide by 2 (unsigned)
			Execute(delta);	// Execute the time since the last call
			if (stat & test) {
				cpuSetNextBranchDelta(c*2);
				m_lastEEcycles = cpuRegs.cycle;
			}
		}
		else cpuSetNextBranchDelta(-delta); // Haven't caught-up from kick start
	}
}

// This function is called by VU0 Macro (COP2) after transferring some
// EE data to VU0's registers. We want to run VU0 Micro right after this
// to ensure that the register is used at the correct time.
// This fixes spinning/hanging in some games like Ratchet and Clank's Intro.
void __fastcall BaseVUmicroCPU::ExecuteBlockJIT(BaseVUmicroCPU* cpu) {
	const u32& stat	= VU0.VI[REG_VPU_STAT].UL;
	const int  test = cpu->m_Idx ? 0x100 : 1;
	const int  c	= 128;	// VU Execution Cycles
	if (stat & test) {		// VU is running
		cpu->Execute(c);	// Execute VU
		if (stat & test) {
			cpu->m_lastEEcycles+=(c*2);
			cpuSetNextBranchDelta(c*2);
		}
	}
}

#else

// Executes a Block based on static preset cycles
void BaseVUmicroCPU::ExecuteBlock(bool startUp) {
	const int vuRunning = m_Idx ? 0x100 : 1;
	if (!(VU0.VI[REG_VPU_STAT].UL & vuRunning)) return;
	if (startUp) { // Start Executing a microprogram
		Execute(vu0RunCycles); // Kick start VU

		// If the VU0 program didn't finish then we'll want to finish it up
		// pretty soon.  This fixes vmhacks in some games (Naruto Ultimate Ninja 2)
		if(VU0.VI[REG_VPU_STAT].UL & vuRunning)
			cpuSetNextBranchDelta( 192 ); // fixme : ideally this should be higher, like 512 or so.
	}
	else {
		Execute(vu0RunCycles);
		//DevCon.Warning("VU0 running when in BranchTest");

		// This helps keep the EE and VU0 in sync.
		// Check Silver Surfer. Currently has SPS varying with different branch deltas set below.
		if(VU0.VI[REG_VPU_STAT].UL & vuRunning)
			cpuSetNextBranchDelta( 768 );
	}
}

// This function is called by VU0 Macro (COP2) after transferring
// EE data to a VU0 register...
void __fastcall BaseVUmicroCPU::ExecuteBlockJIT(BaseVUmicroCPU* cpu) {
	cpu->Execute(128);
}

#endif
