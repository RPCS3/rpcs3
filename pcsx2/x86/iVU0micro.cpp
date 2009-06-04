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

#define useMVU0 CHECK_MICROVU0

namespace VU0micro
{
	void recAlloc()		{ SuperVUAlloc(0);   initVUrec(&VU0, 0); }
	void recShutdown()	{ SuperVUDestroy(0); closeVUrec(0); }

	void __fastcall recClear(u32 Addr, u32 Size) { 
		if (useMVU0) clearVUrec(Addr, Size, 0); 
		else		 SuperVUClear(Addr, Size, 0);
	}

	static void recReset() { 
		if (useMVU0) resetVUrec(0);
		else		 SuperVUReset(0);
		x86FpuState = FPU_STATE; 
	}

	static void recStep() {}
	static void recExecuteBlock() 
	{
		if ((VU0.VI[REG_VPU_STAT].UL & 1) == 0) return;

		FreezeXMMRegs(1);
		if (useMVU0) runVUrec(VU0.VI[REG_TPC].UL, 0x300, 0);
		else		 SuperVUExecuteProgram(VU0.VI[REG_TPC].UL & 0xfff, 0);
		FreezeXMMRegs(0);
	}
}

using namespace VU0micro;

const VUmicroCpu recVU0 = 
{
		recReset
	,	recStep
	,	recExecuteBlock
	,	recClear
};
