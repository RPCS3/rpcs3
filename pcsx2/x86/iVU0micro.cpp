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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "iVUzerorec.h"

#ifndef PCSX2_MICROVU_
namespace VU0micro
{
	void recAlloc() 
	{ 
		SuperVUAlloc(0); 
	}

	void __fastcall recClear(u32 Addr, u32 Size)
	{
		SuperVUClear(Addr, Size, 0); // Size should be a multiple of 8 bytes!
	}

	void recShutdown()
	{
		SuperVUDestroy( 0 );
	}

	static void recReset()
	{
		SuperVUReset(0);

		// this shouldn't be needed, but shouldn't hurt anything either.
		x86FpuState = FPU_STATE;
	}

	static void recStep()
	{
	}

	static void recExecuteBlock()
	{
		if((VU0.VI[REG_VPU_STAT].UL & 1) == 0)
			return;

		FreezeXMMRegs(1);
		SuperVUExecuteProgram(VU0.VI[ REG_TPC ].UL & 0xfff, 0);
		FreezeXMMRegs(0);
	}
}
#else

extern void initVUrec(VURegs* vuRegs, const int vuIndex);
extern void closeVUrec(const int vuIndex);
extern void resetVUrec(const int vuIndex);
extern void clearVUrec(u32 addr, u32 size, const int vuIndex);
extern void runVUrec(u32 startPC, u32 cycles, const int vuIndex);

namespace VU0micro
{
	void recAlloc()								 { initVUrec(&VU0, 0); }
	void __fastcall recClear(u32 Addr, u32 Size) { clearVUrec(Addr, Size, 0); }
	void recShutdown()							 { closeVUrec(0); }
	static void recReset()						 { resetVUrec(0); x86FpuState = FPU_STATE; }
	static void recStep()						 {}
	static void recExecuteBlock()
	{
		if ((VU0.VI[REG_VPU_STAT].UL & 1) == 0) return;

		FreezeXMMRegs(1);
		//FreezeMMXRegs(1);
		runVUrec(VU0.VI[REG_TPC].UL, 50000, 0);
		//FreezeMMXRegs(0);
		FreezeXMMRegs(0);
	}

}
#endif

using namespace VU0micro;

const VUmicroCpu recVU0 = 
{
	recReset
,	recStep
,	recExecuteBlock
,	recClear
};
